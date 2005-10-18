//
//  mainfrm.h
//
//  Main frame handling for Aven.
//
//  Copyright (C) 2000-2001, Mark R. Shinwell.
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

#ifndef mainfrm_h
#define mainfrm_h

#include "wx.h"
#include <wx/docview.h> // for m_FileHistory
#include <wx/print.h>
#include <wx/printdlg.h>

#include "gfxcore.h"
#include "message.h"
#include "aventreectrl.h"
#include "img.h"

#include <list>
using namespace std;

#include <math.h>

extern const int NUM_DEPTH_COLOURS;

enum {
    menu_FILE_OPEN = 1000,
    menu_FILE_PRINT,
    menu_FILE_PAGE_SETUP,
    menu_FILE_EXPORT,
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

    bool IsEntrance() const { return (flags & LFLAG_ENTRANCE) != 0; }
    bool IsFixedPt() const { return (flags & LFLAG_FIXED) != 0; }
    bool IsExportedPt() const { return (flags & LFLAG_EXPORTED) != 0; }
    bool IsUnderground() const { return (flags & LFLAG_UNDERGROUND) != 0; }
    bool IsSurface() const { return (flags & LFLAG_SURFACE) != 0; }
    bool IsHighLighted() const { return (flags & LFLAG_HIGHLIGHTED) != 0; }
};

class MainFrm : public wxFrame {
    wxFileHistory m_history;
public: // FIXME: just public for workaround bodge in
        // wxSplitterWindow::OnSplitterDClick
    int m_SashPosition;
private:
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
    wxString m_Title, m_DateStamp;
    int separator; // character separating survey levels (often '.')

    Vector3 m_Offsets;

    wxPageSetupDialogData m_pageSetupData;
    wxPrintData m_printData;

    wxString icon_path;

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

    void InitialisePensAndBrushes();
    void CreateMenuBar();
    void CreateToolBar();
    void CreateSidePanel();

public:
    MainFrm(const wxString& title, const wxPoint& pos, const wxSize& size);
    ~MainFrm();

    void OnMRUFile(wxCommandEvent& event);
    void OpenFile(const wxString& file, wxString survey = "", bool delay = false);
    void OnFileOpenTerrainUpdate(wxUpdateUIEvent& event);

    void OnFind(wxCommandEvent& event);
    void OnHide(wxCommandEvent& event);

    void OnOpen(wxCommandEvent& event);
    void OnFileOpenTerrain(wxCommandEvent& event);
    void OnPrint(wxCommandEvent& event);
    void OnPageSetup(wxCommandEvent& event);
    void OnExport(wxCommandEvent& event);
    void OnQuit(wxCommandEvent& event);

    void OnAbout(wxCommandEvent& event);
    void OnClose(wxCloseEvent&);

    void OnSetFocus(wxFocusEvent &) { if (m_Gfx) m_Gfx->SetFocus(); }

    void OnKeyPress(wxKeyEvent &e) {
	if (m_Gfx) {
	    m_Gfx->SetFocus();
	    m_Gfx->OnKeyPress(e);
	}
    }

    void OnPrintUpdate(wxUpdateUIEvent &ui) { ui.Enable(!m_File.empty()); }
    void OnExportUpdate(wxUpdateUIEvent &ui) { ui.Enable(!m_File.empty()); }

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
    void OnIndicatorsUpdate(wxUpdateUIEvent& event) { if (m_Gfx) m_Gfx->OnIndicatorsUpdate(event); }

    void OnDefaults(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnDefaults(); }
    void OnPlan(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnPlan(); }
    void OnElevation(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnElevation(); }
    void OnDisplayOverlappingNames(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnDisplayOverlappingNames(); }
    void OnShowCrosses(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnShowCrosses(); }
    void OnShowEntrances(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnShowEntrances(); }
    void OnShowFixedPts(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnShowFixedPts(); }
    void OnShowExportedPts(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnShowExportedPts(); }
    void OnShowStationNames(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnShowStationNames(); }
    void OnShowSurveyLegs(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnShowSurveyLegs(); }
    void OnShowSurface(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnShowSurface(); }
    void OnShowSurfaceDepth(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnShowSurfaceDepth(); }
    void OnShowSurfaceDashed(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnShowSurfaceDashed(); }
    void OnMoveEast(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnMoveEast(); }
    void OnMoveNorth(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnMoveNorth(); }
    void OnMoveSouth(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnMoveSouth(); }
    void OnMoveWest(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnMoveWest(); }
    void OnStartRotation(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnStartRotation(); }
    void OnToggleRotation(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnToggleRotation(); }
    void OnStopRotation(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnStopRotation(); }
    void OnReverseControls(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnReverseControls(); }
    void OnSlowDown(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnSlowDown(); }
    void OnSpeedUp(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnSpeedUp(); }
    void OnStepOnceAnticlockwise(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnStepOnceAnticlockwise(); }
    void OnStepOnceClockwise(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnStepOnceClockwise(); }
    void OnHigherViewpoint(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnHigherViewpoint(); }
    void OnLowerViewpoint(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnLowerViewpoint(); }
    void OnShiftDisplayDown(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnShiftDisplayDown(); }
    void OnShiftDisplayLeft(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnShiftDisplayLeft(); }
    void OnShiftDisplayRight(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnShiftDisplayRight(); }
    void OnShiftDisplayUp(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnShiftDisplayUp(); }
    void OnZoomIn(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnZoomIn(); }
    void OnZoomOut(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnZoomOut(); }
    void OnToggleScalebar(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnToggleScalebar(); }
    void OnToggleDepthbar(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnToggleDepthbar(); }
    void OnViewCompass(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnViewCompass(); }
    void OnViewClino(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnViewClino(); }
    void OnViewGrid(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnViewGrid(); }
    void OnReverseDirectionOfRotation(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnReverseDirectionOfRotation(); }
    void OnCancelDistLine(wxCommandEvent&) { if (m_Gfx) m_Gfx->OnCancelDistLine(); }
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

    Double GetXOffset() const { return m_Offsets.getX(); }
    Double GetYOffset() const { return m_Offsets.getY(); }
    Double GetZOffset() const { return m_Offsets.getZ(); }

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
    wxPageSetupDialogData * GetPageSetupData() { return &m_pageSetupData; }

private:
    DECLARE_EVENT_TABLE()
};

#endif
