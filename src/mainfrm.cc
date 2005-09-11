//
//  mainfrm.cc
//
//  Main frame handling for Aven.
//
//  Copyright (C) 2000-2002,2005 Mark R. Shinwell
//  Copyright (C) 2001-2003,2004,2005 Olly Betts
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "mainfrm.h"
#include "aven.h"
#include "aboutdlg.h"

#include "message.h"
#include "img.h"
#include "namecmp.h"
#include "printwx.h"
#include "filename.h"
#include "useful.h"

#include <wx/confbase.h>
#include <wx/image.h>
#include <wx/imaglist.h>
#include <wx/regex.h>

#include <float.h>
#include <functional>
#include <stack>
#include <vector>

using namespace std;

class AvenSplitterWindow : public wxSplitterWindow {
    MainFrm *parent;

    public:
	AvenSplitterWindow(MainFrm *parent_)
	    : wxSplitterWindow(parent_, -1, wxDefaultPosition, wxDefaultSize,
			       wxSP_3D | wxSP_LIVE_UPDATE),
	      parent(parent_)
	{
	}

	void OnSplitterDClick(wxSplitterEvent &e) {
#if wxCHECK_VERSION(2,3,0)
	    e.Veto();
#endif
#if defined(__UNIX__) && !wxCHECK_VERSION(2,3,5)
	    parent->m_SashPosition = GetSashPosition(); // save width of panel
	    // Calling Unsplit from OnSplitterDClick() doesn't work in debian
	    // wxGtk 2.3.3.2 (which calls itself 2.3.4) - it does work from CVS
	    // prior to the actual 2.3.4 though - FIXME: monitor this
	    // situation...
	    SetSashPosition(0);
#else
	    parent->ToggleSidePanel();
#endif
	}

    private:
	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(AvenSplitterWindow, wxSplitterWindow)
    // The wx docs say "EVT_SPLITTER_DOUBLECLICKED" but the wx headers say
    // "EVT_SPLITTER_DCLICK" (wx docs corrected to agree with headers in 2.3)
#ifdef EVT_SPLITTER_DOUBLECLICKED
    EVT_SPLITTER_DOUBLECLICKED(-1, AvenSplitterWindow::OnSplitterDClick)
#else
    EVT_SPLITTER_DCLICK(-1, AvenSplitterWindow::OnSplitterDClick)
#endif
END_EVENT_TABLE()

class EditMarkDlg : public wxDialog {
    wxTextCtrl * easting, * northing, * altitude;
    wxTextCtrl * angle, * tilt_angle, * scale, * time;
public:
    EditMarkDlg(wxWindow* parent, const PresentationMark & p)
	: wxDialog(parent, 500, wxString("Edit Waypoint"))
    {
	easting = new wxTextCtrl(this, 601, wxString::Format("%.3f", p.x));
	northing = new wxTextCtrl(this, 602, wxString::Format("%.3f", p.y));
	altitude = new wxTextCtrl(this, 603, wxString::Format("%.3f", p.z));
	angle = new wxTextCtrl(this, 604, wxString::Format("%.3f", p.angle));
	tilt_angle = new wxTextCtrl(this, 605, wxString::Format("%.3f", p.tilt_angle));
	scale = new wxTextCtrl(this, 606, wxString::Format("%.3f", p.scale));
	if (p.time > 0.0) {
	    time = new wxTextCtrl(this, 607, wxString::Format("%.3f", p.time));
	} else if (p.time < 0.0) {
	    time = new wxTextCtrl(this, 607, wxString::Format("*%.3f", -p.time));
	} else {
	    time = new wxTextCtrl(this, 607, "0");
	}

	wxBoxSizer * coords = new wxBoxSizer(wxHORIZONTAL);
	coords->Add(new wxStaticText(this, 610, "("), 0, wxALIGN_CENTRE_VERTICAL);
	coords->Add(easting, 1);
	coords->Add(new wxStaticText(this, 611, ","), 0, wxALIGN_CENTRE_VERTICAL);
	coords->Add(northing, 1);
	coords->Add(new wxStaticText(this, 612, ","), 0, wxALIGN_CENTRE_VERTICAL);
	coords->Add(altitude, 1);
	coords->Add(new wxStaticText(this, 613, ")"), 0, wxALIGN_CENTRE_VERTICAL);
	wxBoxSizer* vert = new wxBoxSizer(wxVERTICAL);
	vert->Add(coords, 0, wxALL, 8);
	wxBoxSizer * r2 = new wxBoxSizer(wxHORIZONTAL);
	r2->Add(new wxStaticText(this, 614, "Bearing: "), 0, wxALIGN_CENTRE_VERTICAL);
	r2->Add(angle);
	vert->Add(r2, 0, wxALL, 8);
	wxBoxSizer * r3 = new wxBoxSizer(wxHORIZONTAL);
	r3->Add(new wxStaticText(this, 615, "Elevation: "), 0, wxALIGN_CENTRE_VERTICAL);
	r3->Add(tilt_angle);
	vert->Add(r3, 0, wxALL, 8);
	wxBoxSizer * r4 = new wxBoxSizer(wxHORIZONTAL);
	r4->Add(new wxStaticText(this, 616, "Scale: "), 0, wxALIGN_CENTRE_VERTICAL);
	r4->Add(scale);
	r4->Add(new wxStaticText(this, 617, " (unused in perspective view)"),
		0, wxALIGN_CENTRE_VERTICAL);
	vert->Add(r4, 0, wxALL, 8);

	wxBoxSizer * r5 = new wxBoxSizer(wxHORIZONTAL);
	r5->Add(new wxStaticText(this, 616, "Time: "), 0, wxALIGN_CENTRE_VERTICAL);
	r5->Add(time);
	r5->Add(new wxStaticText(this, 617, " secs (0 = auto; *6 = 6 times auto)"),
		0, wxALIGN_CENTRE_VERTICAL);
	vert->Add(r5, 0, wxALL, 8);

	wxBoxSizer * buttons = new wxBoxSizer(wxHORIZONTAL);
	wxButton* cancel = new wxButton(this, wxID_CANCEL, wxString("Cancel"));
	buttons->Add(cancel, 0, wxALL, 8);
	wxButton* ok = new wxButton(this, wxID_OK, wxString("OK"));
	ok->SetDefault();
	buttons->Add(ok, 0, wxALL, 8);
	vert->Add(buttons, 0, wxALL|wxALIGN_RIGHT);

	SetAutoLayout(true);
	SetSizer(vert);

	vert->Fit(this);
	vert->SetSizeHints(this);
    }
    PresentationMark GetMark() const {
	double x, y, z, a, t, s, T;
	x = atof(easting->GetValue().c_str());
	y = atof(northing->GetValue().c_str());
	z = atof(altitude->GetValue().c_str());
	a = atof(angle->GetValue().c_str());
	t = atof(tilt_angle->GetValue().c_str());
	s = atof(scale->GetValue().c_str());
	wxString str = time->GetValue();
	if (str[0u] == '*') str[0u] = '-';
	T = atof(str.c_str());
	return PresentationMark(x, y, z, a, t, s, T);
    }

private:
    DECLARE_EVENT_TABLE()
};

// Write a value without trailing zeros after the decimal point.
static void write_double(double d, FILE * fh) {
    char buf[64];
    sprintf(buf, "%.21f", d);
    size_t l = strlen(buf);
    while (l > 1 && buf[l - 1] == '0') --l;
    if (l > 1 && buf[l - 1] == '.') --l;
    fwrite(buf, l, 1, fh);
}

class AvenPresList : public wxListCtrl {
    MainFrm * mainfrm;
    GfxCore * gfx;
    vector<PresentationMark> entries;
    long current_item;
    bool modified;
    wxString filename;

    public:
	AvenPresList(MainFrm * mainfrm_, wxWindow * parent, GfxCore * gfx_)
	    : wxListCtrl(parent, listctrl_PRES, wxDefaultPosition, wxDefaultSize,
			 wxLC_REPORT|wxLC_VIRTUAL),
	      mainfrm(mainfrm_), gfx(gfx_), current_item(-1), modified(false)
	    {
		InsertColumn(0, msg(/*Easting*/378));
		InsertColumn(1, msg(/*Northing*/379));
		InsertColumn(2, msg(/*Altitude*/335));
	    }

	void OnBeginLabelEdit(wxListEvent& event) {
	    event.Veto(); // No editting allowed
	}
	void OnDeleteItem(wxListEvent& event) {
	    long item = event.GetIndex();
	    if (current_item == item) {
		current_item = -1;
	    } else if (current_item > item) {
		--current_item;
	    }
	    entries.erase(entries.begin() + item);
	    SetItemCount(entries.size());
	    modified = true;
	}
	void OnDeleteAllItems(wxListEvent& event) {
	    entries.clear();
	    SetItemCount(entries.size());
	    filename = "";
	    modified = false;
	}
	void OnListKeyDown(wxListEvent& event) {
	    switch (event.GetKeyCode()) {
		case WXK_DELETE: {
		    long item = GetNextItem(-1, wxLIST_NEXT_ALL,
					    wxLIST_STATE_SELECTED);
		    while (item != -1) {
			DeleteItem(item);
			// - 1 because the indices were shifted by DeleteItem()
			item = GetNextItem(item - 1, wxLIST_NEXT_ALL,
					   wxLIST_STATE_SELECTED);
		    }
		    break;
		}
		default:
		    //printf("event.GetIndex() = %ld\n", event.GetIndex());
		    event.Skip();
	    }
	}
	void OnActivated(wxListEvent& event) {
	    // Jump to this view.
	    long item = event.GetIndex();
	    gfx->SetView(entries[item]);
	}
	void OnFocused(wxListEvent& event) {
	    current_item = event.GetIndex();
	}
	void OnRightClick(wxListEvent& event) {
	    long item = event.GetIndex();
	    EditMarkDlg edit(mainfrm, entries[item]);
	    if (edit.ShowModal() == wxID_OK) {
		entries[item] = edit.GetMark();
	    }
	}
	void OnChar(wxKeyEvent& event) {
	    if (event.GetKeyCode() == WXK_INSERT) {
		if (event.m_controlDown) {
		    AddMark(current_item, entries[current_item]);
		} else {
		    AddMark(current_item);
		}
	    } else {
		gfx->OnKeyPress(event);
	    }
	}
	void AddMark(long item = -1) {
	    AddMark(item, gfx->GetView());
	}
	void AddMark(long item, const PresentationMark & mark) {
	    if (item == -1) item = entries.size();
	    entries.insert(entries.begin() + item, mark);
	    SetItemCount(entries.size());
	    modified = true;
	}
	virtual wxString OnGetItemText(long item, long column) const {
	    if (item < 0 || item >= (long)entries.size()) return "";
	    const PresentationMark & p = entries[item];
	    double v;
	    switch (column) {
		case 0: v = p.x; break;
		case 1: v = p.y; break;
		case 2: v = p.z; break;
#if 0
		case 3: v = p.angle; break;
		case 4: v = p.tilt_angle; break;
		case 5: v = p.scale; break;
		case 6: v = p.time; break;
#endif
		default: return "";
	    }
	    return wxString::Format("%ld", (long)v);
	}
	void Save(bool use_default_name) {
	    wxString fnm = filename;
	    if (!use_default_name || fnm.empty()) {
		AvenAllowOnTop ontop(mainfrm);
#ifdef __WXMOTIF__
		wxFileDialog dlg (this, wxString(msg(/*Select an output filename*/319)), "", fnm,
				  "*.fly", wxSAVE|wxOVERWRITE_PROMPT);
#else
		wxFileDialog dlg (this, wxString(msg(/*Select an output filename*/319)), "", fnm,
				  wxString::Format("%s|*.fly|%s|%s",
						   msg(/*Aven presentations*/320),
						   msg(/*All files*/208),
						   wxFileSelectorDefaultWildcardStr),
				  wxSAVE|wxOVERWRITE_PROMPT);
#endif
		if (dlg.ShowModal() != wxID_OK) return;
		fnm = dlg.GetPath();
	    }

	    FILE * fh_pres = fopen(fnm.c_str(), "w");
	    if (!fh_pres) {
		wxGetApp().ReportError(wxString::Format(msg(/*Error writing to file `%s'*/110), fnm.c_str()));
		return;
	    }
	    vector<PresentationMark>::const_iterator i;
	    for (i = entries.begin(); i != entries.end(); ++i) {
		const PresentationMark &p = *i;
		write_double(p.x, fh_pres);
		putc(' ', fh_pres);
		write_double(p.y, fh_pres);
		putc(' ', fh_pres);
		write_double(p.z, fh_pres);
		putc(' ', fh_pres);
		write_double(p.angle, fh_pres);
		putc(' ', fh_pres);
		write_double(p.tilt_angle, fh_pres);
		putc(' ', fh_pres);
		write_double(p.scale, fh_pres);
		if (p.time != 0.0) {
		    putc(' ', fh_pres);
		    write_double(p.time, fh_pres);
		}
		putc('\n', fh_pres);
	    }
	    fclose(fh_pres);
	    filename = fnm;
	    modified = false;
	}
	bool Load(const wxString &fnm) {
	    FILE * fh_pres = fopen(fnm.c_str(), "r");
	    if (!fh_pres) {
		wxString m;
		m.Printf(msg(/*Couldn't open file `%s'*/93), fnm.c_str());
		wxGetApp().ReportError(m);
		return false;
	    }
	    DeleteAllItems();
	    long item = 0;
	    while (!feof(fh_pres)) {
		char buf[4096];
		size_t i = 0;
		while (i < sizeof(buf) - 1) {
		    int ch = getc(fh_pres);
		    if (ch == EOF || ch == '\n' || ch == '\r') break;
		    buf[i++] = ch;
		}
		if (i) {
		    buf[i] = 0;
		    double x, y, z, a, t, s, T;
		    int c = sscanf(buf, "%lf %lf %lf %lf %lf %lf %lf", &x, &y, &z, &a, &t, &s, &T);
		    if (c < 6) {
			DeleteAllItems();
			wxGetApp().ReportError(wxString::Format(msg(/*Error in format of presentation file `%s'*/323), fnm.c_str()));
			return false;
		    }
		    if (c == 6) T = 0;
		    AddMark(item, PresentationMark(x, y, z, a, t, s, T));
		    ++item;
		}
	    }
	    fclose(fh_pres);
	    filename = fnm;
	    modified = false;
	    return true;
	}
	bool Modified() const { return modified; }
	bool Empty() const { return entries.empty(); }
	PresentationMark GetPresMark(int which) {
	    long item = current_item;
	    if (which == MARK_FIRST) {
		item = 0;
	    } else if (which == MARK_NEXT) {
		++item;
	    } else if (which == MARK_PREV) {
		--item;
	    }
	    if (item == -1 || item == (long)entries.size())
		return PresentationMark();
	    if (item != current_item) {
		// Move the focus
		if (current_item != -1) {
		    wxListCtrl::SetItemState(current_item, wxLIST_STATE_FOCUSED,
					     0);
		}
		wxListCtrl::SetItemState(item, wxLIST_STATE_FOCUSED,
					 wxLIST_STATE_FOCUSED);
	    }
	    return entries[item];
	}

    private:

	DECLARE_NO_COPY_CLASS(AvenPresList)
	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(EditMarkDlg, wxDialog)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(AvenPresList, wxListCtrl)
    EVT_LIST_BEGIN_LABEL_EDIT(listctrl_PRES, AvenPresList::OnBeginLabelEdit)
    EVT_LIST_DELETE_ITEM(listctrl_PRES, AvenPresList::OnDeleteItem)
    EVT_LIST_DELETE_ALL_ITEMS(listctrl_PRES, AvenPresList::OnDeleteAllItems)
    EVT_LIST_KEY_DOWN(listctrl_PRES, AvenPresList::OnListKeyDown)
    EVT_LIST_ITEM_ACTIVATED(listctrl_PRES, AvenPresList::OnActivated)
    EVT_LIST_ITEM_FOCUSED(listctrl_PRES, AvenPresList::OnFocused)
    EVT_LIST_ITEM_RIGHT_CLICK(listctrl_PRES, AvenPresList::OnRightClick)
    EVT_CHAR(AvenPresList::OnChar)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(MainFrm, wxFrame)
    EVT_TEXT_ENTER(textctrl_FIND, MainFrm::OnFind)
    EVT_MENU(button_FIND, MainFrm::OnFind)
    EVT_MENU(button_HIDE, MainFrm::OnHide)
    EVT_UPDATE_UI(button_HIDE, MainFrm::OnHideUpdate)

    EVT_MENU(menu_FILE_OPEN, MainFrm::OnOpen)
    EVT_MENU(menu_FILE_PRINT, MainFrm::OnPrint)
    EVT_MENU(menu_FILE_PAGE_SETUP, MainFrm::OnPageSetup)
    EVT_MENU(menu_FILE_SCREENSHOT, MainFrm::OnScreenshot)
//    EVT_MENU(menu_FILE_PREFERENCES, MainFrm::OnFilePreferences)
    EVT_MENU(menu_FILE_EXPORT, MainFrm::OnExport)
    EVT_MENU(menu_FILE_QUIT, MainFrm::OnQuit)
    EVT_MENU_RANGE(wxID_FILE1, wxID_FILE9, MainFrm::OnMRUFile)

    EVT_MENU(menu_PRES_NEW, MainFrm::OnPresNew)
    EVT_MENU(menu_PRES_OPEN, MainFrm::OnPresOpen)
    EVT_MENU(menu_PRES_SAVE, MainFrm::OnPresSave)
    EVT_MENU(menu_PRES_SAVE_AS, MainFrm::OnPresSaveAs)
    EVT_MENU(menu_PRES_MARK, MainFrm::OnPresMark)
    EVT_MENU(menu_PRES_FREWIND, MainFrm::OnPresFRewind)
    EVT_MENU(menu_PRES_REWIND, MainFrm::OnPresRewind)
    EVT_MENU(menu_PRES_REVERSE, MainFrm::OnPresReverse)
    EVT_MENU(menu_PRES_PLAY, MainFrm::OnPresPlay)
    EVT_MENU(menu_PRES_FF, MainFrm::OnPresFF)
    EVT_MENU(menu_PRES_FFF, MainFrm::OnPresFFF)
    EVT_MENU(menu_PRES_PAUSE, MainFrm::OnPresPause)
    EVT_MENU(menu_PRES_STOP, MainFrm::OnPresStop)
    EVT_MENU(menu_PRES_EXPORT_MOVIE, MainFrm::OnPresExportMovie)

    EVT_UPDATE_UI(menu_PRES_NEW, MainFrm::OnPresNewUpdate)
    EVT_UPDATE_UI(menu_PRES_OPEN, MainFrm::OnPresOpenUpdate)
    EVT_UPDATE_UI(menu_PRES_SAVE, MainFrm::OnPresSaveUpdate)
    EVT_UPDATE_UI(menu_PRES_SAVE_AS, MainFrm::OnPresSaveAsUpdate)
    EVT_UPDATE_UI(menu_PRES_MARK, MainFrm::OnPresMarkUpdate)
    EVT_UPDATE_UI(menu_PRES_FREWIND, MainFrm::OnPresFRewindUpdate)
    EVT_UPDATE_UI(menu_PRES_REWIND, MainFrm::OnPresRewindUpdate)
    EVT_UPDATE_UI(menu_PRES_REVERSE, MainFrm::OnPresReverseUpdate)
    EVT_UPDATE_UI(menu_PRES_PLAY, MainFrm::OnPresPlayUpdate)
    EVT_UPDATE_UI(menu_PRES_FF, MainFrm::OnPresFFUpdate)
    EVT_UPDATE_UI(menu_PRES_FFF, MainFrm::OnPresFFFUpdate)
    EVT_UPDATE_UI(menu_PRES_PAUSE, MainFrm::OnPresPauseUpdate)
    EVT_UPDATE_UI(menu_PRES_STOP, MainFrm::OnPresStopUpdate)
    EVT_UPDATE_UI(menu_PRES_EXPORT_MOVIE, MainFrm::OnPresExportMovieUpdate)

    EVT_CLOSE(MainFrm::OnClose)
    EVT_SET_FOCUS(MainFrm::OnSetFocus)

    EVT_MENU(menu_ROTATION_TOGGLE, MainFrm::OnToggleRotation)
    EVT_MENU(menu_ROTATION_SPEED_UP, MainFrm::OnSpeedUp)
    EVT_MENU(menu_ROTATION_SLOW_DOWN, MainFrm::OnSlowDown)
    EVT_MENU(menu_ROTATION_REVERSE, MainFrm::OnReverseDirectionOfRotation)
    EVT_MENU(menu_ROTATION_STEP_CCW, MainFrm::OnStepOnceAnticlockwise)
    EVT_MENU(menu_ROTATION_STEP_CW, MainFrm::OnStepOnceClockwise)
    EVT_MENU(menu_ORIENT_MOVE_NORTH, MainFrm::OnMoveNorth)
    EVT_MENU(menu_ORIENT_MOVE_EAST, MainFrm::OnMoveEast)
    EVT_MENU(menu_ORIENT_MOVE_SOUTH, MainFrm::OnMoveSouth)
    EVT_MENU(menu_ORIENT_MOVE_WEST, MainFrm::OnMoveWest)
    EVT_MENU(menu_ORIENT_SHIFT_LEFT, MainFrm::OnShiftDisplayLeft)
    EVT_MENU(menu_ORIENT_SHIFT_RIGHT, MainFrm::OnShiftDisplayRight)
    EVT_MENU(menu_ORIENT_SHIFT_UP, MainFrm::OnShiftDisplayUp)
    EVT_MENU(menu_ORIENT_SHIFT_DOWN, MainFrm::OnShiftDisplayDown)
    EVT_MENU(menu_ORIENT_PLAN, MainFrm::OnPlan)
    EVT_MENU(menu_ORIENT_ELEVATION, MainFrm::OnElevation)
    EVT_MENU(menu_ORIENT_HIGHER_VP, MainFrm::OnHigherViewpoint)
    EVT_MENU(menu_ORIENT_LOWER_VP, MainFrm::OnLowerViewpoint)
    EVT_MENU(menu_ORIENT_ZOOM_IN, MainFrm::OnZoomIn)
    EVT_MENU(menu_ORIENT_ZOOM_OUT, MainFrm::OnZoomOut)
    EVT_MENU(menu_ORIENT_DEFAULTS, MainFrm::OnDefaults)
    EVT_MENU(menu_VIEW_SHOW_LEGS, MainFrm::OnShowSurveyLegs)
    EVT_MENU(menu_VIEW_SHOW_CROSSES, MainFrm::OnShowCrosses)
    EVT_MENU(menu_VIEW_SHOW_ENTRANCES, MainFrm::OnShowEntrances)
    EVT_MENU(menu_VIEW_SHOW_FIXED_PTS, MainFrm::OnShowFixedPts)
    EVT_MENU(menu_VIEW_SHOW_EXPORTED_PTS, MainFrm::OnShowExportedPts)
    EVT_MENU(menu_VIEW_SHOW_NAMES, MainFrm::OnShowStationNames)
    EVT_MENU(menu_VIEW_SHOW_OVERLAPPING_NAMES, MainFrm::OnDisplayOverlappingNames)
    EVT_MENU(menu_VIEW_COLOUR_BY_DEPTH, MainFrm::OnColourByDepth)
    EVT_MENU(menu_VIEW_SHOW_SURFACE, MainFrm::OnShowSurface)
    EVT_MENU(menu_VIEW_GRID, MainFrm::OnViewGrid)
    EVT_MENU(menu_VIEW_BOUNDING_BOX, MainFrm::OnViewBoundingBox)
    EVT_MENU(menu_VIEW_PERSPECTIVE, MainFrm::OnViewPerspective)
    EVT_MENU(menu_VIEW_TEXTURED, MainFrm::OnViewTextured)
    EVT_MENU(menu_VIEW_FOG, MainFrm::OnViewFog)
    EVT_MENU(menu_VIEW_SMOOTH_LINES, MainFrm::OnViewSmoothLines)
    EVT_MENU(menu_VIEW_FULLSCREEN, MainFrm::OnViewFullScreen)
    EVT_MENU(menu_VIEW_SHOW_TUBES, MainFrm::OnToggleTubes)
    EVT_MENU(menu_IND_COMPASS, MainFrm::OnViewCompass)
    EVT_MENU(menu_IND_CLINO, MainFrm::OnViewClino)
    EVT_MENU(menu_IND_DEPTH_BAR, MainFrm::OnToggleDepthbar)
    EVT_MENU(menu_IND_SCALE_BAR, MainFrm::OnToggleScalebar)
    EVT_MENU(menu_CTL_SIDE_PANEL, MainFrm::OnViewSidePanel)
    EVT_MENU(menu_CTL_METRIC, MainFrm::OnToggleMetric)
    EVT_MENU(menu_CTL_DEGREES, MainFrm::OnToggleDegrees)
    EVT_MENU(menu_CTL_REVERSE, MainFrm::OnReverseControls)
    EVT_MENU(menu_CTL_CANCEL_DIST_LINE, MainFrm::OnCancelDistLine)
    EVT_MENU(menu_HELP_ABOUT, MainFrm::OnAbout)

    EVT_UPDATE_UI(menu_FILE_PRINT, MainFrm::OnPrintUpdate)
    EVT_UPDATE_UI(menu_FILE_SCREENSHOT, MainFrm::OnScreenshotUpdate)
    EVT_UPDATE_UI(menu_FILE_EXPORT, MainFrm::OnExportUpdate)
    EVT_UPDATE_UI(menu_ROTATION_TOGGLE, MainFrm::OnToggleRotationUpdate)
    EVT_UPDATE_UI(menu_ROTATION_SPEED_UP, MainFrm::OnSpeedUpUpdate)
    EVT_UPDATE_UI(menu_ROTATION_SLOW_DOWN, MainFrm::OnSlowDownUpdate)
    EVT_UPDATE_UI(menu_ROTATION_REVERSE, MainFrm::OnReverseDirectionOfRotationUpdate)
    EVT_UPDATE_UI(menu_ROTATION_STEP_CCW, MainFrm::OnStepOnceAnticlockwiseUpdate)
    EVT_UPDATE_UI(menu_ROTATION_STEP_CW, MainFrm::OnStepOnceClockwiseUpdate)
    EVT_UPDATE_UI(menu_ORIENT_MOVE_NORTH, MainFrm::OnMoveNorthUpdate)
    EVT_UPDATE_UI(menu_ORIENT_MOVE_EAST, MainFrm::OnMoveEastUpdate)
    EVT_UPDATE_UI(menu_ORIENT_MOVE_SOUTH, MainFrm::OnMoveSouthUpdate)
    EVT_UPDATE_UI(menu_ORIENT_MOVE_WEST, MainFrm::OnMoveWestUpdate)
    EVT_UPDATE_UI(menu_ORIENT_SHIFT_LEFT, MainFrm::OnShiftDisplayLeftUpdate)
    EVT_UPDATE_UI(menu_ORIENT_SHIFT_RIGHT, MainFrm::OnShiftDisplayRightUpdate)
    EVT_UPDATE_UI(menu_ORIENT_SHIFT_UP, MainFrm::OnShiftDisplayUpUpdate)
    EVT_UPDATE_UI(menu_ORIENT_SHIFT_DOWN, MainFrm::OnShiftDisplayDownUpdate)
    EVT_UPDATE_UI(menu_ORIENT_PLAN, MainFrm::OnPlanUpdate)
    EVT_UPDATE_UI(menu_ORIENT_ELEVATION, MainFrm::OnElevationUpdate)
    EVT_UPDATE_UI(menu_ORIENT_HIGHER_VP, MainFrm::OnHigherViewpointUpdate)
    EVT_UPDATE_UI(menu_ORIENT_LOWER_VP, MainFrm::OnLowerViewpointUpdate)
    EVT_UPDATE_UI(menu_ORIENT_ZOOM_IN, MainFrm::OnZoomInUpdate)
    EVT_UPDATE_UI(menu_ORIENT_ZOOM_OUT, MainFrm::OnZoomOutUpdate)
    EVT_UPDATE_UI(menu_ORIENT_DEFAULTS, MainFrm::OnDefaultsUpdate)
    EVT_UPDATE_UI(menu_VIEW_SHOW_LEGS, MainFrm::OnShowSurveyLegsUpdate)
    EVT_UPDATE_UI(menu_VIEW_SHOW_CROSSES, MainFrm::OnShowCrossesUpdate)
    EVT_UPDATE_UI(menu_VIEW_SHOW_ENTRANCES, MainFrm::OnShowEntrancesUpdate)
    EVT_UPDATE_UI(menu_VIEW_SHOW_FIXED_PTS, MainFrm::OnShowFixedPtsUpdate)
    EVT_UPDATE_UI(menu_VIEW_SHOW_EXPORTED_PTS, MainFrm::OnShowExportedPtsUpdate)
    EVT_UPDATE_UI(menu_VIEW_SHOW_NAMES, MainFrm::OnShowStationNamesUpdate)
    EVT_UPDATE_UI(menu_VIEW_SHOW_SURFACE, MainFrm::OnShowSurfaceUpdate)
    EVT_UPDATE_UI(menu_VIEW_SHOW_OVERLAPPING_NAMES, MainFrm::OnDisplayOverlappingNamesUpdate)
    EVT_UPDATE_UI(menu_VIEW_COLOUR_BY_DEPTH, MainFrm::OnColourByDepthUpdate)
    EVT_UPDATE_UI(menu_VIEW_GRID, MainFrm::OnViewGridUpdate)
    EVT_UPDATE_UI(menu_VIEW_BOUNDING_BOX, MainFrm::OnViewBoundingBoxUpdate)
    EVT_UPDATE_UI(menu_VIEW_PERSPECTIVE, MainFrm::OnViewPerspectiveUpdate)
    EVT_UPDATE_UI(menu_VIEW_TEXTURED, MainFrm::OnViewTexturedUpdate)
    EVT_UPDATE_UI(menu_VIEW_FOG, MainFrm::OnViewFogUpdate)
    EVT_UPDATE_UI(menu_VIEW_SMOOTH_LINES, MainFrm::OnViewSmoothLinesUpdate)
    EVT_UPDATE_UI(menu_VIEW_FULLSCREEN, MainFrm::OnViewFullScreenUpdate)
    EVT_UPDATE_UI(menu_VIEW_SHOW_TUBES, MainFrm::OnToggleTubesUpdate)
    EVT_UPDATE_UI(menu_IND_COMPASS, MainFrm::OnViewCompassUpdate)
    EVT_UPDATE_UI(menu_IND_CLINO, MainFrm::OnViewClinoUpdate)
    EVT_UPDATE_UI(menu_IND_DEPTH_BAR, MainFrm::OnToggleDepthbarUpdate)
    EVT_UPDATE_UI(menu_IND_SCALE_BAR, MainFrm::OnToggleScalebarUpdate)
    EVT_UPDATE_UI(menu_CTL_INDICATORS, MainFrm::OnIndicatorsUpdate)
    EVT_UPDATE_UI(menu_CTL_SIDE_PANEL, MainFrm::OnViewSidePanelUpdate)
    EVT_UPDATE_UI(menu_CTL_REVERSE, MainFrm::OnReverseControlsUpdate)
    EVT_UPDATE_UI(menu_CTL_CANCEL_DIST_LINE, MainFrm::OnCancelDistLineUpdate)
    EVT_UPDATE_UI(menu_CTL_METRIC, MainFrm::OnToggleMetricUpdate)
    EVT_UPDATE_UI(menu_CTL_DEGREES, MainFrm::OnToggleDegreesUpdate)
END_EVENT_TABLE()

class LabelCmp : public greater<const LabelInfo*> {
    int separator;
public:
    LabelCmp(int separator_) : separator(separator_) {}
    bool operator()(const LabelInfo* pt1, const LabelInfo* pt2) {
	return name_cmp(pt1->GetText(), pt2->GetText(), separator) < 0;
    }
};

class LabelPlotCmp : public greater<const LabelInfo*> {
    int separator;
public:
    LabelPlotCmp(int separator_) : separator(separator_) {}
    bool operator()(const LabelInfo* pt1, const LabelInfo* pt2) {
	int n = pt1->flags - pt2->flags;
	if (n) return n > 0;
	wxString l1 = pt1->text.AfterLast(separator);
	wxString l2 = pt2->text.AfterLast(separator);
	n = name_cmp(l1, l2, separator);
	if (n) return n < 0;
	// Prefer non-2-nodes...
	// FIXME; implement
	// if leaf names are the same, prefer shorter labels as we can
	// display more of them
	n = pt1->text.length() - pt2->text.length();
	if (n) return n < 0;
	// make sure that we don't ever compare different labels as equal
	return name_cmp(pt1->text, pt2->text, separator) < 0;
    }
};

#if wxUSE_DRAG_AND_DROP
class DnDFile : public wxFileDropTarget {
    public:
	DnDFile(MainFrm *parent) : m_Parent(parent) { }
	virtual bool OnDropFiles(wxCoord, wxCoord,
			const wxArrayString &filenames);

    private:
	MainFrm * m_Parent;
};

bool
DnDFile::OnDropFiles(wxCoord, wxCoord, const wxArrayString &filenames)
{
    // Load a survey file by drag-and-drop.
    assert(filenames.GetCount() > 0);

    if (filenames.GetCount() != 1) {
	wxGetApp().ReportError(msg(/*You may only view one 3d file at a time.*/336));
	return FALSE;
    }

    m_Parent->OpenFile(filenames[0]);
    return TRUE;
}
#endif

MainFrm::MainFrm(const wxString& title, const wxPoint& pos, const wxSize& size) :
    wxFrame(NULL, 101, title, pos, size, wxDEFAULT_FRAME_STYLE | wxNO_FULL_REPAINT_ON_RESIZE),
    m_Gfx(NULL), m_NumEntrances(0), m_NumFixedPts(0), m_NumExportedPts(0),
    m_NumHighlighted(0), m_HasUndergroundLegs(false), m_HasSurfaceLegs(false)
#ifdef PREFDLG
    , m_PrefsDlg(NULL)
#endif
{
    icon_path = msg_cfgpth();
    icon_path += wxCONFIG_PATH_SEPARATOR;
    icon_path += "icons";
    icon_path += wxCONFIG_PATH_SEPARATOR;

#ifdef _WIN32
    // The peculiar name is so that the icon is the first in the file
    // (required by Microsoft Windows for this type of icon)
    SetIcon(wxIcon("aaaaaAven"));
#else
    SetIcon(wxIcon(icon_path + "aven.png", wxBITMAP_TYPE_PNG));
#endif

    CreateMenuBar();
    CreateToolBar();
    CreateStatusBar(3, wxST_SIZEGRIP);
    CreateSidePanel();

    int widths[3] = { 150, -1 /* variable width */, -1 };
    GetStatusBar()->SetStatusWidths(3, widths);

#ifdef __X__ // wxMotif or wxX11
    int x;
    int y;
    GetSize(&x, &y);
    // X seems to require a forced resize.
    SetSize(-1, -1, x, y);
#endif

#if wxUSE_DRAG_AND_DROP
    SetDropTarget(new DnDFile(this));
#endif
}

MainFrm::~MainFrm()
{
    m_Labels.clear();
}

void MainFrm::CreateMenuBar()
{
    // Create the menus and the menu bar.

    wxMenu* filemenu = new wxMenu;
    filemenu->Append(menu_FILE_OPEN, GetTabMsg(/*@Open...##Ctrl+O*/220));
    filemenu->AppendSeparator();
    filemenu->Append(menu_FILE_PRINT, GetTabMsg(/*@Print...##Ctrl+P*/380));
    filemenu->Append(menu_FILE_PAGE_SETUP, GetTabMsg(/*P@age Setup...*/381));
    filemenu->AppendSeparator();
    filemenu->Append(menu_FILE_SCREENSHOT, GetTabMsg(/*@Screenshot...*/201));
    filemenu->Append(menu_FILE_EXPORT, "Export as..."); // FIXME TRANSLATE
    filemenu->AppendSeparator();
    filemenu->Append(menu_FILE_QUIT, GetTabMsg(/*@Quit##Ctrl+Q*/221));

    m_history.UseMenu(filemenu);
    m_history.Load(*wxConfigBase::Get());

    wxMenu* rotmenu = new wxMenu;
    // FIXME: TRANSLATE
    rotmenu->Append(menu_ROTATION_TOGGLE, "&Auto-Rotate\tSpace", "", true);
    rotmenu->AppendSeparator();
    rotmenu->Append(menu_ROTATION_SPEED_UP, GetTabMsg(/*Speed @Up*/232));
    rotmenu->Append(menu_ROTATION_SLOW_DOWN, GetTabMsg(/*Slow @Down*/233));
    rotmenu->AppendSeparator();
    rotmenu->Append(menu_ROTATION_REVERSE, GetTabMsg(/*@Reverse Direction*/234));
    rotmenu->AppendSeparator();
    rotmenu->Append(menu_ROTATION_STEP_CCW, GetTabMsg(/*Step Once @Anticlockwise*/235));
    rotmenu->Append(menu_ROTATION_STEP_CW, GetTabMsg(/*Step Once @Clockwise*/236));

    wxMenu* orientmenu = new wxMenu;
    orientmenu->Append(menu_ORIENT_MOVE_NORTH, GetTabMsg(/*View @North*/240));
    orientmenu->Append(menu_ORIENT_MOVE_EAST, GetTabMsg(/*View @East*/241));
    orientmenu->Append(menu_ORIENT_MOVE_SOUTH, GetTabMsg(/*View @South*/242));
    orientmenu->Append(menu_ORIENT_MOVE_WEST, GetTabMsg(/*View @West*/243));
    orientmenu->AppendSeparator();
    orientmenu->Append(menu_ORIENT_SHIFT_LEFT, GetTabMsg(/*Shift Survey @Left*/244));
    orientmenu->Append(menu_ORIENT_SHIFT_RIGHT, GetTabMsg(/*Shift Survey @Right*/245));
    orientmenu->Append(menu_ORIENT_SHIFT_UP, GetTabMsg(/*Shift Survey @Up*/246));
    orientmenu->Append(menu_ORIENT_SHIFT_DOWN, GetTabMsg(/*Shift Survey @Down*/247));
    orientmenu->AppendSeparator();
    orientmenu->Append(menu_ORIENT_PLAN, GetTabMsg(/*@Plan View*/248));
    orientmenu->Append(menu_ORIENT_ELEVATION, GetTabMsg(/*Ele@vation*/249));
    orientmenu->AppendSeparator();
    orientmenu->Append(menu_ORIENT_HIGHER_VP, GetTabMsg(/*@Higher Viewpoint*/250));
    orientmenu->Append(menu_ORIENT_LOWER_VP, GetTabMsg(/*L@ower Viewpoint*/251));
    orientmenu->AppendSeparator();
    orientmenu->Append(menu_ORIENT_ZOOM_IN, GetTabMsg(/*@Zoom In##]*/252));
    orientmenu->Append(menu_ORIENT_ZOOM_OUT, GetTabMsg(/*Zoo@m Out##[*/253));
    orientmenu->AppendSeparator();
    orientmenu->Append(menu_ORIENT_DEFAULTS, GetTabMsg(/*Restore De@fault View*/254));

    wxMenu* presmenu = new wxMenu;
    presmenu->Append(menu_PRES_NEW, GetTabMsg(/*@New Presentation*/311));
    presmenu->Append(menu_PRES_OPEN, GetTabMsg(/*@Open Presentation...*/312));
    presmenu->Append(menu_PRES_SAVE, GetTabMsg(/*@Save Presentation*/313));
    presmenu->Append(menu_PRES_SAVE_AS, GetTabMsg(/*Save Presentation @As...*/314));
    presmenu->AppendSeparator();
    presmenu->Append(menu_PRES_MARK, GetTabMsg(/*@Mark*/315));
    presmenu->Append(menu_PRES_PLAY, GetTabMsg(/*@Play*/316));
    presmenu->Append(menu_PRES_EXPORT_MOVIE, GetTabMsg(/*@Export as Movie...*/317));

    wxMenu* viewmenu = new wxMenu;
#ifndef PREFDLG
    viewmenu->Append(menu_VIEW_SHOW_NAMES, GetTabMsg(/*Station @Names##Ctrl+N*/270), "", true);
    viewmenu->Append(menu_VIEW_SHOW_TUBES, GetTabMsg(/*Passage @Tubes*/346), "", true);
    viewmenu->Append(menu_VIEW_SHOW_CROSSES, GetTabMsg(/*@Crosses##Ctrl+X*/271), "", true);
    viewmenu->Append(menu_VIEW_GRID, GetTabMsg(/*@Grid##Ctrl+G*/297), "", true);
    viewmenu->Append(menu_VIEW_BOUNDING_BOX, GetTabMsg(/*@Bounding Box##Ctrl+B*/318), "", true);
    viewmenu->AppendSeparator();
    viewmenu->Append(menu_VIEW_SHOW_LEGS, GetTabMsg(/*@Underground Survey Legs##Ctrl+L*/272), "", true);
    viewmenu->Append(menu_VIEW_SHOW_SURFACE, GetTabMsg(/*@Surface Survey Legs##Ctrl+F*/291), "", true);
    viewmenu->AppendSeparator();
    viewmenu->Append(menu_VIEW_SHOW_OVERLAPPING_NAMES, GetTabMsg(/*@Overlapping Names*/273), "", true);
    viewmenu->Append(menu_VIEW_COLOUR_BY_DEPTH, GetTabMsg(/*Co@lour by Depth*/292), "", true);
    viewmenu->AppendSeparator();
    viewmenu->Append(menu_VIEW_SHOW_ENTRANCES, GetTabMsg(/*Highlight @Entrances*/294), "", true);
    viewmenu->Append(menu_VIEW_SHOW_FIXED_PTS, GetTabMsg(/*Highlight @Fixed Points*/295), "", true);
    viewmenu->Append(menu_VIEW_SHOW_EXPORTED_PTS, GetTabMsg(/*Highlight E@xported Points*/296), "", true);
    viewmenu->AppendSeparator();
#else
    viewmenu-> Append(menu_VIEW_CANCEL_DIST_LINE, GetTabMsg(/*@Cancel Measuring Line##Escape*/281));
#endif
    viewmenu->Append(menu_VIEW_PERSPECTIVE, GetTabMsg(/*@Perspective*/237), "", true);
    viewmenu->Append(menu_VIEW_TEXTURED, GetTabMsg(/*Textured @Walls*/238), "", true);
    viewmenu->Append(menu_VIEW_FOG, GetTabMsg(/*Fade @Distant Objects*/239), "", true);
    viewmenu->Append(menu_VIEW_SMOOTH_LINES, GetTabMsg(/*@Smoothed Survey Legs*/298), "", true);
    viewmenu->AppendSeparator();
    viewmenu->Append(menu_VIEW_FULLSCREEN, GetTabMsg(/*@Full Screen Mode##F11*/356), "", true);
#ifdef PREFDLG
    viewmenu->AppendSeparator();
    viewmenu-> Append(menu_VIEW_PREFERENCES, GetTabMsg(/*@Preferences...*/347));
#endif

#ifndef PREFDLG
    wxMenu* ctlmenu = new wxMenu;
    ctlmenu->Append(menu_CTL_REVERSE, GetTabMsg(/*@Reverse Sense##Ctrl+R*/280), "", true);
    ctlmenu->AppendSeparator();
    ctlmenu->Append(menu_CTL_CANCEL_DIST_LINE, GetTabMsg(/*@Cancel Measuring Line##Escape*/281));
    ctlmenu->AppendSeparator();
    wxMenu* indmenu = new wxMenu;
    indmenu->Append(menu_IND_COMPASS, GetTabMsg(/*@Compass*/274), "", true);
    indmenu->Append(menu_IND_CLINO, GetTabMsg(/*C@linometer*/275), "", true);
    indmenu->Append(menu_IND_DEPTH_BAR, GetTabMsg(/*@Depth Bar*/276), "", true);
    indmenu->Append(menu_IND_SCALE_BAR, GetTabMsg(/*@Scale Bar*/277), "", true);
    ctlmenu->Append(menu_CTL_INDICATORS, GetTabMsg(/*@Indicators*/299), indmenu);
    ctlmenu->Append(menu_CTL_SIDE_PANEL, GetTabMsg(/*@Side Panel*/337), "", true);
    ctlmenu->AppendSeparator();
    ctlmenu->Append(menu_CTL_METRIC, GetTabMsg(/*@Metric*/342), "", true);
    ctlmenu->Append(menu_CTL_DEGREES, GetTabMsg(/*@Degrees*/343), "", true);
#endif

    wxMenu* helpmenu = new wxMenu;
    helpmenu->Append(menu_HELP_ABOUT, GetTabMsg(/*@About...*/290));

    wxMenuBar* menubar = new wxMenuBar(wxMB_DOCKABLE);
    menubar->Append(filemenu, GetTabMsg(/*@File*/210));
    menubar->Append(rotmenu, GetTabMsg(/*@Rotation*/211));
    menubar->Append(orientmenu, GetTabMsg(/*@Orientation*/212));
    menubar->Append(viewmenu, GetTabMsg(/*@View*/213));
#ifndef PREFDLG
    menubar->Append(ctlmenu, GetTabMsg(/*@Controls*/214));
#endif
    menubar->Append(presmenu, GetTabMsg(/*@Presentation*/216));
    menubar->Append(helpmenu, GetTabMsg(/*@Help*/215));
    SetMenuBar(menubar);
}

// ICON must be a literal string.
#define TOOLBAR_BITMAP(ICON) wxBitmap(icon_path + ICON".png", wxBITMAP_TYPE_PNG)

void MainFrm::CreateToolBar()
{
    // Create the toolbar.

    wxToolBar* toolbar = wxFrame::CreateToolBar();

#ifdef __WXGTK12__
    toolbar->SetMargins(5, 5);
#endif

    // FIXME: TRANSLATE tooltips
    toolbar->AddTool(menu_FILE_OPEN, TOOLBAR_BITMAP("open"), "Open a 3D file for viewing");
    toolbar->AddTool(menu_PRES_OPEN, TOOLBAR_BITMAP("open-pres"), "Open a presentation");
    toolbar->AddSeparator();
    toolbar->AddTool(menu_ROTATION_TOGGLE, TOOLBAR_BITMAP("rotation"),
		     wxNullBitmap, true, -1, -1, NULL, "Toggle rotation");
    toolbar->AddTool(menu_ORIENT_PLAN, TOOLBAR_BITMAP("plan"), "Switch to plan view");
    toolbar->AddTool(menu_ORIENT_ELEVATION, TOOLBAR_BITMAP("elevation"), "Switch to elevation view");
    toolbar->AddTool(menu_ORIENT_DEFAULTS, TOOLBAR_BITMAP("defaults"), "Restore default view");
    toolbar->AddSeparator();
    toolbar->AddTool(menu_VIEW_SHOW_NAMES, TOOLBAR_BITMAP("names"), wxNullBitmap, true,
		     -1, -1, NULL, "Show station names");
    toolbar->AddTool(menu_VIEW_SHOW_CROSSES, TOOLBAR_BITMAP("crosses"), wxNullBitmap, true,
		     -1, -1, NULL, "Show crosses on stations");
    toolbar->AddTool(menu_VIEW_SHOW_ENTRANCES, TOOLBAR_BITMAP("entrances"), wxNullBitmap, true,
		     -1, -1, NULL, "Highlight entrances");
    toolbar->AddTool(menu_VIEW_SHOW_FIXED_PTS, TOOLBAR_BITMAP("fixed-pts"), wxNullBitmap, true,
		     -1, -1, NULL, "Highlight fixed points");
    toolbar->AddTool(menu_VIEW_SHOW_EXPORTED_PTS, TOOLBAR_BITMAP("exported-pts"), wxNullBitmap, true,
		     -1, -1, NULL, "Highlight exported stations");
    toolbar->AddSeparator();
    toolbar->AddTool(menu_VIEW_SHOW_LEGS, TOOLBAR_BITMAP("ug-legs"), wxNullBitmap, true,
		     -1, -1, NULL, "Show underground surveys");
    toolbar->AddTool(menu_VIEW_SHOW_SURFACE, TOOLBAR_BITMAP("surface-legs"), wxNullBitmap, true,
		     -1, -1, NULL, "Show surface surveys");
    toolbar->AddTool(menu_VIEW_SHOW_TUBES, TOOLBAR_BITMAP("tubes"), wxNullBitmap, true,
		     -1, -1, NULL, "Show passage tubes");
    toolbar->AddSeparator();
    toolbar->AddTool(menu_PRES_FREWIND, TOOLBAR_BITMAP("pres-frew"), wxNullBitmap, true, -1, -1, NULL, "Very Fast Rewind");
    toolbar->AddTool(menu_PRES_REWIND, TOOLBAR_BITMAP("pres-rew"), wxNullBitmap, true, -1, -1, NULL, "Fast Rewind");
    toolbar->AddTool(menu_PRES_REVERSE, TOOLBAR_BITMAP("pres-go-back"), wxNullBitmap, true, -1, -1, NULL, "Play Backwards");
    toolbar->AddTool(menu_PRES_PAUSE, TOOLBAR_BITMAP("pres-pause"), wxNullBitmap, true, -1, -1, NULL, "Pause");
    toolbar->AddTool(menu_PRES_PLAY, TOOLBAR_BITMAP("pres-go"), wxNullBitmap, true, -1, -1, NULL, "Play");
    toolbar->AddTool(menu_PRES_FF, TOOLBAR_BITMAP("pres-ff"), wxNullBitmap, true, -1, -1, NULL, "Fast Forward");
    toolbar->AddTool(menu_PRES_FFF, TOOLBAR_BITMAP("pres-fff"), wxNullBitmap, true, -1, -1, NULL, "Very Fast Forward");
    toolbar->AddTool(menu_PRES_STOP, TOOLBAR_BITMAP("pres-stop"), wxNullBitmap, false, -1, -1, NULL, "Stop");


    toolbar->AddSeparator();
    m_FindBox = new wxTextCtrl(toolbar, textctrl_FIND, "", wxDefaultPosition,
			       wxDefaultSize, wxTE_PROCESS_ENTER);
    toolbar->AddControl(m_FindBox);
    toolbar->AddTool(button_FIND, TOOLBAR_BITMAP("find"),
		     msg(/*Find*/332)/*"Search for station name"*/);
    toolbar->AddTool(button_HIDE, TOOLBAR_BITMAP("hideresults"),
		     msg(/*Hide*/333)/*"Hide search results"*/);

    toolbar->Realize();
}

void MainFrm::CreateSidePanel()
{
    m_Splitter = new AvenSplitterWindow(this);

    m_Notebook = new wxNotebook(m_Splitter, 400, wxDefaultPosition,
				wxDefaultSize,
				wxNB_BOTTOM | wxNB_LEFT);
    m_Notebook->Show(false);

    m_Panel = new wxPanel(m_Notebook);
    m_Tree = new AvenTreeCtrl(this, m_Panel);

//    m_RegexpCheckBox = new wxCheckBox(find_panel, -1,
//				      msg(/*Regular expression*/334));
//    m_Found = new wxStaticText(find_panel, -1, "");

    wxBoxSizer *panel_sizer = new wxBoxSizer(wxVERTICAL);
    panel_sizer->Add(m_Tree, 1, wxALL | wxEXPAND, 2);
    m_Panel->SetAutoLayout(true);
    m_Panel->SetSizer(panel_sizer);
//    panel_sizer->Fit(m_Panel);
//    panel_sizer->SetSizeHints(m_Panel);

    m_Control = new GUIControl();
    m_Gfx = new GfxCore(this, m_Splitter, m_Control);
    m_Control->SetView(m_Gfx);

    // Presentation panel:
    m_PresPanel = new wxPanel(m_Notebook);

    m_PresList = new AvenPresList(this, m_PresPanel, m_Gfx);

    wxBoxSizer *pres_panel_sizer = new wxBoxSizer(wxVERTICAL);
    pres_panel_sizer->Add(m_PresList, 1, wxALL | wxEXPAND, 2);
    m_PresPanel->SetAutoLayout(true);
    m_PresPanel->SetSizer(pres_panel_sizer);

    // Overall tabbed structure:
    // FIXME: this assumes images are 15x15
    wxImageList* image_list = new wxImageList(15, 15);
    const wxString path = wxString(msg_cfgpth()) + wxCONFIG_PATH_SEPARATOR +
			  wxString("icons") + wxCONFIG_PATH_SEPARATOR;
    image_list->Add(wxBitmap(path + "survey-tree.png", wxBITMAP_TYPE_PNG));
    image_list->Add(wxBitmap(path + "pres-tree.png", wxBITMAP_TYPE_PNG));
    m_Notebook->SetImageList(image_list);
    m_Notebook->AddPage(m_Panel, msg(/*Surveys*/376), true, 0);
    m_Notebook->AddPage(m_PresPanel, msg(/*Presentation*/377), false, 1);

    m_Splitter->Initialize(m_Gfx);
}

bool MainFrm::LoadData(const wxString& file, wxString prefix)
{
    // Load survey data from file, centre the dataset around the origin,
    // chop legs such that no legs cross depth colour boundaries and prepare
    // the data for drawing.

    // Load the survey data.
#if 0
    wxStopWatch timer;
    timer.Start();
#endif

    img* survey = img_open_survey(file, prefix.c_str());
    if (!survey) {
	wxString m = wxString::Format(msg(img_error()), file.c_str());
	wxGetApp().ReportError(m);
	return false;
    }

    m_File = survey->filename_opened;

    m_Tree->DeleteAllItems();

    m_TreeRoot = m_Tree->AddRoot(wxFileNameFromPath(file));
    m_Tree->SetEnabled();

    // Create a list of all the leg vertices, counting them and finding the
    // extent of the survey at the same time.

    m_NumCrosses = 0;
    m_NumFixedPts = 0;
    m_NumExportedPts = 0;
    m_NumEntrances = 0;
    m_HasUndergroundLegs = false;
    m_HasSurfaceLegs = false;

    // FIXME: discard existing presentation? ask user about saving if we do!

    // Delete any existing list entries.
    m_Labels.clear();

    Double xmin = DBL_MAX;
    Double xmax = -DBL_MAX;
    Double ymin = DBL_MAX;
    Double ymax = -DBL_MAX;
    m_ZMin = DBL_MAX;
    Double zmax = -DBL_MAX;

    traverses.clear();
    surface_traverses.clear();
    tubes.clear();

    // Ultimately we probably want different types (subclasses perhaps?) for
    // underground and surface data, so we don't need to store LRUD for surface
    // stuff.
    vector<PointInfo> * current_traverse = NULL;
    vector<PointInfo> * current_surface_traverse = NULL;
    vector<XSect> * current_tube = NULL;

    int result;
    img_point prev_pt = {0,0,0};
    bool current_polyline_is_surface = false;
    bool pending_move = false;
    do {
#if 0
	if (++items % 200 == 0) {
	    long pos = ftell(survey->fh);
	    int progress = int((double(pos) / double(file_size)) * 100.0);
	    // SetProgress(progress);
	}
#endif

	img_point pt;
	result = img_read_item(survey, &pt);
	switch (result) {
	    case img_MOVE:
		pending_move = true;
		prev_pt = pt;
		break;

	    case img_LINE: {
		// Update survey extents.
		if (pt.x < xmin) xmin = pt.x;
		if (pt.x > xmax) xmax = pt.x;
		if (pt.y < ymin) ymin = pt.y;
		if (pt.y > ymax) ymax = pt.y;
		if (pt.z < m_ZMin) m_ZMin = pt.z;
		if (pt.z > zmax) zmax = pt.z;

		bool is_surface = (survey->flags & img_FLAG_SURFACE);
		if (pending_move || current_polyline_is_surface != is_surface) {
		    if (!current_polyline_is_surface && current_traverse) {
			//FixLRUD(*current_traverse);
		    }
		    current_polyline_is_surface = is_surface;
		    // Start new traverse (surface or underground).
		    if (is_surface) {
			m_HasSurfaceLegs = true;
			surface_traverses.push_back(vector<PointInfo>());
			current_surface_traverse = &surface_traverses.back();
		    } else {
			m_HasUndergroundLegs = true;
			traverses.push_back(vector<PointInfo>());
			current_traverse = &traverses.back();
		    }
		    if (pending_move) {
			// Update survey extents.  We only need to do this if
			// there's a pending move, since for a surface <->
			// underground transition, we'll already have handled
			// this point.
			if (prev_pt.x < xmin) xmin = prev_pt.x;
			if (prev_pt.x > xmax) xmax = prev_pt.x;
			if (prev_pt.y < ymin) ymin = prev_pt.y;
			if (prev_pt.y > ymax) ymax = prev_pt.y;
			if (prev_pt.z < m_ZMin) m_ZMin = prev_pt.z;
			if (prev_pt.z > zmax) zmax = prev_pt.z;
		    }

		    if (is_surface) {
			current_surface_traverse->push_back(PointInfo(prev_pt));
		    } else {
			current_traverse->push_back(PointInfo(prev_pt));
		    }
		}

		if (is_surface) {
		    current_surface_traverse->push_back(PointInfo(pt));
		} else {
		    current_traverse->push_back(PointInfo(pt));
		}

		prev_pt = pt;
		pending_move = false;
		break;
	    }

	    case img_LABEL: {
		LabelInfo* label = new LabelInfo;
		label->text = survey->label;
		label->x = pt.x;
		label->y = pt.y;
		label->z = pt.z;
		if (survey->flags & img_SFLAG_ENTRANCE) {
		    survey->flags ^= (img_SFLAG_ENTRANCE | LFLAG_ENTRANCE);
		}
		label->flags = survey->flags;
		if (label->IsEntrance()) {
		    m_NumEntrances++;
		}
		if (label->IsFixedPt()) {
		    m_NumFixedPts++;
		}
		if (label->IsExportedPt()) {
		    m_NumExportedPts++;
		}
		m_Labels.push_back(label);
		m_NumCrosses++;

		break;
	    }

	    case img_XSECT: {
		if (!current_tube) {
		    // Start new current_tube.
		    tubes.push_back(vector<XSect>());
		    current_tube = &tubes.back();
		}

		// FIXME: avoid linear search...
		list<LabelInfo*>::const_iterator i = m_Labels.begin();
		while (i != m_Labels.end() && (*i)->GetText() != survey->label) ++i;
		assert(i != m_Labels.end()); // FIXME: shouldn't use assert for this...

		current_tube->push_back(XSect((*i)->GetX(), (*i)->GetY(), (*i)->GetZ(), survey->l, survey->r, survey->u, survey->d));
		if (survey->flags & img_XFLAG_END) {
		    // Finish off current_tube.
		    current_tube = NULL;
		}

		break;
	    }

	    case img_BAD: {
		m_Labels.clear();

		// FIXME: Do we need to reset all these? - Olly
		m_NumCrosses = 0;
		m_NumFixedPts = 0;
		m_NumExportedPts = 0;
		m_NumEntrances = 0;
		m_HasUndergroundLegs = false;
		m_HasSurfaceLegs = false;

		m_ZMin = DBL_MAX;

		img_close(survey);

		wxString m = wxString::Format(msg(img_error()), file.c_str());
		wxGetApp().ReportError(m);

		return false;
	    }

	    default:
		break;
	}
    } while (result != img_STOP);

    if (!current_polyline_is_surface && current_traverse) {
	//FixLRUD(*current_traverse);
    }

    assert(!current_tube);

    separator = survey->separator;
    m_Title = survey->title;
    m_DateStamp = survey->datestamp;
    img_close(survey);

    // Check we've actually loaded some legs or stations!
    if (!m_HasUndergroundLegs && !m_HasSurfaceLegs && m_Labels.empty()) {
	wxString m = wxString::Format(msg(/*No survey data in 3d file `%s'*/202), file.c_str());
	wxGetApp().ReportError(m);
	return false;
    }

    if (traverses.empty() && surface_traverses.empty()) {
	// No legs, so get survey extents from stations
	list<LabelInfo*>::const_iterator i;
	for (i = m_Labels.begin(); i != m_Labels.end(); ++i) {
	    if ((*i)->x < xmin) xmin = (*i)->x;
	    if ((*i)->x > xmax) xmax = (*i)->x;
	    if ((*i)->y < ymin) ymin = (*i)->y;
	    if ((*i)->y > ymax) ymax = (*i)->y;
	    if ((*i)->z < m_ZMin) m_ZMin = (*i)->z;
	    if ((*i)->z > zmax) zmax = (*i)->z;
	}
    }

    m_XExt = xmax - xmin;
    m_YExt = ymax - ymin;
    m_ZExt = zmax - m_ZMin;

    // Sort the labels.
    m_Labels.sort(LabelCmp(separator));

    // Fill the tree of stations and prefixes.
    FillTree();
    m_Tree->Expand(m_TreeRoot);

    // Sort labels so that entrances are displayed in preference,
    // then fixed points, then exported points, then other points.
    //
    // Also sort by leaf name so that we'll tend to choose labels
    // from different surveys, rather than labels from surveys which
    // are earlier in the list.
    m_Labels.sort(LabelPlotCmp(separator));

    // Centre the dataset around the origin.
    CentreDataset(xmin, ymin, m_ZMin);

#if 0
    printf("time to load = %.3f\n", (double)timer.Time());
#endif

    // Update window title.
    SetTitle(wxString(APP_NAME" - [") + m_File + wxString("]"));

    return true;
}

// Run along a newly read in traverse and make up plausible LRUD where
// it is missing.
void
MainFrm::FixLRUD(vector<PointInfo> & centreline)
{
#if 0
    assert(centreline.size() > 1);

    Double last_size = 0;
    vector<PointInfo>::iterator i = centreline.begin();
    while (i != centreline.end()) {
	// Get the coordinates of this vertex.
	PointInfo & pt_v = *i++;
	Double size;

	if (i != centreline.end()) {
	    Double h = sqrd(i->GetX() - pt_v.GetX()) +
		       sqrd(i->GetY() - pt_v.GetY());
	    Double v = sqrd(i->GetZ() - pt_v.GetZ());
	    if (h + v > 30.0 * 30.0) {
		Double scale = 30.0 / sqrt(h + v);
		h *= scale;
		v *= scale;
	    }
	    size = sqrt(h + v / 9);
	    size /= 4;
	    if (i == centreline.begin() + 1) {
		// First segment.
		last_size = size;
	    } else {
		// Intermediate segment.
		swap(size, last_size);
		size += last_size;
		size /= 2;
	    }
	} else {
	    // Last segment.
	    size = last_size;
	}

	Double & l = pt_v.l;
	Double & r = pt_v.r;
	Double & u = pt_v.u;
	Double & d = pt_v.d;

	if (l == 0 && r == 0 && u == 0 && d == 0) {
	    l = r = u = d = -size;
	} else {
	    if (l < 0 && r < 0) {
		l = r = -size;
	    } else if (l < 0) {
		l = -(2 * size - r);
		if (l >= 0) l = -0.01;
	    } else if (r < 0) {
		r = -(2 * size - l);
		if (r >= 0) r = -0.01;
	    }
	    if (u < 0 && d < 0) {
		u = d = -size;
	    } else if (u < 0) {
		u = -(2 * size - d);
		if (u >= 0) u = -0.01;
	    } else if (d < 0) {
		d = -(2 * size - u);
		if (d >= 0) d = -0.01;
	    }
	}
    }
#endif
}

void MainFrm::FillTree()
{
    // Fill the tree of stations and prefixes.

    stack<wxTreeItemId> previous_ids;
    wxString current_prefix = "";
    wxTreeItemId current_id = m_TreeRoot;

    list<LabelInfo*>::iterator pos = m_Labels.begin();
    while (pos != m_Labels.end()) {
	LabelInfo* label = *pos++;

	// Determine the current prefix.
	wxString prefix = label->GetText().BeforeLast(separator);

	// Determine if we're still on the same prefix.
	if (prefix == current_prefix) {
	    // no need to fiddle with branches...
	}
	// If not, then see if we've descended to a new prefix.
	else if (prefix.Length() > current_prefix.Length() &&
		 prefix.StartsWith(current_prefix) &&
		 (prefix[current_prefix.Length()] == separator ||
		  current_prefix == "")) {
	    // We have, so start as many new branches as required.
	    int current_prefix_length = current_prefix.Length();
	    current_prefix = prefix;
	    if (current_prefix_length != 0) {
		prefix = prefix.Mid(current_prefix_length + 1);
	    }
	    int next_dot;
	    do {
		// Extract the next bit of prefix.
		next_dot = prefix.Find(separator);

		wxString bit = next_dot == -1 ? prefix : prefix.Left(next_dot);
		assert(bit != "");

		// Add the current tree ID to the stack.
		previous_ids.push(current_id);

		// Append the new item to the tree and set this as the current branch.
		current_id = m_Tree->AppendItem(current_id, bit);
		m_Tree->SetItemData(current_id, new TreeData(NULL));
		prefix = prefix.Mid(next_dot + 1);
	    } while (next_dot != -1);
	}
	// Otherwise, we must have moved up, and possibly then down again.
	else {
	    size_t count = 0;
	    bool ascent_only = (prefix.Length() < current_prefix.Length() &&
				current_prefix.StartsWith(prefix) &&
				(current_prefix[prefix.Length()] == separator ||
				 prefix == ""));
	    if (!ascent_only) {
		// Find out how much of the current prefix and the new prefix
		// are the same.
		// Note that we require a match of a whole number of parts
		// between dots!
		for (size_t i = 0; prefix[i] == current_prefix[i]; ++i) {
		    if (prefix[i] == separator) count = i + 1;
		}
	    } else {
		count = prefix.Length() + 1;
	    }

	    // Extract the part of the current prefix after the bit (if any)
	    // which has matched.
	    // This gives the prefixes to ascend over.
	    wxString prefixes_ascended = current_prefix.Mid(count);

	    // Count the number of prefixes to ascend over.
	    int num_prefixes = prefixes_ascended.Freq(separator);

	    // Reverse up over these prefixes.
	    for (int i = 1; i <= num_prefixes; i++) {
		previous_ids.pop();
	    }
	    current_id = previous_ids.top();
	    previous_ids.pop();

	    if (!ascent_only) {
		// Now extract the bit of new prefix.
		wxString new_prefix = prefix.Mid(count);

		// Add branches for this new part.
		while (true) {
		    // Extract the next bit of prefix.
		    int next_dot = new_prefix.Find(separator);

		    wxString bit;
		    if (next_dot == -1) {
			bit = new_prefix;
		    } else {
			bit = new_prefix.Left(next_dot);
		    }

		    // Add the current tree ID to the stack.
		    previous_ids.push(current_id);

		    // Append the new item to the tree and set this as the
		    // current branch.
		    current_id = m_Tree->AppendItem(current_id, bit);
		    m_Tree->SetItemData(current_id, new TreeData(NULL));

		    if (next_dot == -1) break;

		    new_prefix = new_prefix.Mid(next_dot + 1);
		}
	    }

	    current_prefix = prefix;
	}

	// Now add the leaf.
	wxString bit = label->GetText().AfterLast(separator);
	assert(bit != "");
	wxTreeItemId id = m_Tree->AppendItem(current_id, bit);
	m_Tree->SetItemData(id, new TreeData(label));
	label->tree_id = id; // before calling SetTreeItemColour()...
	SetTreeItemColour(label);
    }
}

void MainFrm::SelectTreeItem(LabelInfo* label)
{
    m_Tree->SelectItem(label->tree_id);
}

void MainFrm::SetTreeItemColour(LabelInfo* label)
{
    // Set the colour for an item in the survey tree.

    if (label->IsEntrance()) {
	m_Tree->SetItemTextColour(label->tree_id, wxColour(0, 255, 0));
    } else if (label->IsSurface()) {
	m_Tree->SetItemTextColour(label->tree_id, wxColour(49, 158, 79));
    }
}

void MainFrm::CentreDataset(Double xmin, Double ymin, Double zmin)
{
    // Centre the dataset around the origin.

    Double xoff = m_Offsets.x = xmin + (m_XExt / 2.0);
    Double yoff = m_Offsets.y = ymin + (m_YExt / 2.0);
    Double zoff = m_Offsets.z = zmin + (m_ZExt / 2.0);

    list<vector<PointInfo> >::iterator t = traverses.begin();
    while (t != traverses.end()) {
	assert(t->size() > 1);
	vector<PointInfo>::iterator pos = t->begin();
	while (pos != t->end()) {
	    PointInfo & point = *pos++;
	    point.x -= xoff;
	    point.y -= yoff;
	    point.z -= zoff;
	}
	++t;
    }

    t = surface_traverses.begin();
    while (t != surface_traverses.end()) {
	assert(t->size() > 1);
	vector<PointInfo>::iterator pos = t->begin();
	while (pos != t->end()) {
	    PointInfo & point = *pos++;
	    point.x -= xoff;
	    point.y -= yoff;
	    point.z -= zoff;
	}
	++t;
    }

    list<vector<XSect> >::iterator i = tubes.begin();
    while (i != tubes.end()) {
	assert(i->size() > 1);
	vector<XSect>::iterator pos = i->begin();
	while (pos != i->end()) {
	    XSect & point = *pos++;
	    point.x -= xoff;
	    point.y -= yoff;
	    point.z -= zoff;
	}
	++i;
    }

    list<LabelInfo*>::iterator lpos = m_Labels.begin();
    while (lpos != m_Labels.end()) {
	LabelInfo* label = *lpos++;
	label->x -= xoff;
	label->y -= yoff;
	label->z -= zoff;
    }
}

void MainFrm::OnMRUFile(wxCommandEvent& event)
{
    wxString f(m_history.GetHistoryFile(event.GetId() - wxID_FILE1));
    if (!f.empty()) OpenFile(f);
}

void MainFrm::OpenFile(const wxString& file, wxString survey)
{
    wxBusyCursor hourglass;
    if (LoadData(file, survey)) {
	if (wxIsAbsolutePath(m_File)) {
	    m_history.AddFileToHistory(m_File);
	} else {
	    wxString abs = wxGetCwd() + wxString(FNM_SEP_LEV) + m_File;
	    m_history.AddFileToHistory(abs);
	}
	wxConfigBase *b = wxConfigBase::Get();
	m_history.Save(*b);
	b->Flush();

	int x;
	int y;
	GetClientSize(&x, &y);
	if (x < 600)
	    x /= 3;
	else if (x < 1000)
	    x = 200;
	else
	    x /= 5;

	m_Splitter->SplitVertically(m_Notebook, m_Gfx, x);
	m_SashPosition = x; // Save width of panel.

	m_Gfx->Initialise();
	m_Notebook->Show(true);

	m_Gfx->SetFocus();
    }
}

//
//  UI event handlers
//

#undef FILEDIALOG_MULTIGLOBS
// MS Windows supports "*.abc;*.def" natively; wxGtk supports them as of 2.3
#if defined(_WIN32) || wxCHECK_VERSION(2,3,0)
# define FILEDIALOG_MULTIGLOBS
#endif

void MainFrm::OnOpen(wxCommandEvent&)
{
    AvenAllowOnTop ontop(this);
#ifdef __WXMOTIF__
    wxFileDialog dlg (this, wxString(msg(/*Select a 3d file to view*/206)), "", "",
		      "*.3d", wxOPEN);
#else
    wxFileDialog dlg(this, wxString(msg(/*Select a 3d file to view*/206)), "", "",
		     wxString::Format("%s|*.3d"
#ifdef FILEDIALOG_MULTIGLOBS
				      ";*.3D"
#endif
#ifdef FILEDIALOG_MULTIGLOBS
				      "|%s|*.plt;*.plf"
#ifndef _WIN32
				      ";*.PLT;*.PLF"
#endif
#else
				      "|%s|*.pl?" // not ideal...
#endif
				      "|%s|*.xyz"
#ifdef FILEDIALOG_MULTIGLOBS
#ifndef _WIN32
				      ";*.XYZ"
#endif
#endif
				      "|%s|%s",
				      msg(/*Survex 3d files*/207),
				      msg(/*Compass PLT files*/324),
				      msg(/*CMAP XYZ files*/325),
				      msg(/*All files*/208),
				      wxFileSelectorDefaultWildcardStr),
		     wxOPEN);
#endif
    if (dlg.ShowModal() == wxID_OK) {
	OpenFile(dlg.GetPath());
    }
}

void MainFrm::OnScreenshot(wxCommandEvent&)
{
    AvenAllowOnTop ontop(this);
    char *baseleaf = baseleaf_from_fnm(m_File.c_str());
    wxFileDialog dlg (this, msg(/*Save Screenshot*/321), "",
		      wxString(baseleaf) + ".png",
		      "*.png", wxSAVE|wxOVERWRITE_PROMPT);
    free(baseleaf);
    if (dlg.ShowModal() == wxID_OK) {
	static bool png_handled = false;
	if (!png_handled) {
#if 0 // FIXME : enable this is we allow other export formats...
	    ::wxInitAllImageHandlers();
#else
	    wxImage::AddHandler(new wxPNGHandler);
#endif
	    png_handled = true;
	}
	if (!m_Gfx->SaveScreenshot(dlg.GetPath(), wxBITMAP_TYPE_PNG)) {
	    wxGetApp().ReportError(wxString::Format(msg(/*Error writing to file `%s'*/110), dlg.GetPath().c_str()));
	}
    }
}

void MainFrm::OnScreenshotUpdate(wxUpdateUIEvent& event)
{
    event.Enable(!m_File.empty());
}

void MainFrm::OnFilePreferences(wxCommandEvent&)
{
#ifdef PREFDLG
    m_PrefsDlg = new PrefsDlg(m_Gfx, this);
    m_PrefsDlg->Show(true);
#endif
}

void MainFrm::OnPrint(wxCommandEvent&)
{
    m_Gfx->OnPrint(m_File, m_Title, m_DateStamp);
}

void MainFrm::OnPageSetup(wxCommandEvent&)
{
    m_pageSetupData.SetPrintData(m_printData);

    wxPageSetupDialog pageSetupDialog(this, &m_pageSetupData);
    pageSetupDialog.ShowModal();

    m_printData = pageSetupDialog.GetPageSetupData().GetPrintData();
    m_pageSetupData = pageSetupDialog.GetPageSetupData();
}

void MainFrm::OnExport(wxCommandEvent&)
{
    char *baseleaf = baseleaf_from_fnm(m_File.c_str());
    wxFileDialog dlg(this, wxString("Export as:"), "",
		     wxString(baseleaf),
		     "DXF files|*.dxf|SVG files|*.svg|Sketch files|*.sk|EPS files|*.eps|Compass PLT for use with Carto|*.plt",
		     wxSAVE|wxOVERWRITE_PROMPT);
    free(baseleaf);
    if (dlg.ShowModal() == wxID_OK) {
	wxString fnm = dlg.GetPath();
	if (!m_Gfx->OnExport(fnm, m_Title)) {
	    wxGetApp().ReportError(wxString::Format("Couldn't write file `%s'", fnm.c_str()));
	}
    }
}

void MainFrm::OnQuit(wxCommandEvent&)
{
    if (m_PresList->Modified()) {
	AvenAllowOnTop ontop(this);
	// FIXME: better to ask "Do you want to save your changes?" and offer [Save] [Discard] [Cancel]
	if (wxMessageBox(msg(/*The current presentation has been modified.  Abandon unsaved changes?*/327),
			 msg(/*Modified Presentation*/326),
			 wxOK|wxCANCEL|wxICON_QUESTION) == wxCANCEL) {
	    return;
	}
    }
    exit(0);
}

void MainFrm::OnClose(wxCloseEvent&)
{
    if (m_PresList->Modified()) {
	AvenAllowOnTop ontop(this);
	// FIXME: better to ask "Do you want to save your changes?" and offer [Save] [Discard] [Cancel]
	if (wxMessageBox(msg(/*The current presentation has been modified.  Abandon unsaved changes?*/327),
			 msg(/*Modified Presentation*/326),
			 wxOK|wxCANCEL|wxICON_QUESTION) == wxCANCEL) {
	    return;
	}
    }
    exit(0);
}

void MainFrm::OnAbout(wxCommandEvent&)
{
    AvenAllowOnTop ontop(this);
    AboutDlg dlg(this, icon_path);
    dlg.Centre();
    dlg.ShowModal();
}

void MainFrm::ClearTreeSelection()
{
    m_Tree->UnselectAll();
    m_Gfx->SetThere();
}

void MainFrm::ClearCoords()
{
    GetStatusBar()->SetStatusText("");
}

void MainFrm::SetCoords(Double x, Double y, Double z)
{
    wxString str;
    if (m_Gfx->GetMetric()) {
	str.Printf(msg(/*  %d E, %d N*/338), int(x), int(y));
	str += wxString::Format(", %s %dm", msg(/*Altitude*/335), int(z));
    } else {
	str.Printf(msg(/*  %d E, %d N*/338),
		   int(x / METRES_PER_FOOT), int(y / METRES_PER_FOOT));
	str += wxString::Format(", %s %dft", msg(/*Altitude*/335),
		   int(z / METRES_PER_FOOT));
    }
    GetStatusBar()->SetStatusText(str);
}

void MainFrm::SetCoords(Double x, Double y)
{
    wxString str;
    if (m_Gfx->GetMetric()) {
	str.Printf(msg(/*  %d E, %d N*/338), int(x), int(y));
    } else {
	str.Printf(msg(/*  %d E, %d N*/338),
		   int(x / METRES_PER_FOOT), int(y / METRES_PER_FOOT));
    }
    GetStatusBar()->SetStatusText(str);
}

void MainFrm::SetAltitude(Double z)
{
    wxString str;
    if (m_Gfx->GetMetric()) {
	str.Printf("  %s %dm", msg(/*Altitude*/335),
		   int(z));
    } else {
	str.Printf("  %s %dft", msg(/*Altitude*/335),
		   int(z / METRES_PER_FOOT));
    }
    GetStatusBar()->SetStatusText(str);
}

void MainFrm::ClearInfo()
{
    m_Gfx->SetHere();
    m_Gfx->SetThere();
    GetStatusBar()->SetStatusText("", 1);
    GetStatusBar()->SetStatusText("", 2);
}

void MainFrm::ShowInfo(const LabelInfo *label)
{
    assert(m_Gfx);

    wxString str;
    if (m_Gfx->GetMetric()) {
	str.Printf(msg(/*%s: %d E, %d N, %dm altitude*/374),
		   label->text.GetData(),
		   int(label->x + m_Offsets.x),
		   int(label->y + m_Offsets.y),
		   int(label->z + m_Offsets.z));
    } else {
	str.Printf(msg(/*%s: %d E, %d N, %dft altitude*/375),
		   label->text.GetData(),
		   int((label->x + m_Offsets.x) / METRES_PER_FOOT),
		   int((label->y + m_Offsets.y) / METRES_PER_FOOT),
		   int((label->z + m_Offsets.z) / METRES_PER_FOOT));
    }
    GetStatusBar()->SetStatusText(str, 1);
    m_Gfx->SetHere(label->x, label->y, label->z);

    wxTreeItemData* sel_wx;
    bool sel = m_Tree->GetSelectionData(&sel_wx);
    if (sel) {
	TreeData *data = (TreeData*) sel_wx;

	if (data->IsStation()) {
	    const LabelInfo* label2 = data->GetLabel();
	    assert(label2);

	    Double x0 = label2->x;
	    Double x1 = label->x;
	    Double dx = x1 - x0;
	    Double y0 = label2->y;
	    Double y1 = label->y;
	    Double dy = y1 - y0;
	    Double z0 = label2->z;
	    Double z1 = label->z;
	    Double dz = z1 - z0;

	    Double d_horiz = sqrt(dx*dx + dy*dy);
	    Double dr = sqrt(dx*dx + dy*dy + dz*dz);

	    Double brg = deg(atan2(dx, dy));
	    if (brg < 0) brg += 360;

	    wxString from_str;
	    from_str.Printf(msg(/*From %s*/339), label2->text.c_str());

	    wxString hv_str;
	    if (m_Gfx->GetMetric()) {
		hv_str.Printf(msg(/*H %d%s, V %d%s*/340),
			      int(d_horiz), "m",
			      int(dz), "m");
	    } else {
		hv_str.Printf(msg(/*H %d%s, V %d%s*/340),
			      int(d_horiz / METRES_PER_FOOT), "ft",
			      int(dz / METRES_PER_FOOT), "ft");
	    }
	    wxString brg_unit;
	    if (m_Gfx->GetDegrees()) {
		brg_unit = msg(/*&deg;*/344);
	    } else {
		brg *= 400.0 / 360.0;
		brg_unit = msg(/*grad*/345);
	    }
	    if (m_Gfx->GetMetric()) {
		str.Printf(msg(/*%s: %s, Dist %d%s, Brg %03d%s*/341),
			   from_str.c_str(), hv_str.c_str(),
			   int(dr), "m", int(brg), brg_unit.c_str());
	    } else {
		str.Printf(msg(/*%s: %s, Dist %d%s, Brg %03d%s*/341),
			   from_str.c_str(), hv_str.c_str(),
			   int(dr / METRES_PER_FOOT), "ft", int(brg),
			   brg_unit.c_str());
	    }
	    GetStatusBar()->SetStatusText(str, 2);
	    m_Gfx->SetThere(x0, y0, z0);
	} else {
	    GetStatusBar()->SetStatusText("", 2);
	    m_Gfx->SetThere(); // FIXME: not in SetMouseOverStation version?
	}
    }
}

void MainFrm::DisplayTreeInfo(const wxTreeItemData* item)
{
    const TreeData* data = static_cast<const TreeData*>(item);
    if (data) {
	if (data->IsStation()) {
	    const LabelInfo * l = data->GetLabel();
	    ShowInfo(l);
	    m_Gfx->SetHere(l->x, l->y, l->z);
	} else {
	    ClearCoords();
	    m_Gfx->SetHere();
	}
    }
}

void MainFrm::TreeItemSelected(wxTreeItemData* item)
{
    TreeData* data = (TreeData*) item;

    if (data && data->IsStation()) {
	const LabelInfo* label = data->GetLabel();
	m_Gfx->CentreOn(label->x, label->y, label->z);
	m_Gfx->SetThere(label->x, label->y, label->z);
    } else {
	m_Gfx->SetThere();
    }

    // ClearCoords(); // FIXME or update or ?
}

void MainFrm::OnPresNew(wxCommandEvent&)
{
    if (m_PresList->Modified()) {
	AvenAllowOnTop ontop(this);
	// FIXME: better to ask "Do you want to save your changes?" and offer [Save] [Discard] [Cancel]
	if (wxMessageBox(msg(/*The current presentation has been modified.  Abandon unsaved changes?*/327),
			 msg(/*Modified Presentation*/326),
			 wxOK|wxCANCEL|wxICON_QUESTION) == wxCANCEL) {
	    return;
	}
    }
    m_PresList->DeleteAllItems();
}

void MainFrm::OnPresOpen(wxCommandEvent&)
{
    AvenAllowOnTop ontop(this);
    if (m_PresList->Modified()) {
	// FIXME: better to ask "Do you want to save your changes?" and offer [Save] [Discard] [Cancel]
	if (wxMessageBox(msg(/*The current presentation has been modified.  Abandon unsaved changes?*/327),
			 msg(/*Modified Presentation*/326),
			 wxOK|wxCANCEL|wxICON_QUESTION) == wxCANCEL) {
	    return;
	}
    }
#ifdef __WXMOTIF__
    wxFileDialog dlg (this, wxString(msg(/*Select a presentation to open*/322)), "", "",
		      "*.fly", wxSAVE);
#else
    wxFileDialog dlg (this, wxString(msg(/*Select a presentation to open*/322)), "", "",
		      wxString::Format("%s|*.fly|%s|%s",
				       msg(/*Aven presentations*/320),
				       msg(/*All files*/208),
				       wxFileSelectorDefaultWildcardStr),
		      wxOPEN);
#endif
    if (dlg.ShowModal() == wxID_OK) {
	if (!m_PresList->Load(dlg.GetPath())) {
	    return;
	}
	// FIXME : keep a history of loaded/saved presentations, like we do for
	// loaded surveys...
    }
}

void MainFrm::OnPresSave(wxCommandEvent&)
{
    m_PresList->Save(true);
}

void MainFrm::OnPresSaveAs(wxCommandEvent&)
{
    m_PresList->Save(false);
}

void MainFrm::OnPresMark(wxCommandEvent&)
{
    m_PresList->AddMark();
}

void MainFrm::OnPresFRewind(wxCommandEvent&)
{
    m_Gfx->PlayPres(-100);
}

void MainFrm::OnPresRewind(wxCommandEvent&)
{
    m_Gfx->PlayPres(-10);
}

void MainFrm::OnPresReverse(wxCommandEvent&)
{
    m_Gfx->PlayPres(-1);
}

void MainFrm::OnPresPlay(wxCommandEvent&)
{
    m_Gfx->PlayPres(1);
}

void MainFrm::OnPresFF(wxCommandEvent&)
{
    m_Gfx->PlayPres(10);
}

void MainFrm::OnPresFFF(wxCommandEvent&)
{
    m_Gfx->PlayPres(100);
}

void MainFrm::OnPresPause(wxCommandEvent&)
{
    m_Gfx->PlayPres(0);
}

void MainFrm::OnPresStop(wxCommandEvent&)
{
    m_Gfx->PlayPres(0, false);
}

void MainFrm::OnPresExportMovie(wxCommandEvent&)
{
    AvenAllowOnTop ontop(this);
    // FIXME : Taking the leaf of the currently loaded presentation as the
    // default might make more sense?
    char *baseleaf = baseleaf_from_fnm(m_File.c_str());
    wxFileDialog dlg(this, wxString("Export Movie"), "",
		     wxString(baseleaf) + ".mpg",
		     "MPEG|*.mpg|AVI|*.avi|QuickTime|*.mov|WMV|*.wmv;*.asf",
		     wxSAVE|wxOVERWRITE_PROMPT);
    free(baseleaf);
    if (dlg.ShowModal() == wxID_OK) {
	if (!m_Gfx->ExportMovie(dlg.GetPath())) {
	    wxGetApp().ReportError(wxString::Format(msg(/*Error writing to file `%s'*/110), dlg.GetPath().c_str()));
	}
    }
}

PresentationMark MainFrm::GetPresMark(int which)
{
    return m_PresList->GetPresMark(which);
}

//void MainFrm::OnFileOpenTerrainUpdate(wxUpdateUIEvent& event)
//{
//    event.Enable(!m_File.empty());
//}

void MainFrm::OnPresNewUpdate(wxUpdateUIEvent& event)
{
    event.Enable(!m_File.empty());
}

void MainFrm::OnPresOpenUpdate(wxUpdateUIEvent& event)
{
    event.Enable(!m_File.empty());
}

void MainFrm::OnPresSaveUpdate(wxUpdateUIEvent& event)
{
    event.Enable(!m_PresList->Empty());
}

void MainFrm::OnPresSaveAsUpdate(wxUpdateUIEvent& event)
{
    event.Enable(!m_PresList->Empty());
}

void MainFrm::OnPresMarkUpdate(wxUpdateUIEvent& event)
{
    event.Enable(!m_File.empty());
}

void MainFrm::OnPresFRewindUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_Gfx && m_Gfx->GetPresentationMode());
    event.Check(m_Gfx && m_Gfx->GetPresentationSpeed() < -10);
}

void MainFrm::OnPresRewindUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_Gfx && m_Gfx->GetPresentationMode());
    event.Check(m_Gfx && m_Gfx->GetPresentationSpeed() == -10);
}

void MainFrm::OnPresReverseUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_Gfx && m_Gfx->GetPresentationMode());
    event.Check(m_Gfx && m_Gfx->GetPresentationSpeed() == -1);
}

void MainFrm::OnPresPlayUpdate(wxUpdateUIEvent& event)
{
    event.Enable(!m_PresList->Empty());
    event.Check(m_Gfx && m_Gfx->GetPresentationMode() &&
		m_Gfx->GetPresentationSpeed() == 1);
}

void MainFrm::OnPresFFUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_Gfx && m_Gfx->GetPresentationMode());
    event.Check(m_Gfx && m_Gfx->GetPresentationSpeed() == 10);
}

void MainFrm::OnPresFFFUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_Gfx && m_Gfx->GetPresentationMode());
    event.Check(m_Gfx && m_Gfx->GetPresentationSpeed() > 10);
}

void MainFrm::OnPresPauseUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_Gfx && m_Gfx->GetPresentationMode());
    event.Check(m_Gfx && m_Gfx->GetPresentationSpeed() == 0);
}

void MainFrm::OnPresStopUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_Gfx && m_Gfx->GetPresentationMode());
}

void MainFrm::OnPresExportMovieUpdate(wxUpdateUIEvent& event)
{
    event.Enable(!m_PresList->Empty());
}

void MainFrm::OnFind(wxCommandEvent&)
{
    wxBusyCursor hourglass;
    // Find stations specified by a string or regular expression.

    wxString pattern = m_FindBox->GetValue();
    int re_flags = wxRE_NOSUB;

    if (true /* case insensitive */) {
	re_flags |= wxRE_ICASE;
    }

    bool substring = true;
    if (false /*m_RegexpCheckBox->GetValue()*/) {
	re_flags |= wxRE_EXTENDED;
    } else if (true /* simple glob-style */) {
	wxString pat;
	for (size_t i = 0; i < pattern.size(); i++) {
	   char ch = pattern[i];
	   // ^ only special at start; $ at end.  But this is simpler...
	   switch (ch) {
	    case '^': case '$': case '.': case '[': case '\\':
	      pat += '\\';
	      pat += ch;
	      break;
	    case '*':
	      pat += ".*";
              substring = false;
	      break;
	    case '?':
	      pat += '.';
              substring = false;
	      break;
	    default:
	      pat += ch;
	   }
	}
	pattern = pat;
	re_flags |= wxRE_BASIC;
    } else {
	wxString pat;
	for (size_t i = 0; i < pattern.size(); i++) {
	   char ch = pattern[i];
	   // ^ only special at start; $ at end.  But this is simpler...
	   switch (ch) {
	    case '^': case '$': case '*': case '.': case '[': case '\\':
	      pat += '\\';
	   }
	   pat += ch;
	}
	pattern = pat;
	re_flags |= wxRE_BASIC;
    }

    if (!substring) {
	// FIXME "0u" required to avoid compilation error with g++-3.0
	if (pattern.empty() || pattern[0u] != '^') pattern = '^' + pattern;
        // FIXME: this fails to cope with "\$" at the end of pattern...
	if (pattern[pattern.size() - 1] != '$') pattern += '$';
    }

    wxRegEx regex;
    if (!regex.Compile(pattern, re_flags)) {
	wxString m;
	m.Printf(msg(/*Invalid regular expression: %s*/404), pattern.c_str());
	wxGetApp().ReportError(m);
	return;
    }

    list<LabelInfo*>::iterator pos = m_Labels.begin();

    int found = 0;
    while (pos != m_Labels.end()) {
	LabelInfo* label = *pos++;

	if (regex.Matches(label->text)) {
	    label->flags |= LFLAG_HIGHLIGHTED;
	    found++;
	} else {
	    label->flags &= ~LFLAG_HIGHLIGHTED;
	}
    }


    m_NumHighlighted = found;
// FIXME:    m_Found->SetLabel(wxString::Format(msg(/*%d found*/331), found));
#ifdef _WIN32
//    m_Found->Refresh(); // FIXME
#endif
    // Re-sort so highlighted points get names in preference
    if (found) m_Labels.sort(LabelPlotCmp(separator));
    m_Gfx->UpdateBlobs();
    m_Gfx->ForceRefresh();

    if (!found) {
	wxGetApp().ReportError(msg(/*No matches were found.*/328));
        GetToolBar()->SetToolShortHelp(button_HIDE, msg(/*No matches were found.*/328));
    } else {
        GetToolBar()->SetToolShortHelp(button_HIDE, wxString::Format("Unhilight %d found stations", found));
    }

    m_Gfx->SetFocus();
}

void MainFrm::OnHide(wxCommandEvent&)
{
    // Hide any search result highlights.
// FIXME:    m_Found->SetLabel("");
    list<LabelInfo*>::iterator pos = m_Labels.begin();
    while (pos != m_Labels.end()) {
	LabelInfo* label = *pos++;
	label->flags &= ~LFLAG_HIGHLIGHTED;
    }
    m_NumHighlighted = 0;
    m_Gfx->UpdateBlobs();
    m_Gfx->ForceRefresh();
}

void MainFrm::OnHideUpdate(wxUpdateUIEvent& ui)
{
    ui.Enable(m_NumHighlighted != 0);
}

void MainFrm::SetMouseOverStation(LabelInfo* label)
{
    if (label) {
	ShowInfo(label);
    } else {
	ClearCoords();
	m_Gfx->SetHere();
    }
}

void MainFrm::OnViewSidePanel(wxCommandEvent&)
{
    ToggleSidePanel();
}

void MainFrm::ToggleSidePanel()
{
    // Toggle display of the side panel.

    assert(m_Gfx);

    if (m_Splitter->IsSplit()) {
	m_SashPosition = m_Splitter->GetSashPosition(); // save width of panel
	m_Splitter->Unsplit(m_Notebook);
    } else {
	m_Notebook->Show(true);
	m_Gfx->Show(true);
	m_Splitter->SplitVertically(m_Notebook, m_Gfx, m_SashPosition);
    }
}

void MainFrm::OnViewSidePanelUpdate(wxUpdateUIEvent& ui)
{
    ui.Enable(!m_File.empty());
    ui.Check(ShowingSidePanel());
}

bool MainFrm::ShowingSidePanel()
{
    return m_Splitter->IsSplit();
}

void MainFrm::ViewFullScreen() {
    ShowFullScreen(!IsFullScreen());
    static bool sidepanel;
    if (IsFullScreen()) sidepanel = ShowingSidePanel();
    if (sidepanel) ToggleSidePanel();
#ifndef _WIN32
    // wxGTK doesn't currently remove the toolbar, statusbar, or menubar.
    // Can't work out how to lose the menubar right now, but this works for
    // the other two.  FIXME: tidy this code up and submit a patch for
    // wxWindows.
    wxToolBar *tb = GetToolBar();
    if (tb) tb->Show(!IsFullScreen());
    wxStatusBar *sb = GetStatusBar();
    if (sb) sb->Show(!IsFullScreen());
#if 0
    // FIXME: This sort of works, but we lose the top-level shortcuts
    // (e.g. alt-F for File)
    wxMenuBar *mb = GetMenuBar();
    if (mb) {
	static list<wxMenu *> menus;
	static list<wxString> labels;
	if (IsFullScreen()) {
	    // remove menus
	    for (int c = mb->GetMenuCount(); c >= 0; --c) {
		labels.push_back(mb->GetLabelTop(c));
		menus.push_back(mb->Remove(c));
	    }
	} else {
	    while (!menus.empty()) {
		mb->Append(menus.back(), labels.back());
		menus.pop_back();
		labels.pop_back();
	    }
	}
    }
#endif
#endif
}
