/* > defaults.c
 * Code to set up defaults of things for Survex
 * Copyright (C) 1991-1997 Olly Betts
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <limits.h>

#include "cavern.h"
#include "filename.h"
#include "message.h"
#include "datain.h" /* for extern void data_normal( void ); */
#include "defaults.h"

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

extern void
default_grade(settings *s)
{
   s->Var[Q_POS] = (real)sqrd(0.10);
   s->Var[Q_LENGTH] = (real)sqrd(0.10);
   s->Var[Q_BEARING] = (real)sqrd(rad(1.0));
   s->Var[Q_GRADIENT] = (real)sqrd(rad(1.0));
   /* SD of plumbed legs (0.25 degrees?) */
   s->Var[Q_PLUMB] = (real)sqrd(rad(0.25));
   s->Var[Q_DEPTH] = (real)sqrd(0.10);
}

extern void
default_truncate(settings *s)
{
   s->Truncate = INT_MAX;
}

extern void
default_90up(settings *s)
{
   s->f90Up = fFalse;
}

extern void
default_case(settings *s)
{
   s->Case = LOWER;
}

extern void
default_style(settings *s)
{
   static datum normal_default_order[] = { Fr, To, Tape, Comp, Clino, End };
   s->Style = (data_normal);
   s->ordering = normal_default_order;
   s->fOrderingFreeable = fFalse;
}

extern void
default_prefix(settings *s)
{
   s->Prefix = root;
}

extern void
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

extern void
default_units(settings *s)
{
   int quantity;
   for (quantity = 0; quantity < Q_MAC; quantity++) {
      if ((ANG_QMASK >> quantity) & 1)
	 s->units[quantity] = (real)(PI / 180.0); /* degrees */
      else
	 s->units[quantity] = (real)1.0; /* metres */
   }
}

extern void
default_calib(settings *s)
{
   int quantity;
   for (quantity = 0; quantity < Q_MAC; quantity++) {
      s->z[quantity] = (real)0.0;
      s->sc[quantity] = (real)1.0;
   }
}
