/* datain.h
 * Header file for code that...
 * Reads in survey files, dealing with special characters, keywords & data
 * Copyright (C) 1994-2002,2005,2010,2012,2014 Olly Betts
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

#ifdef HAVE_SETJMP_H
# include <setjmp.h>
#endif

#include <stdio.h> /* for FILE */

typedef struct parse {
   FILE *fh;
   const char *filename;
   unsigned int line;
   long lpos;
   bool reported_where;
   struct parse *parent;
#ifdef HAVE_SETJMP_H
   jmp_buf jbSkipLine;
#endif
} parse;

extern int ch;
extern parse file;
extern bool f_export_ok;

#define nextch() (ch = GETC(file.fh))

typedef struct {
   long offset;
   int ch;
} filepos;

void get_pos(filepos *fp);
void set_pos(const filepos *fp);

void skipblanks(void);

/* reads complete data file */
void data_file(const char *pth, const char *fnm);

void skipline(void);

void compile_warning(int en, ...);
void compile_warning_at(const char * file, unsigned line, int en, ...);
void compile_warning_pfx(const prefix * pfx, int en, ...);
void compile_error(int en, ...);
void compile_error_skip(int en, ...);
void compile_error_at(const char * file, unsigned line, int en, ...);
void compile_error_pfx(const prefix * pfx, int en, ...);

void compile_error_token(int en, ...);
void compile_error_token_show(int en);
void compile_error_buffer(int en, ...);
void compile_error_buffer_skip(int en, ...);
void compile_warning_buffer(int en, ...);
