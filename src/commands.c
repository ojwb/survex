/* commands.c
 * Code for directives
 * Copyright (C) 1991-2024 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <config.h>

#include <limits.h>
#include <stddef.h> /* for offsetof */
#include <string.h>

#include <proj.h>
#if PROJ_VERSION_MAJOR < 8
# define proj_context_errno_string(CTX, ERR) proj_errno_string(ERR)
#endif

#include "cavern.h"
#include "commands.h"
#include "datain.h"
#include "date.h"
#include "debug.h"
#include "filename.h"
#include "message.h"
#include "netbits.h"
#include "netskel.h"
#include "out.h"
#include "readval.h"
#include "str.h"

#define WGS84_DATUM_STRING "EPSG:4326"

int fix_station(prefix *fix_name, const double* coords) {
    fix_name->sflags |= BIT(SFLAGS_FIXED);
    node *stn = StnFromPfx(fix_name);
    if (fixed(stn)) {
	if (coords[0] != POS(stn, 0) ||
	    coords[1] != POS(stn, 1) ||
	    coords[2] != POS(stn, 2)) {
	    return -1;
	}
	return 1;
    }

    POS(stn, 0) = coords[0];
    POS(stn, 1) = coords[1];
    POS(stn, 2) = coords[2];

    // Make the station's file:line location reflect where it was fixed.
    fix_name->filename = file.filename;
    fix_name->line = file.line;
    return 0;
}

void fix_station_with_variance(prefix *fix_name, const double* coords,
			       real var_x, real var_y, real var_z,
#ifndef NO_COVARIANCES
			       real cxy, real cyz, real czx
#endif
			      )
{
    node *stn = StnFromPfx(fix_name);
    if (!fixed(stn)) {
	node *fixpt = osnew(node);
	prefix *name;
	name = osnew(prefix);
	name->pos = osnew(pos);
	name->ident.p = NULL;
	name->shape = 0;
	fixpt->name = name;
	name->stn = fixpt;
	name->up = NULL;
	if (TSTBIT(pcs->infer, INFER_EXPORTS)) {
	    name->min_export = USHRT_MAX;
	} else {
	    name->min_export = 0;
	}
	name->max_export = 0;
	name->sflags = 0;
	add_stn_to_list(&stnlist, fixpt);
	POS(fixpt, 0) = coords[0];
	POS(fixpt, 1) = coords[1];
	POS(fixpt, 2) = coords[2];
	fixpt->leg[0] = fixpt->leg[1] = fixpt->leg[2] = NULL;
	addfakeleg(fixpt, stn, 0, 0, 0,
		   var_x, var_y, var_z
#ifndef NO_COVARIANCES
		   , cxy, cyz, czx
#endif
		  );
    }
}

static void
default_grade(settings *s)
{
   /* Values correspond to those in bcra5.svx */
   s->Var[Q_POS] = (real)sqrd(0.05);
   s->Var[Q_LENGTH] = (real)sqrd(0.05);
   s->Var[Q_BACKLENGTH] = (real)sqrd(0.05);
   s->Var[Q_COUNT] = (real)sqrd(0.05);
   s->Var[Q_DX] = s->Var[Q_DY] = s->Var[Q_DZ] = (real)sqrd(0.05);
   s->Var[Q_BEARING] = (real)sqrd(rad(0.5));
   s->Var[Q_GRADIENT] = (real)sqrd(rad(0.5));
   s->Var[Q_BACKBEARING] = (real)sqrd(rad(0.5));
   s->Var[Q_BACKGRADIENT] = (real)sqrd(rad(0.5));
   /* SD of plumbed legs (0.25 degrees?) */
   s->Var[Q_PLUMB] = (real)sqrd(rad(0.25));
   /* SD of level legs (0.25 degrees?) */
   s->Var[Q_LEVEL] = (real)sqrd(rad(0.25));
   s->Var[Q_DEPTH] = (real)sqrd(0.05);
}

static void
default_truncate(settings *s)
{
   s->Truncate = INT_MAX;
}

static void
default_case(settings *s)
{
   s->Case = LOWER;
}

static reading default_order[] = { Fr, To, Tape, Comp, Clino, End };

static void
default_style(settings *s)
{
   s->recorded_style = s->style = STYLE_NORMAL;
   s->ordering = default_order;
   s->dash_for_anon_wall_station = false;
}

static void
default_prefix(settings *s)
{
   s->Prefix = root;
}

static void
init_default_translate_map(short * t)
{
   int i;
   for (i = '0'; i <= '9'; i++) t[i] |= SPECIAL_NAMES;
   for (i = 'A'; i <= 'Z'; i++) t[i] |= SPECIAL_NAMES;
   for (i = 'a'; i <= 'z'; i++) t[i] |= SPECIAL_NAMES;

   t['\t'] |= SPECIAL_BLANK;
   t[' '] |= SPECIAL_BLANK;
   t[','] |= SPECIAL_BLANK;
   t[';'] |= SPECIAL_COMMENT;
   t['\032'] |= SPECIAL_EOL; /* Ctrl-Z, so olde DOS text files are handled ok */
   t['\n'] |= SPECIAL_EOL;
   t['\r'] |= SPECIAL_EOL;
   t['*'] |= SPECIAL_KEYWORD;
   t['-'] |= SPECIAL_OMIT;
   t['\\'] |= SPECIAL_ROOT;
   t['.'] |= SPECIAL_SEPARATOR;
   t['_'] |= SPECIAL_NAMES;
   t['-'] |= SPECIAL_NAMES; /* Added in 0.97 prerelease 4 */
   t['.'] |= SPECIAL_DECIMAL;
   t['-'] |= SPECIAL_MINUS;
   t['+'] |= SPECIAL_PLUS;
#if 0 /* FIXME */
   t['{'] |= SPECIAL_OPEN;
   t['}'] |= SPECIAL_CLOSE;
#endif
}

static void
default_translate(settings *s)
{
   if (s->next && s->next->Translate == s->Translate) {
      /* We're currently using the same character translation map as our parent
       * scope so allocate a new one before we modify it.
       */
      s->Translate = ((short*)osmalloc(ossizeof(short) * 257)) + 1;
   } else {
/*  SVX_ASSERT(EOF==-1);*/ /* important, since we rely on this */
   }
   s->Translate[EOF] = SPECIAL_EOL;
   memset(s->Translate, 0, sizeof(short) * 256);
   init_default_translate_map(s->Translate);
}

/* Flag anything used in SPECIAL_* cumulatively to help us pick a suitable
 * separator to use in the .3d file. */
static short separator_map[256];

void
scan_compass_station_name(prefix *stn)
{
    /* We only need to scan the leaf station name - any survey hierarchy above
     * that must have been set up in .svx files for which we update
     * separator_map via cmd_set() plus adding the defaults in
     * find_output_separator().
     */
    for (const char *p = prefix_ident(stn); *p; ++p) {
	separator_map[(unsigned char)*p] |= SPECIAL_NAMES;
    }
}

static char
find_output_separator(void)
{
    // Fast path to handle most common cases where we'd pick '.'.
    if ((separator_map['.'] & SPECIAL_NAMES) == 0) {
	return '.';
    }

    static bool added_defaults = false;
    if (!added_defaults) {
	/* Add the default settings to separator_map. */
	init_default_translate_map(separator_map);
	added_defaults = true;
    }

    /* 30 punctuation characters plus space to try arranged in a sensible order
     * of decreasing preference (these are all the ASCII punctuation characters
     * excluding '_' and '-' since those are allowed in names by default so are
     * poor choices for the separator).
     */
    int best = -1;
    for (const char *p = "./:;,!|\\ ~+*^='`\"#$%&?@<>()[]{}"; *p; ++p) {
	unsigned char candidate = *p;
	int mask = separator_map[candidate];
	switch (mask & (SPECIAL_SEPARATOR|SPECIAL_NAMES)) {
	    case SPECIAL_SEPARATOR:
		/* A character which is set as a separator character at some
		 * point but never set as a name character is perfect.
		 */
		return candidate;
	    case 0:
		/* A character which is never set as either a separator
		 * character or a name character is a reasonable option.
		 */
		if (best < 0) best = candidate;
		break;
	}
    }
    if (best < 0) {
	/* Argh, no plausible choice!  Just return the default for now. */
	return '.';
    }
    return best;
}

void
default_units(settings *s)
{
   int quantity;
   for (quantity = 0; quantity < Q_MAC; quantity++) {
      if (TSTBIT(ANG_QMASK, quantity))
	 s->units[quantity] = (real)(M_PI / 180.0); /* degrees */
      else
	 s->units[quantity] = (real)1.0; /* metres */
   }
   s->f_clino_percent = s->f_backclino_percent = false;
   s->f_bearing_quadrants = s->f_backbearing_quadrants = false;
}

void
default_calib(settings *s)
{
   int quantity;
   for (quantity = 0; quantity < Q_MAC; quantity++) {
      s->z[quantity] = (real)0.0;
      s->sc[quantity] = (real)1.0;
   }
}

static void
default_flags(settings *s)
{
   s->flags = 0;
}

extern void
default_all(settings *s)
{
   default_truncate(s);
   s->infer = 0;
   default_case(s);
   default_style(s);
   default_prefix(s);
   default_translate(s);
   default_grade(s);
   default_units(s);
   default_calib(s);
   default_flags(s);
}

string token = S_INIT;

string uctoken = S_INIT;

extern void
get_token_legacy(void)
{
   skipblanks();
   get_token_legacy_no_blanks();
}

extern void
get_token_legacy_no_blanks(void)
{
   s_clear(&token);
   s_clear(&uctoken);
   while (isalpha(ch)) {
      s_appendch(&token, ch);
      s_appendch(&uctoken, toupper(ch));
      nextch();
   }

#if 0
   printf("get_token_legacy_no_blanks() got “%s”\n", s_str(&token));
#endif
}

void
do_legacy_token_warning(void)
{
    if (!s_empty(&token)) {
	if (!isBlank(ch) && !isComm(ch) && !isEol(ch)) {
	    compile_diagnostic(DIAG_WARN|DIAG_COL, /*No blank after token*/74);
	}
    }
}

extern void
get_token(void)
{
    skipblanks();
    get_token_no_blanks();
}

extern void
get_token_no_blanks(void)
{
    s_clear(&token);
    s_clear(&uctoken);
    if (isalpha(ch)) {
	do {
	    s_appendch(&token, ch);
	    s_appendch(&uctoken, toupper(ch));
	    nextch();
	} while (isalnum(ch));
    }
}

/* read word */
void
get_word(void)
{
   s_clear(&token);
   skipblanks();
   while (!isBlank(ch) && !isComm(ch) && !isEol(ch)) {
      s_appendch(&token, ch);
      nextch();
   }
#if 0
   printf("get_word() got “%s”\n", s_str(&token));
#endif
}

/* match_tok() now uses binary chop
 * tab argument should be alphabetically sorted (ascending)
 */
extern int
match_tok(const sztok *tab, int tab_size)
{
   int a = 0, b = tab_size - 1, c;
   int r;
   const char* tok = s_str(&uctoken);
   SVX_ASSERT(tab_size > 0); /* catch empty table */
/*  printf("[%d,%d]",a,b); */
   while (a <= b) {
      c = (unsigned)(a + b) / 2;
/*     printf(" %d",c); */
      r = strcmp(tab[c].sz, tok);
      if (r == 0) return tab[c].tok; /* match */
      if (r < 0)
	 a = c + 1;
      else
	 b = c - 1;
   }
   return tab[tab_size].tok; /* no match */
}

typedef enum {
   CMD_NULL = -1, CMD_ALIAS, CMD_BEGIN, CMD_CALIBRATE, CMD_CARTESIAN, CMD_CASE,
   CMD_COPYRIGHT, CMD_CS, CMD_DATA, CMD_DATE, CMD_DECLINATION, CMD_DEFAULT,
   CMD_END, CMD_ENTRANCE, CMD_EQUATE, CMD_EXPORT, CMD_FIX, CMD_FLAGS,
   CMD_INCLUDE, CMD_INFER, CMD_INSTRUMENT, CMD_PREFIX, CMD_REF, CMD_REQUIRE,
   CMD_SD, CMD_SET, CMD_SOLVE, CMD_TEAM, CMD_TITLE, CMD_TRUNCATE, CMD_UNITS
} cmds;

static const sztok cmd_tab[] = {
     {"ALIAS",     CMD_ALIAS},
     {"BEGIN",     CMD_BEGIN},
     {"CALIBRATE", CMD_CALIBRATE},
     {"CARTESIAN", CMD_CARTESIAN},
     {"CASE",      CMD_CASE},
     {"COPYRIGHT", CMD_COPYRIGHT},
     {"CS",        CMD_CS},
     {"DATA",      CMD_DATA},
     {"DATE",      CMD_DATE},
     {"DECLINATION", CMD_DECLINATION},
#ifndef NO_DEPRECATED
     {"DEFAULT",   CMD_DEFAULT},
#endif
     {"END",       CMD_END},
     {"ENTRANCE",  CMD_ENTRANCE},
     {"EQUATE",    CMD_EQUATE},
     {"EXPORT",    CMD_EXPORT},
     {"FIX",       CMD_FIX},
     {"FLAGS",     CMD_FLAGS},
     {"INCLUDE",   CMD_INCLUDE},
     {"INFER",     CMD_INFER},
     {"INSTRUMENT",CMD_INSTRUMENT},
#ifndef NO_DEPRECATED
     {"PREFIX",    CMD_PREFIX},
#endif
     {"REF",	   CMD_REF},
     {"REQUIRE",   CMD_REQUIRE},
     {"SD",	   CMD_SD},
     {"SET",       CMD_SET},
     {"SOLVE",     CMD_SOLVE},
     {"TEAM",      CMD_TEAM},
     {"TITLE",     CMD_TITLE},
     {"TRUNCATE",  CMD_TRUNCATE},
     {"UNITS",     CMD_UNITS},
     {NULL,	   CMD_NULL}
};

/* masks for units which are length and angles respectively */
#define LEN_UMASK (BIT(UNITS_METRES) | BIT(UNITS_FEET) | BIT(UNITS_YARDS))
#define ANG_UMASK (BIT(UNITS_DEGS) | BIT(UNITS_GRADS) | BIT(UNITS_MINUTES))

/* ordering must be the same as the units enum */
const real factor_tab[] = {
   1.0, METRES_PER_FOOT, (METRES_PER_FOOT*3.0),
   (M_PI/180.0), (M_PI/180.0), (M_PI/200.0), 0.01, (M_PI/180.0/60.0)
};

const int units_to_msgno[] = {
    /*m*/424,
    /*ft*/428,
    -1, /* yards */
    /*°*/344, /* quadrants */
    /*°*/344,
    /*ᵍ*/345,
    /*%*/96,
    -1 /* minutes */
};

int get_length_units(int quantity) {
    double factor = pcs->units[quantity];
    if (fabs(factor - METRES_PER_FOOT) <= REAL_EPSILON ||
	fabs(factor - METRES_PER_FOOT * 3.0) <= REAL_EPSILON) {
	return UNITS_FEET;
    }
    return UNITS_METRES;
}

int get_angle_units(int quantity) {
    double factor = pcs->units[quantity];
    if (fabs(factor - M_PI / 200.0) <= REAL_EPSILON) {
	return UNITS_GRADS;
    }
    return UNITS_DEGS;
}

static int
get_units(unsigned long qmask, bool percent_ok)
{
   static const sztok utab[] = {
	{"DEGREES",       UNITS_DEGS },
	{"DEGS",	  UNITS_DEGS },
	{"FEET",	  UNITS_FEET },
	{"GRADS",	  UNITS_GRADS },
	{"METERS",	  UNITS_METRES },
	{"METRES",	  UNITS_METRES },
	{"METRIC",	  UNITS_METRES },
	{"MILS",	  UNITS_DEPRECATED_ALIAS_FOR_GRADS },
	{"MINUTES",	  UNITS_MINUTES },
	{"PERCENT",	  UNITS_PERCENT },
	{"PERCENTAGE",    UNITS_PERCENT },
	{"QUADRANTS",	  UNITS_QUADRANTS },
	{"QUADS",	  UNITS_QUADRANTS },
	{"YARDS",	  UNITS_YARDS },
	{NULL,		  UNITS_NULL }
   };
   get_token();
   int units = match_tok(utab, TABSIZE(utab));
   if (units == UNITS_NULL) {
      compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP, /*Unknown units “%s”*/35,
			 s_str(&token));
      return UNITS_NULL;
   }
   /* Survex has long misdefined "mils" as an alias for "grads", of which
    * there are 400 in a circle.  There are several definitions of "mils"
    * with a circle containing 2000π SI milliradians, 6400 NATO mils, 6000
    * Warsaw Pact mils, and 6300 Swedish streck, and they aren't in common
    * use by cave surveyors, so we now just warn if mils are used.
    */
   if (units == UNITS_DEPRECATED_ALIAS_FOR_GRADS) {
      compile_diagnostic(DIAG_WARN|DIAG_TOKEN|DIAG_SKIP,
			 /*Units “%s” are deprecated, assuming “grads” - see manual for details*/479,
			 s_str(&token));
      units = UNITS_GRADS;
   }
   if (units == UNITS_PERCENT && percent_ok &&
       !(qmask & ~(BIT(Q_GRADIENT)|BIT(Q_BACKGRADIENT)))) {
      return units;
   }
   if (units == UNITS_QUADRANTS &&
       !(qmask & ~(BIT(Q_BEARING)|BIT(Q_BACKBEARING)))) {
      return units;
   }
   if (((qmask & LEN_QMASK) && !TSTBIT(LEN_UMASK, units)) ||
       ((qmask & ANG_QMASK) && !TSTBIT(ANG_UMASK, units))) {
      /* TRANSLATORS: Note: In English you talk about the *units* of a single
       * measurement, but the correct term in other languages may be singular.
       */
      compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP, /*Invalid units “%s” for quantity*/37, s_str(&token));
      return UNITS_NULL;
   }
   return units;
}

/* returns mask with bit x set to indicate quantity x specified */
static unsigned long
get_qlist(unsigned long mask_bad)
{
   static const sztok qtab[] = {
	{"ALTITUDE",	 Q_DZ },
	{"BACKBEARING",  Q_BACKBEARING },
	{"BACKCLINO",    Q_BACKGRADIENT },    /* alternative name */
	{"BACKCOMPASS",  Q_BACKBEARING },     /* alternative name */
	{"BACKGRADIENT", Q_BACKGRADIENT },
	{"BACKLENGTH",   Q_BACKLENGTH },
	{"BACKTAPE",     Q_BACKLENGTH },    /* alternative name */
	{"BEARING",      Q_BEARING },
	{"CEILING",      Q_UP },          /* alternative name */
	{"CLINO",	 Q_GRADIENT },    /* alternative name */
	{"COMPASS",      Q_BEARING },     /* alternative name */
	{"COUNT",	 Q_COUNT },
	{"COUNTER",      Q_COUNT },       /* alternative name */
	{"DECLINATION",  Q_DECLINATION },
	{"DEFAULT",      Q_DEFAULT }, /* not a real quantity... */
	{"DEPTH",	 Q_DEPTH },
	{"DOWN",         Q_DOWN },
	{"DX",		 Q_DX },	  /* alternative name */
	{"DY",		 Q_DY },	  /* alternative name */
	{"DZ",		 Q_DZ },	  /* alternative name */
	{"EASTING",	 Q_DX },
	{"FLOOR",        Q_DOWN },        /* alternative name */
	{"GRADIENT",     Q_GRADIENT },
	{"LEFT",         Q_LEFT },
	{"LENGTH",       Q_LENGTH },
	{"LEVEL",	 Q_LEVEL},
	{"NORTHING",     Q_DY },
	{"PLUMB",	 Q_PLUMB},
	{"POSITION",     Q_POS },
	{"RIGHT",        Q_RIGHT },
	{"TAPE",	 Q_LENGTH },      /* alternative name */
	{"UP",           Q_UP },
	{NULL,		 Q_NULL }
   };
   unsigned long qmask = 0;
   int tok;
   filepos fp;

   while (1) {
      get_pos(&fp);
      get_token_legacy();
      tok = match_tok(qtab, TABSIZE(qtab));
      if (tok == Q_DEFAULT && !(mask_bad & BIT(Q_DEFAULT))) {
	  /* Only recognise DEFAULT if it is the first quantity, and then don't
	   * look for any more. */
	  if (qmask == 0)
	      return BIT(Q_DEFAULT);
	  break;
      }
      /* bail out if we reach the table end with no match */
      if (tok == Q_NULL) break;
      qmask |= BIT(tok);
      if (qmask & mask_bad) {
	 compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP, /*Unknown instrument “%s”*/39, s_str(&token));
	 return 0;
      }
   }

   if (qmask == 0) {
      /* TRANSLATORS: A "quantity" is something measured like "LENGTH",
       * "BEARING", "ALTITUDE", etc. */
      compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP, /*Unknown quantity “%s”*/34, s_str(&token));
   } else {
      set_pos(&fp);
   }

   return qmask;
}

#define SPECIAL_UNKNOWN 0
static void
cmd_set(void)
{
   static const sztok chartab[] = {
	{"BLANK",     SPECIAL_BLANK },
/*FIXME	{"CLOSE",     SPECIAL_CLOSE }, */
	{"COMMENT",   SPECIAL_COMMENT },
	{"DECIMAL",   SPECIAL_DECIMAL },
	{"EOL",       SPECIAL_EOL }, /* EOL won't work well */
	{"KEYWORD",   SPECIAL_KEYWORD },
	{"MINUS",     SPECIAL_MINUS },
	{"NAMES",     SPECIAL_NAMES },
	{"OMIT",      SPECIAL_OMIT },
/*FIXME	{"OPEN",      SPECIAL_OPEN }, */
	{"PLUS",      SPECIAL_PLUS },
#ifndef NO_DEPRECATED
	{"ROOT",      SPECIAL_ROOT },
#endif
	{"SEPARATOR", SPECIAL_SEPARATOR },
	{NULL,	      SPECIAL_UNKNOWN }
   };
   int i;

   get_token();
   int mask = match_tok(chartab, TABSIZE(chartab));

   if (mask == SPECIAL_UNKNOWN) {
      compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP, /*Unknown character class “%s”*/42,
			 s_str(&token));
      return;
   }

#ifndef NO_DEPRECATED
   if (mask == SPECIAL_ROOT) {
      if (root_depr_count < 5) {
	 /* TRANSLATORS: Use of the ROOT character (which is "\" by default) is
	  * deprecated, so this error would be generated by:
	  *
	  * *equate \foo.7 1
	  *
	  * If you're unsure what "deprecated" means, see:
	  * https://en.wikipedia.org/wiki/Deprecation */
	 compile_diagnostic(DIAG_WARN|DIAG_TOKEN, /*ROOT is deprecated*/25);
	 if (++root_depr_count == 5)
	     /* TRANSLATORS: If you're unsure what "deprecated" means, see:
	      * https://en.wikipedia.org/wiki/Deprecation */
	    compile_diagnostic(DIAG_INFO, /*Further uses of this deprecated feature will not be reported*/95);
      }
   }
#endif

   /* if we're currently using an inherited translation table, allocate a new
    * table, and copy old one into it */
   if (pcs->next && pcs->next->Translate == pcs->Translate) {
      short *p;
      p = ((short*)osmalloc(ossizeof(short) * 257)) + 1;
      memcpy(p - 1, pcs->Translate - 1, sizeof(short) * 257);
      pcs->Translate = p;
   }

   skipblanks();

   /* clear this flag for all non-alphanums */
   for (i = 0; i < 256; i++)
      if (!isalnum(i)) pcs->Translate[i] &= ~mask;

   /* now set this flag for all specified chars */
   while (!isEol(ch)) {
      int char_to_set;
      if (!isalnum(ch)) {
	 char_to_set = ch;
      } else if (tolower(ch) == 'x') {
	 int hex;
	 filepos fp;
	 get_pos(&fp);
	 nextch();
	 if (!isxdigit(ch)) {
	    set_pos(&fp);
	    break;
	 }
	 hex = isdigit(ch) ? ch - '0' : tolower(ch) - 'a';
	 nextch();
	 if (!isxdigit(ch)) {
	    set_pos(&fp);
	    break;
	 }
	 hex = hex << 4 | (isdigit(ch) ? ch - '0' : tolower(ch) - 'a');
	 char_to_set = hex;
      } else {
	 break;
      }
      pcs->Translate[char_to_set] |= mask;
      separator_map[char_to_set] |= mask;
      nextch();
   }

   output_separator = find_output_separator();
}

void
update_output_separator(void)
{
   output_separator = find_output_separator();
}

static void
check_reentry(prefix *survey, const filepos* fpos_ptr)
{
   /* Don't try to check "*prefix \" or "*begin \" */
   if (!survey->up) return;
   if (TSTBIT(survey->sflags, SFLAGS_PREFIX_ENTERED)) {
      static int reenter_depr_count = 0;
      filepos fp_tmp;

      if (reenter_depr_count >= 5)
	 return;

      get_pos(&fp_tmp);
      set_pos(fpos_ptr);
      /* TRANSLATORS: The first of two warnings given when a survey which has
       * already been completed is reentered.  This example file crawl.svx:
       *
       * *begin crawl     ; <- second warning here
       * 1 2 9.45 234 -01
       * *end crawl
       * *begin crawl     ; <- first warning here
       * 2 3 7.67 223 -03
       * *end crawl
       *
       * Would lead to:
       *
       * crawl.svx:4:8: warning: Reentering an existing survey is deprecated
       * crawl.svx:1: info: Originally entered here
       *
       * If you're unsure what "deprecated" means, see:
       * https://en.wikipedia.org/wiki/Deprecation */
      compile_diagnostic(DIAG_WARN|DIAG_WORD, /*Reentering an existing survey is deprecated*/29);
      set_pos(&fp_tmp);
      /* TRANSLATORS: The second of two warnings given when a survey which has
       * already been completed is reentered.  This example file crawl.svx:
       *
       * *begin crawl     ; <- second warning here
       * 1 2 9.45 234 -01
       * *end crawl
       * *begin crawl     ; <- first warning here
       * 2 3 7.67 223 -03
       * *end crawl
       *
       * Would lead to:
       *
       * crawl.svx:4:8: warning: Reentering an existing survey is deprecated
       * crawl.svx:1: info: Originally entered here
       *
       * If you're unsure what "deprecated" means, see:
       * https://en.wikipedia.org/wiki/Deprecation */
      compile_diagnostic_pfx(DIAG_INFO, survey, /*Originally entered here*/30);
      if (++reenter_depr_count == 5) {
	 /* After we've warned about 5 uses of the same deprecated feature, we
	  * give up for the rest of the current processing run.
	  *
	  * If you're unsure what "deprecated" means, see:
	  * https://en.wikipedia.org/wiki/Deprecation */
	 compile_diagnostic(DIAG_INFO, /*Further uses of this deprecated feature will not be reported*/95);
      }
   } else {
      survey->sflags |= BIT(SFLAGS_PREFIX_ENTERED);
      survey->filename = file.filename;
      survey->line = file.line;
   }
}

#ifndef NO_DEPRECATED
static void
cmd_prefix(void)
{
   static int prefix_depr_count = 0;
   prefix *survey;
   filepos fp;
   /* Issue warning first, so "*prefix \" warns first that *prefix is
    * deprecated and then that ROOT is...
    */
   if (prefix_depr_count < 5) {
      /* TRANSLATORS: If you're unsure what "deprecated" means, see:
       * https://en.wikipedia.org/wiki/Deprecation */
      compile_diagnostic(DIAG_WARN|DIAG_TOKEN, /**prefix is deprecated - use *begin and *end instead*/6);
      if (++prefix_depr_count == 5)
	 compile_diagnostic(DIAG_INFO, /*Further uses of this deprecated feature will not be reported*/95);
   }
   get_pos(&fp);
   survey = read_prefix(PFX_SURVEY|PFX_ALLOW_ROOT);
   pcs->Prefix = survey;
   check_reentry(survey, &fp);
}
#endif

static void
cmd_alias(void)
{
   /* Currently only two forms are supported:
    * *alias station - ..
    * *alias station -
    */
   get_token();
   if (!S_EQ(&uctoken, "STATION")) {
       compile_diagnostic(DIAG_ERR|DIAG_SKIP|DIAG_TOKEN, /*Bad *alias command*/397);
       return;
   }
   get_word();
   if (!S_EQ(&token, "-"))
      goto bad_word;
   get_word();
   if (!s_empty(&token) && !S_EQ(&token, ".."))
      goto bad_word;
   pcs->dash_for_anon_wall_station = !s_empty(&token);
   return;
bad_word:
   compile_diagnostic(DIAG_ERR|DIAG_SKIP|DIAG_TOKEN, /*Bad *alias command*/397);
}

static void
cmd_begin(void)
{
   settings *pcsNew;

   pcsNew = osnew(settings);
   *pcsNew = *pcs; /* copy contents */
   pcsNew->begin_lineno = file.line;
   pcsNew->begin_lpos = file.lpos;
   pcsNew->next = pcs;
   pcs = pcsNew;

   skipblanks();
   pcs->begin_survey = NULL;
   pcs->begin_col = 0;
   if (!isEol(ch) && !isComm(ch)) {
      filepos fp;
      prefix *survey;
      get_pos(&fp);
      int begin_col = fp.offset - file.lpos;
      survey = read_prefix(PFX_SURVEY|PFX_ALLOW_ROOT|PFX_WARN_SEPARATOR);
      // read_prefix() might fail and longjmp() so only set begin_col if
      // it succeeds.
      pcs->begin_col = begin_col;
      pcs->begin_survey = survey;
      pcs->Prefix = survey;
      check_reentry(survey, &fp);
      f_export_ok = true;
   }
}

void
invalidate_pj_cached(void)
{
    /* Invalidate the cached PJ. */
    if (pj_cached) {
	proj_destroy(pj_cached);
	pj_cached = NULL;
    }
}

void
report_declination(settings *p)
{
    if (p->min_declination <= p->max_declination) {
	int y, m, d;
	char range[128];
	const char* deg_sign = msg(/*°*/344);
	ymd_from_days_since_1900(p->min_declination_days, &y, &m, &d);
	snprintf(range, sizeof(range),
		 "%.1f%s @ %04d-%02d-%02d",
		 deg(p->min_declination), deg_sign, y, m, d);
	if (p->min_declination_days != p->max_declination_days) {
	    size_t len = strlen(range);
	    ymd_from_days_since_1900(p->max_declination_days, &y, &m, &d);
	    snprintf(range + len, sizeof(range) - len,
		     " / %.1f%s @ %04d-%02d-%02d",
		     deg(p->max_declination), deg_sign, y, m, d);
	}
	/* TRANSLATORS: This message gives information about the range of
	 * declination values and the grid convergence value calculated for
	 * each "*declination auto ..." command.
	 *
	 * The first %s will be replaced by the declination range (or single
	 * value), and %.1f%s by the grid convergence angle.
	 */
	compile_diagnostic_at(DIAG_INFO, p->dec_filename, p->dec_line,
			      /*Declination: %s, grid convergence: %.1f%s*/484,
			      range,
			      deg(p->convergence), deg_sign);
	PUTC(' ', STDERR);
	fputs(p->dec_context, STDERR);
	fputnl(STDERR);
	if (p->next && p->dec_context != p->next->dec_context)
	    free(p->dec_context);
	p->dec_context = NULL;
	p->min_declination = HUGE_VAL;
	p->max_declination = -HUGE_VAL;
    }
}

void
set_declination_location(real x, real y, real z, const char *proj_str)
{
    /* Convert to WGS84 lat long. */
    PJ *transform = proj_create_crs_to_crs(PJ_DEFAULT_CTX,
					   proj_str,
					   WGS84_DATUM_STRING,
					   NULL);
    if (transform) {
	/* Normalise the output order so x is longitude and y latitude - by
	 * default new PROJ has them switched for EPSG:4326 which just seems
	 * confusing.
	 */
	PJ* pj_norm = proj_normalize_for_visualization(PJ_DEFAULT_CTX,
						       transform);
	proj_destroy(transform);
	transform = pj_norm;
    }

    if (proj_angular_input(transform, PJ_FWD)) {
	/* Input coordinate system expects radians. */
	x = rad(x);
	y = rad(y);
    }

    PJ_COORD coord = {{x, y, z, HUGE_VAL}};
    coord = proj_trans(transform, PJ_FWD, coord);
    x = coord.xyzt.x;
    y = coord.xyzt.y;
    z = coord.xyzt.z;

    if (x == HUGE_VAL || y == HUGE_VAL || z == HUGE_VAL) {
       compile_diagnostic(DIAG_ERR, /*Failed to convert coordinates: %s*/436,
			  proj_context_errno_string(PJ_DEFAULT_CTX,
						    proj_errno(transform)));
       /* Set dummy values which are finite. */
       x = y = z = 0;
    }
    proj_destroy(transform);

    report_declination(pcs);

    double lon = rad(x);
    double lat = rad(y);
    pcs->z[Q_DECLINATION] = HUGE_REAL;
    pcs->dec_lat = lat;
    pcs->dec_lon = lon;
    pcs->dec_alt = z;
    pcs->dec_filename = file.filename;
    pcs->dec_line = file.line;
    pcs->dec_context = grab_line();
    /* Invalidate cached declination. */
    pcs->declination = HUGE_REAL;
    /* Invalidate cached grid convergence values. */
    pcs->convergence = HUGE_REAL;
    pcs->input_convergence = HUGE_REAL;
}

void
pop_settings(void)
{
    settings * p = pcs;
    pcs = pcs->next;

    SVX_ASSERT(pcs);

    if (pcs->dec_lat != p->dec_lat ||
	pcs->dec_lon != p->dec_lon ||
	pcs->dec_alt != p->dec_alt) {
	report_declination(p);
    } else {
	pcs->min_declination_days = p->min_declination_days;
	pcs->max_declination_days = p->max_declination_days;
	pcs->min_declination = p->min_declination;
	pcs->max_declination = p->max_declination;
	pcs->convergence = p->convergence;
    }

    if (p->proj_str != pcs->proj_str) {
	if (!p->proj_str || !pcs->proj_str ||
	    strcmp(p->proj_str, pcs->proj_str) != 0) {
	    invalidate_pj_cached();
	}
	/* free proj_str if not used by parent */
	osfree(p->proj_str);
    }

    /* don't free default ordering or ordering used by parent */
    if (p->ordering != default_order && p->ordering != pcs->ordering)
	osfree((reading*)p->ordering);

    /* free Translate if not used by parent */
    if (p->Translate != pcs->Translate)
	osfree(p->Translate - 1);

    /* free meta if not used by parent, or in this block */
    if (p->meta && p->meta != pcs->meta && p->meta->ref_count == 0)
	osfree(p->meta);

    osfree(p);
}

static void
cmd_end(void)
{
   filepos fp;

   int begin_lineno = pcs->begin_lineno;
   if (begin_lineno == 0) {
      if (pcs->next == NULL) {
	 /* more ENDs than BEGINs */
	 /* TRANSLATORS: %s is replaced with e.g. BEGIN or .BOOK or #[ */
	 compile_diagnostic(DIAG_ERR|DIAG_SKIP, /*No matching %s*/192, "BEGIN");
      } else {
	 /* TRANSLATORS: %s and %s are replaced with e.g. BEGIN and END
	  * or END and BEGIN or #[ and #] */
	 compile_diagnostic(DIAG_ERR|DIAG_SKIP,
			    /*%s with no matching %s in this file*/23, "END", "BEGIN");
      }
      return;
   }

   prefix *begin_survey = pcs->begin_survey;
   long begin_lpos = pcs->begin_lpos;
   int begin_col = pcs->begin_col;

   pop_settings();

   /* note need to read using root *before* BEGIN */
   prefix *survey = NULL;
   skipblanks();
   if (!isEol(ch) && !isComm(ch)) {
      get_pos(&fp);
      survey = read_prefix(PFX_SURVEY|PFX_ALLOW_ROOT);
   }

   if (survey != begin_survey) {
      filepos fp_save;
      get_pos(&fp_save);
      if (survey) {
	 set_pos(&fp);
	 if (!begin_survey) {
	    /* TRANSLATORS: Used when a BEGIN command has no survey, but the
	     * END command does, e.g.:
	     *
	     * *begin
	     * 1 2 10.00 178 -01
	     * *end entrance      <--[Message given here] */
	    compile_diagnostic(DIAG_ERR|DIAG_WORD, /*Matching BEGIN command has no survey name*/36);
	 } else {
	    /* TRANSLATORS: *BEGIN <survey> and *END <survey> should have the
	     * same <survey> if it’s given at all */
	    compile_diagnostic(DIAG_ERR|DIAG_WORD, /*Survey name doesn’t match BEGIN*/193);
	 }
      } else {
	 /* TRANSLATORS: Used when a BEGIN command has a survey name, but the
	  * END command omits it, e.g.:
	  *
	  * *begin entrance
	  * 1 2 10.00 178 -01
	  * *end     <--[Message given here] */
	 compile_diagnostic(DIAG_WARN|DIAG_COL, /*Survey name omitted from END*/194);
      }
      parse file_save = file;
      file.line = begin_lineno;
      file.lpos = begin_lpos;
      int word_flag = 0;
      if (begin_col) {
	  word_flag = DIAG_WORD;
	  fseek(file.fh, begin_lpos + begin_col - 1, SEEK_SET);
	  nextch();
      }
      compile_diagnostic(DIAG_INFO|word_flag, /*Corresponding %s was here*/22, "BEGIN");
      file = file_save;
      set_pos(&fp_save);
   }
}

static void
cmd_entrance(void)
{
   prefix *pfx = read_prefix(PFX_STATION);
   pfx->sflags |= BIT(SFLAGS_ENTRANCE) | BIT(SFLAGS_USED);
}

static const prefix * first_fix_name = NULL;
static const char * first_fix_filename;
static unsigned first_fix_line;

static void
cmd_fix(void)
{
   static prefix *name_omit_already = NULL;
   static const char * name_omit_already_filename = NULL;
   static unsigned int name_omit_already_line;
   PJ_COORD coord;
   filepos fp_stn, fp;

   get_pos(&fp_stn);
   prefix *fix_name = read_prefix(PFX_STATION|PFX_ALLOW_ROOT);

   get_pos(&fp);
   get_token_legacy();
   bool reference = S_EQ(&uctoken, "REFERENCE");
   if (reference) {
      do_legacy_token_warning();
      /* suppress "unused fixed point" warnings for this station */
      fix_name->sflags |= BIT(SFLAGS_USED);
   } else {
      if (!s_empty(&uctoken)) set_pos(&fp);
   }

   // If `REFERENCE` is specified the coordinates can't be omitted.
   coord.v[0] = read_numeric(!reference);
   if (coord.v[0] == HUGE_REAL) {
      /* If the end of the line isn't blank, read a number after all to
       * get a more helpful error message */
      if (!isEol(ch) && !isComm(ch)) coord.v[0] = read_numeric(false);
   }
   if (coord.v[0] == HUGE_REAL) {
      if (pcs->proj_str || proj_str_out) {
	 compile_diagnostic(DIAG_ERR|DIAG_COL|DIAG_SKIP, /*Coordinates can't be omitted when coordinate system has been specified*/439);
	 return;
      }

      if (fix_name == name_omit_already) {
	 compile_diagnostic(DIAG_WARN|DIAG_COL, /*Same station fixed twice with no coordinates*/61);
	 return;
      }

      if (name_omit_already) {
	 /* TRANSLATORS: Emitted after second and subsequent "FIX" command
	  * with no coordinates.
	  */
	 compile_diagnostic_at(DIAG_ERR,
			       name_omit_already_filename,
			       name_omit_already_line,
			       /*Already had FIX command with no coordinates for station “%s”*/441,
			       sprint_prefix(name_omit_already));
      } else {
	 /* TRANSLATORS: " *fix a " gives this message: */
	 compile_diagnostic(DIAG_INFO|DIAG_COL, /*FIX command with no coordinates - fixing at (0,0,0)*/54);

	 name_omit_already = fix_name;
	 name_omit_already_filename = file.filename;
	 name_omit_already_line = file.line;
      }

      coord.v[0] = coord.v[1] = coord.v[2] = (real)0.0;
   } else {
      coord.v[1] = read_numeric(false);
      coord.v[2] = read_numeric(false);

      if (pcs->proj_str && proj_str_out) {
	 PJ *transform = pj_cached;
	 if (!transform) {
	     transform = proj_create_crs_to_crs(PJ_DEFAULT_CTX,
						pcs->proj_str,
						proj_str_out,
						NULL);
	     if (transform) {
		/* Normalise the output order so x is longitude and y latitude - by
		 * default new PROJ has them switched for EPSG:4326 which just seems
		 * confusing.
		 */
		PJ* pj_norm = proj_normalize_for_visualization(PJ_DEFAULT_CTX,
							       transform);
		proj_destroy(transform);
		transform = pj_norm;
	     }

	     pj_cached = transform;
	 }

	 if (proj_angular_input(transform, PJ_FWD)) {
	    /* Input coordinate system expects radians. */
	    coord.v[0] = rad(coord.v[0]);
	    coord.v[1] = rad(coord.v[1]);
	 }

	 coord.v[3] = HUGE_VAL;
	 coord = proj_trans(transform, PJ_FWD, coord);

	 if (coord.v[0] == HUGE_VAL ||
	     coord.v[1] == HUGE_VAL ||
	     coord.v[2] == HUGE_VAL) {
	    compile_diagnostic(DIAG_ERR, /*Failed to convert coordinates: %s*/436,
			       proj_context_errno_string(PJ_DEFAULT_CTX,
							 proj_errno(transform)));
	    /* Set dummy values which are finite. */
	    coord.v[0] = coord.v[1] = coord.v[2] = 0;
	 }
      } else if (pcs->proj_str) {
	 compile_diagnostic(DIAG_ERR, /*The input projection is set but the output projection isn't*/437);
      } else if (proj_str_out) {
	 compile_diagnostic(DIAG_ERR, /*The output projection is set but the input projection isn't*/438);
      }

      get_pos(&fp);
      real sdx = read_numeric(true);
      if (sdx <= 0) {
	  set_pos(&fp);
	  compile_diagnostic(DIAG_ERR|DIAG_SKIP|DIAG_NUM, /*Standard deviation must be positive*/48);
	  return;
      }
      if (sdx != HUGE_REAL) {
	 real sdy, sdz;
	 real cxy = 0, cyz = 0, czx = 0;
	 get_pos(&fp);
	 sdy = read_numeric(true);
	 if (sdy == HUGE_REAL) {
	    /* only one variance given */
	    sdy = sdz = sdx;
	 } else {
	    if (sdy <= 0) {
	       set_pos(&fp);
	       compile_diagnostic(DIAG_ERR|DIAG_SKIP|DIAG_NUM, /*Standard deviation must be positive*/48);
	       return;
	    }
	    get_pos(&fp);
	    sdz = read_numeric(true);
	    if (sdz == HUGE_REAL) {
	       /* two variances given - horizontal & vertical */
	       sdz = sdy;
	       sdy = sdx;
	    } else {
	       if (sdz <= 0) {
		  set_pos(&fp);
		  compile_diagnostic(DIAG_ERR|DIAG_SKIP|DIAG_NUM, /*Standard deviation must be positive*/48);
		  return;
	       }
	       cxy = read_numeric(true);
	       if (cxy != HUGE_REAL) {
		  /* covariances given */
		  cyz = read_numeric(false);
		  czx = read_numeric(false);
	       } else {
		  cxy = 0;
	       }
	    }
	 }
	 fix_station_with_variance(fix_name, coord.v,
				   sdx * sdx, sdy * sdy, sdz * sdz
#ifndef NO_COVARIANCES
				   , cxy, cyz, czx
#endif
				  );

	 if (!first_fix_name) {
	    /* We track if we've fixed a station yet, and if so what the name
	     * of the first fix was, so that we can issue an error if the
	     * output coordinate system is set after fixing a station. */
	    first_fix_name = fix_name;
	    first_fix_filename = file.filename;
	    first_fix_line = file.line;
	 }

	 return;
      }
   }

   if (!first_fix_name) {
      /* We track if we've fixed a station yet, and if so what the name of the
       * first fix was, so that we can issue an error if the output coordinate
       * system is set after fixing a station. */
      first_fix_name = fix_name;
      first_fix_filename = file.filename;
      first_fix_line = file.line;
   }

   int fix_result = fix_station(fix_name, coord.v);
   if (fix_result == 0) {
      return;
   }

   get_pos(&fp);
   set_pos(&fp_stn);
   if (fix_result < 0) {
       compile_diagnostic(DIAG_ERR|DIAG_WORD, /*Station already fixed or equated to a fixed point*/46);
   } else {
       /* TRANSLATORS: *fix a 1 2 3 / *fix a 1 2 3 */
       compile_diagnostic(DIAG_WARN|DIAG_WORD, /*Station already fixed at the same coordinates*/55);
   }
   compile_diagnostic_pfx(DIAG_INFO, fix_name, /*Previously fixed or equated here*/493);
   set_pos(&fp);
}

static void
cmd_flags(void)
{
   static const sztok flagtab[] = {
	{"DUPLICATE", FLAGS_DUPLICATE },
	{"NOT",	      FLAGS_NOT },
	{"SPLAY",     FLAGS_SPLAY },
	{"SURFACE",   FLAGS_SURFACE },
	{NULL,	      FLAGS_UNKNOWN }
   };
   bool fNot = false;
   bool fEmpty = true;
   while (1) {
      int flag;
      get_token();
      /* If token is empty, it could mean end of line, or maybe
       * some non-alphanumeric junk which is better reported later */
      if (s_empty(&token)) break;

      fEmpty = false;
      flag = match_tok(flagtab, TABSIZE(flagtab));
      /* treat the second NOT in "NOT NOT" as an unknown flag */
      if (flag == FLAGS_UNKNOWN || (fNot && flag == FLAGS_NOT)) {
	 compile_diagnostic(DIAG_ERR|DIAG_TOKEN, /*FLAG “%s” unknown*/68,
			    s_str(&token));
	 /* Recover from “*FLAGS NOT BOGUS SURFACE” by ignoring "NOT BOGUS" */
	 fNot = false;
      } else if (flag == FLAGS_NOT) {
	 fNot = true;
      } else if (fNot) {
	 pcs->flags &= ~BIT(flag);
	 fNot = false;
      } else {
	 pcs->flags |= BIT(flag);
      }
   }

   if (fNot) {
      compile_diagnostic(DIAG_ERR|DIAG_TOKEN,
			 /*Expecting “%s”, “%s”, or “%s”*/188,
			 "DUPLICATE", "SPLAY", "SURFACE");
   } else if (fEmpty) {
      compile_diagnostic(DIAG_ERR|DIAG_TOKEN,
			 /*Expecting “%s”, “%s”, “%s”, or “%s”*/189,
			 "NOT", "DUPLICATE", "SPLAY", "SURFACE");
   }
}

static void
cmd_equate(void)
{
   filepos fp;
   get_pos(&fp);
   prefix *prev_name = NULL;
   prefix *name = read_prefix(PFX_STATION|PFX_ALLOW_ROOT|PFX_SUSPECT_TYPO);
   while (true) {
      if (!name->stn || !fixed(name->stn)) {
	  // If the station isn't already fixed, make its file:line location
	  // reflect this *equate.
	  name->filename = file.filename;
	  name->line = file.line;
      }
      skipblanks();
      if (isEol(ch) || isComm(ch)) {
	 if (prev_name == NULL) {
	    /* E.g. *equate entrance.6 */
	    set_pos(&fp);
	    /* TRANSLATORS: EQUATE is a command name, so shouldn’t be
	     * translated.
	     *
	     * Here "station" is a survey station, not a train station.
	     */
	    compile_diagnostic(DIAG_ERR|DIAG_SKIP|DIAG_WORD, /*Only one station in EQUATE command*/33);
	 }
	 return;
      }

      prev_name = name;
      name = read_prefix(PFX_STATION|PFX_ALLOW_ROOT|PFX_SUSPECT_TYPO);
      process_equate(name, prev_name);
   }
}

static void
report_missing_export(prefix *pfx, int depth)
{
   char *s;
   const char *p;
   prefix *survey = pfx;
   int i;
   for (i = depth + 1; i; i--) {
      survey = survey->up;
      SVX_ASSERT(survey);
   }
   s = osstrdup(sprint_prefix(survey));
   p = sprint_prefix(pfx);
   if (survey->filename) {
      /* TRANSLATORS: A station must be exported out of each level it is in, so
       * this would give "Station “\outer.inner.1” not exported from survey
       * “\outer”)":
       *
       * *equate entrance outer.inner.1
       * *begin outer
       * *begin inner
       * *export 1
       * 1 2 1.23 045 -6
       * *end inner
       * *end outer
       *
       * Here "survey" is a "cave map" rather than list of questions - it should be
       * translated to the terminology that cavers using the language would use.
       */
      compile_diagnostic_pfx(DIAG_ERR, survey,
			     /*Station “%s” not exported from survey “%s”*/26, p, s);
   } else {
      compile_diagnostic(DIAG_ERR, /*Station “%s” not exported from survey “%s”*/26, p, s);
   }
   osfree(s);
}

static void
cmd_export(void)
{
   prefix *pfx;

   fExportUsed = true;
   do {
      int depth = 0;
      pfx = read_prefix(PFX_STATION);
      {
	 prefix *p = pfx;
	 while (p != NULL && p != pcs->Prefix) {
	    depth++;
	    p = p->up;
	 }
	 /* Something like: *export \foo, but we've excluded use of root */
	 SVX_ASSERT(p);
      }
      /* *export \ or similar bogus stuff */
      SVX_ASSERT(depth);
#if 0
      printf("C min %d max %d depth %d pfx %s\n",
	     pfx->min_export, pfx->max_export, depth, sprint_prefix(pfx));
#endif
      if (pfx->min_export == 0) {
	 /* not encountered *export for this name before */
	 if (pfx->max_export > depth) report_missing_export(pfx, depth);
	 pfx->min_export = pfx->max_export = depth;
      } else if (pfx->min_export != USHRT_MAX) {
	 /* FIXME: what to do if a station is marked for inferred exports
	  * but is then explicitly exported?  Currently we just ignore the
	  * explicit export... */
	 if (pfx->min_export - 1 > depth) {
	    report_missing_export(pfx, depth);
	 } else if (pfx->min_export - 1 < depth) {
	    /* TRANSLATORS: Here "station" is a survey station, not a train station.
	     *
	     * Exporting a station twice gives this error:
	     *
	     * *begin example
	     * *export 1
	     * *export 1
	     * 1 2 1.24 045 -6
	     * *end example */
	    compile_diagnostic(DIAG_ERR, /*Station “%s” already exported*/66,
			       sprint_prefix(pfx));
	 }
	 pfx->min_export = depth;
      }
      skipblanks();
   } while (!isEol(ch) && !isComm(ch));
}

static void
cmd_data(void)
{
   static const sztok dtab[] = {
	{"ALTITUDE",	 Dz },
	{"BACKBEARING",  BackComp },
	{"BACKCLINO",    BackClino }, /* alternative name */
	{"BACKCOMPASS",  BackComp }, /* alternative name */
	{"BACKGRADIENT", BackClino },
	{"BACKLENGTH",   BackTape },
	{"BACKTAPE",     BackTape }, /* alternative name */
	{"BEARING",      Comp },
	{"CEILING",      Up }, /* alternative name */
	{"CLINO",	 Clino }, /* alternative name */
	{"COMPASS",      Comp }, /* alternative name */
	{"COUNT",	 Count }, /* FrCount&ToCount in multiline */
	{"DEPTH",	 Depth }, /* FrDepth&ToDepth in multiline */
	{"DEPTHCHANGE",  DepthChange },
	{"DIRECTION",    Dir },
	{"DOWN",         Down },
	{"DX",		 Dx },
	{"DY",		 Dy },
	{"DZ",		 Dz },
	{"EASTING",      Dx },
	{"FLOOR",        Down }, /* alternative name */
	{"FROM",	 Fr },
	{"FROMCOUNT",    FrCount },
	{"FROMDEPTH",    FrDepth },
	{"GRADIENT",     Clino },
	{"IGNORE",       Ignore },
	{"IGNOREALL",    IgnoreAll },
	{"LEFT",         Left },
	{"LENGTH",       Tape },
	{"NEWLINE",      Newline },
	{"NORTHING",     Dy },
	{"RIGHT",        Right },
	{"STATION",      Station }, /* Fr&To in multiline */
	{"TAPE",	 Tape }, /* alternative name */
	{"TO",		 To },
	{"TOCOUNT",      ToCount },
	{"TODEPTH",      ToDepth },
	{"UP",           Up },
	{NULL,		 End }
   };

#define MASK_stns BIT(Fr) | BIT(To) | BIT(Station)
#define MASK_tape BIT(Tape) | BIT(BackTape) | BIT(FrCount) | BIT(ToCount) | BIT(Count)
#define MASK_dpth BIT(FrDepth) | BIT(ToDepth) | BIT(Depth) | BIT(DepthChange)
#define MASK_comp BIT(Comp) | BIT(BackComp)
#define MASK_clin BIT(Clino) | BIT(BackClino)

#define MASK_NORMAL MASK_stns | BIT(Dir) | MASK_tape | MASK_comp | MASK_clin
#define MASK_DIVING MASK_NORMAL | MASK_dpth
#define MASK_CARTESIAN MASK_stns | BIT(Dx) | BIT(Dy) | BIT(Dz)
#define MASK_CYLPOLAR  MASK_stns | BIT(Dir) | MASK_tape | MASK_comp | MASK_dpth
#define MASK_NOSURVEY MASK_stns
#define MASK_PASSAGE BIT(Station) | BIT(Left) | BIT(Right) | BIT(Up) | BIT(Down)
#define MASK_IGNORE 0 // No readings in this style.

   // readings which may be given for each style (index is STYLE_*)
   static const unsigned long mask[] = {
      MASK_NORMAL, MASK_DIVING, MASK_CARTESIAN, MASK_CYLPOLAR, MASK_NOSURVEY,
      MASK_PASSAGE, MASK_IGNORE
   };

   // readings which may be omitted for each style (index is STYLE_*)
   static const unsigned long mask_optional[] = {
      BIT(Dir) | BIT(Clino) | BIT(BackClino),
      BIT(Dir) | BIT(Clino) | BIT(BackClino),
      0,
      BIT(Dir),
      0,
      0, /* BIT(Left) | BIT(Right) | BIT(Up) | BIT(Down), */
      0
   };

   /* all valid readings */
   static const unsigned long mask_all[] = {
      MASK_NORMAL | BIT(Newline) | BIT(Ignore) | BIT(IgnoreAll) | BIT(End),
      MASK_DIVING | BIT(Newline) | BIT(Ignore) | BIT(IgnoreAll) | BIT(End),
      MASK_CARTESIAN | BIT(Newline) | BIT(Ignore) | BIT(IgnoreAll) | BIT(End),
      MASK_CYLPOLAR | BIT(Newline) | BIT(Ignore) | BIT(IgnoreAll) | BIT(End),
      MASK_NOSURVEY | BIT(Ignore) | BIT(IgnoreAll) | BIT(End),
      MASK_PASSAGE | BIT(Ignore) | BIT(IgnoreAll) | BIT(End),
      MASK_IGNORE
   };
#define STYLE_DEFAULT   -2
#define STYLE_UNKNOWN   -1

   static const sztok styletab[] = {
	{"CARTESIAN",    STYLE_CARTESIAN },
	{"CYLPOLAR",     STYLE_CYLPOLAR },
	{"DEFAULT",      STYLE_DEFAULT },
	{"DIVING",       STYLE_DIVING },
	{"IGNORE",       STYLE_IGNORE },
	{"NORMAL",       STYLE_NORMAL },
	{"NOSURVEY",     STYLE_NOSURVEY },
	{"PASSAGE",      STYLE_PASSAGE },
	{"TOPOFIL",      STYLE_NORMAL },
	{NULL,		 STYLE_UNKNOWN }
   };

#define m_multi (BIT(Station) | BIT(Count) | BIT(Depth))

   int style, k = 0;
   reading d;
   unsigned long mUsed = 0;
   int old_style = pcs->style;

   /* after a bad *data command ignore survey data until the next
    * *data command to avoid an avalanche of errors */
   pcs->recorded_style = pcs->style = STYLE_IGNORE;

   get_token();
   style = match_tok(styletab, TABSIZE(styletab));

   if (style == STYLE_DEFAULT) {
      default_style(pcs);
      return;
   }

   if (style == STYLE_IGNORE) {
      return;
   }

   if (style == STYLE_UNKNOWN) {
      if (s_empty(&token)) {
	 /* "*data" without arguments reinitialises the current style - useful
	  * when using *data passage as it provides a way to break the passage
	  * tube without having to repeat the full *data passage command.
	  */
	 pcs->recorded_style = pcs->style = style = old_style;
	 goto reinit_style;
      }
      /* TRANSLATORS: e.g. trying to refer to an invalid FNORD data style */
      compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP, /*Data style “%s” unknown*/65, s_str(&token));
      return;
   }

   skipblanks();
#ifndef NO_DEPRECATED
   /* Olde syntax had optional field for survey grade, so allow an omit
    * but issue a warning about it */
   if (isOmit(ch)) {
      static int data_depr_count = 0;
      if (data_depr_count < 5) {
	 compile_diagnostic(DIAG_WARN|DIAG_TOKEN, /*“*data %s %c …” is deprecated - use “*data %s …” instead*/104,
			    s_str(&token), ch, s_str(&token));
	 if (++data_depr_count == 5)
	    compile_diagnostic(DIAG_INFO, /*Further uses of this deprecated feature will not be reported*/95);
      }
      nextch();
   }
#endif

   int kMac = 6; /* minimum for NORMAL style */
   reading *new_order = osmalloc(kMac * sizeof(reading));
   char *style_name = s_steal(&token);
   do {
      filepos fp;
      get_pos(&fp);
      get_token();
      d = match_tok(dtab, TABSIZE(dtab));
      if (d == End && !s_empty(&token)) {
	 compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP,
			    /*Reading “%s” not allowed in data style “%s”*/63,
			    s_str(&token), style_name);
	 osfree(style_name);
	 osfree(new_order);
	 return;
      }

      /* only token allowed after IGNOREALL is NEWLINE */
      if (k && new_order[k - 1] == IgnoreAll && d != Newline) {
	 set_pos(&fp);
	 break;
      }
      /* Note: an unknown token is reported as trailing garbage */
      if (!TSTBIT(mask_all[style], d)) {
	 /* TRANSLATORS: a data "style" is something like NORMAL, DIVING, etc.
	  * a "reading" is one of FROM, TO, TAPE, COMPASS, CLINO for NORMAL
	  * style.  Neither "style" nor "reading" is a keyword in the program.
	  *
	  * This error complains about a "DEPTH" gauge reading in "NORMAL"
	  * style, for example.
	  */
	 compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP,
			    /*Reading “%s” not allowed in data style “%s”*/63,
			    s_str(&token), style_name);
	 osfree(style_name);
	 osfree(new_order);
	 return;
      }
      if (TSTBIT(mUsed, Newline) && TSTBIT(m_multi, d)) {
	 /* TRANSLATORS: caused by e.g.
	  *
	  * *data diving station newline depth tape compass
	  *
	  * ("depth" needs to occur before "newline"). */
	 compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP,
			    /*Reading “%s” must occur before NEWLINE*/225, s_str(&token));
	 osfree(style_name);
	 osfree(new_order);
	 return;
      }
      /* Check for duplicates unless it's a special reading:
       *   IGNOREALL,IGNORE (duplicates allowed) ; END (not possible)
       */
      if (!((BIT(Ignore) | BIT(End) | BIT(IgnoreAll)) & BIT(d))) {
	 if (TSTBIT(mUsed, d)) {
	    /* TRANSLATORS: complains about a situation like trying to define
	     * two from stations per leg */
	    compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP, /*Duplicate reading “%s”*/67, s_str(&token));
	    osfree(style_name);
	    osfree(new_order);
	    return;
	 } else {
	    /* Check for previously listed readings which are incompatible
	     * with this one - e.g. Count vs FrCount */
	    bool fBad = false;
	    switch (d) {
	     case Station:
	       if (mUsed & (BIT(Fr) | BIT(To))) fBad = true;
	       break;
	     case Fr: case To:
	       if (TSTBIT(mUsed, Station)) fBad = true;
	       break;
	     case Count:
	       if (mUsed & (BIT(FrCount) | BIT(ToCount) | BIT(Tape)))
		  fBad = true;
	       break;
	     case FrCount: case ToCount:
	       if (mUsed & (BIT(Count) | BIT(Tape)))
		  fBad = true;
	       break;
	     case Depth:
	       if (mUsed & (BIT(FrDepth) | BIT(ToDepth) | BIT(DepthChange)))
		  fBad = true;
	       break;
	     case FrDepth: case ToDepth:
	       if (mUsed & (BIT(Depth) | BIT(DepthChange))) fBad = true;
	       break;
	     case DepthChange:
	       if (mUsed & (BIT(FrDepth) | BIT(ToDepth) | BIT(Depth)))
		  fBad = true;
	       break;
	     case Newline:
	       if (mUsed & ~m_multi) {
		  /* TRANSLATORS: e.g.
		   *
		   * *data normal from to tape newline compass clino */
		  compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP, /*NEWLINE can only be preceded by STATION, DEPTH, and COUNT*/226);
		  osfree(style_name);
		  osfree(new_order);
		  return;
	       }
	       if (k == 0) {
		  /* TRANSLATORS: error from:
		   *
		   * *data normal newline from to tape compass clino */
		  compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP, /*NEWLINE can’t be the first reading*/222);
		  osfree(style_name);
		  osfree(new_order);
		  return;
	       }
	       break;
	     default: /* avoid compiler warnings about unhandled enums */
	       break;
	    }
	    if (fBad) {
	       /* Not entirely happy with phrasing this... */
	       /* TRANSLATORS: This is an error from the *DATA command.  It
		* means that a reading (which will appear where %s is isn't
		* valid as the list of readings has already included the same
		* reading, or an equivalent one (e.g. you can't have both
		* DEPTH and DEPTHCHANGE together). */
	       compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP, /*Reading “%s” duplicates previous reading(s)*/77,
				  s_str(&token));
	       osfree(style_name);
	       osfree(new_order);
	       return;
	    }
	    mUsed |= BIT(d); /* used to catch duplicates */
	 }
      }
      if (k && new_order[k - 1] == IgnoreAll) {
	 SVX_ASSERT(d == Newline);
	 k--;
	 d = IgnoreAllAndNewLine;
      }
      if (k >= kMac) {
	 kMac = kMac * 2;
	 new_order = osrealloc(new_order, kMac * sizeof(reading));
      }
      new_order[k++] = d;
   } while (d != End);

   if (k >= 2 && new_order[k - 2] == Newline) {
      /* TRANSLATORS: error from:
       *
       * *data normal from to tape compass clino newline */
      compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP, /*NEWLINE can’t be the last reading*/223);
      osfree(style_name);
      osfree(new_order);
      return;
   }

   if (style == STYLE_NOSURVEY) {
      if (TSTBIT(mUsed, Station)) {
	 if (k >= kMac) {
	    kMac = kMac * 2;
	    new_order = osrealloc(new_order, kMac * sizeof(reading));
	 }
	 new_order[k - 1] = Newline;
	 new_order[k++] = End;
      }
   } else if (style == STYLE_PASSAGE) {
      /* Station doesn't mean "multiline" for STYLE_PASSAGE. */
   } else if (!TSTBIT(mUsed, Newline) && (m_multi & mUsed)) {
      /* TRANSLATORS: Error given by something like:
       *
       * *data normal station tape compass clino
       *
       * ("station" signifies interleaved data). */
      compile_diagnostic(DIAG_ERR|DIAG_SKIP, /*Interleaved readings, but no NEWLINE*/224);
      osfree(style_name);
      osfree(new_order);
      return;
   }

#if 0
   printf("mUsed = 0x%x\n", mUsed);
#endif

   /* Check the supplied readings form a sufficient set. */
   if (style != STYLE_PASSAGE) {
       if ((mUsed & (BIT(Fr) | BIT(To))) == (BIT(Fr) | BIT(To)))
	   mUsed |= BIT(Station);
       else if (TSTBIT(mUsed, Station))
	   mUsed |= BIT(Fr) | BIT(To);
   }

   if (mUsed & (BIT(Comp) | BIT(BackComp)))
      mUsed |= BIT(Comp) | BIT(BackComp);

   if (mUsed & (BIT(Clino) | BIT(BackClino)))
      mUsed |= BIT(Clino) | BIT(BackClino);

   if ((mUsed & (BIT(FrDepth) | BIT(ToDepth))) == (BIT(FrDepth) | BIT(ToDepth)))
      mUsed |= BIT(Depth) | BIT(DepthChange);
   else if (mUsed & (BIT(Depth) | BIT(DepthChange)))
      mUsed |= BIT(FrDepth) | BIT(ToDepth) | BIT(Depth) | BIT(DepthChange);

   if ((mUsed & (BIT(FrCount) | BIT(ToCount))) == (BIT(FrCount) | BIT(ToCount)))
      mUsed |= BIT(Count) | BIT(Tape) | BIT(BackTape);
   else if (mUsed & (BIT(Count) | BIT(Tape) | BIT(BackTape)))
      mUsed |= BIT(FrCount) | BIT(ToCount) | BIT(Count) | BIT(Tape) | BIT(BackTape);

#if 0
   printf("mUsed = 0x%x, opt = 0x%x, mask = 0x%x\n", mUsed,
	  mask_optional[style], mask[style]);
#endif

   if (((mUsed &~ BIT(Newline)) | mask_optional[style]) != mask[style]) {
      /* Test should only fail with too few bits set, not too many */
      SVX_ASSERT((((mUsed &~ BIT(Newline)) | mask_optional[style])
	      &~ mask[style]) == 0);
      /* TRANSLATORS: i.e. not enough readings for the style. */
      compile_diagnostic(DIAG_ERR|DIAG_SKIP, /*Too few readings for data style “%s”*/64, style_name);
      osfree(style_name);
      osfree(new_order);
      return;
   }

   /* don't free default ordering or ordering used by parent */
   if (pcs->ordering != default_order &&
       !(pcs->next && pcs->next->ordering == pcs->ordering))
      osfree((reading*)pcs->ordering);

   pcs->recorded_style = pcs->style = style;
   pcs->ordering = new_order;

   osfree(style_name);

reinit_style:
   if (style == STYLE_PASSAGE) {
      lrudlist * new_psg = osnew(lrudlist);
      new_psg->tube = NULL;
      new_psg->next = model;
      model = new_psg;
      next_lrud = &(new_psg->tube);
   }
}

static void
cmd_units(void)
{
   int units, quantity;
   unsigned long qmask;
   unsigned long m; /* mask with bit x set to indicate quantity x specified */
   real factor;
   filepos fp;

   qmask = get_qlist(BIT(Q_POS)|BIT(Q_PLUMB)|BIT(Q_LEVEL));

   if (!qmask) return;
   if (qmask == BIT(Q_DEFAULT)) {
      default_units(pcs);
      return;
   }

   get_pos(&fp);
   factor = read_numeric(true);
   if (factor == 0.0) {
      set_pos(&fp);
      /* TRANSLATORS: error message given by "*units tape 0 feet" - it’s
       * meaningless to say your tape is marked in "0 feet" (but you might
       * measure distance by counting knots on a diving line, and tie them
       * every "2 feet"). */
      compile_diagnostic(DIAG_ERR|DIAG_WORD, /**UNITS factor must be non-zero*/200);
      skipline();
      return;
   }

   units = get_units(qmask, true);
   if (units == UNITS_NULL) return;
   if (TSTBIT(qmask, Q_GRADIENT))
      pcs->f_clino_percent = (units == UNITS_PERCENT);
   if (TSTBIT(qmask, Q_BACKGRADIENT))
      pcs->f_backclino_percent = (units == UNITS_PERCENT);

   if (TSTBIT(qmask, Q_BEARING)) {
      pcs->f_bearing_quadrants = (units == UNITS_QUADRANTS);
   }
   if (TSTBIT(qmask, Q_BACKBEARING)) {
      pcs->f_backbearing_quadrants = (units == UNITS_QUADRANTS);
   }

   if (factor == HUGE_REAL) {
      factor = factor_tab[units];
   } else {
      factor *= factor_tab[units];
   }

   for (quantity = 0, m = BIT(quantity); m <= qmask; quantity++, m <<= 1)
      if (qmask & m) pcs->units[quantity] = factor;
}

static void
cmd_calibrate(void)
{
   real sc, z;
   unsigned long qmask, m;
   int quantity;
   filepos fp;

   qmask = get_qlist(BIT(Q_POS)|BIT(Q_PLUMB)|BIT(Q_LEVEL));
   if (!qmask) return; /* error already reported */

   if (qmask == BIT(Q_DEFAULT)) {
      default_calib(pcs);
      return;
   }

   if (((qmask & LEN_QMASK)) && ((qmask & ANG_QMASK))) {
      /* TRANSLATORS: e.g.
       *
       * *calibrate tape compass 1 1
       */
      compile_diagnostic(DIAG_ERR|DIAG_SKIP, /*Can’t calibrate angular and length quantities together*/227);
      return;
   }

   z = read_numeric(false);
   get_pos(&fp);
   sc = read_numeric(true);
   if (sc == HUGE_REAL) {
      if (isalpha(ch)) {
	 int units = get_units(qmask, false);
	 if (units == UNITS_NULL) {
	    return;
	 }
	 z *= factor_tab[units];
	 sc = read_numeric(true);
	 if (sc == HUGE_REAL) {
	    sc = (real)1.0;
	 } else {
	    /* Adjustment applied is: (reading - z) * sc
	     * We want: reading * sc - z
	     * So divide z by sc so the applied adjustment does what we want.
	     */
	    z /= sc;
	 }
      } else {
	 sc = (real)1.0;
      }
   }

   if (sc == HUGE_REAL) sc = (real)1.0;
   /* check for declination scale */
   if (TSTBIT(qmask, Q_DECLINATION) && sc != 1.0) {
      set_pos(&fp);
      /* TRANSLATORS: DECLINATION is a built-in keyword, so best not to
       * translate */
      compile_diagnostic(DIAG_ERR|DIAG_WORD, /*Scale factor must be 1.0 for DECLINATION*/40);
      skipline();
      return;
   }
   if (sc == 0.0) {
      set_pos(&fp);
      /* TRANSLATORS: If the scale factor for an instrument is zero, then any
       * reading would be mapped to zero, which doesn't make sense. */
      compile_diagnostic(DIAG_ERR|DIAG_WORD, /*Scale factor must be non-zero*/391);
      skipline();
      return;
   }
   for (quantity = 0, m = BIT(quantity); m <= qmask; quantity++, m <<= 1) {
      if (qmask & m) {
	 pcs->z[quantity] = pcs->units[quantity] * z;
	 pcs->sc[quantity] = sc;
      }
   }
}

static const sztok north_tab[] = {
     { "GRID",		GRID_NORTH },
     { "MAGNETIC",	MAGNETIC_NORTH },
     { "TRUE",		TRUE_NORTH },
     { NULL,		-1 }
};

static void
cmd_cartesian(void)
{
    get_token();
    int north = match_tok(north_tab, TABSIZE(north_tab));
    if (north < 0) {
	compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP,
			   /*Expecting “%s”, “%s”, or “%s”*/188,
			   "GRID", "MAGNETIC", "TRUE");
	return;
    }
    pcs->cartesian_north = north;
    pcs->cartesian_rotation = 0.0;

    skipblanks();
    if (!isEol(ch) && !isComm(ch)) {
	real rotation = read_numeric(false);
	// Accept the same units as *declination does.
	int units = get_units(BIT(Q_DECLINATION), false);
	if (units == UNITS_NULL) {
	    return;
	}
	pcs->cartesian_rotation = rotation * factor_tab[units];
    }
}

static void
cmd_declination(void)
{
    real v = read_numeric(true);
    if (v == HUGE_REAL) {
	get_token_legacy_no_blanks();
	if (!S_EQ(&uctoken, "AUTO")) {
	    compile_diagnostic(DIAG_ERR|DIAG_SKIP|DIAG_COL, /*Expected number or “AUTO”*/309);
	    return;
	}
	do_legacy_token_warning();
	if (!pcs->proj_str) {
	    // TRANSLATORS: %s is replaced by the command that requires it, e.g.
	    // *DECLINATION AUTO
	    compile_diagnostic(DIAG_ERR|DIAG_SKIP|DIAG_TOKEN,
			       /*Input coordinate system must be specified for “%s”*/301,
			       "*DECLINATION AUTO");
	    return;
	}

	/* *declination auto X Y Z */
	real x = read_numeric(false);
	real y = read_numeric(false);
	real z = read_numeric(false);
	set_declination_location(x, y, z, pcs->proj_str);
    } else {
	/* *declination D UNITS */
	int units = get_units(BIT(Q_DECLINATION), false);
	if (units == UNITS_NULL) {
	    return;
	}
	pcs->z[Q_DECLINATION] = -v * factor_tab[units];
	pcs->convergence = 0;
    }
}

#ifndef NO_DEPRECATED
static void
cmd_default(void)
{
   static const sztok defaulttab[] = {
      { "CALIBRATE", CMD_CALIBRATE },
      { "DATA",      CMD_DATA },
      { "UNITS",     CMD_UNITS },
      { NULL,	     CMD_NULL }
   };
   static int default_depr_count = 0;

   if (default_depr_count < 5) {
      /* TRANSLATORS: If you're unsure what "deprecated" means, see:
       * https://en.wikipedia.org/wiki/Deprecation */
      compile_diagnostic(DIAG_WARN|DIAG_TOKEN, /**DEFAULT is deprecated - use *CALIBRATE/DATA/SD/UNITS with argument DEFAULT instead*/20);
      if (++default_depr_count == 5)
	 compile_diagnostic(DIAG_INFO, /*Further uses of this deprecated feature will not be reported*/95);
   }

   get_token();
   switch (match_tok(defaulttab, TABSIZE(defaulttab))) {
    case CMD_CALIBRATE:
      default_calib(pcs);
      break;
    case CMD_DATA:
      default_style(pcs);
      default_grade(pcs);
      pcs->cartesian_north = TRUE_NORTH;
      pcs->cartesian_rotation = 0.0;
      break;
    case CMD_UNITS:
      default_units(pcs);
      break;
    default:
      compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP, /*Unknown setting “%s”*/41,
			 s_str(&token));
   }
}
#endif

static void
cmd_include(void)
{
   char *pth = NULL;
   string fnm = S_INIT;
#ifndef NO_DEPRECATED
   prefix *root_store;
#endif
   int ch_store;

   pth = path_from_fnm(file.filename);

   read_string(&fnm);

#ifndef NO_DEPRECATED
   /* Since *begin / *end nesting cannot cross file boundaries we only
    * need to preserve the prefix if the deprecated *prefix command
    * can be used */
   root_store = root;
   root = pcs->Prefix; /* Root for include file is current prefix */
#endif
   ch_store = ch;

   data_file(pth, s_str(&fnm));

#ifndef NO_DEPRECATED
   root = root_store; /* and restore root */
#endif
   ch = ch_store;

   s_free(&fnm);
   osfree(pth);
}

static void
cmd_sd(void)
{
   real sd, variance;
   int units;
   unsigned long qmask, m;
   int quantity;
   qmask = get_qlist(BIT(Q_DECLINATION));
   if (!qmask) return; /* no quantities found - error already reported */

   if (qmask == BIT(Q_DEFAULT)) {
      default_grade(pcs);
      return;
   }
   sd = read_numeric(false);
   if (sd <= (real)0.0) {
      compile_diagnostic(DIAG_ERR|DIAG_SKIP|DIAG_COL, /*Standard deviation must be positive*/48);
      return;
   }
   units = get_units(qmask, false);
   if (units == UNITS_NULL) return;

   sd *= factor_tab[units];
   variance = sqrd(sd);

   for (quantity = 0, m = BIT(quantity); m <= qmask; quantity++, m <<= 1)
      if (qmask & m) pcs->Var[quantity] = variance;
}

static void
cmd_title(void)
{
   if (!fExplicitTitle && pcs->Prefix == root) {
       /* If we don't have an explicit title yet, and we're currently in the
	* root prefix, use this title explicitly. */
      fExplicitTitle = true;
      read_string(&survey_title);
   } else {
      /* parse and throw away this title (but still check rest of line) */
      string s = S_INIT;
      read_string(&s);
      s_free(&s);
   }
}

static const sztok case_tab[] = {
     {"PRESERVE", OFF},
     {"TOLOWER",  LOWER},
     {"TOUPPER",  UPPER},
     {NULL,       -1}
};

static void
cmd_case(void)
{
   int setting;
   get_token();
   setting = match_tok(case_tab, TABSIZE(case_tab));
   if (setting != -1) {
      pcs->Case = setting;
   } else {
      compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP, /*Found “%s”, expecting “PRESERVE”, “TOUPPER”, or “TOLOWER”*/10, s_str(&token));
   }
}

typedef enum {
    CS_NONE = -1,
    CS_CUSTOM,
    CS_EPSG,
    CS_ESRI,
    CS_EUR79Z30,
    CS_IJTSK,
    CS_IJTSK03,
    CS_JTSK,
    CS_JTSK03,
    CS_LAT,
    CS_LOCAL,
    CS_LONG,
    CS_OSGB,
    CS_S_MERC,
    CS_UTM
} cs_class;

static const sztok cs_tab[] = {
     {"CUSTOM",   CS_CUSTOM},
     {"EPSG",     CS_EPSG},	/* EPSG:<number> */
     {"ESRI",     CS_ESRI},	/* ESRI:<number> */
     {"EUR79Z30", CS_EUR79Z30},
     {"IJTSK",    CS_IJTSK},
     {"IJTSK03",  CS_IJTSK03},
     {"JTSK",     CS_JTSK},
     {"JTSK03",   CS_JTSK03},
     {"LAT",      CS_LAT},	/* LAT-LONG */
     {"LOCAL",    CS_LOCAL},
     {"LONG",     CS_LONG},	/* LONG-LAT */
     {"OSGB",     CS_OSGB},	/* OSGB:<H, N, O, S or T><A-Z except I> */
     {"S",        CS_S_MERC},	/* S-MERC */
     // UTM<zone><N or S or nothing> is handled separately to avoid needing 180
     // entries in this lookup table.
     {NULL,       CS_NONE}
};

static void
cmd_cs(void)
{
   char *proj_str = NULL;
   cs_class cs;
   int cs_sub = INT_MIN;
   filepos fp;
   bool output = false;
   enum { YES, NO, MAYBE } ok_for_output = YES;
   static bool had_cs = false;

   if (!had_cs) {
      had_cs = true;
      if (first_fix_name) {
	 compile_diagnostic_at(DIAG_ERR,
			       first_fix_filename, first_fix_line,
			       /*Station “%s” fixed before CS command first used*/442,
			       sprint_prefix(first_fix_name));
      }
   }

   skipblanks();
   get_pos(&fp);
   get_token_no_blanks();
   if (S_EQ(&uctoken, "OUT")) {
      output = true;
      skipblanks();
      get_pos(&fp);
      get_token_no_blanks();
   }

   if (s_len(&uctoken) > 3 &&
       memcmp(s_str(&uctoken), "UTM", 3) == 0 &&
       isdigit((unsigned char)s_str(&uctoken)[3])) {
       // The token starts "UTM" followed by a digit so handle that separately
       // to avoid needing 180 entries for UTM zones in the cs_tab lookup
       // table.
       cs = CS_UTM;
       // Reposition on the digit after "UTM".
       set_pos(&fp);
       nextch();
       nextch();
       nextch();
       unsigned n = read_uint();
       if (n >= 1 && n <= 60) {
	   int uch = toupper(ch);
	   cs_sub = (int)n;
	   if (uch == 'S') {
	       nextch();
	       cs_sub = -cs_sub;
	   } else if (uch == 'N') {
	       nextch();
	   }
       }
   } else {
       cs = match_tok(cs_tab, TABSIZE(cs_tab));
       switch (cs) {
	 case CS_NONE:
	   break;
	 case CS_CUSTOM:
	   ok_for_output = MAYBE;
	   get_pos(&fp);
	   string str = S_INIT;
	   read_string(&str);
	   proj_str = s_steal(&str);
	   cs_sub = 0;
	   break;
	 case CS_EPSG: case CS_ESRI:
	   ok_for_output = MAYBE;
	   if (ch == ':' && isdigit(nextch())) {
	       unsigned n = read_uint();
	       if (n < 1000000) {
		   cs_sub = (int)n;
	       }
	   }
	   break;
	 case CS_EUR79Z30:
	   cs_sub = 0;
	   break;
	 case CS_JTSK:
	 case CS_JTSK03:
	   ok_for_output = NO;
	   cs_sub = 0;
	   break;
	 case CS_IJTSK:
	 case CS_IJTSK03:
	   cs_sub = 0;
	   break;
	 case CS_LAT: case CS_LONG:
	   ok_for_output = NO;
	   if (ch == '-') {
	       nextch();
	       get_token_no_blanks();
	       cs_class cs2 = match_tok(cs_tab, TABSIZE(cs_tab));
	       if ((cs ^ cs2) == (CS_LAT ^ CS_LONG)) {
		   cs_sub = 0;
	       }
	   }
	   break;
	 case CS_LOCAL:
	   cs_sub = 0;
	   break;
	 case CS_OSGB:
	   if (ch == ':') {
	       int uch1 = toupper(nextch());
	       if (strchr("HNOST", uch1)) {
		   int uch2 = toupper(nextch());
		   if (uch2 >= 'A' && uch2 <= 'Z' && uch2 != 'I') {
		       int x, y;
		       nextch();
		       if (uch1 > 'I') --uch1;
		       uch1 -= 'A';
		       if (uch2 > 'I') --uch2;
		       uch2 -= 'A';
		       x = uch1 % 5;
		       y = uch1 / 5;
		       x = (x * 5) + uch2 % 5;
		       y = (y * 5) + uch2 / 5;
		       cs_sub = y * 25 + x;
		   }
	       }
	   }
	   break;
	 case CS_S_MERC:
	   if (ch == '-') {
	       nextch();
	       get_token_no_blanks();
	       if (S_EQ(&uctoken, "MERC")) {
		   cs_sub = 0;
	       }
	   }
	   break;
	 case CS_UTM:
	   // Handled outside of this switch, but avoid compiler warning about
	   // unhandled enumeration value.
	   break;
       }
   }
   if (cs_sub == INT_MIN || isalnum(ch)) {
      set_pos(&fp);
      compile_diagnostic(DIAG_ERR|DIAG_WORD, /*Unknown coordinate system*/434);
      skipline();
      return;
   }
   /* Actually handle the cs */
   switch (cs) {
      case CS_NONE:
	 break;
      case CS_CUSTOM:
	 /* proj_str already set */
	 break;
      case CS_EPSG:
	 proj_str = osmalloc(32);
	 snprintf(proj_str, 32, "EPSG:%d", cs_sub);
	 break;
      case CS_ESRI:
	 proj_str = osmalloc(32);
	 snprintf(proj_str, 32, "ESRI:%d", cs_sub);
	 break;
      case CS_EUR79Z30:
	 proj_str = osstrdup("+proj=utm +zone=30 +ellps=intl +towgs84=-86,-98,-119,0,0,0,0 +no_defs");
	 break;
      case CS_IJTSK:
	 proj_str = osstrdup("+proj=krovak +ellps=bessel +towgs84=570.8285,85.6769,462.842,4.9984,1.5867,5.2611,3.5623 +no_defs");
	 break;
      case CS_IJTSK03:
	 proj_str = osstrdup("+proj=krovak +ellps=bessel +towgs84=485.021,169.465,483.839,7.786342,4.397554,4.102655,0 +no_defs");
	 break;
      case CS_JTSK:
	 proj_str = osstrdup("+proj=krovak +czech +ellps=bessel +towgs84=570.8285,85.6769,462.842,4.9984,1.5867,5.2611,3.5623 +no_defs");
	 break;
      case CS_JTSK03:
	 proj_str = osstrdup("+proj=krovak +czech +ellps=bessel +towgs84=485.021,169.465,483.839,7.786342,4.397554,4.102655,0 +no_defs");
	 break;
      case CS_LAT:
	 /* FIXME: Requires PROJ >= 4.8.0 for +axis, and the SDs will be
	  * misapplied, so we may want to swap ourselves.  Also, while
	  * therion supports lat-long, I'm not totally convinced that it is
	  * sensible to do so - people often say "lat-long", but probably
	  * don't think that that's actually "Northing, Easting".  This
	  * seems like it'll result in people accidentally getting X and Y
	  * swapped in their fixed points...
	  */
#if 0
	 proj_str = osstrdup("+proj=longlat +ellps=WGS84 +datum=WGS84 +axis=neu +no_defs");
#endif
	 break;
      case CS_LOCAL:
	 /* FIXME: Is it useful to be able to explicitly specify this? */
	 break;
      case CS_LONG:
	 proj_str = osstrdup("EPSG:4326");
	 break;
      case CS_OSGB: {
	 int x = 14 - (cs_sub % 25);
	 int y = (cs_sub / 25) - 20;
	 proj_str = osmalloc(160);
	 snprintf(proj_str, 160,
		  "+proj=tmerc +lat_0=49 +lon_0=-2 +k=0.9996012717 "
		  "+x_0=%d +y_0=%d +ellps=airy +datum=OSGB36 +units=m +no_defs",
		  x * 100000, y * 100000);
	 break;
      }
      case CS_S_MERC:
	 proj_str = osstrdup("EPSG:3857");
	 break;
      case CS_UTM:
	 proj_str = osmalloc(32);
	 if (cs_sub > 0) {
	    snprintf(proj_str, 32, "EPSG:%d", 32600 + cs_sub);
	 } else {
	    snprintf(proj_str, 32, "EPSG:%d", 32700 - cs_sub);
	 }
	 break;
   }

   if (!proj_str) {
      /* printf("CS %d:%d\n", (int)cs, cs_sub); */
      set_pos(&fp);
      compile_diagnostic(DIAG_ERR|DIAG_STRING, /*Unknown coordinate system*/434);
      skipline();
      return;
   }

   if (output) {
      if (ok_for_output == NO) {
	 set_pos(&fp);
	 compile_diagnostic(DIAG_ERR|DIAG_STRING, /*Coordinate system unsuitable for output*/435);
	 skipline();
	 return;
      }

      if (proj_str_out && strcmp(proj_str, proj_str_out) == 0) {
	  /* Same as the output cs that's already set, so nothing to do. */
	  osfree(proj_str);
	  return;
      }

      if (ok_for_output == MAYBE) {
	  /* We only actually create the transformation from input to output when
	   * we need it, but for a custom proj string or EPSG/ESRI code we need
	   * to check that the specified coordinate system is valid and also if
	   * it's suitable for output so we need to test creating it here.
	   */
	  proj_errno_reset(NULL);
	  PJ* pj = proj_create(PJ_DEFAULT_CTX, proj_str);
	  if (!pj) {
	      set_pos(&fp);
	      compile_diagnostic(DIAG_ERR|DIAG_STRING, /*Invalid coordinate system: %s*/443,
				 proj_context_errno_string(PJ_DEFAULT_CTX,
							   proj_context_errno(PJ_DEFAULT_CTX)));
	      skipline();
	      osfree(proj_str);
	      return;
	  }
	  int type = proj_get_type(pj);
	  if (type == PJ_TYPE_GEOGRAPHIC_2D_CRS ||
	      type == PJ_TYPE_GEOGRAPHIC_3D_CRS) {
	      set_pos(&fp);
	      compile_diagnostic(DIAG_ERR|DIAG_STRING, /*Coordinate system unsuitable for output*/435);
	      skipline();
	      osfree(proj_str);
	      return;
	  }
      }

      if (proj_str_out) {
	  /* If the output cs is already set, subsequent attempts to set it
	   * are silently ignored (so you can combine two datasets and set
	   * the output cs to use before you include either).
	   */
	  osfree(proj_str);
      } else {
	  proj_str_out = proj_str;
      }
   } else {
      if (proj_str_out && strcmp(proj_str, proj_str_out) == 0) {
	 /* Same as the current output projection, so valid for input. */
      } else if (pcs->proj_str && strcmp(proj_str, pcs->proj_str) == 0) {
	 /* Same as the current input projection, so nothing to do! */
	 return;
      } else if (ok_for_output == MAYBE) {
	 /* (ok_for_output == MAYBE) also happens to indicate whether we need
	  * to check that the coordinate system is valid for input.
	  */
	 proj_errno_reset(NULL);
	 PJ* pj = proj_create(PJ_DEFAULT_CTX, proj_str);
	 if (!pj) {
	    set_pos(&fp);
	    compile_diagnostic(DIAG_ERR|DIAG_STRING, /*Invalid coordinate system: %s*/443,
			       proj_context_errno_string(PJ_DEFAULT_CTX,
							 proj_context_errno(PJ_DEFAULT_CTX)));
	    skipline();
	    return;
	 }
	 proj_destroy(pj);
      }

      /* Free current input proj_str if not used by parent. */
      settings * p = pcs;
      if (!p->next || p->proj_str != p->next->proj_str)
	 osfree(p->proj_str);
      p->proj_str = proj_str;
      p->input_convergence = HUGE_REAL;
      invalidate_pj_cached();
   }
}

static const sztok infer_tab[] = {
     { "EQUATES",	INFER_EQUATES },
     { "EXPORTS",	INFER_EXPORTS },
     { "PLUMBS",	INFER_PLUMBS },
#if 0 /* FIXME */
     { "SUBSURVEYS",	INFER_SUBSURVEYS },
#endif
     { NULL,		INFER_NULL }
};

static const sztok onoff_tab[] = {
     { "OFF", 0 },
     { "ON",  1 },
     { NULL, -1 }
};

static void
cmd_infer(void)
{
   infer_what setting;
   int on;
   get_token();
   setting = match_tok(infer_tab, TABSIZE(infer_tab));
   if (setting == INFER_NULL) {
      compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP, /*Found “%s”, expecting “EQUATES”, “EXPORTS”, or “PLUMBS”*/31, s_str(&token));
      return;
   }
   get_token();
   on = match_tok(onoff_tab, TABSIZE(onoff_tab));
   if (on == -1) {
      compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP, /*Found “%s”, expecting “ON” or “OFF”*/32, s_str(&token));
      return;
   }

   if (on) {
      pcs->infer |= BIT(setting);
      if (setting == INFER_EXPORTS) fExportUsed = true;
   } else {
      pcs->infer &= ~BIT(setting);
   }
}

static void
cmd_truncate(void)
{
   unsigned int truncate_at = 0; /* default is no truncation */
   filepos fp;

   get_pos(&fp);

   get_token();
   if (!S_EQ(&uctoken, "OFF")) {
      if (!s_empty(&uctoken)) set_pos(&fp);
      truncate_at = read_uint();
   }
   /* for backward compatibility, "*truncate 0" means "*truncate off" */
   pcs->Truncate = (truncate_at == 0) ? INT_MAX : truncate_at;
}

static void
cmd_ref(void)
{
   /* Just syntax check for now. */
   string ref = S_INIT;
   read_string(&ref);
   s_free(&ref);
}

static void
cmd_require(void)
{
    // Add extra 0 so `*require 1.4.10.1` fails with cavern version 1.4.10.
    const unsigned version[] = {COMMAVERSION, 0};

    skipblanks();
    filepos fp;
    get_pos(&fp);

    // Parse the required version number, storing its components in
    // required_version.  We only store at most one more component than
    // COMMAVERSION has since more than that can't affect the comparison.
    size_t i = 0;
    int diff = 0;
    while (1) {
	unsigned component = read_uint();
	if (diff == 0 && i < sizeof(version) / sizeof(version[0])) {
	    if (diff == 0) {
		diff = (int)version[i++] - (int)component;
	    }
	}
	if (ch != '.' || isBlank(nextch()) || isComm(ch) || isEol(ch))
	    break;
    }

    if (diff < 0) {
	// Requirement not satisfied
	size_t len = (size_t)(ftell(file.fh) - fp.offset);
	char *v = osmalloc(len + 1);
	set_pos(&fp);
	for (size_t j = 0; j < len; j++) {
	    v[j] = ch;
	    nextch();
	}
	v[len] = '\0';
	/* TRANSLATORS: Feel free to translate as "or newer" instead of "or
	 * greater" if that gives a more natural translation.  It's
	 * technically not quite right when there are parallel active release
	 * series (e.g. Survex 1.0.40 was released *after* 1.2.0), but this
	 * seems unlikely to confuse users.  "Survex" is the name of the
	 * software, so should not be translated.
	 *
	 * Here "survey" is a "cave map" rather than list of questions - it should be
	 * translated to the terminology that cavers using the language would use.
	 */
	compile_diagnostic(DIAG_FATAL|DIAG_FROM(fp), /*Survex version %s or greater required to process this survey data.*/2, v);
	// Does not return so no point freeing v here.
    }
}

/* allocate new meta_data if need be */
void
copy_on_write_meta(settings *s)
{
   if (!s->meta || s->meta->ref_count != 0) {
       meta_data * meta_new = osnew(meta_data);
       if (!s->meta) {
	   meta_new->days1 = meta_new->days2 = -1;
       } else {
	   *meta_new = *(s->meta);
       }
       meta_new->ref_count = 0;
       s->meta = meta_new;
   }
}

static void
cmd_date(void)
{
    int year, month, day;
    int days1, days2;
    bool implicit_range = false;
    filepos fp, fp2;

    get_pos(&fp);
    read_date(&year, &month, &day);
    days1 = days_since_1900(year, month ? month : 1, day ? day : 1);

    if (days1 > current_days_since_1900) {
	set_pos(&fp);
	compile_diagnostic(DIAG_WARN|DIAG_DATE, /*Date is in the future!*/80);
    }

    skipblanks();
    if (ch == '-') {
	nextch();
	get_pos(&fp2);
	read_date(&year, &month, &day);
    } else {
	if (month && day) {
	    days2 = days1;
	    goto read;
	}
	implicit_range = true;
    }

    if (month == 0) month = 12;
    if (day == 0) day = last_day(year, month);
    days2 = days_since_1900(year, month, day);

    if (!implicit_range && days2 > current_days_since_1900) {
	set_pos(&fp2);
	compile_diagnostic(DIAG_WARN|DIAG_DATE, /*Date is in the future!*/80);
    }

    if (days2 < days1) {
	set_pos(&fp);
	compile_diagnostic(DIAG_ERR|DIAG_WORD, /*End of date range is before the start*/81);
	int tmp = days1;
	days1 = days2;
	days2 = tmp;
    }

read:
    if (!pcs->meta || pcs->meta->days1 != days1 || pcs->meta->days2 != days2) {
	copy_on_write_meta(pcs);
	pcs->meta->days1 = days1;
	pcs->meta->days2 = days2;
	/* Invalidate cached declination. */
	pcs->declination = HUGE_REAL;
    }
}

typedef void (*cmd_fn)(void);

static const cmd_fn cmd_funcs[] = {
   cmd_alias,
   cmd_begin,
   cmd_calibrate,
   cmd_cartesian,
   cmd_case,
   skipline, /*cmd_copyright,*/
   cmd_cs,
   cmd_data,
   cmd_date,
   cmd_declination,
#ifndef NO_DEPRECATED
   cmd_default,
#endif
   cmd_end,
   cmd_entrance,
   cmd_equate,
   cmd_export,
   cmd_fix,
   cmd_flags,
   cmd_include,
   cmd_infer,
   skipline, /*cmd_instrument,*/
#ifndef NO_DEPRECATED
   cmd_prefix,
#endif
   cmd_ref,
   cmd_require,
   cmd_sd,
   cmd_set,
   solve_network,
   skipline, /*cmd_team,*/
   cmd_title,
   cmd_truncate,
   cmd_units
};

extern void
handle_command(void)
{
   filepos fp;
   get_pos(&fp);
   get_token_legacy();
   int cmdtok = match_tok(cmd_tab, TABSIZE(cmd_tab));
   if (cmdtok < 0 || cmdtok >= (int)(sizeof(cmd_funcs) / sizeof(cmd_fn))) {
      set_pos(&fp);
      get_token();
      compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP, /*Unknown command “%s”*/12, s_str(&token));
      return;
   }

   do_legacy_token_warning();

   switch (cmdtok) {
    case CMD_EXPORT:
      if (!f_export_ok)
	 /* TRANSLATORS: The *EXPORT command is only valid just after *BEGIN
	  * <SURVEY>, so this would generate this error:
	  *
	  * *begin fred
	  * 1 2 1.23 045 -6
	  * *export 2
	  * *end fred */
	 compile_diagnostic(DIAG_ERR, /**EXPORT must immediately follow “*BEGIN <SURVEY>”*/57);
      break;
    case CMD_ALIAS:
    case CMD_CALIBRATE:
    case CMD_CARTESIAN:
    case CMD_CASE:
    case CMD_COPYRIGHT:
    case CMD_CS:
    case CMD_DATA:
    case CMD_DATE:
    case CMD_DECLINATION:
    case CMD_DEFAULT:
    case CMD_FLAGS:
    case CMD_INFER:
    case CMD_INSTRUMENT:
    case CMD_REF:
    case CMD_REQUIRE:
    case CMD_SD:
    case CMD_SET:
    case CMD_TEAM:
    case CMD_TITLE:
    case CMD_TRUNCATE:
    case CMD_UNITS:
      /* These can occur between *begin and *export */
      break;
    default:
      /* NB: additional handling for "*begin <survey>" in cmd_begin */
      f_export_ok = false;
      break;
   }

   cmd_funcs[cmdtok]();
}
