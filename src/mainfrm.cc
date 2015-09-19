//
//  mainfrm.cc
//
//  Main frame handling for Aven.
//
//  Copyright (C) 2000-2002,2005,2006 Mark R. Shinwell
//  Copyright (C) 2001-2003,2004,2005,2006,2010,2011,2012,2013,2014,2015 Olly Betts
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
#include "img_hosted.h"
#include "namecompare.h"
#include "printing.h"
#include "filename.h"
#include "useful.h"

#include <wx/confbase.h>
//#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/imaglist.h>
#include <wx/process.h>
#include <wx/regex.h>
#ifdef USING_GENERIC_TOOLBAR
# include <wx/sysopt.h>
#endif

#include <cstdlib>
#include <float.h>
#include <functional>
#include <map>
#include <stack>
#include <vector>

#ifdef __WXMSW__
#define TOOL(x) wxBitmap(wxT(#x), wxBITMAP_TYPE_PNG_RESOURCE)
#else
// XPM files declare the array as static, but we also want it to be const too.
// This avoids a compiler warning, and also means the data can go in a
// read-only page and be shared between processes.
#define static static const
#include "../lib/icons/aven.xpm"
#include "../lib/icons/log.xpm"
#include "../lib/icons/open.xpm"
#include "../lib/icons/open_pres.xpm"
#include "../lib/icons/rotation.xpm"
#include "../lib/icons/plan.xpm"
#include "../lib/icons/elevation.xpm"
#include "../lib/icons/defaults.xpm"
#include "../lib/icons/names.xpm"
#include "../lib/icons/crosses.xpm"
#include "../lib/icons/entrances.xpm"
#include "../lib/icons/fixed_pts.xpm"
#include "../lib/icons/exported_pts.xpm"
#include "../lib/icons/ug_legs.xpm"
#include "../lib/icons/surface_legs.xpm"
#include "../lib/icons/tubes.xpm"
#include "../lib/icons/solid_surface.xpm"
#include "../lib/icons/pres_frew.xpm"
#include "../lib/icons/pres_rew.xpm"
#include "../lib/icons/pres_go_back.xpm"
#include "../lib/icons/pres_pause.xpm"
#include "../lib/icons/pres_go.xpm"
#include "../lib/icons/pres_ff.xpm"
#include "../lib/icons/pres_fff.xpm"
#include "../lib/icons/pres_stop.xpm"
#include "../lib/icons/find.xpm"
#include "../lib/icons/hideresults.xpm"
#include "../lib/icons/survey_tree.xpm"
#include "../lib/icons/pres_tree.xpm"
#undef static
#define TOOL(x) wxBITMAP(x)
#endif

using namespace std;

const static int img2aven_tab[] = {
#include "img2aven.h"
};

inline int
img2aven(int flags)
{
    flags &= (sizeof(img2aven_tab) / sizeof(img2aven_tab[0]) - 1);
    return img2aven_tab[flags];
}

class AvenSplitterWindow : public wxSplitterWindow {
    MainFrm *parent;

    public:
	explicit AvenSplitterWindow(MainFrm *parent_)
	    : wxSplitterWindow(parent_, -1, wxDefaultPosition, wxDefaultSize,
			       wxSP_3DSASH),
	      parent(parent_)
	{
	}

	void OnSplitterDClick(wxSplitterEvent &) {
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
    // TRANSLATORS: Title of dialog to edit a waypoint in a presentation.
    EditMarkDlg(wxWindow* parent, const PresentationMark & p)
	: wxDialog(parent, 500, wmsg(/*Edit Waypoint*/404))
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
	r2->Add(new wxStaticText(this, 614, wmsg(/*Bearing*/259) + wxT(": ")), 0, wxALIGN_CENTRE_VERTICAL);
	r2->Add(angle);
	vert->Add(r2, 0, wxALL, 8);
	wxBoxSizer * r3 = new wxBoxSizer(wxHORIZONTAL);
	r3->Add(new wxStaticText(this, 615, wmsg(/*Elevation*/118) + wxT(": ")), 0, wxALIGN_CENTRE_VERTICAL);
	r3->Add(tilt_angle);
	vert->Add(r3, 0, wxALL, 8);
	wxBoxSizer * r4 = new wxBoxSizer(wxHORIZONTAL);
	r4->Add(new wxStaticText(this, 616, wmsg(/*Scale*/154) + wxT(": ")), 0, wxALIGN_CENTRE_VERTICAL);
	r4->Add(scale);
	/* TRANSLATORS: Note after "Scale" field in dialog to edit a waypoint
	 * in a presentation. */
	r4->Add(new wxStaticText(this, 617, wmsg(/* (unused in perspective view)*/278)),
		0, wxALIGN_CENTRE_VERTICAL);
	vert->Add(r4, 0, wxALL, 8);

	wxBoxSizer * r5 = new wxBoxSizer(wxHORIZONTAL);
	/* TRANSLATORS: Field label in dialog to edit a waypoint in a
	 * presentation. */
	r5->Add(new wxStaticText(this, 616, wmsg(/*Time: */279)), 0, wxALIGN_CENTRE_VERTICAL);
	r5->Add(time);
	/* TRANSLATORS: units+info after time field in dialog to edit a
	 * waypoint in a presentation. */
	r5->Add(new wxStaticText(this, 617, wmsg(/* secs (0 = auto; *6 = 6 times auto)*/282)),
		0, wxALIGN_CENTRE_VERTICAL);
	vert->Add(r5, 0, wxALL, 8);

	wxBoxSizer * buttons = new wxBoxSizer(wxHORIZONTAL);
	wxButton* cancel = new wxButton(this, wxID_CANCEL);
	buttons->Add(cancel, 0, wxALL, 8);
	wxButton* ok = new wxButton(this, wxID_OK);
	ok->SetDefault();
	buttons->Add(ok, 0, wxALL, 8);
	vert->Add(buttons, 0, wxALL|wxALIGN_RIGHT);

	SetAutoLayout(true);
	SetSizer(vert);

	vert->SetSizeHints(this);
    }
    PresentationMark GetMark() const {
	double a, t, s, T;
	Vector3 v(wxAtof(easting->GetValue()),
		  wxAtof(northing->GetValue()),
		  wxAtof(altitude->GetValue()));
	a = wxAtof(angle->GetValue());
	t = wxAtof(tilt_angle->GetValue());
	s = wxAtof(scale->GetValue());
	wxString str = time->GetValue();
	if (!str.empty() && str[0u] == '*') str[0u] = '-';
	T = wxAtof(str);
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
	void OnDeleteAllItems(wxListEvent&) {
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
	    if (item < 0) {
		AddMark(item, gfx->GetView());
		item = 0;
	    }
	    EditMarkDlg edit(mainfrm, entries[item]);
	    if (edit.ShowModal() == wxID_OK) {
		entries[item] = edit.GetMark();
	    }
	}
	void OnChar(wxKeyEvent& event) {
	    switch (event.GetKeyCode()) {
		case WXK_INSERT:
		    if (event.GetModifiers() == wxMOD_CONTROL) {
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
		ext += wxT("|*.fly");
#endif
		wxFileDialog dlg(this, wmsg(/*Select an output filename*/319),
				 wxString(), fnm, ext,
				 wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
		if (dlg.ShowModal() != wxID_OK) return;
		fnm = dlg.GetPath();
	    }

	    FILE * fh_pres = wxFopen(fnm, wxT("w"));
	    if (!fh_pres) {
		wxGetApp().ReportError(wxString::Format(wmsg(/*Error writing to file “%s”*/110), fnm.c_str()));
		return;
	    }
	    vector<PresentationMark>::const_iterator i;
	    for (i = entries.begin(); i != entries.end(); ++i) {
		const PresentationMark &p = *i;
		write_double(p.GetX(), fh_pres);
		PUTC(' ', fh_pres);
		write_double(p.GetY(), fh_pres);
		PUTC(' ', fh_pres);
		write_double(p.GetZ(), fh_pres);
		PUTC(' ', fh_pres);
		write_double(p.angle, fh_pres);
		PUTC(' ', fh_pres);
		write_double(p.tilt_angle, fh_pres);
		PUTC(' ', fh_pres);
		write_double(p.scale, fh_pres);
		if (p.time != 0.0) {
		    PUTC(' ', fh_pres);
		    write_double(p.time, fh_pres);
		}
		PUTC('\n', fh_pres);
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
	    FILE * fh_pres = wxFopen(fnm, wxT("r"));
	    if (!fh_pres) {
		wxString m;
		m.Printf(wmsg(/*Couldn’t open file “%s”*/24), fnm.c_str());
		wxGetApp().ReportError(m);
		return false;
	    }
	    DeleteAllItems();
	    long item = 0;
	    while (!feof(fh_pres)) {
		char buf[4096];
		size_t i = 0;
		while (i < sizeof(buf) - 1) {
		    int ch = GETC(fh_pres);
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
			    wxGetApp().ReportError(wxString::Format(wmsg(/*Error in format of presentation file “%s”*/323), fnm.c_str()));
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
    EVT_MENU(wxID_FIND, MainFrm::OnGotoFound)
    EVT_MENU(button_HIDE, MainFrm::OnHide)
    EVT_UPDATE_UI(button_HIDE, MainFrm::OnHideUpdate)
    EVT_IDLE(MainFrm::OnIdle)

    EVT_MENU(wxID_OPEN, MainFrm::OnOpen)
    EVT_MENU(menu_FILE_OPEN_TERRAIN, MainFrm::OnOpenTerrain)
    EVT_MENU(menu_FILE_LOG, MainFrm::OnShowLog)
    EVT_MENU(wxID_PRINT, MainFrm::OnPrint)
    EVT_MENU(menu_FILE_PAGE_SETUP, MainFrm::OnPageSetup)
    EVT_MENU(menu_FILE_SCREENSHOT, MainFrm::OnScreenshot)
//    EVT_MENU(wxID_PREFERENCES, MainFrm::OnFilePreferences)
    EVT_MENU(menu_FILE_EXPORT, MainFrm::OnExport)
    EVT_MENU(wxID_EXIT, MainFrm::OnQuit)
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
    EVT_MENU(wxID_STOP, MainFrm::OnPresStop)
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
    EVT_UPDATE_UI(wxID_STOP, MainFrm::OnPresStopUpdate)
    EVT_UPDATE_UI(menu_PRES_EXPORT_MOVIE, MainFrm::OnPresExportMovieUpdate)

    EVT_CLOSE(MainFrm::OnClose)
    EVT_SET_FOCUS(MainFrm::OnSetFocus)

    EVT_MENU(menu_ROTATION_TOGGLE, MainFrm::OnToggleRotation)
    EVT_MENU(menu_ROTATION_REVERSE, MainFrm::OnReverseDirectionOfRotation)
    EVT_MENU(menu_ORIENT_MOVE_NORTH, MainFrm::OnMoveNorth)
    EVT_MENU(menu_ORIENT_MOVE_EAST, MainFrm::OnMoveEast)
    EVT_MENU(menu_ORIENT_MOVE_SOUTH, MainFrm::OnMoveSouth)
    EVT_MENU(menu_ORIENT_MOVE_WEST, MainFrm::OnMoveWest)
    EVT_MENU(menu_ORIENT_PLAN, MainFrm::OnPlan)
    EVT_MENU(menu_ORIENT_ELEVATION, MainFrm::OnElevation)
    EVT_MENU(menu_ORIENT_DEFAULTS, MainFrm::OnDefaults)
    EVT_MENU(menu_VIEW_SHOW_LEGS, MainFrm::OnShowSurveyLegs)
    EVT_MENU(menu_SPLAYS_HIDE, MainFrm::OnHideSplays)
    EVT_MENU(menu_SPLAYS_SHOW_NORMAL, MainFrm::OnShowSplaysNormal)
    EVT_MENU(menu_SPLAYS_SHOW_FADED, MainFrm::OnShowSplaysFaded)
    EVT_MENU(menu_VIEW_SHOW_CROSSES, MainFrm::OnShowCrosses)
    EVT_MENU(menu_VIEW_SHOW_ENTRANCES, MainFrm::OnShowEntrances)
    EVT_MENU(menu_VIEW_SHOW_FIXED_PTS, MainFrm::OnShowFixedPts)
    EVT_MENU(menu_VIEW_SHOW_EXPORTED_PTS, MainFrm::OnShowExportedPts)
    EVT_MENU(menu_VIEW_SHOW_NAMES, MainFrm::OnShowStationNames)
    EVT_MENU(menu_VIEW_SHOW_OVERLAPPING_NAMES, MainFrm::OnDisplayOverlappingNames)
    EVT_MENU(menu_COLOUR_BY_DEPTH, MainFrm::OnColourByDepth)
    EVT_MENU(menu_COLOUR_BY_DATE, MainFrm::OnColourByDate)
    EVT_MENU(menu_COLOUR_BY_ERROR, MainFrm::OnColourByError)
    EVT_MENU(menu_COLOUR_BY_GRADIENT, MainFrm::OnColourByGradient)
    EVT_MENU(menu_COLOUR_BY_LENGTH, MainFrm::OnColourByLength)
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
    EVT_MENU(menu_VIEW_TERRAIN, MainFrm::OnViewTerrain)
    EVT_MENU(menu_IND_COMPASS, MainFrm::OnViewCompass)
    EVT_MENU(menu_IND_CLINO, MainFrm::OnViewClino)
    EVT_MENU(menu_IND_COLOUR_KEY, MainFrm::OnToggleColourKey)
    EVT_MENU(menu_IND_SCALE_BAR, MainFrm::OnToggleScalebar)
    EVT_MENU(menu_CTL_SIDE_PANEL, MainFrm::OnViewSidePanel)
    EVT_MENU(menu_CTL_METRIC, MainFrm::OnToggleMetric)
    EVT_MENU(menu_CTL_DEGREES, MainFrm::OnToggleDegrees)
    EVT_MENU(menu_CTL_PERCENT, MainFrm::OnTogglePercent)
    EVT_MENU(menu_CTL_REVERSE, MainFrm::OnReverseControls)
    EVT_MENU(menu_CTL_CANCEL_DIST_LINE, MainFrm::OnCancelDistLine)
    EVT_MENU(wxID_ABOUT, MainFrm::OnAbout)

    EVT_UPDATE_UI(menu_FILE_OPEN_TERRAIN, MainFrm::OnOpenTerrainUpdate)
    EVT_UPDATE_UI(menu_FILE_LOG, MainFrm::OnShowLogUpdate)
    EVT_UPDATE_UI(wxID_PRINT, MainFrm::OnPrintUpdate)
    EVT_UPDATE_UI(menu_FILE_SCREENSHOT, MainFrm::OnScreenshotUpdate)
    EVT_UPDATE_UI(menu_FILE_EXPORT, MainFrm::OnExportUpdate)
    EVT_UPDATE_UI(menu_ROTATION_TOGGLE, MainFrm::OnToggleRotationUpdate)
    EVT_UPDATE_UI(menu_ROTATION_REVERSE, MainFrm::OnReverseDirectionOfRotationUpdate)
    EVT_UPDATE_UI(menu_ORIENT_MOVE_NORTH, MainFrm::OnMoveNorthUpdate)
    EVT_UPDATE_UI(menu_ORIENT_MOVE_EAST, MainFrm::OnMoveEastUpdate)
    EVT_UPDATE_UI(menu_ORIENT_MOVE_SOUTH, MainFrm::OnMoveSouthUpdate)
    EVT_UPDATE_UI(menu_ORIENT_MOVE_WEST, MainFrm::OnMoveWestUpdate)
    EVT_UPDATE_UI(menu_ORIENT_PLAN, MainFrm::OnPlanUpdate)
    EVT_UPDATE_UI(menu_ORIENT_ELEVATION, MainFrm::OnElevationUpdate)
    EVT_UPDATE_UI(menu_ORIENT_DEFAULTS, MainFrm::OnDefaultsUpdate)
    EVT_UPDATE_UI(menu_VIEW_SHOW_LEGS, MainFrm::OnShowSurveyLegsUpdate)
    EVT_UPDATE_UI(menu_VIEW_SPLAYS, MainFrm::OnSplaysUpdate)
    EVT_UPDATE_UI(menu_SPLAYS_HIDE, MainFrm::OnHideSplaysUpdate)
    EVT_UPDATE_UI(menu_SPLAYS_SHOW_NORMAL, MainFrm::OnShowSplaysNormalUpdate)
    EVT_UPDATE_UI(menu_SPLAYS_SHOW_FADED, MainFrm::OnShowSplaysFadedUpdate)
    EVT_UPDATE_UI(menu_VIEW_SHOW_CROSSES, MainFrm::OnShowCrossesUpdate)
    EVT_UPDATE_UI(menu_VIEW_SHOW_ENTRANCES, MainFrm::OnShowEntrancesUpdate)
    EVT_UPDATE_UI(menu_VIEW_SHOW_FIXED_PTS, MainFrm::OnShowFixedPtsUpdate)
    EVT_UPDATE_UI(menu_VIEW_SHOW_EXPORTED_PTS, MainFrm::OnShowExportedPtsUpdate)
    EVT_UPDATE_UI(menu_VIEW_SHOW_NAMES, MainFrm::OnShowStationNamesUpdate)
    EVT_UPDATE_UI(menu_VIEW_SHOW_SURFACE, MainFrm::OnShowSurfaceUpdate)
    EVT_UPDATE_UI(menu_VIEW_SHOW_OVERLAPPING_NAMES, MainFrm::OnDisplayOverlappingNamesUpdate)
    EVT_UPDATE_UI(menu_VIEW_COLOUR_BY, MainFrm::OnColourByUpdate)
    EVT_UPDATE_UI(menu_COLOUR_BY_DEPTH, MainFrm::OnColourByDepthUpdate)
    EVT_UPDATE_UI(menu_COLOUR_BY_DATE, MainFrm::OnColourByDateUpdate)
    EVT_UPDATE_UI(menu_COLOUR_BY_ERROR, MainFrm::OnColourByErrorUpdate)
    EVT_UPDATE_UI(menu_COLOUR_BY_GRADIENT, MainFrm::OnColourByGradientUpdate)
    EVT_UPDATE_UI(menu_COLOUR_BY_LENGTH, MainFrm::OnColourByLengthUpdate)
    EVT_UPDATE_UI(menu_VIEW_GRID, MainFrm::OnViewGridUpdate)
    EVT_UPDATE_UI(menu_VIEW_BOUNDING_BOX, MainFrm::OnViewBoundingBoxUpdate)
    EVT_UPDATE_UI(menu_VIEW_PERSPECTIVE, MainFrm::OnViewPerspectiveUpdate)
    EVT_UPDATE_UI(menu_VIEW_SMOOTH_SHADING, MainFrm::OnViewSmoothShadingUpdate)
    EVT_UPDATE_UI(menu_VIEW_TEXTURED, MainFrm::OnViewTexturedUpdate)
    EVT_UPDATE_UI(menu_VIEW_FOG, MainFrm::OnViewFogUpdate)
    EVT_UPDATE_UI(menu_VIEW_SMOOTH_LINES, MainFrm::OnViewSmoothLinesUpdate)
    EVT_UPDATE_UI(menu_VIEW_FULLSCREEN, MainFrm::OnViewFullScreenUpdate)
    EVT_UPDATE_UI(menu_VIEW_SHOW_TUBES, MainFrm::OnToggleTubesUpdate)
    EVT_UPDATE_UI(menu_VIEW_TERRAIN, MainFrm::OnViewTerrainUpdate)
    EVT_UPDATE_UI(menu_IND_COMPASS, MainFrm::OnViewCompassUpdate)
    EVT_UPDATE_UI(menu_IND_CLINO, MainFrm::OnViewClinoUpdate)
    EVT_UPDATE_UI(menu_IND_COLOUR_KEY, MainFrm::OnToggleColourKeyUpdate)
    EVT_UPDATE_UI(menu_IND_SCALE_BAR, MainFrm::OnToggleScalebarUpdate)
    EVT_UPDATE_UI(menu_CTL_INDICATORS, MainFrm::OnIndicatorsUpdate)
    EVT_UPDATE_UI(menu_CTL_SIDE_PANEL, MainFrm::OnViewSidePanelUpdate)
    EVT_UPDATE_UI(menu_CTL_REVERSE, MainFrm::OnReverseControlsUpdate)
    EVT_UPDATE_UI(menu_CTL_CANCEL_DIST_LINE, MainFrm::OnCancelDistLineUpdate)
    EVT_UPDATE_UI(menu_CTL_METRIC, MainFrm::OnToggleMetricUpdate)
    EVT_UPDATE_UI(menu_CTL_DEGREES, MainFrm::OnToggleDegreesUpdate)
    EVT_UPDATE_UI(menu_CTL_PERCENT, MainFrm::OnTogglePercentUpdate)
END_EVENT_TABLE()

class LabelCmp : public greater<const LabelInfo*> {
    wxChar separator;
public:
    explicit LabelCmp(wxChar separator_) : separator(separator_) {}
    bool operator()(const LabelInfo* pt1, const LabelInfo* pt2) {
	return name_cmp(pt1->GetText(), pt2->GetText(), separator) < 0;
    }
};

class LabelPlotCmp : public greater<const LabelInfo*> {
    wxChar separator;
public:
    explicit LabelPlotCmp(wxChar separator_) : separator(separator_) {}
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
	explicit DnDFile(MainFrm *parent) : m_Parent(parent) { }
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
	/* TRANSLATORS: error if you try to drag multiple files to the aven
	 * window */
	wxGetApp().ReportError(wmsg(/*You may only view one 3d file at a time.*/336));
	return false;
    }

    m_Parent->OpenFile(filenames[0]);
    return true;
}
#endif

MainFrm::MainFrm(const wxString& title, const wxPoint& pos, const wxSize& size) :
    wxFrame(NULL, 101, title, pos, size, wxDEFAULT_FRAME_STYLE),
    m_SashPosition(-1),
    m_Gfx(NULL), m_Log(NULL),
    m_NumEntrances(0), m_NumFixedPts(0), m_NumExportedPts(0),
    m_NumHighlighted(0),
    m_HasUndergroundLegs(false), m_HasSplays(false), m_HasSurfaceLegs(false),
    m_HasErrorInformation(false), m_IsExtendedElevation(false),
    pending_find(false), fullscreen_showing_menus(false)
#ifdef PREFDLG
    , m_PrefsDlg(NULL)
#endif
{
#ifdef _WIN32
    // The peculiar name is so that the icon is the first in the file
    // (required by Microsoft Windows for this type of icon)
    SetIcon(wxICON(AAA_aven));
#else
    SetIcon(wxICON(aven));
#endif

#if wxCHECK_VERSION(3,1,0)
    // Add a full screen button to the right upper corner of title bar under OS
    // X 10.7 and later.
    EnableFullScreenView();
#endif
    CreateMenuBar();
    MakeToolBar();
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

void MainFrm::CreateMenuBar()
{
    // Create the menus and the menu bar.

    wxMenu* filemenu = new wxMenu;
    // wxID_OPEN stock label lacks the ellipses
    /* TRANSLATORS: Aven menu items.  An “&” goes before the letter of any
     * accelerator key.
     *
     * The string "\t" separates the menu text and any accelerator key.
     *
     * "File" menu.  The accelerators must be different within this group.
     * c.f. 201, 380, 381. */
    filemenu->Append(wxID_OPEN, wmsg(/*&Open...\tCtrl+O*/220));
    /* TRANSLATORS: Open a "Terrain file" - i.e. a digital model of the
     * terrain. */
    filemenu->Append(menu_FILE_OPEN_TERRAIN, wmsg(/*Open &Terrain...*/453));
    filemenu->Append(menu_FILE_LOG, wmsg(/*Show &Log*/144));
    filemenu->AppendSeparator();
    // wxID_PRINT stock label lacks the ellipses
    filemenu->Append(wxID_PRINT, wmsg(/*&Print...\tCtrl+P*/380));
    filemenu->Append(menu_FILE_PAGE_SETUP, wmsg(/*P&age Setup...*/381));
    filemenu->AppendSeparator();
    /* TRANSLATORS: In the "File" menu */
    filemenu->Append(menu_FILE_SCREENSHOT, wmsg(/*&Screenshot...*/201));
    filemenu->Append(menu_FILE_EXPORT, wmsg(/*&Export as...*/382));
#ifndef __WXMAC__
    // On wxMac the "Quit" menu item will be moved elsewhere, so we suppress
    // this separator.
    filemenu->AppendSeparator();
#else
    // We suppress the "Help" menu under OS X as it would otherwise end up as
    // an empty menu, but we need to add the "About" menu item somewhere.  It
    // really doesn't matter where as wxWidgets will move it to the "Apple"
    // menu.
    filemenu->Append(wxID_ABOUT);
#endif
    filemenu->Append(wxID_EXIT);

    m_history.UseMenu(filemenu);
    m_history.Load(*wxConfigBase::Get());

    wxMenu* rotmenu = new wxMenu;
    /* TRANSLATORS: "Rotation" menu.  The accelerators must be different within
     * this group.  Tickable menu item which toggles auto rotation */
    rotmenu->AppendCheckItem(menu_ROTATION_TOGGLE, wmsg(/*Au&to-Rotate\tSpace*/231));
    rotmenu->AppendSeparator();
    rotmenu->Append(menu_ROTATION_REVERSE, wmsg(/*&Reverse Direction*/234));

    wxMenu* orientmenu = new wxMenu;
    orientmenu->Append(menu_ORIENT_MOVE_NORTH, wmsg(/*View &North*/240));
    orientmenu->Append(menu_ORIENT_MOVE_EAST, wmsg(/*View &East*/241));
    orientmenu->Append(menu_ORIENT_MOVE_SOUTH, wmsg(/*View &South*/242));
    orientmenu->Append(menu_ORIENT_MOVE_WEST, wmsg(/*View &West*/243));
    orientmenu->AppendSeparator();
    orientmenu->Append(menu_ORIENT_PLAN, wmsg(/*&Plan View*/248));
    orientmenu->Append(menu_ORIENT_ELEVATION, wmsg(/*Ele&vation*/249));
    orientmenu->AppendSeparator();
    orientmenu->Append(menu_ORIENT_DEFAULTS, wmsg(/*Restore De&fault View*/254));

    wxMenu* presmenu = new wxMenu;
    presmenu->Append(menu_PRES_NEW, wmsg(/*&New Presentation*/311));
    presmenu->Append(menu_PRES_OPEN, wmsg(/*&Open Presentation...*/312));
    presmenu->Append(menu_PRES_SAVE, wmsg(/*&Save Presentation*/313));
    presmenu->Append(menu_PRES_SAVE_AS, wmsg(/*Sa&ve Presentation As...*/314));
    presmenu->AppendSeparator();
    /* TRANSLATORS: "Mark" as in "Mark this position" */
    presmenu->Append(menu_PRES_MARK, wmsg(/*&Mark*/315));
    /* TRANSLATORS: "Play" as in "Play back a recording" */
    presmenu->AppendCheckItem(menu_PRES_PLAY, wmsg(/*Pla&y*/316));
    presmenu->Append(menu_PRES_EXPORT_MOVIE, wmsg(/*&Export as Movie...*/317));

    wxMenu* viewmenu = new wxMenu;
#ifndef PREFDLG
    /* TRANSLATORS: Items in the "View" menu: */
    viewmenu->AppendCheckItem(menu_VIEW_SHOW_NAMES, wmsg(/*Station &Names\tCtrl+N*/270));
    /* TRANSLATORS: Toggles drawing of 3D passages */
    viewmenu->AppendCheckItem(menu_VIEW_SHOW_TUBES, wmsg(/*Passage &Tubes\tCtrl+T*/346));
    /* TRANSLATORS: Toggles drawing the surface of the Earth */
    viewmenu->AppendCheckItem(menu_VIEW_TERRAIN, wmsg(/*Terr&ain*/449));
    viewmenu->AppendCheckItem(menu_VIEW_SHOW_CROSSES, wmsg(/*&Crosses\tCtrl+X*/271));
    viewmenu->AppendCheckItem(menu_VIEW_GRID, wmsg(/*&Grid\tCtrl+G*/297));
    viewmenu->AppendCheckItem(menu_VIEW_BOUNDING_BOX, wmsg(/*&Bounding Box\tCtrl+B*/318));
    viewmenu->AppendSeparator();
    /* TRANSLATORS: Here a "survey leg" is a set of measurements between two
     * "survey stations". */
    viewmenu->AppendCheckItem(menu_VIEW_SHOW_LEGS, wmsg(/*&Underground Survey Legs\tCtrl+L*/272));
    /* TRANSLATORS: Here a "survey leg" is a set of measurements between two
     * "survey stations". */
    viewmenu->AppendCheckItem(menu_VIEW_SHOW_SURFACE, wmsg(/*&Surface Survey Legs\tCtrl+F*/291));

    wxMenu* splaymenu = new wxMenu;
    /* TRANSLATORS: Item in the "Splay Legs" submenu - if this is selected,
     * splay legs are not shown. */
    splaymenu->AppendCheckItem(menu_SPLAYS_HIDE, wmsg(/*&Hide*/407));
    /* TRANSLATORS: Item in the "Splay Legs" submenu - if this is selected,
     * aven will show splay legs with less bright colours (rather than the
     * same as other legs "Show" or not shown "Hide"). */
    splaymenu->AppendCheckItem(menu_SPLAYS_SHOW_FADED, wmsg(/*&Fade*/408));
    /* TRANSLATORS: Item in the "Splay Legs" submenu - if this is selected,
     * splay legs are shown the same as other legs. */
    splaymenu->AppendCheckItem(menu_SPLAYS_SHOW_NORMAL, wmsg(/*&Show*/409));
    viewmenu->Append(menu_VIEW_SPLAYS, wmsg(/*Spla&y Legs*/406), splaymenu);

    viewmenu->AppendSeparator();
    viewmenu->AppendCheckItem(menu_VIEW_SHOW_OVERLAPPING_NAMES, wmsg(/*&Overlapping Names*/273));

    wxMenu* colourbymenu = new wxMenu;
    colourbymenu->AppendCheckItem(menu_COLOUR_BY_DEPTH, wmsg(/*Colour by &Depth*/292));
    colourbymenu->AppendCheckItem(menu_COLOUR_BY_DATE, wmsg(/*Colour by D&ate*/293));
    colourbymenu->AppendCheckItem(menu_COLOUR_BY_ERROR, wmsg(/*Colour by &Error*/289));
    colourbymenu->AppendCheckItem(menu_COLOUR_BY_GRADIENT, wmsg(/*Colour by &Gradient*/85));
    colourbymenu->AppendCheckItem(menu_COLOUR_BY_LENGTH, wmsg(/*Colour by &Length*/82));

    viewmenu->Append(menu_VIEW_COLOUR_BY, wmsg(/*Co&lour by*/450), colourbymenu);

    viewmenu->AppendSeparator();
    viewmenu->AppendCheckItem(menu_VIEW_SHOW_ENTRANCES, wmsg(/*Highlight &Entrances*/294));
    viewmenu->AppendCheckItem(menu_VIEW_SHOW_FIXED_PTS, wmsg(/*Highlight &Fixed Points*/295));
    viewmenu->AppendCheckItem(menu_VIEW_SHOW_EXPORTED_PTS, wmsg(/*Highlight E&xported Points*/296));
    viewmenu->AppendSeparator();
#else
    viewmenu-> Append(menu_VIEW_CANCEL_DIST_LINE, wmsg(/*&Cancel Measuring Line\tEscape*/281));
#endif
    viewmenu->AppendCheckItem(menu_VIEW_PERSPECTIVE, wmsg(/*&Perspective*/237));
// FIXME: enable this    viewmenu->AppendCheckItem(menu_VIEW_SMOOTH_SHADING, wmsg(/*&Smooth Shading*/?!?);
    viewmenu->AppendCheckItem(menu_VIEW_TEXTURED, wmsg(/*Textured &Walls*/238));
    /* TRANSLATORS: Toggles OpenGL "Depth Fogging" - feel free to translate
     * using that term instead if it gives a better translation which most
     * users will understand. */
    viewmenu->AppendCheckItem(menu_VIEW_FOG, wmsg(/*Fade Distant Ob&jects*/239));
    /* TRANSLATORS: Here a "survey leg" is a set of measurements between two
     * "survey stations". */
    viewmenu->AppendCheckItem(menu_VIEW_SMOOTH_LINES, wmsg(/*Smoot&hed Survey Legs*/298));
    viewmenu->AppendSeparator();
#ifdef __WXMAC__
    // F11 on OS X is used by the desktop (for speaker volume and/or window
    // navigation).  The standard OS X shortcut for full screen mode is
    // Ctrl-Command-F which in wxWidgets terms is RawCtrl+Ctrl+F.
    wxString wxmac_fullscreen = wmsg(/*Full Screen &Mode\tF11*/356);
    wxmac_fullscreen.Replace(wxT("\tF11"), wxT("\tRawCtrl+Ctrl+F"), false);
    viewmenu->AppendCheckItem(menu_VIEW_FULLSCREEN, wxmac_fullscreen);
#else
    viewmenu->AppendCheckItem(menu_VIEW_FULLSCREEN, wmsg(/*Full Screen &Mode\tF11*/356));
#endif
#ifdef PREFDLG
    viewmenu->AppendSeparator();
    viewmenu-> Append(wxID_PREFERENCES, wmsg(/*&Preferences...*/347));
#endif

#ifndef PREFDLG
    wxMenu* ctlmenu = new wxMenu;
    ctlmenu->AppendCheckItem(menu_CTL_REVERSE, wmsg(/*&Reverse Sense\tCtrl+R*/280));
    ctlmenu->AppendSeparator();
#ifdef __WXGTK__
    // wxGTK (at least with GTK+ v2.24), if we specify a short-cut here then
    // the key handler isn't called, so we can't exit full screen mode on
    // Escape.  wxGTK doesn't actually show the "Escape" shortcut text in the
    // menu item, so removing it doesn't make any visual difference, and doing
    // so allows Escape to still cancel the measuring line, but also serve to
    // exit full screen mode if no measuring line is shown.
    wxString wxgtk_cancelline = wmsg(/*&Cancel Measuring Line\tEscape*/281);
    wxgtk_cancelline.Replace(wxT("\tEscape"), wxT(""), false);
    ctlmenu->Append(menu_CTL_CANCEL_DIST_LINE, wxgtk_cancelline);
#else
    // With wxMac and wxMSW, we can have the short-cut on the menu and still
    // have Escape handled by the key handler to exit full screen mode.
    ctlmenu->Append(menu_CTL_CANCEL_DIST_LINE, wmsg(/*&Cancel Measuring Line\tEscape*/281));
#endif
    ctlmenu->AppendSeparator();
    wxMenu* indmenu = new wxMenu;
    indmenu->AppendCheckItem(menu_IND_COMPASS, wmsg(/*&Compass*/274));
    indmenu->AppendCheckItem(menu_IND_CLINO, wmsg(/*C&linometer*/275));
    /* TRANSLATORS: The "Colour Key" is the thing in aven showing which colour
     * corresponds to which depth, date, survey closure error, etc. */
    indmenu->AppendCheckItem(menu_IND_COLOUR_KEY, wmsg(/*Colour &Key*/276));
    indmenu->AppendCheckItem(menu_IND_SCALE_BAR, wmsg(/*&Scale Bar*/277));
    ctlmenu->Append(menu_CTL_INDICATORS, wmsg(/*&Indicators*/299), indmenu);
    ctlmenu->AppendCheckItem(menu_CTL_SIDE_PANEL, wmsg(/*&Side Panel*/337));
    ctlmenu->AppendSeparator();
    ctlmenu->AppendCheckItem(menu_CTL_METRIC, wmsg(/*&Metric*/342));
    ctlmenu->AppendCheckItem(menu_CTL_DEGREES, wmsg(/*&Degrees*/343));
    ctlmenu->AppendCheckItem(menu_CTL_PERCENT, wmsg(/*&Percent*/430));
#endif

    wxMenuBar* menubar = new wxMenuBar();
    /* TRANSLATORS: Aven menu titles.  An “&” goes before the letter of any
     * accelerator key.  The accelerators must be different within this group
     */
    menubar->Append(filemenu, wmsg(/*&File*/210));
    menubar->Append(rotmenu, wmsg(/*&Rotation*/211));
    menubar->Append(orientmenu, wmsg(/*&Orientation*/212));
    menubar->Append(viewmenu, wmsg(/*&View*/213));
#ifndef PREFDLG
    menubar->Append(ctlmenu, wmsg(/*&Controls*/214));
#endif
    // TRANSLATORS: "Presentation" in the sense of a talk with a slideshow -
    // the items in this menu allow the user to animate between preset
    // views.
    menubar->Append(presmenu, wmsg(/*&Presentation*/216));
#ifndef __WXMAC__
    // On wxMac the "About" menu item will be moved elsewhere, so we suppress
    // this menu since it will then be empty.
    wxMenu* helpmenu = new wxMenu;
    helpmenu->Append(wxID_ABOUT);

    menubar->Append(helpmenu, wmsg(/*&Help*/215));
#endif
    SetMenuBar(menubar);
}

void MainFrm::MakeToolBar()
{
    // Make the toolbar.

#ifdef USING_GENERIC_TOOLBAR
    // This OS-X-specific code is only needed to stop the toolbar icons getting
    // scaled up, which just makes them look nasty and fuzzy.  Once we have
    // larger versions of the icons, we can drop this code.
    wxSystemOptions::SetOption(wxT("mac.toolbar.no-native"), 1);
    wxToolBar* toolbar = new wxToolBar(this, wxID_ANY, wxDefaultPosition,
				       wxDefaultSize, wxNO_BORDER|wxTB_FLAT|wxTB_NODIVIDER|wxTB_NOALIGN);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(toolbar, 0, wxEXPAND);
    SetSizer(sizer);
#else
    wxToolBar* toolbar = wxFrame::CreateToolBar();
#endif

#ifndef __WXGTK20__
    toolbar->SetMargins(5, 5);
#endif

    // FIXME: TRANSLATE tooltips
    toolbar->AddTool(wxID_OPEN, wxT("Open"), TOOL(open), wxT("Open a survey file for viewing"));
    toolbar->AddTool(menu_PRES_OPEN, wxT("Open presentation"), TOOL(open_pres), wxT("Open a presentation"));
    toolbar->AddTool(menu_FILE_LOG, wxT("View log"), TOOL(log), wxT("View log from processing survey data"));
    toolbar->AddSeparator();
    toolbar->AddCheckTool(menu_ROTATION_TOGGLE, wxT("Toggle rotation"), TOOL(rotation), wxNullBitmap, wxT("Toggle rotation"));
    toolbar->AddTool(menu_ORIENT_PLAN, wxT("Plan"), TOOL(plan), wxT("Switch to plan view"));
    toolbar->AddTool(menu_ORIENT_ELEVATION, wxT("Elevation"), TOOL(elevation), wxT("Switch to elevation view"));
    toolbar->AddTool(menu_ORIENT_DEFAULTS, wxT("Default view"), TOOL(defaults), wxT("Restore default view"));
    toolbar->AddSeparator();
    toolbar->AddCheckTool(menu_VIEW_SHOW_NAMES, wxT("Names"), TOOL(names), wxNullBitmap, wxT("Show station names"));
    toolbar->AddCheckTool(menu_VIEW_SHOW_CROSSES, wxT("Crosses"), TOOL(crosses), wxNullBitmap, wxT("Show crosses on stations"));
    toolbar->AddCheckTool(menu_VIEW_SHOW_ENTRANCES, wxT("Entrances"), TOOL(entrances), wxNullBitmap, wxT("Highlight entrances"));
    toolbar->AddCheckTool(menu_VIEW_SHOW_FIXED_PTS, wxT("Fixed points"), TOOL(fixed_pts), wxNullBitmap, wxT("Highlight fixed points"));
    toolbar->AddCheckTool(menu_VIEW_SHOW_EXPORTED_PTS, wxT("Exported points"), TOOL(exported_pts), wxNullBitmap, wxT("Highlight exported stations"));
    toolbar->AddSeparator();
    toolbar->AddCheckTool(menu_VIEW_SHOW_LEGS, wxT("Underground legs"), TOOL(ug_legs), wxNullBitmap, wxT("Show underground surveys"));
    toolbar->AddCheckTool(menu_VIEW_SHOW_SURFACE, wxT("Surface legs"), TOOL(surface_legs), wxNullBitmap, wxT("Show surface surveys"));
    toolbar->AddCheckTool(menu_VIEW_SHOW_TUBES, wxT("Tubes"), TOOL(tubes), wxNullBitmap, wxT("Show passage tubes"));
    toolbar->AddCheckTool(menu_VIEW_TERRAIN, wxT("Terrain"), TOOL(solid_surface), wxNullBitmap, wxT("Show terrain"));
    toolbar->AddSeparator();
    toolbar->AddCheckTool(menu_PRES_FREWIND, wxT("Fast Rewind"), TOOL(pres_frew), wxNullBitmap, wxT("Very Fast Rewind"));
    toolbar->AddCheckTool(menu_PRES_REWIND, wxT("Rewind"), TOOL(pres_rew), wxNullBitmap, wxT("Fast Rewind"));
    toolbar->AddCheckTool(menu_PRES_REVERSE, wxT("Backwards"), TOOL(pres_go_back), wxNullBitmap, wxT("Play Backwards"));
    toolbar->AddCheckTool(menu_PRES_PAUSE, wxT("Pause"), TOOL(pres_pause), wxNullBitmap, wxT("Pause"));
    toolbar->AddCheckTool(menu_PRES_PLAY, wxT("Go"), TOOL(pres_go), wxNullBitmap, wxT("Play"));
    toolbar->AddCheckTool(menu_PRES_FF, wxT("FF"), TOOL(pres_ff), wxNullBitmap, wxT("Fast Forward"));
    toolbar->AddCheckTool(menu_PRES_FFF, wxT("Very FF"), TOOL(pres_fff), wxNullBitmap, wxT("Very Fast Forward"));
    toolbar->AddTool(wxID_STOP, wxT("Stop"), TOOL(pres_stop), wxT("Stop"));

    toolbar->AddSeparator();
    m_FindBox = new wxTextCtrl(toolbar, textctrl_FIND, wxString(), wxDefaultPosition,
			       wxDefaultSize, wxTE_PROCESS_ENTER);
    toolbar->AddControl(m_FindBox);
    /* TRANSLATORS: "Find stations" button tooltip */
    toolbar->AddTool(wxID_FIND, wmsg(/*Find*/332), TOOL(find)/*, "Search for station name"*/);
    /* TRANSLATORS: "Hide stations" button default tooltip */
    toolbar->AddTool(button_HIDE, wmsg(/*Hide*/333), TOOL(hideresults)/*, "Hide search results"*/);

    toolbar->Realize();
}

void MainFrm::CreateSidePanel()
{
    m_Splitter = new AvenSplitterWindow(this);
#ifdef USING_GENERIC_TOOLBAR
    // This OS-X-specific code is only needed to stop the toolbar icons getting
    // scaled up, which just makes them look nasty and fuzzy.  Once we have
    // larger versions of the icons, we can drop this code.
    GetSizer()->Add(m_Splitter, 1, wxEXPAND);
    Layout();
#endif

    m_Notebook = new wxNotebook(m_Splitter, 400, wxDefaultPosition,
				wxDefaultSize,
				wxBK_BOTTOM | wxBK_LEFT);
    m_Notebook->Show(false);

    wxPanel * panel = new wxPanel(m_Notebook);
    m_Tree = new AvenTreeCtrl(this, panel);

//    m_RegexpCheckBox = new wxCheckBox(find_panel, -1,
//				      msg(/*Regular expression*/));

    wxBoxSizer *panel_sizer = new wxBoxSizer(wxVERTICAL);
    panel_sizer->Add(m_Tree, 1, wxALL | wxEXPAND, 2);
    panel->SetAutoLayout(true);
    panel->SetSizer(panel_sizer);
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
    image_list->Add(TOOL(survey_tree));
    image_list->Add(TOOL(pres_tree));
    m_Notebook->SetImageList(image_list);
    /* TRANSLATORS: labels for tabbed side panel this is for the tab with the
     * tree hierarchy of survey station names */
    m_Notebook->AddPage(panel, wmsg(/*Surveys*/376), true, 0);
    m_Notebook->AddPage(prespanel, wmsg(/*Presentation*/377), false, 1);

    m_Splitter->Initialize(m_Gfx);
}

bool MainFrm::LoadData(const wxString& file, const wxString & prefix)
{
    // Load survey data from file, centre the dataset around the origin,
    // and prepare the data for drawing.

#if 0
    wxStopWatch timer;
    timer.Start();
#endif

    // Load the processed survey data.
    img* survey = img_open_survey(file.utf8_str(), prefix.utf8_str());
    if (!survey) {
	wxString m = wxString::Format(wmsg(img_error2msg(img_error())), file.c_str());
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
    m_HasSplays = false;
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

    m_DateMin = INT_MAX;
    int datemax = -1;
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

    map<wxString, LabelInfo *> labelmap;
    list<LabelInfo*>::const_iterator last_mapped_label = m_Labels.begin();

    int result;
    img_point prev_pt = {0,0,0};
    bool current_polyline_is_surface = false;
    bool current_polyline_is_splay = false;
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

		int date = survey->days1;
		if (date != -1) {
		    date += (survey->days2 - date) / 2;
		    if (date < m_DateMin) m_DateMin = date;
		    if (date > datemax) datemax = date;
		} else {
		    complete_dateinfo = false;
		}

		if (survey->flags & img_FLAG_SPLAY)
		    m_HasSplays = true;
		bool is_surface = (survey->flags & img_FLAG_SURFACE);
		bool is_splay = (survey->flags & img_FLAG_SPLAY);
		if (!is_surface) {
		    if (pt.z < m_DepthMin) m_DepthMin = pt.z;
		    if (pt.z > depthmax) depthmax = pt.z;
		}
		if (pending_move || current_polyline_is_surface != is_surface || current_polyline_is_splay != is_splay) {
		    if (!current_polyline_is_surface && current_traverse) {
			//FixLRUD(*current_traverse);
		    }

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
			current_traverse->isSplay = is_splay;
			++n_traverses;
			// The previous point was at a surface->ug transition.
			if (current_polyline_is_surface) {
			    if (prev_pt.z < m_DepthMin) m_DepthMin = prev_pt.z;
			    if (prev_pt.z > depthmax) depthmax = prev_pt.z;
			}
		    }

		    current_polyline_is_surface = is_surface;
		    current_polyline_is_splay = is_splay;

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
		wxString s(survey->label, wxConvUTF8);
		if (s.empty()) {
		    // If label isn't valid UTF-8 then this conversion will
		    // give an empty string.  In this case, assume that the
		    // label is CP1252 (the Microsoft superset of ISO8859-1).
		    static wxCSConv ConvCP1252(wxFONTENCODING_CP1252);
		    s = wxString(survey->label, ConvCP1252);
		    if (s.empty()) {
			// Or if that doesn't work (ConvCP1252 doesn't like
			// strings with some bytes in) let's just go for
			// ISO8859-1.
			s = wxString(survey->label, wxConvISO8859_1);
		    }
		}
		int flags = img2aven(survey->flags);
		LabelInfo* label = new LabelInfo(pt, s, flags);
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

		LabelInfo * lab;
		wxString label(survey->label, wxConvUTF8);
		map<wxString, LabelInfo *>::const_iterator p;
		p = labelmap.find(label);
		if (p != labelmap.end()) {
		    lab = p->second;
		} else {
		    // Initialise labelmap lazily - we may have no
		    // cross-sections.
		    list<LabelInfo*>::const_iterator i;
		    if (labelmap.empty()) {
			i = m_Labels.begin();
		    } else {
			i = last_mapped_label;
			++i;
		    }
		    while (i != m_Labels.end() && (*i)->GetText() != label) {
			labelmap[(*i)->GetText()] = *i;
			++i;
		    }
		    last_mapped_label = i;
		    if (i == m_Labels.end()) {
			// Unattached cross-section - ignore for now.
			printf("unattached cross-section\n");
			if (current_tube->size() <= 1)
			    tubes.resize(tubes.size() - 1);
			current_tube = NULL;
			if (!m_Labels.empty())
			    --last_mapped_label;
			break;
		    }
		    lab = *i;
		    labelmap[label] = lab;
		}

		int date = survey->days1;
		if (date != -1) {
		    date += (survey->days2 - date) / 2;
		    if (date < m_DateMin) m_DateMin = date;
		    if (date > datemax) datemax = date;
		}

		current_tube->push_back(XSect(*lab, date, survey->l, survey->r, survey->u, survey->d));
		break;
	    }

	    case img_XSECT_END:
		// Finish off current_tube.
		// If there's only one cross-section in the tube, just
		// discard it for now.  FIXME: we should handle this
		// when we come to skinning the tubes.
		if (current_tube && current_tube->size() <= 1)
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
		m_HasSplays = false;
		m_HasSurfaceLegs = false;

		img_close(survey);

		wxString m = wxString::Format(wmsg(img_error2msg(img_error())), file.c_str());
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
    if (current_tube && current_tube->size() <= 1)
	tubes.resize(tubes.size() - 1);

    separator = survey->separator;
    m_Title = wxString(survey->title, wxConvUTF8);
    m_DateStamp_numeric = survey->datestamp_numeric;
    if (survey->cs) {
	m_cs_proj = wxString(survey->cs, wxConvUTF8);
    } else {
	m_cs_proj = wxString();
    }
    if (strcmp(survey->datestamp, "?") == 0) {
	/* TRANSLATORS: used a processed survey with no processing date/time info */
	m_DateStamp = wmsg(/*Date and time not available.*/108);
    } else if (survey->datestamp[0] == '@') {
	const struct tm * tm = localtime(&m_DateStamp_numeric);
	char buf[256];
	/* TRANSLATORS: This is the date format string used to timestamp .3d
	 * files internally.  Probably best to keep it the same for all
	 * translations. */
	strftime(buf, 256, msg(/*%a,%Y.%m.%d %H:%M:%S %Z*/107), tm);
	m_DateStamp = wxString(buf, wxConvUTF8);
    }
    if (m_DateStamp.empty()) {
	m_DateStamp = wxString(survey->datestamp, wxConvUTF8);
    }
    img_close(survey);

    // Check we've actually loaded some legs or stations!
    if (!m_HasUndergroundLegs && !m_HasSurfaceLegs && m_Labels.empty()) {
	wxString m = wxString::Format(wmsg(/*No survey data in 3d file “%s”*/202), file.c_str());
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

    if (datemax < m_DateMin) m_DateMin = datemax;
    m_DateExt = datemax - m_DateMin;

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
    SetTitle(m_Title + " - " APP_NAME);

    // Sort the labels ready for filling the tree.
    m_Labels.sort(LabelCmp(separator));

    // Fill the tree of stations and prefixes.
    FillTree(wxFileNameFromPath(file));

    // Sort labels so that entrances are displayed in preference,
    // then fixed points, then exported points, then other points.
    //
    // Also sort by leaf name so that we'll tend to choose labels
    // from different surveys, rather than labels from surveys which
    // are earlier in the list.
    m_Labels.sort(LabelPlotCmp(separator));

    if (!m_FindBox->GetValue().empty()) {
	// Highlight any stations matching the current search.
	DoFind();
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

void MainFrm::FillTree(const wxString & root_name)
{
    // Create the root of the tree.
    wxTreeItemId treeroot = m_Tree->AddRoot(root_name);

    // Fill the tree of stations and prefixes.
    stack<wxTreeItemId> previous_ids;
    wxString current_prefix;
    wxTreeItemId current_id = treeroot;

    list<LabelInfo*>::iterator pos = m_Labels.begin();
    while (pos != m_Labels.end()) {
	LabelInfo* label = *pos++;

	if (label->IsAnon()) continue;

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
		size_t n = min(prefix.length(), current_prefix.length());
		size_t i;
		for (i = 0; i < n && prefix[i] == current_prefix[i]; ++i) {
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
	    m_Tree->SetItemTextColour(id, wxColour(0, 255, 40));
	} else if (label->IsSurface()) {
	    // Surface stations are dark green.
	    m_Tree->SetItemTextColour(id, wxColour(49, 158, 79));
	}
    }

    m_Tree->Expand(treeroot);
    m_Tree->SetEnabled();
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

void MainFrm::OpenFile(const wxString& file, const wxString& survey)
{
    wxBusyCursor hourglass;

    // Check if this is an unprocessed survey data file.
    if (file.length() > 4 && file[file.length() - 4] == '.') {
	wxString ext(file, file.length() - 3, 3);
	ext.MakeLower();
	if (ext == wxT("svx") || ext == wxT("dat") || ext == wxT("mak")) {
	    CavernLogWindow * log = new CavernLogWindow(this, survey, m_Splitter);
	    wxWindow * win = m_Splitter->GetWindow1();
	    m_Splitter->ReplaceWindow(win, log);
	    win->Show(false);
	    if (m_Splitter->GetWindow2() == NULL) {
		if (win != m_Gfx) win->Destroy();
	    } else {
		if (m_Splitter->IsSplit()) m_Splitter->Unsplit();
	    }

	    if (wxFileExists(file)) AddToFileHistory(file);
	    log->process(file);
	    // Log window will tell us to load file if it successfully completes.
	    return;
	}
    }

    if (!LoadData(file, survey))
	return;
    AddToFileHistory(file);
    InitialiseAfterLoad(file);

    // If aven is showing the log for a .svx file and you load a .3d file, then
    // at this point m_Log will be the log window for the .svx file, so destroy
    // it - it should never legitimately be set if we get here.
    if (m_Log) {
	m_Log->Destroy();
	m_Log = NULL;
    }
}

void MainFrm::InitialiseAfterLoad(const wxString & file)
{
    if (m_SashPosition < 0) {
	// Calculate sane default width for side panel.
	int x;
	int y;
	GetClientSize(&x, &y);
	if (x < 600)
	    x /= 3;
	else if (x < 1000)
	    x = 200;
	else
	    x /= 5;
	m_SashPosition = x;
    }

    // Do this before we potentially delete the log window which may own the
    // wxString which parameter file refers to!
    bool same_file = (file == m_File);
    if (!same_file)
	m_File = file;

    wxWindow * win = NULL;
    if (m_Splitter->GetWindow2() == NULL) {
	win = m_Splitter->GetWindow1();
	if (win == m_Gfx) win = NULL;
    }

    if (!IsFullScreen()) {
	m_Splitter->SplitVertically(m_Notebook, m_Gfx, m_SashPosition);
    } else {
	was_showing_sidepanel_before_fullscreen = true;
    }

    m_Gfx->Initialise(same_file);

    if (win) {
	// FIXME: check it actually is the log window!
	if (m_Log && m_Log != win)
	    m_Log->Destroy();
	m_Log = win;
	m_Log->Show(false);
    }

    if (!IsFullScreen()) {
	m_Notebook->Show(true);
    }

    m_Gfx->Show(true);
    m_Gfx->SetFocus();
}

void MainFrm::HideLog(wxWindow * log_window)
{
    if (!IsFullScreen()) {
	m_Splitter->SplitVertically(m_Notebook, m_Gfx, m_SashPosition);
    }

    m_Log = log_window;
    m_Log->Show(false);

    if (!IsFullScreen()) {
	m_Notebook->Show(true);
    }

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
# define CASE(X) ";" X
#endif

void MainFrm::OnOpen(wxCommandEvent&)
{
    AvenAllowOnTop ontop(this);
#ifdef __WXMOTIF__
    wxString filetypes = wxT("*.3d");
#else
    wxString filetypes;
    filetypes.Printf(wxT("%s|*.3d;*.svx;*.plt;*.plf;*.dat;*.mak;*.adj;*.sht;*.una;*.xyz"
		     CASE("*.3D;*.SVX;*.PLT;*.PLF;*.DAT;*.MAK;*.ADJ;*.SHT;*.UNA;*.XYZ")
		     "|%s|*.3d" CASE("*.3D")
		     "|%s|*.svx" CASE("*.SVX")
		     "|%s|*.plt;*.plf" CASE("*.PLT;*.PLF")
		     "|%s|*.dat;*.mak" CASE("*.DAT;*.MAK")
		     "|%s|*.adj;*.sht;*.una;*.xyz" CASE("*.ADJ;*.SHT;*.UNA;*.XYZ")
		     "|%s|%s"),
		     /* TRANSLATORS: Here "survey" is a "cave map" rather than
		      * list of questions - it should be translated to the
		      * terminology that cavers using the language would use.
		      */
		     wmsg(/*All survey files*/229).c_str(),
		     /* TRANSLATORS: Survex is the name of the software, and "3d" refers to a
		      * file extension, so neither should be translated. */
		     wmsg(/*Survex 3d files*/207).c_str(),
		     /* TRANSLATORS: Survex is the name of the software, and "svx" refers to a
		      * file extension, so neither should be translated. */
		     wmsg(/*Survex svx files*/329).c_str(),
		     /* TRANSLATORS: "Compass" as in Larry Fish’s cave
		      * surveying package, so probably shouldn’t be translated
		      */
		     wmsg(/*Compass PLT files*/324).c_str(),
		     /* TRANSLATORS: "Compass" as in Larry Fish’s cave
		      * surveying package, so should not be translated
		      */
		     wmsg(/*Compass DAT and MAK files*/330).c_str(),
		     /* TRANSLATORS: "CMAP" is Bob Thrun’s cave surveying
		      * package, so don’t translate it. */
		     wmsg(/*CMAP XYZ files*/325).c_str(),
		     wmsg(/*All files*/208).c_str(),
		     wxFileSelectorDefaultWildcardStr);
#endif
    /* TRANSLATORS: Here "survey" is a "cave map" rather than list of questions
     * - it should be translated to the terminology that cavers using the
     * language would use.
     *
     * File->Open dialog: */
    wxFileDialog dlg(this, wmsg(/*Select a survey file to view*/206),
		     wxString(), wxString(),
		     filetypes, wxFD_OPEN|wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() == wxID_OK) {
	OpenFile(dlg.GetPath());
    }
}

void MainFrm::OnOpenTerrain(wxCommandEvent&)
{
    if (!m_Gfx) return;

    if (m_cs_proj.empty()) {
	wxMessageBox(wxT("No coordinate system specified in survey data"));
	return;
    }

#ifdef __WXMOTIF__
    wxString filetypes = wxT("*.*");
#else
    wxString filetypes;
    filetypes.Printf(wxT("%s|*.bil;*.hgt;*.zip" CASE("*.BIL;*.HGT;*.ZIP")
		     "|%s|%s"),
		     wmsg(/*Terrain files*/452).c_str(),
		     wmsg(/*All files*/208).c_str(),
		     wxFileSelectorDefaultWildcardStr);
#endif
    /* TRANSLATORS: "Terrain file" being a digital model of the terrain (e.g. a
     * grid of height values). */
    wxFileDialog dlg(this, wmsg(/*Select a terrain file to view*/451),
		     wxString(), wxString(),
		     filetypes, wxFD_OPEN|wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() == wxID_OK && m_Gfx->LoadDEM(dlg.GetPath())) {
	if (!m_Gfx->DisplayingTerrain()) m_Gfx->ToggleTerrain();
    }
}

void MainFrm::OnShowLog(wxCommandEvent&)
{
    if (!m_Log) return;
    wxWindow * win = m_Splitter->GetWindow1();
    m_Splitter->ReplaceWindow(win, m_Log);
    win->Show(false);
    if (m_Splitter->IsSplit()) {
	m_SashPosition = m_Splitter->GetSashPosition(); // save width of panel
	m_Splitter->Unsplit();
    }
    m_Log->Show(true);
    m_Log->SetFocus();
    m_Log = NULL;
}

void MainFrm::OnScreenshot(wxCommandEvent&)
{
    AvenAllowOnTop ontop(this);
    wxString baseleaf;
    wxFileName::SplitPath(m_File, NULL, NULL, &baseleaf, NULL, wxPATH_NATIVE);
    /* TRANSLATORS: title of the save screenshot dialog */
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
	    wxGetApp().ReportError(wxString::Format(wmsg(/*Error writing to file “%s”*/110), dlg.GetPath().c_str()));
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
    m_Gfx->OnPrint(m_File, m_Title, m_DateStamp, m_DateStamp_numeric, m_cs_proj);
}

void MainFrm::PrintAndExit()
{
    m_Gfx->OnPrint(m_File, m_Title, m_DateStamp, m_DateStamp_numeric, m_cs_proj, true);
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
    m_Gfx->OnExport(m_File, m_Title, m_DateStamp, m_DateStamp_numeric, m_cs_proj);
}

void MainFrm::OnQuit(wxCommandEvent&)
{
    if (m_PresList->Modified()) {
	AvenAllowOnTop ontop(this);
	// FIXME: better to ask "Do you want to save your changes?" and offer [Save] [Discard] [Cancel]
	/* TRANSLATORS: and the question in that box */
	if (wxMessageBox(wmsg(/*The current presentation has been modified.  Abandon unsaved changes?*/327),
			 /* TRANSLATORS: title of message box */
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
#ifdef __WXMAC__
    // GetIcon() returns an invalid wxIcon under OS X.
    AboutDlg dlg(this, wxICON(aven));
#else
    AboutDlg dlg(this, GetIcon());
#endif
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
    m_Gfx->SetThere();
    ShowInfo();
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
    Double x = v.GetX();
    Double y = v.GetY();
    Double z = v.GetZ();
    int units;
    if (m_Gfx->GetMetric()) {
	units = /*m*/424;
    } else {
	x /= METRES_PER_FOOT;
	y /= METRES_PER_FOOT;
	z /= METRES_PER_FOOT;
	units = /*ft*/428;
    }
    /* TRANSLATORS: show coordinates (N = North or Northing, E = East or
     * Easting) */
    coords_text.Printf(wmsg(/*%.2f E, %.2f N*/338), x, y);
    coords_text += wxString::Format(wxT(", %s %.2f%s"),
				    wmsg(/*Altitude*/335).c_str(),
				    z, wmsg(units).c_str());
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

void MainFrm::SetCoords(Double x, Double y, const LabelInfo * there)
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
    if (m_Gfx->ShowingMeasuringLine() && there) {
	Vector3 delta(x - m_Offsets.GetX() - there->GetX(),
		      y - m_Offsets.GetY() - there->GetY(), 0);
	Double dh = sqrt(delta.GetX()*delta.GetX() + delta.GetY()*delta.GetY());
	Double brg = deg(atan2(delta.GetX(), delta.GetY()));
	if (brg < 0) brg += 360;

	wxString from_str;
	/* TRANSLATORS: Used in Aven:
	 * From <stationname>
	 * H: 123.45m V: 234.56m
	 * Dist: 456.78m  Brg: 200 */
	from_str.Printf(wmsg(/*From %s*/339), there->name_or_anon().c_str());
	int brg_unit;
	if (m_Gfx->GetDegrees()) {
	    brg_unit = /*°*/344;
	} else {
	    brg *= 400.0 / 360.0;
	    /* TRANSLATORS: alternative angle units (400 grad = 360 degrees =
	     * full circle).  Try to make this as short as sensibly possible.
	     *
	     * E.g. in Italian and Spanish we use "g.cent." rather than
	     * "gradi quattrocentesimali"/"grados centesimales" */
	    brg_unit = /*grad*/345;
	}

	int units;
	if (m_Gfx->GetMetric()) {
	    units = /*m*/424;
	} else {
	    dh /= METRES_PER_FOOT;
	    units = /*ft*/428;
	}
	/* TRANSLATORS: "H" is short for "Horizontal", "Brg" for "Bearing" (as
	 * in Compass bearing) */
	t.Printf(wmsg(/*%s: H %.2f%s, Brg %03d%s*/374),
		 from_str.c_str(), dh, wmsg(units).c_str(),
		 int(brg), wmsg(brg_unit).c_str());
    }

    UpdateStatusBar();
}

void MainFrm::SetAltitude(Double z, const LabelInfo * there)
{
    double alt = z;
    int units;
    if (m_Gfx->GetMetric()) {
	units = /*m*/424;
    } else {
	alt /= METRES_PER_FOOT;
	units = /*ft*/428;
    }
    coords_text.Printf(wxT("%s %.2f%s"), wmsg(/*Altitude*/335).c_str(),
		       alt, wmsg(units).c_str());

    wxString & t = distfree_text;
    t = wxString();
    if (m_Gfx->ShowingMeasuringLine() && there) {
	Double dz = z - m_Offsets.GetZ() - there->GetZ();

	wxString from_str;
	from_str.Printf(wmsg(/*From %s*/339), there->name_or_anon().c_str());

	if (!m_Gfx->GetMetric()) {
	    dz /= METRES_PER_FOOT;
	}
	// TRANSLATORS: "V" is short for "Vertical"
	t.Printf(wmsg(/*%s: V %.2f%s*/375), from_str.c_str(),
		 dz, wmsg(units).c_str());
    }

    UpdateStatusBar();
}

void MainFrm::ShowInfo(const LabelInfo *here, const LabelInfo *there)
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
    Double x = v.GetX();
    Double y = v.GetY();
    Double z = v.GetZ();
    int units;
    if (m_Gfx->GetMetric()) {
	units = /*m*/424;
    } else {
	x /= METRES_PER_FOOT;
	y /= METRES_PER_FOOT;
	z /= METRES_PER_FOOT;
	units = /*ft*/428;
    }
    s.Printf(wmsg(/*%.2f E, %.2f N*/338), x, y);
    s += wxString::Format(wxT(", %s %.2f%s"), wmsg(/*Altitude*/335).c_str(),
			  z, wmsg(units).c_str());
    s += wxT(": ");
    s += here->name_or_anon();
    m_Gfx->SetHere(here);
    m_Tree->SetHere(here->tree_id);

    if (m_Gfx->ShowingMeasuringLine() && there) {
	Vector3 delta = *here - *there;

	Double d_horiz = sqrt(delta.GetX()*delta.GetX() +
			      delta.GetY()*delta.GetY());
	Double dr = delta.magnitude();
	Double dz = delta.GetZ();

	Double brg = deg(atan2(delta.GetX(), delta.GetY()));
	if (brg < 0) brg += 360;

	wxString from_str;
	from_str.Printf(wmsg(/*From %s*/339), there->name_or_anon().c_str());

	wxString hv_str;
	if (m_Gfx->GetMetric()) {
	    units = /*m*/424;
	} else {
	    d_horiz /= METRES_PER_FOOT;
	    dr /= METRES_PER_FOOT;
	    dz /= METRES_PER_FOOT;
	    units = /*ft*/428;
	}
	wxString len_unit = wmsg(units);
	/* TRANSLATORS: "H" is short for "Horizontal", "V" for "Vertical" */
	hv_str.Printf(wmsg(/*H %.2f%s, V %.2f%s*/340),
		      d_horiz, len_unit.c_str(), dz, len_unit.c_str());
	int brg_unit;
	if (m_Gfx->GetDegrees()) {
	    brg_unit = /*°*/344;
	} else {
	    brg *= 400.0 / 360.0;
	    brg_unit = /*grad*/345;
	}
	wxString & d = dist_text;
	/* TRANSLATORS: "Dist" is short for "Distance", "Brg" for "Bearing" (as
	 * in Compass bearing) */
	d.Printf(wmsg(/*%s: %s, Dist %.2f%s, Brg %03d%s*/341),
		 from_str.c_str(), hv_str.c_str(),
		 dr, len_unit.c_str(), int(brg), wmsg(brg_unit).c_str());
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
	m_Gfx->SetHereFromTree(data->GetLabel());
    } else {
	ShowInfo();
    }
}

void MainFrm::TreeItemSelected(const wxTreeItemData* item, bool zoom)
{
    const TreeData* data = static_cast<const TreeData*>(item);
    if (data && data->IsStation()) {
	const LabelInfo* label = data->GetLabel();
	if (zoom) m_Gfx->CentreOn(*label);
	m_Gfx->SetThere(label);
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
    wxFileDialog dlg(this, wmsg(/*Export Movie*/331), wxString(),
		     baseleaf + wxT(".mpg"),
		     wxT("MPEG|*.mpg|AVI|*.avi|QuickTime|*.mov|WMV|*.wmv;*.asf"),
		     wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() == wxID_OK) {
	// Error is reported by GfxCore.
	(void)m_Gfx->ExportMovie(dlg.GetPath());
    }
}

PresentationMark MainFrm::GetPresMark(int which)
{
    return m_PresList->GetPresMark(which);
}

void MainFrm::OnOpenTerrainUpdate(wxUpdateUIEvent& event)
{
    event.Enable(!m_File.empty());
}

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
    pending_find = true;
}

void MainFrm::OnIdle(wxIdleEvent&)
{
    if (pending_find) {
	DoFind();
    }
}

void MainFrm::DoFind()
{
    pending_find = false;
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
	/* TRANSLATORS: "Hide stations" button tooltip when stations are found
	 */
	GetToolBar()->SetToolShortHelp(button_HIDE, wxString::Format(wmsg(/*Hide %d found stations*/334).c_str(), m_NumHighlighted));
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
    GetToolBar()->SetToolShortHelp(button_HIDE, wmsg(/*Hide*/333));
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
#ifdef __WXMAC__
    // On OS X, wxWidgets doesn't currently hide the toolbar or statusbar in
    // full screen mode (last checked with 3.0.2), but it is easy to do
    // ourselves.
    if (!IsFullScreen()) {
	GetToolBar()->Hide();
	GetStatusBar()->Hide();
    }
#endif

    ShowFullScreen(!IsFullScreen());
    fullscreen_showing_menus = false;
    if (IsFullScreen())
	was_showing_sidepanel_before_fullscreen = ShowingSidePanel();
    if (was_showing_sidepanel_before_fullscreen)
	ToggleSidePanel();

#ifdef __WXMAC__
    if (!IsFullScreen()) {
	GetStatusBar()->Show();
	GetToolBar()->Show();
#ifdef USING_GENERIC_TOOLBAR
	Layout();
#endif
    }
#endif
}

bool MainFrm::FullScreenModeShowingMenus() const
{
    return fullscreen_showing_menus;
}

void MainFrm::FullScreenModeShowMenus(bool show)
{
    if (!IsFullScreen() || show == fullscreen_showing_menus)
	return;
#ifdef __WXMAC__
    // On OS X, enabling the menu bar while in full
    // screen mode doesn't have any effect, so instead
    // make moving the mouse to the top of the screen
    // drop us out of full screen mode for now.
    ViewFullScreen();
#else
    GetMenuBar()->Show(show);
    fullscreen_showing_menus = show;
#endif
}
