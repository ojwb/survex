/* > prbitmap.c */
/* Bitmap routines for Survex Dot-matrix and Inkjet printer drivers */
/* [Now only used by printmd0] */
/* Copyright (C) 1993,1994 Olly Betts */

/*
1993.05.04 Added dppX and dppY for font scaling, and (C) symbol
1993.05.06 char ch -> int ch in readnum() so if (ch==EOF) ... works!
1993.06.07 renamed from DM_PCL.c to PrBitmap.c
           length of cross arms now determined by dppX and dppY
1993.06.10 minor fettle
1993.06.16 syserr() -> fatal()
1993.08.13 fettled header
1993.11.03 changed error routines
1993.11.05 changed error numbers so we can use same error list as survex
1993.11.06 turned 'Bad font file format' into an error message
1993.11.18 fopen -> safe_fopen
1993.11.29 error 10->24
1993.11.30 sorted out error vs fatal
1994.03.17 coords now long rather than int to increase coord range for DOS
1994.04.16 fixed 3 warnings from Norcroft
1994.06.20 removed readnum(); created prbitmap.h
           added int argument to warning, error and fatal
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "useful.h"
#include "filename.h"
#include "message.h"
#include "prbitmap.h"

#define DPP(dpi) ( (int) ceil( (dpi)/110.0 ) )
 /* rule of thumb for device_dots/char_pixel given dpi */

#define MAX_DEF_CHAR 129 /* value for highest defined character */

#define CHAR_SPACING 8 /* no. of char_pixels to each char */

       void (*PlotDot)( long X, long Y );

static long xLast,yLast;

static int dppX,dppY; /* dots (device pixels) per pixel (char defn pixels) */

/* Uses Bresenham Line generator algorithm */
extern void DrawLineDots( long x, long y, long x2, long y2 )
 {
 long d,dx,dy;
 int xs,ys;
 if ((dx=x2-x)>0)
  xs=1;
 else
  { dx=-dx; xs=-1; }
 if ((dy=y2-y)>0)
  ys=1;
 else
  { dy=-dy; ys=-1; }
 (PlotDot)(x,y);
 if (dx>dy)
  {
  d=(dy<<1)-dx;
  while (x!=x2)
   {
   x+=xs;
   if (d>=0)
    { y+=ys; d+=(dy-dx)<<1; }
   else
    d+=dy<<1;
   (PlotDot)(x,y);
   }
  return;
  }
 else
  {
  d=(dx<<1)-dy;
  while (y!=y2)
   {
   y+=ys;
   if (d>=0)
    { x+=xs; d+=(dx-dy)<<1; }
   else
    d+=dx<<1;
   (PlotDot)(x,y);
   }
  }
 }

extern void MoveTo( long x, long y )
 {
 xLast=x; yLast=y;
 }

extern void DrawTo( long x, long y )
 {
 DrawLineDots( xLast, yLast, x, y);
 xLast=x; yLast=y;
 }

extern void DrawCross( long x, long y )
 {
 DrawLineDots(x-dppX, y-dppY, x+dppX, y+dppY);
 DrawLineDots(x+dppX, y-dppY, x-dppX, y+dppY);
 xLast=x; yLast=y;
 }

/* Font Driver Routines */

static char font[MAX_DEF_CHAR+1][8];

extern void ReadFont( sz fnm, int dpiX, int dpiY )
 {
 FILE *fh;
 int ch,y,x,b;

 dppX=DPP(dpiX); /* dots (printer pixels) per pixel (char defn pixels) */
 dppY=DPP(dpiY);

/* printf("Debug info: dpp x=%d, y=%d\n\n",dppX,dppY); */

 if ((fh=safe_fopen(fnm,"rb"))==0)
  fatal(24,wr,fnm,0);
 for( ch=' ' ; ch<=MAX_DEF_CHAR ; ch++ )
  {
  for( y=0 ; y<8 ; y++ )
   {
   do { b=fgetc(fh); } while (b!='.' && b!='#' && b!=EOF);
   font[ch][y]=0;
   for( x=1 ; x<256 ; x=x<<1 )
    {
    if (b=='#')
     font[ch][y]|=x;
    b=fgetc(fh);
    }
   if (b!='\n' && b!='\r')
    fatal(88,NULL,NULL,0);
   }
  }
 fclose(fh);
 }

static void WriteLetter( int ch, long X, long Y )
 {
 int x,y,x2,y2,t;
 for( y=0 ; y<8 ; y++ )
  {
  t=font[ch][7-y];
  for( x=0 ; x<8 ; x++ )
   {
   if (t & 1)
    { /* plot mega-pixel */
    for( x2=0 ; x2 < dppX; x2++)
     for( y2=0 ; y2 < dppY; y2++)
      PlotDot( X + (long)x*dppX + x2, Y + (long)y*dppY + y2 );
    }
   t=t>>1;
   }
  }
 }

extern void WriteString(sz sz)
 {
 unsigned char ch=*sz;
 while (ch>=32)
  {
  if (ch<=MAX_DEF_CHAR)
   WriteLetter(ch,xLast,yLast);
  ch=*(++sz);
  xLast+=(long)CHAR_SPACING*dppX;
  }
 }
