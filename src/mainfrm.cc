//
//  mainfrm.cc
//
//  Main frame handling for Aven.
//
//  Copyright (C) 2000-2002 Mark R. Shinwell
//  Copyright (C) 2001-2004 Olly Betts
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
#include "filename.h"
#include "useful.h"

#include <wx/confbase.h>
#include <wx/image.h>

#include <float.h>
#include <functional>
#include <stack>

#ifdef HAVE_REGEX_H
# include <regex.h>
#else
extern "C" {
# ifdef HAVE_RXPOSIX_H
#  include <rxposix.h>
# elif defined(HAVE_RX_RXPOSIX_H)
#  include <rx/rxposix.h>
# elif !defined(REG_NOSUB)
// we must be using wxWindows built-in regexp functions
#  define REG_NOSUB wxRE_NOSUB
#  define REG_ICASE wxRE_ICASE
#  define REG_EXTENDED wxRE_EXTENDED
# endif
}
#endif

#define TOOLBAR_BITMAP(file) wxBitmap(wxString(msg_cfgpth()) + \
    wxCONFIG_PATH_SEPARATOR + wxString("icons") + wxCONFIG_PATH_SEPARATOR + \
    wxString(file), wxBITMAP_TYPE_PNG)

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
#if wxMAJOR_VERSION > 2 || (wxMAJOR_VERSION == 2 && wxMINOR_VERSION >= 3)
	    e.Veto();
#endif
#if defined(__UNIX__) && wxMAJOR_VERSION == 2 && wxMINOR_VERSION == 3 && wxRELEASE_NUMBER <= 4
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

// Write a value without trailing zeros after the decimal point.
static void write_double(double d, FILE * fh) {
    char buf[64];
    sprintf(buf, "%.21f", d);
    size_t l = strlen(buf);
    while (l > 1 && buf[l - 1] == '0') --l;
    if (l > 1 && buf[l - 1] == '.') --l;
    fwrite(buf, l, 1, fh);
}

class AvenListCtrl: public wxListCtrl {
    GfxCore * gfx;
    list<PresentationMark> entries;
    long current_item;
    bool modified;
    wxString filename;

    public:
	AvenListCtrl(wxWindow * parent, GfxCore * gfx_)
	    : wxListCtrl(parent, listctrl_PRES, wxDefaultPosition, wxDefaultSize,
			 wxLC_EDIT_LABELS | wxLC_REPORT),
	      gfx(gfx_), current_item(-1), modified(false)
	    {
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
	    list<PresentationMark>::iterator i = entries.begin();
	    while (item > 0) {
		++i;
		if (i == entries.end()) return;
		--item;
	    }
	    entries.erase(i);
	    modified = true;
	}
	void OnDeleteAllItems(wxListEvent& event) {
	    entries.clear();
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
		case WXK_INSERT:
		    AddMark(event.GetIndex());
		    break;
		default:
		    event.Skip();
	    }
	}
	void OnActivated(wxListEvent& event) {
	    // Jump to this view.
	    long item = event.GetIndex();
	    list<PresentationMark>::const_iterator i = entries.begin();
	    while (item > 0) {
		++i;
		if (i == entries.end()) return;
		--item;
	    }
	    gfx->SetView(*i);
	}
	void OnFocused(wxListEvent& event) {
	    current_item = event.GetIndex();
	}
	void OnChar(wxKeyEvent& event) {
	    gfx->OnKeyPress(event);
	}
	void AddMark(long item = -1) {
	    AddMark(item, gfx->GetView());
	}
	void AddMark(long item, const PresentationMark & mark) {
	    list<PresentationMark>::iterator i;
	    if (item == -1) {
		item = entries.size();
		i = entries.end();
	    } else {
		long c = item;
		i = entries.begin();
		while (c > 0) {
		    if (i == entries.end()) return;
		    ++i;
		    --c;
		}
	    }
	    entries.insert(i, mark);
	    long tmp = InsertItem(item, wxString::Format("%ld", (long)mark.y));
	    SetItemData(tmp, item);
	    SetItem(item, 1, wxString::Format("%ld", (long)mark.x));
	    modified = true;
	}
	void Save(bool use_default_name) {
	    wxString fnm = filename;
	    if (!use_default_name || fnm.empty()) {
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
	    list<PresentationMark>::const_iterator i;
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
		    double x, y, z, a, t, s;
		    if (sscanf(buf, "%lf %lf %lf %lf %lf %lf", &x, &y, &z, &a, &t, &s) != 6) {
			DeleteAllItems();
			wxGetApp().ReportError(wxString::Format(msg(/*Error in format of presentation file `%s'*/323), fnm.c_str()));
			return false;
		    }
		    AddMark(item, PresentationMark(x, y, z, a, t, s));
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
	    list<PresentationMark>::iterator i = entries.begin();
	    long item = current_item;
	    if (which == MARK_FIRST) {
		item = 0;
	    } else if (which == MARK_NEXT) {
		long c = ++item;
		while (c > 0) {
		    if (i == entries.end()) break;
		    ++i;
		    --c;
		}
	    }
	    if (item != current_item) {
		// Move the focus
		if (current_item != -1) {
		    wxListCtrl::SetItemState(current_item, wxLIST_STATE_FOCUSED,
					     0);
		}
		wxListCtrl::SetItemState(item, wxLIST_STATE_FOCUSED,
					 wxLIST_STATE_FOCUSED);
	    }
	    if (i == entries.end()) return PresentationMark();
	    return *i;
	}

    private:

	DECLARE_NO_COPY_CLASS(AvenListCtrl)
	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(AvenListCtrl, wxListCtrl)
    EVT_LIST_BEGIN_LABEL_EDIT(listctrl_PRES, AvenListCtrl::OnBeginLabelEdit)
    EVT_LIST_DELETE_ITEM(listctrl_PRES, AvenListCtrl::OnDeleteItem)
    EVT_LIST_DELETE_ALL_ITEMS(listctrl_PRES, AvenListCtrl::OnDeleteAllItems)
    EVT_LIST_KEY_DOWN(listctrl_PRES, AvenListCtrl::OnListKeyDown)
    EVT_LIST_ITEM_ACTIVATED(listctrl_PRES, AvenListCtrl::OnActivated)
    EVT_LIST_ITEM_FOCUSED(listctrl_PRES, AvenListCtrl::OnFocused)
    EVT_CHAR(AvenListCtrl::OnChar)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(MainFrm, wxFrame)
    EVT_TEXT_ENTER(textctrl_FIND, MainFrm::OnFind)
    EVT_MENU(button_FIND, MainFrm::OnFind)
    EVT_MENU(button_HIDE, MainFrm::OnHide)
    EVT_UPDATE_UI(button_HIDE, MainFrm::OnHideUpdate)

    EVT_MENU(menu_FILE_OPEN, MainFrm::OnOpen)
    EVT_MENU(menu_FILE_SCREENSHOT, MainFrm::OnScreenshot)
//    EVT_MENU(menu_FILE_PREFERENCES, MainFrm::OnFilePreferences)
    EVT_MENU(menu_FILE_QUIT, MainFrm::OnQuit)
    EVT_MENU_RANGE(wxID_FILE1, wxID_FILE9, MainFrm::OnMRUFile)

    EVT_MENU(menu_PRES_NEW, MainFrm::OnPresNew)
    EVT_MENU(menu_PRES_OPEN, MainFrm::OnPresOpen)
    EVT_MENU(menu_PRES_SAVE, MainFrm::OnPresSave)
    EVT_MENU(menu_PRES_SAVE_AS, MainFrm::OnPresSaveAs)
    EVT_MENU(menu_PRES_MARK, MainFrm::OnPresMark)
    EVT_MENU(menu_PRES_RUN, MainFrm::OnPresRun)
    EVT_MENU(menu_PRES_EXPORT_MOVIE, MainFrm::OnPresExportMovie)

    EVT_UPDATE_UI(menu_PRES_NEW, MainFrm::OnPresNewUpdate)
    EVT_UPDATE_UI(menu_PRES_OPEN, MainFrm::OnPresOpenUpdate)
    EVT_UPDATE_UI(menu_PRES_SAVE, MainFrm::OnPresSaveUpdate)
    EVT_UPDATE_UI(menu_PRES_SAVE_AS, MainFrm::OnPresSaveAsUpdate)
    EVT_UPDATE_UI(menu_PRES_MARK, MainFrm::OnPresMarkUpdate)
    EVT_UPDATE_UI(menu_PRES_RUN, MainFrm::OnPresRunUpdate)
    EVT_UPDATE_UI(menu_PRES_EXPORT_MOVIE, MainFrm::OnPresExportMovieUpdate)

    EVT_CLOSE(MainFrm::OnClose)
    EVT_SET_FOCUS(MainFrm::OnSetFocus)

    EVT_MENU(menu_ROTATION_START, MainFrm::OnStartRotation)
    EVT_MENU(menu_ROTATION_TOGGLE, MainFrm::OnToggleRotation)
    EVT_MENU(menu_ROTATION_STOP, MainFrm::OnStopRotation)
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
    EVT_MENU(menu_VIEW_PERSPECTIVE, MainFrm::OnViewPerspective)
    EVT_MENU(menu_VIEW_TEXTURED, MainFrm::OnViewTextured)
    EVT_MENU(menu_VIEW_FOG, MainFrm::OnViewFog)
    EVT_MENU(menu_VIEW_SMOOTH_LINES, MainFrm::OnViewSmoothLines)
    EVT_MENU(menu_VIEW_FULLSCREEN, MainFrm::OnViewFullScreen)
#ifdef AVENGL
    EVT_MENU(menu_VIEW_SHOW_TUBES, MainFrm::OnToggleTubes)
#endif
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

    EVT_UPDATE_UI(menu_FILE_SCREENSHOT, MainFrm::OnScreenshotUpdate)
    EVT_UPDATE_UI(menu_ROTATION_START, MainFrm::OnStartRotationUpdate)
    EVT_UPDATE_UI(menu_ROTATION_TOGGLE, MainFrm::OnToggleRotationUpdate)
    EVT_UPDATE_UI(menu_ROTATION_STOP, MainFrm::OnStopRotationUpdate)
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
    EVT_UPDATE_UI(menu_VIEW_PERSPECTIVE, MainFrm::OnViewPerspectiveUpdate)
    EVT_UPDATE_UI(menu_VIEW_TEXTURED, MainFrm::OnViewTexturedUpdate)
    EVT_UPDATE_UI(menu_VIEW_FOG, MainFrm::OnViewFogUpdate)
    EVT_UPDATE_UI(menu_VIEW_SMOOTH_LINES, MainFrm::OnViewSmoothLinesUpdate)
    EVT_UPDATE_UI(menu_VIEW_FULLSCREEN, MainFrm::OnViewFullScreenUpdate)
#ifdef AVENGL
    EVT_UPDATE_UI(menu_VIEW_SHOW_TUBES, MainFrm::OnToggleTubesUpdate)
#endif
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
#ifdef _WIN32
    // The peculiar name is so that the icon is the first in the file
    // (required by Microsoft Windows for this type of icon)
    SetIcon(wxIcon("aaaaaAven"));
#endif

    CreateMenuBar();
    CreateToolBar();
    CreateStatusBar(3, wxST_SIZEGRIP);
    CreateSidePanel();

    int widths[3] = { 150, -1 /* variable width */, -1 };
    GetStatusBar()->SetStatusWidths(3, widths);

#ifdef __X__
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
    filemenu->Append(menu_FILE_SCREENSHOT, GetTabMsg(/*@Screenshot...*/201));
    filemenu->AppendSeparator();
    filemenu->Append(menu_FILE_QUIT, GetTabMsg(/*E@xit*/221));

    m_history.UseMenu(filemenu);
    m_history.Load(*wxConfigBase::Get());

    wxMenu* rotmenu = new wxMenu;
    rotmenu->Append(menu_ROTATION_START, GetTabMsg(/*@Start Rotation##Return*/230));
    rotmenu->Append(menu_ROTATION_STOP, GetTabMsg(/*S@top Rotation##Space*/231));
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
    orientmenu->Append(menu_ORIENT_DEFAULTS, GetTabMsg(/*Restore De@fault Settings*/254));

    wxMenu* presmenu = new wxMenu;
    presmenu->Append(menu_PRES_NEW, GetTabMsg(/*@New Presentation*/311));
    presmenu->Append(menu_PRES_OPEN, GetTabMsg(/*@Open Presentation...*/312));
    presmenu->Append(menu_PRES_SAVE, GetTabMsg(/*@Save Presentation*/313));
    presmenu->Append(menu_PRES_SAVE_AS, GetTabMsg(/*Save Presentation @As...*/314));
    presmenu->AppendSeparator();
    presmenu->Append(menu_PRES_MARK, GetTabMsg(/*@Mark*/315));
    presmenu->Append(menu_PRES_RUN, GetTabMsg(/*@Play*/316));
    presmenu->Append(menu_PRES_EXPORT_MOVIE, GetTabMsg(/*@Export as Movie...*/317));

    wxMenu* viewmenu = new wxMenu;
#ifndef PREFDLG
    viewmenu->Append(menu_VIEW_SHOW_NAMES, GetTabMsg(/*Station @Names##Ctrl+N*/270), "", true);
#ifdef AVENGL
    viewmenu->Append(menu_VIEW_SHOW_TUBES, GetTabMsg(/*Passage @Tubes*/346), "", true);
#endif
    viewmenu->Append(menu_VIEW_SHOW_CROSSES, GetTabMsg(/*@Crosses##Ctrl+X*/271), "", true);
    viewmenu->Append(menu_VIEW_GRID, GetTabMsg(/*@Grid##Ctrl+G*/297), "", true);
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

void MainFrm::CreateToolBar()
{
    // Create the toolbar.

    wxToolBar* toolbar = wxFrame::CreateToolBar();

#ifndef _WIN32
    toolbar->SetMargins(5, 5);
#endif

    // FIXME: TRANSLATE tooltips
    toolbar->AddTool(menu_FILE_OPEN, TOOLBAR_BITMAP("open.png"), "Open a 3D file for viewing");
    toolbar->AddTool(menu_PRES_OPEN, TOOLBAR_BITMAP("open-pres.png"), "Open a presentation");
    toolbar->AddSeparator();
    toolbar->AddTool(menu_ROTATION_TOGGLE, TOOLBAR_BITMAP("rotation.png"),
		     wxNullBitmap, true, -1, -1, NULL, "Toggle rotation");
    toolbar->AddSeparator();
    toolbar->AddTool(menu_ORIENT_PLAN, TOOLBAR_BITMAP("plan.png"), "Switch to plan view");
    toolbar->AddTool(menu_ORIENT_ELEVATION, TOOLBAR_BITMAP("elevation.png"), "Switch to elevation view");
    toolbar->AddSeparator();
    toolbar->AddTool(menu_ORIENT_DEFAULTS, TOOLBAR_BITMAP("defaults.png"), "Restore default view");
    toolbar->AddSeparator();
    toolbar->AddTool(menu_VIEW_SHOW_NAMES, TOOLBAR_BITMAP("names.png"), wxNullBitmap, true,
		     -1, -1, NULL, "Show station names");
    toolbar->AddTool(menu_VIEW_SHOW_CROSSES, TOOLBAR_BITMAP("crosses.png"), wxNullBitmap, true,
		     -1, -1, NULL, "Show crosses on stations");
    toolbar->AddTool(menu_VIEW_SHOW_ENTRANCES, TOOLBAR_BITMAP("entrances.png"), wxNullBitmap, true,
		     -1, -1, NULL, "Highlight entrances");
    toolbar->AddTool(menu_VIEW_SHOW_FIXED_PTS, TOOLBAR_BITMAP("fixed-pts.png"), wxNullBitmap, true,
		     -1, -1, NULL, "Highlight fixed points");
    toolbar->AddTool(menu_VIEW_SHOW_EXPORTED_PTS, TOOLBAR_BITMAP("exported-pts.png"), wxNullBitmap, true,
		     -1, -1, NULL, "Highlight exported stations");
    toolbar->AddSeparator();
    toolbar->AddTool(menu_VIEW_SHOW_LEGS, TOOLBAR_BITMAP("ug-legs.png"), wxNullBitmap, true,
		     -1, -1, NULL, "Show underground surveys");
    toolbar->AddTool(menu_VIEW_SHOW_SURFACE, TOOLBAR_BITMAP("surface-legs.png"), wxNullBitmap, true,
		     -1, -1, NULL, "Show surface surveys");
#ifdef AVENGL
    toolbar->AddSeparator();
    toolbar->AddTool(menu_VIEW_SHOW_TUBES, TOOLBAR_BITMAP("tubes.png"), wxNullBitmap, true,
		     -1, -1, NULL, "Show passage tubes");
#endif

    toolbar->AddSeparator();
    m_FindBox = new wxTextCtrl(toolbar, textctrl_FIND, "", wxDefaultPosition,
			       wxDefaultSize, wxTE_PROCESS_ENTER);
    toolbar->AddControl(m_FindBox);
    toolbar->AddTool(button_FIND, TOOLBAR_BITMAP("find.png"),
		     msg(/*Find*/332)/*"Search for station name"*/);
    toolbar->AddTool(button_HIDE, TOOLBAR_BITMAP("hideresults.png"),
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

    m_PresList = new AvenListCtrl(m_PresPanel, m_Gfx);
    // FIXME swap order of Northing and Easting (here *and* elsewhere)
    m_PresList->InsertColumn(0, msg(/*Northing*/379));
    m_PresList->InsertColumn(1, msg(/*Easting*/378));
//    m_PresList->SetColumnWidth(0, 100);
  //  m_PresList->SetColumnWidth(1, 40);
    //m_PresList->SetColumnWidth(2, 40);

    wxBoxSizer *pres_panel_sizer = new wxBoxSizer(wxVERTICAL);
    pres_panel_sizer->Add(m_PresList, 1, wxALL | wxEXPAND, 2);
    m_PresPanel->SetAutoLayout(true);
    m_PresPanel->SetSizer(pres_panel_sizer);

    // Overall tabbed structure:
    wxImageList* image_list = new wxImageList();
    image_list->Add(wxGetApp().LoadIcon("survey-tree"));
    image_list->Add(wxGetApp().LoadIcon("pres-tree"));
    m_Notebook->SetImageList(image_list);
    m_Notebook->AddPage(m_Panel, msg(/*Tree*/376), true, 0);
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

#if 0
    Splash* splash = wxGetApp().GetSplashScreen();
    long file_size;
    {
	long pos = ftell(survey->fh);
	fseek(survey->fh, 0, SEEK_END);
	file_size = ftell(survey->fh);
	fseek(survey->fh, pos, SEEK_SET);
    }
#endif

    m_File = file;

    m_Tree->DeleteAllItems();

    m_TreeRoot = m_Tree->AddRoot(wxFileNameFromPath(file));
    m_Tree->SetEnabled();

    // Create a list of all the leg vertices, counting them and finding the
    // extent of the survey at the same time.

    m_NumPoints = 0;
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

    points.clear();

    int result;
    int items = 0;
    do {
#if 0
	if (splash && (items % 200 == 0)) {
	    long pos = ftell(survey->fh);
	    int progress = int((double(pos) / double(file_size)) * 100.0);
	    splash->SetProgress(progress);
	}
#endif

	img_point pt;
	result = img_read_item(survey, &pt);
	items++;
	switch (result) {
	    case img_MOVE:
	    case img_LINE:
	    {
		m_NumPoints++;

		// Update survey extents.
		if (pt.x < xmin) xmin = pt.x;
		if (pt.x > xmax) xmax = pt.x;
		if (pt.y < ymin) ymin = pt.y;
		if (pt.y > ymax) ymax = pt.y;
		if (pt.z < m_ZMin) m_ZMin = pt.z;
		if (pt.z > zmax) zmax = pt.z;

		bool is_surface = false;
		if (result == img_LINE) {
		    // Set flags to say this is a line rather than a move
		    is_surface = (survey->flags & img_FLAG_SURFACE);
		    if (is_surface) {
			m_HasSurfaceLegs = true;
		    } else {
			m_HasUndergroundLegs = true;
		    }
		}

		// Add this point in the list.
		points.push_back(PointInfo(pt.x, pt.y, pt.z, (result == img_LINE), is_surface));
		break;
	    }

	    case img_LABEL:
	    {
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

	    case img_BAD:
	    {
		m_Labels.clear();

		// FIXME: Do we need to reset all these? - Olly
		m_NumPoints = 0;
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

    separator = survey->separator;
    img_close(survey);

    // Check we've actually loaded some legs or stations!
    if (!m_HasUndergroundLegs && !m_HasSurfaceLegs && m_Labels.empty()) {
	wxString m = wxString::Format(msg(/*No survey data in 3d file `%s'*/202), file.c_str());
	wxGetApp().ReportError(m);
	return false;
    }

    if (points.empty()) {
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
    } else {
	// Delete any trailing move.
	if (!points.back().isLine) {
	    m_NumPoints--;
	    points.pop_back();
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
    SetTitle(wxString("Aven - [") + file + wxString("]"));
    m_File = file;

    return true;
}

void MainFrm::FillTree()
{
    // Fill the tree of stations and prefixes.

    list<LabelInfo*>::iterator pos = m_Labels.begin();
    stack<wxTreeItemId> previous_ids;
    wxString current_prefix = "";
    wxTreeItemId current_id = m_TreeRoot;

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
		while (1) {
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

    if (label->IsSurface()) {
	m_Tree->SetItemTextColour(label->tree_id, wxColour(49, 158, 79));
    }

    if (label->IsEntrance()) {
	// FIXME: making this red here doesn't match with entrance blobs
	// being green...
	m_Tree->SetItemTextColour(label->tree_id, wxColour(255, 0, 0));
    }
}

void MainFrm::CentreDataset(Double xmin, Double ymin, Double zmin)
{
    // Centre the dataset around the origin.

    Double xoff = m_Offsets.x = xmin + (m_XExt / 2.0);
    Double yoff = m_Offsets.y = ymin + (m_YExt / 2.0);
    Double zoff = m_Offsets.z = zmin + (m_ZExt / 2.0);

    list<PointInfo>::iterator pos = points.begin();
    while (pos != points.end()) {
	PointInfo & point = *pos++;
	point.x -= xoff;
	point.y -= yoff;
	point.z -= zoff;
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
#if 0
    Splash* splash = wxGetApp().GetSplashScreen();

    if (splash) {
	splash->SetProgress(0);
    }
#endif
    if (LoadData(file, survey)) {
	if (wxIsAbsolutePath(file)) {
	    m_history.AddFileToHistory(file);
	} else {
	    wxString abs = wxGetCwd() + wxString(FNM_SEP_LEV) + file;
	    m_history.AddFileToHistory(abs);
	}
	wxConfigBase *b = wxConfigBase::Get();
	m_history.Save(*b);
	b->Flush();

	m_Gfx->Initialise();

	m_Notebook->Show(true);

	int x;
	int y;
	GetSize(&x, &y);
	if (x < 600)
	    x /= 3;
	else if (x < 1000)
	    x = 200;
	else
	    x /= 5;

	m_Splitter->SplitVertically(m_Notebook, m_Gfx, x);

	m_SashPosition = m_Splitter->GetSashPosition(); // save width of panel

	m_Gfx->SetFocus();
    }
}

//
//  UI event handlers
//

#undef FILEDIALOG_MULTIGLOBS
// MS Windows supports "*.abc;*.def" natively; wxGtk supports them as of 2.3
#if defined(_WIN32) || wxMAJOR_VERSION > 2 || (wxMAJOR_VERSION == 2 && wxMINOR_VERSION >= 3)
# define FILEDIALOG_MULTIGLOBS
#endif

void MainFrm::OnOpen(wxCommandEvent&)
{
#ifdef __WXMOTIF__
    wxFileDialog dlg (this, wxString(msg(/*Select a 3d file to view*/206)), "", "",
		      "*.3d", wxOPEN);
#else
    wxFileDialog dlg (this, wxString(msg(/*Select a 3d file to view*/206)), "", "",
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
				       wxFileSelectorDefaultWildcardStr
				       ), wxOPEN);
#endif
    if (dlg.ShowModal() == wxID_OK) {
	OpenFile(dlg.GetPath());
    }
}

void MainFrm::OnScreenshot(wxCommandEvent&)
{
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

void MainFrm::OnQuit(wxCommandEvent&)
{
    if (m_PresList->Modified()) {
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
    wxDialog* dlg = new AboutDlg(this);
    dlg->Centre();
    dlg->ShowModal();
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
	    m_Gfx->SetThere(); // FIXME: not in SetMouseOverStation version?
	}
    }
}

void MainFrm::DisplayTreeInfo(const wxTreeItemData* item)
{
    const TreeData* data = static_cast<const TreeData*>(item);
    if (data) {
	if (data->IsStation()) {
	    ShowInfo(data->GetLabel());
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
    }

    // ClearCoords(); // FIXME or update or ?
}

void MainFrm::OnPresNew(wxCommandEvent&)
{
    if (m_PresList->Modified()) {
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

void MainFrm::OnPresRun(wxCommandEvent&)
{
    m_Gfx->PlayPres();
}

void MainFrm::OnPresExportMovie(wxCommandEvent&)
{
    // FIXME : Taking the leaf of the currently loaded presentation as the
    // default might make more sense?
    char *baseleaf = baseleaf_from_fnm(m_File.c_str());
    wxFileDialog dlg (this, wxString("Export Movie"), "",
		      wxString(baseleaf) + ".mpg",
		      "*.mpg", wxSAVE|wxOVERWRITE_PROMPT);
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
    event.Enable(!m_PresList->Empty());
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

void MainFrm::OnPresRunUpdate(wxUpdateUIEvent& event)
{
    event.Enable(!m_PresList->Empty());
}

void MainFrm::OnPresExportMovieUpdate(wxUpdateUIEvent& event)
{
    event.Enable(!m_PresList->Empty());
}

void MainFrm::OnFind(wxCommandEvent&)
{
    wxBusyCursor hourglass;
    // Find stations specified by a string or regular expression.

    wxString str = m_FindBox->GetValue();
    int cflags = REG_NOSUB;

    if (true /* case insensitive */) {
	cflags |= REG_ICASE;
    }

    if (false /*m_RegexpCheckBox->GetValue()*/) {
	cflags |= REG_EXTENDED;
    } else if (true /* simple glob-style */) {
	wxString pat;
	for (size_t i = 0; i < str.size(); i++) {
	   char ch = str[i];
	   // ^ only special at start; $ at end.  But this is simpler...
	   switch (ch) {
	    case '^': case '$': case '.': case '[': case '\\':
	      pat += '\\';
	      pat += ch;
	      break;
	    case '*':
	      pat += ".*";
	      break;
	    case '?':
	      pat += '.';
	      break;
	    default:
	      pat += ch;
	   }
	}
	str = pat;
    } else {
	wxString pat;
	for (size_t i = 0; i < str.size(); i++) {
	   char ch = str[i];
	   // ^ only special at start; $ at end.  But this is simpler...
	   switch (ch) {
	    case '^': case '$': case '*': case '.': case '[': case '\\':
	      pat += '\\';
	   }
	   pat += ch;
	}
	str = pat;
    }

    if (!true /* !substring */) {
	// FIXME "0u" required to avoid compilation error with g++-3.0
	if (str.empty() || str[0u] != '^') str = '^' + str;
	if (str[str.size() - 1] != '$') str = str + '$';
    }

    regex_t buffer;
    int errcode = regcomp(&buffer, str.c_str(), cflags);
    if (errcode) {
	size_t len = regerror(errcode, &buffer, NULL, 0);
	char *msg = new char[len];
	regerror(errcode, &buffer, msg, len);
	wxGetApp().ReportError(msg);
	delete[] msg;
	return;
    }

    list<LabelInfo*>::iterator pos = m_Labels.begin();

    int found = 0;
    while (pos != m_Labels.end()) {
	LabelInfo* label = *pos++;

	if (regexec(&buffer, label->text.c_str(), 0, NULL, 0) == 0) {
	    label->flags |= LFLAG_HIGHLIGHTED;
	    found++;
	} else {
	    label->flags &= ~LFLAG_HIGHLIGHTED;
	}
    }

    regfree(&buffer);

    m_NumHighlighted = found;
// FIXME:    m_Found->SetLabel(wxString::Format(msg(/*%d found*/331), found));
#ifdef _WIN32
//    m_Found->Refresh(); // FIXME
#endif
    // Re-sort so highlighted points get names in preference
    if (found) m_Labels.sort(LabelPlotCmp(separator));
    m_Gfx->UpdateBlobs();
    m_Gfx->ForceRefresh();

#if 0
    if (!found) {
	wxGetApp().ReportError(msg(/*No matches were found.*/328));
    }
#endif

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
