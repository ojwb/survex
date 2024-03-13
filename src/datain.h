/* datain.h
 * Header file for code that...
 * Reads in survey files, dealing with special characters, keywords & data
 * Copyright (C) 1994-2022 Olly Betts
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

#include "message.h" /* for DIAG_WARN, etc */

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

/* Read the current line into a string.
 *
 * The string is allocated with malloc() the caller is responsible for calling
 * free().
 */
char* grab_line(void);

/* The severity values are defined in message.h. */
#define DIAG_SEVERITY_MASK 0x03
// Report column number based of the current file position.
#define DIAG_COL	0x04
// Call skipline() after reporting the diagnostic:
#define DIAG_SKIP	0x08
// Set caret_width to s_len(&token):
#define DIAG_BUF	0x10
// The following codes say to parse and discard a value from the current file
// position - caret_width is set to its length:
#define DIAG_WORD	0x20	// Span of non-blanks.
#define DIAG_UINT	0x40	// Span of digits.
#define DIAG_DATE	0x80	// Span of digits and full stops.
#define DIAG_NUM	0x100	// Real number.
#define DIAG_STRING	0x200	// Possibly quoted string value.

void compile_diagnostic(int flags, int en, ...);

void compile_diagnostic_at(int flags, const char * file, unsigned line, int en, ...);
void compile_diagnostic_pfx(int flags, const prefix * pfx, int en, ...);

void compile_diagnostic_token_show(int flags, int en);
void compile_diagnostic_buffer(int flags, int en, ...);
