/* > printps.c & prnthpgl.c */
/* Device dependent part of Survex Postscript printer/HPGL plotter driver */
/* Copyright (C) 1993-1996 Olly Betts */

/*
1993.05.06 First attempt (modified from PCLDriver.c & DM_PCL.c)
           Corrected \CROSS {...} defn -> \CROSS {...} def
           define M and L for moveto and lineto to shorten file
1993.05.07 reduced number of elements in each path from 2500+ to 2
           fixed brackets and deg and (C) in WriteString
1993.05.08 moved 'stroke' from after each 'lineto' to before each 'moveto' so
            that 0 0 M 1 1 L 1 0 L works
1993.05.10 added header comments to PS files produced
1993.05.12 CROSS -> C ; clippath corrected to clip
           printf("%%... -> printf("%%%%... to get %%... in PS file
1993.06.04 added () to #if
1993.06.10 name changed
1993.06.11 now expect margin measurements in pscfg
1993.06.16 syserr() -> fatal(); free() -> osfree()
1993.06.28 (BP) added changes to give degree & (C) symbols
           (BP) added changes to make multi-page printouts to work
           reduced BP clip box by one pixel on each margin
           look for 'pscfg' not 'PSCfg' (pointed out by MF)
1993.06.29 corrected border on multi-page plots
           now clear clip path before printing footer so it can be seen
1993.06.30 added document structuring conventions suggested by MF
           and a little more
1993.07.02 added MF's unix pipe code
1993.07.12 split off printer i/o code into prio.c
1993.08.13 fettled header
1993.11.03 changed error routines
1993.11.05 changed error numbers so we can use same error list as survex
1993.11.15 created print_malloc & print_free to claim & release all memory
1993.11.18 fopen -> safe_fopen
1993.11.28 "pscfg" replaced by macro PS_CFG defd in filelist.h
1993.11.29 error 10->24
1993.11.30 sorted out error vs fatal
1993.12.05 updated args to NewPage; added dummy fBlankPage code
           removed extra #include <ctype.h>
1994.01.05 output no longer sent to printer by print_init
1994.03.15 last line drawn was not being stroke-d
1994.03.17 coords now long rather than int to increase coord range for DOS
1994.03.19 xpPageWidth and ypPageDepth now long rather than int
1994.06.02 fixed bug of passing 0 to %ld in printf
1994.06.20 created prindept.h and suppressed -fh warnings
           added int argument to warning, error and fatal
1994.06.22 removed prototypes now in header files
1994.09.13 fettled a little
1995.04.16 reindented
1995.04.17 merged in prnthpgl.c - change list:
>>1993.05.07 First attempt (modified from PSDriver.c)
>>           Need to fill in the blanks - a manual would be nice
>>1993.06.04 added () to #if
>>1993.06.10 name changed
>>           added note about stdprn
>>1993.06.16 syserr() -> fatal(); free() -> osfree()
>>1993.06.29 (WP) added extra bits to get it all working
>>           (WP) set up extended character set for copyright and degrees
>>           (WP) extra bits of initialisation
>>           (WP) modified WriteString to handle extended char sets
>>           HPGLCfg -> hpglcfg for Unix, etc
>>1993.07.01 de-tabbed
>>           change 'firstpage' stuff to be similar to that in printps.c
>>           added code for DrawCross() which *might* work
>>           removed unused sz szFont[80]; declaration
>>1993.07.02 added MF's unix pipe code
>>           increased size of aligment ticks on multipage plots
>>           changed chars redefined in extended char set to look ok if
>>            plotter doesn't have extended character sets
>>           removed utterly bogus -1 from calculation of clip box size
>>1993.07.03 removed SC command as it's wrong!
>>           moved image up so that footer reappears
>>1993.07.12 split off printer i/o code into prio.c
>>1993.07.18 added 'SR' to scale text output (factors guessed, but look okay)
>>1993.07.19 increased crosses to a (hopefully sensible) size
>>           erroneous reference to "points" changed to "units"
>>1993.07.21 now use UC for User-defined Characters (works on HP7475A)
>>           generally tidied - several comments updated
>>           removed unnecessary (?) IP command
>>           removed superfluous spaces and '\n's from output
>>1993.08.13 text now fixed size, not relative to page size (was bad on A0!)
>>           fettled header
>>           removed CA-1;GM0,800; when HPGL_USE_UC
>>1993.08.16 prettied up c in (C) symbol (UC & DL vsns); corrected DL circle
>>1993.11.03 changed error routines
>>1993.11.05 changed error numbers so we can use same error list as survex
>>1993.11.15 created print_malloc & print_free to claim & release all memory
>>1993.11.18 fopen -> safe_fopen
>>1993.11.28 "hpglcfg" replaced by macro HPGL_CFG defd in filelist.h
>>1993.11.29 error 10->24
>>1993.11.30 sorted out error vs fatal
>>1993.12.05 updated args to NewPage; added dummy fBlankPage code
>>           removed extra #include <ctype.h>
>>1994.01.06 no longer start to output to printer in print_init
>>1994.03.13 Adding missing line to print_init to initialise fFirstPage
>>1994.03.17 coords now long rather than int to increase coord range for DOS
>>1994.03.19 xpPageWidth and ypPageDepth now long rather than int
>>           error fettling
>>1994.06.20 created prindept.h and suppressed -fh warnings
>>	   added int argument to warning, error and fatal
>>1994.06.22 added fNewLines to sprinkle newlines (always on currently)
>>1995.03.25 fettled
>>1995.04.15 added centring to allow for (0,0) at centre of plotter
>>1995.04.16 reindented
1995.04.20 continued fettling; now give "Symbol" in "fonts used" list
1995.04.22 added more functions to device
1995.06.05 XX_name -> XX_Name
1995.06.24 started to add ini file stuff
1995.06.25 added missing #else and #endif
1995.10.06 added "like" to list passed to ini_read_hier
1995.12.11 symbol font name now in szFontSymbol (#define at present)
1995.12.20 added as_xxx functions to simplify reading of .ini files
1996.02.06 added origin_in_centre to HPGL .ini file
1996.02.09 pfont is now in iso-8859-1, so Copyright and degree signs changed
1996.03.10 as_string added (to trap NULL)
1996.04.01 fixed signed char problem
1996.04.03 corrected char literals in switch
1996.05.18 (hopefully) fixed over-keen clipping to top and right
1996.09.12 line width is now read as floating point
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
#include "filename.h"
#include "message.h"
#include "prio.h"
#include "filelist.h"
#include "debug.h" /* for BUG and ASSERT */
#include "prindept.h"
#include "ini.h"

#ifdef HPGL
# define HPGL_USE_UC
/*# define HPGL_USE_SR */ /* for text sized relative to page size */

# define HPGL_UNITS_PER_MM 40
# define HPGL_EOL "\003" /* terminates labelling commands: LB<string>\003 */

# ifndef HPGL_USE_UC
#  define HPGL_SO "\016" /* shift in & shift out of extended character set */
#  define HPGL_SI "\017"
# endif

# define HPGL_TS 120 /* size of alignment 'ticks' on multipage printouts */
# define HPGL_CROSS_SIZE 28 /* length of cross arms (in HPGL units) */

static char *hpgl_Name( void );
static int  hpgl_Pre( int pagesToPrint );
static void hpgl_NewPage( int pg, int pass, int pagesX, int pagesY );
static void hpgl_Init( FILE *fh, const char *pth, float *pscX, float *pscY );
static void hpgl_MoveTo( long x, long y );
static void hpgl_DrawTo( long x, long y );
static void hpgl_DrawCross( long x, long y );
static void hpgl_WriteString( const char *s );
static void hpgl_ShowPage( const char *szPageDetails );

/*device hpgl={*/
device printer={
   hpgl_Name,
   hpgl_Init,
   hpgl_Pre,
   hpgl_NewPage,
   hpgl_MoveTo,
   hpgl_DrawTo,
   hpgl_DrawCross,
   hpgl_WriteString,
   hpgl_ShowPage,
   NULL, /* hpgl_Post,*/
   prio_close
};
#else
# define POINTS_PER_INCH 72.0f
# define POINTS_PER_MM ((float)(POINTS_PER_INCH)/(MM_PER_INCH))

static float MarginLeft,MarginRight,MarginTop,MarginBottom;
static int FontSize;
static float LineWidth;

static char* szFont;

# define PS_TS 9 /* size of alignment 'ticks' on multipage printouts */
# define PS_CROSS_SIZE 2 /* length of cross arms (in points!) */

#define szFontSymbol "Symbol"

static char *ps_Name( void );
static int  ps_Pre( int pagesToPrint );
static void ps_NewPage( int pg, int pass, int pagesX, int pagesY );
static void ps_Init( FILE *fh, const char *pth, float *pscX, float *pscY );
static void ps_MoveTo( long x, long y );
static void ps_DrawTo( long x, long y );
static void ps_DrawCross( long x, long y );
static void ps_WriteString( const char *s );
static void ps_ShowPage( const char *szPageDetails );
static void ps_Quit( void );

/*device ps={*/
device printer={
   ps_Name,
   ps_Init,
   ps_Pre,
   ps_NewPage,
   ps_MoveTo,
   ps_DrawTo,
   ps_DrawCross,
   ps_WriteString,
   ps_ShowPage,
   NULL, /* ps_Post,*/
   ps_Quit
};
#endif

static border clip;

static long xpPageWidth,ypPageDepth;

#ifdef HPGL
static long x_org, y_org;
static int ypFooter;
static bool fNewLines=fTrue;
static bool fOriginInCentre=fFalse;
#endif

#ifndef HPGL
static char *ps_Name( void ) {
   return "Postscript Printer";
}

static void ps_MoveTo( long x, long y ) {
   prio_printf("%ld %ld M\n",x-clip.x_min,y-clip.y_min);
}

static void ps_DrawTo( long x, long y ) {
   prio_printf("%ld %ld L\n",x-clip.x_min,y-clip.y_min);
}

static void ps_DrawCross( long x, long y ) {
   prio_printf("%ld %ld C\n",x-clip.x_min,y-clip.y_min);
}

static void ps_WriteString( const char *s ) {
   unsigned char ch;
   prio_putc('(');
   while (*s) {
      ch = *s++;
      switch (ch) {
       case 0xB0:
         prio_print("\\312"); /* degree symbol */
         break;
       case 0xA9:
         prio_print(") S SF (\\323) S FF ("); /* (C) symbol (symbol font) */
         break;
       case '(': case ')': case '\\': /* need to escape these characters */
         prio_putc('\\');
         prio_putc(ch);
         break;
       default:
         prio_putc(ch);
         break;
      }
   }
   prio_print(") S\n");
}
#endif

#ifdef HPGL
static char *hpgl_Name( void ) {
   return "HPGL Plotter";
}

static int hpgl_Pre( int pagesToPrint ) {
   pagesToPrint=pagesToPrint; /* shut-up warning */
   /* SR scales characters relative to P1 and P2 */
   /* SI scales characters to size given (in cm) */
   /* INitialise; Select Pen 1;  */
   /* Either: Scale chars Relative to P1 & P2 0.5,1.0 (2/3 deflt size) */
   /*     Or: Scale chars absolute to 2/3 of default size on A4 page */
   prio_print("IN;SP1;"
#ifndef HPGL_USE_UC
               "CA-1;GM0,800;" /* Char set Alternate -1; Get Memory; */
#endif
#ifdef HPGL_USE_SR
               "SR0.5,1.0;");
#else
               "SI0.125,.179;");
#endif
   if (fNewLines)
      prio_putc('\n');
#ifndef HPGL_USE_UC
   /* define degree and copyright symbols */
   prio_print("DL32,10,30,12,30,13,29,13,27,12,26,10,26,9,27,9,29,"
               "10,30;DL40,0,0;"); /* Hope this works! Seems to for BP */
   if (fNewLines)
      prio_putc('\n');
   prio_print("DL67,16,14,16,18,17,22,19,25,22,28,26,30,31,31,37,32,"
               "43,32,49,31,53,30,58,28,61,25,63,22,64,18,64,14,63,10,"
               "61,7,58,4,53,2,49,1,43,0,37,0,31,1,26,2,22,4,19,7,17,10,"
               "16,14;");
   if (fNewLines)
      prio_putc('\n');
   prio_print("DL41,4,20,3,19,0,23,-4,24,-9,24,-14,23,-17,22,-20,19,"
               "-21,16,-20,13,-17,10,-14,9,-9,8,-4,8,0,9,3,11,4,12;");
   if (fNewLines)
      prio_putc('\n');
#endif
   return 1; /* only need 1 pass */
}
#else
static int ps_Pre( int pagesToPrint ) {
   prio_print("%%!PS-Adobe-1.0\n"); /* PS file w/ document structuring */
   prio_printf( "%%%%Title: %s\n", "Survex draft survey" );
   prio_print("%%%%Creator: Survex Postscript Printer Driver\n");
/*   prio_print("%%%%CreationDate: Today :)\n"); */
/*   prio_print("%%%%For: A Surveyor\n"); */
   prio_printf( "%%%%DocumentFonts: %s %s\n", szFont, szFontSymbol );
/*   prio_print("%%%%BoundingBox: llx lly urx ury\n"); */
   /* FF switches to text font; SF switches to symbol font */
   prio_printf("/FF {/%s findfont %d scalefont setfont} def\n",
               szFont, (int) FontSize);
   prio_printf("/SF {/%s findfont %d scalefont setfont} def\n",
               szFontSymbol, (int) FontSize);
   prio_print("FF\n");
   /* Postscript definition for drawing a cross */
#define CS PS_CROSS_SIZE
#define CS2 (2*PS_CROSS_SIZE)
   prio_printf("/C { moveto %d %d rmoveto %d %d rlineto "
               "%d 0 rmoveto %d %d rlineto %d %d rmoveto } def\n",
               -CS,-CS, CS2,CS2, -CS2, CS2,-CS2, -CS,CS );
#undef CS
#undef CS2
   /* define M, L and S to keep file short */
   prio_print("/M {stroke moveto} def /L {lineto} def /S {show} def\n");
   prio_printf("%%%%Pages: %d\n",pagesToPrint);
   prio_print("%%%%EndComments\n");
   prio_print("%%%%EndProlog\n");
   return 1; /* only need 1 pass */
}
#endif

#ifdef HPGL
static void hpgl_NewPage( int pg, int pass, int pagesX, int pagesY ) {
#else
static void ps_NewPage( int pg, int pass, int pagesX, int pagesY ) {
#endif
   int x,y;

   if (pass==-1) {
      fBlankPage=fFalse; /* hack for now */
      return;
   }

   x=(pg-1)%pagesX;
   y=pagesY-1-((pg-1)/pagesX);

   clip.x_min=(long)x*xpPageWidth;
   clip.y_min=(long)y*ypPageDepth;
   clip.x_max=clip.x_min+xpPageWidth; /* dm/pcl/ps had -1; */
   clip.y_max=clip.y_min+ypPageDepth; /* dm/pcl/ps had -1; */

#ifdef HPGL
   x_org=clip.x_min;
   y_org=clip.y_min-ypFooter;
   if (fOriginInCentre) {
      x_org+=xpPageWidth/2;
      y_org+=ypPageDepth/2;
   }
#else
   /* page number - not really much use though. (? => no text numbering) */
   prio_printf("%%%%Page: ? %d\n",pg);
   prio_printf("%.1f setlinewidth\n", LineWidth);
   /* shift image up page about 10mm to allow for page footer */
   prio_printf("%ld %ld translate\ngsave\n",
               (long)(MarginLeft*POINTS_PER_MM),
               (long)(((double)MarginBottom+10.0f)*POINTS_PER_MM) );
   /* and set clipping on printer */
#if 0 /* old version */
   prio_printf("%d %d moveto %d %d moveto %d %d moveto %d %d moveto\n"
               "closepath clip\n",
               clip.x_min,clip.y_min, clip.x_min,clip.y_max,
               clip.x_max,clip.y_max, clip.x_max,clip.y_min );
#endif
   /* Bill Purvis' version: */
#ifndef NO_CLIP
   prio_printf("0 0 moveto 0 %ld lineto %ld %ld lineto"
               " %ld 0 lineto\nclosepath clip newpath\n",
               /*0l,0l, 0l,*/ypPageDepth,
               xpPageWidth,ypPageDepth, xpPageWidth/*,0l*/ );
#endif
#endif

#ifdef HPGL
   drawticks(clip,HPGL_TS,x,y);
#else
   drawticks(clip,PS_TS,x,y);
#endif

#ifdef HPGL
   /* and set clipping (Input Window!) on plotter (left,bottom,right,top) */
   prio_printf("IW%ld,%ld,%ld,%ld;", clip.x_min-x_org, clip.y_min-y_org,
      clip.x_min-x_org+xpPageWidth, clip.y_min-y_org+ypPageDepth );
#endif
}

#ifndef HPGL
static void ps_ShowPage( const char *szPageDetails ) {
   prio_printf("stroke grestore\n0 -%d moveto\n",(int)(10.0*POINTS_PER_MM));
   ps_WriteString( szPageDetails );
   prio_print("stroke\nshowpage\n");
}
#endif

/* Initialise HPGL/PS printer routines */
#ifdef HPGL
static void hpgl_Init( FILE *fh, const char *pth, float *pscX, float *pscY ) {
#else
static void ps_Init( FILE *fh, const char *pth, float *pscX, float *pscY ) {
#endif
   char *fnmPrn;
   char **vals;

   pth=pth; /* shut-up warning */
#ifdef HPGL
{
   char *vars[]={ "like", "output", "mm_across_page", "mm_down_page",
                  "origin_in_centre", NULL };
   vals=ini_read_hier( fh, "hpgl", vars );
}
   fnmPrn=as_string(vals[1]);
   PaperWidth=as_float(vals[2],1,FLT_MAX);
   PaperDepth=as_float(vals[3],11,FLT_MAX);
   fOriginInCentre=as_bool(vals[4]);
   PaperDepth-=10; /* Allow 10mm for footer */
   osfree(vals);

   *pscX=*pscY=(float)(HPGL_UNITS_PER_MM);
   xpPageWidth=(long)(HPGL_UNITS_PER_MM*(double)PaperWidth);
   ypPageDepth=(long)(HPGL_UNITS_PER_MM*(double)PaperDepth);

   ypFooter=(int)(HPGL_UNITS_PER_MM*10.0);
#else
{
   /* fix parameter list */
   char *vars[]={ "like", "output", "font",
                  "mm_left_margin", "mm_right_margin",
                  "mm_bottom_margin", "mm_top_margin",
                  "font_size", "line_width", NULL };
   vals=ini_read_hier( fh, "ps", vars );
}
   fnmPrn=as_string(vals[1]);
   szFont=as_string(vals[2]); /* name of font to use */
   MarginLeft=as_float(vals[3],0,FLT_MAX);
   MarginRight=as_float(vals[4],0,FLT_MAX);
   MarginBottom=as_float(vals[5],0,FLT_MAX);
   MarginTop=as_float(vals[6],0,FLT_MAX);
   FontSize=as_int(vals[7],1,INT_MAX);
   LineWidth=as_float(vals[8],0,INT_MAX);
   osfree(vals);
   PaperWidth=MarginRight-MarginLeft;
   PaperDepth=MarginTop-MarginBottom-10; /* Allow 10mm for footer */

   *pscX=*pscY=(float)(POINTS_PER_MM);
   xpPageWidth=(long)(POINTS_PER_MM*PaperWidth);
   ypPageDepth=(long)(POINTS_PER_MM*PaperDepth);
#endif

   prio_open( fnmPrn );
   free( fnmPrn );
}

#ifndef HPGL
static void ps_Quit( void ) {
   prio_print("%%%%Trailer\n");
   prio_close();
}
#endif

#ifdef HPGL
static void hpgl_MoveTo( long x, long y ) {
   prio_printf("PU%ld,%ld;",x-x_org,y-y_org);
}

static void hpgl_DrawTo( long x, long y ) {
   prio_printf("PD%ld,%ld;",x-x_org,y-y_org);
   if (fNewLines)
      prio_putc('\n');
}

static void hpgl_DrawCross( long x, long y ) {
   hpgl_MoveTo(x,y);
   /* SM plots a symbol at each point, but it isn't very convenient here   */
   /* We can write PDPR%d,%dPR%d,%d... but the HP7475A manual doesn't say  */
   /* clearly if this will work on older plotters (such as the HP9872)     */
#define CS HPGL_CROSS_SIZE
#define CS2 (2*HPGL_CROSS_SIZE)
   prio_printf("PD;PR%d,%d;PR%d,%d;PU%d,0;PD%d,%d;PU%d,%d;PA;",
               CS,CS, -CS2,-CS2, CS2,/*0,*/ -CS2,CS2, CS,-CS );
#undef CS
#undef CS2
   if (fNewLines)
      prio_putc('\n');
}

static void hpgl_WriteString( const char *s ) {
   prio_print("LB"); /* Label - terminate text with a ^C */
   while (*s) {
      switch (*s) {
       case '\xB0':
#ifdef HPGL_USE_UC
         /* draw a degree sign */
         prio_print(HPGL_EOL";UC1.25,7.5,99,.25,0,.125,-.25,0,-.5,"
                     "-.125,-.25,-.25,0,-.125,.25,0,.5,.125,.25;LB");
#else
         /* KLUDGE: this prints the degree sign if the plotter supports
          * extended chars or a space if not, since we tried to redefine
          * space.  Nifty, eh? */
         prio_print( HPGL_SO" "HPGL_SI );
#endif
         break;
       case '\xA9':
#ifdef HPGL_USE_UC
         /* (C) needs two chars to look right! */
         /* This bit does the circle of the (C) symbol: */
         prio_print(HPGL_EOL";");
         if (fNewLines)
            prio_putc('\n');
         prio_print("UC2,3.5,99,0,1,0.125,1,0.25,.75,0.375,.75,"
                     ".5,.5,.625,.25,.75,.25,.75,0,.75,-.25,.625,-.25,"
                     ".5,-.5,.375,-.75,.25,-.75,.125,-1,0,-1,-0.125,-1,"
                     "-0.25,-.75,-0.375,-.75,-.5,-.5,-.625,-.25,-.75,-.25,"
                     "-.75,0,-.75,.25,-.625,.25,-.5,.5,-.375,.75,-.25,.75,"
                     "-.125,1;");
         if (fNewLines)
            prio_putc('\n');
         /* And this bit's the c in the middle: */
         prio_print("UC.5,5,99,-.125,.25,-.375,.5,-.5,.25,-.625,0,"
                     "-.625,-.25,-.375,-.25,-.375,-.75,-.125,-.75,.125,-.75,"
                     ".375,-.75,.375,-.25,.625,-.25,.625,0,.5,.25,.375,.5,"
                     ".125,.25;");
         if (fNewLines)
            prio_putc('\n');
         prio_print("LB");
#else
         prio_print( HPGL_SO"(C)"HPGL_SI );
#endif
         break;
       default:
         prio_putc( *s );
      }
      s++;
   }
   prio_print( HPGL_EOL";" );
   if (fNewLines)
      prio_putc('\n');
}

static void hpgl_ShowPage( const char *szPageDetails ) {
   /* clear clipping window and print footer */
   prio_printf("IW;PU%ld,%ld;",
               clip.x_min-x_org,clip.y_min-ypFooter/2L-y_org);
   hpgl_WriteString(szPageDetails);
   prio_print("PG;"); /* New page.  NB PG is a no-op on the HP7475A */
   if (fNewLines)
      prio_putc('\n');
}
#endif
