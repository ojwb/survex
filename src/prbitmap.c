/* > prbitmap.c */
/* Bitmap routines for Survex Dot-matrix and Inkjet printer drivers */
/* [Now only used by printmd0] */
/* Copyright (C) 1993,1994 Olly Betts */

/* FIXME this has fallen behind development - if it's ever wanted again,
 * extract the routines from printdm.c

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

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

extern void read_font( sz pth, sz leaf, int dpiX, int dpiY )
 {
 char *fnm;
 FILE *fh;
 int ch,y,x,b;

 dppX=DPP(dpiX); /* dots (printer pixels) per pixel (char defn pixels) */
 dppY=DPP(dpiY);

/* printf("Debug info: dpp x=%d, y=%d\n\n",dppX,dppY); */

 fnm = UsePth(pth,leaf);
 fh = safe_fopen(fnm, "rb");
 osfree(fnm);
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
   if (b != '\n' && b != '\r') fatalerror(NULL, 0, 88, fnm);
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
