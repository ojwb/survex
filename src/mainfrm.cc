//
//  mainfrm.cxx
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

#include <float.h>

const int NUM_DEPTH_COLOURS = 13;

static const unsigned char REDS[]   = {190, 155, 110, 18, 0, 124, 48, 117, 163, 182, 224, 237, 255, 230};
static const unsigned char GREENS[] = {218, 205, 177, 153, 178, 211, 219, 224, 224, 193, 190, 117, 0, 230};
static const unsigned char BLUES[]  = {247, 255, 244, 237, 169, 175, 139, 40, 40, 17, 40, 18, 0, 230};

BEGIN_EVENT_TABLE(MainFrm, wxFrame)
    EVT_MENU(menu_FILE_OPEN, MainFrm::OnOpen)
    EVT_MENU(menu_FILE_QUIT, MainFrm::OnQuit)

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
    EVT_MENU(menu_VIEW_ANTIALIAS, MainFrm::OnAntiAlias)
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
#endif
    EVT_UPDATE_UI(menu_CTL_REVERSE, MainFrm::OnReverseControlsUpdate)
END_EVENT_TABLE()

MainFrm::MainFrm(const wxString& title, const wxPoint& pos, const wxSize& size) :
    wxFrame(NULL, 101, title, pos, size, wxDEFAULT_FRAME_STYLE | wxNO_FULL_REPAINT_ON_RESIZE),
    m_Gfx(NULL), m_FileToLoad(""), m_NumEntrances(0), m_NumFixedPts(0), m_NumExportedPts(0)
    
{
#ifdef _WIN32
    SetIcon(wxIcon("aaaaaAven"));
#endif

    m_Points = new list<PointInfo*>[NUM_DEPTH_COLOURS+1];
    m_Pens = new wxPen[NUM_DEPTH_COLOURS+1];
    m_Brushes = new wxBrush[NUM_DEPTH_COLOURS+1];
    for (int pen = 0; pen < NUM_DEPTH_COLOURS+1; pen++) {
	m_Pens[pen].SetColour(REDS[pen], GREENS[pen], BLUES[pen]);
	m_Brushes[pen].SetColour(REDS[pen], GREENS[pen], BLUES[pen]);
    }

    // The status bar messages aren't internationalised, 'cos there ain't any
    // status bar at the moment! ;-)
    wxMenu* filemenu = new wxMenu;
    filemenu->Append(menu_FILE_OPEN, GetTabMsg(/*@Open...##Ctrl+O*/220), "Open a Survex 3D file for viewing");
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
    wxMenu* indmenu = new wxMenu;
    indmenu->Append(menu_VIEW_COMPASS, GetTabMsg(/*Co@mpass*/274), "Toggle display of the compass", true);
    indmenu->Append(menu_VIEW_CLINO, GetTabMsg(/*Cl@inometer*/275), "Toggle display of the clinometer", true);
    indmenu->Append(menu_VIEW_DEPTH_BAR, GetTabMsg(/*@Depth Bar*/276), "Toggle display of the depth bar", true);
    indmenu->Append(menu_VIEW_SCALE_BAR, GetTabMsg(/*Sc@ale Bar*/277), "Toggle display of the scale bar", true);
    viewmenu->Append(menu_VIEW_INDICATORS, GetTabMsg(/*@Indicators*/299), indmenu, "");
#ifdef AVENGL
    viewmenu->AppendSeparator();
    viewmenu->Append(menu_VIEW_ANTIALIAS, GetTabMsg(/*S@moothed Survey Legs*/298), "unused", true);
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
    menubar->Append(ctlmenu, GetTabMsg(/*@Controls*/214));
    menubar->Append(helpmenu, GetTabMsg(/*@Help*/215));
    SetMenuBar(menubar);

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

    m_Gfx = new GfxCore(this);

#ifdef __X__
    // X seems to require a forced resize.
    int x;
    int y;
    GetSize(&x, &y);
    SetSize(-1, -1, x, y);
#endif
}

MainFrm::~MainFrm()
{
    ClearPointLists();
    delete[] m_Points;
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

    // Create a list of all the leg vertices, counting them and finding the
    // extent of the survey at the same time.

    m_NumLegs = 0;
    m_NumPoints = 0;
    m_NumExtraLegs = 0;
    m_NumCrosses = 0;
    m_NumFixedPts = 0;
    m_NumExportedPts = 0;
    m_NumEntrances = 0;

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
		label->isEntrance = (survey->flags & img_SFLAG_ENTRANCE);
		label->isFixedPt = (survey->flags & img_SFLAG_FIXED);
		label->isExportedPt = (survey->flags & img_SFLAG_EXPORTED);
		if (label->isEntrance) {
		    m_NumEntrances++;
		}
		if (label->isFixedPt) {
		    m_NumFixedPts++;
		}
		if (label->isExportedPt) {
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

    // Sort out depth colouring boundaries (before centering dataset!)
    SortIntoDepthBands(points);

    // Centre the dataset around the origin.
    CentreDataset(xmin, ymin, m_ZMin);

    // Update window title.
    SetTitle(wxString("Aven - [") + file + wxString("]"));

    return true;
}

void MainFrm::CentreDataset(Double xmin, Double ymin, Double zmin)
{
    // Centre the dataset around the origin.

    Double xoff = xmin + (m_XExt / 2.0f);
    Double yoff = ymin + (m_YExt / 2.0f);
    Double zoff = zmin + (m_ZExt / 2.0f);
    
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

void MainFrm::OpenFile(const wxString& file, const wxString & survey, bool delay)
{
    // FIXME: delay is always false...
    SetCursor(*wxHOURGLASS_CURSOR);
    if (LoadData(file, survey)) {
        if (!delay) {
	    m_Gfx->Initialise();
	}
	else {
	    m_Gfx->InitialiseOnNextResize();
	}
    }
    SetCursor(*wxSTANDARD_CURSOR);
}

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
        OpenFile(dlg.GetPath());
    }
}

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
