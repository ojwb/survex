/* > datain.h
 * Header file for code that...
 * Reads in survey files, dealing with special characters, keywords & data
 * Copyright (C) 1994,1996,1997 Olly Betts
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

#ifdef HAVE_SETJMP
#include <setjmp.h>
#define errorjmp(EN, A, JB) BLK(\
 compile_error(EN); showandskipline(NULL, (A)); longjmp((JB), 1);)
#else
#define errorjmp(EN, A, JB) BLK(\
 compile_error(EN); showandskipline(NULL, (A)); exit(1);)
/* FIXME: sort out above to exit in a more friendly way */
#endif

#include <stdio.h> /* for FILE */

typedef struct parse {
   FILE *fh;
   char *filename;
   unsigned int line;
   struct parse *parent;
#ifdef HAVE_SETJMP
   jmp_buf jbSkipLine;
#endif
} parse;

extern int ch;
extern parse file;

#define nextch() (ch = getc(file.fh))

extern void skipblanks(void);

/* reads complete data file */
extern void data_file(const char *pth, const char *fnm);

extern void skipline(void);
extern void showline(const char *dummy, int n);
extern void showandskipline(const char *dummy, int n);

/* style functions */
#ifdef NEW_STYLE /* FIXME: sort this out */
extern int data_normal(int /*action*/);
extern int data_diving(int /*action*/);
#else
extern int data_normal(void);
extern int data_diving(void);
#endif

#define STYLE_START 0
#define STYLE_READDATA 1
#define STYLE_END 2

void compile_warning(int en, ...);
void compile_error(int en, ...);
