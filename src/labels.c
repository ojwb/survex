/* > labels.c
 * Clever non-overlapping label plotting code
 * Copyright (C) 1995,1996,1997 Olly Betts
 */

/*
1995.03.18 created
1995.03.21 fixed for RISC OS non-square pixel modes
1995.03.23 eig shifts are now done at the last moment
1995.03.24 hacked for DOS
1995.03.25 added fAllNames
1995.03.28 fixed fancy_label for DOS and reworked a little
1995.04.20 fixed typo ycmac -> ycMac
1996.03.22 labels.h created
1996.04.03 fixed Borland C problem
1997.01.26 speed tweak
1997.02.24 converted to cvrotgfx
1997.05.08 fettled
*/

#include <string.h>
#include "filename.h"
#include "message.h"
#include "caverot.h"
#include "cvrotgfx.h"
#include "labels.h"

#if (OS==MSDOS)
# define really_plot(S,X,Y) outtextxy( (X) + (unsigned)xcMac / 2u,\
                                       (Y) + (unsigned)ycMac / 2u, (S) )
#endif

static char **map;
static int width, height;

extern int xcMac, ycMac;

void init_map( void ) {
   int i;
   width = (unsigned)xcMac / 4u;
   height = (unsigned)ycMac / 4u;
   map = osmalloc(height * ossizeof(void*));
   if (!map)
      exit(1);
   for( i = 0; i < height; i++ )
      if ((map[i] = osmalloc(width * ossizeof(char))) == NULL)
         exit(1);
}

void clear_map( void ) {
   int i;
   for( i = 0; i < height; i++ )
      memset( map[i], 0, width );
}

#if (OS==RISCOS)
/* This gets called from some ARM code which corrupts
 * the APCS stack frame registers.  no_check_stack doesn't
 * seem to use their values, but it will corrupt ip (ie r12) */
# pragma no_check_stack
#endif

void fancy_label( char *label, int x, int y ) {
   extern bool fAllNames;
#ifndef really_plot
   extern void really_plot( char * /*label*/, int /*x*/, int /*y*/ );
#endif
   if (!fAllNames) {
      unsigned int X, Y; /* use unsigned so we can test for <0 for free */
      int len, l, m;
      X = (unsigned)(x + (unsigned)xcMac / 2u) / 4u;
      if (X >= width)
         return;
      Y = (unsigned)(y + (unsigned)ycMac / 2u) / 4u;
      if (Y >= height)
         return;
      len = (strlen(label) * 2) + 1;
      if ( X + len >= width) len = width - X; /* or 'return' to right clip */
      for ( l = len - 1; l >= 0; l-- )
         if (map[Y][X+l])
            return;
      l = Y - 2;
      if (l < 0) l = 0;
      m = Y + 2;
      if (m >= height)
         m = height - 1;
      for( ; l <= m; l++ )
         memset( map[l] + X, 1, len );
   }
   really_plot( label, x, y );
}

#if (OS==RISCOS)
# pragma check_stack
#endif
