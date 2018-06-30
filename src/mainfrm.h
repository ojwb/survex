//
//  mainfrm.h
//
//  Main frame handling for Aven.
//
//  Copyright (C) 2000-2003,2005 Mark R. Shinwell
//  Copyright (C) 2001-2003,2004,2005,2006,2010,2011,2012,2013,2014,2015,2016,2018 Olly Betts
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
#include "img_hosted.h"
#include "labelinfo.h"
#include "message.h"
#include "model.h"
#include "vector3.h"
#include "aven.h"
//#include "prefsdlg.h"

#include <list>
#include <vector>

using namespace std;

#include <math.h>
#include <time.h>

#ifdef __WXMAC__
// Currently on OS X, we force use of a generic toolbar, which needs to be put
// in a sizer.
# define USING_GENERIC_TOOLBAR
#endif

#define MARK_FIRST 1
#define MARK_NEXT 2
#define MARK_PREV 3

enum {
    menu_FILE_LOG = 1000,
    menu_FILE_OPEN_TERRAIN,
    menu_FILE_PAGE_SETUP,
    menu_FILE_SCREENSHOT,
    menu_FILE_EXPORT,
    menu_FILE_EXTEND,
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
    menu_ROTATION_START,
    menu_ROTATION_STOP,
    menu_ROTATION_TOGGLE,
    menu_ROTATION_REVERSE,
    menu_ORIENT_MOVE_NORTH,
    menu_ORIENT_MOVE_EAST,
    menu_ORIENT_MOVE_SOUTH,
    menu_ORIENT_MOVE_WEST,
    menu_ORIENT_PLAN,
    menu_ORIENT_ELEVATION,
    menu_ORIENT_DEFAULTS,
    menu_VIEW_SHOW_LEGS,
    menu_VIEW_SPLAYS,
    menu_SPLAYS_HIDE,
    menu_SPLAYS_SHOW_DASHED,
    menu_SPLAYS_SHOW_FADED,
    menu_SPLAYS_SHOW_NORMAL,
    menu_VIEW_DUPES,
    menu_DUPES_HIDE,
    menu_DUPES_SHOW_DASHED,
    menu_DUPES_SHOW_FADED,
    menu_DUPES_SHOW_NORMAL,
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
    menu_VIEW_TERRAIN,
    menu_VIEW_SHOW_TUBES,
    menu_VIEW_PERSPECTIVE,
    menu_VIEW_SMOOTH_SHADING,
    menu_VIEW_TEXTURED,
    menu_VIEW_FOG,
    menu_VIEW_SMOOTH_LINES,
    menu_VIEW_FULLSCREEN,
    menu_VIEW_COLOUR_BY,
    menu_COLOUR_BY_DEPTH,
    menu_COLOUR_BY_DATE,
    menu_COLOUR_BY_ERROR,
    menu_COLOUR_BY_GRADIENT,
    menu_COLOUR_BY_LENGTH,
    menu_IND_COMPASS,
    menu_IND_CLINO,
    menu_IND_COLOUR_KEY,
    menu_IND_SCALE_BAR,
    menu_CTL_INDICATORS,
    menu_CTL_SIDE_PANEL,
    menu_CTL_METRIC,
    menu_CTL_DEGREES,
    menu_CTL_PERCENT,
    menu_CTL_REVERSE,
    menu_CTL_CANCEL_DIST_LINE,
    menu_SURVEY_RESTRICT,
    menu_SURVEY_SHOW_ALL,
    menu_SURVEY_HIDE,
    menu_SURVEY_SHOW,
    menu_SURVEY_HIDE_SIBLINGS,
    textctrl_FIND,
    button_HIDE,
    listctrl_PRES
};

class AvenPresList;

class MainFrm : public wxFrame, public Model {
    wxFileHistory m_history;
    int m_SashPosition;
    bool was_showing_sidepanel_before_fullscreen;
    GfxCore* m_Gfx;
    wxWindow* m_Log;
    GUIControl* m_Control;
    wxSplitterWindow* m_Splitter;
    AvenTreeCtrl* m_Tree;
    wxTextCtrl* m_FindBox;
    // wxCheckBox* m_RegexpCheckBox;
    wxNotebook* m_Notebook;
    AvenPresList* m_PresList;
    wxString m_File;
    // Processed version of data - same as m_File if m_File is processed data.
    wxString m_FileProcessed;
    wxString m_Survey;

    // Strings for status bar reporting of distances.
    wxString here_text, coords_text, dist_text, distfree_text;

    int m_NumHighlighted = 0;
    bool pending_find;

    bool fullscreen_showing_menus;

#ifdef PREFDLG
    PrefsDlg* m_PrefsDlg;
#endif

    void FillTree(const wxString & root_name);
    bool ProcessSVXFile(const wxString & file);
//    void FixLRUD(traverse & centreline);

    void CreateMenuBar();
    void MakeToolBar();
    void CreateSidePanel();

    void UpdateStatusBar();

#ifdef USING_GENERIC_TOOLBAR
    wxToolBar * GetToolBar() const {
	wxSizer * sizer = GetSizer();
	if (!sizer) return NULL;
	return (wxToolBar*)sizer->GetItem(size_t(0))->GetWindow();
    }
#endif

public:
    MainFrm(const wxString& title, const wxPoint& pos, const wxSize& size);

    // public for CavernLog.
    bool LoadData(const wxString& file, const wxString& prefix);
    void AddToFileHistory(const wxString & file);

    void InitialiseAfterLoad(const wxString & file, const wxString & prefix);
    void OnShowLog(wxCommandEvent& event);

    void OnMRUFile(wxCommandEvent& event);
    void OpenFile(const wxString& file, const wxString& survey = wxString());

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
    void OnOpenTerrainUpdate(wxUpdateUIEvent& event);

    void DoFind();
    void OnFind(wxCommandEvent& event);
    void OnIdle(wxIdleEvent& event);
    void OnGotoFound(wxCommandEvent& event);
    void OnHide(wxCommandEvent& event);
    void OnHideUpdate(wxUpdateUIEvent& ui);

    void OnOpen(wxCommandEvent& event);
    void OnOpenTerrain(wxCommandEvent&);
    void HideLog(wxWindow * log_window);
    void OnScreenshot(wxCommandEvent& event);
    void OnScreenshotUpdate(wxUpdateUIEvent& event);
    void OnFilePreferences(wxCommandEvent& event);
    void OnPrint(wxCommandEvent& event);
    void PrintAndExit();
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
    void OnExtend(wxCommandEvent& event);
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

    void OnShowLogUpdate(wxUpdateUIEvent &ui) {
	ui.Enable(m_Log != NULL || (m_Splitter->GetWindow1() != m_Gfx && m_Splitter->GetWindow2() != m_Gfx));
	ui.Check(m_Log == NULL);
    }
    void OnPrintUpdate(wxUpdateUIEvent &ui) { ui.Enable(!m_File.empty()); }
    void OnExportUpdate(wxUpdateUIEvent &ui) { ui.Enable(!m_File.empty()); }
    void OnExtendUpdate(wxUpdateUIEvent &ui) {
	ui.Enable(!IsExtendedElevation());
    }

    // temporary bodges until event handling problem is sorted out:
    void OnDefaultsUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnDefaultsUpdate(event); }
    void OnPlanUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnPlanUpdate(event); }
    void OnElevationUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnElevationUpdate(event); }
    void OnDisplayOverlappingNamesUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnDisplayOverlappingNamesUpdate(event); }
    void OnColourByUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnColourByUpdate(event); }
    void OnColourByDepthUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnColourByDepthUpdate(event); }
    void OnColourByDateUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnColourByDateUpdate(event); }
    void OnColourByErrorUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnColourByErrorUpdate(event); }
    void OnColourByGradientUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnColourByGradientUpdate(event); }
    void OnColourByLengthUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnColourByLengthUpdate(event); }
    void OnShowCrossesUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowCrossesUpdate(event); }
    void OnShowEntrancesUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowEntrancesUpdate(event); }
    void OnShowFixedPtsUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowFixedPtsUpdate(event); }
    void OnShowExportedPtsUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowExportedPtsUpdate(event); }
    void OnShowStationNamesUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowStationNamesUpdate(event); }
    void OnShowSurveyLegsUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowSurveyLegsUpdate(event); }
    void OnSplaysUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnSplaysUpdate(event); }
    void OnHideSplaysUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnHideSplaysUpdate(event); }
    void OnShowSplaysDashedUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowSplaysDashedUpdate(event); }
    void OnShowSplaysFadedUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowSplaysFadedUpdate(event); }
    void OnShowSplaysNormalUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowSplaysNormalUpdate(event); }
    void OnDupesUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnDupesUpdate(event); }
    void OnHideDupesUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnHideDupesUpdate(event); }
    void OnShowDupesDashedUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowDupesDashedUpdate(event); }
    void OnShowDupesFadedUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowDupesFadedUpdate(event); }
    void OnShowDupesNormalUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowDupesNormalUpdate(event); }
    void OnShowSurfaceUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnShowSurfaceUpdate(event); }
    void OnMoveEastUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnMoveEastUpdate(event); }
    void OnMoveNorthUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnMoveNorthUpdate(event); }
    void OnMoveSouthUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnMoveSouthUpdate(event); }
    void OnMoveWestUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnMoveWestUpdate(event); }
    void OnToggleRotationUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnToggleRotationUpdate(event); }
    void OnReverseControlsUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnReverseControlsUpdate(event); }
    void OnToggleScalebarUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnToggleScalebarUpdate(event); }
    void OnToggleColourKeyUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnToggleColourKeyUpdate(event); }
    void OnViewCompassUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnViewCompassUpdate(event); }
    void OnViewGridUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnViewGridUpdate(event); }
    void OnViewBoundingBoxUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnViewBoundingBoxUpdate(event); }
    void OnViewTerrainUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnViewTerrainUpdate(event); }
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
    void OnColourByGradient(wxCommandEvent&) { if (m_Control) m_Control->OnColourByGradient(); }
    void OnColourByLength(wxCommandEvent&) { if (m_Control) m_Control->OnColourByLength(); }
    void OnShowCrosses(wxCommandEvent&) { if (m_Control) m_Control->OnShowCrosses(); }
    void OnShowEntrances(wxCommandEvent&) { if (m_Control) m_Control->OnShowEntrances(); }
    void OnShowFixedPts(wxCommandEvent&) { if (m_Control) m_Control->OnShowFixedPts(); }
    void OnShowExportedPts(wxCommandEvent&) { if (m_Control) m_Control->OnShowExportedPts(); }
    void OnShowStationNames(wxCommandEvent&) { if (m_Control) m_Control->OnShowStationNames(); }
    void OnShowSurveyLegs(wxCommandEvent&) { if (m_Control) m_Control->OnShowSurveyLegs(); }
    void OnHideSplays(wxCommandEvent&) { if (m_Control) m_Control->OnHideSplays(); }
    void OnShowSplaysDashed(wxCommandEvent&) { if (m_Control) m_Control->OnShowSplaysDashed(); }
    void OnShowSplaysFaded(wxCommandEvent&) { if (m_Control) m_Control->OnShowSplaysFaded(); }
    void OnShowSplaysNormal(wxCommandEvent&) { if (m_Control) m_Control->OnShowSplaysNormal(); }
    void OnHideDupes(wxCommandEvent&) { if (m_Control) m_Control->OnHideDupes(); }
    void OnShowDupesDashed(wxCommandEvent&) { if (m_Control) m_Control->OnShowDupesDashed(); }
    void OnShowDupesFaded(wxCommandEvent&) { if (m_Control) m_Control->OnShowDupesFaded(); }
    void OnShowDupesNormal(wxCommandEvent&) { if (m_Control) m_Control->OnShowDupesNormal(); }
    void OnShowSurface(wxCommandEvent&) { if (m_Control) m_Control->OnShowSurface(); }
    void OnMoveEast(wxCommandEvent&) { if (m_Control) m_Control->OnMoveEast(); }
    void OnMoveNorth(wxCommandEvent&) { if (m_Control) m_Control->OnMoveNorth(); }
    void OnMoveSouth(wxCommandEvent&) { if (m_Control) m_Control->OnMoveSouth(); }
    void OnMoveWest(wxCommandEvent&) { if (m_Control) m_Control->OnMoveWest(); }
    void OnToggleRotation(wxCommandEvent&) { if (m_Control) m_Control->OnToggleRotation(); }
    void OnReverseControls(wxCommandEvent&) { if (m_Control) m_Control->OnReverseControls(); }
    void OnToggleScalebar(wxCommandEvent&) { if (m_Control) m_Control->OnToggleScalebar(); }
    void OnToggleColourKey(wxCommandEvent&) { if (m_Control) m_Control->OnToggleColourKey(); }
    void OnViewCompass(wxCommandEvent&) { if (m_Control) m_Control->OnViewCompass(); }
    void OnViewClino(wxCommandEvent&) { if (m_Control) m_Control->OnViewClino(); }
    void OnViewGrid(wxCommandEvent&) { if (m_Control) m_Control->OnViewGrid(); }
    void OnViewBoundingBox(wxCommandEvent&) { if (m_Control) m_Control->OnViewBoundingBox(); }
    void OnViewTerrain(wxCommandEvent&) { if (m_Control) m_Control->OnViewTerrain(); }
    void OnViewPerspective(wxCommandEvent&) { if (m_Control) m_Control->OnViewPerspective(); }
    void OnViewSmoothShading(wxCommandEvent&) { if (m_Control) m_Control->OnViewSmoothShading(); }
    void OnViewTextured(wxCommandEvent&) { if (m_Control) m_Control->OnViewTextured(); }
    void OnViewFog(wxCommandEvent&) { if (m_Control) m_Control->OnViewFog(); }
    void OnViewSmoothLines(wxCommandEvent&) { if (m_Control) m_Control->OnViewSmoothLines(); }
    void OnViewFullScreen(wxCommandEvent&) { ViewFullScreen(); }
    void ViewFullScreen();
    bool FullScreenModeShowingMenus() const;
    void FullScreenModeShowMenus(bool show);
    void OnReverseDirectionOfRotation(wxCommandEvent&) { if (m_Control) m_Control->OnReverseDirectionOfRotation(); }
    void OnCancelDistLine(wxCommandEvent&) { if (m_Control) m_Control->OnCancelDistLine(); }

    void OnToggleMetric(wxCommandEvent&) { if (m_Control) m_Control->OnToggleMetric(); }
    void OnToggleDegrees(wxCommandEvent&) { if (m_Control) m_Control->OnToggleDegrees(); }
    void OnTogglePercent(wxCommandEvent&) { if (m_Control) m_Control->OnTogglePercent(); }
    void OnToggleTubes(wxCommandEvent&) { if (m_Control) m_Control->OnToggleTubes(); }

    void OnToggleMetricUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnToggleMetricUpdate(event); }
    void OnToggleDegreesUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnToggleDegreesUpdate(event); }
    void OnTogglePercentUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnTogglePercentUpdate(event); }
    void OnToggleTubesUpdate(wxUpdateUIEvent& event) { if (m_Control) m_Control->OnToggleTubesUpdate(event); }

    // end of horrible bodges

    void OnViewSidePanelUpdate(wxUpdateUIEvent& event);
    void OnViewSidePanel(wxCommandEvent& event);
    void ToggleSidePanel();
    bool ShowingSidePanel();

    void SelectTreeItem(const LabelInfo* label) {
	if (label->tree_id.IsOk())
	    m_Tree->SelectItem(label->tree_id);
	else
	    m_Tree->UnselectAll();
    }

    void ClearTreeSelection();

    void ClearCoords();
    void SetCoords(const Vector3 &v);
    const LabelInfo * GetTreeSelection() const;
    void SetCoords(Double x, Double y, const LabelInfo * there);
    void SetAltitude(Double z, const LabelInfo * there);

    void ShowInfo(const LabelInfo *here = NULL, const LabelInfo *there = NULL);
    void DisplayTreeInfo(const wxTreeItemData* data = NULL);
    void TreeItemSelected(const wxTreeItemData* data);
    PresentationMark GetPresMark(int which);
    bool Animating() const { return m_Gfx && m_Gfx->Animating(); }

    // Restrict to subsurvey `survey` (or show all if `survey` empty).
    void RestrictTo(const wxString & survey);

    wxString GetSurvey() const { return m_Survey; }

    int GetNumHighlightedPts() const { return m_NumHighlighted; }

    const AvenTreeCtrl* GetTreeFilter() const { return m_Tree->GetFilter(); }

    // Used when the tree filters change.  FIXME: Overkill?
    void ForceFullRedraw() {
	m_Gfx->InvalidateAllLists();
	m_Gfx->ForceRefresh();
    }

private:
    DECLARE_EVENT_TABLE()
};

// Older wxGTK loses pop-up dialogs under the always-on-top, maximised window.
// Not sure when this got fixed, but wx 2.8.10 definitely works properly on
// Debian squeeze.
//
// To work around this issue, create this object on the stack, and it will
// temporarily un-fullscreen the window while the dialog as a workaround.
class AvenAllowOnTop {
#if defined __WXGTK__ && !wxCHECK_VERSION(2,8,10)
	MainFrm * mainfrm;
    public:
	explicit AvenAllowOnTop(MainFrm * mainfrm_) {
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
