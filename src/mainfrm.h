//
//  mainfrm.h
//
//  Main frame handling for Aven.
//
//  Copyright (C) 2000-2001, Mark R. Shinwell.
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
#include "gfxcore.h"
#include "message.h"
#include "aventreectrl.h"
#include "img.h"

#include <list>
#include <hash_map>

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
#ifdef AVENGL
    menu_VIEW_ANTIALIAS,
#endif
    menu_PRES_CREATE,
    menu_PRES_GO,
    menu_PRES_GO_BACK,
    menu_PRES_RESTART,
    menu_PRES_RECORD,
    menu_PRES_FINISH,
    menu_PRES_ERASE,
    menu_PRES_ERASE_ALL,
    menu_CTL_REVERSE,
    menu_HELP_ABOUT
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

class LabelInfo {
    friend class MainFrm;
    Double x, y, z;
    wxString text;
    int flags;

public:
    Double GetX() const { return x; }
    Double GetY() const { return y; }
    Double GetZ() const { return z; }

    wxString GetText() const { return text; }

    bool IsEntrance() const { return flags & img_SFLAG_ENTRANCE; }
    bool IsFixedPt() const { return flags & img_SFLAG_FIXED; }
    bool IsExportedPt() const { return flags & img_SFLAG_EXPORTED; }
    bool IsUnderground() const { return flags & img_SFLAG_UNDERGROUND; }
    bool IsSurface() const { return flags & img_SFLAG_SURFACE; }
};

class MainFrm : public wxFrame {
    list<PointInfo*>* m_Points;
    list<LabelInfo*> m_Labels;
    hash_map<wxTreeItemId, LabelInfo*> m_LabelMap;
    Double m_XExt;
    Double m_YExt;
    Double m_ZExt;
    Double m_XMin;
    Double m_YMin;
    Double m_ZMin;
    int m_NumLegs;
    int m_NumPoints;
    int m_NumCrosses;
    int m_NumExtraLegs;
    GfxCore* m_Gfx;
    wxPen* m_Pens;
    wxBrush* m_Brushes;
    wxString m_FileToLoad;
    int m_NumEntrances;
    int m_NumFixedPts;
    int m_NumExportedPts;
    wxSplitterWindow* m_Splitter;
    wxPanel* m_Panel;
    AvenTreeCtrl* m_Tree;
    wxBoxSizer* m_PanelSizer;
    wxPanel* m_FindPanel;
    wxTreeItemId m_TreeRoot;
    wxButton* m_FindButton;
    wxButton* m_HideButton;
    wxTextCtrl* m_FindBox;
    wxBoxSizer* m_FindButtonSizer;
    wxBoxSizer* m_FindSizer;
    wxStaticText* m_MousePtr;
    wxStaticText* m_Coords;
    wxStaticText* m_StnCoords;
    wxStaticText* m_StnName;
    wxStaticText* m_StnAlt;
    wxStaticText* m_Dist1;
    wxStaticText* m_Dist2;
    wxStaticText* m_Dist3;
    FILE* m_PresFP;
    wxString m_File;
    bool m_PresLoaded;
    bool m_Recording;

    struct {
        Double x, y, z;
    } m_Offsets;
    
    void SetTreeItemColour(wxTreeItemId& id, LabelInfo* label);
    void FillTree();
    void ClearPointLists();
    bool LoadData(const wxString& file, wxString prefix = "");
    void SortIntoDepthBands(list<PointInfo*>& points);
    void IntersectLineWithPlane(Double x0, Double y0, Double z0,
				Double x1, Double y1, Double z1,
				Double z, Double& x, Double& y);
    Double GetDepthBoundaryBetweenBands(int a, int b);
    int GetDepthColour(Double z);
    void CentreDataset(Double xmin, Double ymin, Double zmin);

    wxString GetTabMsg(int key) {
        wxString x(msg(key)); x.Replace("##", "\t"); x.Replace("@", "&"); return x;
    }

public:
    MainFrm(const wxString& title, const wxPoint& pos, const wxSize& size);
    ~MainFrm();

    void OpenFile(const wxString& file, wxString survey = "",
		  bool delay_gfx_init = false);

    void OnOpen(wxCommandEvent& event);
    void OnOpenPres(wxCommandEvent& event);
    void OnQuit(wxCommandEvent& event);
    void OnPresCreate(wxCommandEvent& event);
    void OnPresGo(wxCommandEvent& event);
    void OnPresGoBack(wxCommandEvent& event);
    void OnPresFinish(wxCommandEvent& event);
    void OnPresRestart(wxCommandEvent& event);
    void OnPresRecord(wxCommandEvent& event);
    void OnPresErase(wxCommandEvent& event);
    void OnPresEraseAll(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnClose(wxCloseEvent&);

    // temporary bodges until event handling problem is sorted out:
    void OnDefaultsUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnDefaultsUpdate(event); }
    void OnPlanUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnPlanUpdate(event); }
    void OnElevationUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnElevationUpdate(event); }
    void OnDisplayOverlappingNamesUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnDisplayOverlappingNamesUpdate(event); }
    void OnShowCrossesUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnShowCrossesUpdate(event); }
    void OnShowEntrancesUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnShowEntrancesUpdate(event); }
    void OnShowFixedPtsUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnShowFixedPtsUpdate(event); }
    void OnShowExportedPtsUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnShowExportedPtsUpdate(event); }
    void OnShowStationNamesUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnShowStationNamesUpdate(event); }
    void OnShowSurveyLegsUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnShowSurveyLegsUpdate(event); }
    void OnShowSurfaceUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnShowSurfaceUpdate(event); }
    void OnShowSurfaceDepthUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnShowSurfaceDepthUpdate(event); }
    void OnShowSurfaceDashedUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnShowSurfaceDashedUpdate(event); }
    void OnMoveEastUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnMoveEastUpdate(event); }
    void OnMoveNorthUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnMoveNorthUpdate(event); }
    void OnMoveSouthUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnMoveSouthUpdate(event); }
    void OnMoveWestUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnMoveWestUpdate(event); }
    void OnStartRotationUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnStartRotationUpdate(event); }
    void OnStopRotationUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnStopRotationUpdate(event); }
    void OnReverseControlsUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnReverseControlsUpdate(event); }
    void OnSlowDownUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnSlowDownUpdate(event); }
    void OnSpeedUpUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnSpeedUpUpdate(event); }
    void OnStepOnceAnticlockwiseUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnStepOnceAnticlockwiseUpdate(event); }
    void OnStepOnceClockwiseUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnStepOnceClockwiseUpdate(event); }
    void OnHigherViewpointUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnHigherViewpointUpdate(event); }
    void OnLowerViewpointUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnLowerViewpointUpdate(event); }
    void OnShiftDisplayDownUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnShiftDisplayDownUpdate(event); }
    void OnShiftDisplayLeftUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnShiftDisplayLeftUpdate(event); }
    void OnShiftDisplayRightUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnShiftDisplayRightUpdate(event); }
    void OnShiftDisplayUpUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnShiftDisplayUpUpdate(event); }
    void OnZoomInUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnZoomInUpdate(event); }
    void OnZoomOutUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnZoomOutUpdate(event); }
    void OnToggleScalebarUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnToggleScalebarUpdate(event); }
    void OnToggleDepthbarUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnToggleDepthbarUpdate(event); }
    void OnViewCompassUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnViewCompassUpdate(event); }
    void OnViewGridUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnViewGridUpdate(event); }
    void OnViewClinoUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnViewClinoUpdate(event); }
    void OnReverseDirectionOfRotationUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnReverseDirectionOfRotationUpdate(event); }
#ifdef AVENGL
    void OnAntiAliasUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnAntiAliasUpdate(event); }
#endif
    void OnIndicatorsUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnIndicatorsUpdate(event); }

    void OnDefaults(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnDefaults(event); }
    void OnPlan(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnPlan(event); }
    void OnElevation(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnElevation(event); }
    void OnDisplayOverlappingNames(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnDisplayOverlappingNames(event); }
    void OnShowCrosses(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShowCrosses(event); }
    void OnShowEntrances(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShowEntrances(event); }
    void OnShowFixedPts(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShowFixedPts(event); }
    void OnShowExportedPts(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShowExportedPts(event); }
    void OnShowStationNames(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShowStationNames(event); }
    void OnShowSurveyLegs(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShowSurveyLegs(event); }
    void OnShowSurface(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShowSurface(event); }
    void OnShowSurfaceDepth(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShowSurfaceDepth(event); }
    void OnShowSurfaceDashed(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShowSurfaceDashed(event); }
    void OnMoveEast(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnMoveEast(event); }
    void OnMoveNorth(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnMoveNorth(event); }
    void OnMoveSouth(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnMoveSouth(event); }
    void OnMoveWest(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnMoveWest(event); }
    void OnStartRotation(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnStartRotation(event); }
    void OnStopRotation(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnStopRotation(event); }
    void OnReverseControls(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnReverseControls(event); }
    void OnSlowDown(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnSlowDown(event); }
    void OnSpeedUp(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnSpeedUp(event); }
    void OnStepOnceAnticlockwise(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnStepOnceAnticlockwise(event); }
    void OnStepOnceClockwise(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnStepOnceClockwise(event); }
    void OnHigherViewpoint(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnHigherViewpoint(event); }
    void OnLowerViewpoint(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnLowerViewpoint(event); }
    void OnShiftDisplayDown(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShiftDisplayDown(event); }
    void OnShiftDisplayLeft(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShiftDisplayLeft(event); }
    void OnShiftDisplayRight(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShiftDisplayRight(event); }
    void OnShiftDisplayUp(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShiftDisplayUp(event); }
    void OnZoomIn(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnZoomIn(event); }
    void OnZoomOut(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnZoomOut(event); }
    void OnToggleScalebar(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnToggleScalebar(event); }
    void OnToggleDepthbar(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnToggleDepthbar(event); }
    void OnViewCompass(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnViewCompass(event); }
    void OnViewClino(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnViewClino(event); }
    void OnViewGrid(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnViewGrid(event); }
    void OnReverseDirectionOfRotation(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnReverseDirectionOfRotation(event); }
#ifdef AVENGL
    void OnAntiAlias(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnAntiAlias(event); }
#endif
    // end of horrible bodges

    Double GetXExtent() { return m_XExt; }
    Double GetYExtent() { return m_YExt; }
    Double GetZExtent() { return m_ZExt; }
    Double GetXMin()    { return m_XMin; }
    Double GetYMin()    { return m_YMin; }
    Double GetZMin()    { return m_ZMin; }

    int GetNumLegs()   { return m_NumLegs; }
    int GetNumPoints() { return m_NumPoints; }
    int GetNumCrosses() { return m_NumCrosses; }

    int GetNumDepthBands() { return NUM_DEPTH_COLOURS; }

    wxPen GetPen(int band) {
        assert(band >= 0 && band < NUM_DEPTH_COLOURS);
        return m_Pens[band];
    }

    wxBrush GetBrush(int band) {
        assert(band >= 0 && band < NUM_DEPTH_COLOURS);
        return m_Brushes[band];
    }

    void GetColour(int band, Double& r, Double& g, Double& b);

    wxPen GetSurfacePen() { return m_Pens[NUM_DEPTH_COLOURS]; }

    int GetNumFixedPts() { return m_NumFixedPts; }
    int GetNumExportedPts() { return m_NumExportedPts; }
    int GetNumEntrances() { return m_NumEntrances; }

    void ClearCoords();
    void SetCoords(Double x, Double y);

    Double GetXOffset() { return m_Offsets.x; }
    Double GetYOffset() { return m_Offsets.y; }
    Double GetZOffset() { return m_Offsets.z; }

    list<PointInfo*>::const_iterator GetPoints(int band) {
        assert(band >= 0 && band < NUM_DEPTH_COLOURS);
        return m_Points[band].begin();
    }

    list<LabelInfo*>::const_iterator GetLabels() {
        return m_Labels.begin();
    }

    list<PointInfo*>::const_iterator GetPointsEnd(int band) {
        assert(band >= 0 && band < NUM_DEPTH_COLOURS);
        return m_Points[band].end();
    }

    list<LabelInfo*>::const_iterator GetLabelsEnd() {
        return m_Labels.end();
    }

    void DisplayTreeInfo(wxTreeItemData* data);
    void TreeItemSelected(wxTreeItemData* data);

private:
    DECLARE_EVENT_TABLE()
};

#endif
