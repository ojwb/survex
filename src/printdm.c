/* > printdm.c & printpcl.c */

/* Device dependent part of Survex Dot-matrix/PCL printer driver */
/* Bitmap routines for Survex Dot-matrix and Inkjet printer drivers */
/* Copyright (C) 1993-2001 Olly Betts
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
#include "debug.h" /* for BUG and ASSERT */
#include "prcore.h"
#include "ini.h"

#define FNMFONT "pfont.bit"

static int Pre(int pagesToPrint, const char *title);
static void Post(void);

#ifdef XBM
static const char *xbm_Name(void);
static void xbm_NewPage(int pg, int pass, int pagesX, int pagesY);
static void xbm_Init(FILE **fh_list, const char *pth, float *pscX, float *pscY);
static void xbm_ShowPage(const char *szPageDetails);

static int xbm_page_no = 0;

/*device xbm = {*/
device printer = {
   xbm_Name,
   xbm_Init,
   Pre,
   xbm_NewPage,
   MoveTo,
   DrawTo,
   DrawCross,
   NULL, /* SetFont */
   WriteString,
   NULL, /* DrawCircle */
   xbm_ShowPage,
   Post,
   prio_close
};
#elif defined(PCL)
static bool fPCL = fTrue;
static bool fPCLHTab = fTrue;
static bool fPCLVTab = fTrue;

static const char *pcl_Name(void);
static void pcl_NewPage(int pg, int pass, int pagesX, int pagesY);
static void pcl_Init(FILE **fh_list, const char *pth, float *pscX, float *pscY);
static void pcl_ShowPage(const char *szPageDetails);

/*device pcl = {*/
device printer = {
   pcl_Name,
   pcl_Init,
   Pre,
   pcl_NewPage,
   MoveTo,
   DrawTo,
   DrawCross,
   NULL, /* SetFont */
   WriteString,
   NULL, /* DrawCircle */
   pcl_ShowPage,
   Post,
   prio_close
};
#else
static bool fPCL = fFalse;
static bool fIBM = fFalse;

static const char *dm_Name(void);
static void dm_NewPage(int pg, int pass, int pagesX, int pagesY);
static void dm_Init(FILE **fh_list, const char *pth, float *pscX, float *pscY);
static void dm_ShowPage(const char *szPageDetails);

/*device dm = {*/
device printer = {
   dm_Name,
   dm_Init,
   Pre,
   dm_NewPage,
   MoveTo,
   DrawTo,
   DrawCross,
   NULL, /* SetFont */
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

static int SIZEOFGRAPH_T; /* for DM */

static int dpi; /* for PCL */

#ifndef PCL
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
PlotDotDM(long x, long y)
{
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
}

static void
PlotDotPCL(long x, long y)
{
   if (x < clip.x_min || x > clip.x_max || y < clip.y_min || y > clip.y_max)
      return;
   x -= clip.x_min;
   bitmap[y - clip.y_min][x >> 3] |= 128 >> (x & 7);
}

#ifdef XBM
static void
PlotDotXBM(long x, long y)
{
   if (x < clip.x_min || x > clip.x_max || y < clip.y_min || y > clip.y_max)
      return;
   bitmap[y - clip.y_min][x - clip.x_min] = '*';
}

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
      /* Don't count alignment marks, but do count borders */
      fBlankPage = fNoBorder
	 || (x > 0 && y > 0 && x < pagesX - 1 && y < pagesY - 1);
      PlotDot = PlotDotTest; /* setup function ptr */
   } else {
      clip.y_max -= pass * ((long)ylPassDepth * ypLineDepth);
      if (pass == passMax - 1) {
	 /* remaining lines */
         ylThisPassDepth = (ylPageDepth - 1) % ylPassDepth + 1;
      } else {
         long y_min_new = clip.y_max - (long)ylPassDepth * ypLineDepth + 1;
         ASSERT(y_min_new >= clip.y_min);
         clip.y_min = y_min_new;
         ylThisPassDepth = ylPassDepth;
      }

#ifdef XBM
      /* Don't 0 all rows on last pass */
      for (yi = 0; yi < ylThisPassDepth; yi++) memset(bitmap[yi], '.', lenLine);

      PlotDot = PlotDotXBM;
#else
      /* Don't 0 all rows on last pass */
      for (yi = 0; yi < ylThisPassDepth; yi++) memset(bitmap[yi], 0, lenLine);

      PlotDot = (fPCL ? PlotDotPCL : PlotDotDM); /* setup function ptr */
#endif

      drawticks(edge, 9, x, y);
   }
#ifdef XBM
   xbm_page_no = pg;
#endif
}

static void
#ifdef XBM
xbm_ShowPage(const char *szPageDetails)
#elif defined(PCL)
pcl_ShowPage(const char *szPageDetails)
#else
dm_ShowPage(const char *szPageDetails)
#endif
{
   int y, last;
#ifdef XBM
   if (passStore == 0)
      prio_printf("/* XPM */\n"
		  "static char *page%d[] = {\n"
		  "/* width height num_colors chars_per_pixel */\n"
		  "\"%d %d 1 1\",\n"
		  "/* colors */\n"
		  "\". c #ffffff\",\n"
		  "\"* c #000000\",\n"
		  "/* pixels */\n",
		  xbm_page_no, xpPageWidth, ypPageDepth);


   for (y = ypPageDepth - 1; y >= 0; y--) {
      prio_putc('\"');
      prio_putbuf(bitmap[y], xpPageWidth);
      prio_print("\",\n");
   }
#elif defined(PCL)
# define firstMin 7 /* length of horizontal offset ie  Esc * p <number> X */
   int first;
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
static void
#ifdef XBM
xbm_Init(FILE **fh_list, const char *pth, float *pscX, float *pscY)
#elif defined(PCL)
pcl_Init(FILE **fh_list, const char *pth, float *pscX, float *pscY)
#else
dm_Init(FILE **fh_list, const char *pth, float *pscX, float *pscY)
#endif
{
   char *fnmPrn;
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
#ifdef XBM
      "width",
      "height",
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

#ifdef XBM
   vals = ini_read_hier(fh_list, "xbm", vars);
#elif defined(PCL)
   vals = ini_read_hier(fh_list, "pcl", vars);
#else
   vals = ini_read_hier(fh_list, "dm", vars);
/*   vals = ini_read_hier(fh_list, "bj", vars);*/
#endif

   if (vals[2])
      fnmPrn = vals[2];
   else
      fnmPrn = as_string(vars[1], vals[1]);

#ifdef XBM
   xpPageWidth = as_int(vars[3], vals[3], 1, INT_MAX);
   ypPageDepth = as_int(vars[4], vals[4], 1, INT_MAX);
   ylPageDepth = ypPageDepth;
   ypLineDepth = 1;
   PaperWidth = xpPageWidth;
   PaperDepth = ypPageDepth;
   *pscX = *pscY = 1;
   osfree(vals);
#elif defined(PCL)
   dpi = as_int(vars[3], vals[3], 1, INT_MAX);
   PaperWidth = as_float(vars[4], vals[4], 1, FLT_MAX);
   PaperDepth = as_float(vars[5], vals[5], 11, FLT_MAX);
   PaperDepth -= 10; /* allow 10mm for footer */
   fPCLHTab = as_bool(vars[6], vals[6]);
   fPCLVTab = as_bool(vars[7], vals[7]);
   osfree(vals);

   *pscX = *pscY = (float)(dpi / MM_PER_INCH);
   xpPageWidth = (int)((dpi / MM_PER_INCH) * PaperWidth);
   ylPageDepth = ypPageDepth = (int)((dpi / MM_PER_INCH) * PaperDepth);
   ypLineDepth = 1;
#else
   xpPageWidth = as_int(vars[3], vals[3], 1, INT_MAX);
   ylPageDepth = as_int(vars[4], vals[4], 4, INT_MAX);
   ylPageDepth -= 3; /* allow for footer */
   ypLineDepth = as_int(vars[5], vals[5], 1, INT_MAX);
   PaperWidth = as_float(vars[6], vals[6], 1, FLT_MAX);
   PaperDepth = as_float(vars[7], vals[7], 1, FLT_MAX);

   ypPageDepth = ylPageDepth * ypLineDepth;
   *pscX = xpPageWidth / PaperWidth; /* xp per mm */
   *pscY = ypPageDepth / PaperDepth; /* yp per mm */

   /* Work out # bytes required to hold one column */
   SIZEOFGRAPH_T = (ypLineDepth + 7) >> 3;

   prn.lnsp.str = vals[8];      /* $1b3$18   */
   prn.lnsp.len = as_escstring(vars[8], prn.lnsp.str);
   prn.grph.str = vals[9];      /* $1bL      */
   prn.grph.len = as_escstring(vars[9], prn.grph.str);
   prn.grph2.str = vals[10];    /*           */
   prn.grph2.len = as_escstring(vars[10], prn.grph2.str);
   prn.fnt_big.str = vals[11];  /* $12$1bW1  */
   prn.fnt_big.len = as_escstring(vars[11], prn.fnt_big.str);
   prn.fnt_sml.str = vals[12];  /* $1bW0$0f  */
   prn.fnt_sml.len = as_escstring(vars[12], prn.fnt_sml.str);
   prn.fmfd.str = vals[13];     /* $0c       */
   prn.fmfd.len = as_escstring(vars[13], prn.fmfd.str);
   prn.rst.str = vals[14];      /* $1b2      */
   prn.rst.len = as_escstring(vars[14], prn.rst.str);
   prn.eol.str = vals[15];      /* $0d       */
   prn.eol.len = as_escstring(vars[15], prn.eol.str);
   fIBM = 0; /* default to Epsom */
   if (vals[16]) fIBM = as_bool(vars[16], vals[16]);
   osfree(vals);
#endif

#ifdef XBM
   read_font(pth, FNMFONT, 25, 25);
#else
   if (fPCL) {
      read_font(pth, FNMFONT, dpi, dpi);
   } else {
      /* Note: (*pscX) and (*pscY) are in device_dots / mm */
      read_font(pth, FNMFONT, (int)((*pscX) * MM_PER_INCH),
	       (int)((*pscY) * MM_PER_INCH));
   }
#endif
   prio_open(fnmPrn);
   osfree(fnmPrn);
}

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
#else
   lenLine = ( fPCL ? ((xpPageWidth + 7) >> 3) : xpPageWidth * SIZEOFGRAPH_T);
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
