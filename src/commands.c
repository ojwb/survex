/* commands.c
 * Code for directives
 * Copyright (C) 1991-2003,2004,2005 Olly Betts
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <limits.h>
#include <stddef.h> /* for offsetof */
#include <time.h>

#include "cavern.h"
#include "commands.h"
#include "datain.h"
#include "debug.h"
#include "filename.h"
#include "message.h"
#include "netbits.h"
#include "netskel.h"
#include "out.h"
#include "readval.h"
#include "str.h"

static void
default_grade(settings *s)
{
   /* Values correspond to those in bcra5.svx */
   s->Var[Q_POS] = (real)sqrd(0.05);
   s->Var[Q_LENGTH] = (real)sqrd(0.05);
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
   int i = -1;

   s_zero(&buffer);
   osfree(ucbuffer);
   skipblanks();
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
   printf("get_token() got `%s'\n", buffer);
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
   CMD_NULL = -1, CMD_BEGIN, CMD_CALIBRATE, CMD_CASE, CMD_COPYRIGHT,
   CMD_DATA, CMD_DATE, CMD_DEFAULT, CMD_END, CMD_ENTRANCE, CMD_EQUATE,
   CMD_EXPORT, CMD_FIX, CMD_FLAGS, CMD_INCLUDE, CMD_INFER, CMD_INSTRUMENT,
   CMD_PREFIX, CMD_REQUIRE, CMD_SD, CMD_SET, CMD_SOLVE,
   CMD_TEAM, CMD_TITLE, CMD_TRUNCATE, CMD_UNITS
} cmds;

static sztok cmd_tab[] = {
     {"BEGIN",     CMD_BEGIN},
     {"CALIBRATE", CMD_CALIBRATE},
     {"CASE",      CMD_CASE},
     {"COPYRIGHT", CMD_COPYRIGHT},
     {"DATA",      CMD_DATA},
     {"DATE",      CMD_DATE},
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
static real factor_tab[] = {
   1.0, METRES_PER_FOOT, (METRES_PER_FOOT*3.0),
   (M_PI/180.0), (M_PI/200.0), 0.01, (M_PI/180.0/60.0)
};

static int
get_units(unsigned long qmask, bool percent_ok)
{
   static sztok utab[] = {
	{"DEGREES",       UNITS_DEGS },
	{"DEGS",	  UNITS_DEGS },
	{"FEET",	  UNITS_FEET },
	{"GRADS",	  UNITS_GRADS },
	{"METERS",	  UNITS_METRES },
	{"METRES",	  UNITS_METRES },
	{"METRIC",	  UNITS_METRES },
	{"MILS",	  UNITS_GRADS },
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
      compile_error_skip(/*Unknown units `%s'*/35, buffer);
      return UNITS_NULL;
   }
   if (units == UNITS_PERCENT && percent_ok &&
       !(qmask & ~(BIT(Q_GRADIENT)|BIT(Q_BACKGRADIENT)))) {
      return units;
   }
   if (((qmask & LEN_QMASK) && !TSTBIT(LEN_UMASK, units)) ||
       ((qmask & ANG_QMASK) && !TSTBIT(ANG_UMASK, units))) {
      compile_error_skip(/*Invalid units `%s' for quantity*/37, buffer);
      return UNITS_NULL;
   }
   return units;
}

/* returns mask with bit x set to indicate quantity x specified */
static unsigned long
get_qlist(unsigned long mask_bad)
{
   static sztok qtab[] = {
	{"ALTITUDE",	 Q_DZ },
	{"BACKBEARING",  Q_BACKBEARING },
	{"BACKCLINO",    Q_BACKGRADIENT },    /* alternative name */
	{"BACKCOMPASS",  Q_BACKBEARING },     /* alternative name */
	{"BACKGRADIENT", Q_BACKGRADIENT },
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

   char default_buf[8] = "";
   while (1) {
      get_pos(&fp);
      get_token();
      tok = match_tok(qtab, TABSIZE(qtab));
      /* bail out if we reach the table end with no match */
      if (tok == Q_NULL) break;
      /* Preserve original case version of DeFAUlt token for error reports */
      if (tok == Q_DEFAULT) strcpy(default_buf, buffer);
      qmask |= BIT(tok);
      if (qmask != BIT(Q_DEFAULT) && (qmask & mask_bad)) {
	 if (qmask & mask_bad & BIT(Q_DEFAULT)) strcpy(buffer, default_buf);
	 compile_error_skip(/*Unknown instrument `%s'*/39, buffer);
	 return 0;
      }
   }

   if (qmask == 0) {
      compile_error_skip(/*Unknown quantity `%s'*/34, buffer);
   } else {
      set_pos(&fp);
   }

   return qmask;
}

#define SPECIAL_UNKNOWN 0
static void
cmd_set(void)
{
   static sztok chartab[] = {
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
      compile_error_skip(/*Unknown character class `%s'*/42, buffer);
      return;
   }

#ifndef NO_DEPRECATED
   if (mask == SPECIAL_ROOT) {
      if (root_depr_count < 5) {
	 compile_warning(/*ROOT is deprecated*/25);
	 if (++root_depr_count == 5)
	    compile_warning(/*Further uses of this deprecated feature will not be reported*/95);
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
check_reentry(prefix *tag)
{
   /* Don't try to check "*prefix \" or "*begin \" */
   if (!tag->up) return;
   if (tag->filename) {
      if (tag->line != file.line ||
	  strcmp(tag->filename, file.filename) != 0) {
	 const char *filename_store = file.filename;
	 unsigned int line_store = file.line;
	 static int reenter_depr_count = 0;

	 if (reenter_depr_count < 5) {
	    compile_warning(/*Reentering an existing prefix level is deprecated*/29);
	    if (++reenter_depr_count == 5)
	       compile_warning(/*Further uses of this deprecated feature will not be reported*/95);
	 }

	 file.filename = tag->filename;
	 file.line = tag->line;
	 compile_warning(/*Originally entered here*/30);
	 file.filename = filename_store;
	 file.line = line_store;
      }
   } else {
      tag->filename = file.filename;
      tag->line = file.line;
   }
}

#ifndef NO_DEPRECATED
static void
cmd_prefix(void)
{
   static int prefix_depr_count = 0;
   prefix *tag;
   /* Issue warning first, so "*prefix \" warns first that *prefix is
    * deprecated and then that ROOT is...
    */
   if (prefix_depr_count < 5) {
      compile_warning(/**prefix is deprecated - use *begin and *end instead*/6);
      if (++prefix_depr_count == 5)
	 compile_warning(/*Further uses of this deprecated feature will not be reported*/95);
   }
   tag = read_prefix_survey(fFalse, fTrue);
   pcs->Prefix = tag;
   check_reentry(tag);
}
#endif

static void
cmd_begin(void)
{
   prefix *tag;
   settings *pcsNew;

   pcsNew = osnew(settings);
   *pcsNew = *pcs; /* copy contents */
   pcsNew->begin_lineno = file.line;
   pcsNew->next = pcs;
   pcs = pcsNew;

   tag = read_prefix_survey(fTrue, fTrue);
   pcs->tag = tag;
   if (tag) {
      pcs->Prefix = tag;
      check_reentry(tag);
      f_export_ok = fTrue;
   }
}

extern void
free_settings(settings *p) {
   /* don't free default ordering or ordering used by parent */
   reading *order = p->ordering;
   if (order != default_order && (!p->next || order != p->next->ordering))
      osfree(order);

   /* free Translate if not used by parent */
   if (!p->next || p->Translate != p->next->Translate)
      osfree(p->Translate - 1);

   /* free meta is not used by parent, or in this block */
   if (p->meta && p->meta != p->next->meta && p->meta->ref_count == 0)
       osfree(p->meta);

   osfree(p);
}

static void
cmd_end(void)
{
   settings *pcsParent;
   prefix *tag, *tagBegin;

   pcsParent = pcs->next;

   if (pcs->begin_lineno == 0) {
      if (pcsParent == NULL) {
	 /* more ENDs than BEGINs */
	 compile_error_skip(/*No matching BEGIN*/192);
      } else {
	 compile_error_skip(/*END with no matching BEGIN in this file*/22);
      }
      return;
   }

   tagBegin = pcs->tag;

   SVX_ASSERT(pcsParent);
   free_settings(pcs);
   pcs = pcsParent;

   /* note need to read using root *before* BEGIN */
   tag = read_prefix_survey(fTrue, fTrue);
   if (tag != tagBegin) {
      if (tag) {
	 if (!tagBegin) {
	    /* "*begin" / "*end foo" */
	    compile_error_skip(/*Matching BEGIN tag has no prefix*/36);
	 } else {
	    /* tag mismatch */
	    compile_error_skip(/*Prefix tag doesn't match BEGIN*/193);
	 }
      } else {
	 /* close tag omitted; open tag given */
	 compile_warning(/*Closing prefix omitted from END*/194);
      }
   }
}

static void
cmd_entrance(void)
{
   prefix *pfx = read_prefix_stn(fFalse, fFalse);
   pfx->sflags |= BIT(SFLAGS_ENTRANCE);
}

static void
cmd_fix(void)
{
   prefix *fix_name;
   node *stn = NULL;
   static node *stnOmitAlready = NULL;
   real x, y, z;
   int nx, ny, nz;
   bool fRef = 0;
   filepos fp;

   fix_name = read_prefix_stn(fFalse, fTrue);
   fix_name->sflags |= BIT(SFLAGS_FIXED);

   get_pos(&fp);
   get_token();
   if (strcmp(ucbuffer, "REFERENCE") == 0) {
      fRef = 1;
   } else {
      if (*ucbuffer) set_pos(&fp);
   }

   x = read_numeric(fTrue, &nx);
   if (x == HUGE_REAL) {
      /* If the end of the line isn't blank, read a number after all to
       * get a more helpful error message */
      if (!isEol(ch) && !isComm(ch)) x = read_numeric(fFalse, &nx);
   }
   if (x == HUGE_REAL) {
      if (stnOmitAlready) {
	 if (fix_name != stnOmitAlready->name) {
	    compile_error_skip(/*More than one FIX command with no coordinates*/56);
	 } else {
	    compile_warning(/*Same station fixed twice with no coordinates*/61);
	 }
	 return;
      }
      stn = StnFromPfx(fix_name);
      compile_warning(/*FIX command with no coordinates - fixing at (0,0,0)*/54);
      x = y = z = (real)0.0;
      stnOmitAlready = stn;
   } else {
      real sdx;
      y = read_numeric(fFalse, &ny);
      z = read_numeric(fFalse, &nz);
      sdx = read_numeric(fTrue, NULL);
      if (sdx != HUGE_REAL) {
	 real sdy, sdz;
	 real cxy = 0, cyz = 0, czx = 0;
	 sdy = read_numeric(fTrue, NULL);
	 if (sdy == HUGE_REAL) {
	    /* only one variance given */
	    sdy = sdz = sdx;
	 } else {
	    sdz = read_numeric(fTrue, NULL);
	    if (sdz == HUGE_REAL) {
	       /* two variances given - horizontal & vertical */
	       sdz = sdy;
	       sdy = sdx;
	    } else {
	       cxy = read_numeric(fTrue, NULL);
	       if (cxy != HUGE_REAL) {
		  /* covariances given */
		  cyz = read_numeric(fFalse, NULL);
		  czx = read_numeric(fFalse, NULL);
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
	    /* suppress "unused fixed point" warnings for this station */
	    if (fRef && fix_name->shape == 1) fix_name->shape = -2;
	 }
	 return;
      }
      stn = StnFromPfx(fix_name);
   }

   /* suppress "unused fixed point" warnings for this station */
   if (fRef && fix_name->shape == 0) fix_name->shape = -1;

   if (!fixed(stn)) {
      POS(stn, 0) = x;
      POS(stn, 1) = y;
      POS(stn, 2) = z;
      fix(stn);
      return;
   }

   if (x != POS(stn, 0) || y != POS(stn, 1) || z != POS(stn, 2)) {
      compile_error(/*Station already fixed or equated to a fixed point*/46);
      return;
   }
   compile_warning(/*Station already fixed at the same coordinates*/55);
}

static void
cmd_flags(void)
{
   static sztok flagtab[] = {
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
	 compile_error(/*FLAG `%s' unknown*/68, buffer);
	 /* Recover from `*FLAGS NOT BOGUS SURFACE' by ignoring "NOT BOGUS" */
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
      compile_error(/*Expecting `DUPLICATE', `SPLAY', or `SURFACE'*/188);
   } else if (fEmpty) {
      compile_error(/*Expecting `NOT', `DUPLICATE', `SPLAY', or `SURFACE'*/189);
   }
}

static void
cmd_equate(void)
{
   prefix *name1, *name2;
   bool fOnlyOneStn = fTrue; /* to trap eg *equate entrance.6 */

   name1 = read_prefix_stn_check_implicit(fFalse, fTrue);
   while (fTrue) {
      name2 = name1;
      name1 = read_prefix_stn_check_implicit(fTrue, fTrue);
      if (name1 == NULL) {
	 if (fOnlyOneStn) {
	    compile_error_skip(/*Only one station in EQUATE command*/33);
	 }
	 return;
      }

      process_equate(name1, name2);
      fOnlyOneStn = fFalse;
   }
}

static void
report_missing_export(prefix *pfx, int depth)
{
   const char *filename_store = file.filename;
   unsigned int line_store = file.line;
   prefix *survey = pfx;
   char *s;
   int i;
   for (i = depth + 1; i; i--) {
      survey = survey->up;
      SVX_ASSERT(survey);
   }
   s = osstrdup(sprint_prefix(survey));
   if (survey->filename) {
      file.filename = survey->filename;
      file.line = survey->line;
   }
   compile_error(/*Station `%s' not exported from survey `%s'*/26,
		 sprint_prefix(pfx), s);
   if (survey->filename) {
      file.filename = filename_store;
      file.line = line_store;
   }
   osfree(s);
}

static void
cmd_export(void)
{
   prefix *pfx;

   fExportUsed = fTrue;
   pfx = read_prefix_stn(fFalse, fFalse);
   do {
      int depth = 0;
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
	    compile_error(/*Station `%s' already exported*/66,
			  sprint_prefix(pfx));
	 }
	 pfx->min_export = depth;
      }
      pfx = read_prefix_stn(fTrue, fFalse);
   } while (pfx);
}

static void
cmd_data(void)
{
   static sztok dtab[] = {
	{"ALTITUDE",	 Dz },
	{"BACKBEARING",  BackComp },
	{"BACKCLINO",    BackClino }, /* alternative name */
	{"BACKCOMPASS",  BackComp }, /* alternative name */
	{"BACKGRADIENT", BackClino },
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
#define MASK_tape BIT(Tape) | BIT(FrCount) | BIT(ToCount) | BIT(Count)
#define MASK_dpth BIT(FrDepth) | BIT(ToDepth) | BIT(Depth) | BIT(DepthChange)
#define MASK_comp BIT(Comp) | BIT(BackComp)
#define MASK_clin BIT(Clino) | BIT(BackClino)

#define MASK_NORMAL MASK_stns | BIT(Dir) | MASK_tape | MASK_comp | MASK_clin
#define MASK_DIVING MASK_stns | BIT(Dir) | MASK_tape | MASK_comp | MASK_dpth
#define MASK_CARTESIAN MASK_stns | BIT(Dx) | BIT(Dy) | BIT(Dz)
#define MASK_CYLPOLAR  MASK_stns | BIT(Dir) | MASK_tape | MASK_comp | MASK_dpth
#define MASK_PASSAGE BIT(Station) | BIT(Left) | BIT(Right) | BIT(Up) | BIT(Down)
#define MASK_NOSURVEY MASK_stns

   /* readings which may be given for each style */
   static const unsigned long mask[] = {
      MASK_NORMAL, MASK_DIVING, MASK_CARTESIAN, MASK_CYLPOLAR, MASK_PASSAGE,
      MASK_NOSURVEY
   };

   /* readings which may be omitted for each style */
   static const unsigned long mask_optional[] = {
      BIT(Dir) | BIT(Clino) | BIT(BackClino),
      BIT(Dir),
      0,
      BIT(Dir),
      0, /* BIT(Left) | BIT(Right) | BIT(Up) | BIT(Down), */
      0
   };

   /* all valid readings */
   static const unsigned long mask_all[] = {
      MASK_NORMAL | BIT(Newline) | BIT(Ignore) | BIT(IgnoreAll) | BIT(End),
      MASK_DIVING | BIT(Newline) | BIT(Ignore) | BIT(IgnoreAll) | BIT(End),
      MASK_CARTESIAN | BIT(Newline) | BIT(Ignore) | BIT(IgnoreAll) | BIT(End),
      MASK_CYLPOLAR | BIT(Newline) | BIT(Ignore) | BIT(IgnoreAll) | BIT(End),
      MASK_PASSAGE | BIT(Ignore) | BIT(IgnoreAll) | BIT(End),
      MASK_NOSURVEY | BIT(Ignore) | BIT(IgnoreAll) | BIT(End)
   };
#define STYLE_DEFAULT   -2
#define STYLE_UNKNOWN   -1

   static sztok styletab[] = {
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
      compile_error_skip(/*Data style `%s' unknown*/65, buffer);
      return;
   }

   skipblanks();
#ifndef NO_DEPRECATED
   /* Olde syntax had optional field for survey grade, so allow an omit
    * but issue a warning about it */
   if (isOmit(ch)) {
      static int data_depr_count = 0;
      if (data_depr_count < 5) {
	 compile_warning(/*`*data %s %c ...' is deprecated - use `*data %s ...' instead*/104,
			 buffer, ch, buffer);
	 if (++data_depr_count == 5)
	    compile_warning(/*Further uses of this deprecated feature will not be reported*/95);
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
	 compile_error_skip(/*Reading `%s' not allowed in data style `%s'*/63,
		       buffer, style_name);
	 osfree(style_name);
	 osfree(new_order);
	 return;
      }
      if (TSTBIT(mUsed, Newline) && TSTBIT(m_multi, d)) {
	 /* e.g. "*data diving station newline tape depth compass" */
	 compile_error_skip(/*Reading `%s' must occur before NEWLINE*/225, buffer);
	 osfree(style_name);
	 osfree(new_order);
	 return;
      }
      /* Check for duplicates unless it's a special reading:
       *   IGNOREALL,IGNORE (duplicates allowed) ; END (not possible)
       */
      if (!((BIT(Ignore) | BIT(End) | BIT(IgnoreAll)) & BIT(d))) {
	 if (TSTBIT(mUsed, d)) {
	    compile_error_skip(/*Duplicate reading `%s'*/67, buffer);
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
		  /* e.g. "*data normal from to tape newline compass clino" */
		  compile_error_skip(/*NEWLINE can only be preceded by STATION, DEPTH, and COUNT*/226);
		  osfree(style_name);
		  osfree(new_order);
		  return;
	       }
	       if (k == 0) {
		  compile_error_skip(/*NEWLINE can't be the first reading*/222);
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
	       compile_error_skip(/*Reading `%s' duplicates previous reading(s)*/77,
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
      compile_error_skip(/*NEWLINE can't be the last reading*/223);
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
      /* This is for when they write
       * *data normal station tape compass clino
       * (i.e. no newline, but interleaved readings)
       */
      compile_error_skip(/*Interleaved readings, but no NEWLINE*/224);
      osfree(style_name);
      osfree(new_order);
      return;
   }

#if 0
   printf("mUsed = 0x%x\n", mUsed);
#endif

   /* Check the supplied readings form a sufficient set. */
   if (style != STYLE_PASSAGE) {
       if (mUsed & (BIT(Fr) | BIT(To)))
	   mUsed |= BIT(Station);
       else if (TSTBIT(mUsed, Station))
	   mUsed |= BIT(Fr) | BIT(To);
   }

   if (mUsed & (BIT(Comp) | BIT(BackComp)))
      mUsed |= BIT(Comp) | BIT(BackComp);

   if (mUsed & (BIT(Clino) | BIT(BackClino)))
      mUsed |= BIT(Clino) | BIT(BackClino);

   if (mUsed & (BIT(FrDepth) | BIT(ToDepth)))
      mUsed |= BIT(Depth) | BIT(DepthChange);
   else if (TSTBIT(mUsed, Depth))
      mUsed |= BIT(FrDepth) | BIT(ToDepth) | BIT(DepthChange);
   else if (TSTBIT(mUsed, DepthChange))
      mUsed |= BIT(FrDepth) | BIT(ToDepth) | BIT(Depth);

   if (mUsed & (BIT(FrCount) | BIT(ToCount)))
      mUsed |= BIT(Count) | BIT(Tape);
   else if (TSTBIT(mUsed, Count))
      mUsed |= BIT(FrCount) | BIT(ToCount) | BIT(Tape);
   else if (TSTBIT(mUsed, Tape))
      mUsed |= BIT(FrCount) | BIT(ToCount) | BIT(Count);

#if 0
   printf("mUsed = 0x%x, opt = 0x%x, mask = 0x%x\n", mUsed,
	  mask_optional[style], mask[style]);
#endif

   if (((mUsed &~ BIT(Newline)) | mask_optional[style]) != mask[style]) {
      /* Test should only fail with too few bits set, not too many */
      SVX_ASSERT((((mUsed &~ BIT(Newline)) | mask_optional[style])
	      &~ mask[style]) == 0);
      compile_error_skip(/*Too few readings for data style `%s'*/64, style_name);
      osfree(style_name);
      osfree(new_order);
      return;
   }

   /* don't free default ordering or ordering used by parent */
   if (pcs->ordering != default_order &&
       !(pcs->next && pcs->next->ordering == pcs->ordering))
      osfree(pcs->ordering);

   pcs->style = style;
   pcs->ordering = new_order;

   osfree(style_name);

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

   qmask = get_qlist(BIT(Q_DEFAULT));
   if (!qmask) return;
   if (qmask == BIT(Q_DEFAULT)) {
      default_units(pcs);
      return;
   }

   factor = read_numeric(fTrue, NULL);
   if (factor == 0.0) {
      compile_error_skip(/**UNITS factor must be non-zero*/200);
      return;
   }
   if (factor == HUGE_REAL) factor = (real)1.0;

   units = get_units(qmask, fTrue);
   if (units == UNITS_NULL) return;
   if (TSTBIT(qmask, Q_GRADIENT))
      pcs->f_clino_percent = (units == UNITS_PERCENT);
   if (TSTBIT(qmask, Q_BACKGRADIENT))
      pcs->f_backclino_percent = (units == UNITS_PERCENT);

   factor *= factor_tab[units];

   for (quantity = 0, m = BIT(quantity); m <= qmask; quantity++, m <<= 1)
      if (qmask & m) pcs->units[quantity] = factor;
}

static void
cmd_calibrate(void)
{
   real sc, z;
   unsigned long qmask, m;
   int quantity;

   qmask = get_qlist(BIT(Q_DEFAULT)|BIT(Q_POS)|BIT(Q_PLUMB)|BIT(Q_LEVEL));
   if (!qmask) return; /* error already reported */

   if (qmask == BIT(Q_DEFAULT)) {
      default_calib(pcs);
      return;
   }

   if (((qmask & LEN_QMASK)) && ((qmask & ANG_QMASK))) {
      compile_error_skip(/*Can't calibrate angular and length quantities together*/227);
      return;
   }

   z = read_numeric(fFalse, NULL);
   sc = read_numeric(fTrue, NULL);
   if (sc == HUGE_REAL) sc = (real)1.0;
   /* check for declination scale */
   /* perhaps "*calibrate declination XXX" should be "*declination XXX" ? */
   if (TSTBIT(qmask, Q_DECLINATION) && sc != 1.0) {
      compile_error_skip(/*Scale factor must be 1.0 for DECLINATION*/40);
      return;
   }
   for (quantity = 0, m = BIT(quantity); m <= qmask; quantity++, m <<= 1) {
      if (qmask & m) {
	 pcs->z[quantity] = pcs->units[quantity] * z;
	 pcs->sc[quantity] = sc;
      }
   }
}

#ifndef NO_DEPRECATED
static void
cmd_default(void)
{
   static sztok defaulttab[] = {
      { "CALIBRATE", CMD_CALIBRATE },
      { "DATA",      CMD_DATA },
      { "UNITS",     CMD_UNITS },
      { NULL,	     CMD_NULL }
   };
   static int default_depr_count = 0;

   if (default_depr_count < 5) {
      compile_warning(/**DEFAULT is deprecated - use *CALIBRATE/DATA/SD/UNITS with argument DEFAULT instead*/20);
      if (++default_depr_count == 5)
	 compile_warning(/*Further uses of this deprecated feature will not be reported*/95);
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
      compile_error_skip(/*Unknown setting `%s'*/41, buffer);
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
   sd = read_numeric(fFalse, NULL);
   if (sd <= (real)0.0) {
      compile_error_skip(/*Standard deviation must be positive*/48);
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

static sztok case_tab[] = {
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
      compile_error_skip(/*Found `%s', expecting `PRESERVE', `TOUPPER', or `TOLOWER'*/10,
		    buffer);
   }
}

static sztok infer_tab[] = {
     { "EQUATES",	INFER_EQUATES },
     { "EXPORTS",	INFER_EXPORTS },
     { "PLUMBS",	INFER_PLUMBS },
#if 0 /* FIXME */
     { "SUBSURVEYS",	INFER_SUBSURVEYS },
#endif
     { NULL,		INFER_NULL }
};

static sztok onoff_tab[] = {
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
      compile_error_skip(/*Found `%s', expecting `EQUATES', `EXPORTS', or `PLUMBS'*/31, buffer);
      return;
   }
   get_token();
   on = match_tok(onoff_tab, TABSIZE(onoff_tab));
   if (on == -1) {
      compile_error_skip(/*Found `%s', expecting `ON' or `OFF'*/32, buffer);
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
	 fatalerror_in_file(file.filename, file.line, /*Survex version %s or greater required to process this survey data.*/2, v);
      }
      if (ch != '.') break;
      nextch();
      if (!isdigit(ch) || ver == version + sizeof(version)) break;
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
	   meta_new->date1 = 0;
	   meta_new->date2 = 0;
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
    struct tm t;
    int year, month, day;
    time_t date1, date2;

    memset(&t, 0, sizeof(t));
    read_date(&year, &month, &day);
    t.tm_year = year;
    t.tm_mon = month ? month : 1;
    t.tm_mday = day ? day : 1;

    --t.tm_mon; /* Jan is 0 */
    if (t.tm_year >= 100) t.tm_year -= 1900; /* tm_year is years since 1900 */
    date1 = mktime(&t);
    if (date1 == (time_t)-1) {
	date1 = 0;
    } else if (date1 > tmUserStart) {
	compile_warning(/*Date is in the future!*/80);
    }

    skipblanks();
    if (ch == '-') {
	nextch();
	memset(&t, 0, sizeof(t));
	read_date(&year, &month, &day);
    }
    t.tm_year = year;
    t.tm_mon = month ? month : 12;
    if (day == 0) {
	t.tm_mday = last_day(year, t.tm_mon);
    } else {
	t.tm_mday = day;
    }
    --t.tm_mon; /* Jan is 0 */
    if (t.tm_year >= 100) t.tm_year -= 1900; /* tm_year is years since 1900 */
    date2 = mktime(&t);
    if (date2 == (time_t)-1) date2 = 0;
    if (date2 < date1)
	compile_error(/*End of date range is before the start*/81);

    copy_on_write_meta(pcs);
    pcs->meta->date1 = date1;
    pcs->meta->date2 = date2;
}

typedef void (*cmd_fn)(void);

static cmd_fn cmd_funcs[] = {
   cmd_begin,
   cmd_calibrate,
   cmd_case,
   skipline, /*cmd_copyright,*/
   cmd_data,
   cmd_date,
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
      compile_error_skip(/*Unknown command `%s'*/12, buffer);
      return;
   }

   switch (cmdtok) {
    case CMD_EXPORT:
      if (!f_export_ok)
	 compile_error(/**EXPORT must immediately follow `*BEGIN <SURVEY>'*/57);
      break;
    case CMD_COPYRIGHT:
    case CMD_DATE:
    case CMD_INSTRUMENT:
    case CMD_TEAM:
    case CMD_TITLE:
      /* These can occur between *begin and *export */
      break;
    default:
      /* NB: additional handling for "*begin <survey>" in cmd_begin */
      f_export_ok = fFalse;
      break;
   }

   cmd_funcs[cmdtok]();
}
