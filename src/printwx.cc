/* printwx.c */
/* Device dependent part of Survex wxWindows driver */
/* Copyright (C) 1993-2003,2004,2005 Olly Betts
 * Copyright (C) 2001,2004 Philip Underwood
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

#include <vector>

using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <wx/print.h>
#include <wx/printdlg.h>
#include <wx/spinctrl.h>
#include <wx/radiobox.h>
#include <wx/statbox.h>

#include "debug.h" /* for BUG and SVX_ASSERT */
#include "filelist.h"
#include "filename.h"
#include "ini.h"
#include "message.h"
#include "prio.h"
#include "useful.h"

#include "avenprcore.h"
#include "mainfrm.h"
#include "printwx.h"

class svxPrintout : public wxPrintout {
    MainFrm *mainfrm;
    layout *m_layout;
    wxString m_title;
    wxPageSetupData* m_data;
    wxDC* pdc;
    static const int cur_pass = 0;

    wxPen *pen_frame, *pen_cross, *pen_surface_leg, *pen_leg;
    wxColour colour_text, colour_labels, colour_frame, colour_leg;
    wxColour colour_cross,colour_surface_leg;

    long x_t, y_t;
    double font_scaling_x, font_scaling_y;
    wxFont * current_font;

    int check_intersection(long x_p, long y_p);
    void draw_info_box();
    void draw_scale_bar(double x, double y, double MaxLength);
    int next_page(int *pstate, char **q, int pageLim);
    void drawticks(border clip, int tsize, int x, int y);

    void MOVEMM(double X, double Y) {
	MoveTo((long)(X * m_layout->scX), (long)(Y * m_layout->scY));
    }
    void DRAWMM(double X, double Y) {
	DrawTo((long)(X * m_layout->scX), (long)(Y * m_layout->scY));
    }
    void MoveTo(long x, long y);
    void DrawTo(long x, long y);
    void DrawCross(long x, long y);
    void SetFont(int fontcode);
    void SetColour(int colourcode);
    void WriteString(const char *s);
    void DrawEllipse(long x, long y, long r, long R);
    void SolidRectangle(long x, long y, long w, long h);
    int Charset(void);
    int Pre();
    void NewPage(int pg, int pagesX, int pagesY);
    void ShowPage(const char *szPageDetails);
    void PlotLR(const vector<XSect> & centreline);
    void PlotUD(const vector<XSect> & centreline);
    char * Init(FILE **fh_list, bool fCalibrate);
  public:
    svxPrintout(MainFrm *mainfrm, layout *l, wxPageSetupData *data, const wxString & title);
    bool OnPrintPage(int pageNum);
    void GetPageInfo(int *minPage, int *maxPage,
		     int *pageFrom, int *pageTo);
    wxString GetTitle();
    bool HasPage(int pageNum);
    void OnBeginPrinting();
    void OnEndPrinting();
};

BEGIN_EVENT_TABLE(svxPrintDlg, wxDialog)
    EVT_TEXT(svx_SCALE, svxPrintDlg::OnChange)
    EVT_COMBOBOX(svx_SCALE, svxPrintDlg::OnChange)
    EVT_SPINCTRL(svx_BEARING, svxPrintDlg::OnChangeSpin)
    EVT_SPINCTRL(svx_TILT, svxPrintDlg::OnChangeSpin)
    EVT_BUTTON(svx_PRINT, svxPrintDlg::OnPrint)
    EVT_BUTTON(svx_PREVIEW, svxPrintDlg::OnPreview)
    EVT_BUTTON(svx_PLAN, svxPrintDlg::OnPlan)
    EVT_BUTTON(svx_ELEV, svxPrintDlg::OnElevation)
END_EVENT_TABLE()

static const wxString scales[] = {
    "One page", // FIXME TRANSLATE
    "25",
    "50",
    "100",
    "250",
    "500",
    "1000",
    "2500",
    "5000",
    "10000",
    "25000",
    "50000",
    "100000"
};

// there are three jobs to do here...
// User <-> wx - this should possibly be done in a separate file
svxPrintDlg::svxPrintDlg(MainFrm* mainfrm_, const wxString & filename,
			 const wxString & title, const wxString & datestamp,
			 double angle, double tilt_angle,
			 bool labels, bool crosses, bool legs, bool surf)
	: wxDialog(mainfrm_, -1, wxString(msg(/*Print*/399))),
	  m_layout(mainfrm_->GetPageSetupData()),
	  m_File(filename), mainfrm(mainfrm_)
{
    m_layout.Labels = labels;
    m_layout.Crosses = crosses;
    m_layout.Shots = legs;
    m_layout.Surface = surf;
    m_layout.datestamp = osstrdup(datestamp.c_str());
    if (title.length() > 11 &&
	title.substr(title.length() - 11) == " (extended)") {
	m_layout.title = osstrdup(title.substr(0, title.length() - 11).c_str());
	m_layout.view = layout::EXTELEV;
	m_layout.rot = int(deg(angle) + .001);
	if (m_layout.rot != 0 && m_layout.rot != 180) m_layout.rot = 0;
	m_layout.tilt = 0;
    } else {
	// FIXME rot and tilt shouldn't be integers, but for now add a small
	// fraction before forcing to int as otherwise plan view ends up being
	// 89 degrees!
	m_layout.title = osstrdup(title.c_str());
	m_layout.tilt = int(deg(tilt_angle) + .001);
	if (m_layout.tilt == 90) {
	    m_layout.view = layout::PLAN;
	} else if (m_layout.tilt == 0) {
	    m_layout.view = layout::ELEV;
	} else {
	    m_layout.view = layout::TILT;
	}
    }

    /* setup our print dialog*/
    wxBoxSizer* v1 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* h1 = new wxBoxSizer(wxHORIZONTAL); // holds controls
    wxBoxSizer* v2 = new wxStaticBoxSizer(new wxStaticBox(this, -1, msg(/*View*/255)), wxVERTICAL);
    wxBoxSizer* v3 = new wxStaticBoxSizer(new wxStaticBox(this, -1, "Elements"), wxVERTICAL); // FIXME TRANSLATE
    wxBoxSizer* h2 = new wxBoxSizer(wxHORIZONTAL); // holds buttons

    { // this isn't the "too wide" bit...
    wxStaticText* label;
    label = new wxStaticText(this, -1, wxString(msg(/*Scale*/154)) + " 1:");
    m_scale = new wxComboBox(this, svx_SCALE, scales[0], wxDefaultPosition,
			     wxDefaultSize, sizeof(scales) / sizeof(scales[0]),
			     scales);
    wxBoxSizer* scalebox = new wxBoxSizer(wxHORIZONTAL);
    scalebox->Add(label, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    scalebox->Add(m_scale, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    v2->Add(scalebox, 0, wxALIGN_LEFT|wxALL, 0);
    }

    // Make the dummy string wider than any sane value and use that to
    // fix the width of the control so the sizers allow space for bigger
    // page layouts.
    m_printSize = new wxStaticText(this, -1, wxString::Format(msg(/*%d pages (%dx%d)*/257), 9604, 98, 98));
    v2->Add(m_printSize, 0, wxALIGN_LEFT|wxALL, 5);

    { // this isn't the "too wide" bit...
    wxFlexGridSizer* anglebox = new wxFlexGridSizer(2);
    wxStaticText * brg_label;
    brg_label = new wxStaticText(this, -1, msg(/*Bearing*/259));
    anglebox->Add(brg_label, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxALL, 5);
    m_bearing = new wxSpinCtrl(this, svx_BEARING);
    m_bearing->SetRange(0, 359);
    anglebox->Add(m_bearing, 0, wxALIGN_CENTER|wxALL, 5);
    m_tilttext = new wxStaticText(this, -1, "Tilt angle"); // FIXME TRANSLATE
    anglebox->Add(m_tilttext, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxALL, 5);
    m_tilt = new wxSpinCtrl(this,svx_TILT);
    m_tilt->SetRange(-90, 90);
    anglebox->Add(m_tilt, 0, wxALIGN_CENTER|wxALL, 5);

    v2->Add(anglebox, 0, wxALIGN_LEFT|wxALL, 0);
    }

    if (m_layout.view != layout::EXTELEV) {
	wxBoxSizer * planelevsizer = new wxBoxSizer(wxHORIZONTAL);
	planelevsizer->Add(new wxButton(this, svx_PLAN, "Plan"),
			   0, wxALIGN_CENTRE_VERTICAL|wxALL, 5);
	planelevsizer->Add(new wxButton(this, svx_ELEV, "Elevation"),
			   0, wxALIGN_CENTRE_VERTICAL|wxALL, 5);

	v2->Add(planelevsizer, 0, wxALIGN_LEFT|wxALL, 5);
    }

    h1->Add(v2, 0, wxALIGN_LEFT|wxALL, 5);

    m_legs = new wxCheckBox(this, svx_LEGS, msg(/*Underground Survey Legs*/262));
    v3->Add(m_legs, 0, wxALIGN_LEFT|wxALL, 2);
    m_surface = new wxCheckBox(this, svx_SCALEBAR, msg(/*Sur&amp;face Survey Legs*/403));
    v3->Add(m_surface, 0, wxALIGN_LEFT|wxALL, 2);
    m_stations = new wxCheckBox(this, svx_STATIONS, msg(/*Crosses*/261));
    v3->Add(m_stations, 0, wxALIGN_LEFT|wxALL, 2);
    m_names = new wxCheckBox(this, svx_NAMES, msg(/*Station Names*/260));
    v3->Add(m_names, 0, wxALIGN_LEFT|wxALL, 2);
    m_borders = new wxCheckBox(this, svx_BORDERS, "Borders"); // FIXME TRANSLATE
    v3->Add(m_borders, 0, wxALIGN_LEFT|wxALL, 2);
//    m_blanks = new wxCheckBox(this, svx_BLANKS, "Blank Pages");
//    v3->Add(m_blanks, 0, wxALIGN_LEFT|wxALL, 2);
    m_infoBox = new wxCheckBox(this, svx_INFOBOX, "Info Box"); // FIXME TRANSLATE
    v3->Add(m_infoBox, 0, wxALIGN_LEFT|wxALL, 2);

    h1->Add(v3, 0, wxALIGN_LEFT|wxALL, 5);

    v1->Add(h1, 0, wxALIGN_LEFT|wxALL, 5);

    wxButton * but;
    but = new wxButton(this, wxID_CANCEL, msg(/*&Cancel*/402));
    h2->Add(but, 0, wxALIGN_RIGHT|wxALL, 5);
    but = new wxButton(this, svx_PREVIEW, msg(/*Pre&view*/401));
    h2->Add(but, 0, wxALIGN_RIGHT|wxALL, 5);
    but = new wxButton(this, svx_PRINT, msg(/*&Print*/400));
    but->SetDefault();
    h2->Add(but, 0, wxALIGN_RIGHT|wxALL, 5);
    v1->Add(h2, 0, wxALIGN_RIGHT|wxALL, 5);

    SetAutoLayout(true);
    SetSizer(v1);
    v1->Fit(this);
    v1->SetSizeHints(this);

    LayoutToUI();
    SomethingChanged();
}

svxPrintDlg::~svxPrintDlg() {
    osfree(m_layout.title);
    osfree(m_layout.datestamp);
}

void
svxPrintDlg::OnPrint(wxCommandEvent&) {
    SomethingChanged();
    wxPrintDialogData pd(mainfrm->GetPageSetupData()->GetPrintData());
    wxPrinter pr(&pd);
    svxPrintout po(mainfrm, &m_layout, mainfrm->GetPageSetupData(), m_File);
    if (pr.Print(this, &po, true)) {
	// Close the print dialog if printing succeeded.
	Destroy();
    }
}

void
svxPrintDlg::OnPreview(wxCommandEvent&) {
    SomethingChanged();
    wxPrintDialogData pd(mainfrm->GetPageSetupData()->GetPrintData());
    wxPrintPreview* pv;
    pv = new wxPrintPreview(new svxPrintout(mainfrm, &m_layout,
					    mainfrm->GetPageSetupData(), m_File),
			    new svxPrintout(mainfrm, &m_layout,
					    mainfrm->GetPageSetupData(), m_File),
			    &pd);
    wxPreviewFrame *frame = new wxPreviewFrame(pv, mainfrm, msg(/*Print Preview*/398));
    frame->Initialize();

    // Size preview frame so that all of the controlbar and canvas can be seen
    // if possible.
    int w, h;
    // GetBestSize gives us the width needed to show the whole controlbar.
    frame->GetBestSize(&w, &h);
#ifdef __WXMAC__
    // wxMac opens the preview window at minimum size by default.
    // 360x480 is apparently enough to show A4 portrait.
    if (h < 480 || w < 360) {
	if (h < 480) h = 480;
	if (w < 360) w = 360;
    }
#else
    if (h < w) {
	// On wxGTK at least, GetBestSize() returns much too small a height.
	h = w * 6 / 5;
    }
#endif
    // Ensure that we don't make the window bigger than the screen.
    // Use wxGetClientDisplayRect() so we don't cover the MS Windows
    // task bar either.
    wxRect disp = wxGetClientDisplayRect();
    if (w > disp.GetWidth()) w = disp.GetWidth();
    if (h > disp.GetHeight()) h = disp.GetHeight();
    // Centre the window within the "ClientDisplayRect".
    int x = disp.GetLeft() + (disp.GetWidth() - w) / 2;
    int y = disp.GetTop() + (disp.GetHeight() - h) / 2;
    frame->SetSize(x, y, w, h);

    frame->Show();
}

void
svxPrintDlg::OnPlan(wxCommandEvent&) {
    m_tilt->SetValue(90);
    SomethingChanged();
}

void
svxPrintDlg::OnElevation(wxCommandEvent&) {
    m_tilt->SetValue(0);
    SomethingChanged();
}

void
svxPrintDlg::OnChangeSpin(wxSpinEvent&e) {
    SomethingChanged();
}

void
svxPrintDlg::OnChange(wxCommandEvent&) {
    SomethingChanged();
}

void
svxPrintDlg::SomethingChanged() {
    UIToLayout();
    // Update the bounding box.
    RecalcBounds();
    if (m_layout.xMax >= m_layout.xMin) {
	m_layout.pages_required();
	m_printSize->SetLabel(wxString::Format(msg(/*%d pages (%dx%d)*/257), m_layout.pages, m_layout.pagesX, m_layout.pagesY));
    }
}

void
svxPrintDlg::LayoutToUI(){
    m_names->SetValue(m_layout.Labels);
    m_legs->SetValue(m_layout.Shots);
    m_stations->SetValue(m_layout.Crosses);
    m_borders->SetValue(m_layout.Border);
//    m_blanks->SetValue(m_layout.SkipBlank);
    m_infoBox->SetValue(!m_layout.Raw);
    m_surface->SetValue(m_layout.Surface);
    m_tilt->SetValue(m_layout.tilt);
    // FIXME is EXTELEV disable both buttons and tilt and rot spinctrls
    // FIXME: enable both buttons
    if (m_layout.tilt > 89) {
	// FIXME: disable Plan button
    } else if (m_layout.tilt == 0) {
	// FIXME: disable Elevation button
    }

    m_bearing->SetValue(m_layout.rot);
    // Do this last as it causes an OnChange message which calls UIToLayout
    if (m_layout.Scale != 0) {
	wxString temp;
	temp << m_layout.Scale;
	m_scale->SetValue(temp);
    } else {
	m_scale->SetValue(scales[0]);
    }
}

void
svxPrintDlg::UIToLayout(){
    m_layout.Labels = m_names->IsChecked();
    m_layout.Shots = m_legs->IsChecked();
    m_layout.Crosses = m_stations->IsChecked();
    m_layout.Border = m_borders->IsChecked();
//    m_layout.SkipBlank = m_blanks->IsChecked();
    m_layout.Raw = !m_infoBox->IsChecked();
    m_layout.Surface = m_surface->IsChecked();

    if (m_layout.view != layout::EXTELEV) {
	m_layout.tilt = m_tilt->GetValue();
	if (m_layout.tilt == 90) {
	    m_layout.view = layout::PLAN;
	} else if (m_layout.tilt == 0) {
	    m_layout.view = layout::ELEV;
	} else {
	    m_layout.view = layout::TILT;
	}
	m_layout.rot = m_bearing->GetValue();
    }

    (m_scale->GetValue()).ToDouble(&(m_layout.Scale));
    if (m_layout.Scale == 0.0) {
	m_layout.pick_scale(1, 1);
    }
}

void
svxPrintDlg::RecalcBounds()
{
    m_layout.yMax = m_layout.xMax = -DBL_MAX;
    m_layout.yMin = m_layout.xMin = DBL_MAX;

    double SIN,COS,SINT,COST;
    SIN = sin(rad(m_layout.rot));
    COS = cos(rad(m_layout.rot));
    SINT = sin(rad(m_layout.tilt));
    COST = cos(rad(m_layout.tilt));

    if (m_layout.Shots) {
	list<vector<PointInfo> >::const_iterator trav = mainfrm->traverses_begin();
	list<vector<PointInfo> >::const_iterator tend = mainfrm->traverses_end();
	for ( ; trav != tend; ++trav) {
	    vector<PointInfo>::const_iterator pos = trav->begin();
	    vector<PointInfo>::const_iterator end = trav->end();
	    for ( ; pos != end; ++pos) {
		double x = pos->GetX();
		double y = pos->GetY();
		double z = pos->GetZ();
		double X = x * COS - y * SIN;
		if (X > m_layout.xMax) m_layout.xMax = X;
		if (X < m_layout.xMin) m_layout.xMin = X;
		double Y = (x * SIN + y * COS) * SINT + z * COST;
		if (Y > m_layout.yMax) m_layout.yMax = Y;
		if (Y < m_layout.yMin) m_layout.yMin = Y;
	    }
	}
    }
    if (m_layout.Surface) {
	list<vector<PointInfo> >::const_iterator trav = mainfrm->surface_traverses_begin();
	list<vector<PointInfo> >::const_iterator tend = mainfrm->surface_traverses_end();
	for ( ; trav != tend; ++trav) {
	    vector<PointInfo>::const_iterator pos = trav->begin();
	    vector<PointInfo>::const_iterator end = trav->end();
	    for ( ; pos != end; ++pos) {
		double x = pos->GetX();
		double y = pos->GetY();
		double z = pos->GetZ();
		double X = x * COS - y * SIN;
		if (X > m_layout.xMax) m_layout.xMax = X;
		if (X < m_layout.xMin) m_layout.xMin = X;
		double Y = (x * SIN + y * COS) * SINT + z * COST;
		if (Y > m_layout.yMax) m_layout.yMax = Y;
		if (Y < m_layout.yMin) m_layout.yMin = Y;
	    }
	}
    }
    if (m_layout.Labels || m_layout.Crosses) {
	list<LabelInfo*>::const_iterator label = mainfrm->GetLabels();
	while (label != mainfrm->GetLabelsEnd()) {
	    double x = (*label)->GetX();
	    double y = (*label)->GetY();
	    double z = (*label)->GetZ();
	    if (m_layout.Surface || (*label)->IsUnderground()) {
		double X = x * COS - y * SIN;
		if (X > m_layout.xMax) m_layout.xMax = X;
		if (X < m_layout.xMin) m_layout.xMin = X;
		double Y = (x * SIN + y * COS) * SINT + z * COST;
		if (Y > m_layout.yMax) m_layout.yMax = Y;
		if (Y < m_layout.yMin) m_layout.yMin = Y;
	    }
	    ++label;
	}
    }
}

#define DEG "\xB0" /* degree symbol in iso-8859-1 */

static int xpPageWidth, ypPageDepth;
static long MarginLeft, MarginRight, MarginTop, MarginBottom;
static long x_offset, y_offset;
static wxFont *font_labels, *font_default;
static int fontsize, fontsize_labels;

/* FIXME: allow the font to be set */

static const char *fontname = "Arial", *fontname_labels = "Arial";

// wx <-> prcore (calls to print_page etc...)
svxPrintout::svxPrintout(MainFrm *mainfrm_, layout *l, wxPageSetupData *data,
			 const wxString & title)
    : wxPrintout(title)
{
    mainfrm = mainfrm_;
    m_layout = l;
    m_title = title;
    m_data = data;
}

void
svxPrintout::draw_info_box()
{
   layout *l = m_layout;
   char szTmp[256];
   char *p;
   int boxwidth = 60;
   int boxheight = 30;

   SetColour(PR_COLOUR_FRAME);

   if (l->view != layout::EXTELEV) {
      boxwidth = 100;
      boxheight = 40;
      MOVEMM(60,40);
      DRAWMM(60, 0);
      MOVEMM(0, 30); DRAWMM(60, 30);
   }

   MOVEMM(0, boxheight);
   DRAWMM(boxwidth, boxheight);
   DRAWMM(boxwidth, 0);
   if (!l->Border) {
      DRAWMM(0, 0);
      DRAWMM(0, boxheight);
   }

   MOVEMM(0, 20); DRAWMM(60, 20);
   MOVEMM(0, 10); DRAWMM(60, 10);

   switch (l->view) {
    case layout::PLAN: {
      long ax, ay, bx, by, cx, cy, dx, dy;

#define RADIUS 16.0
      DrawEllipse((long)(80.0 * l->scX), (long)(20.0 * l->scY),
		  (long)(RADIUS * l->scX), (long)(RADIUS * l->scY));

      ax = (long)((80 - 15 * sin(rad(000.0 + l->rot))) * l->scX);
      ay = (long)((20 + 15 * cos(rad(000.0 + l->rot))) * l->scY);
      bx = (long)((80 -  7 * sin(rad(180.0 + l->rot))) * l->scX);
      by = (long)((20 +  7 * cos(rad(180.0 + l->rot))) * l->scY);
      cx = (long)((80 - 15 * sin(rad(160.0 + l->rot))) * l->scX);
      cy = (long)((20 + 15 * cos(rad(160.0 + l->rot))) * l->scY);
      dx = (long)((80 - 15 * sin(rad(200.0 + l->rot))) * l->scX);
      dy = (long)((20 + 15 * cos(rad(200.0 + l->rot))) * l->scY);

      MoveTo(ax, ay);
      DrawTo(bx, by);
      DrawTo(cx, cy);
      DrawTo(ax, ay);
      DrawTo(dx, dy);
      DrawTo(bx, by);

      SetColour(PR_COLOUR_TEXT);
      MOVEMM(62, 36);
      WriteString(msg(/*North*/115));

      MOVEMM(5, 23);
      WriteString(msg(/*Plan view*/117));
      break;
    }
    case layout::ELEV: case layout::TILT:
      MOVEMM(65, 15); DRAWMM(70, 12); DRAWMM(68, 15); DRAWMM(70, 18);

      DRAWMM(65, 15); DRAWMM(95, 15);

      DRAWMM(90, 18); DRAWMM(92, 15); DRAWMM(90, 12); DRAWMM(95, 15);

      MOVEMM(80, 13); DRAWMM(80, 17);

      SetColour(PR_COLOUR_TEXT);
      MOVEMM(62, 33);
      WriteString(msg(/*Elevation on*/116));

      sprintf(szTmp, "%03d"DEG, (l->rot + 270) % 360);
      MOVEMM(65, 20); WriteString(szTmp);
      sprintf(szTmp, "%03d"DEG, (l->rot + 90) % 360);
      MOVEMM(85, 20); WriteString(szTmp);

      MOVEMM(5, 23);
      WriteString(msg(/*Elevation*/118));
      break;
    case layout::EXTELEV:
      SetColour(PR_COLOUR_TEXT);
      MOVEMM(5, 13);
      WriteString(msg(/*Extended elevation*/191));
      break;
   }

   MOVEMM(5, boxheight - 7); WriteString(l->title);

   strcpy(szTmp, msg(/*Scale*/154));
   p = szTmp + strlen(szTmp);
   sprintf(p, " 1:%.0f", l->Scale);
   MOVEMM(5, boxheight - 27); WriteString(szTmp);

   if (l->view != layout::EXTELEV) {
      strcpy(szTmp,
	     msg(l->view == layout::PLAN ? /*Up page*/168 : /*View*/169));
      p = szTmp + strlen(szTmp);
      sprintf(p, " %03d"DEG, l->rot);
      MOVEMM(5, 3); WriteString(szTmp);
   }

   /* This used to be a copyright line, but it was occasionally
    * mis-interpreted as us claiming copyright on the survey, so let's
    * give the website URL instead */
   MOVEMM(boxwidth + 2, 2);
   WriteString("Survex "VERSION" - http://www.survex.com/");

   draw_scale_bar(boxwidth + 10.0, 17.0, l->PaperWidth - boxwidth - 18.0);
}

/* Draw fancy scale bar with bottom left at (x,y) (both in mm) and at most */
/* MaxLength mm long. The scaling in use is 1:scale */
void
svxPrintout::draw_scale_bar(double x, double y, double MaxLength)
{
   double StepEst, d;
   int E, Step, n, le, c;
   char u_buf[3], buf[256];
   char *p;
   static const signed char powers[] = {
      12, 9, 9, 9, 6, 6, 6, 3, 2, 2, 0, 0, 0, -3, -3, -3, -6, -6, -6, -9,
   };
   static const char si_mods[sizeof(powers)] = {
      'p', 'n', 'n', 'n', 'u', 'u', 'u', 'm', 'c', 'c', '\0', '\0', '\0',
      'k', 'k', 'k', 'M', 'M', 'M', 'G'
   };
   /* Limit scalebar to 20cm to stop people with A0 plotters complaining */
   if (MaxLength > 200.0) MaxLength = 200.0;

#define dmin 10.0      /* each division >= dmin mm long */
#define StepMax 5      /* number in steps of at most StepMax (x 10^N) */
#define epsilon (1e-4) /* fudge factor to prevent rounding problems */

   E = (int)ceil(log10((dmin * 0.001 * m_layout->Scale) / StepMax));
   StepEst = pow(10.0, -(double)E) * (dmin * 0.001) * m_layout->Scale - epsilon;

   /* Force labelling to be in multiples of 1, 2, or 5 */
   Step = (StepEst <= 1.0 ? 1 : (StepEst <= 2.0 ? 2 : 5));

   /* Work out actual length of each scale bar division */
   d = Step * pow(10.0, (double)E) / m_layout->Scale * 1000.0;

   /* Choose appropriate units, s.t. if possible E is >=0 and minimized */
   /* Range of units is a little extreme, but it doesn't hurt... */
   n = min(E, 9);
   n = max(n, -10) + 10;
   E += (int)powers[n];

   u_buf[0] = si_mods[n];
   u_buf[1] = '\0';
   strcat(u_buf, "m");

   strcpy(buf, msg(/*Scale*/154));

   /* Add units used - eg. "Scale (10m)" */
   p = buf + strlen(buf);
   sprintf(p, " (%.0f%s)", (double)pow(10.0, (double)E), u_buf);

   SetColour(PR_COLOUR_TEXT);
   MOVEMM(x, y + 4); WriteString(buf);

   /* Work out how many divisions there will be */
   n = (int)(MaxLength / d);

   SetColour(PR_COLOUR_FRAME);

   long Y = long(y * m_layout->scY);
   long Y2 = long((y + 3) * m_layout->scY);
   long X = long(x * m_layout->scX);
   long X2 = long((x + n * d) * m_layout->scX);

   /* Draw top of scale bar */
   MoveTo(X2, Y2);
   DrawTo(X, Y2);
#if 0
   DrawTo(X2, Y);
   DrawTo(X, Y);
   MOVEMM(x + n * d, y); DRAWMM(x, y);
#endif
   /* Draw divisions and label them */
   for (c = 0; c <= n; c++) {
      SetColour(PR_COLOUR_FRAME);
      X = long((x + c * d) * m_layout->scX);
      MoveTo(X, Y);
      DrawTo(X, Y2);
#if 0 // Don't waste toner!
      /* Draw a "zebra crossing" scale bar. */
      if (c < n && (c & 1) == 0) {
	  X2 = long((x + (c + 1) * d) * m_layout->scX);
	  SolidRectangle(X, Y, X2 - X, Y2 - Y);
      }
#endif
      /* ANSI sprintf returns length of formatted string, but some pre-ANSI Unix
       * implementations return char* (ptr to end of written string I think) */
      sprintf(buf, "%d", c * Step);
      le = strlen(buf);
      SetColour(PR_COLOUR_TEXT);
      MOVEMM(x + c * d - le, y - 4);
      WriteString(buf);
   }
}

#if 0
void
make_calibration(layout *l) {
      img_point pt = { 0.0, 0.0, 0.0 };
      l->xMax = l->yMax = 0.1;
      l->xMin = l->yMin = 0;

      stack(l,img_MOVE, NULL, &pt);
      pt.x = 0.1;
      stack(l,img_LINE, NULL, &pt);
      pt.y = 0.1;
      stack(l,img_LINE, NULL, &pt);
      pt.x = 0.0;
      stack(l,img_LINE, NULL, &pt);
      pt.y = 0.0;
      stack(l,img_LINE, NULL, &pt);
      pt.x = 0.05;
      pt.y = 0.001;
      stack(l,img_LABEL, "10cm", &pt);
      pt.x = 0.001;
      pt.y = 0.05;
      stack(l,img_LABEL, "10cm", &pt);
      l->Scale = 1.0;
}
#endif

int
svxPrintout::next_page(int *pstate, char **q, int pageLim)
{
   char *p;
   int page;
   int c;
   p = *q;
   if (*pstate > 0) {
      /* doing a range */
      (*pstate)++;
      SVX_ASSERT(*p == '-');
      p++;
      while (isspace((unsigned char)*p)) p++;
      if (sscanf(p, "%u%n", &page, &c) > 0) {
	 p += c;
      } else {
	 page = pageLim;
      }
      if (*pstate > page) goto err;
      if (*pstate < page) return *pstate;
      *q = p;
      *pstate = 0;
      return page;
   }

   while (isspace((unsigned char)*p) || *p == ',') p++;

   if (!*p) return 0; /* done */

   if (*p == '-') {
      *q = p;
      *pstate = 1;
      return 1; /* range with initial parameter omitted */
   }
   if (sscanf(p, "%u%n", &page, &c) > 0) {
      p += c;
      while (isspace((unsigned char)*p)) p++;
      *q = p;
      if (0 < page && page <= pageLim) {
	 if (*p == '-') *pstate = page; /* range with start */
	 return page;
      }
   }
   err:
   *pstate = -1;
   return 0;
}

/* Draws in alignment marks on each page or borders on edge pages */
void
svxPrintout::drawticks(border clip, int tsize, int x, int y)
{
   long i;
   int s = tsize * 4;
   int o = s / 8;
   bool fAtCorner = fFalse;
   SetColour(PR_COLOUR_FRAME);
   if (x == 0 && m_layout->Border) {
      /* solid left border */
      MoveTo(clip.x_min, clip.y_min);
      DrawTo(clip.x_min, clip.y_max);
      fAtCorner = fTrue;
   } else {
      if (x > 0 || y > 0) {
	 MoveTo(clip.x_min, clip.y_min);
	 DrawTo(clip.x_min, clip.y_min + tsize);
      }
      if (s && x > 0 && m_layout->Cutlines) {
	 /* dashed left border */
	 i = (clip.y_max - clip.y_min) -
	     (tsize + ((clip.y_max - clip.y_min - tsize * 2L) % s) / 2);
	 for ( ; i > tsize; i -= s) {
	    MoveTo(clip.x_min, clip.y_max - (i + o));
	    DrawTo(clip.x_min, clip.y_max - (i - o));
	 }
      }
      if (x > 0 || y < m_layout->pagesY - 1) {
	 MoveTo(clip.x_min, clip.y_max - tsize);
	 DrawTo(clip.x_min, clip.y_max);
	 fAtCorner = fTrue;
      }
   }

   if (y == m_layout->pagesY - 1 && m_layout->Border) {
      /* solid top border */
      if (!fAtCorner) MoveTo(clip.x_min, clip.y_max);
      DrawTo(clip.x_max, clip.y_max);
      fAtCorner = fTrue;
   } else {
      if (y < m_layout->pagesY - 1 || x > 0) {
	 if (!fAtCorner) MoveTo(clip.x_min, clip.y_max);
	 DrawTo(clip.x_min + tsize, clip.y_max);
      }
      if (s && y < m_layout->pagesY - 1 && m_layout->Cutlines) {
	 /* dashed top border */
	 i = (clip.x_max - clip.x_min) -
	     (tsize + ((clip.x_max - clip.x_min - tsize * 2L) % s) / 2);
	 for ( ; i > tsize; i -= s) {
	    MoveTo(clip.x_max - (i + o), clip.y_max);
	    DrawTo(clip.x_max - (i - o), clip.y_max);
	 }
      }
      if (y < m_layout->pagesY - 1 || x < m_layout->pagesX - 1) {
	 MoveTo(clip.x_max - tsize, clip.y_max);
	 DrawTo(clip.x_max, clip.y_max);
	 fAtCorner = fTrue;
      } else {
	 fAtCorner = fFalse;
      }
   }

   if (x == m_layout->pagesX - 1 && m_layout->Border) {
      /* solid right border */
      if (!fAtCorner) MoveTo(clip.x_max, clip.y_max);
      DrawTo(clip.x_max, clip.y_min);
      fAtCorner = fTrue;
   } else {
      if (x < m_layout->pagesX - 1 || y < m_layout->pagesY - 1) {
	 if (!fAtCorner) MoveTo(clip.x_max, clip.y_max);
	 DrawTo(clip.x_max, clip.y_max - tsize);
      }
      if (s && x < m_layout->pagesX - 1 && m_layout->Cutlines) {
	 /* dashed right border */
	 i = (clip.y_max - clip.y_min) -
	     (tsize + ((clip.y_max - clip.y_min - tsize * 2L) % s) / 2);
	 for ( ; i > tsize; i -= s) {
	    MoveTo(clip.x_max, clip.y_min + (i + o));
	    DrawTo(clip.x_max, clip.y_min + (i - o));
	 }
      }
      if (x < m_layout->pagesX - 1 || y > 0) {
	 MoveTo(clip.x_max, clip.y_min + tsize);
	 DrawTo(clip.x_max, clip.y_min);
	 fAtCorner = fTrue;
      } else {
	 fAtCorner = fFalse;
      }
   }

   if (y == 0 && m_layout->Border) {
      /* solid bottom border */
      if (!fAtCorner) MoveTo(clip.x_max, clip.y_min);
      DrawTo(clip.x_min, clip.y_min);
   } else {
      if (y > 0 || x < m_layout->pagesX - 1) {
	 if (!fAtCorner) MoveTo(clip.x_max, clip.y_min);
	 DrawTo(clip.x_max - tsize, clip.y_min);
      }
      if (s && y > 0 && m_layout->Cutlines) {
	 /* dashed bottom border */
	 i = (clip.x_max - clip.x_min) -
	     (tsize + ((clip.x_max - clip.x_min - tsize * 2L) % s) / 2);
	 for ( ; i > tsize; i -= s) {
	    MoveTo(clip.x_min + (i + o), clip.y_min);
	    DrawTo(clip.x_min + (i - o), clip.y_min);
	 }
      }
      if (y > 0 || x > 0) {
	 MoveTo(clip.x_min + tsize, clip.y_min);
	 DrawTo(clip.x_min, clip.y_min);
      }
   }
}

bool
svxPrintout::OnPrintPage(int pageNum) {
    GetPageSizePixels(&xpPageWidth, &ypPageDepth);
    pdc = GetDC();
    if (IsPreview()) {
	int dcx, dcy;
	pdc->GetSize(&dcx, &dcy);
	pdc->SetUserScale((double)dcx / xpPageWidth, (double)dcy / ypPageDepth);
    }

    layout * l = m_layout;
    {
	int pwidth, pdepth;
	GetPageSizeMM(&pwidth, &pdepth);
	l->scX = (double)xpPageWidth / pwidth;
	l->scY = (double)ypPageDepth / pdepth;
	font_scaling_x = l->scX * (25.4 / 72.0);
	font_scaling_y = l->scY * (25.4 / 72.0);
	MarginLeft = m_data->GetMarginTopLeft().x;
	MarginTop = m_data->GetMarginTopLeft().y;
	MarginBottom = m_data->GetMarginBottomRight().y;
	MarginRight = m_data->GetMarginBottomRight().x;
	xpPageWidth -= (int)(l->scX * (MarginLeft + MarginRight));
	ypPageDepth -= (int)(l->scY * (10 + MarginBottom + MarginRight));
	// xpPageWidth -= 1;
	pdepth -= 10;
	x_offset = (long)(l->scX * MarginLeft);
	y_offset = (long)(l->scY * MarginTop);
	l->PaperWidth = pwidth -= MarginLeft + MarginRight;
	l->PaperDepth = pdepth -= MarginTop + MarginBottom;
    }

    double SIN,COS,SINT,COST;
    SIN = sin(rad(l->rot));
    COS = cos(rad(l->rot));
    SINT = sin(rad(l->tilt));
    COST = cos(rad(l->tilt));

    NewPage(pageNum, l->pagesX, l->pagesY);

    if (!l->Raw && pageNum == (l->pagesY - 1) * l->pagesX + 1) {
	SetFont(PR_FONT_DEFAULT);
	draw_info_box();
    }

    const double Sc = 1000 / l->Scale;

    if (l->Shots) {
	SetColour(PR_COLOUR_LEG);
	list<vector<PointInfo> >::const_iterator trav = mainfrm->traverses_begin();
	list<vector<PointInfo> >::const_iterator tend = mainfrm->traverses_end();
	for ( ; trav != tend; ++trav) {
	    vector<PointInfo>::const_iterator pos = trav->begin();
	    vector<PointInfo>::const_iterator end = trav->end();
	    for ( ; pos != end; ++pos) {
		double x = pos->GetX();
		double y = pos->GetY();
		double z = pos->GetZ();
		double X = x * COS - y * SIN;
		double Y = (x * SIN + y * COS) * SINT + z * COST;
		long px = (long)((X * Sc + l->xOrg) * l->scX);
		long py = (long)((Y * Sc + l->yOrg) * l->scY);
		if (pos == trav->begin()) {
		    MoveTo(px, py);
		} else {
		    DrawTo(px, py);
		}
	    }
	}
    }

    if (l->Shots && (l->tilt == 0.0 || l->tilt == 90.0 || l->tilt == -90.0)) {
	list<vector<XSect> >::const_iterator trav = mainfrm->tubes_begin();
	list<vector<XSect> >::const_iterator tend = mainfrm->tubes_end();
	for ( ; trav != tend; ++trav) {
	    if (l->tilt == 90.0 || l->tilt == -90.0) PlotLR(*trav);
	    if (l->tilt == 0.0) PlotUD(*trav);
	}
    }

    if (l->Surface) {
	SetColour(PR_COLOUR_SURFACE_LEG);
	list<vector<PointInfo> >::const_iterator trav = mainfrm->surface_traverses_begin();
	list<vector<PointInfo> >::const_iterator tend = mainfrm->surface_traverses_end();
	for ( ; trav != tend; ++trav) {
	    vector<PointInfo>::const_iterator pos = trav->begin();
	    vector<PointInfo>::const_iterator end = trav->end();
	    for ( ; pos != end; ++pos) {
		double x = pos->GetX();
		double y = pos->GetY();
		double z = pos->GetZ();
		double X = x * COS - y * SIN;
		double Y = (x * SIN + y * COS) * SINT + z * COST;
		long px = (long)((X * Sc + l->xOrg) * l->scX);
		long py = (long)((Y * Sc + l->yOrg) * l->scY);
		if (pos == trav->begin()) {
		    MoveTo(px, py);
		} else {
		    DrawTo(px, py);
		}
	    }
	}
    }

    if (l->Labels || l->Crosses) {
	if (l->Labels) SetFont(PR_FONT_LABELS);
	list<LabelInfo*>::const_iterator label = mainfrm->GetLabels();
	while (label != mainfrm->GetLabelsEnd()) {
	    double px = (*label)->GetX();
	    double py = (*label)->GetY();
	    double pz = (*label)->GetZ();
	    if (l->Surface || (*label)->IsUnderground()) {
		double X = px * COS - py * SIN;
		double Y = (px * SIN + py * COS) * SINT + pz * COST;
		long xnew, ynew;
		xnew = (long)((X * Sc + l->xOrg) * l->scX);
		ynew = (long)((Y * Sc + l->yOrg) * l->scY);
		if (l->Crosses) {
		    SetColour(PR_COLOUR_CROSS);
		    DrawCross(xnew, ynew);
		}
		if (l->Labels) {
		    SetColour(PR_COLOUR_LABELS);
		    MoveTo(xnew, ynew);
		    WriteString((*label)->GetText());
		}
	    }
	    ++label;
	}
    }

    if (!l->Raw) {
	char szTmp[256];
	SetColour(PR_COLOUR_TEXT);
	sprintf(szTmp, l->footer, l->title, pageNum, l->pagesX * l->pagesY,
		l->datestamp);
	ShowPage(szTmp);
    } else {
	ShowPage("");
    }

    return true;
}

void
svxPrintout::GetPageInfo(int *minPage, int *maxPage,
			 int *pageFrom, int *pageTo)
{
    *minPage = *pageFrom = 1;
    *maxPage = *pageTo = m_layout->pages;
}

wxString
svxPrintout::GetTitle() {
    return m_title;
}

bool
svxPrintout::HasPage(int pageNum) {
    return (pageNum <= m_layout->pages);
}

void
svxPrintout::OnBeginPrinting() {
    FILE *fh_list[4];

    FILE **pfh = fh_list;
    FILE *fh;
    const char *pth_cfg;
    char *print_ini;

    /* ini files searched in this order:
     * ~/.survex/print.ini [unix only]
     * /etc/survex/print.ini [unix only]
     * <support file directory>/myprint.ini [not unix]
     * <support file directory>/print.ini [must exist]
     */

#if (OS==UNIX)
    pth_cfg = getenv("HOME");
    if (pth_cfg) {
	fh = fopenWithPthAndExt(pth_cfg, ".survex/print."EXT_INI, NULL,
		"rb", NULL);
	if (fh) *pfh++ = fh;
    }
    pth_cfg = msg_cfgpth();
    fh = fopenWithPthAndExt(NULL, "/etc/survex/print."EXT_INI, NULL, "rb",
	    NULL);
    if (fh) *pfh++ = fh;
#else
    pth_cfg = msg_cfgpth();
    print_ini = add_ext("myprint", EXT_INI);
    fh = fopenWithPthAndExt(pth_cfg, print_ini, NULL, "rb", NULL);
    if (fh) *pfh++ = fh;
#endif
    print_ini = add_ext("print", EXT_INI);
    fh = fopenWithPthAndExt(pth_cfg, print_ini, NULL, "rb", NULL);
    if (!fh) fatalerror(/*Couldn't open data file `%s'*/24, print_ini);
    *pfh++ = fh;
    *pfh = NULL;
    Init(pfh, false);
    for (pfh = fh_list; *pfh; pfh++) (void)fclose(*pfh);
    Pre();
    m_layout->footer = msgPerm(/*Survey `%s'   Page %d (of %d)   Processed on %s*/167);
}

void
svxPrintout::OnEndPrinting() {
    delete(font_labels);
    delete(font_default);
    delete(pen_frame);
    delete(pen_leg);
    delete(pen_surface_leg);
    delete(pen_cross);
}


// prcore -> wx.grafx (calls to move pens around and stuff - low level)
// this seems to have been done...



static border clip;


int
svxPrintout::check_intersection(long x_p, long y_p)
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
   SVX_ASSERT(mask_t & L);
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

void
svxPrintout::MoveTo(long x, long y)
{
    x_t = x_offset + x - clip.x_min;
    y_t = y_offset + clip.y_max - y;
}

void
svxPrintout::DrawTo(long x, long y)
{
    long x_p = x_t, y_p = y_t;
    x_t = x_offset + x - clip.x_min;
    y_t = y_offset + clip.y_max - y;
    if (cur_pass != -1) {
	pdc->DrawLine(x_p,y_p,x_t,y_t);
    } else {
	if (check_intersection(x_p, y_p)) fBlankPage = fFalse;
    }
}

#define POINTS_PER_INCH 72.0
#define POINTS_PER_MM (POINTS_PER_INCH / MM_PER_INCH)
#define PWX_CROSS_SIZE (int)(2 * m_layout->scX / POINTS_PER_MM)

void
svxPrintout::DrawCross(long x, long y)
{
   if (cur_pass != -1) {
      MoveTo(x - PWX_CROSS_SIZE, y - PWX_CROSS_SIZE);
      DrawTo(x + PWX_CROSS_SIZE, y + PWX_CROSS_SIZE);
      MoveTo(x + PWX_CROSS_SIZE, y - PWX_CROSS_SIZE);
      DrawTo(x - PWX_CROSS_SIZE, y + PWX_CROSS_SIZE);
      MoveTo(x, y);
   } else {
      if ((x + PWX_CROSS_SIZE > clip.x_min &&
	   x - PWX_CROSS_SIZE < clip.x_max) ||
	  (y + PWX_CROSS_SIZE > clip.y_min &&
	   y - PWX_CROSS_SIZE < clip.y_max)) {
	 fBlankPage = fFalse;
      }
   }
}

void
svxPrintout::SetFont(int fontcode)
{
    switch (fontcode) {
	case PR_FONT_DEFAULT:
	    current_font = font_default;
	    break;
	case PR_FONT_LABELS:
	    current_font = font_labels;
	    break;
	default:
	    BUG("unknown font code");
    }
}

void
svxPrintout::SetColour(int colourcode)
{
    switch (colourcode) {
	case PR_COLOUR_TEXT:
	    pdc->SetTextForeground(colour_text);
	    break;
	case PR_COLOUR_LABELS:
	    pdc->SetTextForeground(colour_labels);
	    pdc->SetBackgroundMode(wxTRANSPARENT);
	    break;
	case PR_COLOUR_FRAME:
	    pdc->SetPen(*pen_frame);
	    break;
	case PR_COLOUR_LEG:
	    pdc->SetPen(*pen_leg);
	    break;
	case PR_COLOUR_CROSS:
	    pdc->SetPen(*pen_cross);
	    break;
	case PR_COLOUR_SURFACE_LEG:
	    pdc->SetPen(*pen_surface_leg);
	    break;
	default:
	    BUG("unknown colour code");
    }
}

void
svxPrintout::WriteString(const char *s)
{
    double xsc, ysc;
    pdc->GetUserScale(&xsc, &ysc);
    pdc->SetUserScale(xsc * font_scaling_x, ysc * font_scaling_y);
    pdc->SetFont(*current_font);
    int w, h;
    if (cur_pass != -1) {
	pdc->GetTextExtent("My", &w, &h);
	pdc->DrawText(s,
		      long(x_t / font_scaling_x),
		      long(y_t / font_scaling_y) - h);
    } else {
	pdc->GetTextExtent(s, &w, &h);
	if ((y_t + h > 0 && y_t - h < clip.y_max - clip.y_min) ||
	    (x_t < clip.x_max - clip.x_min && x_t + w > 0)) {
	    fBlankPage = fFalse;
	}
    }
    pdc->SetUserScale(xsc, ysc);
}

void
svxPrintout::DrawEllipse(long x, long y, long r, long R)
{
    /* Don't need to check in first-pass - circle is only used in title box */
    if (cur_pass != -1) {
	x_t = x_offset + x - clip.x_min;
	y_t = y_offset + clip.y_max - y;
	pdc->SetBrush(*wxTRANSPARENT_BRUSH);
	pdc->DrawEllipse(x_t - r, y_t - R, 2 * r, 2 * R);
    }
}

void
svxPrintout::SolidRectangle(long x, long y, long w, long h)
{
    long X = x_offset + x - clip.x_min;
    long Y = y_offset + clip.y_max - y;
    pdc->SetBrush(*wxBLACK_BRUSH);
    pdc->DrawRectangle(X, Y - h, w, h);
}

int
svxPrintout::Charset(void)
{
   return CHARSET_ISO_8859_1;
}

int
svxPrintout::Pre()
{
    font_labels = new wxFont(fontsize_labels,wxDEFAULT,wxNORMAL,wxNORMAL,false,fontname_labels,wxFONTENCODING_ISO8859_1);
    font_default = new wxFont(fontsize,wxDEFAULT,wxNORMAL,wxNORMAL,false,fontname,wxFONTENCODING_ISO8859_1);
    current_font = font_default;
    pen_leg = new wxPen(colour_leg,0,wxSOLID);
    pen_surface_leg = new wxPen(colour_surface_leg,0,wxSOLID);
    pen_cross = new wxPen(colour_cross,0,wxSOLID);
    pen_frame = new wxPen(colour_frame,0,wxSOLID);
    return 1; /* only need 1 pass */
}

void
svxPrintout::NewPage(int pg, int pagesX, int pagesY)
{
    int x, y;
    x = (pg - 1) % pagesX;
    y = pagesY - 1 - ((pg - 1) / pagesX);

    clip.x_min = (long)x * xpPageWidth;
    clip.y_min = (long)y * ypPageDepth;
    clip.x_max = clip.x_min + xpPageWidth; /* dm/pcl/ps had -1; */
    clip.y_max = clip.y_min + ypPageDepth; /* dm/pcl/ps had -1; */

    //we have to write the footer here. PostScript is being weird. Really weird.
    pdc->SetFont(*font_labels);
    MoveTo((long)(6 * m_layout->scX) + clip.x_min,
	   clip.y_min - (long)(7 * m_layout->scY));
    char szFooter[256];
    sprintf(szFooter, m_layout->footer, m_layout->title, pg,
	    m_layout->pagesX * m_layout->pagesY,m_layout->datestamp);
    WriteString(szFooter);
    pdc->DestroyClippingRegion();
    pdc->SetClippingRegion(x_offset, y_offset,xpPageWidth+1, ypPageDepth+1);
    drawticks(clip, (int)(9 * m_layout->scX / POINTS_PER_MM), x, y);
}

void
svxPrintout::ShowPage(const char *szPageDetails)
{
}

void
svxPrintout::PlotLR(const vector<XSect> & centreline)
{
    assert(centreline.size() > 1);
    XSect prev_pt_v;
    Vector3 last_right(1.0, 0.0, 0.0);

    const double Sc = 1000 / m_layout->Scale;
    const double SIN = sin(rad(m_layout->rot));
    const double COS = cos(rad(m_layout->rot));

    vector<XSect>::const_iterator i = centreline.begin();
    vector<XSect>::size_type segment = 0;
    while (i != centreline.end()) {
	// get the coordinates of this vertex
	const XSect & pt_v = *i++;

	Vector3 right;

	const Vector3 up_v(0.0, 0.0, 1.0);

	if (segment == 0) {
	    assert(i != centreline.end());
	    // first segment

	    // get the coordinates of the next vertex
	    const XSect & next_pt_v = *i;

	    // calculate vector from this pt to the next one
	    Vector3 leg_v = pt_v.vec() - next_pt_v.vec();

	    // obtain a vector in the LRUD plane
	    right = leg_v * up_v;
	    if (right.magnitude() == 0) {
		right = last_right;
	    } else {
		last_right = right;
	    }
	} else if (segment + 1 == centreline.size()) {
	    // last segment

	    // Calculate vector from the previous pt to this one.
	    Vector3 leg_v = prev_pt_v.vec() - pt_v.vec();

	    // Obtain a horizontal vector in the LRUD plane.
	    right = leg_v * up_v;
	    if (right.magnitude() == 0) {
		right = Vector3(last_right.getX(), last_right.getY(), 0.0);
	    } else {
		last_right = right;
	    }
	} else {
	    assert(i != centreline.end());
	    // Intermediate segment.

	    // Get the coordinates of the next vertex.
	    const XSect & next_pt_v = *i;

	    // Calculate vectors from this vertex to the
	    // next vertex, and from the previous vertex to
	    // this one.
	    Vector3 leg1_v = prev_pt_v.vec() - pt_v.vec();
	    Vector3 leg2_v = pt_v.vec() - next_pt_v.vec();

	    // Obtain horizontal vectors perpendicular to
	    // both legs, then normalise and average to get
	    // a horizontal bisector.
	    Vector3 r1 = leg1_v * up_v;
	    Vector3 r2 = leg2_v * up_v;
	    r1.normalise();
	    r2.normalise();
	    right = r1 + r2;
	    if (right.magnitude() == 0) {
		// This is the "mid-pitch" case...
		right = last_right;
	    }
	    last_right = right;
	}

	// Scale to unit vectors in the LRUD plane.
	right.normalise();

	Double l = pt_v.GetL();
	Double r = pt_v.GetR();

	if (l >= 0) {
	    Vector3 p = pt_v.vec() - right * l;
	    double X = p.getX() * COS - p.getY() * SIN;
	    double Y = (p.getX() * SIN + p.getY() * COS);
	    long x = (long)((X * Sc + m_layout->xOrg) * m_layout->scX);
	    long y = (long)((Y * Sc + m_layout->yOrg) * m_layout->scY);
	    MoveTo(x - PWX_CROSS_SIZE, y - PWX_CROSS_SIZE);
	    DrawTo(x, y);
	    DrawTo(x - PWX_CROSS_SIZE, y + PWX_CROSS_SIZE);
	}
	if (r >= 0) {
	    Vector3 p = pt_v.vec() + right * r;
	    double X = p.getX() * COS - p.getY() * SIN;
	    double Y = (p.getX() * SIN + p.getY() * COS);
	    long x = (long)((X * Sc + m_layout->xOrg) * m_layout->scX);
	    long y = (long)((Y * Sc + m_layout->yOrg) * m_layout->scY);
	    MoveTo(x + PWX_CROSS_SIZE, y - PWX_CROSS_SIZE);
	    DrawTo(x, y);
	    DrawTo(x + PWX_CROSS_SIZE, y + PWX_CROSS_SIZE);
	}

	prev_pt_v = pt_v;

	++segment;
    }
}

void
svxPrintout::PlotUD(const vector<XSect> & centreline)
{
    assert(centreline.size() > 1);
    const double Sc = 1000 / m_layout->Scale;

    vector<XSect>::const_iterator i = centreline.begin();
    while (i != centreline.end()) {
	// get the coordinates of this vertex
	const XSect & pt_v = *i++;

	Double u = pt_v.GetL();
	Double d = pt_v.GetR();

	if (u >= 0 || d >= 0) {
	    Vector3 p = pt_v.vec();
	    double SIN = sin(rad(m_layout->rot));
	    double COS = cos(rad(m_layout->rot));
	    double X = p.getX() * COS - p.getY() * SIN;
	    double Y = p.getZ();
	    long x = (long)((X * Sc + m_layout->xOrg) * m_layout->scX);
	    if (u >= 0) {
		long y = (long)(((Y + u) * Sc + m_layout->yOrg) * m_layout->scY);
		MoveTo(x - PWX_CROSS_SIZE, y + PWX_CROSS_SIZE);
		DrawTo(x, y);
		DrawTo(x + PWX_CROSS_SIZE, y + PWX_CROSS_SIZE);
	    }
	    if (d >= 0) {
		long y = (long)(((Y - d) * Sc + m_layout->yOrg) * m_layout->scY);
		MoveTo(x - PWX_CROSS_SIZE, y - PWX_CROSS_SIZE);
		DrawTo(x, y);
		DrawTo(x + PWX_CROSS_SIZE, y - PWX_CROSS_SIZE);
	    }
	}
    }
}

static wxColour
to_rgb(const char *var, char *val)
{
   unsigned long rgb;
   if (!val) return *wxBLACK;
   rgb = as_colour(var, val);
   return wxColour((rgb & 0xff0000) >> 16, (rgb & 0xff00) >> 8, rgb & 0xff);
}

/* Initialise printer routines */
char *
svxPrintout::Init(FILE **fh_list, bool fCalibrate)
{
   static const char *vars[] = {
      "like",
      "font_size_labels",
      "colour_text",
      "colour_labels",
      "colour_frame",
      "colour_legs",
      "colour_crosses",
      "colour_surface_legs",
      NULL
   };
   char **vals;

   fCalibrate = fCalibrate; /* suppress unused argument warning */

   vals = ini_read_hier(fh_list, "win", vars);
   fontsize_labels = 10;
   if (vals[1]) fontsize_labels = as_int(vars[1], vals[1], 1, INT_MAX);
   fontsize = 10;

   colour_text = colour_labels = colour_frame = colour_leg = colour_cross = colour_surface_leg = *wxBLACK;
   if (vals[2]) colour_text = to_rgb(vars[2], vals[2]);
   if (vals[3]) colour_labels = to_rgb(vars[3], vals[3]);
   if (vals[4]) colour_frame = to_rgb(vars[4], vals[4]);
   if (vals[5]) colour_leg = to_rgb(vars[5], vals[5]);
   if (vals[6]) colour_cross = to_rgb(vars[6], vals[6]);
   if (vals[7]) colour_surface_leg = to_rgb(vars[7], vals[7]);
   m_layout->scX = 1;
   m_layout->scY = 1;
   return NULL;
}
