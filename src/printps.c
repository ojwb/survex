/* > printps.c & prnthpgl.c */
/* Device dependent part of Survex Postscript printer/HPGL plotter driver */
/* Copyright (C) 1993-1996 Olly Betts */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

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
#include "prcore.h"
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

static const char *hpgl_Name(void);
static int hpgl_Pre(int pagesToPrint, const char *title);
static void hpgl_NewPage(int pg, int pass, int pagesX, int pagesY);
static void hpgl_Init(FILE *fh, const char *pth, float *pscX, float *pscY);
static void hpgl_MoveTo(long x, long y);
static void hpgl_DrawTo(long x, long y);
static void hpgl_DrawCross(long x, long y);
static void hpgl_WriteString(const char *s);
static void hpgl_ShowPage(const char *szPageDetails);

/*device hpgl = {*/
device printer = {
   hpgl_Name,
   hpgl_Init,
   hpgl_Pre,
   hpgl_NewPage,
   hpgl_MoveTo,
   hpgl_DrawTo,
   hpgl_DrawCross,
   hpgl_WriteString,
   NULL, /* hpgl_DrawCircle */
   hpgl_ShowPage,
   NULL, /* hpgl_Post,*/
   prio_close
};
#else
# define POINTS_PER_INCH 72.0f
# define POINTS_PER_MM ((float)(POINTS_PER_INCH) / (MM_PER_INCH))

static float MarginLeft, MarginRight, MarginTop, MarginBottom;
static int FontSize;
static float LineWidth;

static char *szFont;

# define PS_TS 9 /* size of alignment 'ticks' on multipage printouts */
# define PS_CROSS_SIZE 2 /* length of cross arms (in points!) */

#define szFontSymbol "Symbol"

static const char *ps_Name(void);
static int ps_Pre(int pagesToPrint, const char *title);
static void ps_NewPage(int pg, int pass, int pagesX, int pagesY);
static void ps_Init(FILE *fh, const char *pth, float *pscX, float *pscY);
static void ps_MoveTo(long x, long y);
static void ps_DrawTo(long x, long y);
static void ps_DrawCross(long x, long y);
static void ps_WriteString(const char *s);
static void ps_DrawCircle(long x, long y, long r);
static void ps_ShowPage(const char *szPageDetails);
static void ps_Quit(void);

/*device ps = {*/
device printer = {
   ps_Name,
   ps_Init,
   ps_Pre,
   ps_NewPage,
   ps_MoveTo,
   ps_DrawTo,
   ps_DrawCross,
   ps_WriteString,
   ps_DrawCircle,
   ps_ShowPage,
   NULL, /* ps_Post,*/
   ps_Quit
};
#endif

static border clip;

static long xpPageWidth, ypPageDepth;

#ifdef HPGL
static long x_org, y_org;
static int ypFooter;
static bool fNewLines = fTrue;
static bool fOriginInCentre = fFalse;
#endif

#ifndef HPGL
static const char *
ps_Name(void)
{
   return "PostScript Printer";
}

static long x_t, y_t;

static void
ps_MoveTo(long x, long y)
{
   x_t = x - clip.x_min;
   y_t = y - clip.y_min;
   prio_printf("%ld %ld M\n", x_t, y_t);
}

static void
ps_DrawTo(long x, long y)
{
   char abuf[64], rbuf[64];
   long x_p = x_t, y_p = y_t;
   x_t = x - clip.x_min;
   y_t = y - clip.y_min;
   sprintf(abuf, "%ld %ld L\n", x_t, y_t);
   sprintf(rbuf, "%ld %ld R\n", x_t - x_p, y_t - y_p);
   if (strlen(rbuf) < strlen(abuf))
      prio_print(rbuf);
   else
      prio_print(abuf);      
}

static void
ps_DrawCross(long x, long y)
{
   prio_printf("%ld %ld X\n", x - clip.x_min, y - clip.y_min);
}

static void
ps_WriteString(const char *s)
{
   unsigned char ch;
   prio_putc('(');
   while (*s) {
      ch = *s++;
      switch (ch) {
       case 0xB0:
         prio_print("\\312"); /* degree symbol */
         break;
       case 0xA9:
         prio_print(") S Z (\\323) S F ("); /* (C) symbol (symbol font) */
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

static void
ps_DrawCircle(long x, long y, long r)
{
   prio_printf("%ld %ld %ld C\n", x - clip.x_min, y - clip.y_min, r);
}
#endif

#ifdef HPGL
static const char *
hpgl_Name(void)
{
   return "HPGL Plotter";
}

static int
hpgl_Pre(int pagesToPrint, const char *title)
{
   pagesToPrint = pagesToPrint; /* shut-up warning */
   title = title; /* shut-up warning */
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
	      "SR0.5,1.0;"
#else
	      "SI0.125,.179;"
#endif
	      );
   if (fNewLines) prio_putc('\n');

#ifndef HPGL_USE_UC
   /* define degree and copyright symbols */
   prio_print("DL32,10,30,12,30,13,29,13,27,12,26,10,26,9,27,9,29,"
	      "10,30;DL40,0,0;"); /* Hope this works! Seems to for BP */
   if (fNewLines) prio_putc('\n');

   prio_print("DL67,16,14,16,18,17,22,19,25,22,28,26,30,31,31,37,32,"
	      "43,32,49,31,53,30,58,28,61,25,63,22,64,18,64,14,63,10,"
	      "61,7,58,4,53,2,49,1,43,0,37,0,31,1,26,2,22,4,19,7,17,10,"
	      "16,14;");
   if (fNewLines) prio_putc('\n');

   prio_print("DL41,4,20,3,19,0,23,-4,24,-9,24,-14,23,-17,22,-20,19,"
	      "-21,16,-20,13,-17,10,-14,9,-9,8,-4,8,0,9,3,11,4,12;");
   if (fNewLines) prio_putc('\n');
#endif

   return 1; /* only need 1 pass */
}
#else
static int
ps_Pre(int pagesToPrint, const char *title)
{
   prio_print("%!PS-Adobe-1.0\n"); /* PS file w/ document structuring */
   prio_printf("%%%%Title: %s\n", title);
   prio_print("%%Creator: Survex "VERSION" Postscript Printer Driver\n");
/* FIXME  prio_print("%%CreationDate: Today :)\n"); */
/*   prio_print("%%For: A Surveyor\n"); */
   prio_printf("%%%%DocumentFonts: %s %s\n", szFont, szFontSymbol);
   prio_printf("%%%%BoundingBox: 0 0 %ld %ld\n",
	       xpPageWidth + (long)(2 * MarginLeft * POINTS_PER_MM),
	       ypPageDepth + (long)((10 + 2 * MarginBottom) * POINTS_PER_MM));
   /* FIXME is this a level 1 feature?   prio_print("%%PageOrder: Ascend\n"); */
   prio_printf("%%%%Pages: %d\n", pagesToPrint);

   /* F switches to text font; Z switches to symbol font */
   prio_printf("/F {/%s findfont %d scalefont setfont} def\n",
	       szFont, (int)FontSize);
   prio_printf("/Z {/%s findfont %d scalefont setfont} def\n",
               szFontSymbol, (int)FontSize);
   prio_print("F\n");
   prio_printf("%.1f setlinewidth\n", LineWidth);

   /* Postscript definition for drawing a cross */
#define CS PS_CROSS_SIZE
#define CS2 (2*PS_CROSS_SIZE)
   prio_printf("/X {stroke moveto %d %d rmoveto %d %d rlineto "
               "%d 0 rmoveto %d %d rlineto %d %d rmoveto} def\n",
               -CS, -CS,  CS2, CS2,  -CS2,  CS2, -CS2,  -CS, CS );
#undef CS
#undef CS2

   /* define some functions to keep file short */
   prio_print("/M {stroke moveto} def\n"
	      "/L {lineto} def\n"
	      "/R {rlineto} def\n"
	      "/S {show} def\n"
	      "/C {stroke newpath 0 360 arc} def\n");

   prio_print("%%EndComments\n");
   prio_print("%%EndProlog\n");

   return 1; /* only need 1 pass */
}
#endif

static void
#ifdef HPGL
hpgl_NewPage(int pg, int pass, int pagesX, int pagesY)
#else
ps_NewPage(int pg, int pass, int pagesX, int pagesY)
#endif
{
   int x, y;

   if (pass == -1) {
      fBlankPage = fFalse; /* hack for now */
      return;
   }

   x = (pg - 1) % pagesX;
   y = pagesY - 1 - ((pg - 1) / pagesX);

   clip.x_min = (long)x * xpPageWidth;
   clip.y_min = (long)y * ypPageDepth;
   clip.x_max = clip.x_min + xpPageWidth; /* dm/pcl/ps had -1; */
   clip.y_max = clip.y_min + ypPageDepth; /* dm/pcl/ps had -1; */

#ifdef HPGL
   x_org = clip.x_min;
   y_org = clip.y_min - ypFooter;
   if (fOriginInCentre) {
      x_org += xpPageWidth / 2;
      y_org += ypPageDepth / 2;
   }
#else
   /* page number - not really much use though. (? => no text numbering) */
/*   prio_printf("%%%%Page: ? %d\n", pg);*/
   prio_printf("%%%%Page: %d %d\n", pg, pg);
   /* shift image up page about 10mm to allow for page footer */
   prio_printf("%ld %ld translate\ngsave\n",
               (long)(MarginLeft * POINTS_PER_MM),
               (long)(((double)MarginBottom + 10.0f) * POINTS_PER_MM));
   /* and set clipping on printer */
#if 0 /* old version */
   prio_printf("%d %d moveto %d %d moveto %d %d moveto %d %d moveto\n"
               "closepath clip\n",
               clip.x_min,clip.y_min, clip.x_min,clip.y_max,
               clip.x_max,clip.y_max, clip.x_max,clip.y_min);
#endif
   /* Bill Purvis' version: */
#ifndef NO_CLIP
   prio_printf("0 0 moveto 0 %ld lineto %ld %ld lineto"
               " %ld 0 lineto\nclosepath clip newpath\n",
               /*0l,0l, 0l,*/ ypPageDepth,
               xpPageWidth, ypPageDepth, xpPageWidth /*,0l*/ );
#endif
#endif

#ifdef HPGL
   drawticks(clip, HPGL_TS, x, y);
#else
   drawticks(clip, PS_TS, x, y);
#endif

#ifdef HPGL
   /* and set clipping (Input Window!) on plotter (left,bottom,right,top) */
   prio_printf("IW%ld,%ld,%ld,%ld;", clip.x_min - x_org, clip.y_min - y_org,
	       clip.x_min - x_org + xpPageWidth,
	       clip.y_min - y_org + ypPageDepth);
#endif
}

#ifndef HPGL
static void
ps_ShowPage(const char *szPageDetails)
{
   prio_printf("stroke grestore\n0 -%d moveto\n", (int)(10.0 * POINTS_PER_MM));
   ps_WriteString(szPageDetails);
   prio_print("stroke\nshowpage\n");
}
#endif

/* Initialise HPGL/PS printer routines */
static void
#ifdef HPGL
hpgl_Init(FILE *fh, const char *pth, float *pscX, float *pscY)
#else
ps_Init(FILE *fh, const char *pth, float *pscX, float *pscY)
#endif
{
   char *fnmPrn;
   char **vals;

   pth = pth; /* shut-up warning */
#ifdef HPGL
{
   char *vars[] = {"like", "output", "mm_across_page", "mm_down_page",
      "origin_in_centre", NULL};
   vals = ini_read_hier(fh, "hpgl", vars);
}
   fnmPrn = as_string(vals[1]);
   PaperWidth = as_float(vals[2], 1, FLT_MAX);
   PaperDepth = as_float(vals[3], 11, FLT_MAX);
   fOriginInCentre = as_bool(vals[4]);
   PaperDepth -= 10; /* Allow 10mm for footer */
   osfree(vals);

   *pscX = *pscY = (float)(HPGL_UNITS_PER_MM);
   xpPageWidth = (long)(HPGL_UNITS_PER_MM * (double)PaperWidth);
   ypPageDepth = (long)(HPGL_UNITS_PER_MM * (double)PaperDepth);

   ypFooter = (int)(HPGL_UNITS_PER_MM * 10.0);
#else
{
   /* fix parameter list */
   char *vars[] = {"like", "output", "font",
      "mm_left_margin", "mm_right_margin",
      "mm_bottom_margin", "mm_top_margin",
      "font_size", "line_width", NULL};
   vals = ini_read_hier(fh, "ps", vars);
}
   fnmPrn = as_string(vals[1]);
   szFont = as_string(vals[2]); /* name of font to use */
   MarginLeft = as_float(vals[3], 0, FLT_MAX);
   MarginRight = as_float(vals[4], 0, FLT_MAX);
   MarginBottom = as_float(vals[5], 0, FLT_MAX);
   MarginTop = as_float(vals[6], 0, FLT_MAX);
   FontSize = as_int(vals[7], 1, INT_MAX);
   LineWidth = as_float(vals[8], 0, INT_MAX);
   osfree(vals);
   PaperWidth = MarginRight - MarginLeft;
   PaperDepth = MarginTop - MarginBottom - 10; /* Allow 10mm for footer */

   *pscX = *pscY = (float)(POINTS_PER_MM);
   xpPageWidth = (long)(POINTS_PER_MM * PaperWidth);
   ypPageDepth = (long)(POINTS_PER_MM * PaperDepth);
#endif

   prio_open(fnmPrn);
   osfree(fnmPrn);
}

#ifndef HPGL
static void
ps_Quit(void)
{
   prio_print("%%Trailer\n");
   prio_close();
}
#endif

#ifdef HPGL
static void
hpgl_MoveTo(long x, long y)
{
   prio_printf("PU%ld,%ld;", x - x_org, y - y_org);
}

static void
hpgl_DrawTo(long x, long y)
{
   prio_printf("PD%ld,%ld;", x - x_org, y - y_org);
   if (fNewLines) prio_putc('\n');
}

static void
hpgl_DrawCross(long x, long y)
{
   hpgl_MoveTo(x, y);
   /* SM plots a symbol at each point, but it isn't very convenient here   */
   /* We can write PDPR%d,%dPR%d,%d... but the HP7475A manual doesn't say  */
   /* clearly if this will work on older plotters (such as the HP9872)     */
#define CS HPGL_CROSS_SIZE
#define CS2 (2 * HPGL_CROSS_SIZE)
   prio_printf("PD;PR%d,%d;PR%d,%d;PU%d,0;PD%d,%d;PU%d,%d;PA;",
               CS, CS,  -CS2, -CS2,  CS2, /*0,*/  -CS2, CS2,  CS, -CS);
#undef CS
#undef CS2
   if (fNewLines) prio_putc('\n');
}

static void
hpgl_WriteString(const char *s)
{
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
         prio_print(HPGL_SO" "HPGL_SI);
#endif
         break;
       case '\xA9':
#ifdef HPGL_USE_UC
         /* (C) needs two chars to look right! */
         /* This bit does the circle of the (C) symbol: */
         prio_print(HPGL_EOL";");
         if (fNewLines) prio_putc('\n');
         prio_print("UC2,3.5,99,0,1,0.125,1,0.25,.75,0.375,.75,"
		    ".5,.5,.625,.25,.75,.25,.75,0,.75,-.25,.625,-.25,"
		    ".5,-.5,.375,-.75,.25,-.75,.125,-1,0,-1,-0.125,-1,"
		    "-0.25,-.75,-0.375,-.75,-.5,-.5,-.625,-.25,-.75,-.25,"
		    "-.75,0,-.75,.25,-.625,.25,-.5,.5,-.375,.75,-.25,.75,"
		    "-.125,1;");
         if (fNewLines) prio_putc('\n');
         /* And this bit's the c in the middle: */
         prio_print("UC.5,5,99,-.125,.25,-.375,.5,-.5,.25,-.625,0,"
		    "-.625,-.25,-.375,-.25,-.375,-.75,-.125,-.75,.125,-.75,"
		    ".375,-.75,.375,-.25,.625,-.25,.625,0,.5,.25,.375,.5,"
		    ".125,.25;");
         if (fNewLines) prio_putc('\n');
         prio_print("LB");
#else
         prio_print(HPGL_SO"(C)"HPGL_SI);
#endif
         break;
       default:
         prio_putc(*s);
      }
      s++;
   }
   prio_print(HPGL_EOL";");
   if (fNewLines) prio_putc('\n');
}

static void
hpgl_ShowPage(const char *szPageDetails)
{
   /* clear clipping window and print footer */
   prio_printf("IW;PU%ld,%ld;",
               clip.x_min - x_org, clip.y_min - ypFooter / 2L - y_org);
   hpgl_WriteString(szPageDetails);
   prio_print("PG;"); /* New page.  NB PG is a no-op on the HP7475A */
   if (fNewLines) prio_putc('\n');
}
#endif
