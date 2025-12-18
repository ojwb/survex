/* avenprcore.h
 * Header file for printer independent parts of Survex printer drivers
 * Copyright (C) 1994-2002,2004,2005,2012,2013,2014,2015 Olly Betts
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

#ifndef survex_included_avenprcore_h
#define survex_included_avenprcore_h

#include <wx/wx.h>
#include "export.h"

/* 1:<DEFAULT_SCALE> is the default scale */
#define DEFAULT_SCALE 500

/* Height of our footer in mm. */
#define FOOTER_HEIGHT_MM 10

class wxPageSetupDialogData;

/* Store everything describing the page layout */
class layout {
public:
    /* caller modifiable bits */
    int show_mask = 0;
    bool SkipBlank = false;
    bool Border = true;
    bool Cutlines = true;
    bool Legend = true;
    wxString title;
    wxString datestamp;
    double Scale = 0.0;
    double rot = 0.0, tilt = 0.0;
    enum {PLAN, ELEV, TILT, EXTELEV} view = PLAN;

    /* internal data, but may be accessed */
    double scX = 1.0, scY = 1.0;
    double xMin = 0.0, xMax = -1.0, yMin = 0.0, yMax = -1.0;
    double PaperWidth, PaperDepth;
    int pagesX = 1, pagesY = 1, pages = 1;
    double xOrg = 0.0, yOrg = 0.0;

    explicit layout(wxPageSetupDialogData* data);
#if 0
    void make_calibration();
#endif
    void pick_scale(int x, int y);
    void pages_required();
    int get_effective_show_mask() const {
	int result = show_mask;
	if (view == tilt) {
	    result &= ~(XSECT|WALLS|PASG);
	}
	return result;
    }
};

#endif
