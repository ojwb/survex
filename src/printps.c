/* printps.c & prnthpgl.c */
/* Device dependent part of Survex Postscript printer/HPGL plotter driver */
/* Copyright (C) 1993-2002 Olly Betts
 *
 * Postscript font remapping code based on code from a2ps
 * Copyright (C) 1995-1999 Akim Demaille, Miguel Santana
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
static char *hpgl_Init(FILE **fh_list, const char *pth, const char *out_fnm,
		      double *pscX, double *pscY, bool fCalibrate);
static void hpgl_MoveTo(long x, long y);
static void hpgl_DrawTo(long x, long y);
static void hpgl_DrawCross(long x, long y);
static void hpgl_WriteString(const char *s);
static void hpgl_ShowPage(const char *szPageDetails);

/*device hpgl = {*/
device printer = {
   0,
   hpgl_Name,
   hpgl_Init,
   NULL, /* Charset */
   hpgl_Pre,
   hpgl_NewPage,
   hpgl_MoveTo,
   hpgl_DrawTo,
   hpgl_DrawCross,
   NULL, /* SetFont */
   NULL, /* SetColour */
   hpgl_WriteString,
   NULL, /* hpgl_DrawCircle */
   hpgl_ShowPage,
   NULL, /* hpgl_Post,*/
   prio_close
};
#else

# if defined(HAVE_GETPWUID) && !defined(__DJGPP__)
#  include <pwd.h>
#  include <sys/types.h>
#  include <unistd.h>
# endif

# define POINTS_PER_INCH 72.0
# define POINTS_PER_MM (POINTS_PER_INCH / MM_PER_INCH)

static double MarginLeft, MarginRight, MarginTop, MarginBottom;
static int fontsize, fontsize_labels;
static double LineWidth;

static const char *fontname, *fontname_labels;

# define PS_TS 9 /* size of alignment 'ticks' on multipage printouts */
# define PS_CROSS_SIZE 2 /* length of cross arms (in points!) */

static unsigned long colour[6];

static const char *ps_Name(void);
static int ps_Pre(int pagesToPrint, const char *title);
static void ps_NewPage(int pg, int pass, int pagesX, int pagesY);
static char *ps_Init(FILE **fh_list, const char *pth, const char *out_fnm,
		    double *pscX, double *pscY, bool fCalibrate);
static int CharsetLatin1(void);
static void ps_MoveTo(long x, long y);
static void ps_DrawTo(long x, long y);
static void ps_DrawCross(long x, long y);
static void ps_SetFont(int fontcode);
static void ps_SetColour(int colourcode);
static void ps_WriteString(const char *s);
static void ps_DrawCircle(long x, long y, long r);
static void ps_ShowPage(const char *szPageDetails);
static void ps_Quit(void);

/*device ps = {*/
device printer = {
   0,
   ps_Name,
   ps_Init,
   CharsetLatin1,
   ps_Pre,
   ps_NewPage,
   ps_MoveTo,
   ps_DrawTo,
   ps_DrawCross,
   ps_SetFont,
   ps_SetColour,
   ps_WriteString,
   ps_DrawCircle,
   ps_ShowPage,
   NULL, /* ps_Post,*/
   ps_Quit
};
#endif

static int cur_pass;

static border clip;

static long xpPageWidth, ypPageDepth;

#ifdef HPGL
static long x_org, y_org;
static int ypFooter;
static bool fNewLines = fTrue;
static bool fOriginInCentre = fFalse;
#endif

static long x_t, y_t;

static int
check_intersection(long x_p, long y_p)
{
#define U 1
#define D 2
#define L 4
#define R 8
   int mask_p = 0, mask_t = 0;
   if (x_p < 0)
      mask_p = L;
   else if (x_p > xpPageWidth)
      mask_p = R;

   if (y_p < 0)
      mask_p |= D;
   else if (y_p > ypPageDepth)
      mask_p |= U;

   if (x_t < 0)
      mask_t = L;
   else if (x_t > xpPageWidth)
      mask_t = R;

   if (y_t < 0)
      mask_t |= D;
   else if (y_t > ypPageDepth)
      mask_t |= U;

#if 0
   /* approximation to correct answer */
   return !(mask_t & mask_p);
#else
   /* One end of the line is on the page */
   if (!mask_t || !mask_p) return 1;

   /* whole line is above, left, right, or below page */
   if (mask_t & mask_p) return 0;

   /* FIXME: these 4 checks are bogus - copy code from gfxcore.cc that works */
   if (mask_t & U) {
      double v = (double)((x_p - x_t) * ypPageDepth + x_t * y_p - x_p * y_t) /
		 (y_p - y_t);
      return v >= 0 && v <= xpPageWidth;
   }
   if (mask_t & D) {
      double v = (double)(x_t * y_p - x_p * y_t) / (y_p - y_t);
      return v >= 0 && v <= xpPageWidth;
   }
   if (mask_t & R) {
      double v = (double)((y_p - y_t) * xpPageWidth + x_p * y_t - x_t * y_p) /
		 (x_p - x_t);
      return v >= 0 && v <= ypPageDepth;
   }
   ASSERT(mask_t & L);
   {
      double v = (double)(x_p * y_t - x_t * y_p) / (x_p - x_t);
      return v >= 0 && v <= ypPageDepth;
   }
#endif
#undef U
#undef D
#undef L
#undef R
}

#ifndef HPGL
static const char *
ps_Name(void)
{
   return "PostScript Printer";
}

static void
ps_MoveTo(long x, long y)
{
   x_t = x - clip.x_min;
   y_t = y - clip.y_min;
   if (cur_pass != -1) prio_printf("%ld %ld M\n", x_t, y_t);
}

static void
ps_DrawTo(long x, long y)
{
   char abuf[64], rbuf[64];
   long x_p = x_t, y_p = y_t;
   x_t = x - clip.x_min;
   y_t = y - clip.y_min;
   if (cur_pass != -1) {
      sprintf(abuf, "%ld %ld L\n", x_t, y_t);
      sprintf(rbuf, "%ld %ld R\n", x_t - x_p, y_t - y_p);
      if (strlen(rbuf) < strlen(abuf))
	 prio_print(rbuf);
      else
	 prio_print(abuf);
   } else {
      if (check_intersection(x_p, y_p)) fBlankPage = fFalse;
   }
}

static void
ps_DrawCross(long x, long y)
{
   if (cur_pass != -1) {
      prio_printf("%ld %ld X\n", x - clip.x_min, y - clip.y_min);
   } else {
      if ((x + PS_CROSS_SIZE > clip.x_min && x - PS_CROSS_SIZE < clip.x_max) ||
	  (y + PS_CROSS_SIZE > clip.y_min && y - PS_CROSS_SIZE < clip.y_max)) {
	 fBlankPage = fFalse;
      }
   }
}

static char current_font_code = 'F';

static void
ps_SetFont(int fontcode)
{
   char old = current_font_code;
   switch (fontcode) {
    case PR_FONT_DEFAULT:
      current_font_code = 'F';
      break;
    case PR_FONT_LABELS:
      current_font_code = 'N';
      break;
    default:
      BUG("unknown font code");
   }
   if (current_font_code != old) {
      prio_putc(current_font_code);
      prio_putc(' ');
   }
}

static int current_colourcode = -1;
static void
ps_SetColour(int colourcode)
{
   if (colourcode != current_colourcode) {
      current_colourcode = colourcode;
      prio_printf("C%d ", colourcode);
   }
}

static void
ps_WriteString(const char *s)
{
   if (cur_pass != -1) {
      unsigned char ch;
      prio_putc('(');
      while (*s) {
	 ch = *s++;
	 switch (ch) {
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
   } else {
      int fs = (current_font_code == 'F' ? fontsize : fontsize_labels);
      if ((y_t + 3 * fs / 4 > clip.y_min && y_t - fs / 4 < clip.y_max) ||
	  (x_t < clip.x_max && x_t + strlen(s) * fs > clip.x_min)) {
	 fBlankPage = fFalse;
      }
   }
}

static void
ps_DrawCircle(long x, long y, long r)
{
   /* Don't need to check in first-pass - circle is only used in title box */
   if (cur_pass != -1) {
      prio_printf("%ld %ld %ld C\n", x - clip.x_min, y - clip.y_min, r);
   }
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
   time_t now = time(NULL);
   char *name = NULL;
   char buf[1024];
#if defined(HAVE_GETPWUID) && !defined(__DJGPP__)
   struct passwd *ent;
#endif

   /* PS file w/ EPS document structuring */
   /* FIXME: prio_print("%!PS-Adobe-3.0 EPSF-3.0\n"); */
   prio_print("%!PS-Adobe-1.0\n");

   prio_print("%%Title: ");
   prio_print(title);
   prio_putc('\n');

   prio_print("%%Creator: Survex "VERSION" Postscript Printer Driver\n");
   if (strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %Z\n", localtime(&now))) {
      prio_print("%%CreationDate: ");
      prio_print(buf);
   }

#if defined(HAVE_GETPWUID) && !defined(__DJGPP__)
   ent = getpwuid(getuid());
   if (ent && ent->pw_gecos[0]) name = ent->pw_gecos;
#endif
   if (!name) name = getenv("LOGNAME");
   if (name) {
      prio_print("%%For: ");
      prio_print(name);
      prio_putc('\n');
   }

   /* FIXME: bounding box is too big when border is turned off... */
   prio_printf("%%%%BoundingBox: 0 0 %ld %ld\n",
	       xpPageWidth + (long)(2.0 * MarginLeft * POINTS_PER_MM),
	       ypPageDepth + (long)((10.0 + 2.0 * MarginBottom) * POINTS_PER_MM));
   prio_print("%%PageOrder: Ascend\n");
   prio_printf("%%%%Pages: %d\n", pagesToPrint);
   prio_print("%%Orientation: Portrait\n");

   prio_print("%%DocumentFonts: ");
   prio_print(fontname);
   if (strcmp(fontname, fontname_labels) != 0) {
      prio_putc(' ');
      prio_print(fontname_labels);
   }
   prio_putc('\n');

   /* this code adapted from a2ps */
   prio_print("%%BeginResource: encoding ISO88591Encoding\n"
              "/ISO88591Encoding [");
   prio_print("/.notdef /.notdef /.notdef /.notdef\n");
   prio_print("/.notdef /.notdef /.notdef /.notdef\n");
   prio_print("/.notdef /.notdef /.notdef /.notdef\n");
   prio_print("/.notdef /.notdef /.notdef /.notdef\n");
   prio_print("/.notdef /.notdef /.notdef /.notdef\n");
   prio_print("/.notdef /.notdef /.notdef /.notdef\n");
   prio_print("/.notdef /.notdef /.notdef /.notdef\n");
   prio_print("/.notdef /.notdef /.notdef /.notdef\n");
   prio_print(
"/space /exclam /quotedbl /numbersign\n"
"/dollar /percent /ampersand /quoteright\n"
"/parenleft /parenright /asterisk /plus\n"
"/comma /minus /period /slash\n"
"/zero /one /two /three\n"
"/four /five /six /seven\n"
"/eight /nine /colon /semicolon\n"
"/less /equal /greater /question\n"
"/at /A /B /C /D /E /F /G\n"
"/H /I /J /K /L /M /N /O\n"
"/P /Q /R /S /T /U /V /W\n"
"/X /Y /Z /bracketleft\n"
"/backslash /bracketright /asciicircum /underscore\n"
"/quoteleft /a /b /c /d /e /f /g\n"
"/h /i /j /k /l /m /n /o\n"
"/p /q /r /s /t /u /v /w\n"
"/x /y /z /braceleft\n"
"/bar /braceright /asciitilde /.notdef\n");
   prio_print("/.notdef /.notdef /.notdef /.notdef\n");
   prio_print("/.notdef /.notdef /.notdef /.notdef\n");
   prio_print("/.notdef /.notdef /.notdef /.notdef\n");
   prio_print("/.notdef /.notdef /.notdef /.notdef\n");
   prio_print("/.notdef /.notdef /.notdef /.notdef\n");
   prio_print("/.notdef /.notdef /.notdef /.notdef\n");
   prio_print("/.notdef /.notdef /.notdef /.notdef\n");
   prio_print("/.notdef /.notdef /.notdef /.notdef\n");
   prio_print(
"/space /exclamdown /cent /sterling\n"
"/currency /yen /brokenbar /section\n"
"/dieresis /copyright /ordfeminine /guillemotleft\n"
"/logicalnot /hyphen /registered /macron\n"
"/degree /plusminus /twosuperior /threesuperior\n"
"/acute /mu /paragraph /bullet\n"
"/cedilla /onesuperior /ordmasculine /guillemotright\n"
"/onequarter /onehalf /threequarters /questiondown\n"
"/Agrave /Aacute /Acircumflex /Atilde\n"
"/Adieresis /Aring /AE /Ccedilla\n"
"/Egrave /Eacute /Ecircumflex /Edieresis\n"
"/Igrave /Iacute /Icircumflex /Idieresis\n"
"/Eth /Ntilde /Ograve /Oacute\n"
"/Ocircumflex /Otilde /Odieresis /multiply\n"
"/Oslash /Ugrave /Uacute /Ucircumflex\n"
"/Udieresis /Yacute /Thorn /germandbls\n"
"/agrave /aacute /acircumflex /atilde\n"
"/adieresis /aring /ae /ccedilla\n"
"/egrave /eacute /ecircumflex /edieresis\n"
"/igrave /iacute /icircumflex /idieresis\n"
"/eth /ntilde /ograve /oacute\n"
"/ocircumflex /otilde /odieresis /divide\n"
"/oslash /ugrave /uacute /ucircumflex\n"
"/udieresis /yacute /thorn /ydieresis\n"
"] def\n"
"%%EndResource\n");

   /* this code adapted from a2ps */
   prio_print(
"/reencode {\n" /* def */
"dup length 5 add dict begin\n"
"{\n" /* forall */
"1 index /FID ne\n"
"{ def }{ pop pop } ifelse\n"
"} forall\n"
"/Encoding exch def\n"

/* Use the font's bounding box to determine the ascent, descent,
 * and overall height; don't forget that these values have to be
 * transformed using the font's matrix.
 * We use `load' because sometimes BBox is executable, sometimes not.
 * Since we need 4 numbers and not an array avoid BBox from being executed
 */
"/FontBBox load aload pop\n"
"FontMatrix transform /Ascent exch def pop\n"
"FontMatrix transform /Descent exch def pop\n"
"/FontHeight Ascent Descent sub def\n"

/* Define these in case they're not in the FontInfo (also, here
 * they're easier to get to.
 */
"/UnderlinePosition 1 def\n"
"/UnderlineThickness 1 def\n"

/* Get the underline position and thickness if they're defined. */
"currentdict /FontInfo known {\n"
"FontInfo\n"

"dup /UnderlinePosition known {\n"
"dup /UnderlinePosition get\n"
"0 exch FontMatrix transform exch pop\n"
"/UnderlinePosition exch def\n"
"} if\n"

"dup /UnderlineThickness known {\n"
"/UnderlineThickness get\n"
"0 exch FontMatrix transform exch pop\n"
"/UnderlineThickness exch def\n"
"} if\n"

"} if\n"
"currentdict\n"
"end\n"
"} bind def\n");

   prio_printf("/txt ISO88591Encoding /%s findfont reencode definefont pop\n",
	       fontname);
   prio_printf("/lab ISO88591Encoding /%s findfont reencode definefont pop\n",
	       fontname_labels);

   /* F switches to text font; N - font for labels (Names) */
   prio_printf("/F {/txt findfont %d scalefont setfont} def\n", (int)fontsize);
   prio_printf("/N {/lab findfont %d scalefont setfont} def\n",
	       (int)fontsize_labels);
   prio_print("F\n");

   /* C<digit> changes colour */
   /* FIXME: read from ini */
   {
      int i;
      for (i = 0; i < sizeof(colour) / sizeof(colour[0]); ++i) {
	 prio_printf("/C%d {stroke %.3f %.3f %.3f setrgbcolor} def\n", i,
		     (double)(colour[i] & 0xff0000) / 0xff0000,
		     (double)(colour[i] & 0xff00) / 0xff00,
		     (double)(colour[i] & 0xff) / 0xff);
      }
   }
   prio_print("C0\n");

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

   prio_print("%%EndComments\n"
	      "%%EndProlog\n");

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

   x = (pg - 1) % pagesX;
   y = pagesY - 1 - ((pg - 1) / pagesX);

   clip.x_min = (long)x * xpPageWidth;
   clip.y_min = (long)y * ypPageDepth;
   clip.x_max = clip.x_min + xpPageWidth; /* dm/pcl/ps had -1; */
   clip.y_max = clip.y_min + ypPageDepth; /* dm/pcl/ps had -1; */

   cur_pass = pass;
   if (pass == -1) return;

#ifdef HPGL
   x_org = clip.x_min;
   y_org = clip.y_min - ypFooter;
   if (fOriginInCentre) {
      x_org += xpPageWidth / 2;
      y_org += ypPageDepth / 2;
   }
#else
   /* page number - first value can be a non-numeric string */
   prio_printf("%%%%Page: %d %d\n", pg, pg);
   /* set the line width */
   prio_printf("%.1f setlinewidth\n", LineWidth);
   /* shift image up page about 10mm to allow for page footer */
   prio_printf("%ld %ld translate\ngsave\n",
	       (long)(MarginLeft * POINTS_PER_MM),
	       (long)((MarginBottom + 10.0) * POINTS_PER_MM));
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
   current_colourcode = -1;
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
static char *
#ifdef HPGL
hpgl_Init(FILE **fh_list, const char *pth, const char *out_fnm,
	  double *pscX, double *pscY, bool fCalibrate)
#else
ps_Init(FILE **fh_list, const char *pth, const char *out_fnm,
	double *pscX, double *pscY, bool fCalibrate)
#endif
{
   char *fnm_prn;
   static const char *vars[] = {
      "like",
      "output",
#if (OS==MSDOS)
      "output_msdos",
#elif (OS==UNIX)
      "output_unix",
#elif (OS==RISCOS)
      "output_riscos",
#elif (OS==WIN32)
      "output_mswindows",
#else
#error unknown operating system
#endif
#ifdef HPGL
      "mm_across_page",
      "mm_down_page",
      "origin_in_centre",
#else
      "font",
      "mm_left_margin",
      "mm_right_margin",
      "mm_bottom_margin",
      "mm_top_margin",
      "font_size",
      "line_width",
      "font_labels",
      "font_size_labels",
      "colour_text",
      "colour_labels",
      "colour_frame",
      "colour_legs",
      "colour_crosses",
      "colour_surface_legs",
#endif
      NULL
   };
   char **vals;

   /* suppress unused argument warnings */
   fCalibrate = fCalibrate;
   pth = pth;

#ifdef HPGL
   vals = ini_read_hier(fh_list, "hpgl", vars);
#else
   vals = ini_read_hier(fh_list, "ps", vars);
#endif

   if (out_fnm)
      fnm_prn = osstrdup(out_fnm);
   else if (vals[2])
      fnm_prn = vals[2];
   else
      fnm_prn = as_string(vars[1], vals[1]);

#ifdef HPGL
   PaperWidth = as_double(vars[3], vals[3], 1, FLT_MAX);
   PaperDepth = as_double(vars[4], vals[4], 11, FLT_MAX);
   fOriginInCentre = as_bool(vars[5], vals[5]);
   PaperDepth -= 10; /* Allow 10mm for footer */
   osfree(vals);

   *pscX = *pscY = (double)HPGL_UNITS_PER_MM;
   xpPageWidth = (long)(HPGL_UNITS_PER_MM * (double)PaperWidth);
   ypPageDepth = (long)(HPGL_UNITS_PER_MM * (double)PaperDepth);

   ypFooter = (int)(HPGL_UNITS_PER_MM * 10.0);
#else
   /* name and size of font to use for text */
   fontname = as_string(vars[3], vals[3]);
   fontsize = as_int(vars[8], vals[8], 1, INT_MAX);

   MarginLeft = as_double(vars[4], vals[4], 0, FLT_MAX);
   MarginRight = as_double(vars[5], vals[5], 0, FLT_MAX);
   MarginBottom = as_double(vars[6], vals[6], 0, FLT_MAX);
   MarginTop = as_double(vars[7], vals[7], 0, FLT_MAX);

   LineWidth = as_double(vars[9], vals[9], 0, INT_MAX);

   /* name and size of font to use for station labels (default to text font) */
   fontname_labels = fontname;
   if (vals[10]) fontname_labels = vals[10];
   fontsize_labels = fontsize;
   if (vals[11]) fontsize_labels = as_int(vars[11], vals[11], 1, INT_MAX);
   {
      int i;
      for (i = 0; i < sizeof(colour) / sizeof(colour[0]); ++i) {
	 if (vals[12 + i]) colour[i] = as_colour(vars[12 + i], vals[12 + i]);
      }
   }

   osfree(vals);

   PaperWidth = MarginRight - MarginLeft;
   PaperDepth = MarginTop - MarginBottom - 10; /* Allow 10mm for footer */

   *pscX = *pscY = POINTS_PER_MM;
   xpPageWidth = (long)(POINTS_PER_MM * PaperWidth);
   ypPageDepth = (long)(POINTS_PER_MM * PaperDepth);
#endif

   prio_open(fnm_prn);
   return fnm_prn;
}

#ifndef HPGL
static int
CharsetLatin1(void)
{
   return CHARSET_ISO_8859_1;
}

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
   if (cur_pass != -1) {
      prio_printf("PU%ld,%ld;", x - x_org, y - y_org);
   } else {
      x_t = x - clip.x_min;
      y_t = y - clip.y_min;
   }
}

static void
hpgl_DrawTo(long x, long y)
{
   if (cur_pass != -1) {
      prio_printf("PD%ld,%ld;", x - x_org, y - y_org);
   } else {
      long x_p = x_t, y_p = y_t;
      x_t = x - clip.x_min;
      y_t = y - clip.y_min;
      if (check_intersection(x_p, y_p)) fBlankPage = fFalse;
   }
}

#define CS HPGL_CROSS_SIZE
#define CS2 (2 * HPGL_CROSS_SIZE)
static void
hpgl_DrawCross(long x, long y)
{
   if (cur_pass != -1) {
      hpgl_MoveTo(x, y);
      /* SM plots a symbol at each point, but it isn't very convenient here   */
      /* We can write PDPR%d,%dPR%d,%d... but the HP7475A manual doesn't say  */
      /* clearly if this will work on older plotters (such as the HP9872)     */
      prio_printf("PD;PR%d,%d;PR%d,%d;PU%d,0;PD%d,%d;PU%d,%d;PA;",
		  CS, CS,  -CS2, -CS2,  CS2, /*0,*/  -CS2, CS2,  CS, -CS);
      if (fNewLines) prio_putc('\n');
   } else {
      if ((x + CS > clip.x_min && x - CS < clip.x_max) ||
	  (y + CS > clip.y_min && y - CS < clip.y_max)) {
	 fBlankPage = fFalse;
      }
   }
}
#undef CS
#undef CS2

static void
hpgl_WriteString(const char *s)
{
   if (cur_pass != -1) {
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
   } else {
#define CHAR_SIZE (6 * HPGL_UNITS_PER_MM) /* Guesstimate of character size */
      if ((y_t + CHAR_SIZE > clip.y_min && y_t < clip.y_max) ||
	  (x_t < clip.x_max && x_t + strlen(s) * CHAR_SIZE > clip.x_min)) {
	 fBlankPage = fFalse;
      }
   }
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
