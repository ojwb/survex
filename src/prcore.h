/* > prindept.h
 * Header file for
 * Printer independent parts of Survex printer drivers
 * Copyright (C) 1994-1995 Olly Betts
 */

#include <stdio.h>

extern bool fNoBorder;
extern bool fBlankPage;
extern float PaperWidth, PaperDepth;

typedef struct {
   long x_min, y_min, x_max, y_max;
} border;

typedef struct {
   const char * (*Name)(void); /* returns "Widgetware printer" or whatever */
   /* A NULL fn ptr is Ok for Init, Alloc, NewPage, ShowPage, Free, Quit */
   /* if Alloc==NULL, it "returns" 1 */
   void (*Init)(FILE *fh, const char *pth, float *pscX, float *pscY);
   int  (*Pre)(int pagesToPrint, const char *title);
   void (*NewPage)(int pg, int pass, int pagesX, int pagesY);
   void (*MoveTo)(long x, long y);
   void (*DrawTo)(long x, long y);
   void (*DrawCross)(long x, long y);
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
