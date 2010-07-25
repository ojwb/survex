//
//  mainfrm.cc
//
//  Main frame handling for Aven.
//
//  Copyright (C) 2000-2002,2005,2006 Mark R. Shinwell
//  Copyright (C) 2001-2003,2004,2005,2006 Olly Betts
//  Copyright (C) 2005 Martin Green
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
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "cavernlog.h"
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
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/imaglist.h>
#include <wx/process.h>
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
	    parent->ToggleSidePanel();
	}

    private:
	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(AvenSplitterWindow, wxSplitterWindow)
    EVT_SPLITTER_DCLICK(-1, AvenSplitterWindow::OnSplitterDClick)
END_EVENT_TABLE()

class EditMarkDlg : public wxDialog {
    wxTextCtrl * easting, * northing, * altitude;
    wxTextCtrl * angle, * tilt_angle, * scale, * time;
public:
    EditMarkDlg(wxWindow* parent, const PresentationMark & p)
	: wxDialog(parent, 500, wxString(wxT("Edit Waypoint")))
    {
	easting = new wxTextCtrl(this, 601, wxString::Format(wxT("%.3f"), p.GetX()));
	northing = new wxTextCtrl(this, 602, wxString::Format(wxT("%.3f"), p.GetY()));
	altitude = new wxTextCtrl(this, 603, wxString::Format(wxT("%.3f"), p.GetZ()));
	angle = new wxTextCtrl(this, 604, wxString::Format(wxT("%.3f"), p.angle));
	tilt_angle = new wxTextCtrl(this, 605, wxString::Format(wxT("%.3f"), p.tilt_angle));
	scale = new wxTextCtrl(this, 606, wxString::Format(wxT("%.3f"), p.scale));
	if (p.time > 0.0) {
	    time = new wxTextCtrl(this, 607, wxString::Format(wxT("%.3f"), p.time));
	} else if (p.time < 0.0) {
	    time = new wxTextCtrl(this, 607, wxString::Format(wxT("*%.3f"), -p.time));
	} else {
	    time = new wxTextCtrl(this, 607, wxT("0"));
	}

	wxBoxSizer * coords = new wxBoxSizer(wxHORIZONTAL);
	coords->Add(new wxStaticText(this, 610, wxT("(")), 0, wxALIGN_CENTRE_VERTICAL);
	coords->Add(easting, 1);
	coords->Add(new wxStaticText(this, 611, wxT(",")), 0, wxALIGN_CENTRE_VERTICAL);
	coords->Add(northing, 1);
	coords->Add(new wxStaticText(this, 612, wxT(",")), 0, wxALIGN_CENTRE_VERTICAL);
	coords->Add(altitude, 1);
	coords->Add(new wxStaticText(this, 613, wxT(")")), 0, wxALIGN_CENTRE_VERTICAL);
	wxBoxSizer* vert = new wxBoxSizer(wxVERTICAL);
	vert->Add(coords, 0, wxALL, 8);
	wxBoxSizer * r2 = new wxBoxSizer(wxHORIZONTAL);
	r2->Add(new wxStaticText(this, 614, wxT("Bearing: ")), 0, wxALIGN_CENTRE_VERTICAL);
	r2->Add(angle);
	vert->Add(r2, 0, wxALL, 8);
	wxBoxSizer * r3 = new wxBoxSizer(wxHORIZONTAL);
	r3->Add(new wxStaticText(this, 615, wxT("Elevation: ")), 0, wxALIGN_CENTRE_VERTICAL);
	r3->Add(tilt_angle);
	vert->Add(r3, 0, wxALL, 8);
	wxBoxSizer * r4 = new wxBoxSizer(wxHORIZONTAL);
	r4->Add(new wxStaticText(this, 616, wxT("Scale: ")), 0, wxALIGN_CENTRE_VERTICAL);
	r4->Add(scale);
	r4->Add(new wxStaticText(this, 617, wxT(" (unused in perspective view)")),
		0, wxALIGN_CENTRE_VERTICAL);
	vert->Add(r4, 0, wxALL, 8);

	wxBoxSizer * r5 = new wxBoxSizer(wxHORIZONTAL);
	r5->Add(new wxStaticText(this, 616, wxT("Time: ")), 0, wxALIGN_CENTRE_VERTICAL);
	r5->Add(time);
	r5->Add(new wxStaticText(this, 617, wxT(" secs (0 = auto; *6 = 6 times auto)")),
		0, wxALIGN_CENTRE_VERTICAL);
	vert->Add(r5, 0, wxALL, 8);

	wxBoxSizer * buttons = new wxBoxSizer(wxHORIZONTAL);
	wxButton* cancel = new wxButton(this, wxID_CANCEL, wxT("Cancel"));
	buttons->Add(cancel, 0, wxALL, 8);
	wxButton* ok = new wxButton(this, wxID_OK, wxT("OK"));
	ok->SetDefault();
	buttons->Add(ok, 0, wxALL, 8);
	vert->Add(buttons, 0, wxALL|wxALIGN_RIGHT);

	SetAutoLayout(true);
	SetSizer(vert);

	vert->Fit(this);
	vert->SetSizeHints(this);
    }
    PresentationMark GetMark() const {
	double a, t, s, T;
	Vector3 v(atof(easting->GetValue().char_str()),
		  atof(northing->GetValue().char_str()),
		  atof(altitude->GetValue().char_str()));
	a = atof(angle->GetValue().char_str());
	t = atof(tilt_angle->GetValue().char_str());
	s = atof(scale->GetValue().char_str());
	wxString str = time->GetValue();
	if (str[0u] == '*') str[0u] = '-';
	T = atof(str.char_str());
	return PresentationMark(v, a, t, s, T);
    }

private:
    DECLARE_EVENT_TABLE()
};

// Write a value without trailing zeros after the decimal point.
static void write_double(double d, FILE * fh) {
    char buf[64];
    sprintf(buf, "%.21f", d);
    char * p = strchr(buf, ',');
    if (p) *p = '.';
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
    bool force_save_as;
    wxString filename;

    public:
	AvenPresList(MainFrm * mainfrm_, wxWindow * parent, GfxCore * gfx_)
	    : wxListCtrl(parent, listctrl_PRES, wxDefaultPosition, wxDefaultSize,
			 wxLC_REPORT|wxLC_VIRTUAL),
	      mainfrm(mainfrm_), gfx(gfx_), current_item(-1), modified(false),
	      force_save_as(true)
	    {
		InsertColumn(0, wmsg(/*Easting*/378));
		InsertColumn(1, wmsg(/*Northing*/379));
		InsertColumn(2, wmsg(/*Altitude*/335));
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
	    filename = wxString();
	    modified = false;
	    force_save_as = true;
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
		    //printf("event.GetIndex() = %ld %d\n", event.GetIndex(), event.GetKeyCode());
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
	    switch (event.GetKeyCode()) {
		case WXK_INSERT:
		    if (event.m_controlDown) {
			if (current_item != -1 &&
			    size_t(current_item) < entries.size()) {
			    AddMark(current_item, entries[current_item]);
			}
		    } else {
			AddMark(current_item);
		    }
		    break;
		case WXK_DELETE:
		    // Already handled in OnListKeyDown.
		    break;
		case WXK_UP: case WXK_DOWN:
		    event.Skip();
		    break;
		default:
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
	    if (item < 0 || item >= (long)entries.size()) return wxString();
	    const PresentationMark & p = entries[item];
	    double v;
	    switch (column) {
		case 0: v = p.GetX(); break;
		case 1: v = p.GetY(); break;
		case 2: v = p.GetZ(); break;
#if 0
		case 3: v = p.angle; break;
		case 4: v = p.tilt_angle; break;
		case 5: v = p.scale; break;
		case 6: v = p.time; break;
#endif
		default: return wxString();
	    }
	    return wxString::Format(wxT("%ld"), (long)v);
	}
	void Save(bool use_default_name) {
	    wxString fnm = filename;
	    if (!use_default_name || force_save_as) {
		AvenAllowOnTop ontop(mainfrm);
#ifdef __WXMOTIF__
		wxString ext(wxT("*.fly"));
#else
		wxString ext = wmsg(/*Aven presentations*/320);
		ext += wxT("|*.fly|");
		ext += wmsg(/*All files*/208);
		ext += wxT("|");
		ext += wxFileSelectorDefaultWildcardStr;
#endif
		wxFileDialog dlg(this, wmsg(/*Select an output filename*/319),
				 wxString(), fnm, ext,
				 wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
		if (dlg.ShowModal() != wxID_OK) return;
		fnm = dlg.GetPath();
	    }

	    // FIXME: This should really use fn_str() - currently we probably can't
	    // save to a Unicode path on wxmsw.
	    FILE * fh_pres = fopen(fnm.char_str(), "w");
	    if (!fh_pres) {
		wxGetApp().ReportError(wxString::Format(wmsg(/*Error writing to file `%s'*/110), fnm.c_str()));
		return;
	    }
	    vector<PresentationMark>::const_iterator i;
	    for (i = entries.begin(); i != entries.end(); ++i) {
		const PresentationMark &p = *i;
		write_double(p.GetX(), fh_pres);
		putc(' ', fh_pres);
		write_double(p.GetY(), fh_pres);
		putc(' ', fh_pres);
		write_double(p.GetZ(), fh_pres);
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
	    force_save_as = false;
	}
	void New(const wxString &fnm) {
	    DeleteAllItems();
	    wxFileName::SplitPath(fnm, NULL, NULL, &filename, NULL, wxPATH_NATIVE);
	    filename += wxT(".fly");
	    force_save_as = true;
	}
	bool Load(const wxString &fnm) {
	    // FIXME: This should really use fn_str() - currently we probably
	    // can't save to a Unicode path on wxmsw.
	    FILE * fh_pres = fopen(fnm.char_str(), "r");
	    if (!fh_pres) {
		wxString m;
		m.Printf(wmsg(/*Couldn't open file `%s'*/93), fnm.c_str());
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
			char *p = buf;
			while ((p = strchr(p, '.'))) *p++ = ',';
			c = sscanf(buf, "%lf %lf %lf %lf %lf %lf %lf", &x, &y, &z, &a, &t, &s, &T);
			if (c < 6) {
			    DeleteAllItems();
			    wxGetApp().ReportError(wxString::Format(wmsg(/*Error in format of presentation file `%s'*/323), fnm.c_str()));
			    return false;
			}
		    }
		    if (c == 6) T = 0;
		    AddMark(item, PresentationMark(Vector3(x, y, z), a, t, s, T));
		    ++item;
		}
	    }
	    fclose(fh_pres);
	    filename = fnm;
	    modified = false;
	    force_save_as = false;
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
    EVT_TEXT(textctrl_FIND, MainFrm::OnFind)
    EVT_TEXT_ENTER(textctrl_FIND, MainFrm::OnGotoFound)
    EVT_MENU(button_FIND, MainFrm::OnGotoFound)
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
    EVT_MENU(menu_VIEW_COLOUR_BY_DATE, MainFrm::OnColourByDate)
    EVT_MENU(menu_VIEW_COLOUR_BY_ERROR, MainFrm::OnColourByError)
    EVT_MENU(menu_VIEW_SHOW_SURFACE, MainFrm::OnShowSurface)
    EVT_MENU(menu_VIEW_GRID, MainFrm::OnViewGrid)
    EVT_MENU(menu_VIEW_BOUNDING_BOX, MainFrm::OnViewBoundingBox)
    EVT_MENU(menu_VIEW_PERSPECTIVE, MainFrm::OnViewPerspective)
    EVT_MENU(menu_VIEW_SMOOTH_SHADING, MainFrm::OnViewSmoothShading)
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
    EVT_UPDATE_UI(menu_VIEW_COLOUR_BY_DATE, MainFrm::OnColourByDateUpdate)
    EVT_UPDATE_UI(menu_VIEW_COLOUR_BY_ERROR, MainFrm::OnColourByErrorUpdate)
    EVT_UPDATE_UI(menu_VIEW_GRID, MainFrm::OnViewGridUpdate)
    EVT_UPDATE_UI(menu_VIEW_BOUNDING_BOX, MainFrm::OnViewBoundingBoxUpdate)
    EVT_UPDATE_UI(menu_VIEW_PERSPECTIVE, MainFrm::OnViewPerspectiveUpdate)
    EVT_UPDATE_UI(menu_VIEW_SMOOTH_SHADING, MainFrm::OnViewSmoothShadingUpdate)
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
	int n = pt1->get_flags() - pt2->get_flags();
	if (n) return n > 0;
	wxString l1 = pt1->GetText().AfterLast(separator);
	wxString l2 = pt2->GetText().AfterLast(separator);
	n = name_cmp(l1, l2, separator);
	if (n) return n < 0;
	// Prefer non-2-nodes...
	// FIXME; implement
	// if leaf names are the same, prefer shorter labels as we can
	// display more of them
	n = pt1->GetText().length() - pt2->GetText().length();
	if (n) return n < 0;
	// make sure that we don't ever compare different labels as equal
	return name_cmp(pt1->GetText(), pt2->GetText(), separator) < 0;
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
	wxGetApp().ReportError(wmsg(/*You may only view one 3d file at a time.*/336));
	return FALSE;
    }

    m_Parent->OpenFile(filenames[0]);
    return TRUE;
}
#endif

MainFrm::MainFrm(const wxString& title, const wxPoint& pos, const wxSize& size) :
    wxFrame(NULL, 101, title, pos, size, wxDEFAULT_FRAME_STYLE | wxNO_FULL_REPAINT_ON_RESIZE), // wxNO_FULL_REPAINT_ON_RESIZE is 0 in wx >= 2.6
    m_Gfx(NULL), m_NumEntrances(0), m_NumFixedPts(0), m_NumExportedPts(0),
    m_NumHighlighted(0), m_HasUndergroundLegs(false), m_HasSurfaceLegs(false),
    m_HasErrorInformation(false), m_IsExtendedElevation(false)
#ifdef PREFDLG
    , m_PrefsDlg(NULL)
#endif
{
    icon_path = wxString(wmsg_cfgpth());
    icon_path += wxCONFIG_PATH_SEPARATOR;
    icon_path += wxT("icons");
    icon_path += wxCONFIG_PATH_SEPARATOR;

#ifdef _WIN32
    // The peculiar name is so that the icon is the first in the file
    // (required by Microsoft Windows for this type of icon)
    SetIcon(wxIcon(wxT("aaaaaAven")));
#else
    SetIcon(wxIcon(icon_path + APP_IMAGE, wxBITMAP_TYPE_PNG));
#endif

    CreateMenuBar();
    CreateToolBar();
    CreateStatusBar(2, wxST_SIZEGRIP);
    CreateSidePanel();

    int widths[2] = { -1 /* variable width */, -1 };
    GetStatusBar()->SetStatusWidths(2, widths);

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
    filemenu->Append(menu_FILE_EXPORT, GetTabMsg(/*@Export as...*/382));
#ifndef __WXMAC__
    // On wxMac the "Quit" menu item will be moved elsewhere, so we suppress
    // this separator.
    filemenu->AppendSeparator();
#endif
    filemenu->Append(menu_FILE_QUIT, GetTabMsg(/*@Quit##Ctrl+Q*/221));

    m_history.UseMenu(filemenu);
    m_history.Load(*wxConfigBase::Get());

    wxMenu* rotmenu = new wxMenu;
    rotmenu->AppendCheckItem(menu_ROTATION_TOGGLE, GetTabMsg(/*@Auto-Rotate##Space*/231));
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
    viewmenu->AppendCheckItem(menu_VIEW_SHOW_NAMES, GetTabMsg(/*Station @Names##Ctrl+N*/270));
    viewmenu->AppendCheckItem(menu_VIEW_SHOW_TUBES, GetTabMsg(/*Passage @Tubes*/346));
    viewmenu->AppendCheckItem(menu_VIEW_SHOW_CROSSES, GetTabMsg(/*@Crosses##Ctrl+X*/271));
    viewmenu->AppendCheckItem(menu_VIEW_GRID, GetTabMsg(/*@Grid##Ctrl+G*/297));
    viewmenu->AppendCheckItem(menu_VIEW_BOUNDING_BOX, GetTabMsg(/*@Bounding Box##Ctrl+B*/318));
    viewmenu->AppendSeparator();
    viewmenu->AppendCheckItem(menu_VIEW_SHOW_LEGS, GetTabMsg(/*@Underground Survey Legs##Ctrl+L*/272));
    viewmenu->AppendCheckItem(menu_VIEW_SHOW_SURFACE, GetTabMsg(/*@Surface Survey Legs##Ctrl+F*/291));
    viewmenu->AppendSeparator();
    viewmenu->AppendCheckItem(menu_VIEW_SHOW_OVERLAPPING_NAMES, GetTabMsg(/*@Overlapping Names*/273));
    viewmenu->AppendCheckItem(menu_VIEW_COLOUR_BY_DEPTH, GetTabMsg(/*Colour by @Depth*/292));
    viewmenu->AppendCheckItem(menu_VIEW_COLOUR_BY_DATE, GetTabMsg(/*Colour by D@ate*/293));
    viewmenu->AppendCheckItem(menu_VIEW_COLOUR_BY_ERROR, GetTabMsg(/*Colour by E@rror*/289));
    viewmenu->AppendSeparator();
    viewmenu->AppendCheckItem(menu_VIEW_SHOW_ENTRANCES, GetTabMsg(/*Highlight @Entrances*/294));
    viewmenu->AppendCheckItem(menu_VIEW_SHOW_FIXED_PTS, GetTabMsg(/*Highlight @Fixed Points*/295));
    viewmenu->AppendCheckItem(menu_VIEW_SHOW_EXPORTED_PTS, GetTabMsg(/*Highlight E@xported Points*/296));
    viewmenu->AppendSeparator();
#else
    viewmenu-> Append(menu_VIEW_CANCEL_DIST_LINE, GetTabMsg(/*@Cancel Measuring Line##Escape*/281));
#endif
    viewmenu->AppendCheckItem(menu_VIEW_PERSPECTIVE, GetTabMsg(/*@Perspective*/237));
// FIXME: enable this    viewmenu->AppendCheckItem(menu_VIEW_SMOOTH_SHADING, GetTabMsg(/*@Smooth Shading*/?!?);
    viewmenu->AppendCheckItem(menu_VIEW_TEXTURED, GetTabMsg(/*Textured @Walls*/238));
    viewmenu->AppendCheckItem(menu_VIEW_FOG, GetTabMsg(/*Fade @Distant Objects*/239));
    viewmenu->AppendCheckItem(menu_VIEW_SMOOTH_LINES, GetTabMsg(/*@Smoothed Survey Legs*/298));
    viewmenu->AppendSeparator();
    viewmenu->AppendCheckItem(menu_VIEW_FULLSCREEN, GetTabMsg(/*@Full Screen Mode##F11*/356));
#ifdef PREFDLG
    viewmenu->AppendSeparator();
    viewmenu-> Append(menu_VIEW_PREFERENCES, GetTabMsg(/*@Preferences...*/347));
#endif

#ifndef PREFDLG
    wxMenu* ctlmenu = new wxMenu;
    ctlmenu->AppendCheckItem(menu_CTL_REVERSE, GetTabMsg(/*@Reverse Sense##Ctrl+R*/280));
    ctlmenu->AppendSeparator();
    ctlmenu->Append(menu_CTL_CANCEL_DIST_LINE, GetTabMsg(/*@Cancel Measuring Line##Escape*/281));
    ctlmenu->AppendSeparator();
    wxMenu* indmenu = new wxMenu;
    indmenu->AppendCheckItem(menu_IND_COMPASS, GetTabMsg(/*@Compass*/274));
    indmenu->AppendCheckItem(menu_IND_CLINO, GetTabMsg(/*C@linometer*/275));
    indmenu->AppendCheckItem(menu_IND_DEPTH_BAR, GetTabMsg(/*@Depth Bar*/276));
    indmenu->AppendCheckItem(menu_IND_SCALE_BAR, GetTabMsg(/*@Scale Bar*/277));
    ctlmenu->Append(menu_CTL_INDICATORS, GetTabMsg(/*@Indicators*/299), indmenu);
    ctlmenu->AppendCheckItem(menu_CTL_SIDE_PANEL, GetTabMsg(/*@Side Panel*/337));
    ctlmenu->AppendSeparator();
    ctlmenu->AppendCheckItem(menu_CTL_METRIC, GetTabMsg(/*@Metric*/342));
    ctlmenu->AppendCheckItem(menu_CTL_DEGREES, GetTabMsg(/*@Degrees*/343));
#endif

    wxMenu* helpmenu = new wxMenu;
    helpmenu->Append(menu_HELP_ABOUT, GetTabMsg(/*@About...*/290));

    wxMenuBar* menubar = new wxMenuBar();
    menubar->Append(filemenu, GetTabMsg(/*@File*/210));
    menubar->Append(rotmenu, GetTabMsg(/*@Rotation*/211));
    menubar->Append(orientmenu, GetTabMsg(/*@Orientation*/212));
    menubar->Append(viewmenu, GetTabMsg(/*@View*/213));
#ifndef PREFDLG
    menubar->Append(ctlmenu, GetTabMsg(/*@Controls*/214));
#endif
    menubar->Append(presmenu, GetTabMsg(/*@Presentation*/216));
#ifndef __WXMAC__
    // On wxMac the "About" menu item will be moved elsewhere, so we suppress
    // this menu since it will then be empty.
    menubar->Append(helpmenu, GetTabMsg(/*@Help*/215));
#endif
    SetMenuBar(menubar);
}

// ICON must be a literal string.
#define TOOLBAR_BITMAP(ICON) wxBitmap(icon_path + wxT(ICON".png"), wxBITMAP_TYPE_PNG)

void MainFrm::CreateToolBar()
{
    // Create the toolbar.

    wxToolBar* toolbar = wxFrame::CreateToolBar();

#ifndef __WXGTK20__
    toolbar->SetMargins(5, 5);
#endif

    // FIXME: TRANSLATE tooltips
    toolbar->AddTool(menu_FILE_OPEN, wxT("Open"), TOOLBAR_BITMAP("open"), wxT("Open a 3D file for viewing"));
    toolbar->AddTool(menu_PRES_OPEN, wxT("Open presentation"), TOOLBAR_BITMAP("open-pres"), wxT("Open a presentation"));
    toolbar->AddSeparator();
    toolbar->AddCheckTool(menu_ROTATION_TOGGLE, wxT("Toggle rotation"), TOOLBAR_BITMAP("rotation"), wxNullBitmap, wxT("Toggle rotation"));
    toolbar->AddTool(menu_ORIENT_PLAN, wxT("Plan"), TOOLBAR_BITMAP("plan"), wxT("Switch to plan view"));
    toolbar->AddTool(menu_ORIENT_ELEVATION, wxT("Elevation"), TOOLBAR_BITMAP("elevation"), wxT("Switch to elevation view"));
    toolbar->AddTool(menu_ORIENT_DEFAULTS, wxT("Default view"), TOOLBAR_BITMAP("defaults"), wxT("Restore default view"));
    toolbar->AddSeparator();
    toolbar->AddCheckTool(menu_VIEW_SHOW_NAMES, wxT("Names"), TOOLBAR_BITMAP("names"), wxNullBitmap, wxT("Show station names"));
    toolbar->AddCheckTool(menu_VIEW_SHOW_CROSSES, wxT("Crosses"), TOOLBAR_BITMAP("crosses"), wxNullBitmap, wxT("Show crosses on stations"));
    toolbar->AddCheckTool(menu_VIEW_SHOW_ENTRANCES, wxT("Entrances"), TOOLBAR_BITMAP("entrances"), wxNullBitmap, wxT("Highlight entrances"));
    toolbar->AddCheckTool(menu_VIEW_SHOW_FIXED_PTS, wxT("Fixed points"), TOOLBAR_BITMAP("fixed-pts"), wxNullBitmap, wxT("Highlight fixed points"));
    toolbar->AddCheckTool(menu_VIEW_SHOW_EXPORTED_PTS, wxT("Exported points"), TOOLBAR_BITMAP("exported-pts"), wxNullBitmap, wxT("Highlight exported stations"));
    toolbar->AddSeparator();
    toolbar->AddCheckTool(menu_VIEW_SHOW_LEGS, wxT("Underground legs"), TOOLBAR_BITMAP("ug-legs"), wxNullBitmap, wxT("Show underground surveys"));
    toolbar->AddCheckTool(menu_VIEW_SHOW_SURFACE, wxT("Surface legs"), TOOLBAR_BITMAP("surface-legs"), wxNullBitmap, wxT("Show surface surveys"));
    toolbar->AddCheckTool(menu_VIEW_SHOW_TUBES, wxT("Tubes"), TOOLBAR_BITMAP("tubes"), wxNullBitmap, wxT("Show passage tubes"));
    toolbar->AddSeparator();
    toolbar->AddCheckTool(menu_PRES_FREWIND, wxT("Fast Rewind"), TOOLBAR_BITMAP("pres-frew"), wxNullBitmap, wxT("Very Fast Rewind"));
    toolbar->AddCheckTool(menu_PRES_REWIND, wxT("Rewind"), TOOLBAR_BITMAP("pres-rew"), wxNullBitmap, wxT("Fast Rewind"));
    toolbar->AddCheckTool(menu_PRES_REVERSE, wxT("Backwards"), TOOLBAR_BITMAP("pres-go-back"), wxNullBitmap, wxT("Play Backwards"));
    toolbar->AddCheckTool(menu_PRES_PAUSE, wxT("Pause"), TOOLBAR_BITMAP("pres-pause"), wxNullBitmap, wxT("Pause"));
    toolbar->AddCheckTool(menu_PRES_PLAY, wxT("Go"), TOOLBAR_BITMAP("pres-go"), wxNullBitmap, wxT("Play"));
    toolbar->AddCheckTool(menu_PRES_FF, wxT("FF"), TOOLBAR_BITMAP("pres-ff"), wxNullBitmap, wxT("Fast Forward"));
    toolbar->AddCheckTool(menu_PRES_FFF, wxT("Very FF"), TOOLBAR_BITMAP("pres-fff"), wxNullBitmap, wxT("Very Fast Forward"));
    toolbar->AddTool(menu_PRES_STOP, wxT("Stop"), TOOLBAR_BITMAP("pres-stop"), wxT("Stop"));

    toolbar->AddSeparator();
    m_FindBox = new wxTextCtrl(toolbar, textctrl_FIND, wxString(), wxDefaultPosition,
			       wxDefaultSize, wxTE_PROCESS_ENTER);
    toolbar->AddControl(m_FindBox);
    toolbar->AddTool(button_FIND, TOOLBAR_BITMAP("find"),
		     wmsg(/*Find*/332)/*"Search for station name"*/);
    toolbar->AddTool(button_HIDE, TOOLBAR_BITMAP("hideresults"),
		     wmsg(/*Hide*/333)/*"Hide search results"*/);

    toolbar->Realize();
}

void MainFrm::CreateSidePanel()
{
    m_Splitter = new AvenSplitterWindow(this);

    m_Notebook = new wxNotebook(m_Splitter, 400, wxDefaultPosition,
				wxDefaultSize,
				wxBK_BOTTOM | wxBK_LEFT);
    m_Notebook->Show(false);

    wxPanel * panel = new wxPanel(m_Notebook);
    m_Tree = new AvenTreeCtrl(this, panel);

//    m_RegexpCheckBox = new wxCheckBox(find_panel, -1,
//				      msg(/*Regular expression*/334));

    wxBoxSizer *panel_sizer = new wxBoxSizer(wxVERTICAL);
    panel_sizer->Add(m_Tree, 1, wxALL | wxEXPAND, 2);
    panel->SetAutoLayout(true);
    panel->SetSizer(panel_sizer);
//    panel_sizer->Fit(panel);
//    panel_sizer->SetSizeHints(panel);

    m_Control = new GUIControl();
    m_Gfx = new GfxCore(this, m_Splitter, m_Control);
    m_Control->SetView(m_Gfx);

    // Presentation panel:
    wxPanel * prespanel = new wxPanel(m_Notebook);

    m_PresList = new AvenPresList(this, prespanel, m_Gfx);

    wxBoxSizer *pres_panel_sizer = new wxBoxSizer(wxVERTICAL);
    pres_panel_sizer->Add(m_PresList, 1, wxALL | wxEXPAND, 2);
    prespanel->SetAutoLayout(true);
    prespanel->SetSizer(pres_panel_sizer);

    // Overall tabbed structure:
    // FIXME: this assumes images are 15x15
    wxImageList* image_list = new wxImageList(15, 15);
    wxString path = wxString(wmsg_cfgpth());
    path += wxCONFIG_PATH_SEPARATOR;
    path += wxT("icons") ;
    path += wxCONFIG_PATH_SEPARATOR;
    image_list->Add(wxBitmap(path + wxT("survey-tree.png"), wxBITMAP_TYPE_PNG));
    image_list->Add(wxBitmap(path + wxT("pres-tree.png"), wxBITMAP_TYPE_PNG));
    m_Notebook->SetImageList(image_list);
    m_Notebook->AddPage(panel, wmsg(/*Surveys*/376), true, 0);
    m_Notebook->AddPage(prespanel, wmsg(/*Presentation*/377), false, 1);

    m_Splitter->Initialize(m_Gfx);
}

bool
MainFrm::ProcessSVXFile(const wxString & file)
{
    Show(true);
    m_Splitter->Show(false);
    CavernLogWindow * log = new CavernLogWindow(this);
    m_Splitter->ReplaceWindow(m_Gfx, log);

    int result = log->process(file);
    if (result == 0) {
	m_Splitter->ReplaceWindow(log, m_Gfx);
	m_Splitter->Show();
	log->Destroy();
    }
    return result >= 0;
}

bool MainFrm::LoadData(const wxString& file, wxString prefix)
{
    // Load survey data from file, centre the dataset around the origin,
    // and prepare the data for drawing.

#if 0
    wxStopWatch timer;
    timer.Start();
#endif

    // Load the processed survey data.
    img* survey = img_open_survey(file.char_str(), prefix.char_str());
    if (!survey) {
	wxString m = wxString::Format(wmsg(img_error()), file.c_str());
	wxGetApp().ReportError(m);
	return false;
    }

    m_IsExtendedElevation = survey->is_extended_elevation;

    m_Tree->DeleteAllItems();

    // Create a list of all the leg vertices, counting them and finding the
    // extent of the survey at the same time.

    m_NumFixedPts = 0;
    m_NumExportedPts = 0;
    m_NumEntrances = 0;
    m_HasUndergroundLegs = false;
    m_HasSurfaceLegs = false;
    m_HasErrorInformation = false;

    // FIXME: discard existing presentation? ask user about saving if we do!

    // Delete any existing list entries.
    m_Labels.clear();

    Double xmin = DBL_MAX;
    Double xmax = -DBL_MAX;
    Double ymin = DBL_MAX;
    Double ymax = -DBL_MAX;
    Double zmin = DBL_MAX;
    Double zmax = -DBL_MAX;

    m_DepthMin = DBL_MAX;
    Double depthmax = -DBL_MAX;

    m_DateMin = (time_t)-1;
    if (m_DateMin < 0) {
	// Hmm, signed time_t!
	// FIXME: find a cleaner way to do this...
	time_t x = time_t(1) << (sizeof(time_t) * 8 - 2);
	m_DateMin = x;
	while ((x>>=1) != 0) m_DateMin |= x;
    }
    time_t datemax = 0;
    complete_dateinfo = true;

    traverses.clear();
    surface_traverses.clear();
    tubes.clear();

    // Ultimately we probably want different types (subclasses perhaps?) for
    // underground and surface data, so we don't need to store LRUD for surface
    // stuff.
    traverse * current_traverse = NULL;
    traverse * current_surface_traverse = NULL;
    vector<XSect> * current_tube = NULL;

    int result;
    img_point prev_pt = {0,0,0};
    bool current_polyline_is_surface = false;
    bool pending_move = false;
    // When a traverse is split between surface and underground, we split it
    // into contiguous traverses of each, but we need to track these so we can
    // assign the error statistics to all of them.  So we keep counts of how
    // many surface_traverses and traverses we've generated for the current
    // traverse.
    size_t n_traverses = 0;
    size_t n_surface_traverses = 0;
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
		n_traverses = n_surface_traverses = 0;
		pending_move = true;
		prev_pt = pt;
		break;

	    case img_LINE: {
		// Update survey extents.
		if (pt.x < xmin) xmin = pt.x;
		if (pt.x > xmax) xmax = pt.x;
		if (pt.y < ymin) ymin = pt.y;
		if (pt.y > ymax) ymax = pt.y;
		if (pt.z < zmin) zmin = pt.z;
		if (pt.z > zmax) zmax = pt.z;

		time_t date;
		date = survey->date1 + (survey->date2 - survey->date1) / 2;
		if (date) {
		    if (date < m_DateMin) m_DateMin = date;
		    if (date > datemax) datemax = date;
		} else {
		    complete_dateinfo = false;
		}

		bool is_surface = (survey->flags & img_FLAG_SURFACE);
		if (!is_surface) {
		    if (pt.z < m_DepthMin) m_DepthMin = pt.z;
		    if (pt.z > depthmax) depthmax = pt.z;
		}
		if (pending_move || current_polyline_is_surface != is_surface) {
		    if (!current_polyline_is_surface && current_traverse) {
			//FixLRUD(*current_traverse);
		    }
		    current_polyline_is_surface = is_surface;
		    // Start new traverse (surface or underground).
		    if (is_surface) {
			m_HasSurfaceLegs = true;
			surface_traverses.push_back(traverse());
			current_surface_traverse = &surface_traverses.back();
			++n_surface_traverses;
		    } else {
			m_HasUndergroundLegs = true;
			traverses.push_back(traverse());
			current_traverse = &traverses.back();
			++n_traverses;
			// The previous point was at a surface->ug transition.
			if (prev_pt.z < m_DepthMin) m_DepthMin = prev_pt.z;
			if (prev_pt.z > depthmax) depthmax = prev_pt.z;
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
			if (prev_pt.z < zmin) zmin = prev_pt.z;
			if (prev_pt.z > zmax) zmax = prev_pt.z;
		    }

		    if (is_surface) {
			current_surface_traverse->push_back(PointInfo(prev_pt));
		    } else {
			current_traverse->push_back(PointInfo(prev_pt));
		    }
		}

		if (is_surface) {
		    current_surface_traverse->push_back(PointInfo(pt, date));
		} else {
		    current_traverse->push_back(PointInfo(pt, date));
		}

		prev_pt = pt;
		pending_move = false;
		break;
	    }

	    case img_LABEL: {
		int flags = survey->flags;
		if (flags & img_SFLAG_ENTRANCE) {
		    flags ^= (img_SFLAG_ENTRANCE | LFLAG_ENTRANCE);
		}
		LabelInfo* label = new LabelInfo(pt, wxString(survey->label, wxConvUTF8), flags);
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
		wxString label(survey->label, wxConvUTF8);
		while (i != m_Labels.end() && (*i)->GetText() != label) ++i;

		if (i == m_Labels.end()) {
		    // Unattached cross-section - ignore for now.
		    printf("unattached cross-section\n");
		    if (current_tube->size() == 1)
			tubes.resize(tubes.size() - 1);
		    current_tube = NULL;
		    break;
		}

		time_t date;
		date = survey->date1 + (survey->date2 - survey->date1) / 2;
		if (date) {
		    if (date < m_DateMin) m_DateMin = date;
		    if (date > datemax) datemax = date;
		}

		current_tube->push_back(XSect(**i, date, survey->l, survey->r, survey->u, survey->d));
		break;
	    }

	    case img_XSECT_END:
		// Finish off current_tube.
		// If there's only one cross-section in the tube, just
		// discard it for now.  FIXME: we should handle this
		// when we come to skinning the tubes.
		if (current_tube && current_tube->size() == 1)
		    tubes.resize(tubes.size() - 1);
		current_tube = NULL;
		break;

	    case img_ERROR_INFO: {
		if (survey->E == 0.0) {
		    // Currently cavern doesn't spot all articulating traverses
		    // so we assume that any traverse with no error isn't part
		    // of a loop.  FIXME: fix cavern!
		    break;
		}
		m_HasErrorInformation = true;
		list<traverse>::reverse_iterator t;
		t = surface_traverses.rbegin();
		while (n_surface_traverses) {
		    assert(t != surface_traverses.rend());
		    t->n_legs = survey->n_legs;
		    t->length = survey->length;
		    t->E = survey->E;
		    t->H = survey->H;
		    t->V = survey->V;
		    --n_surface_traverses;
		    ++t;
		}
		t = traverses.rbegin();
		while (n_traverses) {
		    assert(t != traverses.rend());
		    t->n_legs = survey->n_legs;
		    t->length = survey->length;
		    t->E = survey->E;
		    t->H = survey->H;
		    t->V = survey->V;
		    --n_traverses;
		    ++t;
		}
		break;
	    }

	    case img_BAD: {
		m_Labels.clear();

		// FIXME: Do we need to reset all these? - Olly
		m_NumFixedPts = 0;
		m_NumExportedPts = 0;
		m_NumEntrances = 0;
		m_HasUndergroundLegs = false;
		m_HasSurfaceLegs = false;

		img_close(survey);

		wxString m = wxString::Format(wmsg(img_error()), file.c_str());
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

    // Finish off current_tube.
    // If there's only one cross-section in the tube, just
    // discard it for now.  FIXME: we should handle this
    // when we come to skinning the tubes.
    if (current_tube && current_tube->size() == 1)
	tubes.resize(tubes.size() - 1);

    separator = survey->separator;
    m_Title = wxString(survey->title, wxConvUTF8);
    m_DateStamp = wxString(survey->datestamp, wxConvUTF8);
    img_close(survey);

    // Check we've actually loaded some legs or stations!
    if (!m_HasUndergroundLegs && !m_HasSurfaceLegs && m_Labels.empty()) {
	wxString m = wxString::Format(wmsg(/*No survey data in 3d file `%s'*/202), file.c_str());
	wxGetApp().ReportError(m);
	return false;
    }

    if (traverses.empty() && surface_traverses.empty()) {
	// No legs, so get survey extents from stations
	list<LabelInfo*>::const_iterator i;
	for (i = m_Labels.begin(); i != m_Labels.end(); ++i) {
	    if ((*i)->GetX() < xmin) xmin = (*i)->GetX();
	    if ((*i)->GetX() > xmax) xmax = (*i)->GetX();
	    if ((*i)->GetY() < ymin) ymin = (*i)->GetY();
	    if ((*i)->GetY() > ymax) ymax = (*i)->GetY();
	    if ((*i)->GetZ() < zmin) zmin = (*i)->GetZ();
	    if ((*i)->GetZ() > zmax) zmax = (*i)->GetZ();
	}
    }

    m_Ext.assign(xmax - xmin, ymax - ymin, zmax - zmin);

    if (datemax < m_DateMin) m_DateMin = 0;
    m_DateExt = datemax - m_DateMin;

    // Sort the labels.
    m_Labels.sort(LabelCmp(separator));

    // Fill the tree of stations and prefixes.
    FillTree();

    // Sort labels so that entrances are displayed in preference,
    // then fixed points, then exported points, then other points.
    //
    // Also sort by leaf name so that we'll tend to choose labels
    // from different surveys, rather than labels from surveys which
    // are earlier in the list.
    m_Labels.sort(LabelPlotCmp(separator));

    // Centre the dataset around the origin.
    CentreDataset(Vector3(xmin, ymin, zmin));

    if (depthmax < m_DepthMin) {
	m_DepthMin = 0;
	m_DepthExt = 0;
    } else {
	m_DepthExt = depthmax - m_DepthMin;
	m_DepthMin -= m_Offsets.GetZ();
    }

#if 0
    printf("time to load = %.3f\n", (double)timer.Time());
#endif

    // Update window title.
    SetTitle(m_Title + " - "APP_NAME);

    if (!m_FindBox->GetValue().empty()) {
	// Highlight any stations matching the current search.
	wxCommandEvent dummy;
	OnFind(dummy);
    }

    return true;
}

#if 0
// Run along a newly read in traverse and make up plausible LRUD where
// it is missing.
void
MainFrm::FixLRUD(traverse & centreline)
{
    assert(centreline.size() > 1);

    Double last_size = 0;
    vector<PointInfo>::iterator i = centreline.begin();
    while (i != centreline.end()) {
	// Get the coordinates of this vertex.
	Point & pt_v = *i++;
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
}
#endif

void MainFrm::FillTree()
{
    // Create the root of the tree.
    wxTreeItemId treeroot = m_Tree->AddRoot(wxFileNameFromPath(m_File));

    // Fill the tree of stations and prefixes.
    stack<wxTreeItemId> previous_ids;
    wxString current_prefix;
    wxTreeItemId current_id = treeroot;

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
	else if (prefix.length() > current_prefix.length() &&
		 prefix.StartsWith(current_prefix) &&
		 (prefix[current_prefix.length()] == separator ||
		  current_prefix.empty())) {
	    // We have, so start as many new branches as required.
	    int current_prefix_length = current_prefix.length();
	    current_prefix = prefix;
	    size_t next_dot = current_prefix_length;
	    if (!next_dot) --next_dot;
	    do {
		size_t prev_dot = next_dot + 1;

		// Extract the next bit of prefix.
		next_dot = prefix.find(separator, prev_dot + 1);

		wxString bit = prefix.substr(prev_dot, next_dot - prev_dot);
		assert(!bit.empty());

		// Add the current tree ID to the stack.
		previous_ids.push(current_id);

		// Append the new item to the tree and set this as the current branch.
		current_id = m_Tree->AppendItem(current_id, bit);
		m_Tree->SetItemData(current_id, new TreeData(prefix.substr(0, next_dot)));
	    } while (next_dot != wxString::npos);
	}
	// Otherwise, we must have moved up, and possibly then down again.
	else {
	    size_t count = 0;
	    bool ascent_only = (prefix.length() < current_prefix.length() &&
				current_prefix.StartsWith(prefix) &&
				(current_prefix[prefix.length()] == separator ||
				 prefix.empty()));
	    if (!ascent_only) {
		// Find out how much of the current prefix and the new prefix
		// are the same.
		// Note that we require a match of a whole number of parts
		// between dots!
		for (size_t i = 0; prefix[i] == current_prefix[i]; ++i) {
		    if (prefix[i] == separator) count = i + 1;
		}
	    } else {
		count = prefix.length() + 1;
	    }

	    // Extract the part of the current prefix after the bit (if any)
	    // which has matched.
	    // This gives the prefixes to ascend over.
	    wxString prefixes_ascended = current_prefix.substr(count);

	    // Count the number of prefixes to ascend over.
	    int num_prefixes = prefixes_ascended.Freq(separator);

	    // Reverse up over these prefixes.
	    for (int i = 1; i <= num_prefixes; i++) {
		previous_ids.pop();
	    }
	    current_id = previous_ids.top();
	    previous_ids.pop();

	    if (!ascent_only) {
		// Add branches for this new part.
		size_t next_dot = count - 1;
		do {
		    size_t prev_dot = next_dot + 1;

		    // Extract the next bit of prefix.
		    next_dot = prefix.find(separator, prev_dot + 1);

		    wxString bit = prefix.substr(prev_dot, next_dot - prev_dot);
		    assert(!bit.empty());

		    // Add the current tree ID to the stack.
		    previous_ids.push(current_id);

		    // Append the new item to the tree and set this as the current branch.
		    current_id = m_Tree->AppendItem(current_id, bit);
		    m_Tree->SetItemData(current_id, new TreeData(prefix.substr(0, next_dot)));
		} while (next_dot != wxString::npos);
	    }

	    current_prefix = prefix;
	}

	// Now add the leaf.
	wxString bit = label->GetText().AfterLast(separator);
	assert(!bit.empty());
	wxTreeItemId id = m_Tree->AppendItem(current_id, bit);
	m_Tree->SetItemData(id, new TreeData(label));
	label->tree_id = id;
	// Set the colour for an item in the survey tree.
	if (label->IsEntrance()) {
	    // Entrances are green (like entrance blobs).
	    m_Tree->SetItemTextColour(id, wxColour(0, 255, 0));
	} else if (label->IsSurface()) {
	    // Surface stations are dark green.
	    m_Tree->SetItemTextColour(id, wxColour(49, 158, 79));
	}
    }

    m_Tree->Expand(treeroot);
    m_Tree->SetEnabled();
}

void MainFrm::SelectTreeItem(LabelInfo* label)
{
    m_Tree->SelectItem(label->tree_id);
}

void MainFrm::CentreDataset(const Vector3 & vmin)
{
    // Centre the dataset around the origin.

    m_Offsets = vmin + (m_Ext * 0.5);

    list<traverse>::iterator t = traverses.begin();
    while (t != traverses.end()) {
	assert(t->size() > 1);
	vector<PointInfo>::iterator pos = t->begin();
	while (pos != t->end()) {
	    Point & point = *pos++;
	    point -= m_Offsets;
	}
	++t;
    }

    t = surface_traverses.begin();
    while (t != surface_traverses.end()) {
	assert(t->size() > 1);
	vector<PointInfo>::iterator pos = t->begin();
	while (pos != t->end()) {
	    Point & point = *pos++;
	    point -= m_Offsets;
	}
	++t;
    }

    list<vector<XSect> >::iterator i = tubes.begin();
    while (i != tubes.end()) {
	assert(i->size() > 1);
	vector<XSect>::iterator pos = i->begin();
	while (pos != i->end()) {
	    Point & point = *pos++;
	    point -= m_Offsets;
	}
	++i;
    }

    list<LabelInfo*>::iterator lpos = m_Labels.begin();
    while (lpos != m_Labels.end()) {
	Point & point = **lpos++;
	point -= m_Offsets;
    }
}

void MainFrm::OnMRUFile(wxCommandEvent& event)
{
    wxString f(m_history.GetHistoryFile(event.GetId() - wxID_FILE1));
    if (!f.empty()) OpenFile(f);
}

void MainFrm::AddToFileHistory(const wxString & file)
{
    if (wxIsAbsolutePath(file)) {
	m_history.AddFileToHistory(file);
    } else {
	wxString abs = wxGetCwd();
	abs += wxCONFIG_PATH_SEPARATOR;
	abs += file;
	m_history.AddFileToHistory(abs);
    }
    wxConfigBase *b = wxConfigBase::Get();
    m_history.Save(*b);
    b->Flush();
}

void MainFrm::OpenFile(const wxString& file, wxString survey)
{
    wxBusyCursor hourglass;

    // Check if this is an unprocessed survey data file.
    if (file.length() > 4 && file[file.length() - 4] == '.') {
	wxString ext(file, file.length() - 3, 3);
	ext.MakeLower();
	if (ext == wxT("svx") || ext == wxT("dat") || ext == wxT("mak")) {
	    if (!ProcessSVXFile(file)) {
		// ProcessSVXFile() reports the error to the user.
		return;
	    }
	    AddToFileHistory(file);
	    wxString file3d(file, 0, file.length() - 3);
	    file3d.append(wxT("3d"));
	    if (!LoadData(file3d, survey))
		return;
	    InitialiseAfterLoad(file);
	    return;
	}
    }

    if (!LoadData(file, survey))
	return;
    AddToFileHistory(file);
    InitialiseAfterLoad(file);
}

void MainFrm::InitialiseAfterLoad(const wxString & file)
{
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

    bool same_file = (file == m_File);
    if (!same_file)
	m_File = file;

    m_Gfx->Initialise(same_file);
    m_Notebook->Show(true);

    m_Gfx->Show(true);
    m_Gfx->SetFocus();
}

//
//  UI event handlers
//

// For Unix we want "*.svx;*.SVX" while for Windows we only want "*.svx".
#ifdef _WIN32
# define CASE(X)
#else
# define CASE(X) ";"X
#endif

void MainFrm::OnOpen(wxCommandEvent&)
{
    AvenAllowOnTop ontop(this);
#ifdef __WXMOTIF__
    wxString filetypes = wxT("*.3d");
#else
    wxString filetypes;
    filetypes.Printf(wxT("%s|*.3d;*.svx;*.plt;*.plf;*.dat;*.mak;*.xyz"
		     CASE("*.3D;*.SVX;*.PLT;*.PLF;*.DAT;*.MAK;*.XYZ")
		     "|%s|*.3d"CASE("*.3D")
		     "|%s|*.svx"CASE("*.SVX")
		     "|%s|*.plt;*.plf"CASE("*.PLT;*.PLF")
		     "|%s|*.dat;*.mak"CASE("*.DAT;*.MAK")
		     "|%s|*.xyz"CASE("*.XYZ")
		     "|%s|%s"),
		     wxT("All survey files"),
		     wmsg(/*Survex 3d files*/207).c_str(),
		     wxT("Survex svx files"),
		     wmsg(/*Compass PLT files*/324).c_str(),
		     wxT("Compass DAT and MAK files"),
		     wmsg(/*CMAP XYZ files*/325).c_str(),
		     wmsg(/*All files*/208).c_str(),
		     wxFileSelectorDefaultWildcardStr);
#endif
    // FIXME: drop "3d" from this message?
    wxFileDialog dlg(this, wmsg(/*Select a 3d file to view*/206),
		     wxString(), wxString(),
		     filetypes, wxFD_OPEN|wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() == wxID_OK) {
	OpenFile(dlg.GetPath());
    }
}

void MainFrm::OnScreenshot(wxCommandEvent&)
{
    AvenAllowOnTop ontop(this);
    wxString baseleaf;
    wxFileName::SplitPath(m_File, NULL, NULL, &baseleaf, NULL, wxPATH_NATIVE);
    wxFileDialog dlg(this, wmsg(/*Save Screenshot*/321), wxString(),
		     baseleaf + wxT(".png"),
		     wxT("*.png"), wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() == wxID_OK) {
	static bool png_handled = false;
	if (!png_handled) {
#if 0 // FIXME : enable this to allow other export formats...
	    ::wxInitAllImageHandlers();
#else
	    wxImage::AddHandler(new wxPNGHandler);
#endif
	    png_handled = true;
	}
	if (!m_Gfx->SaveScreenshot(dlg.GetPath(), wxBITMAP_TYPE_PNG)) {
	    wxGetApp().ReportError(wxString::Format(wmsg(/*Error writing to file `%s'*/110), dlg.GetPath().c_str()));
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
    wxPageSetupDialog dlg(this, wxGetApp().GetPageSetupDialogData());
    if (dlg.ShowModal() == wxID_OK) {
	wxGetApp().SetPageSetupDialogData(dlg.GetPageSetupData());
    }
}

void MainFrm::OnExport(wxCommandEvent&)
{
    m_Gfx->OnExport(m_File, m_Title);
}

void MainFrm::OnQuit(wxCommandEvent&)
{
    if (m_PresList->Modified()) {
	AvenAllowOnTop ontop(this);
	// FIXME: better to ask "Do you want to save your changes?" and offer [Save] [Discard] [Cancel]
	if (wxMessageBox(wmsg(/*The current presentation has been modified.  Abandon unsaved changes?*/327),
			 wmsg(/*Modified Presentation*/326),
			 wxOK|wxCANCEL|wxICON_QUESTION) == wxCANCEL) {
	    return;
	}
    }
    wxConfigBase *b = wxConfigBase::Get();
    if (IsFullScreen()) {
	b->Write(wxT("width"), -2);
	b->DeleteEntry(wxT("height"));
    } else if (IsMaximized()) {
	b->Write(wxT("width"), -1);
	b->DeleteEntry(wxT("height"));
    } else {
	int width, height;
	GetSize(&width, &height);
	b->Write(wxT("width"), width);
	b->Write(wxT("height"), height);
    }
    b->Flush();
    exit(0);
}

void MainFrm::OnClose(wxCloseEvent&)
{
    wxCommandEvent dummy;
    OnQuit(dummy);
}

void MainFrm::OnAbout(wxCommandEvent&)
{
    AvenAllowOnTop ontop(this);
    AboutDlg dlg(this, icon_path);
    dlg.Centre();
    dlg.ShowModal();
}

void MainFrm::UpdateStatusBar()
{
    if (!here_text.empty()) {
	GetStatusBar()->SetStatusText(here_text);
	GetStatusBar()->SetStatusText(dist_text, 1);
    } else if (!coords_text.empty()) {
	GetStatusBar()->SetStatusText(coords_text);
	GetStatusBar()->SetStatusText(distfree_text, 1);
    } else {
	GetStatusBar()->SetStatusText(wxString());
	GetStatusBar()->SetStatusText(wxString(), 1);
    }
}

void MainFrm::ClearTreeSelection()
{
    m_Tree->UnselectAll();
    if (!dist_text.empty()) {
	dist_text = wxString();
	UpdateStatusBar();
    }
    m_Gfx->SetThere();
}

void MainFrm::ClearCoords()
{
    if (!coords_text.empty()) {
	coords_text = wxString();
	UpdateStatusBar();
    }
}

void MainFrm::SetCoords(const Vector3 &v)
{
    wxString & s = coords_text;
    if (m_Gfx->GetMetric()) {
	s.Printf(wmsg(/*%.2f E, %.2f N*/338), v.GetX(), v.GetY());
	s += wxString::Format(wxT(", %s %.2fm"), wmsg(/*Altitude*/335).c_str(), v.GetZ());
    } else {
	s.Printf(wmsg(/*%.2f E, %.2f N*/338),
		 v.GetX() / METRES_PER_FOOT, v.GetY() / METRES_PER_FOOT);
	s += wxString::Format(wxT(", %s %.2fft"), wmsg(/*Altitude*/335).c_str(),
			      v.GetZ() / METRES_PER_FOOT);
    }
    distfree_text = wxString();
    UpdateStatusBar();
}

const LabelInfo * MainFrm::GetTreeSelection() const {
    wxTreeItemData* sel_wx;
    if (!m_Tree->GetSelectionData(&sel_wx)) return NULL;

    const TreeData* data = static_cast<const TreeData*>(sel_wx);
    if (!data->IsStation()) return NULL;

    return data->GetLabel();
}

void MainFrm::SetCoords(Double x, Double y)
{
    wxString & s = coords_text;
    if (m_Gfx->GetMetric()) {
	s.Printf(wmsg(/*%.2f E, %.2f N*/338), x, y);
    } else {
	s.Printf(wmsg(/*%.2f E, %.2f N*/338),
		 x / METRES_PER_FOOT, y / METRES_PER_FOOT);
    }

    wxString & t = distfree_text;
    t = wxString();
    const LabelInfo* label;
    if (m_Gfx->ShowingMeasuringLine() && (label = GetTreeSelection())) {
	Vector3 delta(x - m_Offsets.GetX() - label->GetX(),
		      y - m_Offsets.GetY() - label->GetY(), 0);
	Double dh = sqrt(delta.GetX()*delta.GetX() + delta.GetY()*delta.GetY());
	Double brg = deg(atan2(delta.GetX(), delta.GetY()));
	if (brg < 0) brg += 360;

	wxString from_str;
	from_str.Printf(wmsg(/*From %s*/339), label->GetText().c_str());

	wxString brg_unit;
	if (m_Gfx->GetDegrees()) {
	    brg_unit = wmsg(/*&deg;*/344);
	} else {
	    brg *= 400.0 / 360.0;
	    brg_unit = wmsg(/*grad*/345);
	}

	if (m_Gfx->GetMetric()) {
	    t.Printf(wmsg(/*%s: H %.2f%s, Brg %03d%s*/374),
		     from_str.c_str(), dh, wxT("m"), int(brg), brg_unit.c_str());
	} else {
	    t.Printf(wmsg(/*%s: H %.2f%s, Brg %03d%s*/374),
		     from_str.c_str(), dh / METRES_PER_FOOT, wxT("ft"), int(brg),
		     brg_unit.c_str());
	}
    }

    UpdateStatusBar();
}

void MainFrm::SetAltitude(Double z)
{
    wxString & s = coords_text;
    if (m_Gfx->GetMetric()) {
	s.Printf(wxT("%s %.2fm"), wmsg(/*Altitude*/335).c_str(), double(z));
    } else {
	s.Printf(wxT("%s %.2fft"), wmsg(/*Altitude*/335).c_str(), double(z / METRES_PER_FOOT));
    }

    wxString & t = distfree_text;
    t = wxString();
    const LabelInfo* label;
    if (m_Gfx->ShowingMeasuringLine() && (label = GetTreeSelection())) {
	Double dz = z - m_Offsets.GetZ() - label->GetZ();

	wxString from_str;
	from_str.Printf(wmsg(/*From %s*/339), label->GetText().c_str());

	if (m_Gfx->GetMetric()) {
	    t.Printf(wmsg(/*%s: V %.2f%s*/375),
		     from_str.c_str(), dz, wxT("m"));
	} else {
	    t.Printf(wmsg(/*%s: V %.2f%s*/375),
		     from_str.c_str(), dz / METRES_PER_FOOT, wxT("ft"));
	}
    }

    UpdateStatusBar();
}

void MainFrm::ShowInfo(const LabelInfo *here)
{
    assert(m_Gfx);

    if (!here) {
	m_Gfx->SetHere();
	m_Tree->SetHere(wxTreeItemId());
	// Don't clear "There" mark here.
	if (here_text.empty() && dist_text.empty()) return;
	here_text = wxString();
	dist_text = wxString();
	UpdateStatusBar();
	return;
    }

    Vector3 v = *here + m_Offsets;
    wxString & s = here_text;
    if (m_Gfx->GetMetric()) {
	s.Printf(wmsg(/*%.2f E, %.2f N*/338), v.GetX(), v.GetY());
	s += wxString::Format(wxT(", %s %.2fm"), wmsg(/*Altitude*/335).c_str(), v.GetZ());
    } else {
	s.Printf(wmsg(/*%.2f E, %.2f N*/338),
		 v.GetX() / METRES_PER_FOOT, v.GetY() / METRES_PER_FOOT);
	s += wxString::Format(wxT(", %s %.2fft"), wmsg(/*Altitude*/335).c_str(),
			      v.GetZ() / METRES_PER_FOOT);
    }
    s += wxT(": ");
    s += here->GetText();
    m_Gfx->SetHere(*here);
    m_Tree->SetHere(here->tree_id);

    const LabelInfo* label;
    if (m_Gfx->ShowingMeasuringLine() && (label = GetTreeSelection())) {
	Vector3 delta = *here - *label;

	Double d_horiz = sqrt(delta.GetX()*delta.GetX() + delta.GetY()*delta.GetY());
	Double dr = delta.magnitude();

	Double brg = deg(atan2(delta.GetX(), delta.GetY()));
	if (brg < 0) brg += 360;

	wxString from_str;
	from_str.Printf(wmsg(/*From %s*/339), label->GetText().c_str());

	wxString hv_str;
	if (m_Gfx->GetMetric()) {
	    hv_str.Printf(wmsg(/*H %.2f%s, V %.2f%s*/340),
			  d_horiz, wxT("m"), delta.GetZ(), wxT("m"));
	} else {
	    hv_str.Printf(wmsg(/*H %.2f%s, V %.2f%s*/340),
			  d_horiz / METRES_PER_FOOT, wxT("ft"),
			  delta.GetZ() / METRES_PER_FOOT, wxT("ft"));
	}
	wxString brg_unit;
	if (m_Gfx->GetDegrees()) {
	    brg_unit = wmsg(/*&deg;*/344);
	} else {
	    brg *= 400.0 / 360.0;
	    brg_unit = wmsg(/*grad*/345);
	}
	wxString & d = dist_text;
	if (m_Gfx->GetMetric()) {
	    d.Printf(wmsg(/*%s: %s, Dist %.2f%s, Brg %03d%s*/341),
		     from_str.c_str(), hv_str.c_str(),
		     dr, wxT("m"), int(brg), brg_unit.c_str());
	} else {
	    d.Printf(wmsg(/*%s: %s, Dist %.2f%s, Brg %03d%s*/341),
		     from_str.c_str(), hv_str.c_str(),
		     dr / METRES_PER_FOOT, wxT("ft"), int(brg),
		     brg_unit.c_str());
	}
	m_Gfx->SetThere(*label);
    } else {
	dist_text = wxString();
	m_Gfx->SetThere();
    }
    UpdateStatusBar();
}

void MainFrm::DisplayTreeInfo(const wxTreeItemData* item)
{
    const TreeData* data = static_cast<const TreeData*>(item);
    if (data && data->IsStation()) {
	const LabelInfo * label = data->GetLabel();
	ShowInfo(label);
	m_Gfx->SetHere(*label);
    } else {
	ShowInfo(NULL);
    }
}

void MainFrm::TreeItemSelected(const wxTreeItemData* item, bool zoom)
{
    const TreeData* data = static_cast<const TreeData*>(item);
    if (data && data->IsStation()) {
	const LabelInfo* label = data->GetLabel();
	if (zoom) m_Gfx->CentreOn(*label);
	m_Gfx->SetThere(*label);
	dist_text = wxString();
	// FIXME: Need to update dist_text (From ... etc)
	// But we don't currently know where "here" is at this point in the
	// code!
    } else {
	dist_text = wxString();
	m_Gfx->SetThere();
    }
    if (!data) {
	// Must be the root.
	m_FindBox->SetValue(wxString());
	if (zoom) {
	    wxCommandEvent dummy;
	    OnDefaults(dummy);
	}
    } else if (data && !data->IsStation()) {
	m_FindBox->SetValue(data->GetSurvey() + wxT(".*"));
	if (zoom) {
	    wxCommandEvent dummy;
	    OnGotoFound(dummy);
	}
    }
    UpdateStatusBar();
}

void MainFrm::OnPresNew(wxCommandEvent&)
{
    if (m_PresList->Modified()) {
	AvenAllowOnTop ontop(this);
	// FIXME: better to ask "Do you want to save your changes?" and offer [Save] [Discard] [Cancel]
	if (wxMessageBox(wmsg(/*The current presentation has been modified.  Abandon unsaved changes?*/327),
			 wmsg(/*Modified Presentation*/326),
			 wxOK|wxCANCEL|wxICON_QUESTION) == wxCANCEL) {
	    return;
	}
    }
    m_PresList->New(m_File);
    if (!ShowingSidePanel()) ToggleSidePanel();
    // Select the presentation page in the notebook.
    m_Notebook->SetSelection(1);
}

void MainFrm::OnPresOpen(wxCommandEvent&)
{
    AvenAllowOnTop ontop(this);
    if (m_PresList->Modified()) {
	// FIXME: better to ask "Do you want to save your changes?" and offer [Save] [Discard] [Cancel]
	if (wxMessageBox(wmsg(/*The current presentation has been modified.  Abandon unsaved changes?*/327),
			 wmsg(/*Modified Presentation*/326),
			 wxOK|wxCANCEL|wxICON_QUESTION) == wxCANCEL) {
	    return;
	}
    }
#ifdef __WXMOTIF__
    wxFileDialog dlg(this, wmsg(/*Select a presentation to open*/322), wxString(), wxString(),
		     wxT("*.fly"), wxFD_OPEN);
#else
    wxFileDialog dlg(this, wmsg(/*Select a presentation to open*/322), wxString(), wxString(),
		     wxString::Format(wxT("%s|*.fly|%s|%s"),
		       	       wmsg(/*Aven presentations*/320).c_str(),
		       	       wmsg(/*All files*/208).c_str(),
		       	       wxFileSelectorDefaultWildcardStr),
		     wxFD_OPEN|wxFD_FILE_MUST_EXIST);
#endif
    if (dlg.ShowModal() == wxID_OK) {
	if (!m_PresList->Load(dlg.GetPath())) {
	    return;
	}
	// FIXME : keep a history of loaded/saved presentations, like we do for
	// loaded surveys...
	// Select the presentation page in the notebook.
	m_Notebook->SetSelection(1);
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
    wxString baseleaf;
    wxFileName::SplitPath(m_File, NULL, NULL, &baseleaf, NULL, wxPATH_NATIVE);
    wxFileDialog dlg(this, wxT("Export Movie"), wxString(),
		     baseleaf + wxT(".mpg"),
		     wxT("MPEG|*.mpg|AVI|*.avi|QuickTime|*.mov|WMV|*.wmv;*.asf"),
		     wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() == wxID_OK) {
	if (!m_Gfx->ExportMovie(dlg.GetPath())) {
	    wxGetApp().ReportError(wxString::Format(wmsg(/*Error writing to file `%s'*/110), dlg.GetPath().c_str()));
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
    // Find stations specified by a string or regular expression pattern.

    wxString pattern = m_FindBox->GetValue();
    if (pattern.empty()) {
	// Hide any search result highlights.
	list<LabelInfo*>::iterator pos = m_Labels.begin();
	while (pos != m_Labels.end()) {
	    LabelInfo* label = *pos++;
	    label->clear_flags(LFLAG_HIGHLIGHTED);
	}
	m_NumHighlighted = 0;
    } else {
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
	       wxChar ch = pattern[i];
	       // ^ only special at start; $ at end.  But this is simpler...
	       switch (ch) {
		case '^': case '$': case '.': case '[': case '\\':
		  pat += wxT('\\');
		  pat += ch;
		  break;
		case '*':
		  pat += wxT(".*");
		  substring = false;
		  break;
		case '?':
		  pat += wxT('.');
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
	       wxChar ch = pattern[i];
	       // ^ only special at start; $ at end.  But this is simpler...
	       switch (ch) {
		case '^': case '$': case '*': case '.': case '[': case '\\':
		  pat += wxT('\\');
	       }
	       pat += ch;
	    }
	    pattern = pat;
	    re_flags |= wxRE_BASIC;
	}

	if (!substring) {
	    // FIXME "0u" required to avoid compilation error with g++-3.0
	    if (pattern.empty() || pattern[0u] != '^') pattern = wxT('^') + pattern;
	    // FIXME: this fails to cope with "\$" at the end of pattern...
	    if (pattern[pattern.size() - 1] != '$') pattern += wxT('$');
	}

	wxRegEx regex;
	if (!regex.Compile(pattern, re_flags)) {
	    wxBell();
	    return;
	}

	int found = 0;

	list<LabelInfo*>::iterator pos = m_Labels.begin();
	while (pos != m_Labels.end()) {
	    LabelInfo* label = *pos++;

	    if (regex.Matches(label->GetText())) {
		label->set_flags(LFLAG_HIGHLIGHTED);
		++found;
	    } else {
		label->clear_flags(LFLAG_HIGHLIGHTED);
	    }
	}

	m_NumHighlighted = found;

	// Re-sort so highlighted points get names in preference
	if (found) m_Labels.sort(LabelPlotCmp(separator));
    }

    m_Gfx->UpdateBlobs();
    m_Gfx->ForceRefresh();

    if (!m_NumHighlighted) {
        GetToolBar()->SetToolShortHelp(button_HIDE, wmsg(/*No matches were found.*/328));
    } else {
        GetToolBar()->SetToolShortHelp(button_HIDE, wxString::Format(wxT("Unhilight %d found stations"), m_NumHighlighted));
    }
}

void MainFrm::OnGotoFound(wxCommandEvent&)
{
    if (!m_NumHighlighted) {
	wxGetApp().ReportError(wmsg(/*No matches were found.*/328));
	return;
    }

    Double xmin = DBL_MAX;
    Double xmax = -DBL_MAX;
    Double ymin = DBL_MAX;
    Double ymax = -DBL_MAX;
    Double zmin = DBL_MAX;
    Double zmax = -DBL_MAX;

    list<LabelInfo*>::iterator pos = m_Labels.begin();
    while (pos != m_Labels.end()) {
	LabelInfo* label = *pos++;

	if (label->get_flags() & LFLAG_HIGHLIGHTED) {
	    if (label->GetX() < xmin) xmin = label->GetX();
	    if (label->GetX() > xmax) xmax = label->GetX();
	    if (label->GetY() < ymin) ymin = label->GetY();
	    if (label->GetY() > ymax) ymax = label->GetY();
	    if (label->GetZ() < zmin) zmin = label->GetZ();
	    if (label->GetZ() > zmax) zmax = label->GetZ();
	}
    }

    m_Gfx->SetViewTo(xmin, xmax, ymin, ymax, zmin, zmax);
    m_Gfx->SetFocus();
}

void MainFrm::OnHide(wxCommandEvent&)
{
    m_FindBox->SetValue(wxString());
}

void MainFrm::OnHideUpdate(wxUpdateUIEvent& ui)
{
    ui.Enable(m_NumHighlighted != 0);
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
#ifdef __WXGTK__
    // wxGTK doesn't currently remove the toolbar, statusbar, or menubar.
    // Can't work out how to lose the menubar right now, but this works for
    // the other two.  FIXME: tidy this code up and submit a patch for
    // wxWidgets.
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
