/* > printmd0.c
 * Device dependent part of Survex Archie-Mode-0 Driver (for testing)
 * Copyright (C) 1993,1994 Olly Betts
 */

/*
1993.05.03 Added fNoBorder
1993.05.04 added scalable fonts
           char * -> sz
1993.06.10 name changed
1993.06.16 free() -> osfree()
1993.08.13 fettled header
1993.08.16 "PFont" -> FNMFONT
1993.11.15 created print_malloc & print_free to claim & release all memory
1993.11.17 added pass to argument list for NewPage()
1993.12.06 updated to work with -!P
1994.03.17 coords now long rather than int to increase coord range for DOS
1994.06.20 created prindept.h and suppressed -fh warnings
1994.06.22 removed prototypes now in header files
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "useful.h"
#include "error.h"
#include "prindept.h"
#include "prbitmap.h"

sz szDriverDesc="Mode 0 Screen";

#define FNMFONT "pfont"

#include "bbc.h"

#define OSUNITS_PER_INCH 180 /* RISCOS std */

extern void ReadFont( sz fnm, int dpiX, int dpiY );

typedef struct
 {
 long x_min, y_min, x_max, y_max;
 } border;

static border clip;

static int xpPageWidth,ypPageDepth;

static void PlotDotTest( long x, long y )
 {
 if (!(x<clip.x_min || x>clip.x_max || y<clip.y_min || y>clip.y_max))
  fBlankPage=fFalse;
 }

static void PlotDotMd0( long x, long y )
 {
 if (x<clip.x_min || x>clip.x_max
   ||y<clip.y_min || y>clip.y_max)
  return;
 bbc_plot(69,(int)((x-clip.x_min)*2),(int)((y-clip.y_min)*4));
 }

extern void NewPage( int pg, int pass, int pagesX, int pagesY )
 {
 int x,y;

 x=(pg-1)%pagesX;
 y=pagesY-1-((pg-1)/pagesX);
 clip.x_min=(long)x*xpPageWidth;
 clip.y_min=(long)y*ypPageDepth;
 clip.x_max=clip.x_min+xpPageWidth-1;
 clip.y_max=clip.y_min+ypPageDepth-1;

 if (pass==-1)
  {
  /* Don't count alignment marks, but do count borders */
  fBlankPage=fNoBorder || (x>0 && y>0 && x<pagesX-1 & y<pagesY-1);
  PlotDot=PlotDotTest; /* setup function ptr */
  }
 else
  {
  bbc_mode(0);
  bbc_origin(0,64);

  PlotDot=PlotDotMd0;
  /* Draws in alignment marks on each page or borders on edge pages */
  MoveTo(clip.x_min,   clip.y_min);
  if (fNoBorder || x)
   { DrawTo(clip.x_min,   clip.y_min+9); MoveTo(clip.x_min,   clip.y_max-9);}
  DrawTo(clip.x_min,   clip.y_max);
  if (fNoBorder || y<pagesY-1)
   { DrawTo(clip.x_min+9, clip.y_max);   MoveTo(clip.x_max-9, clip.y_max);  }
  DrawTo(clip.x_max,   clip.y_max);
  if (fNoBorder || x<pagesX-1)
   { DrawTo(clip.x_max,   clip.y_max-9); MoveTo(clip.x_max,   clip.y_min+9);}
  DrawTo(clip.x_max,   clip.y_min);
  if (fNoBorder || y)
   { DrawTo(clip.x_max-9, clip.y_min);   MoveTo(clip.x_min+9, clip.y_min);  }
  DrawTo(clip.x_min,   clip.y_min);
  }
 }

extern void ShowPage( sz szPageDetails )
 {
 bbc_tab(0,31);
 printf( szPageDetails );
 bbc_get();
 }

/* Initialise 'printer' routines */
extern void print_init( sz pth, float *pscX, float *pscY )
 {
 sz fnm;

 PaperWidth=(float)(1280/(double)OSUNITS_PER_INCH*MM_PER_INCH);
 PaperDepth=(float)((1024-64)/(double)OSUNITS_PER_INCH*MM_PER_INCH);

 *pscX=(float)(OSUNITS_PER_INCH/MM_PER_INCH/2);
 *pscY=(float)(OSUNITS_PER_INCH/MM_PER_INCH/4);
 xpPageWidth=(int)(*pscX*PaperWidth);
 ypPageDepth=(int)(*pscY*PaperDepth);

 /* Note: (*pscX) & (*pscY) are in device_dots / mm */
 fnm=UsePth(pth,FNMFONT);
 ReadFont( fnm, (int)((*pscX)*MM_PER_INCH), (int)((*pscY)*MM_PER_INCH) );
 osfree(fnm);
 }

extern int print_malloc( void )
 {
 return 1; /* only need one pass */
 }

extern void print_free( void )
 {
 return;
 }

extern void print_quit( void )
 {
 return;
 }
