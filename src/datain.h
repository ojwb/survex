/* datain.h
 * Header file for code that...
 * Reads in survey files, dealing with special characters, keywords & data
 * Copyright (C) 1994-2024 Olly Betts
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

#ifndef DATAIN_H
#define DATAIN_H

#include <setjmp.h>
#include <stdio.h> /* for FILE */

#include "message.h" /* for DIAG_WARN, etc */

// We rely on implicit initialisation of this struct, so members will be
// initialised to NULL, 0, false, etc.
typedef struct parse {
   FILE *fh;
   const char *filename;
   // ftell() at start of the current line.
   long lpos;
   unsigned int line;
   bool reported_where : 1;
   unsigned prev_line_len : 31;
   struct parse *parent;
} parse;

extern int ch;
extern parse file;
extern jmp_buf jbSkipLine;
extern bool f_export_ok;

#define nextch() (ch = GETC(file.fh))

typedef struct {
   long offset;
   int ch;
} filepos;

void get_pos(filepos *fp);
void set_pos(const filepos *fp);

void set_declination_location(real x, real y, real z, const char *proj_str,
			      filepos *fp);

void skipblanks(void);

/* reads complete data file */
void data_file(const char *pth, const char *fnm);

real calculate_convergence_xy(const char *proj_str,
			      double x, double y, double z);

void skipline(void);

/* Read the current line into a string, converting each tab to a space.
 *
 * The string is allocated with malloc() the caller is responsible for calling
 * free().
 */
char* grab_line(void);

/* The severity values are defined in message.h. */
#define DIAG_SEVERITY_MASK	0x03

// Call skipline() after reporting the diagnostic:
#define DIAG_SKIP		0x04

// Context type values:

#define DIAG_CONTEXT_MASK	0x78
// Report column number based on the current file position.
#define DIAG_COL		0x08
// Set caret_width to s_len(&token):
#define DIAG_TOKEN		0x10
// The following codes say to parse and discard a value from the current file
// position - caret_width is set to its length:
#define DIAG_WORD		0x18	// Span of non-blanks and non-comments.
#define DIAG_UINT		0x20	// Span of digits.
#define DIAG_DATE		0x28	// Span of digits and full stops.
#define DIAG_NUM		0x30	// Real number.
#define DIAG_STRING		0x38	// Possibly quoted string value.
#define DIAG_TAIL		0x40	// Rest of the line (not including
					// trailing blanks or comment).

// A non-zero caret_width value can be encoded in the upper bits.
#define DIAG_FROM_SHIFT 7
// Mask to detect embedded caret_width value.
#define DIAG_FROM_MASK	(~((1U << DIAG_FROM_SHIFT) - 1))
// Specify the caret_width explicitly.
#define DIAG_WIDTH(W)	((W) << DIAG_FROM_SHIFT)
// Specify caret_width to be from filepos POS to the current position.
#define DIAG_FROM(POS)	DIAG_WIDTH(ftell(file.fh) - (POS).offset)

void compile_diagnostic(int flags, int en, ...);

void compile_diagnostic_at(int flags, const char * file, unsigned line, int en, ...);
void compile_diagnostic_pfx(int flags, const prefix * pfx, int en, ...);

void compile_diagnostic_token_show(int flags, int en);
void compile_diagnostic_buffer(int flags, int en, ...);

#endif
