/* > defaults.c
 * Code to set up defaults of things for Survex
 * Copyright (C) 1991-1997 Olly Betts
 */

/*
1993.01.25 PC Compatibility OKed
1993.05.02 settings.Translate[] is now short not long
1993.05.03 settings.Ascii -> settings.fAscii
1993.06.04 FALSE -> fFalse
1993.06.16 lowercased filename
1993.08.10 ^Z is now SPECIAL_EOL by default, so olde DOS text files read ok
           fettled a little
1993.08.13 fettled header
1993.11.11 cs.Style is now a function pointer
1994.03.16 the order of the columns can now be varied
1994.03.21 added sd for plumbed legs
1994.04.18 uses |= rather than = to set ^Z to SPECIAL_EOL; created defaults.h
1994.06.22 cs.ordering is now osfree-able
1994.06.27 fixed bug - omly allocated 4 bytes to data ordering
1994.08.15 angular sds were in degrees not radians
1994.10.08 osnew() added
1994.11.12 added new sd stuff
1994.11.13 various
1995.01.29 no longer predivide posn error by 3.0
1996.01.11 added characters needed for schwabenschact
1996.02.13 removed chars for schwab again (since *set added)
1996.03.23 Translate is now a pointer, one elt into array
1996.04.15 SPECIAL_POINT -> SPECIAL_DECIMAL
1997.08.24 Unique -> Truncate
1998.03.21 fixed up to compile cleanly on Linux
*/

#include <limits.h>

#include "survex.h"
#include "error.h"
#include "datain.h" /* for extern void data_normal( void ); */
#include "defaults.h"

extern void default_grade( settings *s ) {
  s->Var[Q_POS]      = (real)sqrd(0.10);     /*pos*/
  s->Var[Q_LENGTH]   = (real)sqrd(0.10);     /*tape*/
  s->Var[Q_BEARING]  = (real)sqrd(rad(1.0)); /*comp*/
  s->Var[Q_GRADIENT] = (real)sqrd(rad(1.0)); /*clino*/
  /* SD of plumbed legs (0.25 degrees?) */
  s->Var[Q_PLUMB]    = (real)sqrd(rad(0.25));/*plumb*/
  s->Var[Q_DEPTH]    = (real)sqrd(0.10);     /*depth*/
}

extern void default_truncate( settings *s ) { s->Truncate = INT_MAX; }

extern void default_ascii( settings *s ) { s->fAscii=fFalse; }

extern void default_90up( settings *s ) { s->f90Up=fFalse; }

extern void default_case( settings *s ) { s->Case=LOWER; }

extern void default_style( settings *s ) {
  static datum normal_default_order[]={ Fr, To, Tape, Comp, Clino, End };
  s->Style=(data_normal);
  s->ordering=normal_default_order;
  s->fOrderingFreeable=fFalse;
}

extern void default_prefix( settings *s ) { s->Prefix=root; }

extern void default_translate( settings *s ) {
  int i;
  short *t;
  if (s->next && s->next->Translate==s->Translate) {
     t=((short*)osmalloc(ossizeof(short)*257))+1;
     memcpy(t-1,s->Translate-1,sizeof(short)*257);
     s->Translate=t;
  }
/*  ASSERT(EOF==-1);*/ /* important, since we rely on this */
  t=s->Translate;
  for ( i=-1 ; i<=255 ; i++ )
    t[i]=0;
  for ( i='0' ; i<='9' ; i++)
    t[i] |= SPECIAL_NAMES;
  for ( i='A' ; i<='Z' ; i++)
    t[i] |= SPECIAL_NAMES;
  for ( i='a' ; i<='z' ; i++)
    t[i] |= SPECIAL_NAMES;

  t[TAB]|=SPECIAL_BLANK;
  t[' ']|=SPECIAL_BLANK;
  t[',']|=SPECIAL_BLANK;
  t[';']|=SPECIAL_COMMENT;
  t['\032']|=SPECIAL_EOL; /* Ctrl-Z, so olde DOS text files are handled ok */
  t[EOF]|=SPECIAL_EOL;
  t[LF ]|=SPECIAL_EOL;
  t[CR ]|=SPECIAL_EOL;
  t['*']|=SPECIAL_KEYWORD;
  t['-']|=SPECIAL_OMIT;
  t['\\']|=SPECIAL_ROOT;
  t['.']|=SPECIAL_SEPARATOR;
  t['_']|=SPECIAL_NAMES;
  t['.']|=SPECIAL_DECIMAL;
  t['-']|=SPECIAL_MINUS;
  t['+']|=SPECIAL_PLUS;
}

extern void default_units( settings *s ) {
  int quantity;
  for( quantity=0 ; quantity<Q_MAC ; quantity++ ) {
    if ((ANG_QMASK>>quantity)&1)
      s->units[quantity]=(real)(PI/180.0); /* degrees */
    else
      s->units[quantity]=(real)1.0; /* metres */
  }
}

extern void default_calib( settings *s ) {
  int quantity;
  for( quantity=0 ; quantity<Q_MAC ; quantity++ ) {
    s->z[quantity]=(real)0.0;
    s->sc[quantity]=(real)1.0;
  }
}
