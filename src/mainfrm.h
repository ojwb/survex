//
//  mainfrm.h
//
//  Main frame handling for Aven.
//
//  Copyright (C) 2000-2003 Mark R. Shinwell
//  Copyright (C) 2001-2004,2005 Olly Betts
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

#ifndef mainfrm_h
#define mainfrm_h

#include "wx.h"
#include <wx/docview.h> // for m_FileHistory
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <wx/print.h>
#include <wx/printdlg.h>

#include "gfxcore.h"
#include "message.h"
#include "aventreectrl.h"
#include "img.h"
#include "guicontrol.h"
//#include "prefsdlg.h"

#include <list>
#include <vector>

using namespace std;

#include <math.h>

#define MARK_FIRST 1
#define MARK_NEXT 2
#define MARK_PREV 3

enum {
    menu_FILE_OPEN = 1000,
    menu_FILE_PRINT,
    menu_FILE_PAGE_SETUP,
    menu_FILE_SCREENSHOT,
    menu_FILE_EXPORT,
    menu_FILE_QUIT,
    menu_PRES_NEW,
    menu_PRES_OPEN,
    menu_PRES_SAVE,
    menu_PRES_SAVE_AS,
    menu_PRES_MARK,
    menu_PRES_EXPORT_MOVIE,
    menu_PRES_FREWIND,
    menu_PRES_REWIND,
    menu_PRES_REVERSE,
    menu_PRES_PLAY,
    menu_PRES_FF,
    menu_PRES_FFF,
    menu_PRES_PAUSE,
    menu_PRES_STOP,
    menu_ROTATION_START,
    menu_ROTATION_STOP,
    menu_ROTATION_TOGGLE,
    menu_ROTATION_SPEED_UP,
    menu_ROTATION_SLOW_DOWN,
    menu_ROTATION_REVERSE,
    menu_ROTATION_STEP_CCW,
    menu_ROTATION_STEP_CW,
    menu_ORIENT_MOVE_NORTH,
    menu_ORIENT_MOVE_EAST,
    menu_ORIENT_MOVE_SOUTH,
    menu_ORIENT_MOVE_WEST,
    menu_ORIENT_SHIFT_LEFT,
    menu_ORIENT_SHIFT_RIGHT,
    menu_ORIENT_SHIFT_UP,
    menu_ORIENT_SHIFT_DOWN,
    menu_ORIENT_PLAN,
    menu_ORIENT_ELEVATION,
    menu_ORIENT_HIGHER_VP,
    menu_ORIENT_LOWER_VP,
    menu_ORIENT_ZOOM_IN,
    menu_ORIENT_ZOOM_OUT,
    menu_ORIENT_DEFAULTS,
    menu_VIEW_SHOW_LEGS,
    menu_VIEW_SHOW_CROSSES,
    menu_VIEW_SHOW_NAMES,
    menu_VIEW_SHOW_SURFACE,
    menu_VIEW_SHOW_OVERLAPPING_NAMES,
    menu_VIEW_SHOW_ENTRANCES,
    menu_VIEW_SHOW_FIXED_PTS,
    menu_VIEW_SHOW_EXPORTED_PTS,
    menu_VIEW_STATUS_BAR,
    menu_VIEW_GRID,
    menu_VIEW_SHOW_TUBES,
    menu_VIEW_PERSPECTIVE,
    menu_VIEW_TEXTURED,
    menu_VIEW_FOG,
    menu_VIEW_SMOOTH_LINES,
    menu_VIEW_FULLSCREEN,
    menu_VIEW_PREFERENCES,
    menu_VIEW_COLOUR_BY_DEPTH,
    menu_IND_COMPASS,
    menu_IND_CLINO,
    menu_IND_DEPTH_BAR,
    menu_IND_SCALE_BAR,
    menu_CTL_INDICATORS,
    menu_CTL_SIDE_PANEL,
    menu_CTL_METRIC,
    menu_CTL_DEGREES,
    menu_CTL_REVERSE,
    menu_CTL_CANCEL_DIST_LINE,
    menu_HELP_ABOUT,
    textctrl_FIND,
    button_FIND,
    button_HIDE,
    listctrl_PRES
};

class PointInfo {
    friend class MainFrm;
    Double x, y, z;
    Double l, r, u, d;

public:
    PointInfo() : x(0), y(0), z(0), l(0), r(0), u(0), d(0) { }
    PointInfo(const img_point & pt, Double l_, Double r_, Double u_, Double d_)
	: x(pt.x), y(pt.y), z(pt.z), l(l_), r(r_), u(u_), d(d_) { }
    PointInfo(const img_point & pt)
	: x(pt.x), y(pt.y), z(pt.z), l(0), r(0), u(0), d(0) { }
    Double GetX() const { return x; }
    Double GetY() const { return y; }
    Double GetZ() const { return z; }
    Double GetL() const { return l; }
    Double GetR() const { return r; }
    Double GetU() const { return u; }
    Double GetD() const { return d; }
    Vector3 vec() const { return Vector3(x, y, z); }
};

#define LFLAG_SURFACE		img_SFLAG_SURFACE
#define LFLAG_UNDERGROUND	img_SFLAG_UNDERGROUND
#define LFLAG_EXPORTED		img_SFLAG_EXPORTED
#define LFLAG_FIXED		img_SFLAG_FIXED
#define LFLAG_ENTRANCE		0x100
#define LFLAG_HIGHLIGHTED	0x200

class LabelPlotCmp;
class AvenPresList;

class LabelInfo {
    friend class MainFrm;
    friend class GfxCore;
    friend class LabelPlotCmp;
    Double x, y, z;
    wxString text;
    unsigned width;
    int flags;
    wxTreeItemId tree_id;

public:
    Double GetX() const { return x; }
    Double GetY() const { return y; }
    Double GetZ() const { return z; }

    wxString GetText() const { return text; }

    bool IsEntrance() const { return (flags & LFLAG_ENTRANCE) != 0; }
    bool IsFixedPt() const { return (flags & LFLAG_FIXED) != 0; }
    bool IsExportedPt() const { return (flags & LFLAG_EXPORTED) != 0; }
    bool IsUnderground() const { return (flags & LFLAG_UNDERGROUND) != 0; }
    bool IsSurface() const { return (flags & LFLAG_SURFACE) != 0; }
    bool IsHighLighted() const { return (flags & LFLAG_HIGHLIGHTED) != 0; }
};

class MainFrm : public wxFrame {
    wxFileHistory m_history;
    int m_SashPosition;
    list<vector<PointInfo> > traverses;
    list<vector<PointInfo> > surface_traverses;
    list<LabelInfo*> m_Labels;
    Double m_XExt;
    Double m_YExt;
    Double m_ZExt;
    Double m_ZMin;
    int m_NumCrosses;
    GfxCore* m_Gfx;
    GUIControl* m_Control;
    int m_NumEntrances;
    int m_NumFixedPts;
    int m_NumExportedPts;
    int m_NumHighlighted;
    bool m_HasUndergroundLegs;
    bool m_HasSurfaceLegs;
    wxSplitterWindow* m_Splitter;
    wxPanel* m_Panel;
    AvenTreeCtrl* m_Tree;
    wxTreeItemId m_TreeRoot;
    wxTextCtrl* m_FindBox;
    wxStaticText* m_Found;
    // wxCheckBox* m_RegexpCheckBox;
    wxNotebook* m_Notebook;
    wxPanel* m_PresPanel;
    AvenPresList* m_PresList;
    wxString m_File;
    wxString m_Title, m_DateStamp;
    int separator; // character separating survey levels (often '.')
#ifdef PREFDLG
    PrefsDlg* m_PrefsDlg;
#endif
    wxString m_pres_filename;
    bool m_pres_modified;

    struct {
	Double x, y, z;
    } m_Offsets;

    wxPageSetupData m_pageSetupData;
    wxPrintData m_printData;

    wxString icon_path;

    void SetTreeItemColour(LabelInfo* label);
    void FillTree();
    bool LoadData(const wxString& file, wxString prefix = "");
    void FixLRUD(vector<PointInfo> & centreline);
    void CentreDataset(Double xmin, Double ymin, Double zmin);

    wxString GetTabMsg(int key) {
	wxString x(msg(key)); x.Replace("##", "\t"); x.Replace("@", "&"); return x;
    }

    void CreateMenuBar();
    void CreateToolBar();
    void CreateSidePanel();

public:
    MainFrm(const wxString& title, const wxPoint& pos, const wxSize& size);
    ~MainFrm();

    void OnMRUFile(wxCommandEvent& event);
    void OpenFile(const wxString& file, wxString survey = "");

    void OnPresNewUpdate(wxUpdateUIEvent& event);
    void OnPresOpenUpdate(wxUpdateUIEvent& event);
    void OnPresSaveUpdate(wxUpdateUIEvent& event);
    void OnPresSaveAsUpdate(wxUpdateUIEvent& event);
    void OnPresMarkUpdate(wxUpdateUIEvent& event);
    void OnPresFRewindUpdate(wxUpdateUIEvent& event);
    void OnPresRewindUpdate(wxUpdateUIEvent& event);
    void OnPresReverseUpdate(wxUpdateUIEvent& event);
    void OnPresPlayUpdate(wxUpdateUIEvent& event);
    void OnPresFFUpdate(wxUpdateUIEvent& event);
    void OnPresFFFUpdate(wxUpdateUIEvent& event);
    void OnPresPauseUpdate(wxUpdateUIEvent& event);
    void OnPresStopUpdate(wxUpdateUIEvent& event);
    void OnPresExportMovieUpdate(wxUpdateUIEvent& event);
    //void OnFileOpenTerrainUpdate(wxUpdateUIEvent& event);

    void OnFind(wxCommandEvent& event);
    void OnHide(wxCommandEvent& event);
    void OnHideUpdate(wxUpdateUIEvent& ui);

    void OnOpen(wxCommandEvent& event);
    void OnScreenshot(wxCommandEvent& event);
    void OnScreenshotUpdate(wxUpdateUIEvent& event);
    void OnFilePreferences(wxCommandEvent& event);
    void OnFileOpenTerrain(wxCommandEvent& event);
    void OnPrint(wxCommandEvent& event);
    void OnPageSetup(wxCommandEvent& event);
    void OnPresNew(wxCommandEvent& event);
    void OnPresOpen(wxCommandEvent& event);
    void OnPresSave(wxCommandEvent& event);
    void OnPresSaveAs(wxCommandEvent& event);
    void OnPresMark(wxCommandEvent& event);
    void OnPresFRewind(wxCommandEvent& event);
    void OnPresRewind(wxCommandEvent& event);
    void OnPresReverse(wxCommandEvent& event);
    void OnPresPlay(wxCommandEvent& event);
    void OnPresFF(wxCommandEvent& event);
    void OnPresFFF(wxCommandEvent& event);
    void OnPresPause(wxCommandEvent& event);
    void OnPresStop(wxCommandEvent& event);
    void OnPresExportMovie(wxCommandEvent& event);
    void OnExport(wxCommandEvent& event);
    void OnQuit(wxCommandEvent& event);

    void OnAbout(wxCommandEvent& event);
    void OnClose(wxCloseEvent&);

    void OnSetFocus(wxFocusEvent &) { if (m_Gfx) m_Gfx->SetFocus(); }

    void OnPrintUpdate(wxUpdateUIEvent &ui) { ui.Enable(!m_File.empty()); }
    void OnExportUpdate(wxUpdateUIEvent &ui) { ui.Enable(!m_File.empty()); }

    // temporary bodges until event handling problem is sorted out:
    void OnDefaultsUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnDefaultsUpdate(event); }
    void OnPlanUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnPlanUpdate(event); }
    void OnElevationUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnElevationUpdate(event); }
    void OnDisplayOverlappingNamesUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnDisplayOverlappingNamesUpdate(event); }
    void OnColourByDepthUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnColourByDepthUpdate(event); }
    void OnShowCrossesUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowCrossesUpdate(event); }
    void OnShowEntrancesUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowEntrancesUpdate(event); }
    void OnShowFixedPtsUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowFixedPtsUpdate(event); }
    void OnShowExportedPtsUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowExportedPtsUpdate(event); }
    void OnShowStationNamesUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowStationNamesUpdate(event); }
    void OnShowSurveyLegsUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowSurveyLegsUpdate(event); }
    void OnShowSurfaceUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowSurfaceUpdate(event); }
    void OnMoveEastUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnMoveEastUpdate(event); }
    void OnMoveNorthUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnMoveNorthUpdate(event); }
    void OnMoveSouthUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnMoveSouthUpdate(event); }
    void OnMoveWestUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnMoveWestUpdate(event); }
    void OnStartRotationUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnStartRotationUpdate(event); }
    void OnToggleRotationUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnToggleRotationUpdate(event); }
    void OnStopRotationUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnStopRotationUpdate(event); }
    void OnReverseControlsUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnReverseControlsUpdate(event); }
    void OnSlowDownUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnSlowDownUpdate(event); }
    void OnSpeedUpUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnSpeedUpUpdate(event); }
    void OnStepOnceAnticlockwiseUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnStepOnceAnticlockwiseUpdate(event); }
    void OnStepOnceClockwiseUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnStepOnceClockwiseUpdate(event); }
    void OnHigherViewpointUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnHigherViewpointUpdate(event); }
    void OnLowerViewpointUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnLowerViewpointUpdate(event); }
    void OnShiftDisplayDownUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShiftDisplayDownUpdate(event); }
    void OnShiftDisplayLeftUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShiftDisplayLeftUpdate(event); }
    void OnShiftDisplayRightUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShiftDisplayRightUpdate(event); }
    void OnShiftDisplayUpUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShiftDisplayUpUpdate(event); }
    void OnZoomInUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnZoomInUpdate(event); }
    void OnZoomOutUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnZoomOutUpdate(event); }
    void OnToggleScalebarUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnToggleScalebarUpdate(event); }
    void OnToggleDepthbarUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnToggleDepthbarUpdate(event); }
    void OnViewCompassUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnViewCompassUpdate(event); }
    void OnViewGridUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnViewGridUpdate(event); }
    void OnViewClinoUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnViewClinoUpdate(event); }
    void OnViewPerspectiveUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnViewPerspectiveUpdate(event); }
    void OnViewTexturedUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnViewTexturedUpdate(event); }
    void OnViewFogUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnViewFogUpdate(event); }
    void OnViewSmoothLinesUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnViewSmoothLinesUpdate(event); }
    void OnViewFullScreenUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnViewFullScreenUpdate(event); }
    void OnReverseDirectionOfRotationUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnReverseDirectionOfRotationUpdate(event); }
    void OnCancelDistLineUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnCancelDistLineUpdate(event); }
    void OnIndicatorsUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnIndicatorsUpdate(event); }

    void OnDefaults(wxCommandEvent&) { if (m_Control) m_Control->OnDefaults(); }
    void OnPlan(wxCommandEvent&) { if (m_Control) m_Control->OnPlan(); }
    void OnElevation(wxCommandEvent&) { if (m_Control) m_Control->OnElevation(); }
    void OnDisplayOverlappingNames(wxCommandEvent&) { if (m_Control) m_Control->OnDisplayOverlappingNames(); }
    void OnColourByDepth(wxCommandEvent&) { if (m_Control) m_Control->OnColourByDepth(); }
    void OnShowCrosses(wxCommandEvent&) { if (m_Control) m_Control->OnShowCrosses(); }
    void OnShowEntrances(wxCommandEvent&) { if (m_Control) m_Control->OnShowEntrances(); }
    void OnShowFixedPts(wxCommandEvent&) { if (m_Control) m_Control->OnShowFixedPts(); }
    void OnShowExportedPts(wxCommandEvent&) { if (m_Control) m_Control->OnShowExportedPts(); }
    void OnShowStationNames(wxCommandEvent&) { if (m_Control) m_Control->OnShowStationNames(); }
    void OnShowSurveyLegs(wxCommandEvent&) { if (m_Control) m_Control->OnShowSurveyLegs(); }
    void OnShowSurface(wxCommandEvent&) { if (m_Control) m_Control->OnShowSurface(); }
    void OnMoveEast(wxCommandEvent&) { if (m_Control) m_Control->OnMoveEast(); }
    void OnMoveNorth(wxCommandEvent&) { if (m_Control) m_Control->OnMoveNorth(); }
    void OnMoveSouth(wxCommandEvent&) { if (m_Control) m_Control->OnMoveSouth(); }
    void OnMoveWest(wxCommandEvent&) { if (m_Control) m_Control->OnMoveWest(); }
    void OnStartRotation(wxCommandEvent&) { if (m_Control) m_Control->OnStartRotation(); }
    void OnToggleRotation(wxCommandEvent&) { if (m_Control) m_Control->OnToggleRotation(); }
    void OnStopRotation(wxCommandEvent&) { if (m_Control) m_Control->OnStopRotation(); }
    void OnReverseControls(wxCommandEvent&) { if (m_Control) m_Control->OnReverseControls(); }
    void OnSlowDown(wxCommandEvent&) { if (m_Control) m_Control->OnSlowDown(); }
    void OnSpeedUp(wxCommandEvent&) { if (m_Control) m_Control->OnSpeedUp(); }
    void OnStepOnceAnticlockwise(wxCommandEvent&) { if (m_Control) m_Control->OnStepOnceAnticlockwise(); }
    void OnStepOnceClockwise(wxCommandEvent&) { if (m_Control) m_Control->OnStepOnceClockwise(); }
    void OnHigherViewpoint(wxCommandEvent&) { if (m_Control) m_Control->OnHigherViewpoint(); }
    void OnLowerViewpoint(wxCommandEvent&) { if (m_Control) m_Control->OnLowerViewpoint(); }
    void OnShiftDisplayDown(wxCommandEvent&) { if (m_Control) m_Control->OnShiftDisplayDown(); }
    void OnShiftDisplayLeft(wxCommandEvent&) { if (m_Control) m_Control->OnShiftDisplayLeft(); }
    void OnShiftDisplayRight(wxCommandEvent&) { if (m_Control) m_Control->OnShiftDisplayRight(); }
    void OnShiftDisplayUp(wxCommandEvent&) { if (m_Control) m_Control->OnShiftDisplayUp(); }
    void OnZoomIn(wxCommandEvent&) { if (m_Control) m_Control->OnZoomIn(); }
    void OnZoomOut(wxCommandEvent&) { if (m_Control) m_Control->OnZoomOut(); }
    void OnToggleScalebar(wxCommandEvent&) { if (m_Control) m_Control->OnToggleScalebar(); }
    void OnToggleDepthbar(wxCommandEvent&) { if (m_Control) m_Control->OnToggleDepthbar(); }
    void OnViewCompass(wxCommandEvent&) { if (m_Control) m_Control->OnViewCompass(); }
    void OnViewClino(wxCommandEvent&) { if (m_Control) m_Control->OnViewClino(); }
    void OnViewGrid(wxCommandEvent&) { if (m_Control) m_Control->OnViewGrid(); }
    void OnViewPerspective(wxCommandEvent&) { if (m_Control) m_Control->OnViewPerspective(); }
    void OnViewTextured(wxCommandEvent&) { if (m_Control) m_Control->OnViewTextured(); }
    void OnViewFog(wxCommandEvent&) { if (m_Control) m_Control->OnViewFog(); }
    void OnViewSmoothLines(wxCommandEvent&) { if (m_Control) m_Control->OnViewSmoothLines(); }
    void OnViewFullScreen(wxCommandEvent&) { ViewFullScreen(); }
    void ViewFullScreen();
    void OnReverseDirectionOfRotation(wxCommandEvent&) { if (m_Control) m_Control->OnReverseDirectionOfRotation(); }
    void OnCancelDistLine(wxCommandEvent&) { if (m_Control) m_Control->OnCancelDistLine(); }

    void OnToggleMetric(wxCommandEvent&) { if (m_Control) m_Control->OnToggleMetric(); }
    void OnToggleDegrees(wxCommandEvent&) { if (m_Control) m_Control->OnToggleDegrees(); }
    void OnToggleTubes(wxCommandEvent&) { if (m_Control) m_Control->OnToggleTubes(); }

    void OnToggleMetricUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnToggleMetricUpdate(event); }
    void OnToggleDegreesUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnToggleDegreesUpdate(event); }
    void OnToggleTubesUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnToggleTubesUpdate(event); }

    // end of horrible bodges

    void OnViewSidePanelUpdate(wxUpdateUIEvent& event);
    void OnViewSidePanel(wxCommandEvent& event);
    void ToggleSidePanel();
    bool ShowingSidePanel();

    Double GetXExtent() const { return m_XExt; }
    Double GetYExtent() const { return m_YExt; }
    Double GetZExtent() const { return m_ZExt; }
    Double GetZMin() const { return m_ZMin; }
    Double GetZMax() const { return m_ZMin + m_ZExt; }

    int GetNumCrosses() const { return m_NumCrosses; } // FIXME: unused

    void SelectTreeItem(LabelInfo* label);
    void ClearTreeSelection();

    int GetNumFixedPts() const { return m_NumFixedPts; }
    int GetNumExportedPts() const { return m_NumExportedPts; }
    int GetNumEntrances() const { return m_NumEntrances; }
    int GetNumHighlightedPts() const { return m_NumHighlighted; }

    bool HasUndergroundLegs() const { return m_HasUndergroundLegs; }
    bool HasSurfaceLegs() const { return m_HasSurfaceLegs; }

    void ClearCoords();
    void SetCoords(Double x, Double y, Double z);
    void SetCoords(Double x, Double y);
    void SetAltitude(Double z);

    Double GetXOffset() const { return m_Offsets.x; }
    Double GetYOffset() const { return m_Offsets.y; }
    Double GetZOffset() const { return m_Offsets.z; }

    void SetMouseOverStation(LabelInfo* label);

    list<vector<PointInfo> >::const_iterator traverses_begin() const {
	return traverses.begin();
    }

    list<vector<PointInfo> >::const_iterator traverses_end() const {
	return traverses.end();
    }

    list<vector<PointInfo> >::const_iterator surface_traverses_begin() const {
	return surface_traverses.begin();
    }

    list<vector<PointInfo> >::const_iterator surface_traverses_end() const {
	return surface_traverses.end();
    }

    list<LabelInfo*>::const_iterator GetLabels() const {
	return m_Labels.begin();
    }

    list<LabelInfo*>::const_iterator GetLabelsEnd() const {
	return m_Labels.end();
    }

    list<LabelInfo*>::const_reverse_iterator GetRevLabels() const {
	return m_Labels.rbegin();
    }

    list<LabelInfo*>::const_reverse_iterator GetRevLabelsEnd() const {
	return m_Labels.rend();
    }

    list<LabelInfo*>::iterator GetLabelsNC() {
	return m_Labels.begin();
    }

    list<LabelInfo*>::iterator GetLabelsNCEnd() {
	return m_Labels.end();
    }

    void ShowInfo(const LabelInfo *label);
    void DisplayTreeInfo(const wxTreeItemData* data);
    void TreeItemSelected(wxTreeItemData* data);
    wxPageSetupData * GetPageSetupData() { return &m_pageSetupData; }
    PresentationMark GetPresMark(int which);

private:
    DECLARE_EVENT_TABLE()
};

// wxGtk loses the dialog under the always-on-top, maximised window.
// By creating this object on the stack, you can get the dialog on top...
class AvenAllowOnTop {
#ifndef _WIN32
        MainFrm * mainfrm;
#endif
    public:
	AvenAllowOnTop(MainFrm * mainfrm_)
#ifndef _WIN32
	    : mainfrm(0)
#endif
	{
#ifndef _WIN32
	    if (mainfrm_->IsFullScreen()) {
		mainfrm = mainfrm_;
		mainfrm->ViewFullScreen();
	    }
#else
	    (void)mainfrm_;
#endif
	}
	~AvenAllowOnTop() {
#ifndef _WIN32
	    if (mainfrm) mainfrm->ViewFullScreen();
#endif
	}
};

#endif
