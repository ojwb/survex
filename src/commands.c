/* commands.c
 * Code for directives
 * Copyright (C) 1991-2003,2004,2005,2006,2010,2011,2012,2013,2014,2015,2016,2019 Olly Betts
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <limits.h>
#include <stddef.h> /* for offsetof */
#include <string.h>

#ifdef HAVE_PROJ_H
/* Work around broken check in proj.h:
 * https://github.com/OSGeo/PROJ/issues/1523
 */
# ifndef PROJ_H
#  include <proj.h>
# endif
#endif
#define ACCEPT_USE_OF_DEPRECATED_PROJ_API_H 1
#include <proj_api.h>

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

/*** Extracted from proj.4 projects (yuck, but grass also does this): */
struct DERIVS {
    double x_l, x_p; /* derivatives of x for lambda-phi */
    double y_l, y_p; /* derivatives of y for lambda-phi */
};

struct FACTORS {
    struct DERIVS der;
    double h, k;	/* meridinal, parallel scales */
    double omega, thetap;	/* angular distortion, theta prime */
    double conv;	/* convergence */
    double s;		/* areal scale factor */
    double a, b;	/* max-min scale error */
    int code;		/* info as to analytics, see following */
};

int pj_factors(projLP, projPJ *, double, struct FACTORS *);
/***/

static projPJ proj_wgs84;

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
   s->style = STYLE_NORMAL;
   s->ordering = default_order;
   s->dash_for_anon_wall_station = fFalse;
}

static void
default_prefix(settings *s)
{
   s->Prefix = root;
}

static void
default_translate(settings *s)
{
   int i;
   short *t;
   if (s->next && s->next->Translate == s->Translate) {
      t = ((short*)osmalloc(ossizeof(short) * 257)) + 1;
      memcpy(t - 1, s->Translate - 1, sizeof(short) * 257);
      s->Translate = t;
   }
/*  SVX_ASSERT(EOF==-1);*/ /* important, since we rely on this */
   t = s->Translate;
   memset(t - 1, 0, sizeof(short) * 257);
   for (i = '0'; i <= '9'; i++) t[i] = SPECIAL_NAMES;
   for (i = 'A'; i <= 'Z'; i++) t[i] = SPECIAL_NAMES;
   for (i = 'a'; i <= 'z'; i++) t[i] = SPECIAL_NAMES;

   t['\t'] |= SPECIAL_BLANK;
   t[' '] |= SPECIAL_BLANK;
   t[','] |= SPECIAL_BLANK;
   t[';'] |= SPECIAL_COMMENT;
   t['\032'] |= SPECIAL_EOL; /* Ctrl-Z, so olde DOS text files are handled ok */
   t[EOF] |= SPECIAL_EOL;
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
   s->f_clino_percent = s->f_backclino_percent = fFalse;
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

char *buffer = NULL;
static int buf_len;

static char *ucbuffer = NULL;

/* read token */
extern void
get_token(void)
{
   skipblanks();
   get_token_no_blanks();
}

extern void
get_token_no_blanks(void)
{
   int i = -1;

   s_zero(&buffer);
   osfree(ucbuffer);
   while (isalpha(ch)) {
      s_catchar(&buffer, &buf_len, (char)ch);
      nextch();
   }

   if (!buffer) s_catchar(&buffer, &buf_len, '\0');

   ucbuffer = osmalloc(buf_len);
   do {
      i++;
      ucbuffer[i] = toupper(buffer[i]);
   } while (buffer[i]);
#if 0
   printf("get_token_no_blanks() got “%s”\n", buffer);
#endif
}

/* read word */
static void
get_word(void)
{
   s_zero(&buffer);
   skipblanks();
   while (!isBlank(ch) && !isEol(ch)) {
      s_catchar(&buffer, &buf_len, (char)ch);
      nextch();
   }

   if (!buffer) s_catchar(&buffer, &buf_len, '\0');
#if 0
   printf("get_word() got “%s”\n", buffer);
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
   assert(tab_size > 0); /* catch empty table */
/*  printf("[%d,%d]",a,b); */
   while (a <= b) {
      c = (unsigned)(a + b) / 2;
/*     printf(" %d",c); */
      r = strcmp(tab[c].sz, ucbuffer);
      if (r == 0) return tab[c].tok; /* match */
      if (r < 0)
	 a = c + 1;
      else
	 b = c - 1;
   }
   return tab[tab_size].tok; /* no match */
}

typedef enum {
   CMD_NULL = -1, CMD_ALIAS, CMD_BEGIN, CMD_CALIBRATE, CMD_CASE, CMD_COPYRIGHT,
   CMD_CS, CMD_DATA, CMD_DATE, CMD_DECLINATION, CMD_DEFAULT, CMD_END,
   CMD_ENTRANCE, CMD_EQUATE, CMD_EXPORT, CMD_FIX, CMD_FLAGS, CMD_INCLUDE,
   CMD_INFER, CMD_INSTRUMENT, CMD_PREFIX, CMD_REF, CMD_REQUIRE, CMD_SD,
   CMD_SET, CMD_SOLVE, CMD_TEAM, CMD_TITLE, CMD_TRUNCATE, CMD_UNITS
} cmds;

static const sztok cmd_tab[] = {
     {"ALIAS",     CMD_ALIAS},
     {"BEGIN",     CMD_BEGIN},
     {"CALIBRATE", CMD_CALIBRATE},
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
   (M_PI/180.0), (M_PI/200.0), 0.01, (M_PI/180.0/60.0)
};

const int units_to_msgno[] = {
    /*m*/424,
    /*ft*/428,
    -1, /* yards */
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
	{"YARDS",	  UNITS_YARDS },
	{NULL,		  UNITS_NULL }
   };
   int units;
   get_token();
   units = match_tok(utab, TABSIZE(utab));
   if (units == UNITS_NULL) {
      compile_diagnostic(DIAG_ERR|DIAG_BUF|DIAG_SKIP, /*Unknown units “%s”*/35, buffer);
      return UNITS_NULL;
   }
   /* Survex has long misdefined "mils" as an alias for "grads", of which
    * there are 400 in a circle.  There are several definitions of "mils"
    * with a circle containing 2000π SI milliradians, 6400 NATO mils, 6000
    * Warsaw Pact mils, and 6300 Swedish streck, and they aren't in common
    * use by cave surveyors, so we now just warn if mils are used.
    */
   if (units == UNITS_DEPRECATED_ALIAS_FOR_GRADS) {
      compile_diagnostic(DIAG_WARN|DIAG_BUF|DIAG_SKIP,
			 /*Units “%s” are deprecated, assuming “grads” - see manual for details*/479,
			 buffer);
      units = UNITS_GRADS;
   }
   if (units == UNITS_PERCENT && percent_ok &&
       !(qmask & ~(BIT(Q_GRADIENT)|BIT(Q_BACKGRADIENT)))) {
      return units;
   }
   if (((qmask & LEN_QMASK) && !TSTBIT(LEN_UMASK, units)) ||
       ((qmask & ANG_QMASK) && !TSTBIT(ANG_UMASK, units))) {
      /* TRANSLATORS: Note: In English you talk about the *units* of a single
       * measurement, but the correct term in other languages may be singular.
       */
      compile_diagnostic(DIAG_ERR|DIAG_BUF|DIAG_SKIP, /*Invalid units “%s” for quantity*/37, buffer);
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
      get_token();
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
	 compile_diagnostic(DIAG_ERR|DIAG_BUF|DIAG_SKIP, /*Unknown instrument “%s”*/39, buffer);
	 return 0;
      }
   }

   if (qmask == 0) {
      /* TRANSLATORS: A "quantity" is something measured like "LENGTH",
       * "BEARING", "ALTITUDE", etc. */
      compile_diagnostic(DIAG_ERR|DIAG_BUF|DIAG_SKIP, /*Unknown quantity “%s”*/34, buffer);
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
   int mask;
   int i;

   get_token();
   mask = match_tok(chartab, TABSIZE(chartab));

   if (mask == SPECIAL_UNKNOWN) {
      compile_diagnostic(DIAG_ERR|DIAG_BUF|DIAG_SKIP, /*Unknown character class “%s”*/42, buffer);
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
	 compile_diagnostic(DIAG_WARN|DIAG_BUF, /*ROOT is deprecated*/25);
	 if (++root_depr_count == 5)
	     /* TRANSLATORS: If you're unsure what "deprecated" means, see:
	      * https://en.wikipedia.org/wiki/Deprecation */
	    compile_diagnostic(DIAG_WARN, /*Further uses of this deprecated feature will not be reported*/95);
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
      if (!isalnum(ch)) {
	 pcs->Translate[ch] |= mask;
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
	 pcs->Translate[hex] |= mask;
      } else {
	 break;
      }
      nextch();
   }
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
       * crawl.svx:4: Reentering an existing survey is deprecated
       * crawl.svx:1: Originally entered here
       *
       * If you're unsure what "deprecated" means, see:
       * https://en.wikipedia.org/wiki/Deprecation */
      compile_diagnostic(DIAG_WARN|DIAG_TOKEN, /*Reentering an existing survey is deprecated*/29);
      set_pos(&fp_tmp);
      /* TRANSLATORS: The second of two warnings given when a survey which has
       * already been completed is reentered.  This example file crawl.svx:
       *
       * *begin crawl
       * 1 2 9.45 234 -01 # <- second warning here
       * *end crawl
       * *begin crawl     # <- first warning here
       * 2 3 7.67 223 -03
       * *end crawl
       *
       * Would lead to:
       *
       * crawl.svx:3: Reentering an existing survey is deprecated
       * crawl.svx:1: Originally entered here
       *
       * If you're unsure what "deprecated" means, see:
       * https://en.wikipedia.org/wiki/Deprecation */
      compile_diagnostic_pfx(DIAG_WARN, survey, /*Originally entered here*/30);
      if (++reenter_depr_count == 5) {
	 /* After we've warned about 5 uses of the same deprecated feature, we
	  * give up for the rest of the current processing run.
	  *
	  * If you're unsure what "deprecated" means, see:
	  * https://en.wikipedia.org/wiki/Deprecation */
	 compile_diagnostic(DIAG_WARN, /*Further uses of this deprecated feature will not be reported*/95);
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
      compile_diagnostic(DIAG_WARN|DIAG_BUF, /**prefix is deprecated - use *begin and *end instead*/6);
      if (++prefix_depr_count == 5)
	 compile_diagnostic(DIAG_WARN, /*Further uses of this deprecated feature will not be reported*/95);
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
   if (strcmp(ucbuffer, "STATION") != 0)
      goto bad;
   get_word();
   if (strcmp(buffer, "-") != 0)
      goto bad;
   get_word();
   if (*buffer && strcmp(buffer, "..") != 0)
      goto bad;
   pcs->dash_for_anon_wall_station = (*buffer != '\0');
   return;
bad:
   compile_diagnostic(DIAG_ERR|DIAG_SKIP, /*Bad *alias command*/397);
}

static void
cmd_begin(void)
{
   settings *pcsNew;

   pcsNew = osnew(settings);
   *pcsNew = *pcs; /* copy contents */
   pcsNew->begin_lineno = file.line;
   pcsNew->next = pcs;
   pcs = pcsNew;

   skipblanks();
   pcs->begin_survey = NULL;
   if (!isEol(ch) && !isComm(ch)) {
      filepos fp;
      prefix *survey;
      get_pos(&fp);
      survey = read_prefix(PFX_SURVEY|PFX_ALLOW_ROOT|PFX_WARN_SEPARATOR);
      pcs->begin_survey = survey;
      pcs->Prefix = survey;
      check_reentry(survey, &fp);
      f_export_ok = fTrue;
   }
}

extern void
free_settings(settings *p) {
   /* don't free default ordering or ordering used by parent */
   const reading *order = p->ordering;
   if (order != default_order && (!p->next || order != p->next->ordering))
      osfree((reading*)order);

   /* free Translate if not used by parent */
   if (!p->next || p->Translate != p->next->Translate)
      osfree(p->Translate - 1);

   /* free meta if not used by parent, or in this block */
   if (p->meta && (!p->next || p->meta != p->next->meta) && p->meta->ref_count == 0)
       osfree(p->meta);

   /* free proj if not used by parent, or as the output projection */
   if (p->proj && (!p->next || p->proj != p->next->proj) && p->proj != proj_out)
       pj_free(p->proj);

   osfree(p);
}

static void
cmd_end(void)
{
   settings *pcsParent;
   prefix *survey, *begin_survey;
   filepos fp;

   pcsParent = pcs->next;

   if (pcs->begin_lineno == 0) {
      if (pcsParent == NULL) {
	 /* more ENDs than BEGINs */
	 compile_diagnostic(DIAG_ERR|DIAG_SKIP, /*No matching BEGIN*/192);
      } else {
	 compile_diagnostic(DIAG_ERR|DIAG_SKIP, /*END with no matching BEGIN in this file*/22);
      }
      return;
   }

   begin_survey = pcs->begin_survey;

   SVX_ASSERT(pcsParent);
   free_settings(pcs);
   pcs = pcsParent;

   /* note need to read using root *before* BEGIN */
   skipblanks();
   if (isEol(ch) || isComm(ch)) {
      survey = NULL;
   } else {
      get_pos(&fp);
      survey = read_prefix(PFX_SURVEY|PFX_ALLOW_ROOT);
   }

   if (survey != begin_survey) {
      if (survey) {
	 set_pos(&fp);
	 if (!begin_survey) {
	    /* TRANSLATORS: Used when a BEGIN command has no survey, but the
	     * END command does, e.g.:
	     *
	     * *begin
	     * 1 2 10.00 178 -01
	     * *end entrance      <--[Message given here] */
	    compile_diagnostic(DIAG_ERR|DIAG_TOKEN, /*Matching BEGIN command has no survey name*/36);
	 } else {
	    /* TRANSLATORS: *BEGIN <survey> and *END <survey> should have the
	     * same <survey> if it’s given at all */
	    compile_diagnostic(DIAG_ERR|DIAG_TOKEN, /*Survey name doesn’t match BEGIN*/193);
	 }
	 skipline();
      } else {
	 /* TRANSLATORS: Used when a BEGIN command has a survey name, but the
	  * END command omits it, e.g.:
	  *
	  * *begin entrance
	  * 1 2 10.00 178 -01
	  * *end     <--[Message given here] */
	 compile_diagnostic(DIAG_WARN|DIAG_COL, /*Survey name omitted from END*/194);
      }
   }
}

static void
cmd_entrance(void)
{
   prefix *pfx = read_prefix(PFX_STATION);
   pfx->sflags |= BIT(SFLAGS_ENTRANCE);
}

static const prefix * first_fix_name = NULL;
static const char * first_fix_filename;
static unsigned first_fix_line;

static void
cmd_fix(void)
{
   prefix *fix_name;
   node *stn = NULL;
   static prefix *name_omit_already = NULL;
   static const char * name_omit_already_filename = NULL;
   static unsigned int name_omit_already_line;
   real x, y, z;
   filepos fp;

   fix_name = read_prefix(PFX_STATION|PFX_ALLOW_ROOT);
   fix_name->sflags |= BIT(SFLAGS_FIXED);

   get_pos(&fp);
   get_token();
   if (strcmp(ucbuffer, "REFERENCE") == 0) {
      /* suppress "unused fixed point" warnings for this station */
      fix_name->sflags |= BIT(SFLAGS_USED);
   } else {
      if (*ucbuffer) set_pos(&fp);
   }

   x = read_numeric(fTrue);
   if (x == HUGE_REAL) {
      /* If the end of the line isn't blank, read a number after all to
       * get a more helpful error message */
      if (!isEol(ch) && !isComm(ch)) x = read_numeric(fFalse);
   }
   if (x == HUGE_REAL) {
      if (pcs->proj || proj_out) {
	 compile_diagnostic(DIAG_ERR|DIAG_COL|DIAG_SKIP, /*Coordinates can't be omitted when coordinate system has been specified*/439);
	 return;
      }

      if (fix_name == name_omit_already) {
	 compile_diagnostic(DIAG_WARN|DIAG_COL, /*Same station fixed twice with no coordinates*/61);
	 return;
      }

      /* TRANSLATORS: " *fix a " gives this message: */
      compile_diagnostic(DIAG_WARN|DIAG_COL, /*FIX command with no coordinates - fixing at (0,0,0)*/54);

      if (name_omit_already) {
	 /* TRANSLATORS: Emitted after second and subsequent "FIX command with
	  * no coordinates - fixing at (0,0,0)" warnings.
	  */
	 compile_diagnostic_at(DIAG_ERR|DIAG_COL,
			       name_omit_already_filename,
			       name_omit_already_line,
			       /*Already had FIX command with no coordinates for station “%s”*/441,
			       sprint_prefix(name_omit_already));
      } else {
	 name_omit_already = fix_name;
	 name_omit_already_filename = file.filename;
	 name_omit_already_line = file.line;
      }

      x = y = z = (real)0.0;
   } else {
      real sdx;
      y = read_numeric(fFalse);
      z = read_numeric(fFalse);

      if (pcs->proj && proj_out) {
	 if (pj_is_latlong(pcs->proj)) {
	    /* PROJ expects lat and long in radians. */
	    x = rad(x);
	    y = rad(y);
	 }
	 int r = pj_transform(pcs->proj, proj_out, 1, 1, &x, &y, &z);
	 if (r != 0) {
	    compile_diagnostic(DIAG_ERR, /*Failed to convert coordinates: %s*/436, pj_strerrno(r));
	 }
      } else if (pcs->proj) {
	 compile_diagnostic(DIAG_ERR, /*The input projection is set but the output projection isn't*/437);
      } else if (proj_out) {
	 compile_diagnostic(DIAG_ERR, /*The output projection is set but the input projection isn't*/438);
      }

      get_pos(&fp);
      sdx = read_numeric(fTrue);
      if (sdx <= 0) {
	  set_pos(&fp);
	  compile_diagnostic(DIAG_ERR|DIAG_SKIP|DIAG_NUM, /*Standard deviation must be positive*/48);
	  return;
      }
      if (sdx != HUGE_REAL) {
	 real sdy, sdz;
	 real cxy = 0, cyz = 0, czx = 0;
	 get_pos(&fp);
	 sdy = read_numeric(fTrue);
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
	    sdz = read_numeric(fTrue);
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
	       cxy = read_numeric(fTrue);
	       if (cxy != HUGE_REAL) {
		  /* covariances given */
		  cyz = read_numeric(fFalse);
		  czx = read_numeric(fFalse);
	       } else {
		  cxy = 0;
	       }
	    }
	 }
	 stn = StnFromPfx(fix_name);
	 if (!fixed(stn)) {
	    node *fixpt = osnew(node);
	    prefix *name;
	    name = osnew(prefix);
	    name->pos = osnew(pos);
	    name->ident = NULL;
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
	    POS(fixpt, 0) = x;
	    POS(fixpt, 1) = y;
	    POS(fixpt, 2) = z;
	    fix(fixpt);
	    fixpt->leg[0] = fixpt->leg[1] = fixpt->leg[2] = NULL;
	    addfakeleg(fixpt, stn, 0, 0, 0,
		       sdx * sdx, sdy * sdy, sdz * sdz
#ifndef NO_COVARIANCES
		       , cxy, cyz, czx
#endif
		       );
	 }

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

   stn = StnFromPfx(fix_name);
   if (!fixed(stn)) {
      POS(stn, 0) = x;
      POS(stn, 1) = y;
      POS(stn, 2) = z;
      fix(stn);
      return;
   }

   if (x != POS(stn, 0) || y != POS(stn, 1) || z != POS(stn, 2)) {
      compile_diagnostic(DIAG_ERR, /*Station already fixed or equated to a fixed point*/46);
      return;
   }
   /* TRANSLATORS: *fix a 1 2 3 / *fix a 1 2 3 */
   compile_diagnostic(DIAG_WARN, /*Station already fixed at the same coordinates*/55);
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
   bool fNot = fFalse;
   bool fEmpty = fTrue;
   while (1) {
      int flag;
      get_token();
      /* If buffer is empty, it could mean end of line, or maybe
       * some non-letter junk which is better reported later */
      if (!buffer[0]) break;

      fEmpty = fFalse;
      flag = match_tok(flagtab, TABSIZE(flagtab));
      /* treat the second NOT in "NOT NOT" as an unknown flag */
      if (flag == FLAGS_UNKNOWN || (fNot && flag == FLAGS_NOT)) {
	 compile_diagnostic(DIAG_ERR|DIAG_BUF, /*FLAG “%s” unknown*/68, buffer);
	 /* Recover from “*FLAGS NOT BOGUS SURFACE” by ignoring "NOT BOGUS" */
	 fNot = fFalse;
      } else if (flag == FLAGS_NOT) {
	 fNot = fTrue;
      } else if (fNot) {
	 pcs->flags &= ~BIT(flag);
	 fNot = fFalse;
      } else {
	 pcs->flags |= BIT(flag);
      }
   }

   if (fNot) {
      compile_diagnostic(DIAG_ERR|DIAG_BUF, /*Expecting “DUPLICATE”, “SPLAY”, or “SURFACE”*/188);
   } else if (fEmpty) {
      compile_diagnostic(DIAG_ERR|DIAG_BUF, /*Expecting “NOT”, “DUPLICATE”, “SPLAY”, or “SURFACE”*/189);
   }
}

static void
cmd_equate(void)
{
   prefix *name1, *name2;
   bool fOnlyOneStn = fTrue; /* to trap eg *equate entrance.6 */
   filepos fp;

   get_pos(&fp);
   name1 = read_prefix(PFX_STATION|PFX_ALLOW_ROOT|PFX_SUSPECT_TYPO);
   while (fTrue) {
      name2 = name1;
      skipblanks();
      if (isEol(ch) || isComm(ch)) {
	 if (fOnlyOneStn) {
	    set_pos(&fp);
	    /* TRANSLATORS: EQUATE is a command name, so shouldn’t be
	     * translated.
	     *
	     * Here "station" is a survey station, not a train station.
	     */
	    compile_diagnostic(DIAG_ERR|DIAG_SKIP|DIAG_TOKEN, /*Only one station in EQUATE command*/33);
	 }
	 return;
      }

      name1 = read_prefix(PFX_STATION|PFX_ALLOW_ROOT|PFX_SUSPECT_TYPO);
      process_equate(name1, name2);
      fOnlyOneStn = fFalse;
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

   fExportUsed = fTrue;
   do {
      int depth = 0;
      pfx = read_prefix(PFX_STATION|PFX_NEW);
      if (pfx == NULL) {
	 /* The argument was an existing station. */
	 /* FIXME */
      } else {
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

   /* readings which may be given for each style */
   static const unsigned long mask[] = {
      MASK_NORMAL, MASK_DIVING, MASK_CARTESIAN, MASK_CYLPOLAR, MASK_NOSURVEY,
      MASK_PASSAGE
   };

   /* readings which may be omitted for each style */
   static const unsigned long mask_optional[] = {
      BIT(Dir) | BIT(Clino) | BIT(BackClino),
      BIT(Dir) | BIT(Clino) | BIT(BackClino),
      0,
      BIT(Dir),
      0,
      0 /* BIT(Left) | BIT(Right) | BIT(Up) | BIT(Down), */
   };

   /* all valid readings */
   static const unsigned long mask_all[] = {
      MASK_NORMAL | BIT(Newline) | BIT(Ignore) | BIT(IgnoreAll) | BIT(End),
      MASK_DIVING | BIT(Newline) | BIT(Ignore) | BIT(IgnoreAll) | BIT(End),
      MASK_CARTESIAN | BIT(Newline) | BIT(Ignore) | BIT(IgnoreAll) | BIT(End),
      MASK_CYLPOLAR | BIT(Newline) | BIT(Ignore) | BIT(IgnoreAll) | BIT(End),
      MASK_NOSURVEY | BIT(Ignore) | BIT(IgnoreAll) | BIT(End),
      MASK_PASSAGE | BIT(Ignore) | BIT(IgnoreAll) | BIT(End)
   };
#define STYLE_DEFAULT   -2
#define STYLE_UNKNOWN   -1

   static const sztok styletab[] = {
	{"CARTESIAN",    STYLE_CARTESIAN },
	{"CYLPOLAR",     STYLE_CYLPOLAR },
	{"DEFAULT",      STYLE_DEFAULT },
	{"DIVING",       STYLE_DIVING },
	{"NORMAL",       STYLE_NORMAL },
	{"NOSURVEY",     STYLE_NOSURVEY },
	{"PASSAGE",      STYLE_PASSAGE },
	{"TOPOFIL",      STYLE_NORMAL },
	{NULL,		 STYLE_UNKNOWN }
   };

#define m_multi (BIT(Station) | BIT(Count) | BIT(Depth))

   int style, k = 0, kMac;
   reading *new_order, d;
   unsigned long mUsed = 0;
   char *style_name;
   int old_style = pcs->style;

   /* after a bad *data command ignore survey data until the next
    * *data command to avoid an avalanche of errors */
   pcs->style = STYLE_IGNORE;

   kMac = 6; /* minimum for NORMAL style */
   new_order = osmalloc(kMac * sizeof(reading));

   get_token();
   style = match_tok(styletab, TABSIZE(styletab));

   if (style == STYLE_DEFAULT) {
      default_style(pcs);
      return;
   }

   if (style == STYLE_UNKNOWN) {
      if (!buffer[0]) {
	 /* "*data" reinitialises the current style - for *data passage that
	  * breaks the passage.
	  */
	 style = old_style;
	 goto reinit_style;
      }
      /* TRANSLATORS: e.g. trying to refer to an invalid FNORD data style */
      compile_diagnostic(DIAG_ERR|DIAG_BUF|DIAG_SKIP, /*Data style “%s” unknown*/65, buffer);
      return;
   }

   skipblanks();
#ifndef NO_DEPRECATED
   /* Olde syntax had optional field for survey grade, so allow an omit
    * but issue a warning about it */
   if (isOmit(ch)) {
      static int data_depr_count = 0;
      if (data_depr_count < 5) {
	 compile_diagnostic(DIAG_WARN|DIAG_BUF, /*“*data %s %c …” is deprecated - use “*data %s …” instead*/104,
			    buffer, ch, buffer);
	 if (++data_depr_count == 5)
	    compile_diagnostic(DIAG_WARN, /*Further uses of this deprecated feature will not be reported*/95);
      }
      nextch();
   }
#endif

   style_name = osstrdup(buffer);
   do {
      filepos fp;
      get_pos(&fp);
      get_token();
      d = match_tok(dtab, TABSIZE(dtab));
      /* only token allowed after IGNOREALL is NEWLINE */
      if (k && new_order[k - 1] == IgnoreAll && d != Newline) {
	 set_pos(&fp);
	 break;
      }
      /* Note: an unknown token is reported as trailing garbage */
      if (!TSTBIT(mask_all[style], d)) {
	 /* TRANSLATORS: a data "style" is something like NORMAL, DIVING, etc.
	  * a "reading" is one of FROM, TO, TAPE, COMPASS, CLINO for NORMAL
	  * neither style nor reading is a keyword in the program This error
	  * complains about a depth gauge reading in normal style, for example
	  */
	 compile_diagnostic(DIAG_ERR|DIAG_BUF|DIAG_SKIP,
			    /*Reading “%s” not allowed in data style “%s”*/63,
			    buffer, style_name);
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
	 compile_diagnostic(DIAG_ERR|DIAG_BUF|DIAG_SKIP,
			    /*Reading “%s” must occur before NEWLINE*/225, buffer);
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
	    compile_diagnostic(DIAG_ERR|DIAG_BUF|DIAG_SKIP, /*Duplicate reading “%s”*/67, buffer);
	    osfree(style_name);
	    osfree(new_order);
	    return;
	 } else {
	    /* Check for previously listed readings which are incompatible
	     * with this one - e.g. Count vs FrCount */
	    bool fBad = fFalse;
	    switch (d) {
	     case Station:
	       if (mUsed & (BIT(Fr) | BIT(To))) fBad = fTrue;
	       break;
	     case Fr: case To:
	       if (TSTBIT(mUsed, Station)) fBad = fTrue;
	       break;
	     case Count:
	       if (mUsed & (BIT(FrCount) | BIT(ToCount) | BIT(Tape)))
		  fBad = fTrue;
	       break;
	     case FrCount: case ToCount:
	       if (mUsed & (BIT(Count) | BIT(Tape)))
		  fBad = fTrue;
	       break;
	     case Depth:
	       if (mUsed & (BIT(FrDepth) | BIT(ToDepth) | BIT(DepthChange)))
		  fBad = fTrue;
	       break;
	     case FrDepth: case ToDepth:
	       if (mUsed & (BIT(Depth) | BIT(DepthChange))) fBad = fTrue;
	       break;
	     case DepthChange:
	       if (mUsed & (BIT(FrDepth) | BIT(ToDepth) | BIT(Depth)))
		  fBad = fTrue;
	       break;
	     case Newline:
	       if (mUsed & ~m_multi) {
		  /* TRANSLATORS: e.g.
		   *
		   * *data normal from to tape newline compass clino */
		  compile_diagnostic(DIAG_ERR|DIAG_BUF|DIAG_SKIP, /*NEWLINE can only be preceded by STATION, DEPTH, and COUNT*/226);
		  osfree(style_name);
		  osfree(new_order);
		  return;
	       }
	       if (k == 0) {
		  /* TRANSLATORS: error from:
		   *
		   * *data normal newline from to tape compass clino */
		  compile_diagnostic(DIAG_ERR|DIAG_BUF|DIAG_SKIP, /*NEWLINE can’t be the first reading*/222);
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
	       compile_diagnostic(DIAG_ERR|DIAG_BUF|DIAG_SKIP, /*Reading “%s” duplicates previous reading(s)*/77,
					 buffer);
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
      compile_diagnostic(DIAG_ERR|DIAG_BUF|DIAG_SKIP, /*NEWLINE can’t be the last reading*/223);
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

   pcs->style = style;
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
   factor = read_numeric(fTrue);
   if (factor == 0.0) {
      set_pos(&fp);
      /* TRANSLATORS: error message given by "*units tape 0 feet" - it’s
       * meaningless to say your tape is marked in "0 feet" (but you might
       * measure distance by counting knots on a diving line, and tie them
       * every "2 feet"). */
      compile_diagnostic(DIAG_ERR|DIAG_TOKEN, /**UNITS factor must be non-zero*/200);
      skipline();
      return;
   }

   units = get_units(qmask, fTrue);
   if (units == UNITS_NULL) return;
   if (TSTBIT(qmask, Q_GRADIENT))
      pcs->f_clino_percent = (units == UNITS_PERCENT);
   if (TSTBIT(qmask, Q_BACKGRADIENT))
      pcs->f_backclino_percent = (units == UNITS_PERCENT);

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

   z = read_numeric(fFalse);
   get_pos(&fp);
   sc = read_numeric(fTrue);
   if (sc == HUGE_REAL) {
      if (isalpha(ch)) {
	 int units = get_units(qmask, fFalse);
	 if (units == UNITS_NULL) {
	    return;
	 }
	 z *= factor_tab[units];
	 sc = read_numeric(fTrue);
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
      compile_diagnostic(DIAG_ERR|DIAG_TOKEN, /*Scale factor must be 1.0 for DECLINATION*/40);
      skipline();
      return;
   }
   if (sc == 0.0) {
      set_pos(&fp);
      /* TRANSLATORS: If the scale factor for an instrument is zero, then any
       * reading would be mapped to zero, which doesn't make sense. */
      compile_diagnostic(DIAG_ERR|DIAG_TOKEN, /*Scale factor must be non-zero*/391);
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

static void
cmd_declination(void)
{
    real v = read_numeric(fTrue);
    if (v == HUGE_REAL) {
	get_token_no_blanks();
	if (strcmp(ucbuffer, "AUTO") != 0) {
	    compile_diagnostic(DIAG_ERR|DIAG_SKIP|DIAG_COL, /*Expected number or “AUTO”*/309);
	    return;
	}
	/* *declination auto X Y Z */
	real x = read_numeric(fFalse);
	real y = read_numeric(fFalse);
	real z = read_numeric(fFalse);
	if (!pcs->proj) {
	    compile_diagnostic(DIAG_ERR, /*Input coordinate system must be specified for “*DECLINATION AUTO”*/301);
	    return;
	}
	if (!proj_wgs84) {
	    proj_wgs84 = pj_init_plus("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");
	}
	/* Convert to WGS84 lat long. */
	if (pj_is_latlong(pcs->proj)) {
	    /* PROJ expects lat and long in radians. */
	    x = rad(x);
	    y = rad(y);
	}
	int r = pj_transform(pcs->proj, proj_wgs84, 1, 1, &x, &y, &z);
	if (r != 0) {
	    compile_diagnostic(DIAG_ERR, /*Failed to convert coordinates: %s*/436, pj_strerrno(r));
	    return;
	}
	pcs->z[Q_DECLINATION] = HUGE_REAL;
	pcs->dec_x = x;
	pcs->dec_y = y;
	pcs->dec_z = z;
	/* Invalidate cached declination. */
	pcs->declination = HUGE_REAL;
	{
#ifdef HAVE_PROJ_H
	    PJ_COORD lp;
	    lp.lp.lam = x;
	    lp.lp.phi = y;
	    PJ_FACTORS factors = proj_factors(proj_out, lp);
	    pcs->convergence = factors.meridian_convergence;
#else
	    projLP lp = { x, y };
	    struct FACTORS factors;
	    memset(&factors, 0, sizeof(factors));
	    pj_factors(lp, proj_out, 0.0, &factors);
	    pcs->convergence = factors.conv;
#endif
	}
    } else {
	/* *declination D UNITS */
	int units = get_units(BIT(Q_DECLINATION), fFalse);
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
      compile_diagnostic(DIAG_WARN|DIAG_COL, /**DEFAULT is deprecated - use *CALIBRATE/DATA/SD/UNITS with argument DEFAULT instead*/20);
      if (++default_depr_count == 5)
	 compile_diagnostic(DIAG_WARN, /*Further uses of this deprecated feature will not be reported*/95);
   }

   get_token();
   switch (match_tok(defaulttab, TABSIZE(defaulttab))) {
    case CMD_CALIBRATE:
      default_calib(pcs);
      break;
    case CMD_DATA:
      default_style(pcs);
      default_grade(pcs);
      break;
    case CMD_UNITS:
      default_units(pcs);
      break;
    default:
      compile_diagnostic(DIAG_ERR|DIAG_BUF|DIAG_SKIP, /*Unknown setting “%s”*/41, buffer);
   }
}
#endif

static void
cmd_include(void)
{
   char *pth, *fnm = NULL;
   int fnm_len;
#ifndef NO_DEPRECATED
   prefix *root_store;
#endif
   int ch_store;

   pth = path_from_fnm(file.filename);

   read_string(&fnm, &fnm_len);

#ifndef NO_DEPRECATED
   /* Since *begin / *end nesting cannot cross file boundaries we only
    * need to preserve the prefix if the deprecated *prefix command
    * can be used */
   root_store = root;
   root = pcs->Prefix; /* Root for include file is current prefix */
#endif
   ch_store = ch;

   data_file(pth, fnm);

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
   sd = read_numeric(fFalse);
   if (sd <= (real)0.0) {
      compile_diagnostic(DIAG_ERR|DIAG_SKIP|DIAG_COL, /*Standard deviation must be positive*/48);
      return;
   }
   units = get_units(qmask, fFalse);
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
      fExplicitTitle = fTrue;
      read_string(&survey_title, &survey_title_len);
   } else {
      /* parse and throw away this title (but still check rest of line) */
      char *s = NULL;
      int len;
      read_string(&s, &len);
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
      compile_diagnostic(DIAG_ERR|DIAG_BUF|DIAG_SKIP, /*Found “%s”, expecting “PRESERVE”, “TOUPPER”, or “TOLOWER”*/10, buffer);
   }
}

typedef enum {
    CS_NONE = -1,
    CS_CUSTOM,
    CS_EPSG,
    CS_ESRI,
    CS_EUR,
    CS_IJTSK,
    CS_JTSK,
    CS_LAT,
    CS_LOCAL,
    CS_LONG,
    CS_OSGB,
    CS_S_MERC,
    CS_UTM
} cs_class;

static const sztok cs_tab[] = {
     {"CUSTOM", CS_CUSTOM},
     {"EPSG",   CS_EPSG},	/* EPSG:<number> */
     {"ESRI",   CS_ESRI},	/* ESRI:<number> */
     {"EUR",    CS_EUR},	/* EUR79Z30 */
     {"IJTSK",  CS_IJTSK},	/* IJTSK or IJTSK03 */
     {"JTSK",   CS_JTSK},	/* JTSK or JTSK03 */
     {"LAT",    CS_LAT},	/* LAT-LONG */
     {"LOCAL",  CS_LOCAL},
     {"LONG",   CS_LONG},	/* LONG-LAT */
     {"OSGB",   CS_OSGB},	/* OSGB:<H, N, O, S or T><A-Z except I> */
     {"S",      CS_S_MERC},	/* S-MERC */
     {"UTM",    CS_UTM},	/* UTM<zone><N or S or nothing> */
     {NULL,     CS_NONE}
};

static void
cmd_cs(void)
{
   char * proj_str = NULL;
   int proj_str_len;
   cs_class cs;
   int cs_sub = INT_MIN;
   filepos fp;
   bool output = fFalse;
   enum { YES, NO, MAYBE } ok_for_output = YES;
   static bool had_cs = fFalse;

   if (!had_cs) {
      had_cs = fTrue;
      if (first_fix_name) {
	 compile_diagnostic_at(DIAG_ERR,
			       first_fix_filename, first_fix_line,
			       /*Station “%s” fixed before CS command first used*/442,
			       sprint_prefix(first_fix_name));
      }
   }

   get_pos(&fp);
   /* Note get_token() only accepts letters - it'll stop at digits so "UTM12"
    * will give token "UTM". */
   get_token();
   if (strcmp(ucbuffer, "OUT") == 0) {
      output = fTrue;
      get_pos(&fp);
      get_token();
   }
   cs = match_tok(cs_tab, TABSIZE(cs_tab));
   switch (cs) {
      case CS_NONE:
	 break;
      case CS_CUSTOM:
	 ok_for_output = MAYBE;
	 get_pos(&fp);
	 read_string(&proj_str, &proj_str_len);
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
      case CS_EUR:
	 if (isdigit(ch) &&
	     read_uint() == 79 &&
	     (ch == 'Z' || ch == 'z') &&
	     isdigit(nextch()) &&
	     read_uint() == 30) {
	    cs_sub = 7930;
	 }
	 break;
      case CS_JTSK:
	 ok_for_output = NO;
	 /* FALLTHRU */
      case CS_IJTSK:
	 if (ch == '0') {
	    if (nextch() == '3') {
	       nextch();
	       cs_sub = 3;
	    }
	 } else {
	    cs_sub = 0;
	 }
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
	    if (strcmp(ucbuffer, "MERC") == 0) {
	       cs_sub = 0;
	    }
	 }
	 break;
      case CS_UTM:
	 if (isdigit(ch)) {
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
	 }
	 break;
   }
   if (cs_sub == INT_MIN || isalnum(ch)) {
      set_pos(&fp);
      compile_diagnostic(DIAG_ERR|DIAG_TOKEN, /*Unknown coordinate system*/434);
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
	 sprintf(proj_str, "+init=epsg:%d +no_defs", cs_sub);
	 break;
      case CS_ESRI:
	 proj_str = osmalloc(32);
	 sprintf(proj_str, "+init=esri:%d +no_defs", cs_sub);
	 break;
      case CS_EUR:
	 proj_str = osstrdup("+proj=utm +zone=30 +ellps=intl +towgs84=-86,-98,-119,0,0,0,0 +no_defs");
	 break;
      case CS_IJTSK:
	 if (cs_sub == 0)
	    proj_str = osstrdup("+proj=krovak +ellps=bessel +towgs84=570.8285,85.6769,462.842,4.9984,1.5867,5.2611,3.5623 +no_defs");
	 else
	    proj_str = osstrdup("+proj=krovak +ellps=bessel +towgs84=485.021,169.465,483.839,7.786342,4.397554,4.102655,0 +no_defs");
	 break;
      case CS_JTSK:
	 if (cs_sub == 0)
	    proj_str = osstrdup("+proj=krovak +czech +ellps=bessel +towgs84=570.8285,85.6769,462.842,4.9984,1.5867,5.2611,3.5623 +no_defs");
	 else
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
	 proj_str = osstrdup("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");
	 break;
      case CS_OSGB: {
	 int x = 14 - (cs_sub % 25);
	 int y = (cs_sub / 25) - 20;
	 proj_str = osmalloc(160);
	 sprintf(proj_str, "+proj=tmerc +lat_0=49 +lon_0=-2 +k=0.9996012717 +x_0=%d +y_0=%d +ellps=airy +datum=OSGB36 +units=m +no_defs", x * 100000, y * 100000);
	 break;
      }
      case CS_S_MERC:
	 proj_str = osstrdup("+proj=merc +lat_ts=0 +lon_0=0 +k=1 +x_0=0 +y_0=0 +a=6378137 +b=6378137 +units=m +nadgrids=@null +no_defs");
	 break;
      case CS_UTM:
	 proj_str = osmalloc(74);
	 if (cs_sub > 0) {
	    sprintf(proj_str, "+proj=utm +ellps=WGS84 +datum=WGS84 +units=m +zone=%d +no_defs", cs_sub);
	 } else {
	    sprintf(proj_str, "+proj=utm +ellps=WGS84 +datum=WGS84 +units=m +zone=%d +south +no_defs", -cs_sub);
	 }
	 break;
   }

   if (!proj_str) {
      /* printf("CS %d:%d\n", (int)cs, cs_sub); */
      set_pos(&fp);
      compile_diagnostic(DIAG_ERR|DIAG_TOKEN, /*Unknown coordinate system*/434);
      skipline();
      return;
   }

   if (output) {
      if (ok_for_output == NO) {
	 set_pos(&fp);
	 compile_diagnostic(DIAG_ERR|DIAG_TOKEN, /*Coordinate system unsuitable for output*/435);
	 skipline();
	 return;
      }

      /* If the output projection is already set, we still need to create the
       * projection object for a custom projection, so we can report errors.
       * But if the string is identical, we know it's valid.
       */
      if (!proj_out ||
	  (ok_for_output == MAYBE && strcmp(proj_str, proj_str_out) != 0)) {
	 projPJ pj = pj_init_plus(proj_str);
	 if (!pj) {
	    set_pos(&fp);
	    compile_diagnostic(DIAG_ERR|DIAG_TOKEN, /*Invalid coordinate system: %s*/443,
			       pj_strerrno(pj_errno));
	    skipline();
	    return;
	 }
	 if (ok_for_output == MAYBE && pj_is_latlong(pj)) {
	    set_pos(&fp);
	    compile_diagnostic(DIAG_ERR|DIAG_TOKEN, /*Coordinate system unsuitable for output*/435);
	    skipline();
	    return;
	 }
	 if (proj_out) {
	    pj_free(pj);
	    osfree(proj_str);
	 } else {
	    proj_out = pj;
	    proj_str_out = proj_str;
	 }
      }
   } else {
      projPJ pj;
      if (proj_str_out && strcmp(proj_str, proj_str_out) == 0) {
	 /* Same as the current output projection. */
	 pj = proj_out;
      } else {
	 pj = pj_init_plus(proj_str);
	 if (!pj) {
	    set_pos(&fp);
	    compile_diagnostic(DIAG_ERR|DIAG_TOKEN, /*Invalid coordinate system: %s*/443,
			       pj_strerrno(pj_errno));
	    skipline();
	    return;
	 }
      }

      /* Free proj if not used by parent, or as the output projection. */
      settings * p = pcs;
      if (p->proj && (!p->next || p->proj != p->next->proj))
	 if (p->proj != proj_out)
	    pj_free(p->proj);
      p->proj = pj;
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
      compile_diagnostic(DIAG_ERR|DIAG_BUF|DIAG_SKIP, /*Found “%s”, expecting “EQUATES”, “EXPORTS”, or “PLUMBS”*/31, buffer);
      return;
   }
   get_token();
   on = match_tok(onoff_tab, TABSIZE(onoff_tab));
   if (on == -1) {
      compile_diagnostic(DIAG_ERR|DIAG_BUF|DIAG_SKIP, /*Found “%s”, expecting “ON” or “OFF”*/32, buffer);
      return;
   }

   if (on) {
      pcs->infer |= BIT(setting);
      if (setting == INFER_EXPORTS) fExportUsed = fTrue;
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
   if (strcmp(ucbuffer, "OFF") != 0) {
      if (*ucbuffer) set_pos(&fp);
      truncate_at = read_uint();
   }
   /* for backward compatibility, "*truncate 0" means "*truncate off" */
   pcs->Truncate = (truncate_at == 0) ? INT_MAX : truncate_at;
}

static void
cmd_ref(void)
{
   /* Just syntax check for now. */
   char *ref = NULL;
   int ref_len;
   read_string(&ref, &ref_len);
   s_free(&ref);
}

static void
cmd_require(void)
{
   const unsigned int version[] = {COMMAVERSION};
   const unsigned int *ver = version;
   filepos fp;

   skipblanks();
   get_pos(&fp);
   while (1) {
      int diff = *ver++ - read_uint();
      if (diff > 0) break;
      if (diff < 0) {
	 size_t i, len;
	 char *v;
	 filepos fp_tmp;

	 /* find end of version number */
	 while (isdigit(ch) || ch == '.') nextch();
	 get_pos(&fp_tmp);
	 len = (size_t)(fp_tmp.offset - fp.offset);
	 v = osmalloc(len + 1);
	 set_pos(&fp);
	 for (i = 0; i < len; i++) {
	    v[i] = ch;
	    nextch();
	 }
	 v[i] = '\0';
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
	 fatalerror_in_file(file.filename, file.line, /*Survex version %s or greater required to process this survey data.*/2, v);
      }
      if (ch != '.') break;
      nextch();
      if (!isdigit(ch) || ver == version + sizeof(version) / sizeof(*version))
	 break;
   }
   /* skip rest of version number */
   while (isdigit(ch) || ch == '.') nextch();
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
    bool implicit_range = fFalse;
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
	implicit_range = fTrue;
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
	compile_diagnostic(DIAG_ERR|DIAG_TOKEN, /*End of date range is before the start*/81);
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
   int cmdtok;
   get_token();
   cmdtok = match_tok(cmd_tab, TABSIZE(cmd_tab));

   if (cmdtok < 0 || cmdtok >= (int)(sizeof(cmd_funcs) / sizeof(cmd_fn))) {
      compile_diagnostic(DIAG_ERR|DIAG_BUF|DIAG_SKIP, /*Unknown command “%s”*/12, buffer);
      return;
   }

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
      f_export_ok = fFalse;
      break;
   }

   cmd_funcs[cmdtok]();
}
