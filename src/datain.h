/* > datain.h
 * Header file for code that...
 * Reads in survey files, dealing with special characters, keywords & data
 * Copyright (C) 1994-2000 Olly Betts
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

#ifdef HAVE_SETJMP
# include <setjmp.h>
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
extern int data_normal(void);
extern int data_diving(void);
extern int data_cartesian(void);
extern int data_nosurvey(void);

void compile_warning(int en, ...);
void compile_error(int en, ...);
