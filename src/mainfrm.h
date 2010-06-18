//
//  mainfrm.h
//
//  Main frame handling for Aven.
//
//  Copyright (C) 2000-2003,2005 Mark R. Shinwell
//  Copyright (C) 2001-2003,2004,2005,2006 Olly Betts
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
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

#ifndef mainfrm_h
#define mainfrm_h

#include "wx.h"
#include <wx/docview.h> // for m_FileHistory
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <wx/print.h>
#include <wx/printdlg.h>

#include "aventreectrl.h"
#include "gfxcore.h"
#include "guicontrol.h"
#include "img.h"
#include "message.h"
#include "vector3.h"
#include "aven.h"
//#include "prefsdlg.h"

#include <list>
#include <vector>

using namespace std;

#include <math.h>
#include <time.h>

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
    menu_VIEW_BOUNDING_BOX,
    menu_VIEW_SHOW_TUBES,
    menu_VIEW_PERSPECTIVE,
    menu_VIEW_SMOOTH_SHADING,
    menu_VIEW_TEXTURED,
    menu_VIEW_FOG,
    menu_VIEW_SMOOTH_LINES,
    menu_VIEW_FULLSCREEN,
    menu_VIEW_PREFERENCES,
    menu_VIEW_COLOUR_BY_DEPTH,
    menu_VIEW_COLOUR_BY_DATE,
    menu_VIEW_COLOUR_BY_ERROR,
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

class PointInfo : public Point {
    time_t date;

public:
    PointInfo() : Point(), date(0) { }
    PointInfo(const img_point & pt) : Point(pt), date(0) { }
    PointInfo(const img_point & pt, time_t date_) : Point(pt), date(date_) { }
    PointInfo(const Point & p, time_t date_) : Point(p), date(date_) { }
    time_t GetDate() const { return date; }
};

class XSect : public PointInfo {
    friend class MainFrm;
    Double l, r, u, d;

public:
    XSect() : PointInfo(), l(0), r(0), u(0), d(0) { }
    XSect(const Point &p, time_t date_,
	  Double l_, Double r_, Double u_, Double d_)
	: PointInfo(p, date_), l(l_), r(r_), u(u_), d(d_) { }
    Double GetL() const { return l; }
    Double GetR() const { return r; }
    Double GetU() const { return u; }
    Double GetD() const { return d; }
};

#define LFLAG_SURFACE		img_SFLAG_SURFACE
#define LFLAG_UNDERGROUND	img_SFLAG_UNDERGROUND
#define LFLAG_EXPORTED		img_SFLAG_EXPORTED
#define LFLAG_FIXED		img_SFLAG_FIXED
#define LFLAG_ENTRANCE		0x100
#define LFLAG_HIGHLIGHTED	0x200

class LabelPlotCmp;
class AvenPresList;

class LabelInfo : public Point {
    wxString text;
    unsigned width;
    int flags;

public:
    wxTreeItemId tree_id;

    LabelInfo(const img_point &pt, const wxString &text_, int flags_)
	: Point(pt), text(text_), flags(flags_) { }
    const wxString & GetText() const { return text; }
    int get_flags() const { return flags; }
    void set_flags(int mask) { flags |= mask; }
    void clear_flags(int mask) { flags &= ~mask; }
    unsigned get_width() const { return width; }
    void set_width(unsigned width_) { width = width_; }

    bool IsEntrance() const { return (flags & LFLAG_ENTRANCE) != 0; }
    bool IsFixedPt() const { return (flags & LFLAG_FIXED) != 0; }
    bool IsExportedPt() const { return (flags & LFLAG_EXPORTED) != 0; }
    bool IsUnderground() const { return (flags & LFLAG_UNDERGROUND) != 0; }
    bool IsSurface() const { return (flags & LFLAG_SURFACE) != 0; }
    bool IsHighLighted() const { return (flags & LFLAG_HIGHLIGHTED) != 0; }
};

class traverse : public vector<PointInfo> {
  public:
    int n_legs;
    double length;
    double E, H, V;

    traverse() : n_legs(0), length(0), E(-1), H(-1), V(-1) { }
};

class MainFrm : public wxFrame {
    wxFileHistory m_history;
    int m_SashPosition;
    list<traverse> traverses;
    list<traverse> surface_traverses;
    list<vector<XSect> > tubes;
    list<LabelInfo*> m_Labels;
    Vector3 m_Ext;
    Double m_DepthMin, m_DepthExt;
    time_t m_DateMin, m_DateExt;
    bool complete_dateinfo;
    GfxCore* m_Gfx;
    GUIControl* m_Control;
    int m_NumEntrances;
    int m_NumFixedPts;
    int m_NumExportedPts;
    int m_NumHighlighted;
    bool m_HasUndergroundLegs;
    bool m_HasSurfaceLegs;
    bool m_HasErrorInformation;
    wxSplitterWindow* m_Splitter;
    AvenTreeCtrl* m_Tree;
    wxTextCtrl* m_FindBox;
    // wxCheckBox* m_RegexpCheckBox;
    wxNotebook* m_Notebook;
    AvenPresList* m_PresList;
    wxString m_File;
    wxString m_Title, m_DateStamp;
    int separator; // character separating survey levels (often '.')
#ifdef PREFDLG
    PrefsDlg* m_PrefsDlg;
#endif

    Vector3 m_Offsets;

    wxString icon_path;

    // Strings for status bar reporting of distances.
    wxString here_text, coords_text, dist_text, distfree_text;

    bool m_IsExtendedElevation;

    void FillTree();
    bool ProcessSVXFile(const wxString & file);
    bool LoadData(const wxString& file, wxString prefix = wxString());
//    void FixLRUD(traverse & centreline);
    void CentreDataset(const Vector3 & vmin);

    wxString GetTabMsg(int key) {
	wxString x(wmsg(key)); x.Replace(wxT("##"), wxT("\t")); x.Replace(wxT("@"), wxT("&")); return x;
    }

    void CreateMenuBar();
    void CreateToolBar();
    void CreateSidePanel();

    void UpdateStatusBar();

public:
    MainFrm(const wxString& title, const wxPoint& pos, const wxSize& size);
    ~MainFrm();

    void OnMRUFile(wxCommandEvent& event);
    void OpenFile(const wxString& file, wxString survey = wxString());

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
    void OnGotoFound(wxCommandEvent& event);
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

    void OnKeyPress(wxKeyEvent &e) {
	if (m_Gfx) {
	    m_Gfx->SetFocus();
	    m_Gfx->OnKeyPress(e);
	}
    }

    void OnPrintUpdate(wxUpdateUIEvent &ui) { ui.Enable(!m_File.empty()); }
    void OnExportUpdate(wxUpdateUIEvent &ui) { ui.Enable(!m_File.empty()); }

    // temporary bodges until event handling problem is sorted out:
    void OnDefaultsUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnDefaultsUpdate(event); }
    void OnPlanUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnPlanUpdate(event); }
    void OnElevationUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnElevationUpdate(event); }
    void OnDisplayOverlappingNamesUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnDisplayOverlappingNamesUpdate(event); }
    void OnColourByDepthUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnColourByDepthUpdate(event); }
    void OnColourByDateUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnColourByDateUpdate(event); }
    void OnColourByErrorUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnColourByErrorUpdate(event); }
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
    void OnToggleRotationUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnToggleRotationUpdate(event); }
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
    void OnViewBoundingBoxUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnViewBoundingBoxUpdate(event); }
    void OnViewClinoUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnViewClinoUpdate(event); }
    void OnViewPerspectiveUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnViewPerspectiveUpdate(event); }
    void OnViewSmoothShadingUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnViewSmoothShadingUpdate(event); }
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
    void OnColourByDate(wxCommandEvent&) { if (m_Control) m_Control->OnColourByDate(); }
    void OnColourByError(wxCommandEvent&) { if (m_Control) m_Control->OnColourByError(); }
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
    void OnToggleRotation(wxCommandEvent&) { if (m_Control) m_Control->OnToggleRotation(); }
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
    void OnViewBoundingBox(wxCommandEvent&) { if (m_Control) m_Control->OnViewBoundingBox(); }
    void OnViewPerspective(wxCommandEvent&) { if (m_Control) m_Control->OnViewPerspective(); }
    void OnViewSmoothShading(wxCommandEvent&) { if (m_Control) m_Control->OnViewSmoothShading(); }
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

    const Vector3 & GetExtent() const { return m_Ext; }
    Double GetXExtent() const { return m_Ext.GetX(); }
    Double GetYExtent() const { return m_Ext.GetY(); }
    Double GetZExtent() const { return m_Ext.GetZ(); }

    Double GetDepthExtent() const { return m_DepthExt; }
    Double GetDepthMin() const { return m_DepthMin; }

    bool HasCompleteDateInfo() const { return complete_dateinfo; }
    time_t GetDateExtent() const { return m_DateExt; }
    time_t GetDateMin() const { return m_DateMin; }

    void SelectTreeItem(LabelInfo* label);
    void ClearTreeSelection();

    int GetNumFixedPts() const { return m_NumFixedPts; }
    int GetNumExportedPts() const { return m_NumExportedPts; }
    int GetNumEntrances() const { return m_NumEntrances; }
    int GetNumHighlightedPts() const { return m_NumHighlighted; }

    bool HasUndergroundLegs() const { return m_HasUndergroundLegs; }
    bool HasSurfaceLegs() const { return m_HasSurfaceLegs; }
    bool HasTubes() const { return !tubes.empty(); }
    bool HasErrorInformation() const { return m_HasErrorInformation; }

    bool IsExtendedElevation() const { return m_IsExtendedElevation; }

    void ClearCoords();
    void SetCoords(const Vector3 &v);
    const LabelInfo * GetTreeSelection() const;
    void SetCoords(Double x, Double y);
    void SetAltitude(Double z);

    const Vector3 & GetOffset() const { return m_Offsets; }

    list<traverse>::const_iterator traverses_begin() const {
	return traverses.begin();
    }

    list<traverse>::const_iterator traverses_end() const {
	return traverses.end();
    }

    list<traverse>::const_iterator surface_traverses_begin() const {
	return surface_traverses.begin();
    }

    list<traverse>::const_iterator surface_traverses_end() const {
	return surface_traverses.end();
    }

    list<vector<XSect> >::const_iterator tubes_begin() const {
	return tubes.begin();
    }

    list<vector<XSect> >::const_iterator tubes_end() const {
	return tubes.end();
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
    void TreeItemSelected(const wxTreeItemData* data, bool zoom);
    PresentationMark GetPresMark(int which);

private:
    DECLARE_EVENT_TABLE()
};

// wxGTK loses the dialog under the always-on-top, maximised window.
// By creating this object on the stack, you can get the dialog on top...
class AvenAllowOnTop {
#ifndef _WIN32
	MainFrm * mainfrm;
    public:
	AvenAllowOnTop(MainFrm * mainfrm_) {
	    if (mainfrm_ && mainfrm_->IsFullScreen()) {
		mainfrm = mainfrm_;
		mainfrm->ViewFullScreen();
	    } else {
		mainfrm = 0;
	    }
	}
	~AvenAllowOnTop() {
	    if (mainfrm) mainfrm->ViewFullScreen();
	}
#else
    public:
	AvenAllowOnTop(MainFrm *) { }
#endif
};

#endif
