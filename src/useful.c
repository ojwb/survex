/* useful.c
 * Copyright (C) 1993-2001,2003 Olly Betts
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

#include "useful.h"
#include "osdepend.h"

#ifndef WORDS_BIGENDIAN

/* used by macro versions of useful_get<nn> functions */
INT16_T useful_w16;
INT32_T useful_w32;

#if 0 /* these functions aren't needed - macros do the job */
/* the numbers in the file are little endian, so use fread/fwrite */
extern void Far
useful_put16(INT16_T w, FILE *fh)
{
   fwrite(&w, 2, 1, fh);
}

#undef useful_put32
extern void Far
useful_put32(INT32_T w, FILE *fh)
{
   fwrite(&w, 4, 1, fh);
}

#undef useful_get16
extern INT16_T Far
useful_get16(FILE *fh)
{
   INT16_T w;
   fread(&w, 2, 1, fh);
   return w;
}

#undef useful_put32
extern INT32_T Far
useful_get32(FILE *fh)
{
   INT32_T w;
   fread(&w, 4, 1, fh);
   return w;
}
#endif

#else

extern void Far
useful_put16(INT16_T w, FILE *fh)
{
   putc((char)(w), fh);
   putc((char)(w >> 8l), fh);
}

extern void Far
useful_put32(INT32_T w, FILE *fh)
{
   putc((char)(w), fh);
   putc((char)(w >> 8l), fh);
   putc((char)(w >> 16l), fh);
   putc((char)(w >> 24l), fh);
}

extern INT16_T Far
useful_get16(FILE *fh)
{
   INT16_T w;
   w = getc(fh);
   w |= (INT16_T)(getc(fh) << 8l);
   return w;
}

extern INT32_T Far
useful_get32(FILE *fh)
{
   INT32_T w;
   w = getc(fh);
   w |= (INT32_T)(getc(fh) << 8l);
   w |= (INT32_T)(getc(fh) << 16l);
   w |= (INT32_T)(getc(fh) << 24l);
   return w;
}
#endif
