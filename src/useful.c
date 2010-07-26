/* useful.c
 * Copyright (C) 1993-2001,2003,2010 Olly Betts
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "useful.h"
#include "osdepend.h"

#ifndef WORDS_BIGENDIAN

/* used by macro versions of useful_get<nn> functions */
int16_t useful_w16;
int32_t useful_w32;

#else

extern void Far
useful_put16(int16_t w, FILE *fh)
{
   putc((char)(w), fh);
   putc((char)(w >> 8l), fh);
}

extern void Far
useful_put32(int32_t w, FILE *fh)
{
   putc((char)(w), fh);
   putc((char)(w >> 8l), fh);
   putc((char)(w >> 16l), fh);
   putc((char)(w >> 24l), fh);
}

extern int16_t Far
useful_get16(FILE *fh)
{
   int16_t w;
   w = getc(fh);
   w |= (int16_t)(getc(fh) << 8l);
   return w;
}

extern int32_t Far
useful_get32(FILE *fh)
{
   int32_t w;
   w = getc(fh);
   w |= (int32_t)(getc(fh) << 8l);
   w |= (int32_t)(getc(fh) << 16l);
   w |= (int32_t)(getc(fh) << 24l);
   return w;
}
#endif
