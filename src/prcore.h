/* > prindept.h
 * Header file for
 * Printer independent parts of Survex printer drivers
 * Copyright (C) 1994-1995 Olly Betts
 */
/*
1994.06.20 created
1995.04.17 rearranged into device struct
1995.06.04 added a comment about which fn ptrs can be NULL in device
1995.06.05 szDesc -> Name function ; added prototypes
1995.06.23 removed readnum
1995.06.25 added device.Pre and device.Post
1995.10.07 added prototype for drawticks
           moved prototypes for ps_ and hpgl_ fns to printps.c
           Alloc and Free removed - Pre now returns int
1995.12.20 added as_xxx functions to simplify reading of .ini files
1996.03.10 added prototypes for as_string and as_escstring
1998.03.21 fixed up to compile cleanly on Linux
*/

#include <stdio.h>

extern bool fNoBorder;
extern bool fBlankPage;
extern float PaperWidth, PaperDepth;

typedef struct {
   long x_min, y_min, x_max, y_max;
} border;

typedef struct {
   char * (*Name)( void ); /* returns "Widgetware printer" or whatever */
   /* A NULL fn ptr is Ok for Init, Alloc, NewPage, ShowPage, Free, Quit */
   /* if Alloc==NULL, it "returns" 1 */
   void (*Init)( FILE *fh, const char *pth, float *pscX, float *pscY );
   int  (*Pre)( int pagesToPrint );
   void (*NewPage)( int pg, int pass, int pagesX, int pagesY );
   void (*MoveTo)( long x, long y );
   void (*DrawTo)( long x, long y );
   void (*DrawCross)( long x, long y );
   void (*WriteString)( const char *s );
   void (*ShowPage)( const char *szPageDetails );
   void (*Post)( void );
   void (*Quit)( void );
} device;

extern device printer;

extern void drawticks( border clip, int tick_size, int x, int y );

extern int as_int( char *p, int min_val, int max_val );
extern int as_bool( char *p );
extern float as_float( char *p, float min_val, float max_val );
int as_escstring( char *s );
char *as_string( char *p );
