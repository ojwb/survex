/* > prcore.h
 * Header file for printer independent parts of Survex printer drivers
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

#include <stdio.h>

extern bool fNoBorder;
extern bool fBlankPage;
extern float PaperWidth, PaperDepth;

typedef struct {
   long x_min, y_min, x_max, y_max;
} border;

#define PR_FONT_DEFAULT 0
#define PR_FONT_LABELS 1

typedef struct {
   const char * (*Name)(void); /* returns "Widgetware printer" or whatever */
   /* A NULL fn ptr is Ok for Init, Alloc, NewPage, ShowPage, Free, Quit */
   /* if Alloc==NULL, it "returns" 1 */
   void (*Init)(FILE **fh_list, const char *pth, float *pscX, float *pscY);
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

extern void drawticks(border clip, int tick_size, int x, int y);

extern int as_int(char *p, int min_val, int max_val);
extern int as_bool(char *p);
extern float as_float(char *p, float min_val, float max_val);
int as_escstring(char *s);
char *as_string(char *p);

char **prcore_read_ini(char *section, char **vars);
