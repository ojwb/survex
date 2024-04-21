/* useful.c
 * Copyright (C) 1993-2021 Olly Betts
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

#include <config.h>

#include "useful.h"
#include "osdepend.h"

#ifdef WORDS_BIGENDIAN

extern void
useful_put16(int16_t word, FILE *fh)
{
   uint16_t w = (uint16_t)word;
   PUTC((char)(w), fh);
   PUTC((char)(w >> 8l), fh);
}

extern void
useful_put32(int32_t word, FILE *fh)
{
   uint32_t w = (uint32_t)word;
   PUTC((char)(w), fh);
   PUTC((char)(w >> 8l), fh);
   PUTC((char)(w >> 16l), fh);
   PUTC((char)(w >> 24l), fh);
}

extern int16_t
useful_get16(FILE *fh)
{
   uint16_t w;
   w = GETC(fh);
   w |= (uint16_t)(GETC(fh) << 8l);
   return (int16_t)w;
}

extern int32_t
useful_get32(FILE *fh)
{
   uint32_t w;
   w = GETC(fh);
   w |= (uint32_t)(GETC(fh) << 8l);
   w |= (uint32_t)(GETC(fh) << 16l);
   w |= (uint32_t)(GETC(fh) << 24l);
   return (int32_t)w;
}
#endif
