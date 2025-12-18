/* avenprcore.cc
 * Printer independent parts of Survex printer drivers
 * Copyright (C) 1993-2002,2004,2005,2006,2010,2011,2012,2013,2014,2015,2016 Olly Betts
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

#include <config.h>

#include <algorithm>
#include <math.h>

#include "useful.h"
#include "avenprcore.h"

#if !(defined __WXMSW__ || defined __WXMAC__)
# include <wx/dcps.h>
#endif
#include <wx/print.h>

layout::layout(wxPageSetupDialogData* data)
{
    if (data) {
	// Printing.

	// Create a temporary wxPrinterDC/wxPostScriptDC so we can get access
	// to the size of the printable area in mm to allow us to calculate how
	// many pages will be needed.
	//
	// It may seem like data->GetPaperSize() would tell us this page size
	// without having to construct a temporary DC, but that just returns
	// (0, 0) for the size (most recent version checked was wxGTK 3.2.6).
#if defined __WXMSW__ || defined __WXMAC__
	wxPrinterDC pdc(data->GetPrintData());
#else
	wxPostScriptDC pdc(data->GetPrintData());
#endif
	// Calculate the size of the printable area in mm to allow us to work
	// out how many pages will be needed for a given scale.
	wxSize size = pdc.GetSizeMM();
	size.DecBy(data->GetMarginBottomRight());
	size.DecBy(data->GetMarginTopLeft());
	PaperWidth = size.x;
	// Allow for our footer.
	PaperDepth = size.y - FOOTER_HEIGHT_MM;
    } else {
	// Exporting.
	PaperWidth = PaperDepth = 0;
    }
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

#define DEF_RATIO (1.0/(double)DEFAULT_SCALE)

/* pick a scale which will make it fit in the desired size */
void
layout::pick_scale(int x, int y)
{
   pagesX = x;
   pagesY = y;
   pages = x * y;

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

   Sc_x = std::min(Sc_x, Sc_y) * 0.99; /* shrink by 1% so we don't cock up */
#if 0 /* this picks a nice (in some sense) ratio, but is too stingy */
   double E = pow(10.0, floor(log10(Sc_x)));
   Sc_x = floor(Sc_x / E) * E;
#endif

   double Scale_exact = 1000.0 / Sc_x;

   /* trim to 2 s.f. (rounding up) */
   double w = pow(10.0, floor(log10(Scale_exact) - 1.0));
   Scale = ceil(Scale_exact / w) * w;
}
