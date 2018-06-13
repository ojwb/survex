/* printing.cc */
/* Aven printing code */
/* Copyright (C) 1993-2003,2004,2005,2006,2010,2011,2012,2013,2014,2015,2016,2017 Olly Betts
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <wx/confbase.h>
#include <wx/filename.h>
#include <wx/print.h>
#include <wx/printdlg.h>
#include <wx/spinctrl.h>
#include <wx/radiobox.h>
#include <wx/statbox.h>
#include <wx/valgen.h>

#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <limits.h>

#include "export.h"
#include "filelist.h"
#include "filename.h"
#include "message.h"
#include "useful.h"

#include "aven.h"
#include "avenprcore.h"
#include "mainfrm.h"
#include "printing.h"

using namespace std;

// How many decimal points to show on angles:
#define ANGLE_DP 1

#if ANGLE_DP == 0
# define ANGLE_FMT wxT("%03.f")
# define ANGLE2_FMT wxT("%.f")
#elif ANGLE_DP == 1
# define ANGLE_FMT wxT("%05.1f")
# define ANGLE2_FMT wxT("%.1f")
#elif ANGLE_DP == 2
# define ANGLE_FMT wxT("%06.2f")
# define ANGLE2_FMT wxT("%.2f")
#else
# error Need to add ANGLE_FMT and ANGLE2_FMT for the currently set ANGLE_DP
#endif

static wxString
format_angle(const wxChar * fmt, double angle)
{
    wxString s;
    s.Printf(fmt, angle);
    size_t dot = s.find('.');
    size_t i = s.size();
    while (i > dot) {
	--i;
	if (s[i] != '0') {
	    if (i != dot) ++i;
	    s.resize(i);
	    break;
	}
    }
    s += wmsg(/*°*/344);
    return s;
}

enum {
	svx_EXPORT = 1200,
	svx_FORMAT,
	svx_SCALE,
	svx_BEARING,
	svx_TILT,
	svx_LEGS,
	svx_STATIONS,
	svx_NAMES,
	svx_XSECT,
	svx_WALLS,
	svx_PASSAGES,
	svx_BORDERS,
	svx_BLANKS,
	svx_LEGEND,
	svx_SURFACE,
	svx_SPLAYS,
	svx_PLAN,
	svx_ELEV,
	svx_ENTS,
	svx_FIXES,
	svx_EXPORTS,
	svx_PROJ_LABEL,
	svx_PROJ,
	svx_GRID,
	svx_TEXT_HEIGHT,
	svx_MARKER_SIZE,
	svx_CENTRED,
	svx_FULLCOORDS
};

class BitValidator : public wxValidator {
    // Disallow assignment.
    BitValidator & operator=(const BitValidator&);

  protected:
    int * val;

    int mask;

  public:
    BitValidator(int * val_, int mask_)
	: val(val_), mask(mask_) { }

    BitValidator(const BitValidator &o) : wxValidator() {
	Copy(o);
    }

    ~BitValidator() { }

    wxObject *Clone() const { return new BitValidator(val, mask); }

    bool Copy(const BitValidator& o) {
	wxValidator::Copy(o);
	val = o.val;
	mask = o.mask;
	return true;
    }

    bool Validate(wxWindow *) { return true; }

    bool TransferToWindow() {
	if (!m_validatorWindow->IsKindOf(CLASSINFO(wxCheckBox)))
	    return false;
	((wxCheckBox*)m_validatorWindow)->SetValue(*val & mask);
	return true;
    }

    bool TransferFromWindow() {
	if (!m_validatorWindow->IsKindOf(CLASSINFO(wxCheckBox)))
	    return false;
	if (((wxCheckBox*)m_validatorWindow)->IsChecked())
	    *val |= mask;
	else
	    *val &= ~mask;
	return true;
    }
};

class svxPrintout : public wxPrintout {
    MainFrm *mainfrm;
    layout *m_layout;
    wxPageSetupDialogData* m_data;
    wxDC* pdc;
    wxFont *font_labels, *font_default;
    // Currently unused, but "skip blank pages" would use it.
    bool scan_for_blank_pages;

    wxPen *pen_frame, *pen_cross, *pen_leg, *pen_surface_leg, *pen_splay;
    wxColour colour_text, colour_labels;

    long x_t, y_t;
    double font_scaling_x, font_scaling_y;

    struct {
	long x_min, y_min, x_max, y_max;
    } clip;

    bool fBlankPage;

    int check_intersection(long x_p, long y_p);
    void draw_info_box();
    void draw_scale_bar(double x, double y, double MaxLength);
    int next_page(int *pstate, char **q, int pageLim);
    void drawticks(int tsize, int x, int y);

    void MOVEMM(double X, double Y) {
	MoveTo((long)(X * m_layout->scX), (long)(Y * m_layout->scY));
    }
    void DRAWMM(double X, double Y) {
	DrawTo((long)(X * m_layout->scX), (long)(Y * m_layout->scY));
    }
    void MoveTo(long x, long y);
    void DrawTo(long x, long y);
    void DrawCross(long x, long y);
    void SetFont(wxFont * font) {
	pdc->SetFont(*font);
    }
    void WriteString(const wxString & s);
    void DrawEllipse(long x, long y, long r, long R);
    void SolidRectangle(long x, long y, long w, long h);
    void NewPage(int pg, int pagesX, int pagesY);
    void PlotLR(const vector<XSect> & centreline);
    void PlotUD(const vector<XSect> & centreline);
  public:
    svxPrintout(MainFrm *mainfrm, layout *l, wxPageSetupDialogData *data, const wxString & title);
    bool OnPrintPage(int pageNum);
    void GetPageInfo(int *minPage, int *maxPage,
		     int *pageFrom, int *pageTo);
    bool HasPage(int pageNum);
    void OnBeginPrinting();
    void OnEndPrinting();
};

BEGIN_EVENT_TABLE(svxPrintDlg, wxDialog)
    EVT_CHOICE(svx_FORMAT, svxPrintDlg::OnChange)
    EVT_TEXT(svx_SCALE, svxPrintDlg::OnChange)
    EVT_COMBOBOX(svx_SCALE, svxPrintDlg::OnChange)
    EVT_SPINCTRLDOUBLE(svx_BEARING, svxPrintDlg::OnChangeSpin)
    EVT_SPINCTRLDOUBLE(svx_TILT, svxPrintDlg::OnChangeSpin)
    EVT_BUTTON(wxID_PRINT, svxPrintDlg::OnPrint)
    EVT_BUTTON(svx_EXPORT, svxPrintDlg::OnExport)
    EVT_BUTTON(wxID_CANCEL, svxPrintDlg::OnCancel)
#ifdef AVEN_PRINT_PREVIEW
    EVT_BUTTON(wxID_PREVIEW, svxPrintDlg::OnPreview)
#endif
    EVT_BUTTON(svx_PLAN, svxPrintDlg::OnPlan)
    EVT_BUTTON(svx_ELEV, svxPrintDlg::OnElevation)
    EVT_UPDATE_UI(svx_PLAN, svxPrintDlg::OnPlanUpdate)
    EVT_UPDATE_UI(svx_ELEV, svxPrintDlg::OnElevationUpdate)
    EVT_CHECKBOX(svx_LEGS, svxPrintDlg::OnChange)
    EVT_CHECKBOX(svx_STATIONS, svxPrintDlg::OnChange)
    EVT_CHECKBOX(svx_NAMES, svxPrintDlg::OnChange)
    EVT_CHECKBOX(svx_SURFACE, svxPrintDlg::OnChange)
    EVT_CHECKBOX(svx_SPLAYS, svxPrintDlg::OnChange)
    EVT_CHECKBOX(svx_ENTS, svxPrintDlg::OnChange)
    EVT_CHECKBOX(svx_FIXES, svxPrintDlg::OnChange)
    EVT_CHECKBOX(svx_EXPORTS, svxPrintDlg::OnChange)
END_EVENT_TABLE()

static wxString scales[] = {
    wxT(""),
    wxT("25"),
    wxT("50"),
    wxT("100"),
    wxT("250"),
    wxT("500"),
    wxT("1000"),
    wxT("2500"),
    wxT("5000"),
    wxT("10000"),
    wxT("25000"),
    wxT("50000"),
    wxT("100000")
};

// The order of these arrays must match export_format in export.h.

static wxString formats[] = {
    wxT("DXF"),
    wxT("EPS"),
    wxT("GPX"),
    wxT("HPGL"),
    wxT("JSON"),
    wxT("KML"),
    wxT("Plot"),
    wxT("Skencil"),
    wxT("Survex pos"),
    wxT("SVG")
};

#if 0
static wxString projs[] = {
    /* CUCC Austria: */
    wxT("+proj=tmerc +lat_0=0 +lon_0=13d20 +k=1 +x_0=0 +y_0=-5200000 +ellps=bessel +towgs84=577.326,90.129,463.919,5.137,1.474,5.297,2.4232"),
    /* British grid SD (Yorkshire): */
    wxT("+proj=tmerc +lat_0=49d +lon_0=-2d +k=0.999601 +x_0=100000 +y_0=-500000 +ellps=airy +towgs84=375,-111,431,0,0,0,0"),
    /* British full grid reference: */
    wxT("+proj=tmerc +lat_0=49d +lon_0=-2d +k=0.999601 +x_0=400000 +y_0=-100000 +ellps=airy +towgs84=375,-111,431,0,0,0,0")
};
#endif

static const unsigned format_info[] = {
    LABELS|LEGS|SURF|SPLAYS|STNS|PASG|XSECT|WALLS|MARKER_SIZE|TEXT_HEIGHT|GRID|FULL_COORDS,
    LABELS|LEGS|SURF|SPLAYS|STNS|PASG|XSECT|WALLS,
    LABELS|LEGS|SURF|SPLAYS|ENTS|FIXES|EXPORTS|PROJ|EXPORT_3D,
    LABELS|LEGS|SURF|SPLAYS|STNS|CENTRED,
    LEGS|SPLAYS|CENTRED|EXPORT_3D,
    LABELS|LEGS|SPLAYS|PASG|XSECT|WALLS|ENTS|FIXES|EXPORTS|PROJ|EXPORT_3D,
    LABELS|LEGS|SURF|SPLAYS,
    LABELS|LEGS|SURF|SPLAYS|STNS|MARKER_SIZE|GRID|SCALE,
    LABELS|ENTS|FIXES|EXPORTS|EXPORT_3D,
    LABELS|LEGS|SURF|SPLAYS|STNS|PASG|XSECT|WALLS|MARKER_SIZE|TEXT_HEIGHT|SCALE
};

static const char * extension[] = {
    ".dxf",
    ".eps",
    ".gpx",
    ".hpgl",
    ".json",
    ".kml",
    ".plt",
    ".sk",
    ".pos",
    ".svg"
};

static const int msg_filetype[] = {
    /*DXF files*/411,
    /*EPS files*/412,
    /*GPX files*/413,
    /* TRANSLATORS: Here "plotter" refers to a machine which draws a printout
     * on a (usually large) sheet of paper using a pen mounted in a motorised
     * mechanism. */
    /*HPGL for plotters*/414,
    /*JSON files*/445,
    /*KML files*/444,
    /* TRANSLATORS: "Compass" and "Carto" are the names of software packages,
     * so should not be translated:
     * http://www.fountainware.com/compass/
     * http://www.psc-cavers.org/carto/ */
    /*Compass PLT for use with Carto*/415,
    /* TRANSLATORS: "Skencil" is the name of a software package, so should not be
     * translated: http://www.skencil.org/ */
    /*Skencil files*/416,
    /* TRANSLATORS: Survex is the name of the software, and "pos" refers to a
     * file extension, so neither should be translated. */
    /*Survex pos files*/166,
    /*SVG files*/417
};

// We discriminate as "One Page" isn't valid for exporting.
static wxString default_scale_print;
static wxString default_scale_export;

svxPrintDlg::svxPrintDlg(MainFrm* mainfrm_, const wxString & filename,
			 const wxString & title, const wxString & cs_proj,
			 const wxString & datestamp,
			 double angle, double tilt_angle,
			 bool labels, bool crosses, bool legs, bool surf,
			 bool splays, bool tubes, bool ents, bool fixes,
			 bool exports, bool printing, bool close_after_)
	: wxDialog(mainfrm_, -1, wxString(printing ?
					  /* TRANSLATORS: Title of the print
					   * dialog */
					  wmsg(/*Print*/399) :
					  /* TRANSLATORS: Title of the export
					   * dialog */
					  wmsg(/*Export*/383))),
	  m_layout(printing ? wxGetApp().GetPageSetupDialogData() : NULL),
	  m_File(filename), mainfrm(mainfrm_), close_after(close_after_)
{
    m_scale = NULL;
    m_printSize = NULL;
    m_bearing = NULL;
    m_tilt = NULL;
    m_format = NULL;
    int show_mask = 0;
    if (labels)
	show_mask |= LABELS;
    if (crosses)
	show_mask |= STNS;
    if (legs)
	show_mask |= LEGS;
    if (surf)
	show_mask |= SURF;
    if (splays)
	show_mask |= SPLAYS;
    if (tubes)
	show_mask |= XSECT|WALLS|PASG;
    if (ents)
	show_mask |= ENTS;
    if (fixes)
	show_mask |= FIXES;
    if (exports)
	show_mask |= EXPORTS;
    m_layout.show_mask = show_mask;
    m_layout.datestamp = datestamp;
    m_layout.rot = angle;
    m_layout.title = title;
    m_layout.cs_proj = cs_proj;
    if (mainfrm->IsExtendedElevation()) {
	m_layout.view = layout::EXTELEV;
	if (m_layout.rot != 0.0 && m_layout.rot != 180.0) m_layout.rot = 0;
	m_layout.tilt = 0;
    } else {
	m_layout.tilt = tilt_angle;
	if (m_layout.tilt == -90.0) {
	    m_layout.view = layout::PLAN;
	} else if (m_layout.tilt == 0.0) {
	    m_layout.view = layout::ELEV;
	} else {
	    m_layout.view = layout::TILT;
	}
    }

    /* setup our print dialog*/
    wxBoxSizer* v1 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* h1 = new wxBoxSizer(wxHORIZONTAL); // holds controls
    /* TRANSLATORS: Used as a label for the surrounding box for the "Bearing"
     * and "Tilt angle" fields, and the "Plan view" and "Elevation" buttons in
     * the "what to print/export" dialog. */
    m_viewbox = new wxStaticBoxSizer(new wxStaticBox(this, -1, wmsg(/*View*/283)), wxVERTICAL);
    /* TRANSLATORS: Used as a label for the surrounding box for the "survey
     * legs" "stations" "names" etc checkboxes in the "what to print" dialog.
     * "Elements" isn’t a good name for this but nothing better has yet come to
     * mind! */
    wxBoxSizer* v3 = new wxStaticBoxSizer(new wxStaticBox(this, -1, wmsg(/*Elements*/256)), wxVERTICAL);
    wxBoxSizer* h2 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* h3 = new wxBoxSizer(wxHORIZONTAL); // holds buttons

    if (!printing) {
	wxStaticText* label;
	label = new wxStaticText(this, -1, wxString(wmsg(/*Export format*/410)));
	const size_t n_formats = sizeof(formats) / sizeof(formats[0]);
	m_format = new wxChoice(this, svx_FORMAT,
				wxDefaultPosition, wxDefaultSize,
				n_formats, formats);
	unsigned current_format = 0;
	wxConfigBase * cfg = wxConfigBase::Get();
	wxString s;
	if (cfg->Read(wxT("export_format"), &s, wxString())) {
	    for (unsigned i = 0; i != n_formats; ++i) {
		if (s == formats[i]) {
		    current_format = i;
		    break;
		}
	    }
	}
	m_format->SetSelection(current_format);
	wxBoxSizer* formatbox = new wxBoxSizer(wxHORIZONTAL);
	formatbox->Add(label, 0, wxALIGN_CENTRE_VERTICAL|wxALL, 5);
	formatbox->Add(m_format, 0, wxALIGN_CENTRE_VERTICAL|wxALL, 5);

	v1->Add(formatbox, 0, wxALIGN_LEFT|wxALL, 0);
    }

    wxStaticText* label;
    label = new wxStaticText(this, -1, wxString(wmsg(/*Scale*/154)) + wxT(" 1:"));
    if (printing && scales[0].empty()) {
	/* TRANSLATORS: used in the scale drop down selector in the print
	 * dialog the implicit meaning is "choose a suitable scale to fit
	 * the plot on a single page", but we need something shorter */
	scales[0].assign(wmsg(/*One page*/258));
    }
    wxString default_scale;
    if (printing) {
	default_scale = default_scale_print;
	if (default_scale.empty()) default_scale = scales[0];
    } else {
	default_scale = default_scale_export;
	if (default_scale.empty()) default_scale = wxT("1000");
    }
    const wxString* scale_list = scales;
    size_t n_scales = sizeof(scales) / sizeof(scales[0]);
    if (!printing) {
	++scale_list;
	--n_scales;
    }
    m_scale = new wxComboBox(this, svx_SCALE, default_scale, wxDefaultPosition,
			     wxDefaultSize, n_scales, scale_list);
    m_scalebox = new wxBoxSizer(wxHORIZONTAL);
    m_scalebox->Add(label, 0, wxALIGN_CENTRE_VERTICAL|wxALL, 5);
    m_scalebox->Add(m_scale, 0, wxALIGN_CENTRE_VERTICAL|wxALL, 5);

    m_viewbox->Add(m_scalebox, 0, wxALIGN_LEFT|wxALL, 0);

    if (printing) {
	// Make the dummy string wider than any sane value and use that to
	// fix the width of the control so the sizers allow space for bigger
	// page layouts.
	m_printSize = new wxStaticText(this, -1, wxString::Format(wmsg(/*%d pages (%dx%d)*/257), 9604, 98, 98));
	m_viewbox->Add(m_printSize, 0, wxALIGN_LEFT|wxALL, 5);
    }

    /* FIXME:
     * svx_GRID, // double - spacing, default: 100m
     * svx_TEXT_HEIGHT, // default 0.6
     * svx_MARKER_SIZE // default 0.8
     */

    if (m_layout.view != layout::EXTELEV) {
	wxFlexGridSizer* anglebox = new wxFlexGridSizer(2);
	wxStaticText * brg_label, * tilt_label;
	brg_label = new wxStaticText(this, -1, wmsg(/*Bearing*/259));
	anglebox->Add(brg_label, 0, wxALIGN_CENTRE_VERTICAL|wxALIGN_LEFT|wxALL, 5);
	// wSP_WRAP means that you can scroll past 360 to 0, and vice versa.
	m_bearing = new wxSpinCtrlDouble(this, svx_BEARING, wxEmptyString,
		wxDefaultPosition, wxDefaultSize,
		wxSP_ARROW_KEYS|wxALIGN_RIGHT|wxSP_WRAP);
	m_bearing->SetRange(0.0, 360.0);
	m_bearing->SetDigits(ANGLE_DP);
	anglebox->Add(m_bearing, 0, wxALIGN_CENTRE|wxALL, 5);
	/* TRANSLATORS: Used in the print dialog: */
	tilt_label = new wxStaticText(this, -1, wmsg(/*Tilt angle*/263));
	anglebox->Add(tilt_label, 0, wxALIGN_CENTRE_VERTICAL|wxALIGN_LEFT|wxALL, 5);
	m_tilt = new wxSpinCtrlDouble(this, svx_TILT);
	m_tilt->SetRange(-90.0, 90.0);
	m_tilt->SetDigits(ANGLE_DP);
	anglebox->Add(m_tilt, 0, wxALIGN_CENTRE|wxALL, 5);

	m_viewbox->Add(anglebox, 0, wxALIGN_LEFT|wxALL, 0);

	wxBoxSizer * planelevsizer = new wxBoxSizer(wxHORIZONTAL);
	planelevsizer->Add(new wxButton(this, svx_PLAN, wmsg(/*P&lan view*/117)),
			   0, wxALIGN_CENTRE_VERTICAL|wxALL, 5);
	planelevsizer->Add(new wxButton(this, svx_ELEV, wmsg(/*&Elevation*/285)),
			   0, wxALIGN_CENTRE_VERTICAL|wxALL, 5);

	m_viewbox->Add(planelevsizer, 0, wxALIGN_LEFT|wxALL, 5);
    }

    /* TRANSLATORS: Here a "survey leg" is a set of measurements between two
     * "survey stations". */
    v3->Add(new wxCheckBox(this, svx_LEGS, wmsg(/*Underground Survey Legs*/262),
			   wxDefaultPosition, wxDefaultSize, 0,
			   BitValidator(&m_layout.show_mask, LEGS)),
	    0, wxALIGN_LEFT|wxALL, 2);
    /* TRANSLATORS: Here a "survey leg" is a set of measurements between two
     * "survey stations". */
    v3->Add(new wxCheckBox(this, svx_SURFACE, wmsg(/*Sur&face Survey Legs*/403),
			   wxDefaultPosition, wxDefaultSize, 0,
			   BitValidator(&m_layout.show_mask, SURF)),
	    0, wxALIGN_LEFT|wxALL, 2);
    v3->Add(new wxCheckBox(this, svx_SPLAYS, wmsg(/*Spla&y Legs*/406),
			   wxDefaultPosition, wxDefaultSize, 0,
			   BitValidator(&m_layout.show_mask, SPLAYS)),
	    0, wxALIGN_LEFT|wxALL, 2);
    v3->Add(new wxCheckBox(this, svx_STATIONS, wmsg(/*Crosses*/261),
			   wxDefaultPosition, wxDefaultSize, 0,
			   BitValidator(&m_layout.show_mask, STNS)),
	    0, wxALIGN_LEFT|wxALL, 2);
    v3->Add(new wxCheckBox(this, svx_NAMES, wmsg(/*Station Names*/260),
			   wxDefaultPosition, wxDefaultSize, 0,
			   BitValidator(&m_layout.show_mask, LABELS)),
	    0, wxALIGN_LEFT|wxALL, 2);
    v3->Add(new wxCheckBox(this, svx_ENTS, wmsg(/*Entrances*/418),
			   wxDefaultPosition, wxDefaultSize, 0,
			   BitValidator(&m_layout.show_mask, ENTS)),
	    0, wxALIGN_LEFT|wxALL, 2);
    v3->Add(new wxCheckBox(this, svx_FIXES, wmsg(/*Fixed Points*/419),
			   wxDefaultPosition, wxDefaultSize, 0,
			   BitValidator(&m_layout.show_mask, FIXES)),
	    0, wxALIGN_LEFT|wxALL, 2);
    v3->Add(new wxCheckBox(this, svx_EXPORTS, wmsg(/*Exported Stations*/420),
			   wxDefaultPosition, wxDefaultSize, 0,
			   BitValidator(&m_layout.show_mask, EXPORTS)),
	    0, wxALIGN_LEFT|wxALL, 2);
    v3->Add(new wxCheckBox(this, svx_XSECT, wmsg(/*Cross-sections*/393),
			   wxDefaultPosition, wxDefaultSize, 0,
			   BitValidator(&m_layout.show_mask, XSECT)),
	    0, wxALIGN_LEFT|wxALL, 2);
    if (!printing) {
	v3->Add(new wxCheckBox(this, svx_WALLS, wmsg(/*Walls*/394),
			       wxDefaultPosition, wxDefaultSize, 0,
			       BitValidator(&m_layout.show_mask, WALLS)),
		0, wxALIGN_LEFT|wxALL, 2);
	// TRANSLATORS: Label for checkbox which controls whether there's a
	// layer in the exported file (for formats such as DXF and SVG)
	// containing polygons for the inside of cave passages).
	v3->Add(new wxCheckBox(this, svx_PASSAGES, wmsg(/*Passages*/395),
			       wxDefaultPosition, wxDefaultSize, 0,
			       BitValidator(&m_layout.show_mask, PASG)),
		0, wxALIGN_LEFT|wxALL, 2);
	v3->Add(new wxCheckBox(this, svx_CENTRED, wmsg(/*Origin in centre*/421),
			       wxDefaultPosition, wxDefaultSize, 0,
			       BitValidator(&m_layout.show_mask, CENTRED)),
		0, wxALIGN_LEFT|wxALL, 2);
	v3->Add(new wxCheckBox(this, svx_FULLCOORDS, wmsg(/*Full coordinates*/422),
			       wxDefaultPosition, wxDefaultSize, 0,
			       BitValidator(&m_layout.show_mask, FULL_COORDS)),
		0, wxALIGN_LEFT|wxALL, 2);
    }
    if (printing) {
	/* TRANSLATORS: used in the print dialog - controls drawing lines
	 * around each page */
	v3->Add(new wxCheckBox(this, svx_BORDERS, wmsg(/*Page Borders*/264),
			       wxDefaultPosition, wxDefaultSize, 0,
			       wxGenericValidator(&m_layout.Border)),
		0, wxALIGN_LEFT|wxALL, 2);
	/* TRANSLATORS: will be used in the print dialog - check this to print
	 * blank pages (otherwise they’ll be skipped to save paper) */
//	m_blanks = new wxCheckBox(this, svx_BLANKS, wmsg(/*Blank Pages*/266));
//	v3->Add(m_blanks, 0, wxALIGN_LEFT|wxALL, 2);
	/* TRANSLATORS: As in the legend on a map.  Used in the print dialog -
	 * controls drawing the box at the lower left with survey name, view
	 * angles, etc */
	v3->Add(new wxCheckBox(this, svx_LEGEND, wmsg(/*Legend*/265),
			       wxDefaultPosition, wxDefaultSize, 0,
			       wxGenericValidator(&m_layout.Legend)),
		0, wxALIGN_LEFT|wxALL, 2);
    }

    h1->Add(v3, 0, wxALIGN_LEFT|wxALL, 5);
    h1->Add(m_viewbox, 0, wxALIGN_LEFT|wxLEFT, 5);

    if (!printing) {
	/* TRANSLATORS: The PROJ library is used to do coordinate
	 * transformations (https://trac.osgeo.org/proj/) - if the .3d file
	 * doesn't contain details of the coordinate projection in use, the
	 * user must specify it here for export formats which need to know it
	 * (e.g. GPX).
	 */
	h2->Add(new wxStaticText(this, svx_PROJ_LABEL, wmsg(/*Coordinate projection*/440)),
		0, wxLEFT|wxALIGN_CENTRE_VERTICAL, 5);
	long style = 0;
	if (!m_layout.cs_proj.empty()) {
	    // If the input file specified the coordinate system, don't let the
	    // user mess with it.
	    style = wxTE_READONLY;
	} else {
#if 0 // FIXME: Is it a good idea to save this?
	    wxConfigBase * cfg = wxConfigBase::Get();
	    wxString input_projection;
	    cfg->Read(wxT("input_projection"), &input_projection);
	    if (!input_projection.empty())
		proj_edit.SetValue(input_projection);
#endif
	}
	wxTextCtrl * proj_edit = new wxTextCtrl(this, svx_PROJ, m_layout.cs_proj,
						wxDefaultPosition, wxDefaultSize,
						style);
	h2->Add(proj_edit, 1, wxALL|wxEXPAND, 5);
	v1->Add(h2, 0, wxALIGN_LEFT|wxEXPAND, 5);
    }

    v1->Add(h1, 0, wxALIGN_LEFT|wxALL, 5);

    // When we enable/disable checkboxes in the export dialog, ideally we'd
    // like the dialog to resize, but not sure how to achieve that, so we
    // add a stretchable spacer here so at least the buttons stay in the
    // lower right corner.
    v1->AddStretchSpacer();

    wxButton * but;
    but = new wxButton(this, wxID_CANCEL);
    h3->Add(but, 0, wxALL, 5);
    if (printing) {
#ifdef AVEN_PRINT_PREVIEW
	but = new wxButton(this, wxID_PREVIEW);
	h3->Add(but, 0, wxALL, 5);
	but = new wxButton(this, wxID_PRINT);
#else
	but = new wxButton(this, wxID_PRINT, wmsg(/*&Print...*/400));
#endif
    } else {
	/* TRANSLATORS: The text on the action button in the "Export" settings
	 * dialog */
	but = new wxButton(this, svx_EXPORT, wmsg(/*&Export...*/230));
    }
    but->SetDefault();
    h3->Add(but, 0, wxALL, 5);
    v1->Add(h3, 0, wxALIGN_RIGHT|wxALL, 5);

    SetAutoLayout(true);
    SetSizer(v1);
    v1->SetSizeHints(this);

    LayoutToUI();
    SomethingChanged(0);
}

void
svxPrintDlg::OnPrint(wxCommandEvent&) {
    SomethingChanged(0);
    TransferDataFromWindow();
    wxPageSetupDialogData * psdd = wxGetApp().GetPageSetupDialogData();
    wxPrintDialogData pd(psdd->GetPrintData());
    wxPrinter pr(&pd);
    svxPrintout po(mainfrm, &m_layout, psdd, m_File);
    if (m_layout.SkipBlank) {
	// FIXME: wx's printing requires a contiguous range of valid page
	// numbers.  To achieve that, we need to run a scan for blank pages
	// here, so that GetPageInfo() knows what range to return, and so
	// that OnPrintPage() can map a page number back to where in the
	// MxN multi-page layout.
#if 0
	po.scan_for_blank_pages = true;
	for (int page = 1; page <= m_layout->pages; ++page) {
	    po.fBlankPage = fTrue;
	    po.OnPrintPage(page);
	    // FIXME: Do something with po.fBlankPage
	}
	po.scan_for_blank_pages = false;
#endif
    }
    if (pr.Print(this, &po, true)) {
	// Close the print dialog if printing succeeded.
	Destroy();
    }
}

void
svxPrintDlg::OnExport(wxCommandEvent&) {
    UIToLayout();
    TransferDataFromWindow();
    wxString leaf;
    wxFileName::SplitPath(m_File, NULL, NULL, &leaf, NULL, wxPATH_NATIVE);
    unsigned format_idx = ((wxChoice*)FindWindow(svx_FORMAT))->GetSelection();
    leaf += wxString::FromUTF8(extension[format_idx]);

    wxString filespec = wmsg(msg_filetype[format_idx]);
    filespec += wxT("|*");
    filespec += wxString::FromUTF8(extension[format_idx]);
    filespec += wxT("|");
    filespec += wmsg(/*All files*/208);
    filespec += wxT("|");
    filespec += wxFileSelectorDefaultWildcardStr;

    /* TRANSLATORS: Title of file dialog to choose name and type of exported
     * file. */
    wxFileDialog dlg(this, wmsg(/*Export as:*/401), wxString(), leaf,
		     filespec, wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() == wxID_OK) {
	wxString input_projection = ((wxTextCtrl*)FindWindow(svx_PROJ))->GetValue();
	double grid = 100; // metres
	double text_height = 0.6;
	double marker_size = 0.8;

	try {
	    const wxString& export_fnm = dlg.GetPath();
	    unsigned mask = format_info[format_idx];
	    double rot, tilt;
	    if (mask & EXPORT_3D) {
		rot = 0.0;
		tilt = -90.0;
	    } else {
		rot = m_layout.rot;
		tilt = m_layout.tilt;
	    }
	    if (!Export(export_fnm, m_layout.title,
			m_layout.datestamp, *mainfrm,
			rot, tilt, m_layout.get_effective_show_mask(),
			export_format(format_idx), input_projection.utf8_str(),
			grid, text_height, marker_size, m_layout.Scale)) {
		wxString m = wxString::Format(wmsg(/*Couldn’t write file “%s”*/402).c_str(),
					      export_fnm.c_str());
		wxGetApp().ReportError(m);
	    }
	} catch (const wxString & m) {
	    wxGetApp().ReportError(m);
	}
    }
    Destroy();
}

#ifdef AVEN_PRINT_PREVIEW
void
svxPrintDlg::OnPreview(wxCommandEvent&) {
    SomethingChanged(0);
    TransferDataFromWindow();
    wxPageSetupDialogData * psdd = wxGetApp().GetPageSetupDialogData();
    wxPrintDialogData pd(psdd->GetPrintData());
    wxPrintPreview* pv;
    pv = new wxPrintPreview(new svxPrintout(mainfrm, &m_layout, psdd, m_File),
			    new svxPrintout(mainfrm, &m_layout, psdd, m_File),
			    &pd);
    // TRANSLATORS: Title of the print preview dialog
    wxPreviewFrame *frame = new wxPreviewFrame(pv, mainfrm, wmsg(/*Print Preview*/398));
    frame->Initialize();

    // Size preview frame so that all of the controlbar and canvas can be seen
    // if possible.
    int w, h;
    // GetBestSize gives us the width needed to show the whole controlbar.
    frame->GetBestSize(&w, &h);
    if (h < w) {
	// On wxGTK at least, GetBestSize() returns much too small a height.
	h = w * 6 / 5;
    }
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
#endif

void
svxPrintDlg::OnPlan(wxCommandEvent&) {
    m_tilt->SetValue(-90.0);
    SomethingChanged(svx_TILT);
}

void
svxPrintDlg::OnElevation(wxCommandEvent&) {
    m_tilt->SetValue(0.0);
    SomethingChanged(svx_TILT);
}

void
svxPrintDlg::OnPlanUpdate(wxUpdateUIEvent& e) {
    e.Enable(m_tilt->GetValue() != -90.0);
}

void
svxPrintDlg::OnElevationUpdate(wxUpdateUIEvent& e) {
    e.Enable(m_tilt->GetValue() != 0.0);
}

void
svxPrintDlg::OnChangeSpin(wxSpinDoubleEvent& e) {
    SomethingChanged(e.GetId());
}

void
svxPrintDlg::OnChange(wxCommandEvent& e) {
    if (e.GetId() == svx_SCALE && m_scale) {
	default_scale_print = m_scale->GetValue();
	if (default_scale_print != scales[0]) {
	    // Don't store "One Page" for use when exporting.
	    default_scale_export = default_scale_print;
	}
    }
    SomethingChanged(e.GetId());
}

void
svxPrintDlg::OnCancel(wxCommandEvent&) {
    if (close_after)
	mainfrm->Close();
    Destroy();
}

void
svxPrintDlg::SomethingChanged(int control_id) {
    if ((control_id == 0 || control_id == svx_FORMAT) && m_format) {
	// Update the shown/hidden fields for the newly selected export filter.
	int new_filter_idx = m_format->GetSelection();
	if (new_filter_idx != wxNOT_FOUND) {
	    unsigned mask = format_info[new_filter_idx];
	    static const struct { int id; unsigned mask; } controls[] = {
		{ svx_LEGS, LEGS },
		{ svx_SURFACE, SURF },
		{ svx_SPLAYS, SPLAYS },
		{ svx_STATIONS, STNS },
		{ svx_NAMES, LABELS },
		{ svx_XSECT, XSECT },
		{ svx_WALLS, WALLS },
		{ svx_PASSAGES, PASG },
		{ svx_ENTS, ENTS },
		{ svx_FIXES, FIXES },
		{ svx_EXPORTS, EXPORTS },
		{ svx_CENTRED, CENTRED },
		{ svx_FULLCOORDS, FULL_COORDS },
		{ svx_PROJ_LABEL, PROJ },
		{ svx_PROJ, PROJ },
	    };
	    static unsigned n_controls = sizeof(controls) / sizeof(controls[0]);
	    for (unsigned i = 0; i != n_controls; ++i) {
		wxWindow * control = FindWindow(controls[i].id);
		if (control) control->Show(mask & controls[i].mask);
	    }
	    m_scalebox->Show(bool(mask & SCALE));
	    m_viewbox->Show(!bool(mask & EXPORT_3D));
	    GetSizer()->Layout();
	    if (control_id == svx_FORMAT) {
		wxConfigBase * cfg = wxConfigBase::Get();
		cfg->Write(wxT("export_format"), formats[new_filter_idx]);
	    }
	}
    }

    UIToLayout();

    if (m_printSize || m_scale) {
	// Update the bounding box.
	RecalcBounds();

	if (m_scale) {
	    if (!(m_scale->GetValue()).ToDouble(&(m_layout.Scale)) ||
		m_layout.Scale == 0.0) {
		m_layout.pick_scale(1, 1);
	    }
	}
    }

    if (m_printSize && m_layout.xMax >= m_layout.xMin) {
	m_layout.pages_required();
	m_printSize->SetLabel(wxString::Format(wmsg(/*%d pages (%dx%d)*/257), m_layout.pages, m_layout.pagesX, m_layout.pagesY));
    }
}

void
svxPrintDlg::LayoutToUI()
{
//    m_blanks->SetValue(m_layout.SkipBlank);
    if (m_layout.view != layout::EXTELEV) {
	m_tilt->SetValue(m_layout.tilt);
	m_bearing->SetValue(m_layout.rot);
    }

    if (m_scale && m_layout.Scale != 0) {
	// Do this last as it causes an OnChange message which calls UIToLayout
	wxString temp;
	temp << m_layout.Scale;
	m_scale->SetValue(temp);
    }
}

void
svxPrintDlg::UIToLayout()
{
//    m_layout.SkipBlank = m_blanks->IsChecked();

    if (m_layout.view != layout::EXTELEV && m_tilt) {
	m_layout.tilt = m_tilt->GetValue();
	if (m_layout.tilt == -90.0) {
	    m_layout.view = layout::PLAN;
	} else if (m_layout.tilt == 0.0) {
	    m_layout.view = layout::ELEV;
	} else {
	    m_layout.view = layout::TILT;
	}

	bool enable_passage_opts = (m_layout.view != layout::TILT);
	wxWindow * win;
	win = FindWindow(svx_XSECT);
	if (win) win->Enable(enable_passage_opts);
	win = FindWindow(svx_WALLS);
	if (win) win->Enable(enable_passage_opts);
	win = FindWindow(svx_PASSAGES);
	if (win) win->Enable(enable_passage_opts);

	m_layout.rot = m_bearing->GetValue();
    }
}

void
svxPrintDlg::RecalcBounds()
{
    m_layout.yMax = m_layout.xMax = -DBL_MAX;
    m_layout.yMin = m_layout.xMin = DBL_MAX;

    double SIN = sin(rad(m_layout.rot));
    double COS = cos(rad(m_layout.rot));
    double SINT = sin(rad(m_layout.tilt));
    double COST = cos(rad(m_layout.tilt));

    int show_mask = m_layout.get_effective_show_mask();
    if (show_mask & LEGS) {
	for (int f = 0; f != 8; ++f) {
	    if ((show_mask & (f & img_FLAG_SURFACE) ? SURF : LEGS) == 0) {
		// Not showing traverse because of surface/underground status.
		continue;
	    }
	    if ((f & img_FLAG_SPLAY) && (show_mask & SPLAYS) == 0) {
		// Not showing because it's a splay.
		continue;
	    }
	    list<traverse>::const_iterator trav = mainfrm->traverses_begin(f);
	    list<traverse>::const_iterator tend = mainfrm->traverses_end(f);
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
		    double Y = z * COST - (x * SIN + y * COS) * SINT;
		    if (Y > m_layout.yMax) m_layout.yMax = Y;
		    if (Y < m_layout.yMin) m_layout.yMin = Y;
		}
	    }
	}
    }

    if ((show_mask & XSECT) &&
	(m_layout.tilt == 0.0 || m_layout.tilt == 90.0 || m_layout.tilt == -90.0)) {
	list<vector<XSect> >::const_iterator trav = mainfrm->tubes_begin();
	list<vector<XSect> >::const_iterator tend = mainfrm->tubes_end();
	for ( ; trav != tend; ++trav) {
	    XSect prev_pt_v;
	    Vector3 last_right(1.0, 0.0, 0.0);

	    vector<XSect>::const_iterator i = trav->begin();
	    vector<XSect>::size_type segment = 0;
	    while (i != trav->end()) {
		// get the coordinates of this vertex
		const XSect & pt_v = *i++;
		if (m_layout.tilt == 0.0) {
		    Double u = pt_v.GetU();
		    Double d = pt_v.GetD();

		    if (u >= 0 || d >= 0) {
			double x = pt_v.GetX();
			double y = pt_v.GetY();
			double z = pt_v.GetZ();
			double X = x * COS - y * SIN;
			double Y = z * COST - (x * SIN + y * COS) * SINT;

			if (X > m_layout.xMax) m_layout.xMax = X;
			if (X < m_layout.xMin) m_layout.xMin = X;
			double U = Y + max(0.0, pt_v.GetU());
			if (U > m_layout.yMax) m_layout.yMax = U;
			double D = Y - max(0.0, pt_v.GetD());
			if (D < m_layout.yMin) m_layout.yMin = D;
		    }
		} else {
		    // More complex, and this duplicates the algorithm from
		    // PlotLR() - we should try to share that, maybe via a
		    // template.
		    Vector3 right;

		    const Vector3 up_v(0.0, 0.0, 1.0);

		    if (segment == 0) {
			assert(i != trav->end());
			// first segment

			// get the coordinates of the next vertex
			const XSect & next_pt_v = *i;

			// calculate vector from this pt to the next one
			Vector3 leg_v = next_pt_v - pt_v;

			// obtain a vector in the LRUD plane
			right = leg_v * up_v;
			if (right.magnitude() == 0) {
			    right = last_right;
			} else {
			    last_right = right;
			}
		    } else if (segment + 1 == trav->size()) {
			// last segment

			// Calculate vector from the previous pt to this one.
			Vector3 leg_v = pt_v - prev_pt_v;

			// Obtain a horizontal vector in the LRUD plane.
			right = leg_v * up_v;
			if (right.magnitude() == 0) {
			    right = Vector3(last_right.GetX(), last_right.GetY(), 0.0);
			} else {
			    last_right = right;
			}
		    } else {
			assert(i != trav->end());
			// Intermediate segment.

			// Get the coordinates of the next vertex.
			const XSect & next_pt_v = *i;

			// Calculate vectors from this vertex to the
			// next vertex, and from the previous vertex to
			// this one.
			Vector3 leg1_v = pt_v - prev_pt_v;
			Vector3 leg2_v = next_pt_v - pt_v;

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

		    if (l >= 0 || r >= 0) {
			// Get the x and y coordinates of the survey station
			double pt_X = pt_v.GetX() * COS - pt_v.GetY() * SIN;
			double pt_Y = pt_v.GetX() * SIN + pt_v.GetY() * COS;

			double X, Y;
			if (l >= 0) {
			    // Get the x and y coordinates of the end of the left arrow
			    Vector3 p = pt_v - right * l;
			    X = p.GetX() * COS - p.GetY() * SIN;
			    Y = (p.GetX() * SIN + p.GetY() * COS);
			} else {
			    X = pt_X;
			    Y = pt_Y;
			}
			if (X > m_layout.xMax) m_layout.xMax = X;
			if (X < m_layout.xMin) m_layout.xMin = X;
			if (Y > m_layout.yMax) m_layout.yMax = Y;
			if (Y < m_layout.yMin) m_layout.yMin = Y;

			if (r >= 0) {
			    // Get the x and y coordinates of the end of the right arrow
			    Vector3 p = pt_v + right * r;
			    X = p.GetX() * COS - p.GetY() * SIN;
			    Y = (p.GetX() * SIN + p.GetY() * COS);
			} else {
			    X = pt_X;
			    Y = pt_Y;
			}
			if (X > m_layout.xMax) m_layout.xMax = X;
			if (X < m_layout.xMin) m_layout.xMin = X;
			if (Y > m_layout.yMax) m_layout.yMax = Y;
			if (Y < m_layout.yMin) m_layout.yMin = Y;
		    }

		    prev_pt_v = pt_v;

		    ++segment;
		}
	    }
	}
    }

    if (show_mask & (LABELS|STNS)) {
	list<LabelInfo*>::const_iterator label = mainfrm->GetLabels();
	while (label != mainfrm->GetLabelsEnd()) {
	    double x = (*label)->GetX();
	    double y = (*label)->GetY();
	    double z = (*label)->GetZ();
	    if ((show_mask & SURF) || (*label)->IsUnderground()) {
		double X = x * COS - y * SIN;
		if (X > m_layout.xMax) m_layout.xMax = X;
		if (X < m_layout.xMin) m_layout.xMin = X;
		double Y = z * COST - (x * SIN + y * COS) * SINT;
		if (Y > m_layout.yMax) m_layout.yMax = Y;
		if (Y < m_layout.yMin) m_layout.yMin = Y;
	    }
	    ++label;
	}
    }
}

static int xpPageWidth, ypPageDepth;
static long x_offset, y_offset;
static int fontsize, fontsize_labels;

/* FIXME: allow the font to be set */

static const char *fontname = "Arial", *fontname_labels = "Arial";

svxPrintout::svxPrintout(MainFrm *mainfrm_, layout *l,
			 wxPageSetupDialogData *data, const wxString & title)
    : wxPrintout(title), font_labels(NULL), font_default(NULL),
      scan_for_blank_pages(false)
{
    mainfrm = mainfrm_;
    m_layout = l;
    m_data = data;
}

void
svxPrintout::draw_info_box()
{
   layout *l = m_layout;
   int boxwidth = 70;
   int boxheight = 30;

   pdc->SetPen(*pen_frame);

   int div = boxwidth;
   if (l->view != layout::EXTELEV) {
      boxwidth += boxheight;
      MOVEMM(div, boxheight);
      DRAWMM(div, 0);
      MOVEMM(0, 30); DRAWMM(div, 30);
   }

   MOVEMM(0, boxheight);
   DRAWMM(boxwidth, boxheight);
   DRAWMM(boxwidth, 0);
   if (!l->Border) {
      DRAWMM(0, 0);
      DRAWMM(0, boxheight);
   }

   MOVEMM(0, 20); DRAWMM(div, 20);
   MOVEMM(0, 10); DRAWMM(div, 10);

   switch (l->view) {
    case layout::PLAN: {
      long ax, ay, bx, by, cx, cy, dx, dy;

      long xc = boxwidth - boxheight / 2;
      long yc = boxheight / 2;
      const double RADIUS = boxheight / 3;
      DrawEllipse(long(xc * l->scX), long(yc * l->scY),
		  long(RADIUS * l->scX), long(RADIUS * l->scY));

      ax = (long)((xc - (RADIUS - 1) * sin(rad(000.0 + l->rot))) * l->scX);
      ay = (long)((yc + (RADIUS - 1) * cos(rad(000.0 + l->rot))) * l->scY);
      bx = (long)((xc - RADIUS * 0.5 * sin(rad(180.0 + l->rot))) * l->scX);
      by = (long)((yc + RADIUS * 0.5 * cos(rad(180.0 + l->rot))) * l->scY);
      cx = (long)((xc - (RADIUS - 1) * sin(rad(160.0 + l->rot))) * l->scX);
      cy = (long)((yc + (RADIUS - 1) * cos(rad(160.0 + l->rot))) * l->scY);
      dx = (long)((xc - (RADIUS - 1) * sin(rad(200.0 + l->rot))) * l->scX);
      dy = (long)((yc + (RADIUS - 1) * cos(rad(200.0 + l->rot))) * l->scY);

      MoveTo(ax, ay);
      DrawTo(bx, by);
      DrawTo(cx, cy);
      DrawTo(ax, ay);
      DrawTo(dx, dy);
      DrawTo(bx, by);

      pdc->SetTextForeground(colour_text);
      MOVEMM(div + 0.5, boxheight - 5.5);
      WriteString(wmsg(/*North*/115));

      wxString angle = format_angle(ANGLE_FMT, l->rot);
      wxString s;
      /* TRANSLATORS: This is used on printouts of plans, with %s replaced by
       * something like "123°".  The bearing is up the page. */
      s.Printf(wmsg(/*Plan view, %s up page*/168), angle.c_str());
      MOVEMM(2, 12); WriteString(s);
      break;
    }
    case layout::ELEV: case layout::TILT: {
      const int L = div + 2;
      const int R = boxwidth - 2;
      const int H = boxheight / 2;
      MOVEMM(L, H); DRAWMM(L + 5, H - 3); DRAWMM(L + 3, H); DRAWMM(L + 5, H + 3);

      DRAWMM(L, H); DRAWMM(R, H);

      DRAWMM(R - 5, H + 3); DRAWMM(R - 3, H); DRAWMM(R - 5, H - 3); DRAWMM(R, H);

      MOVEMM((L + R) / 2, H - 2); DRAWMM((L + R) / 2, H + 2);

      pdc->SetTextForeground(colour_text);
      MOVEMM(div + 2, boxheight - 8);
      /* TRANSLATORS: "Elevation on" 020 <-> 200 degrees */
      WriteString(wmsg(/*Elevation on*/116));

      MOVEMM(L, 2);
      WriteString(format_angle(ANGLE_FMT, fmod(l->rot + 270.0, 360.0)));
      MOVEMM(R - 10, 2);
      WriteString(format_angle(ANGLE_FMT, fmod(l->rot + 90.0, 360.0)));

      wxString angle = format_angle(ANGLE_FMT, l->rot);
      wxString s;
      if (l->view == layout::ELEV) {
	  /* TRANSLATORS: This is used on printouts of elevations, with %s
	   * replaced by something like "123°".  The bearing is the direction
	   * we’re looking. */
	  s.Printf(wmsg(/*Elevation facing %s*/169), angle.c_str());
      } else {
	  wxString a2 = format_angle(ANGLE2_FMT, l->tilt);
	  /* TRANSLATORS: This is used on printouts of tilted elevations, with
	   * the first %s replaced by something like "123°", and the second by
	   * something like "-45°".  The bearing is the direction we’re
	   * looking. */
	  s.Printf(wmsg(/*Elevation facing %s, tilted %s*/284), angle.c_str(), a2.c_str());
      }
      MOVEMM(2, 12); WriteString(s);
      break;
    }
    case layout::EXTELEV:
      pdc->SetTextForeground(colour_text);
      MOVEMM(2, 12);
      /* TRANSLATORS: This is used on printouts of extended elevations. */
      WriteString(wmsg(/*Extended elevation*/191));
      break;
   }

   MOVEMM(2, boxheight - 8); WriteString(l->title);

   MOVEMM(2, 2);
   // FIXME: "Original Scale" better?
   WriteString(wxString::Format(wmsg(/*Scale*/154) + wxT(" 1:%.0f"),
				l->Scale));

   /* This used to be a copyright line, but it was occasionally
    * mis-interpreted as us claiming copyright on the survey, so let's
    * give the website URL instead */
   MOVEMM(boxwidth + 2, 2);
   WriteString(wxT("Survex " VERSION " - https://survex.com/"));

   draw_scale_bar(boxwidth + 10.0, 17.0, l->PaperWidth - boxwidth - 18.0);
}

/* Draw fancy scale bar with bottom left at (x,y) (both in mm) and at most */
/* MaxLength mm long. The scaling in use is 1:scale */
void
svxPrintout::draw_scale_bar(double x, double y, double MaxLength)
{
   double StepEst, d;
   int E, Step, n, c;
   wxString buf;
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

   /* FIXME: Non-metric units here... */
   /* Choose appropriate units, s.t. if possible E is >=0 and minimized */
   int units;
   if (E >= 3) {
      E -= 3;
      units = /*km*/423;
   } else if (E >= 0) {
      units = /*m*/424;
   } else {
      E += 2;
      units = /*cm*/425;
   }

   buf = wmsg(/*Scale*/154);

   /* Add units used - eg. "Scale (10m)" */
   double pow10_E = pow(10.0, (double)E);
   if (E >= 0) {
      buf += wxString::Format(wxT(" (%.f%s)"), pow10_E, wmsg(units).c_str());
   } else {
      int sf = -(int)floor(E);
      buf += wxString::Format(wxT(" (%.*f%s)"), sf, pow10_E, wmsg(units).c_str());
   }
   pdc->SetTextForeground(colour_text);
   MOVEMM(x, y + 4); WriteString(buf);

   /* Work out how many divisions there will be */
   n = (int)(MaxLength / d);

   pdc->SetPen(*pen_frame);

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
      pdc->SetPen(*pen_frame);
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
      buf.Printf(wxT("%d"), c * Step);
      pdc->SetTextForeground(colour_text);
      MOVEMM(x + c * d - buf.length(), y - 5);
      WriteString(buf);
   }
}

#if 0
void
make_calibration(layout *l) {
      img_point pt = { 0.0, 0.0, 0.0 };
      l->xMax = l->yMax = 0.1;
      l->xMin = l->yMin = 0;

      stack(l, img_MOVE, NULL, &pt);
      pt.x = 0.1;
      stack(l, img_LINE, NULL, &pt);
      pt.y = 0.1;
      stack(l, img_LINE, NULL, &pt);
      pt.x = 0.0;
      stack(l, img_LINE, NULL, &pt);
      pt.y = 0.0;
      stack(l, img_LINE, NULL, &pt);
      pt.x = 0.05;
      pt.y = 0.001;
      stack(l, img_LABEL, "10cm", &pt);
      pt.x = 0.001;
      pt.y = 0.05;
      stack(l, img_LABEL, "10cm", &pt);
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
      wxASSERT(*p == '-');
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
svxPrintout::drawticks(int tsize, int x, int y)
{
   long i;
   int s = tsize * 4;
   int o = s / 8;
   bool fAtCorner = fFalse;
   pdc->SetPen(*pen_frame);
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
    pdc->SetBackgroundMode(wxTRANSPARENT);
#ifdef AVEN_PRINT_PREVIEW
    if (IsPreview()) {
	int dcx, dcy;
	pdc->GetSize(&dcx, &dcy);
	pdc->SetUserScale((double)dcx / xpPageWidth, (double)dcy / ypPageDepth);
    }
#endif

    layout * l = m_layout;
    {
	int pwidth, pdepth;
	GetPageSizeMM(&pwidth, &pdepth);
	l->scX = (double)xpPageWidth / pwidth;
	l->scY = (double)ypPageDepth / pdepth;
	font_scaling_x = l->scX * (25.4 / 72.0);
	font_scaling_y = l->scY * (25.4 / 72.0);
	long MarginLeft = m_data->GetMarginTopLeft().x;
	long MarginTop = m_data->GetMarginTopLeft().y;
	long MarginBottom = m_data->GetMarginBottomRight().y;
	long MarginRight = m_data->GetMarginBottomRight().x;
	xpPageWidth -= (int)(l->scX * (MarginLeft + MarginRight));
	ypPageDepth -= (int)(l->scY * (FOOTER_HEIGHT_MM + MarginBottom + MarginTop));
	// xpPageWidth -= 1;
	pdepth -= FOOTER_HEIGHT_MM;
	x_offset = (long)(l->scX * MarginLeft);
	y_offset = (long)(l->scY * MarginTop);
	l->PaperWidth = pwidth -= MarginLeft + MarginRight;
	l->PaperDepth = pdepth -= MarginTop + MarginBottom;
    }

    double SIN = sin(rad(l->rot));
    double COS = cos(rad(l->rot));
    double SINT = sin(rad(l->tilt));
    double COST = cos(rad(l->tilt));

    NewPage(pageNum, l->pagesX, l->pagesY);

    if (l->Legend && pageNum == (l->pagesY - 1) * l->pagesX + 1) {
	SetFont(font_default);
	draw_info_box();
    }

    pdc->SetClippingRegion(x_offset, y_offset, xpPageWidth + 1, ypPageDepth + 1);

    const double Sc = 1000 / l->Scale;

    int show_mask = l->get_effective_show_mask();
    if (show_mask & (LEGS|SURF)) {
	for (int f = 0; f != 8; ++f) {
	    if ((show_mask & (f & img_FLAG_SURFACE) ? SURF : LEGS) == 0) {
		// Not showing traverse because of surface/underground status.
		continue;
	    }
	    if ((f & img_FLAG_SPLAY) && (show_mask & SPLAYS) == 0) {
		// Not showing because it's a splay.
		continue;
	    }
	    if (f & img_FLAG_SPLAY) {
		pdc->SetPen(*pen_splay);
	    } else if (f & img_FLAG_SURFACE) {
		pdc->SetPen(*pen_surface_leg);
	    } else {
		pdc->SetPen(*pen_leg);
	    }
	    list<traverse>::const_iterator trav = mainfrm->traverses_begin(f);
	    list<traverse>::const_iterator tend = mainfrm->traverses_end(f);
	    for ( ; trav != tend; ++trav) {
		vector<PointInfo>::const_iterator pos = trav->begin();
		vector<PointInfo>::const_iterator end = trav->end();
		for ( ; pos != end; ++pos) {
		    double x = pos->GetX();
		    double y = pos->GetY();
		    double z = pos->GetZ();
		    double X = x * COS - y * SIN;
		    double Y = z * COST - (x * SIN + y * COS) * SINT;
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
    }

    if ((show_mask & XSECT) &&
	(l->tilt == 0.0 || l->tilt == 90.0 || l->tilt == -90.0)) {
	pdc->SetPen(*pen_splay);
	list<vector<XSect> >::const_iterator trav = mainfrm->tubes_begin();
	list<vector<XSect> >::const_iterator tend = mainfrm->tubes_end();
	for ( ; trav != tend; ++trav) {
	    if (l->tilt == 0.0) {
		PlotUD(*trav);
	    } else {
		// m_layout.tilt is 90.0 or -90.0 due to check above.
		PlotLR(*trav);
	    }
	}
    }

    if (show_mask & (LABELS|STNS)) {
	if (show_mask & LABELS) SetFont(font_labels);
	list<LabelInfo*>::const_iterator label = mainfrm->GetLabels();
	while (label != mainfrm->GetLabelsEnd()) {
	    double px = (*label)->GetX();
	    double py = (*label)->GetY();
	    double pz = (*label)->GetZ();
	    if ((show_mask & SURF) || (*label)->IsUnderground()) {
		double X = px * COS - py * SIN;
		double Y = pz * COST - (px * SIN + py * COS) * SINT;
		long xnew, ynew;
		xnew = (long)((X * Sc + l->xOrg) * l->scX);
		ynew = (long)((Y * Sc + l->yOrg) * l->scY);
		if (show_mask & STNS) {
		    pdc->SetPen(*pen_cross);
		    DrawCross(xnew, ynew);
		}
		if (show_mask & LABELS) {
		    pdc->SetTextForeground(colour_labels);
		    MoveTo(xnew, ynew);
		    WriteString((*label)->GetText());
		}
	    }
	    ++label;
	}
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

bool
svxPrintout::HasPage(int pageNum) {
    return (pageNum <= m_layout->pages);
}

void
svxPrintout::OnBeginPrinting() {
    /* Initialise printer routines */
    fontsize_labels = 10;
    fontsize = 10;

    colour_text = colour_labels = *wxBLACK;

    wxColour colour_frame, colour_cross, colour_leg, colour_surface_leg;
    colour_frame = colour_cross = colour_leg = colour_surface_leg = *wxBLACK;

    pen_frame = new wxPen(colour_frame);
    pen_cross = new wxPen(colour_cross);
    pen_leg = new wxPen(colour_leg);
    pen_surface_leg = new wxPen(colour_surface_leg);
    pen_splay = new wxPen(wxColour(128, 128, 128));

    m_layout->scX = 1;
    m_layout->scY = 1;

    font_labels = new wxFont(fontsize_labels, wxFONTFAMILY_DEFAULT,
			     wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
			     false, wxString(fontname_labels, wxConvUTF8),
			     wxFONTENCODING_ISO8859_1);
    font_default = new wxFont(fontsize, wxFONTFAMILY_DEFAULT,
			      wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
			      false, wxString(fontname, wxConvUTF8),
			      wxFONTENCODING_ISO8859_1);
}

void
svxPrintout::OnEndPrinting() {
    delete font_labels;
    delete font_default;
    delete pen_frame;
    delete pen_cross;
    delete pen_leg;
    delete pen_surface_leg;
    delete pen_splay;
}

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
   wxASSERT(mask_t & L);
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
    if (!scan_for_blank_pages) {
	pdc->DrawLine(x_p, y_p, x_t, y_t);
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
   if (!scan_for_blank_pages) {
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
svxPrintout::WriteString(const wxString & s)
{
    double xsc, ysc;
    pdc->GetUserScale(&xsc, &ysc);
    pdc->SetUserScale(xsc * font_scaling_x, ysc * font_scaling_y);
    if (!scan_for_blank_pages) {
	pdc->DrawText(s,
		      long(x_t / font_scaling_x),
		      long(y_t / font_scaling_y) - pdc->GetCharHeight());
    } else {
	int w, h;
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
    if (!scan_for_blank_pages) {
	x_t = x_offset + x - clip.x_min;
	y_t = y_offset + clip.y_max - y;
	const wxBrush & save_brush = pdc->GetBrush();
	pdc->SetBrush(*wxTRANSPARENT_BRUSH);
	pdc->DrawEllipse(x_t - r, y_t - R, 2 * r, 2 * R);
	pdc->SetBrush(save_brush);
    } else {
	/* No need to check - this is only used in the legend. */
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

void
svxPrintout::NewPage(int pg, int pagesX, int pagesY)
{
    pdc->DestroyClippingRegion();

    int x, y;
    x = (pg - 1) % pagesX;
    y = pagesY - 1 - ((pg - 1) / pagesX);

    clip.x_min = (long)x * xpPageWidth;
    clip.y_min = (long)y * ypPageDepth;
    clip.x_max = clip.x_min + xpPageWidth; /* dm/pcl/ps had -1; */
    clip.y_max = clip.y_min + ypPageDepth; /* dm/pcl/ps had -1; */

    const int FOOTERS = 4;
    wxString footer[FOOTERS];
    footer[0] = m_layout->title;

    double rot = m_layout->rot;
    double tilt = m_layout->tilt;
    double scale = m_layout->Scale;
    switch (m_layout->view) {
	case layout::PLAN:
	    // TRANSLATORS: Used in the footer of printouts to compactly
	    // indicate this is a plan view and what the viewing angle is.
	    // Aven will replace %s with the bearing, and %.0f with the scale.
	    //
	    // This message probably doesn't need translating for most languages.
	    footer[1].Printf(wmsg(/*↑%s 1:%.0f*/233),
		    format_angle(ANGLE_FMT, rot).c_str(),
		    scale);
	    break;
	case layout::ELEV:
	    // TRANSLATORS: Used in the footer of printouts to compactly
	    // indicate this is an elevation view and what the viewing angle
	    // is.  Aven will replace the %s codes with the bearings to the
	    // left and right of the viewer, and %.0f with the scale.
	    //
	    // This message probably doesn't need translating for most languages.
	    footer[1].Printf(wmsg(/*%s↔%s 1:%.0f*/235),
		    format_angle(ANGLE_FMT, fmod(rot + 270.0, 360.0)).c_str(),
		    format_angle(ANGLE_FMT, fmod(rot + 90.0, 360.0)).c_str(),
		    scale);
	    break;
	case layout::TILT:
	    // TRANSLATORS: Used in the footer of printouts to compactly
	    // indicate this is a tilted elevation view and what the viewing
	    // angles are.  Aven will replace the %s codes with the bearings to
	    // the left and right of the viewer and the angle the view is
	    // tilted at, and %.0f with the scale.
	    //
	    // This message probably doesn't need translating for most languages.
	    footer[1].Printf(wmsg(/*%s↔%s ∡%s 1:%.0f*/236),
		    format_angle(ANGLE_FMT, fmod(rot + 270.0, 360.0)).c_str(),
		    format_angle(ANGLE_FMT, fmod(rot + 90.0, 360.0)).c_str(),
		    format_angle(ANGLE2_FMT, tilt).c_str(),
		    scale);
	    break;
	case layout::EXTELEV:
	    // TRANSLATORS: Used in the footer of printouts to compactly
	    // indicate this is an extended elevation view.  Aven will replace
	    // %.0f with the scale.
	    //
	    // Try to keep the translation short (for example, in English we
	    // use "Extended" not "Extended elevation") - there is limited room
	    // in the footer, and the details there are mostly to make it easy
	    // to check that you have corresponding pages from a multiple page
	    // printout.
	    footer[1].Printf(wmsg(/*Extended 1:%.0f*/244), scale);
	    break;
    }

    // TRANSLATORS: N/M meaning page N of M in the page footer of a printout.
    footer[2].Printf(wmsg(/*%d/%d*/232), pg, m_layout->pagesX * m_layout->pagesY);

    wxString datestamp = m_layout->datestamp;
    if (!datestamp.empty()) {
	// Remove any timezone suffix (e.g. " UTC" or " +1200").
	wxChar ch = datestamp[datestamp.size() - 1];
	if (ch >= 'A' && ch <= 'Z') {
	    for (size_t i = datestamp.size() - 1; i; --i) {
		ch = datestamp[i];
		if (ch < 'A' || ch > 'Z') {
		    if (ch == ' ') datestamp.resize(i);
		    break;
		}
	    }
	} else if (ch >= '0' && ch <= '9') {
	    for (size_t i = datestamp.size() - 1; i; --i) {
		ch = datestamp[i];
		if (ch < '0' || ch > '9') {
		    if ((ch == '-' || ch == '+') && datestamp[--i] == ' ')
			datestamp.resize(i);
		    break;
		}
	    }
	}

	// Remove any day prefix (e.g. "Mon,").
	for (size_t i = 0; i != datestamp.size(); ++i) {
	    if (datestamp[i] == ',' && i + 1 != datestamp.size()) {
		// Also skip a space after the comma.
		if (datestamp[i + 1] == ' ') ++i;
		datestamp.erase(0, i + 1);
		break;
	    }
	}
    }

    // TRANSLATORS: Used in the footer of printouts to compactly indicate that
    // the date which follows is the date that the survey data was processed.
    //
    // Aven will replace %s with a string giving the date and time (e.g.
    // "2015-06-09 12:40:44").
    footer[3].Printf(wmsg(/*Processed: %s*/167), datestamp.c_str());

    const wxChar * footer_sep = wxT("    ");
    int fontsize_footer = fontsize_labels;
    wxFont * font_footer;
    font_footer = new wxFont(fontsize_footer, wxFONTFAMILY_DEFAULT,
			     wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
			     false, wxString(fontname_labels, wxConvUTF8),
			     wxFONTENCODING_UTF8);
    font_footer->Scale(font_scaling_x);
    SetFont(font_footer);
    int w[FOOTERS], ws, h;
    pdc->GetTextExtent(footer_sep, &ws, &h);
    int wtotal = ws * (FOOTERS - 1);
    for (int i = 0; i < FOOTERS; ++i) {
	pdc->GetTextExtent(footer[i], &w[i], &h);
	wtotal += w[i];
    }

    long X = x_offset;
    long Y = y_offset + ypPageDepth + (long)(7 * m_layout->scY) - pdc->GetCharHeight();

    if (wtotal > xpPageWidth) {
	// Rescale the footer so it fits.
	double rescale = double(wtotal) / xpPageWidth;
	double xsc, ysc;
	pdc->GetUserScale(&xsc, &ysc);
	pdc->SetUserScale(xsc / rescale, ysc / rescale);
	SetFont(font_footer);
	wxString fullfooter = footer[0];
	for (int i = 1; i < FOOTERS - 1; ++i) {
	    fullfooter += footer_sep;
	    fullfooter += footer[i];
	}
	pdc->DrawText(fullfooter, X * rescale, Y * rescale);
	// Draw final item right aligned to avoid misaligning.
	wxRect rect(x_offset * rescale, Y * rescale,
		    xpPageWidth * rescale, pdc->GetCharHeight() * rescale);
	pdc->DrawLabel(footer[FOOTERS - 1], rect, wxALIGN_RIGHT|wxALIGN_TOP);
	pdc->SetUserScale(xsc, ysc);
    } else {
	// Space out the elements of the footer to fill the line.
	double extra = double(xpPageWidth - wtotal) / (FOOTERS - 1);
	for (int i = 0; i < FOOTERS - 1; ++i) {
	    pdc->DrawText(footer[i], X + extra * i, Y);
	    X += ws + w[i];
	}
	// Draw final item right aligned to avoid misaligning.
	wxRect rect(x_offset, Y, xpPageWidth, pdc->GetCharHeight());
	pdc->DrawLabel(footer[FOOTERS - 1], rect, wxALIGN_RIGHT|wxALIGN_TOP);
    }
    drawticks((int)(9 * m_layout->scX / POINTS_PER_MM), x, y);
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
	    Vector3 leg_v = next_pt_v - pt_v;

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
	    Vector3 leg_v = pt_v - prev_pt_v;

	    // Obtain a horizontal vector in the LRUD plane.
	    right = leg_v * up_v;
	    if (right.magnitude() == 0) {
		right = Vector3(last_right.GetX(), last_right.GetY(), 0.0);
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
	    Vector3 leg1_v = pt_v - prev_pt_v;
	    Vector3 leg2_v = next_pt_v - pt_v;

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

	if (l >= 0 || r >= 0) {
	    // Get the x and y coordinates of the survey station
	    double pt_X = pt_v.GetX() * COS - pt_v.GetY() * SIN;
	    double pt_Y = pt_v.GetX() * SIN + pt_v.GetY() * COS;
	    long pt_x = (long)((pt_X * Sc + m_layout->xOrg) * m_layout->scX);
	    long pt_y = (long)((pt_Y * Sc + m_layout->yOrg) * m_layout->scY);

	    // Calculate dimensions for the right arrow
	    double COSR = right.GetX();
	    double SINR = right.GetY();
	    long CROSS_MAJOR = (COSR + SINR) * PWX_CROSS_SIZE;
	    long CROSS_MINOR = (COSR - SINR) * PWX_CROSS_SIZE;

	    if (l >= 0) {
		// Get the x and y coordinates of the end of the left arrow
		Vector3 p = pt_v - right * l;
		double X = p.GetX() * COS - p.GetY() * SIN;
		double Y = (p.GetX() * SIN + p.GetY() * COS);
		long x = (long)((X * Sc + m_layout->xOrg) * m_layout->scX);
		long y = (long)((Y * Sc + m_layout->yOrg) * m_layout->scY);

		// Draw the arrow stem
		MoveTo(pt_x, pt_y);
		DrawTo(x, y);

		// Rotate the arrow by the page rotation
		long dx1 = (+CROSS_MINOR) * COS - (+CROSS_MAJOR) * SIN;
		long dy1 = (+CROSS_MINOR) * SIN + (+CROSS_MAJOR) * COS;
		long dx2 = (+CROSS_MAJOR) * COS - (-CROSS_MINOR) * SIN;
		long dy2 = (+CROSS_MAJOR) * SIN + (-CROSS_MINOR) * COS;

		// Draw the arrow
		MoveTo(x + dx1, y + dy1);
		DrawTo(x, y);
		DrawTo(x + dx2, y + dy2);
	    }

	    if (r >= 0) {
		// Get the x and y coordinates of the end of the right arrow
		Vector3 p = pt_v + right * r;
		double X = p.GetX() * COS - p.GetY() * SIN;
		double Y = (p.GetX() * SIN + p.GetY() * COS);
		long x = (long)((X * Sc + m_layout->xOrg) * m_layout->scX);
		long y = (long)((Y * Sc + m_layout->yOrg) * m_layout->scY);

		// Draw the arrow stem
		MoveTo(pt_x, pt_y);
		DrawTo(x, y);

		// Rotate the arrow by the page rotation
		long dx1 = (-CROSS_MINOR) * COS - (-CROSS_MAJOR) * SIN;
		long dy1 = (-CROSS_MINOR) * SIN + (-CROSS_MAJOR) * COS;
		long dx2 = (-CROSS_MAJOR) * COS - (+CROSS_MINOR) * SIN;
		long dy2 = (-CROSS_MAJOR) * SIN + (+CROSS_MINOR) * COS;

		// Draw the arrow
		MoveTo(x + dx1, y + dy1);
		DrawTo(x, y);
		DrawTo(x + dx2, y + dy2);
	    }
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

	Double u = pt_v.GetU();
	Double d = pt_v.GetD();

	if (u >= 0 || d >= 0) {
	    // Get the coordinates of the survey point
	    Vector3 p = pt_v;
	    double SIN = sin(rad(m_layout->rot));
	    double COS = cos(rad(m_layout->rot));
	    double X = p.GetX() * COS - p.GetY() * SIN;
	    double Y = p.GetZ();
	    long x = (long)((X * Sc + m_layout->xOrg) * m_layout->scX);
	    long pt_y = (long)((Y * Sc + m_layout->yOrg) * m_layout->scX);

	    if (u >= 0) {
		// Get the y coordinate of the up arrow
		long y = (long)(((Y + u) * Sc + m_layout->yOrg) * m_layout->scY);

		// Draw the arrow stem
		MoveTo(x, pt_y);
		DrawTo(x, y);

		// Draw the up arrow
		MoveTo(x - PWX_CROSS_SIZE, y - PWX_CROSS_SIZE);
		DrawTo(x, y);
		DrawTo(x + PWX_CROSS_SIZE, y - PWX_CROSS_SIZE);
	    }

	    if (d >= 0) {
		// Get the y coordinate of the down arrow
		long y = (long)(((Y - d) * Sc + m_layout->yOrg) * m_layout->scY);

		// Draw the arrow stem
		MoveTo(x, pt_y);
		DrawTo(x, y);

		// Draw the down arrow
		MoveTo(x - PWX_CROSS_SIZE, y + PWX_CROSS_SIZE);
		DrawTo(x, y);
		DrawTo(x + PWX_CROSS_SIZE, y + PWX_CROSS_SIZE);
	    }
	}
    }
}
