/* > useful.c
 * Copyright (C) 1993-1996 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "useful.h"
#include "osdepend.h"

#ifndef WORDS_BIGENDIAN

/* used by macro versions of useful_get<nn> functions */
w16 useful_w16;
w32 useful_w32;

#if 0 /* these functions aren't needed - macros do the job */
/* the numbers in the file are little endian, so use fread/fwrite */
extern void FAR
useful_put16(w16 w, FILE *fh)
{
   fwrite(&w, 2, 1, fh);
}

#undef useful_put32
extern void FAR
useful_put32(w32 w, FILE *fh)
{
   fwrite(&w, 4, 1, fh);
}

#undef useful_get16
extern w16 FAR
useful_get16(FILE *fh)
{
   w16 w;
   fread(&w, 2, 1, fh);
   return w;
}

#undef useful_put32
extern w32 FAR
useful_get32(FILE *fh)
{
   w32 w;
   fread(&w, 4, 1, fh);
   return w;
}
#endif

#else

extern void FAR
useful_put16(w16 w, FILE *fh)
{
   putc((char)(w), fh);
   putc((char)(w >> 8l), fh);
}

extern void FAR
useful_put32(w32 w, FILE *fh)
{
   putc((char)(w), fh);
   putc((char)(w >> 8l), fh);
   putc((char)(w >> 16l), fh);
   putc((char)(w >> 24l), fh);
}

extern w16 FAR
useful_get16(FILE *fh)
{
   w16 w;
   w = getc(fh);
   w |= (w16)(getc(fh) << 8l);
   return w;
}

extern w32 FAR
useful_get32(FILE *fh)
{
   w32 w;
   w = getc(fh);
   w |= (w32)(getc(fh) << 8l);
   w |= (w32)(getc(fh) << 16l);
   w |= (w32)(getc(fh) << 24l);
   return w;
}
#endif

/* this reads a line from a stream to a buffer. Any eol chars are removed
 * from the file and the length of the string including '\0' returned
 */
extern int FAR
useful_getline(char *buf, OSSIZE_T len, FILE *fh)
{
   int i = 0;
   int ch;

   ch = getc(fh);
   while (ch != '\n' && ch != '\r' && ch != EOF) {
      if (i >= len - 1) {
         printf("LINE TOO LONG!\n"); /* FIXME */
         exit(1);
      }
      buf[i++] = ch;
      ch = getc(fh);
   }
   if (ch != EOF) {
      /* remove any further eol chars (for DOS) */
      do {
         ch = fgetc(fh);
      } while (ch == '\n' || ch == '\r');
      ungetc(ch, fh); /* we don't want it, so put it back */
   }
   buf[i] = '\0';
   return (i + 1);
}
