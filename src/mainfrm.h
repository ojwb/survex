//
//  mainfrm.h
//
//  Main frame handling for Aven.
//
//  Copyright (C) 2000-2001, Mark R. Shinwell.
//  Copyright (C) 2001-2002 Olly Betts
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
#include "wx/docview.h"
#include "gfxcore.h"
#include "message.h"
#include "aventreectrl.h"
#include "img.h"

#include <list>
#if 0 // if you turn this back on, reenabled the check in configure.in too
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
#ifdef AVENGL
    menu_FILE_OPEN_TERRAIN,
#endif
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
#ifdef AVENGL
    menu_VIEW_ANTIALIAS,
    menu_VIEW_SOLID_SURFACE,
#endif
    menu_VIEW_METRIC,
    menu_VIEW_DEGREES,
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
    list<PointInfo*>* m_Points;
    list<LabelInfo*> m_Labels;
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
    wxString m_File;
    int separator; // character separating survey levels (often '.')
#ifdef AVENPRES
    FILE* m_PresFP;
    bool m_PresLoaded;
    bool m_Recording;
#endif

    struct {
	Double x, y, z;
    } m_Offsets;

#ifdef AVENGL
    struct {
	Double xmin, xmax;
	Double ymin, ymax;
	Double zmin, zmax;
    } m_TerrainExtents;

    struct {
	int x, y;
    } m_TerrainSize;

    Double* m_TerrainGrid;
#endif

    void SetTreeItemColour(LabelInfo* label);
    void FillTree();
    void ClearPointLists();
    bool LoadData(const wxString& file, wxString prefix = "");
#ifdef AVENGL
    bool LoadTerrain(const wxString& file);
    void OpenTerrain(const wxString& file);
#endif
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

    void InitialisePensAndBrushes();
    void CreateMenuBar();
    void CreateToolBar();
    void CreateSidePanel();

public:
    MainFrm(const wxString& title, const wxPoint& pos, const wxSize& size);
    ~MainFrm();

    void OnMRUFile(wxCommandEvent& event);
    void OpenFile(const wxString& file, wxString survey = "", bool delay = false);
#ifdef AVENPRES
    void OnOpenPresUpdate(wxUpdateUIEvent& event);
#endif
    void OnFileOpenTerrainUpdate(wxUpdateUIEvent& event);

    void OnFind(wxCommandEvent& event);
    void OnHide(wxCommandEvent& event);

    void OnOpen(wxCommandEvent& event);
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
    void OnToggleRotationUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnToggleRotationUpdate(event); }
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
    void OnCancelDistLineUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnCancelDistLineUpdate(event); }
#ifdef AVENGL
    void OnAntiAliasUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnAntiAliasUpdate(event); }
    void OnSolidSurfaceUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnSolidSurfaceUpdate(event); }
#endif
    void OnIndicatorsUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnIndicatorsUpdate(event); }

    void OnDefaults(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnDefaults(); }
    void OnPlan(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnPlan(); }
    void OnElevation(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnElevation(); }
    void OnDisplayOverlappingNames(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnDisplayOverlappingNames(); }
    void OnShowCrosses(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShowCrosses(); }
    void OnShowEntrances(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShowEntrances(); }
    void OnShowFixedPts(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShowFixedPts(); }
    void OnShowExportedPts(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShowExportedPts(); }
    void OnShowStationNames(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShowStationNames(); }
    void OnShowSurveyLegs(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShowSurveyLegs(); }
    void OnShowSurface(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShowSurface(); }
    void OnShowSurfaceDepth(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShowSurfaceDepth(); }
    void OnShowSurfaceDashed(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShowSurfaceDashed(); }
    void OnMoveEast(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnMoveEast(); }
    void OnMoveNorth(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnMoveNorth(); }
    void OnMoveSouth(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnMoveSouth(); }
    void OnMoveWest(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnMoveWest(); }
    void OnStartRotation(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnStartRotation(); }
    void OnToggleRotation(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnToggleRotation(); }
    void OnStopRotation(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnStopRotation(); }
    void OnReverseControls(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnReverseControls(); }
    void OnSlowDown(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnSlowDown(); }
    void OnSpeedUp(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnSpeedUp(); }
    void OnStepOnceAnticlockwise(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnStepOnceAnticlockwise(); }
    void OnStepOnceClockwise(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnStepOnceClockwise(); }
    void OnHigherViewpoint(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnHigherViewpoint(); }
    void OnLowerViewpoint(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnLowerViewpoint(); }
    void OnShiftDisplayDown(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShiftDisplayDown(); }
    void OnShiftDisplayLeft(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShiftDisplayLeft(); }
    void OnShiftDisplayRight(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShiftDisplayRight(); }
    void OnShiftDisplayUp(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnShiftDisplayUp(); }
    void OnZoomIn(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnZoomIn(); }
    void OnZoomOut(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnZoomOut(); }
    void OnToggleScalebar(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnToggleScalebar(); }
    void OnToggleDepthbar(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnToggleDepthbar(); }
    void OnViewCompass(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnViewCompass(); }
    void OnViewClino(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnViewClino(); }
    void OnViewGrid(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnViewGrid(); }
    void OnReverseDirectionOfRotation(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnReverseDirectionOfRotation(); }
    void OnCancelDistLine(wxCommandEvent& event) { if (m_Gfx) m_Gfx->OnCancelDistLine(); }
#ifdef AVENGL
    void OnAntiAlias(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnAntiAlias(); }
    void OnSolidSurface(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnSolidSurface(); }
#endif
    // end of horrible bodges

    void OnToggleMetric(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnToggleMetric(); }
    void OnToggleDegrees(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnToggleDegrees(); }

    void OnToggleMetricUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnToggleMetricUpdate(event); }
    void OnToggleDegreesUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnToggleDegreesUpdate(event); }

    void OnViewSidePanelUpdate(wxUpdateUIEvent& event);
    void OnViewSidePanel(wxCommandEvent& event);
    void ToggleSidePanel();

    Double GetXExtent() const { return m_XExt; }
    Double GetYExtent() const { return m_YExt; }
    Double GetZExtent() const { return m_ZExt; }
    Double GetXMin() const { return m_XMin; }
    Double GetYMin() const { return m_YMin; }
    Double GetYMax() const { return m_YMin + m_YExt; }
    Double GetZMin() const { return m_ZMin; }
    Double GetZMax() const { return m_ZMin + m_ZExt; }

#ifdef AVENGL
    int GetTerrainXSize() const { return m_TerrainSize.x; }
    int GetTerrainYSize() const { return m_TerrainSize.y; }

    Double GetTerrainMinX() const { return m_TerrainExtents.xmin; }
    Double GetTerrainMaxX() const { return m_TerrainExtents.xmax; }
    Double GetTerrainMinY() const { return m_TerrainExtents.ymin; }
    Double GetTerrainMaxY() const { return m_TerrainExtents.ymax; }
    Double GetTerrainMinZ() const { return m_TerrainExtents.zmin; }
    Double GetTerrainMaxZ() const { return m_TerrainExtents.zmax; }

    Double GetTerrainXSquareSize() const {
	return (m_TerrainExtents.xmax - m_TerrainExtents.xmin)
		/ m_TerrainSize.x;
    }
    Double GetTerrainYSquareSize() const {
	return (m_TerrainExtents.ymax - m_TerrainExtents.ymin)
		/ m_TerrainSize.y;
    }

    Double GetTerrainHeight(int x, int y) const {
	assert(x >= 0 && x < m_TerrainSize.x);
	assert(y >= 0 && y < m_TerrainSize.y);

	return m_TerrainGrid[x + m_TerrainSize.x * y];
    }
#endif

    int GetNumLegs() const { return m_NumLegs; }
    int GetNumPoints() const { return m_NumPoints; }
    int GetNumCrosses() const { return m_NumCrosses; }

    int GetNumDepthBands() const { return NUM_DEPTH_COLOURS; }

    wxPen GetPen(int band) const {
	assert(band >= 0 && band < NUM_DEPTH_COLOURS);
	return m_Pens[band];
    }

    wxBrush GetBrush(int band) const {
	assert(band >= 0 && band < NUM_DEPTH_COLOURS);
	return m_Brushes[band];
    }

    void GetColour(int band, Double& r, Double& g, Double& b) const;

    wxPen GetSurfacePen() const { return m_Pens[NUM_DEPTH_COLOURS]; }

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

    list<PointInfo*>::iterator GetPointsNC(int band) const {
	assert(band >= 0 && band < NUM_DEPTH_COLOURS);
	return m_Points[band].begin();
    }

    list<PointInfo*>::iterator GetPointsEndNC(int band) const {
	assert(band >= 0 && band < NUM_DEPTH_COLOURS);
	return m_Points[band].end();
    }

    list<PointInfo*>::const_iterator GetPoints(int band) const {
	assert(band >= 0 && band < NUM_DEPTH_COLOURS);
	return m_Points[band].begin();
    }

    list<PointInfo*>::const_iterator GetPointsEnd(int band) const {
	assert(band >= 0 && band < NUM_DEPTH_COLOURS);
	return m_Points[band].end();
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
