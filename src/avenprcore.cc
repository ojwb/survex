/* avenprcore.cc
 * Printer independent parts of Survex printer drivers
 * Copyright (C) 1993-2002,2004,2005,2006,2010,2011,2012,2013 Olly Betts
 * Copyright (C) 2004 Philip Underwood
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

/* FIXME provide more explanation when reporting errors in print.ini */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <float.h>

#include "mainfrm.h"

#include "useful.h"
#include "filename.h"
#include "message.h"
#include "filelist.h"
#include "img_hosted.h"
#include "avenprcore.h"
#include "debug.h"

#if defined __WXMSW__ || defined __WXMAC__
# include <wx/dcprint.h>
#else
# include <wx/dcps.h>
#endif

layout::layout(wxPageSetupDialogData* data)
	: show_mask(0),
	  SkipBlank(false), Border(true), Cutlines(true), Legend(true),
	  title(), datestamp(), Scale(0), rot(0), tilt(0),
	  view(PLAN), scX(1), scY(1), xMin(0), xMax(-1), yMin(0), yMax(-1),
	  pagesX(1), pagesY(1), pages(1), xOrg(0), yOrg(0), footer()
{
    // Create a temporary wxPrinterDC/wxPostScriptDC so we can get access to
    // the size of the printable area in mm to allow us to calculate how many
    // pages will be needed.
#if defined __WXMSW__ || defined __WXMAC__
    wxPrinterDC pdc(data->GetPrintData());
#else
    wxPostScriptDC pdc(data->GetPrintData());
#endif
    int width, depth;
    pdc.GetSizeMM(&width, &depth);
    width -= data->GetMarginBottomRight().x + data->GetMarginTopLeft().x;
    PaperWidth = width;
    depth -= data->GetMarginBottomRight().y + data->GetMarginTopLeft().y;
    // Allow for the 10mm footer.
    PaperDepth = depth - 10;
}

void
layout::pages_required() {
    double image_dx, image_dy;
    double image_centre_x, image_centre_y;
    double paper_centre_x, paper_centre_y;

    double allow = 21.0;
    if (Legend) allow += 30.0;
    double Sc = 1000 / Scale;
    image_dx = (xMax - xMin) * Sc;
    if (PaperWidth > 0.0) {
	pagesX = (int)ceil((image_dx + 19.0) / PaperWidth);
    } else {
	/* paperwidth not fixed (eg window or roll printer/plotter) */
	pagesX = 1;
	PaperWidth = image_dx + 19.0;
    }
    paper_centre_x = (pagesX * PaperWidth) / 2;
    image_centre_x = Sc * (xMax + xMin) / 2;
    xOrg = paper_centre_x - image_centre_x;

    image_dy = (yMax - yMin) * Sc;
    if (PaperDepth > 0.0) {
	pagesY = (int)ceil((image_dy + allow) / PaperDepth);
    } else {
	/* paperdepth not fixed (eg window or roll printer/plotter) */
	pagesY = 1;
	PaperDepth = image_dy + allow;
    }
    paper_centre_y = 20 + (pagesY * PaperDepth) / 2;
    image_centre_y = Sc * (yMax + yMin) / 2;
    yOrg = paper_centre_y - image_centre_y;

    pages = pagesX * pagesY;
}

static void setting_missing(const char *v)
{
   fatalerror(/*Parameter “%s” missing in printer configuration file*/85, v);
}

static void setting_bad_value(const char *v, const char *p)
{
   fatalerror(/*Parameter “%s” has invalid value “%s” in printer configuration file*/82,
	      v, p);
}

int
as_int(const char *v, char *p, int min_val, int max_val)
{
   long val;
   char *pEnd;
   if (!p) setting_missing(v);
   val = strtol(p, &pEnd, 10);
   if (pEnd == p || val < (long)min_val || val > (long)max_val)
      setting_bad_value(v, p);
   osfree(p);
   return (int)val;
}

/* Converts '0'-'9' to 0-9, 'A'-'F' to 10-15 and 'a'-'f' to 10-15.
 * Undefined on other values */
#define CHAR2HEX(C) (((C)+((C)>64?9:0))&15)

unsigned long
as_colour(const char *v, char *p)
{
   unsigned long val = 0xffffffff;
   if (!p) setting_missing(v);
   switch (tolower(*p)) {
      case '#': {
	 char *q = p + 1;
	 while (isxdigit((unsigned char)*q)) q++;
	 if (q - p == 4) {
	    val = CHAR2HEX(p[1]) * 0x110000;
	    val |= CHAR2HEX(p[2]) * 0x1100;
	    val |= CHAR2HEX(p[3]) * 0x11;
	 } else if (q - p == 7) {
	    val = ((CHAR2HEX(p[1]) << 4) | CHAR2HEX(p[2])) << 16;
	    val |= ((CHAR2HEX(p[3]) << 4) | CHAR2HEX(p[4])) << 8;
	    val |= (CHAR2HEX(p[5]) << 4) | CHAR2HEX(p[6]);
	 }
	 break;
      }
      case 'a':
	 if (strcasecmp(p, "aqua") == 0) val = 0x00fffful;
	 break;
      case 'b':
	 if (strcasecmp(p, "black") == 0) val = 0x000000ul;
	 else if (strcasecmp(p, "blue") == 0) val = 0x0000fful;
	 break;
      case 'f':
	 if (strcasecmp(p, "fuchsia") == 0) val = 0xff00fful;
	 break;
      case 'g':
	 if (strcasecmp(p, "gray") == 0) val = 0x808080ul;
	 else if (strcasecmp(p, "green") == 0) val = 0x008000ul;
	 break;
      case 'l':
	 if (strcasecmp(p, "lime") == 0) val = 0x00ff00ul;
	 break;
      case 'm':
	 if (strcasecmp(p, "maroon") == 0) val = 0x800000ul;
	 break;
      case 'n':
	 if (strcasecmp(p, "navy") == 0) val = 0x000080ul;
	 break;
      case 'o':
	 if (strcasecmp(p, "olive") == 0) val = 0x808000ul;
	 break;
      case 'p':
	 if (strcasecmp(p, "purple") == 0) val = 0x800080ul;
	 break;
      case 'r':
	 if (strcasecmp(p, "red") == 0) val = 0xff0000ul;
	 break;
      case 's':
	 if (strcasecmp(p, "silver") == 0) val = 0xc0c0c0ul;
	 break;
      case 't':
	 if (strcasecmp(p, "teal") == 0) val = 0x008080ul;
	 break;
      case 'w':
	 if (strcasecmp(p, "white") == 0) val = 0xfffffful;
	 break;
      case 'y':
	 if (strcasecmp(p, "yellow") == 0) val = 0xffff00ul;
	 break;
   }
   if (val == 0xffffffff) setting_bad_value(v, p);
   osfree(p);
   return val;
}

#define DEF_RATIO (1.0/(double)DEFAULT_SCALE)

/* pick a scale which will make it fit in the desired size */
void
layout::pick_scale(int x, int y)
{
   double Sc_x, Sc_y;
   /*    pagesY = ceil((image_dy+allow)/PaperDepth)
    * so (image_dy+allow)/PaperDepth <= pagesY < (image_dy+allow)/PaperDepth+1
    * so image_dy <= pagesY*PaperDepth-allow < image_dy+PaperDepth
    * and Sc = image_dy / (yMax-yMin)
    * so Sc <= (pagesY*PaperDepth-allow)/(yMax-yMin) < Sc+PaperDepth/(yMax-yMin)
    */
   Sc_x = Sc_y = DEF_RATIO;
   if (PaperWidth > 0.0 && xMax > xMin)
      Sc_x = (x * PaperWidth - 19.0) / (xMax - xMin);
   if (PaperDepth > 0.0 && yMax > yMin) {
      double allow = 21.0;
      if (Legend) allow += 30.0;
      Sc_y = (y * PaperDepth - allow) / (yMax - yMin);
   }

   Sc_x = min(Sc_x, Sc_y) * 0.99; /* shrink by 1% so we don't cock up */
#if 0 /* this picks a nice (in some sense) ratio, but is too stingy */
   double E = pow(10.0, floor(log10(Sc_x)));
   Sc_x = floor(Sc_x / E) * E;
#endif

   double Scale_exact = 1000.0 / Sc_x;

   /* trim to 2 s.f. (rounding up) */
   double w = pow(10.0, floor(log10(Scale_exact) - 1.0));
   Scale = ceil(Scale_exact / w) * w;
}

#if 0
bool fBlankPage = fFalse;

void print_all(MainFrm *m_parent, layout *l, device *pri) {
    int cPasses, pass;
    unsigned int cPagesPrinted;
    const char *msg166;
    int state;
    char *p;
    int old_charset;
    int page, pageLim;
    pageLim = l->pagesX*l->pagesY;
    PaperWidth = l->PaperWidth;
    PaperDepth = l->PaperDepth;
    /* if no explicit Alloc, default to one pass */
    cPasses = Pre(l->pages, l->title);

    /* note down so we can switch to printer charset */
    msg166 = msgPerm(/*Page %d of %d*/166);
    old_charset = select_charset(CHARSET_ISO_8859_1);

    l->footer = msgPerm(/*Survey “%s”   Page %d (of %d)   Processed on %s*/167);

    cPagesPrinted = 0;
    page = state = 0;
    p = l->szPages;
    while (1) {
	if (pageLim == 1) {
	    if (page == 0)
		page = 1;
	    else
		page = 0; /* we've already printed the only page */
	} else if (!*l->szPages) {
	    page++;
	    if (page > pageLim) page = 0; /* all pages printed */
	} else {
	    page = next_page(&state, &p, pageLim);
	}
	SVX_ASSERT(state >= 0); /* errors should have been caught above */
	if (page == 0) break;
	cPagesPrinted++;
	if (l->pages > 1) {
	    putchar('\r');
	    printf(msg166, (int)cPagesPrinted, l->pages);
	}
	/* don't skip the page with the info box on */
	if (l->SkipBlank && (int)page != (l->pagesY - 1) * l->pagesX + 1) {
	    pass = -1;
	    fBlankPage = fTrue;
	} else {
	    pass = 0;
	    fBlankPage = fFalse;
	}
	print_page(m_parent, l, page, pass, cPasses);
    }

    Quit();
    select_charset(old_charset);
}
#endif
