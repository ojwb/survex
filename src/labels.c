/* > labels.c
 * Clever non-overlapping label plotting code
 * Copyright (C) 1995,1996,1997 Olly Betts
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
# include <config.h>
#endif

#include <string.h>
#include "filename.h"
#include "message.h"
#include "caverot.h"
#include "cvrotgfx.h"
#include "labels.h"

#if (OS==MSDOS) || (OS==UNIX)
# define really_plot(S, X, Y) outtextxy((X) + (unsigned)xcMac / 2u,\
                                        (Y) + (unsigned)ycMac / 2u, (S))
#endif

static char **map;
static int width, height;

extern int xcMac, ycMac;

void
init_map(void)
{
   int i;
   width = (unsigned)xcMac / 4u;
   height = (unsigned)ycMac / 4u;
   map = osmalloc(height * ossizeof(void*));
   if (!map) exit(1);
   for (i = 0; i < height; i++) {
      map[i] = osmalloc(width * ossizeof(char));
      if (!map[i]) exit(1);
   }
}

void
clear_map(void)
{
   int i;
   for (i = 0; i < height; i++) memset(map[i], 0, width);
}

#if (OS==RISCOS)
/* This gets called from some ARM code which corrupts
 * the APCS stack frame registers.  no_check_stack doesn't
 * seem to use their values, but it will corrupt ip (ie r12) */
# pragma no_check_stack
#endif

void
fancy_label(const char *label, int x, int y)
{
#ifndef really_plot
   extern void really_plot(const char * /*label*/, int /*x*/, int /*y*/);
#endif
   if (!fAllNames) {
      unsigned int X, Y; /* use unsigned so we can test for <0 for free */
      int len, l, m;
      X = (unsigned)(x + (unsigned)xcMac / 2u) / 4u;
      if (X >= width) return;
      Y = (unsigned)(y + (unsigned)ycMac / 2u) / 4u;
      if (Y >= height) return;
      len = (strlen(label) * 2) + 1;
      if (X + len >= width) len = width - X; /* or 'return' to right clip */
      for (l = len - 1; l >= 0; l--)
	 if (map[Y][X + l]) return;
      l = Y - 2;
      if (l < 0) l = 0;
      m = Y + 2;
      if (m >= height) m = height - 1;
      for ( ; l <= m; l++)
         memset(map[l] + X, 1, len);
   }
   really_plot(label, x, y);
}

#if (OS==RISCOS)
# pragma check_stack
#endif
