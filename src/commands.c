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
#ifdef NEW3DFORMAT
#include "new3dout.h"
#endif

static void
default_grade(settings *s)
{
   s->Var[Q_POS] = (real)sqrd(0.10);
   s->Var[Q_LENGTH] = (real)sqrd(0.10);
   s->Var[Q_COUNT] = (real)sqrd(0.10); /* FIXME ??? */
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

datum default_order[] = { Fr, To, Tape, Comp, Clino, End };

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
	 s->units[quantity] = (real)(PI / 180.0); /* degrees */
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
}

static void
read_string(char **pstr, int *plen)
{
   bool fQuoted = fFalse;

   s_zero(pstr);

   skipblanks();
   if (ch == '\"') {
      fQuoted = fTrue;
      nextch();
   }
   while (!isEol(ch)) {
      if (ch == (fQuoted ? '\"' : ' ')) {
	 nextch();
	 fQuoted = fFalse;
	 break;
      }
      s_catchar(pstr, plen, ch);
      nextch();
   }

   if (fQuoted) {
      compile_error(/*Missing &quot;*/69);
      skipline();
      return;
   }
}

#define MAX_KEYWORD_LEN 24

static char ucbuffer[MAX_KEYWORD_LEN];
static char buffer[MAX_KEYWORD_LEN];

/* read token */
extern void
get_token(void)
{
   int j = 0;
   skipblanks();
   while (isalpha(ch)) {
      if (j < MAX_KEYWORD_LEN) {
	 buffer[j] = ch;
	 ucbuffer[j] = toupper(ch);
	 j++;
      }
      nextch();
   }
   if (j < MAX_KEYWORD_LEN) {
      buffer[j] = '\0';
      ucbuffer[j] = '\0';
   } else {
      /* if token is too long, change end to "..." - that way it won't
       * match, but will look sensible in any error message */
      /* FIXME: this is naff, better to use resizing buffer */
      strcpy(buffer + MAX_KEYWORD_LEN - 4, "...");
      strcpy(ucbuffer + MAX_KEYWORD_LEN - 4, "...");
   }
   /* printf("get_token() got "); puts(buffer); */
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
   CMD_NULL = -1, CMD_BEGIN, CMD_CALIBRATE, CMD_CASE, CMD_DATA, CMD_DEFAULT,
   CMD_END, CMD_EQUATE, CMD_EXPORT, CMD_FIX, CMD_INCLUDE, CMD_INFER,
   CMD_LRUD, CMD_MEASURE, CMD_PREFIX, CMD_REQUIRE, CMD_SD, CMD_SET,
   CMD_SOLVE, CMD_TITLE, CMD_TRUNCATE, CMD_UNITS
} cmds;

static sztok cmd_tab[] = {
     {"BEGIN",     CMD_BEGIN},
     {"CALIBRATE", CMD_CALIBRATE},
     {"CASE",      CMD_CASE},
     {"DATA",      CMD_DATA},
     {"DEFAULT",   CMD_DEFAULT},
     {"END",       CMD_END},
     {"EQUATE",    CMD_EQUATE},
     {"EXPORT",    CMD_EXPORT},
     {"FIX",       CMD_FIX},
     {"INCLUDE",   CMD_INCLUDE},
     {"INFER",     CMD_INFER},
     {"LRUD",      CMD_LRUD},
     {"MEASURE",   CMD_MEASURE},
     {"PREFIX",    CMD_PREFIX},
     {"REQUIRE",   CMD_REQUIRE},
     {"SD",        CMD_SD},
     {"SET",       CMD_SET},
     {"SOLVE",     CMD_SOLVE},
     {"TITLE",     CMD_TITLE},
     {"TRUNCATE",  CMD_TRUNCATE},
     {"UNITS",     CMD_UNITS},
     {NULL,        CMD_NULL}
};

/* NB match_units *doesn't* call get_token, so do it yourself if approp. */
static int
match_units(void)
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
   long fp = ftell(file.fh);
   int units = match_tok(utab, TABSIZE(utab));
   if (units == UNITS_NULL) {
      fseek(file.fh, fp, SEEK_SET);
      compile_error(/*Unknown units `%s'*/35, buffer);
      skipline();
   }
   if (units == UNITS_PERCENT) NOT_YET;
   return units;
}

/* returns mask with bit x set to indicate quantity x specified */
static ulong
get_qlist(void)
{
   static sztok qtab[] = {
	{"ANGLEOUTPUT",  Q_ANGLEOUTPUT },
	{"BEARING",      Q_BEARING },
	{"CLINO",        Q_GRADIENT },    /* alternative name */
	{"COMPASS",      Q_BEARING },     /* alternative name */
	{"COUNT",        Q_COUNT },
	{"COUNTER",      Q_COUNT },       /* alternative name */
	{"DECLINATION",  Q_DECLINATION },
	{"DEFAULT",      Q_DEFAULT }, /* not a real quantity... */
	{"DEPTH",        Q_DEPTH },
	{"DX",           Q_DX },
	{"DY",           Q_DY },
	{"DZ",           Q_DZ },
	{"GRADIENT",     Q_GRADIENT },
	{"LENGTH",       Q_LENGTH },
	{"LENGTHOUTPUT", Q_LENGTHOUTPUT },
	{"LEVEL",        Q_LEVEL},
	{"PLUMB",        Q_PLUMB},
	{"POSITION",     Q_POS },
	{"TAPE",         Q_LENGTH },      /* alternative name */
	{NULL,           Q_NULL }
   };
   ulong qmask = 0;
   int tok;
   while (1) {
      get_token();
      tok = match_tok(qtab, TABSIZE(qtab));
      /* bail out if we reach the table end with no match */
      if (tok == Q_NULL) break;
      qmask |= BIT(tok);
   }
   if (qmask == 0) {
      compile_error(/*Unknown quantity `%s'*/34, buffer);
      skipline();
   }
   return qmask;
}

#define SPECIAL_UNKNOWN 0
static void
set_chars(void)
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

   if (mask == SPECIAL_ROOT)
      compile_warning(/*ROOT is deprecated*/25);

   /* if we're currently using an inherited translation table, allocate a new
    * table, and copy old one into it */
   if (pcs->next && pcs->next->Translate == pcs->Translate) {
      short *p;
      p = ((short*)osmalloc(ossizeof(short) * 257)) + 1;
      memcpy(p - 1, pcs->Translate - 1, sizeof(short) * 257);
      pcs->Translate = p;
   }
   /* clear this flag for all non-alphanums */
   for (i = 0; i < 256; i++)
      if (!isalnum(i)) pcs->Translate[i] &= ~mask;
   skipblanks();
   /* now set this flag for all specified chars */
   while (!isEol(ch)) {
      if (!isalnum(ch)) pcs->Translate[ch] |= mask;
      nextch();
   }
}

static void
check_reentry(prefix *tag)
{
   if (tag->filename) {
      if (tag->line != file.line ||
	  strcmp(tag->filename, file.filename) != 0) {
	 compile_warning(/*Reentering an existing prefix level is deprecated (previously entered at %s:%lu)*/29,
			 tag->filename, tag->line);
      }
   } else {
      tag->filename = file.filename;
      tag->line = file.line;
   }
}

static void
set_prefix(void)
{
   prefix *tag = read_prefix(fFalse);
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
   compile_warning(/**prefix is deprecated - use *begin and *end instead*/6);
   check_reentry(tag);
}

static void
begin_block(void)
{
   prefix *tag;
   settings *pcsNew;

   pcsNew = osnew(settings);
   *pcsNew = *pcs; /* copy contents */
   pcsNew->begin_lineno = file.line;
   pcsNew->next = pcs;
   pcs = pcsNew;

   tag = read_prefix(fTrue);
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
free_settings(settings *pcs) {
   /* don't free default ordering or ordering used by parent */
   datum *order = pcs->ordering;
   if (order != default_order && order != pcs->next->ordering) osfree(order);

   /* free Translate if not used by parent */
   if (pcs->Translate != pcs->next->Translate) osfree(pcs->Translate - 1);

   osfree(pcs);
}

static void
end_block(void)
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

   tag = read_prefix(fTrue); /* need to read using root *before* BEGIN */
   if (tag != tagBegin) {
      if (tag) {
	 if (!tagBegin) {
	    /* "*begin" / "*end foo" */
	    compile_error(/*Matching BEGIN tag has no prefix*/36);
	 } else {
	    /* tag mismatch */
	    compile_error(/*Prefix tag doesn't match BEGIN*/193);
	 }
	 skipline();
      } else {
	 /* close tag omitted; open tag given */
	 compile_warning(/*Closing prefix omitted from END*/194);
      }
   }
}

static void
fix_station(void)
{
   prefix *fix_name;
   node *stn;
   static node *stnOmitAlready = NULL;
   real x, y, z;
   bool fRef = 0;
   long fp;

   fix_name = read_prefix(fFalse);
   stn = StnFromPfx(fix_name);

   fp = ftell(file.fh);
   get_token();
   if (strcmp(ucbuffer, "REFERENCE") != 0) {
      if (*ucbuffer) fseek(file.fh, fp, SEEK_SET);
   } else {
      fRef = 1;
   }
   x = read_numeric(fTrue);
   if (x == HUGE_REAL) {
      if (stnOmitAlready) {
	 if (stn != stnOmitAlready)
	    compile_error(/*More than one FIX command with no coordinates*/56);
	 else
	    compile_warning(/*Same station fixed twice with no coordinates*/61);
	 skipline();
	 return;
      }
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
	 sdy = read_numeric(fTrue);
	 if (sdy == HUGE_REAL) {
	    /* only one variance given */
	    sdy = sdz = sdx;
	 } else {
	    sdz = read_numeric(fFalse);
	    if (sdz == HUGE_REAL) {
	       /* two variances given - horizontal & vertical */
	       sdz = sdy;
	       sdy = sdx;
	    }
	    /* FIXME: covariances... ? */
	 }
	 if (!fixed(stn)) {
	    node *fixpt = osnew(node);
	    prefix *name = osnew(prefix);
	    name->ident = ""; /* root has ident[0] == "\" */
	    fixpt->name = name;
	    name->pos = osnew(pos);
	    name->pos->shape = 0;
	    name->stn = fixpt;
	    name->up = NULL;
	    name->exported = 0;
	    add_stn_to_list(&stnlist, fixpt);
	    POS(fixpt, 0) = x;
	    POS(fixpt, 1) = y;
	    POS(fixpt, 2) = z;
	    fix(fixpt);
	    addfakeleg(fixpt, stn, 0, 0, 0,
		       sdx * sdx, sdy * sdy, sdz * sdz
#ifndef NO_COVARIANCES
		       , 0, 0, 0
#endif
		       );
	    /* FIXME: unused fixed point warnings don't work for stations
	     * fixed with sds (though they should) so fRef can be ignored
	     * for now */
	 }
	 return;
      }
   }

   /* suppress "unused fixed point" warnings for this station */
   if (fRef && fix_name->pos->shape == 0) fix_name->pos->shape = -1;

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
      skipline();
      return;
   }
   compile_warning(/*Station already fixed at the same coordinates*/55);
}

/* helper function for replace_pfx */
static void
replace_pfx_(node *stn, node *from, pos *pos_replace, pos *pos_with)
{
   int d;
   stn->name->pos = pos_with;
   for (d = 0; d < 3; d++) {
      linkfor *leg = stn->leg[d];
      node *to;
      if (!leg) break;
      to = leg->l.to;
      if (to == from) continue;

      if (fZero(data_here(leg) ? &leg->v : &reverse_leg(leg)->v))
	 replace_pfx_(to, stn, pos_replace, pos_with);
   }
}

/* We used to iterate over the whole station list (inefficient) - now we
 * just look at any neighbouring nodes to see if they are equated */
static void
replace_pfx(const prefix *pfx_replace, const prefix *pfx_with)
{
   pos *pos_replace;
   ASSERT(pfx_replace);
   ASSERT(pfx_with);
   pos_replace = pfx_replace->pos;
   ASSERT(pos_replace != pfx_with->pos);

   replace_pfx_(pfx_replace->stn, NULL, pos_replace, pfx_with->pos);

#if DEBUG_INVALID
   {
      node *stn;
      FOR_EACH_STN(stn, stnlist) {
	 ASSERT(stn->name->pos != pos_replace);
      }
   }
#endif

   /* free the (now-unused) old pos */
   osfree(pos_replace);
}

static void
equate_list(void)
{
   node *stn1, *stn2;
   prefix *name1, *name2;
   bool fOnlyOneStn = fTrue; /* to trap eg *equate entrance.6 */

   name1 = read_prefix_check_implicit(fFalse);
   stn1 = StnFromPfx(name1);
   while (fTrue) {
      stn2 = stn1;
      name2 = name1;
      name1 = read_prefix_check_implicit(fTrue);
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

      if (name1 == name2) {
	 /* catch something like *equate "fred fred" */
	 compile_warning(/*Station equated to itself*/13);
	 /* FIXME: this won't catch: "*equate fred jim fred" */
      }
      fOnlyOneStn = fFalse;
      stn1 = StnFromPfx(name1);
      /* equate nodes if not already equated */
      if (name1->pos != name2->pos) {
	 if (fixed(stn1)) {
	    if (fixed(stn2)) {
	       /* both are fixed, but let them off iff their coordinates match */
	       int d;
	       for (d = 2; d >= 0; d--)
		  if (name1->pos->p[d] != name2->pos->p[d]) {
		     compile_error(/*Tried to equate two non-equal fixed stations*/52);
		     showandskipline(NULL, 0);
		     return;
		  }
	       compile_warning(/*Equating two equal fixed points*/53);
	       showline(NULL, 0);
	    }

	    /* stn1 is fixed, so replace all refs to stn2's pos with stn1's */
	    replace_pfx(name2, name1);
	 } else {
	    /* stn1 isn't fixed, so replace all refs to its pos with stn2's */
	    replace_pfx(name1, name2);
	 }

	 /* count equates as legs for now... */
#ifdef NO_COVARIANCES
	 addleg(stn1, stn2,
		(real)0.0, (real)0.0, (real)0.0,
		(real)0.0, (real)0.0, (real)0.0);
#else
	 addleg(stn1, stn2,
		(real)0.0, (real)0.0, (real)0.0,
		(real)0.0, (real)0.0, (real)0.0,
		(real)0.0, (real)0.0, (real)0.0);
#endif
      }
   }
}

static void
export(void)
{
   while (1) {
      prefix *pfx = read_prefix(fTrue);
      int depth = 0;
      /* FIXME: empty export OK? probably... */
      if (pfx == NULL) break;
      {
         prefix *p = pfx;
	 while (p != NULL && p != pcs->Prefix) {
	    depth++;
	    p = p->up;
	 }
	 if (p == NULL) {
	    /* FIXME: something like: *export \foo */
	    NOT_YET;
	 }
      }
      if (depth == 0) {
         /* FIXME: *export \ or similarly bogus shit */
         NOT_YET;
      }
      if (depth < pfx->exported + 1) {
         /* FIXME: warning already exported */
         printf("*** warning already exported depth %d pfx->exported %d\n", depth, pfx->exported);
      } else if (depth > pfx->exported + 1) {
         /* FIXME: not exported enough */
         printf("*** warning not exported enough depth %d pfx->exported %d\n", depth, pfx->exported);
      } else {
         pfx->exported++;
	 printf("*** exporting %s to %d\n", sprint_prefix(pfx), pfx->exported);
      }
   }
}

static void
data(void)
{
   /* FIXME: also BackComp, BackClino, Dr */
   static sztok dtab[] = {
#ifdef SVX_MULTILINEDATA /* NEW_STYLE */
	{"BACK",         Back },
#endif
	{"BEARING",      Comp },
	{"CLINO",        Clino }, /* alternative name */
	{"COMPASS",      Comp }, /* alternative name */
	{"DX",		 Dx },
	{"DY",		 Dy },
	{"DZ",		 Dz },
	{"FROM",         Fr },
	{"FROMCOUNT",    FrCount },
	{"FROMDEPTH",    FrDepth },
	{"GRADIENT",     Clino },
	{"IGNORE",       Ignore },
	{"IGNOREALL",    IgnoreAll },
	{"LENGTH",       Tape },
#ifdef SVX_MULTILINEDATA /* NEW_STYLE */
	{"NEXT",         Next },
#endif
	{"TAPE",         Tape }, /* alternative name */
	{"TO",           To },
	{"TOCOUNT",      ToCount },
	{"TODEPTH",      ToDepth },
	{NULL,           End }
   };

   static style fn[] = {
      data_normal,
      data_normal,
      data_diving,
      data_cartesian,
      data_nosurvey
   };

   /* readings which may be given for each style */
   static ulong mask[] = {
      BIT(Fr) | BIT(To) | BIT(Tape) | BIT(Comp) | BIT(Clino),
      BIT(Fr) | BIT(To) | BIT(FrCount) | BIT(ToCount) | BIT(Comp) | BIT(Clino),
      BIT(Fr) | BIT(To) | BIT(Tape) | BIT(Comp) | BIT(FrDepth) | BIT(ToDepth),
      BIT(Fr) | BIT(To) | BIT(Dx) | BIT(Dy) | BIT(Dz),
      BIT(Fr) | BIT(To)
   };

   /* readings which may be omitted for each style */
   static ulong mask_optional[] = {
      BIT(Clino),
      BIT(Clino),
      0,
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

   int style, k = 0, kMac;
   datum *new_order, d;
   unsigned long m, mUsed = 0;

   kMac = 6; /* minimum for NORMAL style */
   new_order = osmalloc(kMac * sizeof(datum));

   get_token();
   style = match_tok(styletab, TABSIZE(styletab));

   if (style == STYLE_DEFAULT) {
      default_style(pcs);
      return;
   }

   if (style == STYLE_UNKNOWN) {
      compile_error(/*STYLE `%s' unknown*/65, buffer);
      skipline();
      return;
   }

   pcs->Style = fn[style - 1];
#ifdef SVX_MULTILINEDATA /* NEW_STYLE */
   m = mask[style - 1] | BIT(Back) | BIT(Next) | BIT(Ignore) | BIT(IgnoreAll) | BIT(End);
#else
   m = mask[style - 1] | BIT(Ignore) | BIT(IgnoreAll) | BIT(End);
#endif

   skipblanks();
   /* olde syntax had optional field for survey grade, so allow an omit */
   if (isOmit(ch)) nextch();

   do {
      get_token();
      d = match_tok(dtab, TABSIZE(dtab));
      /* An unknown token is reported as trailing garbage */
      if (!TSTBIT(m, d)) {
	 /* token not valid for this data style */
	 compile_error(/*%s: Datum not allowed for this style*/63, buffer);
	 skipline();
	 return;
      }
#ifdef SVX_MULTILINEDATA /* NEW_STYLE */
      /* Check for duplicates unless it's a special datum:
       *   IGNORE,NEXT (duplicates allowed)
       *   END,IGNOREALL (not possible)
       */
      if (d == Next && (mUsed & BIT(Back))) {
	 /* FIXME: "... back ... next ..." not allowed */
      }
# define mask_dup_ok (BIT(Ignore) | BIT(End) | BIT(IgnoreAll) | BIT(Next))
      if (!(mask_dup_ok & BIT(d))) {
#else
      /* check for duplicates unless it's IGNORE (duplicates allowed) */
      /* or End/IGNOREALL (not possible) */
      if (d != Ignore && d != End && d != IgnoreAll) {
# if 0
	 cRealData++;
	 if (cRealData > cData) {
	    compile_error(/*Too many data*/);
	    showandskipline(NULL, -(int)strlen(buffer));
	    return;
	 }
# endif
#endif
	 if (mUsed & BIT(d)) {
	    compile_error(/*Duplicate datum `%s'*/67, buffer);
	    skipline();
	    return;
	 }
	 mUsed |= BIT(d); /* used to catch duplicates */
      }
      if (k >= kMac) {
	 kMac = kMac * 2;
	 new_order = osrealloc(new_order, kMac * sizeof(datum));
      }
      new_order[k++] = d;
#define mask_done (BIT(End) | BIT(IgnoreAll))
   } while (!(mask_done & BIT(d)));

   if ((mUsed | mask_optional[style-1]) != mask[style-1]) {
      osfree(new_order);
      compile_error(/*Too few data*/64);
      skipline();
      return;
   }

   /* don't free default ordering or ordering used by parent */
   if (pcs->ordering != default_order &&
       !(pcs->next && pcs->next->ordering == pcs->ordering))
      osfree(pcs->ordering);

   pcs->ordering = new_order;
}

/* masks for units which are length and angles respectively */
#define LEN_UMASK (BIT(UNITS_METRES) | BIT(UNITS_FEET) | BIT(UNITS_YARDS))
#define ANG_UMASK (BIT(UNITS_DEGS) | BIT(UNITS_GRADS) | BIT(UNITS_MINUTES))

/* ordering must be the same as the units enum */
static real factor_tab[] = {
   1.0, METRES_PER_FOOT, (METRES_PER_FOOT*3.0),
   (PI/180.0), (PI/200.0), 0.0, (PI/180.0/60.0)
};

static void
units(void)
{
   int units, quantity;
   ulong qmask, m; /* mask with bit x set to indicate quantity x specified */
   real factor;
   long fp;

   qmask = get_qlist();
   if (!qmask) return;
   if (qmask == BIT(Q_DEFAULT)) {
      default_units(pcs);
      return;
   }

   fp = ftell(file.fh);
   /* If factor given then read units else units in buffer already */
   factor = read_numeric(fTrue);
   if (factor == HUGE_REAL) {
      factor = (real)1.0;
      /* If factor given then read units else units in buffer already */
   } else {
      /* eg check for stuff like: *UNITS LENGTH BOLLOX 3.5 METRES */
      if (*buffer != '\0') {
	 fseek(file.fh, fp, SEEK_SET);
	 compile_error(/*Unknown quantity `%s'*/34, buffer);
	 skipline();
	 return;
      }
      get_token();
   }

   units = match_units();
   if (units == UNITS_NULL) return;
   factor *= factor_tab[units];

   if (((qmask & LEN_QMASK) && !(BIT(units) & LEN_UMASK)) ||
       ((qmask & ANG_QMASK) && !(BIT(units) & ANG_UMASK))) {
      compile_error(/*Invalid units for quantity*/37);
      showandskipline(NULL, -(int)strlen(buffer));
      return;
   }
   if (qmask & (BIT(Q_LENGTHOUTPUT) | BIT(Q_ANGLEOUTPUT)) ) {
      NOT_YET;
   }
   for (quantity = 0, m = BIT(quantity); m <= qmask; quantity++, m <<= 1)
      if (qmask & m) pcs->units[quantity] = factor;
}

static void
calibrate(void)
{
   real sc, z; /* added so we don't modify current values if error given */
   ulong qmask, m;
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
   /* perhaps "*scale declination XXX" should be "*declination XXX" ? */
   if ((qmask & BIT(Q_DECLINATION)) && sc != 1.0) {
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

#define EQ(S) (strcmp(ucbuffer, (S)) == 0)
static void
set_default(void)
{
   compile_warning(/**default is deprecated - use *calibrate/data/style/units with argument DEFAULT instead*/20);
   get_token();
   if (EQ("CALIBRATE")) {
      default_calib(pcs);
   } else if (EQ("DATA")) {
      default_style(pcs);
      default_grade(pcs);
   } else if (EQ("UNITS")) {
      default_units(pcs);
   } else {
      compile_error(/*Unknown setting `%s'*/41, buffer);
      skipline();
   }
}
#undef EQ

static void
include(void)
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
set_sd(void)
{
   real sd, variance;
   int units;
   ulong qmask, m;
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
   get_token();
   units = match_units();
   if (units == UNITS_NULL) return;
   if (((qmask & LEN_QMASK) && !(BIT(units) & LEN_UMASK)) ||
       ((qmask & ANG_QMASK) && !(BIT(units) & ANG_UMASK))) {
      compile_error(/*Invalid units for quantity*/37);
      showandskipline(NULL, -(int)strlen(buffer));
      return;
   }

   sd *= factor_tab[units];
   variance = sqrd(sd);

   for (quantity = 0, m = BIT(quantity); m <= qmask; quantity++, m <<= 1)
      if (qmask & m) pcs->Var[quantity] = variance;
}

static void
set_title(void)
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
     {"TOLOWER",  UPPER},
     {"TOUPPER",  LOWER},
     {NULL,       -1}
};

static void
case_handling(void)
{
   int setting;
   get_token();
   setting = match_tok(case_tab, TABSIZE(case_tab));
   if (setting != -1) {
      pcs->Case = setting;
   } else {
      compile_error(/*Found `%s', expecting `OFF', `UPPER', or `LOWER'*/10,
		    buffer);
      skipline();
   }
}

static sztok infer_tab[] = {
#if 0 /* FIXME allow EQUATES to be inferred too */
     {"EQUATES",    2},
#endif
     {"PLUMBS",     1},
     {NULL,      0}
};

static sztok onoff_tab[] = {
     {"OFF", 0},
     {"ON",  1},
     {NULL,       -1}
};

static void
infer(void)
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
#if 0
    case 2:
      pcs->f0Eq = on;
      break;
#endif
    case 1:
      pcs->f90Up = on;
      break;
    default:
      BUG("unexpected case");
   }
}

static void
set_truncate(void)
{
   unsigned int truncate_at = 0; /* default is no truncation */
   long fp = ftell(file.fh);
   get_token();
   if (strcmp(ucbuffer, "OFF") != 0) {
      if (*ucbuffer) fseek(file.fh, fp, SEEK_SET);
      truncate_at = read_uint();
   }
   /* for backward compatibility, "*truncate 0" means "*truncate off" */
   pcs->Truncate = (truncate_at == 0) ? INT_MAX : truncate_at;
}

static void
require(void)
{
   static char version[] = VERSION;
   char *p = version;
   long fp;
   char ch_old;
   skipblanks();
   ch_old = ch;
   fp = ftell(file.fh);
   while (1) {
      unsigned int have, want;
      have = (unsigned int)strtoul(p, &p, 10);
      want = read_uint();
      if (have > want) break;
      if (have < want) {
	 size_t i, len;
	 char *v;
	 /* find end of version number */
	 while (isdigit(ch) || ch == '.') nextch();
	 len = (size_t)(ftell(file.fh) - fp);
	 v = osmalloc(len + 1);
	 fseek(file.fh, fp, SEEK_SET);
	 ch = ch_old;
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

extern void
handle_command(void)
{
   get_token();
   switch (match_tok(cmd_tab, TABSIZE(cmd_tab))) {
    case CMD_CALIBRATE: calibrate(); break;
    case CMD_CASE: case_handling(); break;
    case CMD_DATA: data(); break;
    case CMD_DEFAULT: set_default(); break;
    case CMD_EQUATE: equate_list(); break;
    case CMD_EXPORT: export(); break;
    case CMD_FIX: fix_station(); break;
    case CMD_INCLUDE: include(); break;
    case CMD_INFER: infer(); break;
    case CMD_LRUD: {
       /* just ignore *lrud for now so Tunnel can put it in */
       /* FIXME warn? */
       skipline();
       break;
    }
    case CMD_MEASURE: NOT_YET; break;
    case CMD_PREFIX: set_prefix(); break;
    case CMD_UNITS: units(); break;
    case CMD_BEGIN: begin_block(); break;
    case CMD_END: end_block(); break;
    case CMD_REQUIRE: require(); break;
    case CMD_SOLVE: solve_network(/*stnlist*/); break;
    case CMD_SD: set_sd(); break;
    case CMD_SET: set_chars(); break;
    case CMD_TITLE: set_title(); break;
    case CMD_TRUNCATE: set_truncate(); break;
    default:
      compile_error(/*Unknown command `%s'*/12, buffer);
      skipline();
   }
}
