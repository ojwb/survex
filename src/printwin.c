/* printwin.c */
/* Device dependent part of Survex Win32 driver */
/* Copyright (C) 1993-2002 Olly Betts
 * Copyright (C) 2001 Philip Underwood
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

#include "debug.h" /* for BUG and ASSERT */
#include "filelist.h"
#include "filename.h"
#include "ini.h"
#include "message.h"
#include "prcore.h"
#include "prio.h"
#include "useful.h"

#include <windows.h>

static double MarginLeft, MarginRight, MarginTop, MarginBottom;
static int fontsize, fontsize_labels;
static double LineWidth;

static char *fontname, *fontname_labels;

static const char *win_Name(void);
static int win_Pre(int pagesToPrint, const char *title);
static void win_NewPage(int pg, int pass, int pagesX, int pagesY);
static void win_Init(FILE **fh_list, const char *pth, const char *outfnm,
		     double *pscX, double *pscY, bool fCalibrate);
static int  win_Charset(void);
static void win_MoveTo(long x, long y);
static void win_DrawTo(long x, long y);
static void win_DrawCross(long x, long y);
static void win_SetFont(int fontcode);
static void win_WriteString(const char *s);
static void win_DrawCircle(long x, long y, long r);
static void win_ShowPage(const char *szPageDetails);
static void win_Quit(void);

device printer = {
   PR_FLAG_NOFILEOUTPUT, /* |PR_FLAG_NOINI - now read fontsize from ini file */
   win_Name,
   win_Init,
   win_Charset,
   win_Pre,
   win_NewPage,
   win_MoveTo,
   win_DrawTo,
   win_DrawCross,
   win_SetFont,
   win_WriteString,
   win_DrawCircle,
   win_ShowPage,
   NULL, /* win_Post */
   win_Quit
};

static HDC pd; /* printer context */

static TEXTMETRIC tm, tm_labels, tm_default; /* font info */

static double scX, scY;

static int cur_pass;

static border clip;

static long xpPageWidth, ypPageDepth;

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

   if (mask_t == 0) mask_t = mask_p;
   if (mask_t & U) {
      double v = (double)(y_p - ypPageDepth) / (y_p - y_t);
      return v >= 0 && v <= 1;
   }
   if (mask_t & D) {
      double v = (double)y_p / (y_p - y_t);
      return v >= 0 && v <= 1;
   }
   if (mask_t & R) {
      double v = (double)(x_p - xpPageWidth) / (x_p - x_t);
      return v >= 0 && v <= 1;
   }
   ASSERT(mask_t & L);
   {
      double v = (double)x_p / (x_p - x_t);
      return v >= 0 && v <= 1;
   }
#endif
#undef U
#undef D
#undef L
#undef R
}

static const char *
win_Name(void)
{
   return "Win32 Printer";
}

static void
win_MoveTo(long x, long y)
{
   x_t = x - clip.x_min;
   y_t = clip.y_max - y;
   if (cur_pass != -1) MoveToEx(pd, x_t, y_t, NULL);
}

static void
win_DrawTo(long x, long y)
{
   long x_p = x_t, y_p = y_t;
   x_t = x - clip.x_min;
   y_t = clip.y_max - y;
   if (cur_pass != -1) {
      LineTo(pd, x_t, y_t);
   } else {
      if (check_intersection(x_p, y_p)) fBlankPage = fFalse;
   }
}

#define POINTS_PER_INCH 72.0
#define POINTS_PER_MM (POINTS_PER_INCH / MM_PER_INCH)
#define WIN_CROSS_SIZE (2 * scX / POINTS_PER_MM)

static void
win_DrawCross(long x, long y)
{
   if (cur_pass != -1) {
      win_MoveTo(x - WIN_CROSS_SIZE, y - WIN_CROSS_SIZE);
      win_DrawTo(x + WIN_CROSS_SIZE, y + WIN_CROSS_SIZE);
      win_MoveTo(x + WIN_CROSS_SIZE, y - WIN_CROSS_SIZE);
      win_DrawTo(x - WIN_CROSS_SIZE, y + WIN_CROSS_SIZE);
      win_MoveTo(x, y);
   } else {
      if ((x + WIN_CROSS_SIZE > clip.x_min &&
	   x - WIN_CROSS_SIZE < clip.x_max) ||
	  (y + WIN_CROSS_SIZE > clip.y_min &&
	   y - WIN_CROSS_SIZE < clip.y_max)) {
	 fBlankPage = fFalse;
      }
   }
}

static HFONT font_labels, font_default, font_old;

static void
win_SetFont(int fontcode)
{
   switch (fontcode) {
      case PR_FONT_DEFAULT:
	 SelectObject(pd, font_default);
	 tm = tm_default;
	 break;
      case PR_FONT_LABELS:
	 SelectObject(pd, font_labels);
	 tm = tm_labels;
	 break;
      default:
	 BUG("unknown font code");
   }
}

static void
win_WriteString(const char *s)
{
   if (cur_pass != -1) {
      TextOut(pd, x_t, y_t - tm.tmAscent, s, strlen(s));
   } else {
      if ((y_t + tm.tmDescent > 0 &&
	   y_t - tm.tmAscent < clip.y_max - clip.y_min) ||
	  (x_t < clip.x_max - clip.x_min &&
	   x_t + strlen(s) * tm.tmAveCharWidth > 0)) {
	 fBlankPage = fFalse;
      }
   }
}

static void
win_DrawCircle(long x, long y, long r)
{
   /* Don't need to check in first-pass - circle is only used in title box */
   if (cur_pass != -1) {
      x_t = x - clip.x_min;
      y_t = clip.y_max - y;
      Ellipse(pd, x_t - r, y_t - r, x_t + r, y_t + r);
   }
}

static int
win_Charset(void)
{
   return CHARSET_ISO_8859_1;
}

static int
win_Pre(int pagesToPrint, const char *title)
{
   PRINTDLGA psd;
   DOCINFO info;

   pagesToPrint = pagesToPrint; /* suppress compiler warning */

   memset(&psd, 0, sizeof(PRINTDLGA));
   psd.lStructSize = 66;
   psd.Flags = PD_RETURNDC | PD_RETURNDEFAULT;

   if (!PrintDlgA(&psd)) exit(1);
   pd = psd.hDC;

   font_labels = CreateFont(-fontsize_labels, 0, 0, 0, FW_NORMAL, 0, 0, 0,
			    ANSI_CHARSET, OUT_DEFAULT_PRECIS,
			    CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			    FF_DONTCARE | DEFAULT_PITCH, "Arial");   
   font_default = CreateFont(-fontsize, 0, 0, 0, FW_NORMAL, 0, 0, 0,
		   	  ANSI_CHARSET, OUT_DEFAULT_PRECIS,
			  CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			  FF_DONTCARE | DEFAULT_PITCH, "Arial");   
   font_old = SelectObject(pd, font_labels);
   GetTextMetrics(pd, &tm_labels);
   SelectObject(pd, font_default);
   GetTextMetrics(pd, &tm_default);
   
   memset(&info, 0, sizeof(DOCINFO));
   info.cbSize = sizeof(DOCINFO);
   info.lpszDocName = title;

   StartDoc(pd, &info);
   return 1; /* only need 1 pass */
}

static void
win_NewPage(int pg, int pass, int pagesX, int pagesY)
{
   int x, y;

   x = (pg - 1) % pagesX;
   y = pagesY - 1 - ((pg - 1) / pagesX);

   clip.x_min = (long)x * xpPageWidth;
   clip.y_min = (long)y * ypPageDepth;
   clip.x_max = clip.x_min + xpPageWidth; /* dm/pcl/ps had -1; */
   clip.y_max = clip.y_min + ypPageDepth; /* dm/pcl/ps had -1; */

   cur_pass = pass;
   if (pass == -1) {
      /* Don't count alignment marks, but do count borders */
      fBlankPage = fNoBorder
	 || (x > 0 && y > 0 && x < pagesX - 1 && y < pagesY - 1);
      return;
   }

   StartPage(pd);
   drawticks(clip, 9 * scX / POINTS_PER_MM, x, y);
}

static void
win_ShowPage(const char *szPageDetails)
{
   win_MoveTo((long)(6 * scX) + clip.x_min, clip.y_min - (long)(7 * scY));
   win_WriteString(szPageDetails);
   EndPage(pd);
}

/* Initialise printer routines */
static void
win_Init(FILE **fh_list, const char *pth, const char *out_fnm,
	 double *pscX, double *pscY, bool fCalibrate)
{
   PRINTDLGA psd;
   static const char *vars[] = {
      "like",
      "font_size_labels",
      NULL
   };
   char **vals;

   fCalibrate = fCalibrate; /* suppress unused argument warning */
   out_fnm = out_fnm;
   pth = pth;

   vals = ini_read_hier(fh_list, "win", vars);

   fontsize_labels = as_int(vars[1], vals[1], 1, INT_MAX);
   fontsize = 10;

   memset(&psd, 0, sizeof(PRINTDLGA));
   psd.lStructSize = 66;
   psd.Flags = PD_RETURNDC | PD_RETURNDEFAULT;

   if (!PrintDlgA(&psd)) exit(1);

   PaperWidth = GetDeviceCaps(psd.hDC, HORZSIZE);
   PaperDepth = GetDeviceCaps(psd.hDC, VERTSIZE);
   xpPageWidth = GetDeviceCaps(psd.hDC, HORZRES);
   ypPageDepth = GetDeviceCaps(psd.hDC, VERTRES);
   MarginLeft = MarginBottom = 0;
   MarginRight = PaperWidth;
   MarginTop = PaperDepth;
   LineWidth = 0;
   scX = *pscX = xpPageWidth / PaperWidth;
   scY = *pscY = ypPageDepth / PaperDepth;
   xpPageWidth--;
   ypPageDepth = ypPageDepth - (int)(10 * *pscY);
   DeleteDC(psd.hDC);
}

static void
win_Quit(void)
{
   SelectObject(pd, font_old);
   DeleteObject(font_labels);
   DeleteObject(font_default);
   EndDoc(pd);
   DeleteDC(pd);
}
