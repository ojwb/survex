/* prcore.h
 * Header file for printer independent parts of Survex printer drivers
 * Copyright (C) 1994-2002 Olly Betts
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

#include <stdio.h>

extern bool fNoBorder;
extern bool fBlankPage;
extern double PaperWidth, PaperDepth;

typedef struct {
   long x_min, y_min, x_max, y_max;
} border;

#define PR_FONT_DEFAULT 0
#define PR_FONT_LABELS 1

#define PR_FLAG_NOFILEOUTPUT 1
#define PR_FLAG_NOINI 2
#define PR_FLAG_CALIBRATE 4

typedef struct {
   int flags;
   const char * (*Name)(void); /* returns "Widgetware printer" or whatever */
   /* A NULL fn ptr is Ok for Init, Charset, Pre, NewPage, ShowPage,
    & Post, Quit */
   /* if Pre==NULL, it "returns" 1 */
   char * (*Init)(FILE **fh_list, const char *pth, const char *out_fnm,
		double *pscX, double *pscY, bool fCalibrate);
   int  (*Charset)(void);
   int  (*Pre)(int pagesToPrint, const char *title);
   void (*NewPage)(int pg, int pass, int pagesX, int pagesY);
   void (*MoveTo)(long x, long y);
   void (*DrawTo)(long x, long y);
   void (*DrawCross)(long x, long y);
   void (*SetFont)(int font); /* takes PR_FONT_xxx values */
   void (*WriteString)(const char *s);
   void (*DrawCircle)(long x, long y, long r);
   void (*ShowPage)(const char *szPageDetails);
   void (*Post)(void);
   void (*Quit)(void);
} device;

extern device printer;

void drawticks(border clip, int tick_size, int x, int y);

int as_int(const char *v, char *p, int min_val, int max_val);
int as_bool(const char *v, char *p);
double as_double(const char *v, char *p, double min_val, double max_val);
int as_escstring(const char *v, char *s);
char *as_string(const char *v, char *p);
