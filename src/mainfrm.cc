//
//  mainfrm.cc
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

#include "mainfrm.h"
#include "aven.h"
#include "aboutdlg.h"

#include "message.h"
#include "img.h"
#include "namecmp.h"

#include <wx/confbase.h>
#include <float.h>
#include <stack>
#include <regex.h>

const int NUM_DEPTH_COLOURS = 13;

#define TOOLBAR_BITMAP(file) wxBitmap(wxString(msg_cfgpth()) + wxCONFIG_PATH_SEPARATOR + wxString(file),\
                                      wxBITMAP_TYPE_PNG)

static const unsigned char REDS[]   = {190, 155, 110, 18, 0, 124, 48, 117, 163, 182, 224, 237, 255, 230};
static const unsigned char GREENS[] = {218, 205, 177, 153, 178, 211, 219, 224, 224, 193, 190, 117, 0, 230};
static const unsigned char BLUES[]  = {247, 255, 244, 237, 169, 175, 139, 40, 40, 17, 40, 18, 0, 230};

BEGIN_EVENT_TABLE(MainFrm, wxFrame)
    EVT_BUTTON(button_FIND, MainFrm::OnFind)
    EVT_BUTTON(button_HIDE, MainFrm::OnHide)

    EVT_MENU(menu_FILE_OPEN, MainFrm::OnOpen)
    EVT_MENU(menu_FILE_OPEN_PRES, MainFrm::OnOpenPres)
    EVT_MENU(menu_FILE_QUIT, MainFrm::OnQuit)

    EVT_MENU(menu_PRES_CREATE, MainFrm::OnPresCreate)
    EVT_MENU(menu_PRES_GO, MainFrm::OnPresGo)
    EVT_MENU(menu_PRES_GO_BACK, MainFrm::OnPresGoBack)
    EVT_MENU(menu_PRES_RESTART, MainFrm::OnPresRestart)
    EVT_MENU(menu_PRES_RECORD, MainFrm::OnPresRecord)
    EVT_MENU(menu_PRES_FINISH, MainFrm::OnPresFinish)
    EVT_MENU(menu_PRES_ERASE, MainFrm::OnPresErase)
    EVT_MENU(menu_PRES_ERASE_ALL, MainFrm::OnPresEraseAll)

    EVT_UPDATE_UI(menu_FILE_OPEN_PRES, MainFrm::OnOpenPresUpdate)

    EVT_UPDATE_UI(menu_PRES_CREATE, MainFrm::OnPresCreateUpdate)
    EVT_UPDATE_UI(menu_PRES_GO, MainFrm::OnPresGoUpdate)
    EVT_UPDATE_UI(menu_PRES_GO_BACK, MainFrm::OnPresGoBackUpdate)
    EVT_UPDATE_UI(menu_PRES_RESTART, MainFrm::OnPresRestartUpdate)
    EVT_UPDATE_UI(menu_PRES_RECORD, MainFrm::OnPresRecordUpdate)
    EVT_UPDATE_UI(menu_PRES_FINISH, MainFrm::OnPresFinishUpdate)
    EVT_UPDATE_UI(menu_PRES_ERASE, MainFrm::OnPresEraseUpdate)
    EVT_UPDATE_UI(menu_PRES_ERASE_ALL, MainFrm::OnPresEraseAllUpdate)

    EVT_CLOSE(MainFrm::OnClose)

    EVT_MENU(menu_ROTATION_START, MainFrm::OnStartRotation)
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
    EVT_MENU(menu_VIEW_SHOW_SURFACE, MainFrm::OnShowSurface)
    EVT_MENU(menu_VIEW_SURFACE_DEPTH, MainFrm::OnShowSurfaceDepth)
    EVT_MENU(menu_VIEW_SURFACE_DASHED, MainFrm::OnShowSurfaceDashed)
    EVT_MENU(menu_VIEW_COMPASS, MainFrm::OnViewCompass)
    EVT_MENU(menu_VIEW_CLINO, MainFrm::OnViewClino)
    EVT_MENU(menu_VIEW_GRID, MainFrm::OnViewGrid)
    EVT_MENU(menu_VIEW_DEPTH_BAR, MainFrm::OnToggleDepthbar)
    EVT_MENU(menu_VIEW_SCALE_BAR, MainFrm::OnToggleScalebar)
#ifdef AVENGL
    EVT_MENU(menu_FILE_OPEN_TERRAIN, MainFrm::OnFileOpenTerrain)
    EVT_MENU(menu_VIEW_ANTIALIAS, MainFrm::OnAntiAlias)
    EVT_MENU(menu_VIEW_SOLID_SURFACE, MainFrm::OnSolidSurface)
#endif
    EVT_MENU(menu_CTL_REVERSE, MainFrm::OnReverseControls)
    EVT_MENU(menu_HELP_ABOUT, MainFrm::OnAbout)

    EVT_UPDATE_UI(menu_ROTATION_START, MainFrm::OnStartRotationUpdate)
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
    EVT_UPDATE_UI(menu_VIEW_SURFACE_DEPTH, MainFrm::OnShowSurfaceDepthUpdate)
    EVT_UPDATE_UI(menu_VIEW_SURFACE_DASHED, MainFrm::OnShowSurfaceDashedUpdate)
    EVT_UPDATE_UI(menu_VIEW_SHOW_OVERLAPPING_NAMES, MainFrm::OnDisplayOverlappingNamesUpdate)
    EVT_UPDATE_UI(menu_VIEW_COMPASS, MainFrm::OnViewCompassUpdate)
    EVT_UPDATE_UI(menu_VIEW_CLINO, MainFrm::OnViewClinoUpdate)
    EVT_UPDATE_UI(menu_VIEW_DEPTH_BAR, MainFrm::OnToggleDepthbarUpdate)
    EVT_UPDATE_UI(menu_VIEW_SCALE_BAR, MainFrm::OnToggleScalebarUpdate)
    EVT_UPDATE_UI(menu_VIEW_GRID, MainFrm::OnViewGridUpdate)
    EVT_UPDATE_UI(menu_VIEW_INDICATORS, MainFrm::OnIndicatorsUpdate)
#ifdef AVENGL
    EVT_UPDATE_UI(menu_VIEW_ANTIALIAS, MainFrm::OnAntiAliasUpdate)
    EVT_UPDATE_UI(menu_VIEW_SOLID_SURFACE, MainFrm::OnSolidSurfaceUpdate)
#endif
    EVT_UPDATE_UI(menu_CTL_REVERSE, MainFrm::OnReverseControlsUpdate)
END_EVENT_TABLE()

class LabelCmp {
public:
    bool operator()(const LabelInfo* pt1, const LabelInfo* pt2) {
        return name_cmp(pt1->GetText(), pt2->GetText()) < 0;
    }
};

class LabelPlotCmp {
public:
    bool operator()(const LabelInfo* pt1, const LabelInfo* pt2) {
	int n;
	n = pt1->IsEntrance() - pt2->IsEntrance();
	if (n) return n > 0;
	n = pt1->IsFixedPt() - pt2->IsFixedPt();
	if (n) return n > 0;
	n = pt1->IsExportedPt() - pt2->IsExportedPt();
	if (n) return n > 0;
        wxString l1 = pt1->GetText().AfterLast('.');
	wxString l2 = pt2->GetText().AfterLast('.');
        return name_cmp(l1, l2) < 0;	
    }
};

class TreeData : public wxTreeItemData {
    LabelInfo* m_Label;

public:
    TreeData(LabelInfo* label) : m_Label(label) {}
    LabelInfo* GetLabel() { return m_Label; }
    bool IsStation() { return m_Label != NULL; }
};

MainFrm::MainFrm(const wxString& title, const wxPoint& pos, const wxSize& size) :
    wxFrame(NULL, 101, title, pos, size, wxDEFAULT_FRAME_STYLE | wxNO_FULL_REPAINT_ON_RESIZE),
    m_Gfx(NULL), m_FileToLoad(""), m_NumEntrances(0), m_NumFixedPts(0), m_NumExportedPts(0)
    
{
#ifdef _WIN32
    SetIcon(wxIcon("aaaaaAven")); // the peculiar name is so that the icon is the first in the file
                                  // (required by Windows for this type of icon)
#endif

    m_HighlightedPtValid = false;

    InitialisePensAndBrushes();
    CreateMenuBar();
    CreateAcceleratorTable();
    CreateToolBar();
    CreateSidePanel();

#ifdef __X__
    int x;
    int y;
    GetSize(&x, &y);
    // X seems to require a forced resize.
    SetSize(-1, -1, x, y);
#endif

    m_PresLoaded = false;
    m_Recording = false;

    // Set regular expression syntax.
    re_set_syntax(RE_SYNTAX_POSIX_EXTENDED);
}

MainFrm::~MainFrm()
{
    ClearPointLists();
    delete[] m_Points;
}

void MainFrm::InitialisePensAndBrushes()
{
    m_Points = new list<PointInfo*>[NUM_DEPTH_COLOURS+1];
    m_Pens = new wxPen[NUM_DEPTH_COLOURS+1];
    m_Brushes = new wxBrush[NUM_DEPTH_COLOURS+1];
    for (int pen = 0; pen < NUM_DEPTH_COLOURS+1; pen++) {
        m_Pens[pen].SetColour(REDS[pen], GREENS[pen], BLUES[pen]);
        m_Brushes[pen].SetColour(REDS[pen], GREENS[pen], BLUES[pen]);
    }
}

void MainFrm::CreateMenuBar()
{
    // Create the menus and the menu bar.

    // The status bar messages aren't internationalised, 'cos there ain't any
    // status bar at the moment! ;-)
    wxMenu* filemenu = new wxMenu;
    filemenu->Append(menu_FILE_OPEN, GetTabMsg(/*@Open...##Ctrl+O*/220), "Open a Survex 3D file for viewing");
    filemenu->Append(menu_FILE_OPEN_PRES, GetTabMsg(/*Open @Presentation...*/321), "unused");
#ifdef AVENGL
    filemenu->Append(menu_FILE_OPEN_TERRAIN, GetTabMsg(/*Open @Terrain...*/329), "unused");
#endif
    filemenu->AppendSeparator();
    filemenu->Append(menu_FILE_QUIT, GetTabMsg(/*@Exit*/221), "Quit Aven");

    wxMenu* rotmenu = new wxMenu;
    rotmenu->Append(menu_ROTATION_START, GetTabMsg(/*@Start Rotation##Return*/230), "Start the survey rotating");
    rotmenu->Append(menu_ROTATION_STOP, GetTabMsg(/*S@top Rotation##Space*/231), "Stop the survey rotating");
    rotmenu->AppendSeparator();
    rotmenu->Append(menu_ROTATION_SPEED_UP, GetTabMsg(/*Speed @Up##Z*/232), "Speed up rotation of the survey");
    rotmenu->Append(menu_ROTATION_SLOW_DOWN, GetTabMsg(/*S@low Down##X*/233), "Slow down rotation of the survey");
    rotmenu->AppendSeparator();
    rotmenu->Append(menu_ROTATION_REVERSE, GetTabMsg(/*@Reverse Direction##R*/234), "Reverse the direction of rotation");
    rotmenu->AppendSeparator();
    rotmenu->Append(menu_ROTATION_STEP_CCW, GetTabMsg(/*Step Once @Anticlockwise##C*/235), "Rotate the cave one step anticlockwise");
    rotmenu->Append(menu_ROTATION_STEP_CW, GetTabMsg(/*Step Once @Clockwise##V*/236), "Rotate the cave one step clockwise");

    wxMenu* orientmenu = new wxMenu;
    orientmenu->Append(menu_ORIENT_MOVE_NORTH, GetTabMsg(/*View @North##N*/240), "Move the survey so it aims North");
    orientmenu->Append(menu_ORIENT_MOVE_EAST, GetTabMsg(/*View @East##E*/241), "Move the survey so it aims East");
    orientmenu->Append(menu_ORIENT_MOVE_SOUTH, GetTabMsg(/*View @South##S*/242), "Move the survey so it aims South");
    orientmenu->Append(menu_ORIENT_MOVE_WEST, GetTabMsg(/*View @West##W*/243), "Move the survey so it aims West");
    orientmenu->AppendSeparator();
    orientmenu->Append(menu_ORIENT_SHIFT_LEFT, GetTabMsg(/*Shift Survey @Left##Left Arrow*/244), "Shift the survey to the left");
    orientmenu->Append(menu_ORIENT_SHIFT_RIGHT, GetTabMsg(/*Shift Survey @Right##Right Arrow*/245), "Shift the survey to the right");
    orientmenu->Append(menu_ORIENT_SHIFT_UP, GetTabMsg(/*Shift Survey @Up##Up Arrow*/246), "Shift the survey upwards");
    orientmenu->Append(menu_ORIENT_SHIFT_DOWN, GetTabMsg(/*Shift Survey @Down##Down Arrow*/247), "Shift the survey downwards");
    orientmenu->AppendSeparator();
    orientmenu->Append(menu_ORIENT_PLAN, GetTabMsg(/*@Plan View##P*/248), "Switch to plan view");
    orientmenu->Append(menu_ORIENT_ELEVATION, GetTabMsg(/*Ele@vation View##L*/249), "Switch to elevation view");
    orientmenu->AppendSeparator();
    orientmenu->Append(menu_ORIENT_HIGHER_VP, GetTabMsg(/*@Higher Viewpoint##'*/250), "Raise the angle of viewing");
    orientmenu->Append(menu_ORIENT_LOWER_VP, GetTabMsg(/*Lo@wer Viewpoint##/ */251), "Lower the angle of viewing");
    orientmenu->AppendSeparator();
    orientmenu->Append(menu_ORIENT_ZOOM_IN, GetTabMsg(/*@Zoom In##]*/252), "Zoom further into the survey");
    orientmenu->Append(menu_ORIENT_ZOOM_OUT, GetTabMsg(/*Zoo@m Out##[*/253), "Zoom further out from the survey");
    orientmenu->AppendSeparator();
    orientmenu->Append(menu_ORIENT_DEFAULTS, GetTabMsg(/*Restore De@fault Settings##Delete*/254), "Restore default settings for zoom, scale and rotation");

    wxMenu* viewmenu = new wxMenu;
    viewmenu->Append(menu_VIEW_SHOW_NAMES, GetTabMsg(/*Station @Names##Ctrl+N*/270), "Toggle display of survey station names", true);
    viewmenu->Append(menu_VIEW_SHOW_CROSSES, GetTabMsg(/*@Crosses##Ctrl+X*/271), "Toggle display of crosses at survey stations", true);
    viewmenu->Append(menu_VIEW_GRID, GetTabMsg(/*@Grid##Ctrl+G*/297), "unused", true);
    viewmenu->AppendSeparator();
    viewmenu->Append(menu_VIEW_SHOW_LEGS, GetTabMsg(/*Underground Survey @Legs##Ctrl+L*/272), "Toggle display of survey legs", true);
    viewmenu->Append(menu_VIEW_SHOW_SURFACE, GetTabMsg(/*Sur@face Survey Legs##Ctrl+F*/291), "unused", true);
    viewmenu->AppendSeparator();
    viewmenu->Append(menu_VIEW_SURFACE_DEPTH, GetTabMsg(/*Depth Colours on Surface Surveys*/292), "unused", true);
    viewmenu->Append(menu_VIEW_SURFACE_DASHED, GetTabMsg(/*Dashed Surface Surveys*/293), "unused", true);
    viewmenu->AppendSeparator();
    viewmenu->Append(menu_VIEW_SHOW_OVERLAPPING_NAMES, GetTabMsg(/*@Overlapping Names##O*/273), "Toggle display all station names, whether or not they overlap", true);
    viewmenu->AppendSeparator();
    viewmenu->Append(menu_VIEW_SHOW_ENTRANCES, GetTabMsg(/*Highlight @Entrances*/294), "unused", true);
    viewmenu->Append(menu_VIEW_SHOW_FIXED_PTS, GetTabMsg(/*Highlight Fi@xed Points*/295), "unused", true);
    viewmenu->Append(menu_VIEW_SHOW_EXPORTED_PTS, GetTabMsg(/*Highlight Ex&ported Points*/296), "unused", true);
    viewmenu->AppendSeparator();
    wxMenu* presmenu = new wxMenu;
    presmenu->Append(menu_PRES_CREATE, GetTabMsg(/*@Create...*/311), "unused", false);
    presmenu->AppendSeparator();
    presmenu->Append(menu_PRES_GO, GetTabMsg(/*@Go*/312), "unused", false);
    presmenu->Append(menu_PRES_GO_BACK, GetTabMsg(/*@Go Back*/318), "unused", false);
    presmenu->Append(menu_PRES_RESTART, GetTabMsg(/*Res@tart*/324), "unused", false);
    presmenu->AppendSeparator();
    presmenu->Append(menu_PRES_RECORD, GetTabMsg(/*@Record Position*/313), "unused", false);
    presmenu->Append(menu_PRES_FINISH, GetTabMsg(/*@Finish and Save*/314), "unused", false);
    presmenu->AppendSeparator();
    presmenu->Append(menu_PRES_ERASE, GetTabMsg(/*@Erase Last Position*/315), "unused", false);
    presmenu->Append(menu_PRES_ERASE_ALL, GetTabMsg(/*Er@ase All Positions*/316), "unused", false);
    wxMenu* indmenu = new wxMenu;
    indmenu->Append(menu_VIEW_COMPASS, GetTabMsg(/*Co@mpass*/274), "Toggle display of the compass", true);
    indmenu->Append(menu_VIEW_CLINO, GetTabMsg(/*Cl@inometer*/275), "Toggle display of the clinometer", true);
    indmenu->Append(menu_VIEW_DEPTH_BAR, GetTabMsg(/*@Depth Bar*/276), "Toggle display of the depth bar", true);
    indmenu->Append(menu_VIEW_SCALE_BAR, GetTabMsg(/*Sc@ale Bar*/277), "Toggle display of the scale bar", true);
    viewmenu->Append(menu_VIEW_INDICATORS, GetTabMsg(/*@Indicators*/299), indmenu, "");
#ifdef AVENGL
    viewmenu->AppendSeparator();
    viewmenu->Append(menu_VIEW_ANTIALIAS, GetTabMsg(/*S@moothed Survey Legs*/298), "unused", true);
    viewmenu->AppendSeparator();
    viewmenu->Append(menu_VIEW_SOLID_SURFACE, GetTabMsg(/*So@lid Surface*/330), "unused", true);
#endif

    wxMenu* ctlmenu = new wxMenu;
    ctlmenu->Append(menu_CTL_REVERSE, GetTabMsg(/*@Reverse Sense##Ctrl+R*/280), "Reverse the sense of the orientation controls", true);

    wxMenu* helpmenu = new wxMenu;
    helpmenu->Append(menu_HELP_ABOUT, GetTabMsg(/*@About Aven...*/290), "Display program information, version number, copyright and licence agreement");

    wxMenuBar* menubar = new wxMenuBar(wxMB_DOCKABLE);
    menubar->Append(filemenu, GetTabMsg(/*@File*/210));
    menubar->Append(rotmenu, GetTabMsg(/*@Rotation*/211));
    menubar->Append(orientmenu, GetTabMsg(/*@Orientation*/212));
    menubar->Append(viewmenu, GetTabMsg(/*@View*/213));
    menubar->Append(presmenu, GetTabMsg(/*@Presentation*/317));
    menubar->Append(ctlmenu, GetTabMsg(/*@Controls*/214));
    menubar->Append(helpmenu, GetTabMsg(/*@Help*/215));
    SetMenuBar(menubar);
}

void MainFrm::CreateToolBar()
{
    // Create the toolbar.

    wxToolBar* toolbar = wxFrame::CreateToolBar();

    toolbar->AddTool(menu_FILE_OPEN, TOOLBAR_BITMAP("open.png"), "Open a 3D file for viewing");
    toolbar->AddTool(menu_FILE_OPEN_PRES, TOOLBAR_BITMAP("open-pres.png"), "Open a presentation");
    toolbar->AddSeparator();
    toolbar->AddTool(menu_ROTATION_TOGGLE, TOOLBAR_BITMAP("rotation.png"), wxNullBitmap, true,
                     -1, -1, NULL, "Toggle rotation");
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
    toolbar->AddSeparator();
#ifdef AVENGL
    toolbar->AddTool(menu_VIEW_SOLID_SURFACE, TOOLBAR_BITMAP("solid-surface.png"), wxNullBitmap, true,
                     -1, -1, NULL, "Show solid surface");
    toolbar->AddSeparator();
#endif
    toolbar->AddTool(menu_PRES_CREATE, TOOLBAR_BITMAP("pres-create.png"),
                     "Create a new presentation");
    toolbar->AddTool(menu_PRES_RECORD, TOOLBAR_BITMAP("pres-record.png"),
                     "Record a presentation step");
    toolbar->AddTool(menu_PRES_FINISH, TOOLBAR_BITMAP("pres-finish.png"),
                     "Finish this presentation and save it to disk");
    toolbar->AddSeparator();
    toolbar->AddTool(menu_PRES_RESTART, TOOLBAR_BITMAP("pres-restart.png"),
                     "Go to the start of the presentation");
    toolbar->AddTool(menu_PRES_GO_BACK, TOOLBAR_BITMAP("pres-go-back.png"),
                     "Go back one presentation step");
    toolbar->AddTool(menu_PRES_GO, TOOLBAR_BITMAP("pres-go.png"), "Go forwards one presentation step");

    toolbar->Realize();
}

void MainFrm::CreateAcceleratorTable()
{
    // Create the accelerator key table.

    wxAcceleratorEntry entries[11];
    entries[0].Set(wxACCEL_NORMAL, WXK_DELETE, menu_ORIENT_DEFAULTS);
    entries[1].Set(wxACCEL_NORMAL, WXK_UP, menu_ORIENT_SHIFT_UP);
    entries[2].Set(wxACCEL_NORMAL, WXK_DOWN, menu_ORIENT_SHIFT_DOWN);
    entries[3].Set(wxACCEL_NORMAL, WXK_LEFT, menu_ORIENT_SHIFT_LEFT);
    entries[4].Set(wxACCEL_NORMAL, WXK_RIGHT, menu_ORIENT_SHIFT_RIGHT);
    entries[5].Set(wxACCEL_NORMAL, (int) '\'', menu_ORIENT_HIGHER_VP);
    entries[6].Set(wxACCEL_NORMAL, (int) '/', menu_ORIENT_LOWER_VP);
    entries[7].Set(wxACCEL_NORMAL, (int) ']', menu_ORIENT_ZOOM_IN);
    entries[8].Set(wxACCEL_NORMAL, (int) '[', menu_ORIENT_ZOOM_OUT);
    entries[9].Set(wxACCEL_NORMAL, WXK_RETURN, menu_ROTATION_START);
    entries[10].Set(wxACCEL_NORMAL, WXK_SPACE, menu_ROTATION_STOP);

    wxAcceleratorTable accel(11, entries);
    SetAcceleratorTable(accel);
}

void MainFrm::CreateSidePanel()
{
    m_Splitter = new wxSplitterWindow(this, -1, wxDefaultPosition, wxDefaultSize,
                                      wxSP_3D | wxSP_LIVE_UPDATE);
    m_Panel = new wxPanel(m_Splitter);
    m_Tree = new AvenTreeCtrl(this, m_Panel);
    m_FindPanel = new wxPanel(m_Panel);
    m_Panel->Show(false);

    m_FindButton = new wxButton(m_FindPanel, button_FIND, "Find");
    m_FindButton->SetDefault();
    m_FindPanel->SetDefaultItem(m_FindButton);
    m_HideButton = new wxButton(m_FindPanel, button_HIDE, "Hide");
    m_RegexpCheckBox = new wxCheckBox(m_FindPanel, -1, "Regular expression");
    m_Coords = new wxStaticText(m_FindPanel, -1, "");
    m_StnCoords = new wxStaticText(m_FindPanel, -1, "");
    //  m_MousePtr = new wxStaticText(m_FindPanel, -1, "Mouse coordinates");
    m_StnName = new wxStaticText(m_FindPanel, -1, "");
    m_StnAlt = new wxStaticText(m_FindPanel, -1, "");
    m_Dist1 = new wxStaticText(m_FindPanel, -1, "");
    m_Dist2 = new wxStaticText(m_FindPanel, -1, "");
    m_Dist3 = new wxStaticText(m_FindPanel, -1, "");

    m_FindButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_FindButtonSizer->Add(m_FindButton, 1, wxALL, 2);
    m_FindButtonSizer->Add(m_HideButton, 1, wxALL, 2);

    m_FindSizer = new wxBoxSizer(wxVERTICAL);
    m_FindSizer->Add(m_FindBox = new wxTextCtrl(m_FindPanel, -1, ""), 0, wxALL | wxEXPAND, 2);
    m_FindSizer->Add(m_FindButtonSizer, 0, wxALL | wxEXPAND, 2);
    m_FindSizer->Add(m_RegexpCheckBox, 0, wxALL | wxEXPAND, 2);
    m_FindSizer->Add(10, 5, 0, wxALL | wxEXPAND, 2);
    //   m_FindSizer->Add(m_MousePtr, 0, wxALL | wxEXPAND, 2);
    m_FindSizer->Add(m_Coords, 0, wxALL | wxEXPAND, 2);
    m_FindSizer->Add(10, 5, 0, wxALL | wxEXPAND, 2);
    m_FindSizer->Add(m_StnName, 0, wxALL | wxEXPAND, 2);
    m_FindSizer->Add(m_StnCoords, 0, wxALL | wxEXPAND, 2);
    m_FindSizer->Add(m_StnAlt, 0, wxALL | wxEXPAND, 2);
    m_FindSizer->Add(10, 5, 0, wxALL | wxEXPAND, 2);
    m_FindSizer->Add(m_Dist1, 0, wxALL | wxEXPAND, 2);
    m_FindSizer->Add(m_Dist2, 0, wxALL | wxEXPAND, 2);
    m_FindSizer->Add(m_Dist3, 0, wxALL | wxEXPAND, 2);

    m_FindPanel->SetAutoLayout(true);
    m_FindPanel->SetSizer(m_FindSizer);

    m_PanelSizer = new wxBoxSizer(wxVERTICAL);
    m_PanelSizer->Add(m_Tree, 3, wxALL | wxEXPAND, 2);
    m_PanelSizer->Add(m_FindPanel, 2, wxALL | wxEXPAND, 2);
    m_Panel->SetAutoLayout(true);
    m_Panel->SetSizer(m_PanelSizer);
    m_PanelSizer->Fit(m_Panel);
    m_PanelSizer->SetSizeHints(m_Panel);

    m_Gfx = new GfxCore(this, m_Splitter);

    m_Splitter->Initialize(m_Gfx);
}

void MainFrm::ClearPointLists()
{
    // Free memory occupied by the contents of the point and label lists.

    for (int band = 0; band < NUM_DEPTH_COLOURS + 1; band++) {
        list<PointInfo*>::iterator pos = m_Points[band].begin();
        list<PointInfo*>::iterator end = m_Points[band].end();
        while (pos != end) {
            PointInfo* point = *pos++;
            delete point;
        }
        m_Points[band].clear();
    }

    list<LabelInfo*>::iterator pos = m_Labels.begin();
    while (pos != m_Labels.end()) {
        LabelInfo* label = *pos++;
        delete label;
    }
    m_Labels.clear();
}

bool MainFrm::LoadData(const wxString& file, wxString prefix)
{
    // Load survey data from file, centre the dataset around the origin,
    // chop legs such that no legs cross depth colour boundaries and prepare
    // the data for drawing.

    // Load the survey data.

    img* survey = img_open_survey(file, NULL, NULL, prefix.c_str());
    if (!survey) {
        wxString m = wxString::Format(msg(img_error()), file.c_str());
        wxGetApp().ReportError(m);
        return false;
    }

    m_File = file;

    m_TreeRoot = m_Tree->AddRoot(wxFileNameFromPath(file));
    m_Tree->SetEnabled();

    // Create a list of all the leg vertices, counting them and finding the
    // extent of the survey at the same time.

    m_NumLegs = 0;
    m_NumPoints = 0;
    m_NumExtraLegs = 0;
    m_NumCrosses = 0;
    m_NumFixedPts = 0;
    m_NumExportedPts = 0;
    m_NumEntrances = 0;

    m_PresLoaded = false;
    m_Recording = false;
    //--Pres: FIXME: discard existing one, ask user about saving

    // Delete any existing list entries.
    ClearPointLists();

    Double xmin = DBL_MAX;
    Double xmax = -DBL_MAX;
    Double ymin = DBL_MAX;
    Double ymax = -DBL_MAX;
    m_ZMin = DBL_MAX;
    Double zmax = -DBL_MAX;

    list<PointInfo*> points;

    int result;
    do {
        img_point pt;
        result = img_read_item(survey, &pt);
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

                PointInfo* info = new PointInfo;
                info->x = pt.x;
                info->y = pt.y;
                info->z = pt.z;

                if (result == img_LINE) {
                    // Set flags to say this is a line rather than a move
                    m_NumLegs++;
                    info->isLine = true;
                    info->isSurface = (survey->flags & img_FLAG_SURFACE);
                } else {
                    info->isLine = false;
                }

                // Store this point in the list.
                points.push_back(info);

                break;
            }

            case img_LABEL:
            {
                LabelInfo* label = new LabelInfo;
                label->text = survey->label;
                label->x = pt.x;
                label->y = pt.y;
                label->z = pt.z;
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
                m_NumLegs = 0;
                m_NumPoints = 0;
                m_NumExtraLegs = 0;
                m_NumCrosses = 0;
                m_NumFixedPts = 0;
                m_NumExportedPts = 0;
                m_NumEntrances = 0;

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

    img_close(survey);

    // Check we've actually loaded some legs or stations!
    if (m_NumLegs == 0 && m_Labels.empty()) {
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
        PointInfo* pt = points.back();
        if (!pt->isLine) {
            m_NumPoints--;
            points.pop_back();
            delete pt;
        }
    }

    m_XExt = xmax - xmin;
    m_YExt = ymax - ymin;
    m_ZExt = zmax - m_ZMin;

    // FIXME -- temporary bodge
    m_XMin = xmin;
    m_YMin = ymin;

    // Sort the labels.
    m_Labels.sort(LabelCmp());

    // Fill the tree of stations and prefixes.
    FillTree();
    m_Tree->Expand(m_TreeRoot);

    // Sort labels so that entrances are displayed in preference,
    // then fixed points, then exported points, then other points.
    //
    // Also sort by leaf name so that we'll tend to choose labels
    // from different surveys, rather than labels from surveys which
    // are earlier in the list.
    m_Labels.sort(LabelPlotCmp());

    // Sort out depth colouring boundaries (before centering dataset!)
    SortIntoDepthBands(points);

    // Centre the dataset around the origin.
    CentreDataset(xmin, ymin, m_ZMin);

    // Update window title.
    SetTitle(wxString("Aven - [") + file + wxString("]"));
    m_File = file;

    return true;
}

#ifdef AVENGL
bool MainFrm::LoadTerrain(const wxString& file)
{
    // Load terrain data from a 3D file (temporary bodge).

    img* survey = img_open_survey(file, NULL, NULL, "");
    if (!survey) {
        wxString m = wxString::Format(msg(img_error()), file.c_str());
        wxGetApp().ReportError(m);
        return false;
    }

    //--FIXME: need to be specified properly
    m_TerrainSize.x = 40;
    m_TerrainSize.y = 40;

    m_TerrainExtents.xmin = DBL_MAX;
    m_TerrainExtents.xmax = -DBL_MAX;
    m_TerrainExtents.ymin = DBL_MAX;
    m_TerrainExtents.ymax = -DBL_MAX;
    m_TerrainExtents.zmin = DBL_MAX;
    m_TerrainExtents.zmax = -DBL_MAX;

    m_TerrainGrid = new Double[m_TerrainSize.x * m_TerrainSize.y];

    int result;
    do {
        img_point pt;
        result = img_read_item(survey, &pt);
        switch (result) {
            case img_MOVE:
            case img_LINE:
            {
                // Update survey extents.
                if (pt.x < m_TerrainExtents.xmin) m_TerrainExtents.xmin = pt.x;
                if (pt.x > m_TerrainExtents.xmax) m_TerrainExtents.xmax = pt.x;
                if (pt.y < m_TerrainExtents.ymin) m_TerrainExtents.ymin = pt.y;
                if (pt.y > m_TerrainExtents.ymax) m_TerrainExtents.ymax = pt.y;
                if (pt.z < m_TerrainExtents.zmin) m_TerrainExtents.zmin = pt.z;
                if (pt.z > m_TerrainExtents.zmax) m_TerrainExtents.zmax = pt.z;

                break;
            }

            case img_BAD:
	    	assert(0);
                break;

            default:
                break;
        }
    } while (result != img_STOP);

    img_rewind(survey);

    Double xext = m_TerrainExtents.xmax - m_TerrainExtents.xmin;
    Double yext = m_TerrainExtents.ymax - m_TerrainExtents.ymin;

    do {
        img_point pt;
        result = img_read_item(survey, &pt);
        switch (result) {
            case img_LABEL:
            {
		int x = int(floor(0.5 + ((pt.x - m_TerrainExtents.xmin) * (m_TerrainSize.x - 2) / xext)));
		int y = int(floor(0.5 + ((pt.y - m_TerrainExtents.ymin) * (m_TerrainSize.y - 1) / yext)));

		m_TerrainGrid[x + m_TerrainSize.x * y] = pt.z - m_Offsets.z;
		
                break;
            }

            case img_BAD:
	    	assert(0);
                break;

            default:
                break;
        }
    } while (result != img_STOP);

    for (int i = 0; i < m_TerrainSize.y; i++)
        m_TerrainGrid[m_TerrainSize.x-1 + m_TerrainSize.y*i] =
            m_TerrainGrid[m_TerrainSize.x-2 + m_TerrainSize.y*i];

    img_close(survey);
    
    m_TerrainExtents.xmin -= m_Offsets.x;
    m_TerrainExtents.xmax -= m_Offsets.x;
    m_TerrainExtents.ymin -= m_Offsets.y;
    m_TerrainExtents.ymax -= m_Offsets.y;

    return true;
}
#endif

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
        wxString prefix = label->GetText().BeforeLast('.');

        // Determine if we're still on the same prefix.
        if (prefix == current_prefix) {
            // no need to fiddle with branches...
        }
        // If not, then see if we've descended to a new prefix.
        else if (prefix.Length() > current_prefix.Length() && prefix.StartsWith(current_prefix) &&
                 (prefix[current_prefix.Length()] == '.' || current_prefix == "")) {
            // We have, so start as many new branches as required.
            int current_prefix_length = current_prefix.Length();
            current_prefix = prefix;
            if (current_prefix_length != 0) {
                prefix = prefix.Mid(current_prefix_length + 1);
            }
            int next_dot;
            do {
                // Extract the next bit of prefix.
                next_dot = prefix.Find('.');

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
            int count = 0;
            bool ascent_only = (prefix.Length() < current_prefix.Length() && current_prefix.StartsWith(prefix) &&
                                (current_prefix[prefix.Length()] == '.' || prefix == ""));
            if (!ascent_only) {
                // Find out how much of the current prefix and the new prefix are the same.
                // Note that we require a match of a whole number of parts between dots!
                int pos = 0;
                while (prefix[pos] == current_prefix[pos]) {
                    if (prefix[pos] == '.') count = pos + 1;
                    pos++;
                }
            }
            else {
                count = prefix.Length() + 1;
            }

            // Extract the part of the current prefix after the bit (if any) which has matched.
            // This gives the prefixes to ascend over.
            wxString prefixes_ascended = current_prefix.Mid(count);

            // Count the number of prefixes to ascend over.
            int num_prefixes = prefixes_ascended.Freq('.') + 1;

            // Reverse up over these prefixes.
            for (int i = 1; i <= num_prefixes; i++) {
                current_id = previous_ids.top();
                previous_ids.pop();
            }

            if (!ascent_only) {
                // Now extract the bit of new prefix.
                wxString new_prefix = prefix.Mid(count);
               
                // Add branches for this new part.
                int next_dot;
                while (1) {
                    // Extract the next bit of prefix.
                    next_dot = new_prefix.Find('.');

                    wxString bit = next_dot == -1 ? new_prefix : new_prefix.Left(next_dot);

                    // Add the current tree ID to the stack.
                    previous_ids.push(current_id);

                    // Append the new item to the tree and set this as the current branch.
                    current_id = m_Tree->AppendItem(current_id, bit);
                    m_Tree->SetItemData(current_id, new TreeData(NULL));

                    if (next_dot == -1) {
                        break;
                    }

                    new_prefix = new_prefix.Mid(next_dot + 1);
                }
            }

            current_prefix = prefix;
        }

        // Now add the leaf
        wxString bit = label->GetText().AfterLast('.');
        assert(bit != "");
        wxTreeItemId id = m_Tree->AppendItem(current_id, bit);
        m_Tree->SetItemData(id, new TreeData(label));
        SetTreeItemColour(id, label);
    }
}

void MainFrm::SetTreeItemColour(wxTreeItemId& id, LabelInfo* label)
{
    // Set the colour for an item in the survey tree.

    if (label->IsSurface()) {
        m_Tree->SetItemTextColour(id, wxColour(49, 158, 79));
    }

    if (label->IsEntrance()) {
        m_Tree->SetItemTextColour(id, wxColour(255, 0, 0));
    }
}

void MainFrm::CentreDataset(Double xmin, Double ymin, Double zmin)
{
    // Centre the dataset around the origin.

    Double xoff = m_Offsets.x = xmin + (m_XExt / 2.0f);
    Double yoff = m_Offsets.y = ymin + (m_YExt / 2.0f);
    Double zoff = m_Offsets.z = zmin + (m_ZExt / 2.0f);
    
    for (int band = 0; band < NUM_DEPTH_COLOURS + 1; band++) {
        list<PointInfo*>::iterator pos = m_Points[band].begin();
        list<PointInfo*>::iterator end = m_Points[band].end();
        while (pos != end) {
            PointInfo* point = *pos++;
            point->x -= xoff;
            point->y -= yoff;
            point->z -= zoff;
        }
    }

    list<LabelInfo*>::iterator pos = m_Labels.begin();
    while (pos != m_Labels.end()) {
        LabelInfo* label = *pos++;
        label->x -= xoff;
        label->y -= yoff;
        label->z -= zoff;
    }
}

int MainFrm::GetDepthColour(Double z)
{
    // Return the (0-based) depth colour band index for a z-coordinate.
    return int(((z - m_ZMin) / (m_ZExt == 0.0f ? 1.0f : m_ZExt)) * (NUM_DEPTH_COLOURS - 1));
}

Double MainFrm::GetDepthBoundaryBetweenBands(int a, int b)
{
    // Return the z-coordinate of the depth colour boundary between
    // two adjacent depth colour bands (specified by 0-based indices).

    assert((a == b - 1) || (a == b + 1));

    int band = (a > b) ? a : b; // boundary N lies on the bottom of band N.
    return m_ZMin + (m_ZExt * band / (NUM_DEPTH_COLOURS - 1));
}

void MainFrm::IntersectLineWithPlane(Double x0, Double y0, Double z0,
                                     Double x1, Double y1, Double z1,
                                     Double z, Double& x, Double& y)
{
    // Find the intersection point of the line (x0, y0, z0) -> (x1, y1, z1) with
    // the plane parallel to the xy-plane with z-axis intersection z.

    Double t = (z - z0) / (z1 - z0);
    x = x0 + t*(x1 - x0);
    y = y0 + t*(y1 - y0);
}

void MainFrm::SortIntoDepthBands(list<PointInfo*>& points)
{
    // Split legs which cross depth colouring boundaries and classify all
    // points into the correct depth bands.

    list<PointInfo*>::iterator pos = points.begin();
    PointInfo* prev_point = NULL;
    while (pos != points.end()) {
        PointInfo* point = *pos++;
        assert(point);

        // If this is a leg, then check if it intersects a depth
        // colour boundary.
        if (prev_point && point->isLine) {
            int col1 = GetDepthColour(prev_point->z);
            int col2 = GetDepthColour(point->z);
            if (col1 != col2) {
                // The leg does cross at least one boundary, so split it as
                // many times as required...
                int inc = (col1 > col2) ? -1 : 1;
                for (int band = col1; band != col2; band += inc) {
                    int next_band = band + inc;

                    // Determine the z-coordinate of the boundary being intersected.
                    Double split_at_z = GetDepthBoundaryBetweenBands(band, next_band);
                    Double split_at_x, split_at_y;

                    // Find the coordinates of the intersection point.
                    IntersectLineWithPlane(prev_point->x, prev_point->y, prev_point->z,
                                           point->x, point->y, point->z,
                                           split_at_z, split_at_x, split_at_y);
                    
                    // Create a new leg only as far as this point.
                    PointInfo* info = new PointInfo;
                    info->x = split_at_x;
                    info->y = split_at_y;
                    info->z = split_at_z;
                    info->isLine = true;
                    info->isSurface = point->isSurface;
                    m_Points[band].push_back(info);

                    // Create a move to this point in the next band.
                    info = new PointInfo;
                    info->x = split_at_x;
                    info->y = split_at_y;
                    info->z = split_at_z;
                    info->isLine = false;
                    info->isSurface = point->isSurface;
                    m_Points[next_band].push_back(info);
                    
                    m_NumExtraLegs++;
                    m_NumPoints += 2;
                }
            }

            // Add the last point of the (possibly split) leg.
            m_Points[col2].push_back(point);
        }
        else {
            // The first point, a surface point, or another move: put it in the correct list
            // according to depth.
            assert(point->isSurface || !point->isLine);
            int band = GetDepthColour(point->z);
            m_Points[band].push_back(point);
        }

        prev_point = point;
    }
}

void MainFrm::OpenFile(const wxString& file, wxString survey, bool delay)
{
    SetCursor(*wxHOURGLASS_CURSOR);
    if (LoadData(file, survey)) {
        m_HighlightedPtValid = false;

        if (delay) {
            m_Gfx->InitialiseOnNextResize();
        }
        else {
            m_Gfx->Initialise();
        }

#ifndef AVENGL
        m_Panel->Show(true);
        int x;
        int y;
        GetSize(&x, &y);
        m_Splitter->SplitVertically(m_Panel, m_Gfx, x / 4);
#endif
    }
    SetCursor(*wxSTANDARD_CURSOR);
}

#ifdef AVENGL
void MainFrm::OpenTerrain(const wxString& file)
{
    SetCursor(*wxHOURGLASS_CURSOR);
    if (LoadTerrain(file)) {
    	m_Gfx->InitialiseTerrain();
    }
    SetCursor(*wxSTANDARD_CURSOR);
}
#endif

//
//  UI event handlers
//

void MainFrm::OnOpen(wxCommandEvent&)
{
#ifdef __WXMOTIF__
    wxFileDialog dlg (this, wxString(msg(/*Select a 3D file to view*/206)), "", "",
                      "*.3d", wxOPEN);
#else
    wxFileDialog dlg (this, wxString(msg(/*Select a 3D file to view*/206)), "", "",
                      wxString::Format("%s|*.3d|%s|*.*",
                                       msg(/*Survex 3d files*/207),
                                       msg(/*All files*/208)), wxOPEN);
#endif
    if (dlg.ShowModal() == wxID_OK) {
        OpenFile(dlg.GetPath(), "", false);
    }
}

#ifdef AVENGL
void MainFrm::OnFileOpenTerrain(wxCommandEvent&)
{
#ifdef __WXMOTIF__
    wxFileDialog dlg (this, wxString(msg(/*Select a terrain file to view*/206)), "", "",
                      "*.3d", wxOPEN);
#else
    wxFileDialog dlg (this, wxString(msg(/*Select a terrain file to view*/206)), "", "",
                      wxString::Format("%s|*.3d|%s|*.*",
                                       msg(/*Terrain files*/207),
                                       msg(/*All files*/208)), wxOPEN);
#endif
    if (dlg.ShowModal() == wxID_OK) {
        OpenTerrain(dlg.GetPath());
    }
}
#endif

void MainFrm::OnQuit(wxCommandEvent&)
{
    exit(0);
}

void MainFrm::OnClose(wxCloseEvent&)
{
    exit(0);
}

void MainFrm::OnAbout(wxCommandEvent&)
{
    wxDialog* dlg = new AboutDlg(this);
    dlg->Centre();
    dlg->ShowModal();
}

void MainFrm::GetColour(int band, Double& r, Double& g, Double& b)
{
    assert(band >= 0 && band < NUM_DEPTH_COLOURS);
    r = Double(REDS[band]) / 255.0;
    g = Double(GREENS[band]) / 255.0;
    b = Double(BLUES[band]) / 255.0;
}

void MainFrm::ClearCoords()
{
    m_Coords->SetLabel("   -");
}

void MainFrm::SetCoords(Double x, Double y)
{
    wxString str;
    str.Printf("At: %d N, %d E", (int) y, (int) x);
    m_Coords->SetLabel(str);
}

void MainFrm::DisplayTreeInfo(wxTreeItemData* item)
{
    TreeData* data = (TreeData*) item;
    
    if (data && data->IsStation()) {
        LabelInfo* label = data->GetLabel();
        wxString str;
        str.Printf("   %d N, %d E", (int) (label->y + m_Offsets.y), (int) (label->x + m_Offsets.x));
        m_StnCoords->SetLabel(str);
        m_StnName->SetLabel(label->text);
        str.Printf("   Altitude: %dm", (int) (label->z + m_Offsets.z));
        m_StnAlt->SetLabel(str);
        if (m_HighlightedPtValid) {
            m_Gfx->DeleteSpecialPoint(m_HighlightedPt);
            m_HighlightedPtValid = false;
        }
        m_HighlightedPt = m_Gfx->AddSpecialPoint(label->x, label->y, label->z, col_WHITE);
        m_Gfx->DisplaySpecialPoints();
        m_HighlightedPtValid = true;

        wxTreeItemData* sel_wx;
        bool sel = m_Tree->GetSelectionData(&sel_wx);
        if (sel) {
            data = (TreeData*) sel_wx;

            if (data->IsStation()) {
                LabelInfo* label2 = data->GetLabel();
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

                int brg = int(atan2(dx, dy) * 180.0 / M_PI);
                if (brg < 0) {
                    brg += 360;
                }
                
                m_Dist1->SetLabel(wxString("From ") + label2->text);
                str.Printf("   H: %dm, V: %dm", (int) d_horiz, (int) dz);
                m_Dist2->SetLabel(str);
                str.Printf("   Dist: %dm  Brg: %03d", (int) sqrt(dx*dx + dy*dy + dz*dz), brg);
                m_Dist3->SetLabel(str);
            }
        }
    }
    else if (!data) {
        m_StnName->SetLabel("");
        m_StnCoords->SetLabel("");
        m_StnAlt->SetLabel("");
        if (m_HighlightedPtValid) {
            m_Gfx->DeleteSpecialPoint(m_HighlightedPt);
            m_HighlightedPtValid = false;
        }
        m_Dist1->SetLabel("");
        m_Dist2->SetLabel("");
        m_Dist3->SetLabel("");
    }
}

void MainFrm::TreeItemSelected(wxTreeItemData* item)
{
    TreeData* data = (TreeData*) item;
    
    if (data && data->IsStation()) {
        LabelInfo* label = data->GetLabel();
        m_Gfx->CentreOn(label->x, label->y, label->z);
    }

    m_Dist1->SetLabel("");
    m_Dist2->SetLabel("");
    m_Dist3->SetLabel("");
}

void MainFrm::OnPresCreate(wxCommandEvent& event)
{
#ifdef __WXMOTIF__
    wxFileDialog dlg (this, wxString(msg(/*Select an output filename*/319)), "", "",
                      "*.avp", wxSAVE);
#else
    wxFileDialog dlg (this, wxString(msg(/*Select an output filename*/319)), "", "",
                      wxString::Format("%s|*.avp|%s|*.*",
                                       msg(/*Aven presentations*/320),
                                       msg(/*All files*/208)), wxSAVE);
#endif
    if (dlg.ShowModal() == wxID_OK) {
        m_PresFP = fopen(dlg.GetPath().c_str(), "w");
        assert(m_PresFP); //--Pres: FIXME

        // Update window title.
        SetTitle(wxString("Aven - [") + m_File + wxString(msg(/*] - Recording Presentation*/323)));

        //--Pres: FIXME: discard existing one
        m_PresLoaded = true;
        m_Recording = true;
    }
}

void MainFrm::OnPresGo(wxCommandEvent& event)
{
    assert(m_PresLoaded && !m_Recording); //--Pres: FIXME

    m_Gfx->PresGo();
}

void MainFrm::OnPresGoBack(wxCommandEvent& event)
{
    assert(m_PresLoaded && !m_Recording); //--Pres: FIXME

    m_Gfx->PresGoBack();
}

void MainFrm::OnPresRecord(wxCommandEvent& event)
{
    assert(m_PresLoaded && m_Recording); //--Pres: FIXME

    m_Gfx->RecordPres(m_PresFP);
}

void MainFrm::OnPresFinish(wxCommandEvent& event)
{
    assert(m_PresFP); //--Pres: FIXME
    fclose(m_PresFP);

    // Update window title.
    SetTitle(wxString("Aven - [") + m_File + wxString("]"));
}

void MainFrm::OnPresRestart(wxCommandEvent& event)
{
    assert(m_PresLoaded && !m_Recording); //--Pres: FIXME

    m_Gfx->RestartPres();
}

void MainFrm::OnPresErase(wxCommandEvent& event)
{
    assert(m_PresLoaded && m_Recording); //--Pres: FIXME


}

void MainFrm::OnPresEraseAll(wxCommandEvent& event)
{
    assert(m_PresLoaded && m_Recording); //--Pres: FIXME


}

void MainFrm::OnOpenPres(wxCommandEvent& event)
{
#ifdef __WXMOTIF__
    wxFileDialog dlg (this, wxString(msg(/*Select a presentation to open*/322)), "", "",
                      "*.avp", wxSAVE);
#else
    wxFileDialog dlg (this, wxString(msg(/*Select a presentation to open*/322)), "", "",
                      wxString::Format("%s|*.avp|%s|*.*",
                                       msg(/*Aven presentations*/320),
                                       msg(/*All files*/208)), wxOPEN);
#endif
    if (dlg.ShowModal() == wxID_OK) {
        m_PresFP = fopen(dlg.GetPath(), "rb");
        assert(m_PresFP); //--Pres: FIXME

        m_Gfx->LoadPres(m_PresFP);

        fclose(m_PresFP);

        m_PresLoaded = true;
        m_Recording = false;
    }
}

void MainFrm::OnFileOpenTerrainUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_File != "");
}

void MainFrm::OnOpenPresUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_File != "");
}

void MainFrm::OnPresCreateUpdate(wxUpdateUIEvent& event)
{
    event.Enable(!m_PresLoaded && m_File != "");
}

void MainFrm::OnPresGoUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_PresLoaded && !m_Recording && !m_Gfx->AtEndOfPres());
}

void MainFrm::OnPresGoBackUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_PresLoaded && !m_Recording && !m_Gfx->AtStartOfPres());
}

void MainFrm::OnPresFinishUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_PresLoaded && m_Recording);
}

void MainFrm::OnPresRestartUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_PresLoaded && !m_Recording && !m_Gfx->AtStartOfPres());
}

void MainFrm::OnPresRecordUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_PresLoaded && m_Recording);
}

void MainFrm::OnPresEraseUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_PresLoaded && m_Recording); //--Pres: FIXME
}

void MainFrm::OnPresEraseAllUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_PresLoaded && m_Recording); //--Pres: FIXME
}

void MainFrm::OnFind(wxCommandEvent& event)
{
    // Find stations specified by a string or regular expression.

    wxString str = m_FindBox->GetValue();
    re_pattern_buffer buffer;
    bool regexp = m_RegexpCheckBox->GetValue();
    bool found = false;

    if (regexp) {
        buffer.translate = NULL;
        buffer.fastmap = new char[256];
        buffer.allocated = 0;
        buffer.buffer = NULL;
        buffer.can_be_null = 0;
        buffer.no_sub = 0;

        const char* error = re_compile_pattern(str.c_str(), str.Length(), &buffer);
    
        if (error) {
            wxGetApp().ReportError(
                msg(/*Regular expression compilation failed (perhaps your syntax was incorrect).*/325));
            return;
        }

        if (re_compile_fastmap(&buffer) != 0) {
            wxGetApp().ReportError(msg(/*Regular expression compilation (stage 2) failed.*/326));
            return;
        }
    }

    m_Gfx->ClearSpecialPoints();

    list<LabelInfo*>::iterator pos = m_Labels.begin();

    while (pos != m_Labels.end()) {
        LabelInfo* label = *pos++;
        
        if (regexp) {
            re_registers regs;
            int ret = re_search(&buffer, label->text.c_str(), label->text.Length(), 0, label->text.Length(),
                                NULL);

            switch (ret) {
                case -2:
                    wxGetApp().ReportError(msg(/*Regular expression search failed.*/327));
                    return;

                case -1:
                    break;

                default:
                    m_Gfx->AddSpecialPoint(label->x, label->y, label->z, col_YELLOW, 1);
                    found = true;
                    break;
            }
        }
        else {
            if (label->text.Contains(str)) {
                m_Gfx->AddSpecialPoint(label->x, label->y, label->z, col_YELLOW, 1);
                found = true;
            }
        }
    }

    if (regexp) {
        delete[] buffer.fastmap;
    }

    m_Gfx->DisplaySpecialPoints();

    if (!found) {
        wxGetApp().ReportError(msg(/*No matches were found.*/328));
    }
}

void MainFrm::OnHide(wxCommandEvent& event)
{
    // Hide any search result highlights.

    m_Gfx->ClearSpecialPoints();
}

void MainFrm::SetMouseOverStation(LabelInfo* label)
{
    //-- FIXME: share with code above

    if (label) {
        wxString str;
        str.Printf("   %d N, %d E", (int) (label->y + m_Offsets.y), (int) (label->x + m_Offsets.x));
        m_StnCoords->SetLabel(str);
        m_StnName->SetLabel(label->text);
        str.Printf("   Altitude: %dm", (int) (label->z + m_Offsets.z));
        m_StnAlt->SetLabel(str);
        if (m_HighlightedPtValid) {
            m_Gfx->DeleteSpecialPoint(m_HighlightedPt);
            m_HighlightedPtValid = false;
        }
        m_HighlightedPt = m_Gfx->AddSpecialPoint(label->x, label->y, label->z, col_WHITE);
        m_Gfx->DisplaySpecialPoints();
        m_HighlightedPtValid = true;
    }
    else {
        m_StnName->SetLabel("");
        m_StnCoords->SetLabel("");
        m_StnAlt->SetLabel("");
        if (m_HighlightedPtValid) {
            m_Gfx->DeleteSpecialPoint(m_HighlightedPt);
            m_HighlightedPtValid = false;
        }
        m_Dist1->SetLabel("");
        m_Dist2->SetLabel("");
        m_Dist3->SetLabel("");
    }
}

