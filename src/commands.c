/* > commands.c
 * Code for directives
 * Copyright (C) 1991-2001 Olly Betts
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
   s->Var[Q_POS] = (real)sqrd(0.10);
   s->Var[Q_LENGTH] = (real)sqrd(0.10);
   s->Var[Q_COUNT] = (real)sqrd(0.10);
   s->Var[Q_DX] = s->Var[Q_DY] = s->Var[Q_DZ] = (real)sqrd(0.10);
   s->Var[Q_BEARING] = (real)sqrd(rad(1.0));
   s->Var[Q_GRADIENT] = (real)sqrd(rad(1.0));
   /* SD of plumbed legs (0.25 degrees?) */
   s->Var[Q_PLUMB] = (real)sqrd(rad(0.25));
   /* SD of level legs (0.25 degrees?) */
   s->Var[Q_LEVEL] = (real)sqrd(rad(0.25));
   s->Var[Q_DEPTH] = (real)sqrd(0.10);
}

static void
default_truncate(settings *s)
{
   s->Truncate = INT_MAX;
}

static void
default_90up(settings *s)
{
   s->f90Up = fFalse;
}

static void
default_case(settings *s)
{
   s->Case = LOWER;
}

reading default_order[] = { Fr, To, Tape, Comp, Clino, End };

static void
default_style(settings *s)
{
   s->Style = (data_normal);
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
/*  ASSERT(EOF==-1);*/ /* important, since we rely on this */
   t = s->Translate;
   for (i = -1; i <= 255; i++) t[i] = 0;
   for (i = '0'; i <= '9'; i++) t[i] |= SPECIAL_NAMES;
   for (i = 'A'; i <= 'Z'; i++) t[i] |= SPECIAL_NAMES;
   for (i = 'a'; i <= 'z'; i++) t[i] |= SPECIAL_NAMES;

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
}

static void
default_units(settings *s)
{
   int quantity;
   for (quantity = 0; quantity < Q_MAC; quantity++) {
      if (TSTBIT(ANG_QMASK, quantity))
	 s->units[quantity] = (real)(M_PI / 180.0); /* degrees */
      else
	 s->units[quantity] = (real)1.0; /* metres */
   }
}

static void
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
   default_90up(s);
   default_case(s);
   default_style(s);
   default_prefix(s);
   default_translate(s);
   default_grade(s);
   default_units(s);
   default_calib(s);
   default_flags(s);
}

static char *buffer = NULL;
static int buf_len;

static char *ucbuffer = NULL;

/* read token */
extern void
get_token(void)
{
   s_zero(&buffer);
   osfree(ucbuffer);
   skipblanks();
   while (isalpha(ch)) {
      s_catchar(&buffer, &buf_len, ch);
      nextch();
   }

   ucbuffer = osmalloc(buf_len);
   {
      int i = -1;
      do {
         i++;
         ucbuffer[i] = toupper(buffer[i]);
      } while (buffer[i]);
   }
#if 0
   printf("get_token() got "); puts(buffer);
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
   CMD_LRUD, CMD_PREFIX, CMD_REQUIRE, CMD_SD, CMD_SET, CMD_SOLVE,
   CMD_TEAM, CMD_TITLE, CMD_TRUNCATE, CMD_UNITS
} cmds;

static sztok cmd_tab[] = {
     {"BEGIN",     CMD_BEGIN},
     {"CALIBRATE", CMD_CALIBRATE},
     {"CASE",      CMD_CASE},
     {"COPYRIGHT", CMD_COPYRIGHT},
     {"DATA",      CMD_DATA},
     {"DATE",      CMD_DATE},
     {"DEFAULT",   CMD_DEFAULT},
     {"END",       CMD_END},
     {"ENTRANCE",  CMD_ENTRANCE},
     {"EQUATE",    CMD_EQUATE},
     {"EXPORT",    CMD_EXPORT},
     {"FIX",       CMD_FIX},
     {"FLAGS",     CMD_FLAGS},
     {"INCLUDE",   CMD_INCLUDE},
     {"INFER",     CMD_INFER},
     {"INSTRUMENT",CMD_INSTRUMENT},
     {"LRUD",      CMD_LRUD},
     {"PREFIX",    CMD_PREFIX},
     {"REQUIRE",   CMD_REQUIRE},
     {"SD",        CMD_SD},
     {"SET",       CMD_SET},
     {"SOLVE",     CMD_SOLVE},
     {"TEAM",      CMD_TEAM},
     {"TITLE",     CMD_TITLE},
     {"TRUNCATE",  CMD_TRUNCATE},
     {"UNITS",     CMD_UNITS},
     {NULL,        CMD_NULL}
};

static int
get_units(void)
{
   static sztok utab[] = {
	{"DEGREES",       UNITS_DEGS },
	{"DEGS",          UNITS_DEGS },
	{"FEET",          UNITS_FEET },
	{"GRADS",         UNITS_GRADS },
	{"METERS",        UNITS_METRES },
	{"METRES",        UNITS_METRES },
	{"METRIC",        UNITS_METRES },
	{"MILS",          UNITS_GRADS },
	{"MINUTES",       UNITS_MINUTES },
	{"PERCENT",       UNITS_PERCENT },
	{"PERCENTAGE",    UNITS_PERCENT },
	{"YARDS",         UNITS_YARDS },
	{NULL,            UNITS_NULL }
   };
   int units;
   get_token();
   units = match_tok(utab, TABSIZE(utab));
   if (units == UNITS_NULL) {
      compile_error(/*Unknown units `%s'*/35, buffer);
   } else if (units == UNITS_PERCENT) {
      NOT_YET;
   }
   return units;
}

/* returns mask with bit x set to indicate quantity x specified */
static unsigned long
get_qlist(void)
{
   static sztok qtab[] = {
	{"ALTITUDE",	 Q_DZ },
	{"ANGLEOUTPUT",  Q_ANGLEOUTPUT },
	{"BEARING",      Q_BEARING },
	{"CLINO",        Q_GRADIENT },    /* alternative name */
	{"COMPASS",      Q_BEARING },     /* alternative name */
	{"COUNT",        Q_COUNT },
	{"COUNTER",      Q_COUNT },       /* alternative name */
	{"DECLINATION",  Q_DECLINATION },
	{"DEFAULT",      Q_DEFAULT }, /* not a real quantity... */
	{"DEPTH",        Q_DEPTH },
	{"DX",           Q_DX },          /* alternative name */
	{"DY",           Q_DY },          /* alternative name */
	{"DZ",           Q_DZ },          /* alternative name */
	{"EASTING",      Q_DX },
	{"GRADIENT",     Q_GRADIENT },
	{"LENGTH",       Q_LENGTH },
	{"LENGTHOUTPUT", Q_LENGTHOUTPUT },
	{"LEVEL",        Q_LEVEL},
	{"NORTHING",     Q_DY },
	{"PLUMB",        Q_PLUMB},
	{"POSITION",     Q_POS },
	{"TAPE",         Q_LENGTH },      /* alternative name */
	{NULL,           Q_NULL }
   };
   unsigned long qmask = 0;
   int tok;
   filepos fp;

   while (1) {
      get_pos(&fp);
      get_token();
      tok = match_tok(qtab, TABSIZE(qtab));
      /* bail out if we reach the table end with no match */
      if (tok == Q_NULL) break;
      qmask |= BIT(tok);
   }

   if (qmask == 0) {
      compile_error(/*Unknown quantity `%s'*/34, buffer);
      skipline();      
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
	{"COMMENT",   SPECIAL_COMMENT },
	{"DECIMAL",   SPECIAL_DECIMAL },
	{"EOL",       SPECIAL_EOL }, /* EOL won't work well */
	{"KEYWORD",   SPECIAL_KEYWORD },
	{"MINUS",     SPECIAL_MINUS },
	{"NAMES",     SPECIAL_NAMES },
	{"OMIT",      SPECIAL_OMIT },
	{"PLUS",      SPECIAL_PLUS },
	{"ROOT",      SPECIAL_ROOT },
	{"SEPARATOR", SPECIAL_SEPARATOR },
	{NULL,        SPECIAL_UNKNOWN }
   };
   int mask;
   int i;

   get_token();
   mask = match_tok(chartab, TABSIZE(chartab));

   if (mask == SPECIAL_UNKNOWN) {
      compile_error(/*Unknown character class `%s'*/42, buffer);
      skipline();
      return;
   }

   if (mask == SPECIAL_ROOT) {
      if (root_depr_count < 5) {
	 compile_warning(/*ROOT is deprecated*/25);
	 if (++root_depr_count == 5)
	    compile_warning(/*No further uses of this deprecated feature will be reported*/95);
      }
   }

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
	       compile_warning(/*No further uses of this deprecated feature will be reported*/95);
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
	 compile_warning(/*No further uses of this deprecated feature will be reported*/95);
   }
   tag = read_prefix_survey(fFalse, fTrue);
   pcs->Prefix = tag;
#ifdef NEW3DFORMAT
   if (fUseNewFormat) {
      limb = get_twig(tag);
      if (limb->up->sourceval < 1) {
      	 osfree(limb->up->source);
       	 limb->up->sourceval = 1;
       	 limb->up->source = osstrdup(file.filename);
      }
   }
#endif
   check_reentry(tag);
}

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
   }

#ifdef NEW3DFORMAT
   if (fUseNewFormat) {
      limb = get_twig(pcs->Prefix);
      if (limb->up->sourceval < 5) {
       	 osfree(limb->up->source);
       	 limb->up->sourceval = 5;
       	 limb->up->source = osstrdup(file.filename);
      }
   }
#endif
}

extern void
free_settings(settings *p) {
   /* don't free default ordering or ordering used by parent */
   reading *order = p->ordering;
   if (order != default_order && order != p->next->ordering) osfree(order);

   /* free Translate if not used by parent */
   if (p->Translate != p->next->Translate) osfree(p->Translate - 1);

   osfree(p);
}

static void
cmd_end(void)
{
   settings *pcsParent;
   prefix *tag, *tagBegin;

   pcsParent = pcs->next;
#ifdef NEW3DFORMAT
   if (fUseNewFormat) limb = get_twig(pcsParent->Prefix);
#endif

   if (pcs->begin_lineno == 0) {
      if (pcsParent == NULL) {
	 /* more ENDs than BEGINs */
	 compile_error(/*No matching BEGIN*/192);
      } else {
	 compile_error(/*END with no matching BEGIN in this file*/22);
      }
      skipline();
      return;
   }

   tagBegin = pcs->tag;

   ASSERT(pcsParent);
   free_settings(pcs);
   pcs = pcsParent;

   /* note need to read using root *before* BEGIN */
   tag = read_prefix_survey(fTrue, fTrue);
   if (tag != tagBegin) {
      if (tag) {
	 if (!tagBegin) {
	    /* "*begin" / "*end foo" */
	    compile_error(/*Matching BEGIN tag has no prefix*/36);
	 } else {
	    /* tag mismatch */
	    compile_error(/*Prefix tag doesn't match BEGIN*/193);
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
   /* FIXME: what about stations created by *entrance? */
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

   x = read_numeric(fTrue);
   if (x == HUGE_REAL) {
      /* If the end of the line isn't blank, read a number after all to
       * get a more helpful error message */
      if (!isEol(ch) && !isComm(ch)) x = read_numeric(fFalse);
   }
   if (x == HUGE_REAL) {
      if (stnOmitAlready) {
	 if (fix_name != stnOmitAlready->name) {
	    compile_error(/*More than one FIX command with no coordinates*/56);
	    skipline();
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
      y = read_numeric(fFalse);
      z = read_numeric(fFalse);
      sdx = read_numeric(fTrue);
      if (sdx != HUGE_REAL) {
	 real sdy, sdz;
	 real cxy = 0, cyz = 0, czx = 0;
	 sdy = read_numeric(fTrue);
	 if (sdy == HUGE_REAL) {
	    /* only one variance given */
	    sdy = sdz = sdx;
	 } else {
	    sdz = read_numeric(fTrue);
	    if (sdz == HUGE_REAL) {
	       /* two variances given - horizontal & vertical */
	       sdz = sdy;
	       sdy = sdx;
	    } else {
	       cxy = read_numeric(fTrue);
	       if (cxy != HUGE_REAL) {
		  /* covariances given */
		  cyz = read_numeric(fFalse);
		  czx = read_numeric(fFalse);
	       }
	    }
	 }
	 stn = StnFromPfx(fix_name);
	 if (!fixed(stn)) {
	    node *fixpt = osnew(node);
	    prefix *name = osnew(prefix);
	    name->ident = ""; /* root has ident[0] == "\" */
	    name->shape = 0;
	    fixpt->name = name;
	    name->pos = osnew(pos);
	    name->stn = fixpt;
	    name->up = NULL;
	    name->min_export = name->max_export = 0;
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
#ifdef NEW3DFORMAT
      if (fUseNewFormat) {
	 fix_name->twig_link->from = fix_name; /* insert fixed point.. */
	 fix_name->twig_link->to = NULL;
#if 0
	 osfree(fix_name->twig_link->down);
	 fix_name->twig_link->down = NULL;
#endif
      }
#endif
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
	{NULL,        FLAGS_UNKNOWN }
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
	    compile_error(/*Only one station in equate list*/33);
	    skipline();
	 }
#ifdef NEW3DFORMAT
	 if (fUseNewFormat) limb = get_twig(pcs->Prefix);
#endif
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
      ASSERT(survey);
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
	 ASSERT(p);
      }
      /* *export \ or similar bogus stuff */
      ASSERT(depth);
#if 0
      printf("C min %d max %d depth %d pfx %s\n",
	     pfx->min_export, pfx->max_export, depth, sprint_prefix(pfx));
#endif
      if (pfx->min_export == 0) {
         /* not encountered *export for this name before */
         if (pfx->max_export > depth) report_missing_export(pfx, depth);
         pfx->min_export = pfx->max_export = depth;
      } else {
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
	{"BEARING",      Comp },
	{"CLINO",        Clino }, /* alternative name */
	{"COMPASS",      Comp }, /* alternative name */
        {"COUNT",        Count }, /* FrCount&ToCount in multiline */
        {"DEPTH",        Depth }, /* FrDepth&ToDepth in multiline */
        {"DIRECTION",    Dir },
	{"DX",		 Dx },
	{"DY",		 Dy },
	{"DZ",		 Dz },
	{"EASTING",      Dx },
	{"FROM",         Fr },
	{"FROMCOUNT",    FrCount },
	{"FROMDEPTH",    FrDepth },
	{"GRADIENT",     Clino },
	{"IGNORE",       Ignore },
	{"IGNOREALL",    IgnoreAll },
	{"LENGTH",       Tape },
	{"NEWLINE",      Newline },
	{"NORTHING",     Dy },
        {"STATION",      Station }, /* Fr&To in multiline */
	{"TAPE",         Tape }, /* alternative name */
	{"TO",           To },
	{"TOCOUNT",      ToCount },
	{"TODEPTH",      ToDepth },
	{NULL,           End }
   };

   static style_fn fn[] = {
      data_normal,
      data_normal,
      data_diving,
      data_cartesian,
      data_nosurvey
   };

   /* readings which may be given for each style */
   static unsigned long mask[] = {
      BIT(Fr) | BIT(To) | BIT(Station) | BIT(Dir)
	 | BIT(Tape) | BIT(Comp) | BIT(Clino),
      BIT(Fr) | BIT(To) | BIT(Station) | BIT(Dir)
	 | BIT(FrCount) | BIT(ToCount) | BIT(Count) | BIT(Comp) | BIT(Clino),
      BIT(Fr) | BIT(To) | BIT(Station) | BIT(Dir) | BIT(Tape) | BIT(Comp)
	 | BIT(FrDepth) | BIT(ToDepth) | BIT(Depth),
      BIT(Fr) | BIT(To) | BIT(Station) | BIT(Dx) | BIT(Dy) | BIT(Dz),
      BIT(Fr) | BIT(To) | BIT(Station)
   };

   /* readings which may be omitted for each style */
   static unsigned long mask_optional[] = {
      BIT(Dir) | BIT(Clino),
      BIT(Dir) | BIT(Clino),
      BIT(Dir),
      0,
      0
   };

   static sztok styletab[] = {
	{"CARTESIAN",    STYLE_CARTESIAN },
	{"DEFAULT",      STYLE_DEFAULT },
	{"DIVING",       STYLE_DIVING },
	{"NORMAL",       STYLE_NORMAL },
	{"NOSURVEY",     STYLE_NOSURVEY },
        {"TOPOFIL",      STYLE_TOPOFIL },
	{NULL,           STYLE_UNKNOWN }
   };

#define m_multi (BIT(Station) | BIT(Count) | BIT(Depth))

   int style, k = 0, kMac;
   reading *new_order, d;
   unsigned long m, mUsed = 0;
   char *style_name;
   bool fHadNewline = fFalse;

   /* after a bad *data command ignore survey data until the next
    * *data command to avoid an avalanche of errors */
   pcs->Style = data_ignore;

   kMac = 6; /* minimum for NORMAL style */
   new_order = osmalloc(kMac * sizeof(reading));

   get_token();
   style = match_tok(styletab, TABSIZE(styletab));

   if (style == STYLE_DEFAULT) {
      default_style(pcs);
      return;
   }

   if (style == STYLE_UNKNOWN) {
      compile_error(/*Data style `%s' unknown*/65, buffer);
      skipline();
      return;
   }

   pcs->Style = fn[style];
   m = mask[style] | BIT(Newline) | BIT(Ignore) | BIT(IgnoreAll) | BIT(End);

   skipblanks();
   /* olde syntax had optional field for survey grade, so allow an omit */
   if (isOmit(ch)) nextch(); /* FIXME: deprecate... */

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
      if (!TSTBIT(m, d)) {
	 compile_error(/*Reading `%s' not allowed in data style `%s'*/63,
		       buffer, style_name);
	 osfree(style_name);
 	 skipline();
	 return;
      }
      if (fHadNewline && TSTBIT(m_multi, d)) {
	 /* FIXME: redo message - this is for when they write
	  * *data diving station newline tape depth compass
	  */
	 compile_error(/*Reading `%s' not allowed in data style `%s'*/63,
		       buffer, style_name);
	 osfree(style_name);
 	 skipline();
	 return;
      }
      /* Check for duplicates unless it's a special reading:
       *   IGNOREALL,IGNORE (duplicates allowed) ; END (not possible)
       */
      if (!((BIT(Ignore) | BIT(End) | BIT(IgnoreAll)) & BIT(d))) {
	 if (TSTBIT(mUsed, d)) {
	    compile_error(/*Duplicate reading `%s'*/67, buffer);
	    osfree(style_name);
	    skipline();
	    return;
	 } else {
	    bool fBad = fFalse;
	    switch (d) {
	     case Station:
	       if (mUsed & (BIT(Fr) | BIT(To))) fBad = fTrue;
	       break;
	     case Fr: case To:
	       if (TSTBIT(mUsed, Station)) fBad = fTrue;
	       break;
	     case Count:
	       if (mUsed & (BIT(FrCount) | BIT(ToCount))) fBad = fTrue;
	       break;
	     case FrCount: case ToCount:
	       if (TSTBIT(mUsed, Count)) fBad = fTrue;
	       break;
	     case Depth:
	       if (mUsed & (BIT(FrDepth) | BIT(ToDepth))) fBad = fTrue;
	       break;
	     case FrDepth: case ToDepth:
	       if (TSTBIT(mUsed, Depth)) fBad = fTrue;
	       break;
	     case Newline:
	       if (mUsed & ~m_multi) {
		  /* FIXME: redo message - this is for when they write
		   * *data normal station tape newline compass clino
		   */
		  compile_error(/*Reading `%s' not allowed in data style `%s'*/63,
				buffer, style_name);
		  osfree(style_name);
		  skipline();
		  return;
	       }
	       break;
	     default: /* avoid compiler warnings about unhandled enums */
	       break;
	    }		  
	    if (fBad) {
	       /* FIXME: not really the correct error here */
	       compile_error(/*Duplicate reading `%s'*/67, buffer);
	       osfree(style_name);
	       skipline();
	       return;
	    }
	    mUsed |= BIT(d); /* used to catch duplicates */
	 }
      }
      if (k && new_order[k - 1] == IgnoreAll) {
	 ASSERT(d == Newline);
	 k--;
	 d = IgnoreAllAndNewLine;
      }
      if (k >= kMac) {
	 kMac = kMac * 2;
	 new_order = osrealloc(new_order, kMac * sizeof(reading));
      }
      new_order[k++] = d;
   } while (d != End);

   /* printf("mUsed = 0x%x\n", mUsed); */

   if (mUsed & (BIT(Fr) | BIT(To))) mUsed |= BIT(Station);
   else if (TSTBIT(mUsed, Station)) mUsed |= BIT(Fr) | BIT(To);

   if (mUsed & (BIT(FrDepth) | BIT(ToDepth))) mUsed |= BIT(Depth);
   else if (TSTBIT(mUsed, Depth)) mUsed |= BIT(FrDepth) | BIT(ToDepth);

   if (mUsed & (BIT(FrCount) | BIT(ToCount))) mUsed |= BIT(Count);
   else if (TSTBIT(mUsed, Count)) mUsed |= BIT(FrCount) | BIT(ToCount);

   /* printf("mUsed = 0x%x, opt = 0x%x, mask = 0x%x\n", mUsed,
	  mask_optional[style], mask[style]); */

   mUsed &= ~BIT(Newline);

   if ((mUsed | mask_optional[style]) != mask[style]) {
      osfree(new_order);
      compile_error(/*Too few readings for data style `%s'*/64, style_name);
      osfree(style_name);
      return;
   }

   /* don't free default ordering or ordering used by parent */
   if (pcs->ordering != default_order &&
       !(pcs->next && pcs->next->ordering == pcs->ordering))
      osfree(pcs->ordering);

   pcs->ordering = new_order;

   osfree(style_name);      
}

/* masks for units which are length and angles respectively */
#define LEN_UMASK (BIT(UNITS_METRES) | BIT(UNITS_FEET) | BIT(UNITS_YARDS))
#define ANG_UMASK (BIT(UNITS_DEGS) | BIT(UNITS_GRADS) | BIT(UNITS_MINUTES))

/* ordering must be the same as the units enum */
static real factor_tab[] = {
   1.0, METRES_PER_FOOT, (METRES_PER_FOOT*3.0),
   (M_PI/180.0), (M_PI/200.0), 0.0, (M_PI/180.0/60.0)
};

static void
cmd_units(void)
{
   int units, quantity;
   unsigned long qmask, m; /* mask with bit x set to indicate quantity x specified */
   real factor;

   qmask = get_qlist();
   if (!qmask) return;
   if (qmask == BIT(Q_DEFAULT)) {
      default_units(pcs);
      return;
   }

   /* If factor given then read units else units in buffer already */
   factor = read_numeric(fTrue);
   if (factor == HUGE_REAL) {
      factor = (real)1.0;
      /* If factor given then read units else units in buffer already */
   }

   units = get_units();
   if (units == UNITS_NULL) return;
   factor *= factor_tab[units];

   if (((qmask & LEN_QMASK) && !TSTBIT(LEN_UMASK, units)) ||
       ((qmask & ANG_QMASK) && !TSTBIT(ANG_UMASK, units))) {
      compile_error(/*Invalid units `%s' for quantity*/37, buffer);
      return;
   }
   if (qmask & (BIT(Q_LENGTHOUTPUT) | BIT(Q_ANGLEOUTPUT)) ) {
      NOT_YET;
   }
   for (quantity = 0, m = BIT(quantity); m <= qmask; quantity++, m <<= 1)
      if (qmask & m) pcs->units[quantity] = factor;
}

static void
cmd_calibrate(void)
{
   real sc, z; /* added so we don't modify current values if error given */
   unsigned long qmask, m;
   int quantity;
   qmask = get_qlist();
   if (!qmask) return;
   if (qmask == BIT(Q_DEFAULT)) {
      default_calib(pcs);
      return;
   }
   /* useful check? */
   if (((qmask & LEN_QMASK)) && ((qmask & ANG_QMASK))) {
      /* FIXME: mixed angles/lengths */
   }
   /* check for things with no valid calibration (like station posn) */
   if (qmask & (BIT(Q_DEFAULT)|BIT(Q_POS)|BIT(Q_LENGTHOUTPUT)
		|BIT(Q_ANGLEOUTPUT)|BIT(Q_PLUMB)|BIT(Q_LEVEL))) {
      compile_error(/*Unknown instrument `%s'*/39, buffer);
      skipline();
      return;
   }
   z = read_numeric(fFalse);
   sc = read_numeric(fTrue);
   if (sc == HUGE_REAL) sc = (real)1.0;
   /* check for declination scale */
   /* perhaps "*calibrate declination XXX" should be "*declination XXX" ? */
   if (TSTBIT(qmask, Q_DECLINATION) && sc != 1.0) {
      compile_error(/*Scale factor must be 1.0 for DECLINATION*/40);
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
cmd_default(void)
{
   static sztok defaulttab[] = {
      { "CALIBRATE", CMD_CALIBRATE },
      { "DATA",      CMD_DATA },
      { "UNITS",     CMD_UNITS },
      { NULL,        CMD_NULL }
   };
   static int default_depr_count = 0;

   if (default_depr_count < 5) {
      compile_warning(/**DEFAULT is deprecated - use *CALIBRATE/DATA/UNITS with argument DEFAULT instead*/20);
      if (++default_depr_count == 5)
	 compile_warning(/*No further uses of this deprecated feature will be reported*/95);
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
      compile_error(/*Unknown setting `%s'*/41, buffer);
   }
}

static void
cmd_include(void)
{
   char *pth, *fnm = NULL;
   int fnm_len;
   prefix *root_store;
   int ch_store;

   pth = path_from_fnm(file.filename);

   read_string(&fnm, &fnm_len);

   root_store = root;
   root = pcs->Prefix; /* Root for include file is current prefix */
   ch_store = ch;

   data_file(pth, fnm);

   root = root_store; /* and restore root */
#ifdef NEW3DFORMAT
   if (fUseNewFormat) limb = get_twig(root);
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
   qmask = get_qlist();
   if (!qmask) return;
   if (qmask == BIT(Q_DEFAULT)) {
      default_grade(pcs);
      return;
   }
   sd = read_numeric(fFalse);
   if (sd < (real)0.0) {
      /* FIXME: complain about negative sd */
   }
   units = get_units();
   if (units == UNITS_NULL) return;
   if (((qmask & LEN_QMASK) && !TSTBIT(LEN_UMASK, units)) ||
       ((qmask & ANG_QMASK) && !TSTBIT(ANG_UMASK, units))) {
      compile_error(/*Invalid units `%s' for quantity*/37, buffer);
      skipline();
      return;
   }

   sd *= factor_tab[units];
   variance = sqrd(sd);

   for (quantity = 0, m = BIT(quantity); m <= qmask; quantity++, m <<= 1)
      if (qmask & m) pcs->Var[quantity] = variance;
}

static void
cmd_title(void)
{
   if (!fExplicitTitle) {
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
      compile_error(/*Found `%s', expecting `PRESERVE', `TOUPPER', or `TOLOWER'*/10,
		    buffer);
      skipline();
   }
}

static sztok infer_tab[] = {
     {"EQUATES",    2},
     {"PLUMBS",     1},
     {NULL,      0}
};

static sztok onoff_tab[] = {
     {"OFF", 0},
     {"ON",  1},
     {NULL,       -1}
};

static void
cmd_infer(void)
{
   int setting;
   int on;
   get_token();
   setting = match_tok(infer_tab, TABSIZE(infer_tab));
   if (setting == -1) {
      compile_error(/*Found `%s', expecting `PLUMBS'*/31, buffer);
      skipline();
      return;
   }
   get_token();
   on = match_tok(onoff_tab, TABSIZE(onoff_tab));
   if (on == -1) {
      compile_error(/*Found `%s', expecting `ON' or `OFF'*/32, buffer);
      skipline();
      return;
   }

   switch (setting) {
    case 2:
      pcs->f0Eq = on;
      break;
    case 1:
      pcs->f90Up = on;
      break;
    default:
      BUG("unexpected case");
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
   static char version[] = VERSION;
   char *p = version;
   filepos fp;

   skipblanks();
   get_pos(&fp);
   while (1) {
      unsigned int have, want;
      have = (unsigned int)strtoul(p, &p, 10);
      want = read_uint();
      if (have > want) break;
      if (have < want) {
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
      if (!isdigit(ch) || *p != '.') break;
      p++;
   }
   /* skip rest of version number */
   while (isdigit(ch) || ch == '.') nextch();
}

typedef void (*cmd_fn)(void);

static cmd_fn cmd_funcs[] = {
   cmd_begin,
   cmd_calibrate,
   cmd_case,
   skipline, /*cmd_copyright,*/
   cmd_data,
   skipline, /*cmd_date,*/
   cmd_default,
   cmd_end,
   cmd_entrance,
   cmd_equate,
   cmd_export,
   cmd_fix,
   cmd_flags,
   cmd_include,
   cmd_infer,
   skipline, /*cmd_instrument,*/
   skipline, /*cmd_lrud,*/
   cmd_prefix,
   cmd_require,
   cmd_sd,
   cmd_set,
   solve_network,
   skipline, /*cmd_team,*/
   cmd_title,
   cmd_truncate,
   cmd_units
};

/* Just ignore *lrud for now so Tunnel can put it in */
/* FIXME: except that tunnel keeps x-sections in a separate file... */
 
extern void
handle_command(void)
{
   int cmdtok;
   get_token();
   cmdtok = match_tok(cmd_tab, TABSIZE(cmd_tab));

   if (cmdtok < 0 || cmdtok >= (int)(sizeof(cmd_funcs) / sizeof(cmd_fn))) {
      compile_error(/*Unknown command `%s'*/12, buffer);
      skipline();
      return;
   }

   switch (cmdtok) {
    case CMD_EXPORT:
      if (!f_export_ok)
	 compile_error(/**EXPORT must immediately follow *BEGIN*/57);
      break;
    case CMD_BEGIN:
      f_export_ok = fTrue;
      break;
    case CMD_COPYRIGHT:
    case CMD_DATE:
    case CMD_INSTRUMENT:
    case CMD_TEAM:
    case CMD_TITLE:
      /* these can occur between *begin and *export */
      break;      
    default:
      f_export_ok = fFalse;
      break;
   }
   
   cmd_funcs[cmdtok]();
}
