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

static char *map = NULL;
static unsigned int width, height;
static unsigned int size;

static unsigned int x_mid, y_mid;

static bool fAllNames = fFalse;

int
labels_init(unsigned int w, unsigned int h)
{
   x_mid = w / 2u;
   y_mid = h / 2u;
   width = w / 4u;
   height = h / 4u;
   size = width * height;
   if (map) osfree(map);
   /* Allocate in one block for simplicity and space efficiency.
    * Under 16 bit MSDOS compilers, this limits us to resolutions with a
    * product strictly less than 1024*1024 - but Borland's BGI doesn't
    * support any resolution this high so it's not a problem.
    */
   map = xosmalloc(size);
   return (map != NULL);
}

void
labels_reset(void)
{
   if (map) memset(map, 0, size);       
}

void
labels_plotall(bool plot_all)
{
   fAllNames = plot_all;
}

#if (OS==RISCOS)
/* This gets called from some ARM code which corrupts
 * the APCS stack frame registers.  no_check_stack doesn't
 * seem to use their values, but it will corrupt ip (ie r12) */
# pragma no_check_stack
#endif

int
labels_plot(const char *label, int x, int y)
{
   if (map && !fAllNames) {
      unsigned int X, Y; /* use unsigned so we can test for <0 for free */
      unsigned int len, rows;
      char *p;      
      X = (unsigned)(x + x_mid) / 4u;
      if (X >= width) return 0;
      Y = (unsigned)(y + y_mid) / 4u;
      if (Y >= height) return 0;
      len = (strlen(label) * 2) + 1;
      if (X + len > width) len = width - X; /* or 'return' to right clip */
      p = map + Y * width + X;
      if (memchr(p, 1, len)) return 0;
      rows = max(Y, 2);
      p -= width * rows;
      rows += 3;
      while (rows && p < map + size) {
	 memset(p, 1, len);
	 p += width;
	 rows--;
      }
   }
   return 1;
}

#if (OS==RISCOS)
# pragma check_stack
#endif
