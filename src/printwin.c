/* > printwin.c */
/* Device dependent part of Survex Win32 driver */
/* Copyright (C) 1993-2000 Olly Betts
 * Copyright (C) 2001 Philip Underwood, Olly Betts
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
#include <windows.h>

static float MarginLeft, MarginRight, MarginTop, MarginBottom;
static int fontsize, fontsize_labels;
static float LineWidth;

static char *fontname, *fontname_labels;

#define WIN_TS 9 /* size of alignment 'ticks' on multipage printouts */
#define WIN_CROSS_SIZE 2 /* length of cross arms (in points!) */

#define fontname_symbol "Symbol"

static const char *win_Name(void);
static int win_Pre(int pagesToPrint, const char *title);
static void win_NewPage(int pg, int pass, int pagesX, int pagesY);
static void win_Init(FILE **fh_list, const char *pth, float *pscX, float *pscY);
static int  win_Charset(void);
static void win_MoveTo(long x, long y);
static void win_DrawTo(long x, long y);
static void win_DrawCross(long x, long y);
static void win_WriteString(const char *s);
static void win_DrawCircle(long x, long y, long r);
static void win_ShowPage(const char *szPageDetails);
static void win_Quit(void);

device printer = {
   win_Name,
   win_Init,
   win_Charset,
   win_Pre,
   win_NewPage,
   win_MoveTo,
   win_DrawTo,
   win_DrawCross,
   NULL,
   win_WriteString,
   win_DrawCircle,
   win_ShowPage,
   NULL, /* win_Post */
   win_Quit
};

static HDC pd; /* printer context */

static int midtextheight; /*height of text*/

static float scX, scY;

static border clip;

static long xpPageWidth, ypPageDepth;

static const char *
win_Name(void)
{
   return "Win32 Printer";
}

static long x_t, y_t;

static void
win_MoveTo(long x, long y)
{
   x_t = x - clip.x_min;
   y_t = clip.y_max - y;
   MoveToEx(pd, x_t, y_t, NULL);
}

static void
win_DrawTo(long x, long y)
{
   x_t = x - clip.x_min;
   y_t = clip.y_max - y;
   LineTo(pd, x_t, y_t);
}

static void
win_DrawCross(long x, long y)
{
   win_MoveTo(x - WIN_CROSS_SIZE, y);
   win_DrawTo(x + WIN_CROSS_SIZE, y);
   win_MoveTo(x, y - WIN_CROSS_SIZE);
   win_DrawTo(x, y + WIN_CROSS_SIZE);
   win_MoveTo(x, y);
}


static void
win_WriteString(const char *s)
{
   TextOut(pd, x_t, y_t - midtextheight, s, strlen(s));
}

static void
win_DrawCircle(long x, long y, long r)
{
   x_t = x - clip.x_min;
   y_t = clip.y_max - y;
   Ellipse(pd, x_t - r, y_t - r, x_t + r, y_t + r);
}

static int
win_Charset(void)
{
   return CHARSET_ISO_8859_1;
}

static int
win_Pre(int pagesToPrint, const char *title)
{
   PRINTDLGA psd = {0};
   DOCINFO info = {0};

   pagesToPrint = pagesToPrint;

   psd.lStructSize = 66;
   psd.hwndOwner = NULL;
   psd.hDevMode = NULL;
   psd.hDevNames = NULL; 
   psd.hDC = NULL;
   psd.Flags = PD_RETURNDC + PD_RETURNDEFAULT;
   psd.hInstance = NULL;
   PrintDlgA(&psd);
   pd = psd.hDC;
   info.lpszDocName = title;
   info.cbSize = sizeof(DOCINFO);
   StartDoc(pd, &info);
   return 1; /* only need 1 pass */
}

static void
win_NewPage(int pg, int pass, int pagesX, int pagesY)
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
   StartPage(pd);
   drawticks(clip, WIN_TS, x, y);
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
win_Init(FILE **fh_list, const char *pth, float *pscX, float *pscY)
{
   /* name and size of font to use for text */
   TEXTMETRIC temp;
   PRINTDLGA psd = {0};

   fh_list = fh_list;
   pth = pth;

   psd.lStructSize = 66;
   psd.hwndOwner = NULL;
   psd.hDevMode = NULL;
   psd.hDevNames = NULL; 
   psd.hDC = NULL;
   psd.Flags = PD_RETURNDC + PD_RETURNDEFAULT;
   psd.hInstance = NULL;
   PrintDlgA(&psd);
   fontsize = 12;
  
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
   GetTextMetrics(psd.hDC, &temp);
   midtextheight = temp.tmAscent;
   DeleteDC(psd.hDC);
  
   /* name and size of font to use for station labels (default to text font) */
   fontname_labels = fontname;
   fontsize_labels = fontsize;
}

static void
win_Quit(void)
{
   EndDoc(pd);
   DeleteDC(pd);
}
