/* printdm.c & printpcl.c */

/* Device dependent part of Survex Dot-matrix/PCL printer driver */
/* Bitmap routines for Survex Dot-matrix and Inkjet printer drivers */
/* Copyright (C) 1993-2002,2005 Olly Betts
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
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
#include "prbitmap.h"
#include "prio.h"
#include "filelist.h"
#include "debug.h" /* for BUG and SVX_ASSERT */
#include "prcore.h"
#include "ini.h"

#if !defined(PCL) && !defined(XBM)
# define DM
#endif

static int Pre(int pagesToPrint, const char *title);
static void Post(void);
#if defined(PCL) || defined(XBM)
static int CharsetLatin1(void);
#endif

#ifdef XBM
static const char *xbm_Name(void);
static void xbm_NewPage(int pg, int pass, int pagesX, int pagesY);
static char *xbm_Init(FILE **fh_list, const char *pth, const char *out_fnm,
		     double *pscX, double *pscY, bool fCalibrate);
static void xbm_SetColour(int colourcode);
static void xbm_ShowPage(const char *szPageDetails);

static int xbm_page_no = 0;

static char xbm_char = '0';

static unsigned long colour[6];

/*device xbm = {*/
device printer = {
   0,
   xbm_Name,
   xbm_Init,
   CharsetLatin1,
   Pre,
   xbm_NewPage,
   MoveTo,
   DrawTo,
   DrawCross,
   SetFont,
   xbm_SetColour,
   WriteString,
   NULL, /* DrawCircle */
   xbm_ShowPage,
   Post,
   prio_close
};
#elif defined(PCL)
static bool fPCLHTab = fTrue;
static bool fPCLVTab = fTrue;

static const char *pcl_Name(void);
static void pcl_NewPage(int pg, int pass, int pagesX, int pagesY);
static char *pcl_Init(FILE **fh_list, const char *pth, const char *out_fnm,
		     double *pscX, double *pscY, bool fCalibrate);
static void pcl_ShowPage(const char *szPageDetails);

/*device pcl = {*/
device printer = {
   0,
   pcl_Name,
   pcl_Init,
   CharsetLatin1,
   Pre,
   pcl_NewPage,
   MoveTo,
   DrawTo,
   DrawCross,
   SetFont,
   NULL, /* SetColour */
   WriteString,
   NULL, /* DrawCircle */
   pcl_ShowPage,
   Post,
   prio_close
};
#else
static bool fIBM = fFalse;

static const char *dm_Name(void);
static void dm_NewPage(int pg, int pass, int pagesX, int pagesY);
static char *dm_Init(FILE **fh_list, const char *pth, const char *out_fnm,
		    double *pscX, double *pscY, bool fCalibrate);
static void dm_ShowPage(const char *szPageDetails);

/*device dm = {*/
device printer = {
   PR_FLAG_CALIBRATE,
   dm_Name,
   dm_Init,
   NULL, /*CharsetLatin1,*/
   Pre,
   dm_NewPage,
   MoveTo,
   DrawTo,
   DrawCross,
   SetFont,
   NULL, /* SetColour */
   WriteString,
   NULL, /* DrawCircle */
   dm_ShowPage,
   Post,
   prio_close
};
#endif

static OSSIZE_T lenLine; /* number of chars in each line */

static border clip;

static int ylPageDepth, ylPassDepth, ylThisPassDepth, ypLineDepth;
static int passMax, passStore;

#ifdef PCL
static int dpi;
#elif defined(DM)
static int SIZEOFGRAPH_T;

typedef struct {
   pstr lnsp, grph, fnt_big;
   pstr fnt_sml, fmfd, rst;
   pstr eol, grph2;
} dmprinter;

static dmprinter prn;
#endif

static unsigned char **bitmap;

static int xpPageWidth, ypPageDepth;

static void
PlotDotTest(long x, long y)
{
   if (!(x < clip.x_min || x > clip.x_max || y < clip.y_min || y > clip.y_max))
      fBlankPage = fFalse;
}

static void
PlotDotReal(long x, long y)
{
#ifdef DM
   int v;
   if (x < clip.x_min || x > clip.x_max || y < clip.y_min || y > clip.y_max)
      return;
   x -= clip.x_min;
   y -= clip.y_min;
   /* Shift up to fit snugly at high end */
   v = (int)(y % ypLineDepth) + ((-ypLineDepth) & 7);
   y /= ypLineDepth;
   x = (int)(x * SIZEOFGRAPH_T + (SIZEOFGRAPH_T - (v >> 3) - 1));
   bitmap[y][x] |= 1 << (v & 7);
#elif defined(PCL)
   if (x < clip.x_min || x > clip.x_max || y < clip.y_min || y > clip.y_max)
      return;
   x -= clip.x_min;
   bitmap[y - clip.y_min][x >> 3] |= 128 >> (x & 7);
#else
   if (x < clip.x_min || x > clip.x_max || y < clip.y_min || y > clip.y_max)
      return;
   bitmap[y - clip.y_min][x - clip.x_min] = xbm_char;
#endif
}

#ifdef XBM
static const char *
xbm_Name(void)
{
   return "XBM Printer";
}
#elif defined(PCL)
static const char *
pcl_Name(void)
{
   return "HP PCL Printer";
}
#else
static const char *
dm_Name(void)
{
   return "Dot-Matrix Printer";
}
#endif

/* pass = -1 => check if this page is blank
 * otherwise pass goes from 0 to (passMax-1)
 */
static void
#ifdef XBM
xbm_NewPage(int pg, int pass, int pagesX, int pagesY)
#elif defined(PCL)
pcl_NewPage(int pg, int pass, int pagesX, int pagesY)
#else
dm_NewPage(int pg, int pass, int pagesX, int pagesY)
#endif
{
   int x, y, yi;
   border edge;

   x = (pg - 1) % pagesX;
   y = pagesY - 1 - ((pg - 1) / pagesX);

   edge.x_min = (long)x * xpPageWidth;
   edge.y_min = (long)y * ypPageDepth;
   /* need to round down clip rectangle for bitmap devices */
   edge.x_max = edge.x_min + xpPageWidth - 1;
   edge.y_max = edge.y_min + ypPageDepth - 1;

   clip = edge;
   passStore = pass;

   if (pass == -1) {
      PlotDot = PlotDotTest; /* setup function ptr */
   } else {
      clip.y_max -= pass * ((long)ylPassDepth * ypLineDepth);
      if (pass == passMax - 1) {
	 /* remaining lines */
	 ylThisPassDepth = (ylPageDepth - 1) % ylPassDepth + 1;
      } else {
	 long y_min_new = clip.y_max - (long)ylPassDepth * ypLineDepth + 1;
	 SVX_ASSERT(y_min_new >= clip.y_min);
	 clip.y_min = y_min_new;
	 ylThisPassDepth = ylPassDepth;
      }

#ifdef XBM
      /* Don't 0 all rows on last pass */
      for (yi = 0; yi < ylThisPassDepth; yi++) memset(bitmap[yi], '.', lenLine);
#else
      /* Don't 0 all rows on last pass */
      for (yi = 0; yi < ylThisPassDepth; yi++) memset(bitmap[yi], 0, lenLine);
#endif
      PlotDot = PlotDotReal;

      drawticks(edge, 9, x, y);
   }
#ifdef XBM
   xbm_page_no = pg;
#endif
}

#ifdef XBM
static void
xbm_SetColour(int colourcode)
{
   xbm_char = (char)('0' + colourcode);
}
#endif

static void
#ifdef XBM
xbm_ShowPage(const char *szPageDetails)
#elif defined(PCL)
pcl_ShowPage(const char *szPageDetails)
#else
dm_ShowPage(const char *szPageDetails)
#endif
{
   int y;
#ifdef XBM
   if (passStore == 0) {
      size_t i;
      prio_printf("/* XPM */\n"
		  "/* %s */\n"
		  "static char *page%d[] = {\n"
		  "/* width height num_colors chars_per_pixel */\n"
		  "\"%d %d %d 1\",\n"
		  "/* colors */\n"
		  "\". c #ffffff\",\n",
		  szPageDetails, xbm_page_no, xpPageWidth, ypPageDepth,
		  (int)(sizeof(colour) / sizeof(colour[0]) + 1));
      for (i = 0; i < sizeof(colour) / sizeof(colour[0]); ++i) {
	 prio_printf("\"%d c #%06x\",\n", i, colour[i]);
      }
      prio_print("/* pixels */\n");
   }

   for (y = ypPageDepth - 1; y >= 0; y--) {
      prio_putc('\"');
      prio_putbuf(bitmap[y], xpPageWidth);
      prio_print("\",\n");
   }
#elif defined(PCL)
# define firstMin 7 /* length of horizontal offset ie  Esc * p <number> X */
   int first, last;
   static int cEmpties = 0; /* static so we can store them up between passes */

   if (passStore == 0) prio_printf("\x1b*t%dR", dpi);

   for (y = ylThisPassDepth - 1; y >= 0; y--) {
      /* Scan in from right end to last used byte, then stop */
      last = ((xpPageWidth + 7) >> 3) - 1;
      while (last >= 0 && !bitmap[y][last]) last--;

      first = 0;
      if (fPCLHTab) {
	 /* Check there are enough bytes that an htab can save space */
	 if (last > firstMin) {
	    /* Scan in from left end to first used byte, then stop */
	    while (!bitmap[y][first]) first++;

	    /* firstMin is the threshold above which a horiz tab saves bytes */
	    if (first < firstMin) first = 0;
	 }

	 if (first) prio_printf("\x1b*b%dX", first * 8);
      }

      /* need blank lines (last==-1) for PCL dump ... (sigh) */
      if (fPCLVTab && last == -1) {
	 cEmpties++; /* line is completely blank */
      } else {
	 int len;
	 if (cEmpties) {
	    /* if we've just had some blank lines, do a vertical offset */
	    prio_printf("\x1b*b%dY", cEmpties);
	    cEmpties = 0;
	 }
	 /* don't trust the printer to do a 0 byte line */
	 len = max(1, last + 1 - first);
	 prio_printf("\x1b*b%dW", len);
	 prio_putbuf(bitmap[y] + first, len);
      }
   }
   if (cEmpties && passStore == passMax - 1) {
      /* finished, so vertical offset any remaining blank lines */
      prio_printf("\x1b*b%dY", cEmpties);
      cEmpties = 0;
   }
#else
   int last;
   if (passStore == 0) {
      prio_putpstr(&prn.rst);
      prio_putpstr(&prn.lnsp);
   }
   for (y = ylThisPassDepth - 1; y >= 0; y--) {
      for (last = xpPageWidth * SIZEOFGRAPH_T - 1; last >= 0; last--)
	 if (bitmap[y][last]) break;
      /* Scan in from right hand end to first used 'byte', then stop */
      if (last >= 0) {
	 int len;
	 last = last / SIZEOFGRAPH_T + 1;
	 len = last * SIZEOFGRAPH_T;
	 /* IBM Proprinter style codes pass the length differently */
	 if (fIBM) last = len + 1;
	 prio_putpstr(&prn.grph);
	 prio_putc(last & 255);
	 prio_putc(last >> 8);
	 prio_putpstr(&prn.grph2);
	 prio_putbuf(bitmap[y], len);
      }
      prio_putpstr(&prn.eol);
   }
#endif
   putchar(' ');
   if (passStore == passMax - 1) {
#ifdef XBM
      prio_print("};\n");
#elif defined(PCL)
      prio_print("\x1b*rB\n\n"); /* End graphics */
      prio_print("\x1b(s0p20h12v0s3t2Q"); /* select a narrow-ish font */
      prio_print("\x1b(0N"); /* iso-8859-1 encoding */
      prio_print(szPageDetails);
      prio_putc('\x0c');
#else
      prio_putpstr(&prn.fnt_sml);
      prio_putc('\n');
      prio_print(szPageDetails);
      prio_putpstr(&prn.fmfd);
      prio_putpstr(&prn.rst);
#endif
   }
}

/* Initialise DM/PCL printer routines */
static char *
#ifdef XBM
xbm_Init(FILE **fh_list, const char *pth, const char *out_fnm,
	 double *pscX, double *pscY, bool fCalibrate)
#elif defined(PCL)
pcl_Init(FILE **fh_list, const char *pth, const char *out_fnm,
	 double *pscX, double *pscY, bool fCalibrate)
#else
dm_Init(FILE **fh_list, const char *pth, const char *out_fnm,
	double *pscX, double *pscY, bool fCalibrate)
#endif
{
   char *fnm_prn, *fnm_font;
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
      "font_bitmap",
      "font_size_labels",
#ifdef XBM
      "width",
      "height",
      "colour_text",
      "colour_labels",
      "colour_frame",
      "colour_legs",
      "colour_crosses",
      "colour_surface_legs",
#elif defined(PCL)
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

   fCalibrate = fCalibrate; /* suppress unused argument warning */

#ifdef XBM
   vals = ini_read_hier(fh_list, "xbm", vars);
#elif defined(PCL)
   vals = ini_read_hier(fh_list, "pcl", vars);
#else
   vals = ini_read_hier(fh_list, "dm", vars);
/*   vals = ini_read_hier(fh_list, "bj", vars);*/
#endif

   if (out_fnm)
      fnm_prn = osstrdup(out_fnm);
   else if (vals[2])
      fnm_prn = vals[2];
   else
      fnm_prn = as_string(vars[1], vals[1]);

   if (vals[3]) {
      fnm_font = add_ext(vals[3], "bit");
      osfree(vals[3]);
   } else {
      fnm_font = add_ext("default", "bit");
   }
   fontsize_labels = as_int(vars[4], vals[4], 1, INT_MAX);
#ifdef XBM
   xpPageWidth = as_int(vars[5], vals[5], 1, INT_MAX);
   ypPageDepth = as_int(vars[6], vals[6], 1, INT_MAX);
   ylPageDepth = ypPageDepth;
   ypLineDepth = 1;
   PaperWidth = xpPageWidth;
   PaperDepth = ypPageDepth;
   *pscX = *pscY = 1;
   {
      int i;
      for (i = 0; i < 5; ++i) {
	 if (vals[7 + i]) colour[i] = as_colour(vars[7 + i], vals[7 + i]);
      }
   }
   osfree(vals);
#elif defined(PCL)
   dpi = as_int(vars[5], vals[5], 1, INT_MAX);
   PaperWidth = as_double(vars[6], vals[6], 1, DBL_MAX);
   PaperDepth = as_double(vars[7], vals[7], 11, DBL_MAX);
   PaperDepth -= 10; /* allow 10mm for footer */
   fPCLHTab = as_bool(vars[8], vals[8]);
   fPCLVTab = as_bool(vars[9], vals[9]);
   osfree(vals);

   *pscX = *pscY = (double)dpi / MM_PER_INCH;
   xpPageWidth = (int)((dpi / MM_PER_INCH) * PaperWidth);
   ylPageDepth = ypPageDepth = (int)((dpi / MM_PER_INCH) * PaperDepth);
   ypLineDepth = 1;
#else
   xpPageWidth = as_int(vars[5], vals[5], 1, INT_MAX);
   ylPageDepth = as_int(vars[6], vals[6], 4, INT_MAX);
   ylPageDepth -= 3; /* allow for footer */
   ypLineDepth = as_int(vars[7], vals[7], 1, INT_MAX);

   if (!vals[8] || !vals[9]) {
      if (!fCalibrate)
	 fatalerror(/*You need to calibrate your printer - see the manual for details.*/103);

      /* Set temporary values, so a calibration plot can be produced */
      if (!vals[8]) PaperWidth = 200.0;
      if (!vals[9]) PaperWidth = 300.0;
   }

   if (vals[8]) PaperWidth = as_double(vars[8], vals[8], 1, DBL_MAX);
   if (vals[9]) PaperDepth = as_double(vars[9], vals[9], 1, DBL_MAX);

   ypPageDepth = ylPageDepth * ypLineDepth;
   *pscX = xpPageWidth / PaperWidth; /* xp per mm */
   *pscY = ypPageDepth / PaperDepth; /* yp per mm */

   /* Work out # bytes required to hold one column */
   SIZEOFGRAPH_T = (ypLineDepth + 7) >> 3;

   prn.lnsp.str = vals[10];
   prn.lnsp.len = as_escstring(vars[10], prn.lnsp.str);
   prn.grph.str = vals[11];
   prn.grph.len = as_escstring(vars[11], prn.grph.str);
   prn.grph2.str = vals[12];
   prn.grph2.len = as_escstring(vars[12], prn.grph2.str);
   prn.fnt_big.str = vals[13];
   prn.fnt_big.len = as_escstring(vars[13], prn.fnt_big.str);
   prn.fnt_sml.str = vals[14];
   prn.fnt_sml.len = as_escstring(vars[14], prn.fnt_sml.str);
   prn.fmfd.str = vals[15];
   prn.fmfd.len = as_escstring(vars[15], prn.fmfd.str);
   prn.rst.str = vals[16];
   prn.rst.len = as_escstring(vars[16], prn.rst.str);
   prn.eol.str = vals[17];
   prn.eol.len = as_escstring(vars[17], prn.eol.str);
   fIBM = 0; /* default to Epson */
   if (vals[18]) fIBM = as_bool(vars[18], vals[18]);
   osfree(vals);
#endif

#ifdef XBM
   read_font(pth, fnm_font, 25, 25);
#elif defined(PCL)
   read_font(pth, fnm_font, dpi, dpi);
#else
   /* Note: (*pscX) and (*pscY) are in device_dots / mm */
   read_font(pth, fnm_font, (int)((*pscX) * MM_PER_INCH),
	     (int)((*pscY) * MM_PER_INCH));
#endif
   osfree(fnm_font);
   prio_open(fnm_prn);
   return fnm_prn;
}

#if defined(PCL) || defined(XBM)
static int
CharsetLatin1(void)
{
   return CHARSET_ISO_8859_1;
}
#endif

static int
Pre(int pagesToPrint, const char *title)
{
   int y;

   pagesToPrint = pagesToPrint; /* shut-up warning */
   title = title; /* shut-up warning */

   passMax = 1;
   bitmap = osmalloc(ylPageDepth * ossizeof(void*));
#ifdef XBM
   lenLine = xpPageWidth;
#elif defined (PCL)
   lenLine = (xpPageWidth + 7) >> 3;
#else
   lenLine = xpPageWidth * SIZEOFGRAPH_T;
#endif
   bitmap[0] = osmalloc(lenLine);
   for (y = 1; y < ylPageDepth; y++) {
      bitmap[y] = xosmalloc(lenLine);
      if (!bitmap[y]) {
	 passMax = (int)ceil((double)ylPageDepth / y);
	 bitmap = osrealloc(bitmap, y * ossizeof(void*));
	 break;
      }
   }
   ylPassDepth = y;
   return passMax;
}

static void
Post(void)
{
   int y;
   for (y = 0; y < ylPassDepth; y++) osfree(bitmap[y]);
   osfree(bitmap);
}
