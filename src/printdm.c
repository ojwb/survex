/* > printdm.c & printpcl.c */

/* Device dependent part of Survex Dot-matrix/PCL printer driver */
/* Bitmap routines for Survex Dot-matrix and Inkjet printer drivers */
/* Copyright (C) 1993-1997 Olly Betts */

/*
1993.04.30 Tidied & checked PCLDrivr.c against DMDriver.c
1993.05.03 Cured blank line bug [PCL]
           Added fNoBorder option
1993.05.04 added scalable fonts
1993.05.03 Added fNoBorder option
1993.05.08 added horiztl offset code to ShowPage to compress data lots [PCL]
           added vertical offset code too [PCL]
1993.05.19 (W) added comment about stdprn being BCC
1993.06.04 added () to #if
           FALSE -> fFalse
1993.06.10 name changed
           added note about DJGPP having stdprn too
1993.06.16 malloc() -> osmalloc(); syserr() -> fatal(); free() -> osfree()
1993.06.29 PCLCfg -> pclcfg, DMCfg -> dmcfg for Unix, etc
1993.07.02 added MF's unix pipe code
1993.07.04 "Pfont" -> "pfont", and moved into #define FNMFONT
1993.07.12 split off printer i/o code into prio.c
1993.07.18 select narrow font for footer, so it won't overflow the page [PCL]
1993.08.13 fettled header
1993.11.03 changed error routines
1993.11.05 changed error numbers so we can use same error list as survex
1993.11.15 created print_malloc & print_free to claim & release all memory
1993.11.16 merged printpcl.c & printdm.c
           first attempt at several passes per page code
           page[][] -> bitmap[][]
1993.11.17 finally got it working (tested for PCL)
           PCL empty lines are concatenated across passes
1993.11.18 fopen -> safe_fopen
1993.11.28 moved "dmcfg", "pclcfg" to DM_CFG, PCL_CFG now defd in filelist.h
1993.11.29 error 10->24
1993.12.04 added some commented-out code to skip blank pages
1993.12.05 size_t -> OSSIZE_T
           merged in prbitmap.c - change list:
           added fPCL
>>1993.05.04 Added dppX and dppY for font scaling, and (C) symbol
>>1993.05.06 char ch -> int ch in readnum() so if (ch==EOF) ... works!
>>1993.06.07 renamed from DM_PCL.c to PrBitmap.c
>>           length of cross arms now determined by dppX and dppY
>>1993.06.10 minor fettle
>>1993.06.16 syserr() -> fatal()
>>1993.08.13 fettled header
>>1993.11.03 changed error routines
>>1993.11.05 changed error numbers so we can use same error list as survex
>>1993.11.06 turned 'Bad font file format' into an error message
>>1993.11.18 fopen -> safe_fopen
>>1993.11.29 error 10->24
>>1993.11.30 sorted out error vs fatal
1993.12.06 alignment marks/border were being drawn with PlotDotTest
1993.12.13 corrected use of & to &&
1994.03.17 coords now long rather than int to increase coord range for DOS
           SizeOfGraph_t -> SIZEOFGRAPH_T as POSIX reserve *_t
1994.03.19 fettling error stuff
1994.04.10 introduced BUG() macro
1994.06.20 created prindept.h and suppressed -fh warnings
           added int argument to warning, error and fatal
1994.06.22 removed prototypes now in header files
1994.07.14 fixed mis-use of ypPageDepth (ylPageDepth meant) - printdm bug
1994.08.15 added prn.eol
1994.08.22 added prn.grph2
1994.09.08 added temporary code to allow prevention of use of PCL horiztl tab
           bitmap is now explicitly unsigned char**
1994.09.13 if (E) BUG(M); -> ASSERT(!E,M)
1995.04.15 ASSERT -> one arg
1995.04.16 changed to use ini code and reindented
1995.04.17 now uses memset to zero each row of bitmap
1995.10.06 continued rearrangements to .ini files
1995.10.07 ditto
1995.12.18 added fNoPCLVTab code (rewrote 'cos the original's at work)
           added code_graphics_mode_suffix and code_end_of_line
1995.12.20 added as_xxx functions to simplify reading of .ini files
1996.02.10 switched to use as_escstring
1996.02.13 pstr is typedef-ed in prio.h not here
1996.02.14 corrected MAX_DEF_CHAR to 255
1996.03.09 now free vals[1] after use
1996.03.10 added as_string() ; double osfree()-ing of vals fixed
1996.03.21 round down clip box for bitmap devices
1996.03.26 fixed spurious page borders between passes
1996.04.11 fixed printpcl (and probably printdm) bug introduced by above fix
1996.09.21 printdm: added "is_ibm" .ini file option
1996.10.01 is_ibm now defaults to fFalse if not specified
1997.01.16 now look for [dm] or [pcl] in .ini file
1998.03.21 fixed up to compile cleanly on Linux
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <float.h>
#include <limits.h>

#include "useful.h"
#include "error.h"
#include "prio.h"
#include "filelist.h"
#include "debug.h" /* for BUG and ASSERT */
#include "prindept.h"
#include "ini.h"

static void MoveTo( long x, long y );
static void DrawTo( long x, long y );
static void DrawCross( long x, long y );
static void WriteString( const char *s );
static int Pre( int pagesToPrint );
static void Post( void );

#ifdef PCL
static bool fPCL=fTrue;
static bool fNoPCLHTab=fFalse;
static bool fNoPCLVTab=fFalse;

static char *pcl_Name( void );
static void pcl_NewPage( int pg, int pass, int pagesX, int pagesY );
static void pcl_Init( FILE *fh, const char *pth, float *pscX, float *pscY );
static void pcl_ShowPage( const char *szPageDetails );

/*device pcl={*/
device printer={
   pcl_Name,
   pcl_Init,
   Pre,
   pcl_NewPage,
   MoveTo,
   DrawTo,
   DrawCross,
   WriteString,
   pcl_ShowPage,
   Post,
   prio_close
};
#else
static bool fPCL=fFalse;
static bool fIBM=fFalse;

static char *dm_Name( void );
static void dm_NewPage( int pg, int pass, int pagesX, int pagesY );
static void dm_Init( FILE *fh, const char *pth, float *pscX, float *pscY );
static void dm_ShowPage( const char *szPageDetails );

/*device dm={*/
device printer={
   dm_Name,
   dm_Init,
   Pre,
   dm_NewPage,
   MoveTo,
   DrawTo,
   DrawCross,
   WriteString,
   dm_ShowPage,
   Post,
   prio_close
};
#endif

#define FNMFONT "pfont"

#define DPP(dpi) ( (int) ceil( (dpi)/110.0 ) )
 /* rule of thumb for device_dots/char_pixel given dpi */

#define MAX_DEF_CHAR 255 /* value for highest defined character */

#define CHAR_SPACING 8 /* no. of char_pixels to each char */

static long xLast,yLast;

static int dppX,dppY; /* dots (device pixels) per pixel (char defn pixels) */

static void ReadFont( sz fnm, int dpiX, int dpiY );

static OSSIZE_T lenLine; /* number of chars in each line */

static border clip;

static int ylPageDepth,ylPassDepth,ylThisPassDepth,ypLineDepth;
static int passMax,passStore;

static int SIZEOFGRAPH_T; /* for DM */

static int dpi; /* for PCL */

#ifndef PCL
/* Max length of printer sequence */
/*#define CMAX 40*/

typedef struct {
   pstr lnsp, grph, fnt_big;
   pstr fnt_sml, fmfd, rst;
   pstr eol, grph2;
/*
   char lnsp[CMAX],grph[CMAX],fnt_big[CMAX];
   char fnt_sml[CMAX],fmfd[CMAX],rst[CMAX];
   char eol[CMAX],grph2[CMAX];
*/
} dmprinter;

static dmprinter prn;
#endif

static unsigned char **bitmap;

static int xpPageWidth,ypPageDepth;

static void (*PlotDot)( long, long );

/* no longer used */
#if 0
static bool ReadPrinterCode( char *sz, sz prnstr ) {
   char ch;
   char n;
   int c=0;

   do {
      ch=*sz++;
   } while (isspace(ch));
   while ( !isspace(ch) && c<CMAX ) {
      if (ch=='$') {
         ch=*sz++;
         if (isxdigit(ch)) {
            n = ( ch<'A' ? (char)ch-'0' : ((char)ch|0x20)-'a'+10 );
            ch=*sz++;
            if (isxdigit(ch))
               prnstr[c++] = (n<<4) + ( ch<'A' ? ch-'0' : (ch|0x20)-'a'+10 );
         }
         if (!isxdigit(ch) && ch!=EOF) {
            printf("Bad hex digit %c\n",ch);
            return fFalse;
         }
      } else
         prnstr[c++]= ch;
      ch=*sz++;
   }
   prnstr[c]='\0';
   while ((ch=*sz++)!='\0')
      ;
   return (isspace(ch));
}
#endif

static void PlotDotTest( long x, long y ) {
   if (!(x<clip.x_min || x>clip.x_max || y<clip.y_min || y>clip.y_max))
      fBlankPage=fFalse;
}

static void PlotDotDM( long x, long y ) {
   int v;
   if (x<clip.x_min || x>clip.x_max || y<clip.y_min || y>clip.y_max)
      return;
   x-=clip.x_min;
   y-=clip.y_min;
   /* Shift up to fit snugly at high end */
   v=(int)(y%ypLineDepth)+((-ypLineDepth)&7);
   bitmap[y/ypLineDepth][(int)x*SIZEOFGRAPH_T+(SIZEOFGRAPH_T-(v>>3)-1)]
      |= 1<<(v&7);
}

static void PlotDotPCL( long x, long y ) {
   if (x<clip.x_min || x>clip.x_max || y<clip.y_min || y>clip.y_max)
      return;
/*   printf("%ld %ld ",x,y); */
   x-=clip.x_min;
   bitmap[y-clip.y_min][x>>3]|=128>>(x&7);
/*   printf("%ld %ld\n",x>>3,y-clip.y_min); */
/*   if (x==0 && y==0) printf("@@@ %ld\n",clip.y_max-clip.y_min);*/

/*   if ((x>>3)<0 || (x>>3)>=lenLine) printf("+++ x out of range [0-%ld)\n",(long)lenLine);*/
/*   if ((y-clip.y_min)<0 || (y-clip.y_min)>=ylPassDepth) printf("+++ y out of range [0-%ld)\n",(long)ylPassDepth);*/
}

#ifdef PCL
static char *pcl_Name( void ) {
   return "HP PCL Printer";
}
#else
static char *dm_Name( void ) {
   return "Dot-Matrix Printer";
}
#endif

/* pass = -1 => check if this page is blank
 * otherwise pass goes from 0 to (passMax-1)
 */
#ifdef PCL
static void pcl_NewPage( int pg, int pass, int pagesX, int pagesY ) {
#else
static void dm_NewPage( int pg, int pass, int pagesX, int pagesY ) {
#endif
   int x,y,yi;
   border edge;

   x=(pg-1)%pagesX;
   y=pagesY-1-((pg-1)/pagesX);

   edge.x_min=(long)x*xpPageWidth;
   edge.y_min=(long)y*ypPageDepth;
   /* need to round down clip rectangle for bitmap devices */
   edge.x_max=edge.x_min+xpPageWidth-1;
   edge.y_max=edge.y_min+ypPageDepth-1;

#if 0
   clip.x_min=edge.x_min;
   clip.x_max=edge.x_max;
   clip.y_min=edge.y_min;
   clip.y_max=edge.y_max;
#endif
   clip=edge;
   passStore=pass;

/*   printf("\\\\\\ page %d pass %d clip.y_min %ld clip.y_max %ld\n",pg,pass,clip.y_min,clip.y_max); */

   if (pass==-1) {
      /* Don't count alignment marks, but do count borders */
      fBlankPage=fNoBorder || (x>0 && y>0 && x<pagesX-1 && y<pagesY-1);
      PlotDot=PlotDotTest; /* setup function ptr */
   } else {
      clip.y_max -= pass*((long)ylPassDepth*ypLineDepth);
/*      printf("%ld\n",(long)(pass*((long)ylPassDepth*ypLineDepth))); */
      if (pass==passMax-1)
         ylThisPassDepth=(ylPageDepth-1)%ylPassDepth+1; /* remaining lines */
      else {
         long y_min_new=clip.y_max-(long)ylPassDepth*ypLineDepth+1;
         ASSERT(y_min_new>=clip.y_min);
         clip.y_min=y_min_new;
         ylThisPassDepth=ylPassDepth;
      }

/*      printf("\\\\\\ clip.y_min %ld clip.y_max %ld\n",clip.y_min,clip.y_max); */
/*      printf("/// clip height = %ld, pass depth = %ld\n",clip.y_max-clip.y_min,(long)ylPassDepth); */

      /* Don't 0 all rows on last pass */
      for( yi=0; yi<ylThisPassDepth; yi++ )
         memset( bitmap[yi], 0, lenLine );

      PlotDot=( fPCL ? PlotDotPCL : PlotDotDM ); /* setup function ptr */

      drawticks(edge,9,x,y);
   }
}

#ifdef PCL
static void pcl_ShowPage( const char *szPageDetails ) {
#else
static void dm_ShowPage( const char *szPageDetails ) {
#endif
   int x, y, last;
#ifdef PCL
# define firstMin 7 /* length of horizontal offset ie  Esc * p <number> X */
   int first;
   static cEmpties=0; /* static so we can store them up between passes */

   if (passStore==0)
      prio_printf( "\x1b*t%dR", dpi );

   for( y=ylThisPassDepth-1 ; y>=0 ; y-- ) {
      for( last=((xpPageWidth+7)>>3)-1 ; last>=0 && !bitmap[y][last] ; )
         last--; /* Scan in from right end to last used byte, then stop */
      first=0;
      if (last>firstMin) /* NB this ensures at least one nonzero byte */
         for( ; !bitmap[y][first] ; )
           first++; /* Scan in from left end to first used byte, then stop */
      /* can't do horiz tabs, or won't save any bytes (indent < firstMin) */
      if (fNoPCLHTab || first<firstMin)
         first=0;
      else
         prio_printf( "\x1b*b%dX", first*8 );
      /* need blank lines (last==-1) for PCL dump ... (sigh) */
      if (!fNoPCLVTab && last==-1)
         cEmpties++; /* line is completely blank */
      else {
         if (cEmpties) {
            /* if we've just had some blank lines, do a vertical offset */
            prio_printf( "\x1b*b%dY", cEmpties );
            cEmpties=0;
         }
         /* don't trust the printer to do a 0 byte line */
         prio_printf( "\x1b*b%dW", max(1,last+1-first) );
         for( x=first ; x<=last ; x++ )
            prio_putc( bitmap[y][x] );
      }
   }
   if (cEmpties && passStore==passMax-1) {
      /* finished, so vertical offset any remaining blank lines */
      prio_printf( "\x1b*b%dY", cEmpties );
      cEmpties=0;
   }
#else
   if (passStore==0) {
      prio_putpstr( &prn.rst );
      prio_putpstr( &prn.lnsp );
   }
   for( y=ylThisPassDepth-1 ; y>=0 ; y-- ) {
      for( last=xpPageWidth*SIZEOFGRAPH_T-1 ; last>=0 ; last-- )
         if (bitmap[y][last])
            break;
      /* Scan in from right hand end to first used 'byte', then stop */
      if (last>=0) {
         int len;
         last = last/SIZEOFGRAPH_T+1;
         len = last*SIZEOFGRAPH_T;
         if (fIBM) {
            /* IBM Proprinter style codes */
            last = len+1;
         }
         prio_putpstr( &prn.grph );
         prio_putc( last&255 );
         prio_putc( last>>8 );
         prio_putpstr( &prn.grph2 );
         for( x=0 ; x<len ; x++ )
            prio_putc( bitmap[y][x] );
      }
      prio_putpstr( &prn.eol );
   }
#endif
   putchar(' ');
   if (passStore==passMax-1) {
#ifdef PCL
      prio_printf( "\x1b*rB\n\n" ); /* End graphics */
      prio_printf( "\x1b(s0p20h12v0s3t2Q" ); /* select a narrow-ish font */
      prio_printf( szPageDetails );
      prio_putc( '\x0c' );
#else
      prio_putpstr( &prn.fnt_sml );
      prio_putc( '\n' );
      prio_printf( szPageDetails );
      prio_putpstr( &prn.fmfd );
      prio_putpstr( &prn.rst );
#endif
   }
}

/* Initialise DM/PCL printer routines */
#ifdef PCL
static void pcl_Init( FILE *fh, const char *pth, float *pscX, float *pscY ) {
#else
static void dm_Init( FILE *fh, const char *pth, float *pscX, float *pscY ) {
#endif
   char *fnm;
   char *fnmPrn;
   static char *vars[]={
      "like",
      "output",
#ifdef PCL
      "dpi",
      "mm_across_page",
      "mm_down_page",
      "horizontal_tab_ok",
      "vertical_tab_ok",
#else
      "pixels_across_page",
      "lines_down_page",
      "dots_per_pass",
      "mm_across_page",
      "mm_down_page",
      "code_line_spacing",
      "code_graphics_mode",
      "code_graphics_mode_suffix",
      "code_large_font",
      "code_small_font",
      "code_formfeed",
      "code_reset_printer",
      "code_end_of_line",
      "is_ibm",
#endif
      NULL
   };
   char **vals;

#ifdef PCL
   vals=ini_read_hier(fh,"pcl",vars);
   fnmPrn=as_string(vals[1]);
   dpi=as_int(vals[2],1,INT_MAX);
   PaperWidth=as_float(vals[3],1,FLT_MAX);
   PaperDepth=as_float(vals[4],11,FLT_MAX);
   PaperDepth-=10; /* allow 10mm for footer */
   fNoPCLHTab=!as_bool(vals[5]);
   fNoPCLVTab=!as_bool(vals[6]);
   osfree(vals);

   *pscX=*pscY=(float)(dpi/MM_PER_INCH);
   xpPageWidth=(int)((dpi/MM_PER_INCH)*PaperWidth);
   ylPageDepth=ypPageDepth=(int)((dpi/MM_PER_INCH)*PaperDepth);
   ypLineDepth=1;
#else
   vals=ini_read_hier(fh,"dm",vars);
/*   vals=ini_read_hier(fh,"bj",vars);*/
   fnmPrn=as_string(vals[1]);
   xpPageWidth=as_int(vals[2],1,INT_MAX);
   ylPageDepth=as_int(vals[3],4,INT_MAX);
   ylPageDepth-=3; /* allow for footer */
   ypLineDepth=as_int(vals[4],1,INT_MAX);
   PaperWidth=as_float(vals[5],1,FLT_MAX);
   PaperDepth=as_float(vals[6],1,FLT_MAX);

   ypPageDepth=ylPageDepth*ypLineDepth;
   *pscX=xpPageWidth/PaperWidth; /* xp per mm */
   *pscY=ypPageDepth/PaperDepth; /* yp per mm */

   /* Work out # bytes required to hold one column */
   SIZEOFGRAPH_T=(ypLineDepth+7)>>3;

   prn.lnsp.str=vals[7];     /* $1b3$18   */
   prn.lnsp.len=as_escstring(prn.lnsp.str);
   prn.grph.str=vals[8];     /* $1bL      */
   prn.grph.len=as_escstring(prn.grph.str);
   prn.grph2.str=vals[9];    /*           */
   prn.grph2.len=as_escstring(prn.grph2.str);
   prn.fnt_big.str=vals[10];  /* $12$1bW1  */
   prn.fnt_big.len=as_escstring(prn.fnt_big.str);
   prn.fnt_sml.str=vals[11];  /* $1bW0$0f  */
   prn.fnt_sml.len=as_escstring(prn.fnt_sml.str);
   prn.fmfd.str=vals[12];     /* $0c       */
   prn.fmfd.len=as_escstring(prn.fmfd.str);
   prn.rst.str=vals[13];      /* $1b2      */
   prn.rst.len=as_escstring(prn.rst.str);
   prn.eol.str=vals[14];      /* $0d       */
   prn.eol.len=as_escstring(prn.eol.str);
   fIBM = vals[15] ? as_bool(vals[15]) : fFalse; /* default to Epsom */
   osfree(vals);
#endif

   fnm=UsePth(pth,FNMFONT);
   if (fPCL)
      ReadFont(fnm,dpi,dpi);
   else { /* Note: (*pscX) & (*pscY) are in device_dots / mm */
      ReadFont(fnm, (int)((*pscX)*MM_PER_INCH), (int)((*pscY)*MM_PER_INCH) );
   }
   osfree(fnm);

   prio_open(fnmPrn);
   osfree(fnmPrn);
}

static int Pre( int pagesToPrint ) {
   OSSIZE_T y;
   pagesToPrint=pagesToPrint; /* shut-up warning */
   passMax=1;
   bitmap=osmalloc(ylPageDepth*ossizeof(void*));
   lenLine=( fPCL ? ((xpPageWidth+7)>>3) : xpPageWidth*SIZEOFGRAPH_T );
   bitmap[0]=osmalloc(lenLine);
   for( y=1 ; y<ylPageDepth ; y++ ) {
      bitmap[y]=xosmalloc(lenLine);
      if (bitmap[y]==NULL) {
         passMax=(int)ceil((double)ylPageDepth/y);
         bitmap=osrealloc(bitmap,y*ossizeof(void*));
         break;
      }
   }
   ylPassDepth=y;
   return passMax;
}

static void Post( void ) {
   OSSIZE_T y;
   for( y=0 ; y<ylPassDepth ; y++ )
      osfree(bitmap[y]);
   osfree(bitmap);
}

/* Uses Bresenham Line generator algorithm */
static void DrawLineDots( long x, long y, long x2, long y2 ) {
   long d,dx,dy;
   int xs,ys;
/*   printf("*** drawlinedots( %ld, %ld, %ld, %ld )\n",x,y,x2,y2); */
   if ((dx=x2-x)>0)
      xs=1;
   else
      { dx=-dx; xs=-1; }
   if ((dy=y2-y)>0)
      ys=1;
   else
      { dy=-dy; ys=-1; }
   (PlotDot)(x,y);
   if (dx>dy) {
      d=(dy<<1)-dx;
      while (x!=x2) {
         x+=xs;
         if (d>=0)
            { y+=ys; d+=(dy-dx)<<1; }
         else
            d+=dy<<1;
         (PlotDot)(x,y);
      }
   } else {
      d=(dx<<1)-dy;
      while (y!=y2) {
         y+=ys;
         if (d>=0)
            { x+=xs; d+=(dx-dy)<<1; }
         else
            d+=dx<<1;
         (PlotDot)(x,y);
      }
   }
}

static void MoveTo( long x, long y ) {
   xLast=x; yLast=y;
}

static void DrawTo( long x, long y ) {
   DrawLineDots( xLast, yLast, x, y);
   xLast=x; yLast=y;
}

static void DrawCross( long x, long y ) {
   DrawLineDots(x-dppX, y-dppY, x+dppX, y+dppY);
   DrawLineDots(x+dppX, y-dppY, x-dppX, y+dppY);
   xLast=x; yLast=y;
}

/* Font Driver Routines */

static char font[MAX_DEF_CHAR+1][8];

static void ReadFont( sz fnm, int dpiX, int dpiY ) {
   FILE *fh;
   int ch,y,x,b;

   dppX=DPP(dpiX); /* dots (printer pixels) per pixel (char defn pixels) */
   dppY=DPP(dpiY);
/* printf("Debug info: dpp x=%d, y=%d\n\n",dppX,dppY); */
   fh=safe_fopen(fnm,"rb");
   if (!fh)
      fatal(24,wr,fnm,0);
   for( ch=' ' ; ch<=MAX_DEF_CHAR ; ch++ ) {
      for( y=0 ; y<8 ; y++ ) {
         do {
            b=fgetc(fh);
         } while (b!='.' && b!='#' && b!=EOF);
         font[ch][y]=0;
         for( x=1 ; x<256 ; x=x<<1 ) {
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

static void WriteLetter( int ch, long X, long Y ) {
   int x,y,x2,y2,t;
/*   printf("*** writeletter( %c, %ld, %ld )\n",ch,X,Y); */
   for( y=0 ; y<8 ; y++ ) {
      t=font[ch][7-y];
      for( x=0 ; x<8 ; x++ ) {
         if (t & 1) { /* plot mega-pixel */
            for( x2=0 ; x2 < dppX; x2++)
               for( y2=0 ; y2 < dppY; y2++)
                  (PlotDot)( X + (long)x*dppX + x2, Y + (long)y*dppY + y2 );
         }
         t=t>>1;
      }
   }
}

static void WriteString( const char *s ) {
   unsigned char ch = *s;
   while (ch >= 32) {
      if (ch <= MAX_DEF_CHAR)
         WriteLetter( ch, xLast, yLast );
      ch = *(++s);
      xLast += (long)CHAR_SPACING * dppX;
   }
}
