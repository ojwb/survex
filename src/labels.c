/* > labels.c
 * Clever non-overlapping label plotting code
 * Copyright (C) 1995,1996,1997,2001 Olly Betts
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
#include "osalloc.h"
#include "labels.h"

#include "caverot.h" /* only for fAllNames - FIXME: try to eliminate */

static char **map;
static int width, height;

static unsigned int x_mid, y_mid;

int
init_map(unsigned int w, unsigned int h)
{
   int i;
   x_mid = w / 2u;
   y_mid = h / 2u;
   width = w / 4u;
   height = h / 4u;
   map = osmalloc(height * ossizeof(void*));
   if (!map) return 0;
#ifdef __TURBOC__
   /* under 16 bit MSDOS compilers, allocate each line separately to avoid
    * problems if we exceed the segment size */
   for (i = 0; i < height; i++) {
      map[i] = osmalloc(width);
      if (!map[i]) {
         while (--i >= 0) osfree(map[i]);
         osfree(map);
         return 0;
      }
   }
#else
   /* otherwise allocate in one block so we can zero with one memset() */
   map[0] = osmalloc(width * height);
   if (!map[0]) {
      osfree(map);
      return 0;
   }
   for (i = 1; i < height; i++) {
      map[i] = map[i - 1] + width;
   }
#endif
   return 1;
}

void
clear_map(void)
{
#ifdef __TURBOC__
   int i;
   for (i = 0; i < height; i++) memset(map[i], 0, width);
#else
   memset(map[0], 0, width * height);
#endif
}

#if (OS==RISCOS)
/* This gets called from some ARM code which corrupts
 * the APCS stack frame registers.  no_check_stack doesn't
 * seem to use their values, but it will corrupt ip (ie r12) */
# pragma no_check_stack
#endif

int
fancy_label(const char *label, int x, int y)
{
   if (!fAllNames) {
      unsigned int X, Y; /* use unsigned so we can test for <0 for free */
      int len, l, m;
      X = (unsigned)(x + x_mid) / 4u;
      if (X >= width) return 0;
      Y = (unsigned)(y + y_mid) / 4u;
      if (Y >= height) return 0;
      len = (strlen(label) * 2) + 1;
      if (X + len >= width) len = width - X; /* or 'return' to right clip */
      for (l = len - 1; l >= 0; l--)
	 if (map[Y][X + l]) return 0;
      l = Y - 2;
      if (l < 0) l = 0;
      m = Y + 2;
      if (m >= height) m = height - 1;
      for ( ; l <= m; l++)
         memset(map[l] + X, 1, len);
   }
   return 1;
}

#if (OS==RISCOS)
# pragma check_stack
#endif
