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

#include "img_hosted.h"
#include <wx.h>

extern bool fBlankPage;

struct border {
   long x_min, y_min, x_max, y_max;
};

#define PR_FONT_DEFAULT	0
#define PR_FONT_LABELS	1

#define PR_COLOUR_TEXT		0
#define PR_COLOUR_LABELS	1
#define PR_COLOUR_FRAME		2
#define PR_COLOUR_LEG		3
#define PR_COLOUR_CROSS		4
#define PR_COLOUR_SURFACE_LEG	5

#define PR_FLAG_NOFILEOUTPUT	1
#define PR_FLAG_NOINI		2
#define PR_FLAG_CALIBRATE	4

/* 1:<DEFAULT_SCALE> is the default scale */
#define DEFAULT_SCALE 500

/* Store everything describing the page layout */
class layout {
public:
    /* caller modifiable bits */
    int show_mask;
    bool SkipBlank;
    bool Border;
    bool Cutlines;
    bool Legend;
    wxString title;
    wxString cs_proj;
    wxString datestamp;
    time_t datestamp_numeric;
    double Scale;
    double rot, tilt;
    enum {PLAN, ELEV, TILT, EXTELEV} view;

    /* internal data, but may be accessed */
    double scX, scY;
    double xMin, xMax, yMin, yMax;
    double PaperWidth, PaperDepth;
    int pagesX, pagesY, pages;
    double xOrg, yOrg;
    wxString footer;

    layout(wxPageSetupDialogData* data);
#if 0
    void make_calibration();
#endif
    void pick_scale(int x, int y);
    void pages_required();
};

/* things for a back end */
void drawticks(border clip, int tick_size, int x, int y);

int as_int(const char *v, char *p, int min_val, int max_val);
unsigned long as_colour(const char *v, char *p);

#if 0
class MainFrm;
void print_all(MainFrm *m_parent, layout *l, device *pri);
void print_page(MainFrm *m_parent, layout *l, int page, int pass, int cPasses);
int next_page(int *pstate, char **q, int pageLim);
#endif

#endif
