//
//  mainfrm.h
//
//  Main frame handling for Aven.
//
//  Copyright (C) 2000-2003 Mark R. Shinwell
//  Copyright (C) 2001-2003 Olly Betts
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
#include <wx/docview.h>
#include <wx/notebook.h>
#include "gfxcore.h"
#include "message.h"
#include "aventreectrl.h"
#include "img.h"
#include "guicontrol.h"
#include "prefsdlg.h"

#include <list>
#if 0 // if you turn this back on, reenable the check in configure.in too
#ifdef HAVE_EXT_HASH_MAP
#include <ext/hash_map>
#elif defined HAVE_HASH_MAP
#include <hash_map>
#else
#include <map>
#define hash_map map
#endif
#endif

using namespace std;

#include <math.h>

// This is for mingw32/Visual C++:
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern const int NUM_DEPTH_COLOURS;

enum {
    menu_FILE_OPEN = 1000,
    menu_FILE_OPEN_PRES,
    menu_FILE_PREFERENCES,
    menu_FILE_QUIT,
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
    menu_VIEW_SURFACE_DEPTH,
    menu_VIEW_SURFACE_DASHED,
    menu_VIEW_SHOW_OVERLAPPING_NAMES,
    menu_VIEW_SHOW_ENTRANCES,
    menu_VIEW_SHOW_FIXED_PTS,
    menu_VIEW_SHOW_EXPORTED_PTS,
    menu_VIEW_INDICATORS,
    menu_VIEW_COMPASS,
    menu_VIEW_CLINO,
    menu_VIEW_DEPTH_BAR,
    menu_VIEW_SCALE_BAR,
    menu_VIEW_STATUS_BAR,
    menu_VIEW_GRID,
    menu_VIEW_SIDE_PANEL,
    menu_VIEW_METRIC,
    menu_VIEW_DEGREES,
    menu_VIEW_SHOW_TUBES,
    menu_VIEW_FULLSCREEN,
    menu_PRES_CREATE,
    menu_PRES_GO,
    menu_PRES_GO_BACK,
    menu_PRES_RESTART,
    menu_PRES_RECORD,
    menu_PRES_FINISH,
    menu_PRES_ERASE,
    menu_PRES_ERASE_ALL,
    menu_CTL_REVERSE,
    menu_CTL_CANCEL_DIST_LINE,
    menu_HELP_ABOUT,
    button_FIND,
    button_HIDE
};

class PointInfo {
    friend class MainFrm;
    Double x, y, z;
    bool isLine; // false => move, true => draw line
    bool isSurface;

public:
    Double GetX() const { return x; }
    Double GetY() const { return y; }
    Double GetZ() const { return z; }
    bool IsLine() const { return isLine; }
    bool IsSurface() const { return isSurface; }
};

#define LFLAG_SURFACE		img_SFLAG_SURFACE
#define LFLAG_UNDERGROUND	img_SFLAG_UNDERGROUND
#define LFLAG_EXPORTED		img_SFLAG_EXPORTED
#define LFLAG_FIXED		img_SFLAG_FIXED
#define LFLAG_ENTRANCE		0x100
#define LFLAG_HIGHLIGHTED	0x200

class LabelPlotCmp;

class LabelInfo {
    friend class MainFrm;
    friend class GfxCore;
    friend class LabelPlotCmp;
    Double x, y, z;
    wxString text;
    int flags;
    wxTreeItemId tree_id;

public:
    Double GetX() const { return x; }
    Double GetY() const { return y; }
    Double GetZ() const { return z; }

    wxString GetText() const { return text; }

    bool IsEntrance() const { return flags & LFLAG_ENTRANCE; }
    bool IsFixedPt() const { return flags & LFLAG_FIXED; }
    bool IsExportedPt() const { return flags & LFLAG_EXPORTED; }
    bool IsUnderground() const { return flags & LFLAG_UNDERGROUND; }
    bool IsSurface() const { return flags & LFLAG_SURFACE; }
    bool IsHighLighted() const { return flags & LFLAG_HIGHLIGHTED; }
};

class MainFrm : public wxFrame {
    wxFileHistory m_history;
    int m_SashPosition;
    list<PointInfo*> points;
    list<LabelInfo*> m_Labels;
    Double m_XExt;
    Double m_YExt;
    Double m_ZExt;
    Double m_ZMin;
    int m_NumLegs;
    int m_NumPoints;
    int m_NumCrosses;
    int m_NumExtraLegs;
    GfxCore* m_Gfx;
    GUIControl* m_Control;
    GLAPen* m_Pens;
    wxBrush* m_Brushes;
    int m_NumEntrances;
    int m_NumFixedPts;
    int m_NumExportedPts;
    wxSplitterWindow* m_Splitter;
    wxPanel* m_Panel;
    AvenTreeCtrl* m_Tree;
    wxTreeItemId m_TreeRoot;
    wxTextCtrl* m_FindBox;
    wxStaticText* m_MousePtr;
    wxStaticText* m_Coords;
    wxStaticText* m_StnCoords;
    wxStaticText* m_StnName;
    wxStaticText* m_StnAlt;
    wxStaticText* m_Dist1;
    wxStaticText* m_Dist2;
    wxStaticText* m_Dist3;
    wxStaticText* m_Found;
    wxCheckBox* m_RegexpCheckBox;
    wxNotebook* m_Notebook;
    wxPanel* m_PresPanel;
//    wxListCtrl* m_PresList;
    wxString m_File;
    int separator; // character separating survey levels (often '.')
    PrefsDlg* m_PrefsDlg;
#ifdef AVENPRES
    FILE* m_PresFP;
    bool m_PresLoaded;
    bool m_Recording;
#endif

    struct {
	Double x, y, z;
    } m_Offsets;

    void SetTreeItemColour(LabelInfo* label);
    void FillTree();
    void ClearPointLists();
    bool LoadData(const wxString& file, wxString prefix = "");
    void CentreDataset(Double xmin, Double ymin, Double zmin);

    wxString GetTabMsg(int key) {
	wxString x(msg(key)); x.Replace("##", "\t"); x.Replace("@", "&"); return x;
    }

    void InitialisePensAndBrushes();
    void CreateMenuBar();
    void CreateToolBar();
    void CreateSidePanel();

public:
    MainFrm(const wxString& title, const wxPoint& pos, const wxSize& size);
    ~MainFrm();

    void OnMRUFile(wxCommandEvent& event);
    void OpenFile(const wxString& file, wxString survey = "");
#ifdef AVENPRES
    void OnOpenPresUpdate(wxUpdateUIEvent& event);
#endif
    void OnFileOpenTerrainUpdate(wxUpdateUIEvent& event);

    void OnFind(wxCommandEvent& event);
    void OnHide(wxCommandEvent& event);

    void OnOpen(wxCommandEvent& event);
    void OnFilePreferences(wxCommandEvent& event);
    void OnFileOpenTerrain(wxCommandEvent& event);
#ifdef AVENPRES
    void OnOpenPres(wxCommandEvent& event);
#endif
    void OnQuit(wxCommandEvent& event);

#ifdef AVENPRES
    void OnPresCreate(wxCommandEvent& event);
    void OnPresGo(wxCommandEvent& event);
    void OnPresGoBack(wxCommandEvent& event);
    void OnPresFinish(wxCommandEvent& event);
    void OnPresRestart(wxCommandEvent& event);
    void OnPresRecord(wxCommandEvent& event);
    void OnPresErase(wxCommandEvent& event);
    void OnPresEraseAll(wxCommandEvent& event);

    void OnPresCreateUpdate(wxUpdateUIEvent& event);
    void OnPresGoUpdate(wxUpdateUIEvent& event);
    void OnPresGoBackUpdate(wxUpdateUIEvent& event);
    void OnPresFinishUpdate(wxUpdateUIEvent& event);
    void OnPresRestartUpdate(wxUpdateUIEvent& event);
    void OnPresRecordUpdate(wxUpdateUIEvent& event);
    void OnPresEraseUpdate(wxUpdateUIEvent& event);
    void OnPresEraseAllUpdate(wxUpdateUIEvent& event);
#endif

    void OnAbout(wxCommandEvent& event);
    void OnClose(wxCloseEvent&);

    void OnSetFocus(wxFocusEvent &e) { if (m_Gfx) m_Gfx->SetFocus(); }

    // temporary bodges until event handling problem is sorted out:
    void OnDefaultsUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnDefaultsUpdate(event); }
    void OnPlanUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnPlanUpdate(event); }
    void OnElevationUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnElevationUpdate(event); }
    void OnDisplayOverlappingNamesUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnDisplayOverlappingNamesUpdate(event); }
    void OnShowCrossesUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowCrossesUpdate(event); }
    void OnShowEntrancesUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowEntrancesUpdate(event); }
    void OnShowFixedPtsUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowFixedPtsUpdate(event); }
    void OnShowExportedPtsUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowExportedPtsUpdate(event); }
    void OnShowStationNamesUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowStationNamesUpdate(event); }
    void OnShowSurveyLegsUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowSurveyLegsUpdate(event); }
    void OnShowSurfaceUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowSurfaceUpdate(event); }
    void OnShowSurfaceDepthUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowSurfaceDepthUpdate(event); }
    void OnShowSurfaceDashedUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowSurfaceDashedUpdate(event); }
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
    void OnViewFullScreenUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnViewFullScreenUpdate(event); }
    void OnReverseDirectionOfRotationUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnReverseDirectionOfRotationUpdate(event); }
    void OnCancelDistLineUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnCancelDistLineUpdate(event); }
    void OnIndicatorsUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnIndicatorsUpdate(event); }

    void OnDefaults(wxCommandEvent& event) { if (m_Control) m_Control->OnDefaults(); }
    void OnPlan(wxCommandEvent& event) { if (m_Control) m_Control->OnPlan(); }
    void OnElevation(wxCommandEvent& event) { if (m_Control) m_Control->OnElevation(); }
    void OnDisplayOverlappingNames(wxCommandEvent& event) { if (m_Control) m_Control->OnDisplayOverlappingNames(); }
    void OnShowCrosses(wxCommandEvent& event) { if (m_Control) m_Control->OnShowCrosses(); }
    void OnShowEntrances(wxCommandEvent& event) { if (m_Control) m_Control->OnShowEntrances(); }
    void OnShowFixedPts(wxCommandEvent& event) { if (m_Control) m_Control->OnShowFixedPts(); }
    void OnShowExportedPts(wxCommandEvent& event) { if (m_Control) m_Control->OnShowExportedPts(); }
    void OnShowStationNames(wxCommandEvent& event) { if (m_Control) m_Control->OnShowStationNames(); }
    void OnShowSurveyLegs(wxCommandEvent& event) { if (m_Control) m_Control->OnShowSurveyLegs(); }
    void OnShowSurface(wxCommandEvent& event) { if (m_Control) m_Control->OnShowSurface(); }
    void OnShowSurfaceDepth(wxCommandEvent& event) { if (m_Control) m_Control->OnShowSurfaceDepth(); }
    void OnShowSurfaceDashed(wxCommandEvent& event) { if (m_Control) m_Control->OnShowSurfaceDashed(); }
    void OnMoveEast(wxCommandEvent& event) { if (m_Control) m_Control->OnMoveEast(); }
    void OnMoveNorth(wxCommandEvent& event) { if (m_Control) m_Control->OnMoveNorth(); }
    void OnMoveSouth(wxCommandEvent& event) { if (m_Control) m_Control->OnMoveSouth(); }
    void OnMoveWest(wxCommandEvent& event) { if (m_Control) m_Control->OnMoveWest(); }
    void OnStartRotation(wxCommandEvent& event) { if (m_Control) m_Control->OnStartRotation(); }
    void OnToggleRotation(wxCommandEvent& event) { if (m_Control) m_Control->OnToggleRotation(); }
    void OnStopRotation(wxCommandEvent& event) { if (m_Control) m_Control->OnStopRotation(); }
    void OnReverseControls(wxCommandEvent& event) { if (m_Control) m_Control->OnReverseControls(); }
    void OnSlowDown(wxCommandEvent& event) { if (m_Control) m_Control->OnSlowDown(); }
    void OnSpeedUp(wxCommandEvent& event) { if (m_Control) m_Control->OnSpeedUp(); }
    void OnStepOnceAnticlockwise(wxCommandEvent& event) { if (m_Control) m_Control->OnStepOnceAnticlockwise(); }
    void OnStepOnceClockwise(wxCommandEvent& event) { if (m_Control) m_Control->OnStepOnceClockwise(); }
    void OnHigherViewpoint(wxCommandEvent& event) { if (m_Control) m_Control->OnHigherViewpoint(); }
    void OnLowerViewpoint(wxCommandEvent& event) { if (m_Control) m_Control->OnLowerViewpoint(); }
    void OnShiftDisplayDown(wxCommandEvent& event) { if (m_Control) m_Control->OnShiftDisplayDown(); }
    void OnShiftDisplayLeft(wxCommandEvent& event) { if (m_Control) m_Control->OnShiftDisplayLeft(); }
    void OnShiftDisplayRight(wxCommandEvent& event) { if (m_Control) m_Control->OnShiftDisplayRight(); }
    void OnShiftDisplayUp(wxCommandEvent& event) { if (m_Control) m_Control->OnShiftDisplayUp(); }
    void OnZoomIn(wxCommandEvent& event) { if (m_Control) m_Control->OnZoomIn(); }
    void OnZoomOut(wxCommandEvent& event) { if (m_Control) m_Control->OnZoomOut(); }
    void OnToggleScalebar(wxCommandEvent& event) { if (m_Control) m_Control->OnToggleScalebar(); }
    void OnToggleDepthbar(wxCommandEvent& event) { if (m_Control) m_Control->OnToggleDepthbar(); }
    void OnViewCompass(wxCommandEvent& event) { if (m_Control) m_Control->OnViewCompass(); }
    void OnViewClino(wxCommandEvent& event) { if (m_Control) m_Control->OnViewClino(); }
    void OnViewGrid(wxCommandEvent& event) { if (m_Control) m_Control->OnViewGrid(); }
    void OnViewFullScreen(wxCommandEvent& event) {
	ViewFullScreen();
    }
    void ViewFullScreen() {
	ShowFullScreen(!IsFullScreen());
#ifndef _WIN32
	// wxGTK doesn't currently remove the toolbar, statusbar, or menubar.
	// Can't work out how to lose the menubar right now, but this works for
	// the other two.  FIXME: tidy this code up and submit a patch for
	// wxWindows.
	wxToolBar *tb = GetToolBar();
	if (tb) tb->Show(!IsFullScreen());
	wxStatusBar *sb = GetStatusBar();
	if (sb) sb->Show(!IsFullScreen());
#endif
    }
    void OnReverseDirectionOfRotation(wxCommandEvent& event) { if (m_Control) m_Control->OnReverseDirectionOfRotation(); }
    void OnCancelDistLine(wxCommandEvent& event) { if (m_Control) m_Control->OnCancelDistLine(); }

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

    int GetNumLegs() const { return m_NumLegs; }
    int GetNumPoints() const { return m_NumPoints; }
    int GetNumCrosses() const { return m_NumCrosses; }

    int GetNumDepthBands() const { return NUM_DEPTH_COLOURS; }

    GLAPen& GetPen(int band) const {
	assert(band >= 0 && band < NUM_DEPTH_COLOURS);
	return m_Pens[band];
    }

    wxBrush GetBrush(int band) const {
	assert(band >= 0 && band < NUM_DEPTH_COLOURS);
	return m_Brushes[band];
    }

    void GetColour(int band, Double& r, Double& g, Double& b) const;

    GLAPen& GetSurfacePen() const { return m_Pens[NUM_DEPTH_COLOURS]; }

    void SelectTreeItem(LabelInfo* label);
    void ClearTreeSelection();

    int GetNumFixedPts() const { return m_NumFixedPts; }
    int GetNumExportedPts() const { return m_NumExportedPts; }
    int GetNumEntrances() const { return m_NumEntrances; }

    void ClearCoords();
    void SetCoords(Double x, Double y);
    void SetAltitude(Double z);

    Double GetXOffset() const { return m_Offsets.x; }
    Double GetYOffset() const { return m_Offsets.y; }
    Double GetZOffset() const { return m_Offsets.z; }

    void SetMouseOverStation(LabelInfo* label);

    list<PointInfo*>::const_iterator GetPoints() const {
	return points.begin();
    }

    list<PointInfo*>::const_iterator GetPointsEnd() const {
	return points.end();
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

    void ShowInfo(const LabelInfo *label);
    void DisplayTreeInfo(const wxTreeItemData* data);
    void TreeItemSelected(wxTreeItemData* data);

private:
    DECLARE_EVENT_TABLE()
};

#endif
