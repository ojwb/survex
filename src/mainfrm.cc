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

    EVT_PAINT(MainFrm::OnPaint)
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
    EVT_MENU(menu_VIEW_SHOW_NAMES, MainFrm::OnShowStationNames)
    EVT_MENU(menu_VIEW_SHOW_OVERLAPPING_NAMES, MainFrm::OnDisplayOverlappingNames)
    EVT_MENU(menu_VIEW_SHOW_SURFACE, MainFrm::OnShowSurface)
    EVT_MENU(menu_VIEW_SURFACE_DEPTH, MainFrm::OnShowSurfaceDepth)
    EVT_MENU(menu_VIEW_SURFACE_DASHED, MainFrm::OnShowSurfaceDashed)
    EVT_MENU(menu_VIEW_COMPASS, MainFrm::OnViewCompass)
    EVT_MENU(menu_VIEW_CLINO, MainFrm::OnViewClino)
    EVT_MENU(menu_VIEW_DEPTH_BAR, MainFrm::OnToggleDepthbar)
    EVT_MENU(menu_VIEW_SCALE_BAR, MainFrm::OnToggleScalebar)
    EVT_MENU(menu_VIEW_STATUS_BAR, MainFrm::OnToggleStatusbar)
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
    EVT_UPDATE_UI(menu_VIEW_SHOW_NAMES, MainFrm::OnShowStationNamesUpdate)
    EVT_UPDATE_UI(menu_VIEW_SHOW_SURFACE, MainFrm::OnShowSurfaceUpdate)
    EVT_UPDATE_UI(menu_VIEW_SURFACE_DEPTH, MainFrm::OnShowSurfaceDepthUpdate)
    EVT_UPDATE_UI(menu_VIEW_SURFACE_DASHED, MainFrm::OnShowSurfaceDashedUpdate)
    EVT_UPDATE_UI(menu_VIEW_SHOW_OVERLAPPING_NAMES, MainFrm::OnDisplayOverlappingNamesUpdate)
    EVT_UPDATE_UI(menu_VIEW_COMPASS, MainFrm::OnViewCompassUpdate)
    EVT_UPDATE_UI(menu_VIEW_CLINO, MainFrm::OnViewClinoUpdate)
    EVT_UPDATE_UI(menu_VIEW_DEPTH_BAR, MainFrm::OnToggleDepthbarUpdate)
    EVT_UPDATE_UI(menu_VIEW_SCALE_BAR, MainFrm::OnToggleScalebarUpdate)
    EVT_UPDATE_UI(menu_VIEW_STATUS_BAR, MainFrm::OnToggleStatusbarUpdate)
    EVT_UPDATE_UI(menu_CTL_REVERSE, MainFrm::OnReverseControlsUpdate)
END_EVENT_TABLE()

MainFrm::MainFrm(const wxString& title, const wxPoint& pos, const wxSize& size) :
    wxFrame(NULL, 101, title, pos, size), m_Gfx(NULL), m_StatusBar(NULL), m_FileToLoad("")
{
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
    orientmenu->Append(menu_ORIENT_LOWER_VP, GetTabMsg(/*Lo@wer Viewpoint##/*/251), "Lower the angle of viewing");
    orientmenu->AppendSeparator();
    orientmenu->Append(menu_ORIENT_ZOOM_IN, GetTabMsg(/*@Zoom In##]*/252), "Zoom further into the survey");
    orientmenu->Append(menu_ORIENT_ZOOM_OUT, GetTabMsg(/*Zoo@m Out##[*/253), "Zoom further out from the survey");
    orientmenu->AppendSeparator();
    orientmenu->Append(menu_ORIENT_DEFAULTS, GetTabMsg(/*Restore De@fault Settings##Delete*/254), "Restore default settings for zoom, scale and rotation");

    wxMenu* viewmenu = new wxMenu;
    viewmenu->Append(menu_VIEW_SHOW_NAMES, GetTabMsg(/*Station @Names##Ctrl+N*/270), "Toggle display of survey station names", true);
    viewmenu->Append(menu_VIEW_SHOW_CROSSES, GetTabMsg(/*@Crosses##Ctrl+X*/271), "Toggle display of crosses at survey stations", true);
    viewmenu->AppendSeparator();
    viewmenu->Append(menu_VIEW_SHOW_LEGS, GetTabMsg(/*Underground Survey @Legs##Ctrl+L*/272), "Toggle display of survey legs", true);
    viewmenu->Append(menu_VIEW_SHOW_SURFACE, GetTabMsg(/*Sur@face Survey Legs##Ctrl+F*/291), "unused", true);
    viewmenu->AppendSeparator();
    viewmenu->Append(menu_VIEW_SURFACE_DEPTH, GetTabMsg(/*Depth Colours on Surface Surveys*/292), "unused", true);
    viewmenu->Append(menu_VIEW_SURFACE_DASHED, GetTabMsg(/*Dashed Surface Surveys*/293), "unused", true);
    viewmenu->AppendSeparator();
    viewmenu->Append(menu_VIEW_SHOW_OVERLAPPING_NAMES, GetTabMsg(/*@Overlapping Names##O*/273), "Toggle display all station names, whether or not they overlap", true);
    viewmenu->AppendSeparator();
    viewmenu->Append(menu_VIEW_COMPASS, GetTabMsg(/*Co@mpass*/274), "Toggle display of the compass", true);
    viewmenu->Append(menu_VIEW_CLINO, GetTabMsg(/*Cl@inometer*/275), "Toggle display of the clinometer", true);
    viewmenu->Append(menu_VIEW_DEPTH_BAR, GetTabMsg(/*@Depth Bar*/276), "Toggle display of the depth bar", true);
    viewmenu->Append(menu_VIEW_SCALE_BAR, GetTabMsg(/*Sc@ale Bar*/277), "Toggle display of the scale bar", true);
    //    viewmenu->AppendSeparator();
    //    viewmenu->Append(menu_VIEW_STATUS_BAR, "&Status Bar",
    //		     "Toggle display of the status bar", true);

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

    //CreateStatusBar(2);
    //SetStatusText("Ready");
}

MainFrm::~MainFrm()
{
    ClearPointLists();
    delete[] m_Points;
}
#if 0
bool MainFrm::ProcessEvent(wxEvent& event)
{
    bool result = false;

    // Dispatch certain command events down to the child drawing area window.
    if (m_Gfx && event.IsCommandEvent() && event.GetId() >= menu_ROTATION_START &&
        event.GetId() <= menu_CTL_REVERSE) {
	result = m_Gfx->ProcessEvent(event);
    }

    if (!result) { // child didn't process event
        return wxEvtHandler::ProcessEvent(event);
    }

    return true;
}
#endif
void MainFrm::OnPaint(wxPaintEvent&) //-- sort this out!
{
    wxPaintDC dc(this);

    if (!m_Gfx) {
        m_Gfx = new GfxCore(this);
	if (m_FileToLoad != wxString("")) {
	    OpenFile(m_FileToLoad, true);
	}
    }
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

bool MainFrm::LoadData(const wxString& file)
{
    // Load survey data from file, centre the dataset around the origin,
    // chop legs such that no legs cross depth colour boundaries and prepare
    // the data for drawing.

    // Load the survey data.
    //SetStatusText(wxString("Attempting to open 3D file ") + file);

    img* survey = img_open(file, NULL, NULL);
    if (!survey) {
        //SetStatusText("");
	wxString m = wxString::Format(msg(img_error()), file.c_str());
	wxGetApp().ReportError(m);
	//SetStatusText("Ready");
    }
    else {
        // Create a list of all the leg vertices, counting them and finding the
        // extent of the survey at the same time.
        //SetStatusText(wxString("Parsing ") + file);
 
	m_NumLegs = 0;
	m_NumPoints = 0;
	m_NumExtraLegs = 0;
	m_NumCrosses = 0;

	// Delete any existing list entries.
	ClearPointLists();

	double xmin = DBL_MAX;
	double xmax = -DBL_MAX;
	double ymin = DBL_MAX;
	double ymax = -DBL_MAX;
	m_ZMin = DBL_MAX;
	double zmax = -DBL_MAX;

	bool first = true;
	bool last_was_move = false;

	list<PointInfo*> points;

	int result;
	do {
	    img_point pt;
	    char buffer[256];
	    switch (result = img_read_item(survey, buffer, &pt)) {
	        case img_MOVE:
	        case img_LINE:
		{
		    // Delete <move1> where we get ... <move1> <move2> ... in the data.
		    if (result == img_MOVE) {
		        if (last_was_move) {
			    PointInfo* pt = points.back();
			    delete pt;
			    points.pop_back();
			    m_NumPoints--;
			}
			last_was_move = true;
		    }
		    else {
		        last_was_move = false;
		    }

		    m_NumPoints++;

		    // We must ensure that the resulting list has a move as it's first
		    // datum.
		    if (first && (result != img_MOVE)) {
		        PointInfo* info = new PointInfo;
			info->x = 0.0;
			info->y = 0.0;
			info->z = 0.0;
			info->isLine = false;
			points.push_back(info);
			m_NumPoints++;
		    }
		    
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

		    // Set flags if this datum is a line rather than a move; update number of
		    // legs if it's a line.
		    if (result == img_LINE) {
		        m_NumLegs++;
		        info->isLine = true;
		    }
		    else {
		        info->isLine = false;
		    }
		    
		    info->isSurface = (survey->flags & img_FLAG_SURFACE);

		    /*
		    if (info->isSurface && info->isLine) {
		        if (points.size() > 1) {
			    PointInfo* p = points.back();
			    // if (!p->isLine) {
			        PointInfo* q = new PointInfo;
				memcpy(q, p, sizeof(PointInfo));
				q->isSurface = true;
				points.push_back(q);
				m_NumPoints++;
				//}
			}
			}*/

		    // Store this point in the list.
		    points.push_back(info);
		    
		    break;
		}

	        case img_LABEL:
		{
		    LabelInfo* label = new LabelInfo;
		    label->text = buffer;
		    label->x = pt.x;
		    label->y = pt.y;
		    label->z = pt.z;
		    m_Labels.push_back(label);
		    m_NumCrosses++;

		    // Update survey extents (just in case there are no legs).
		    if (pt.x < xmin) xmin = pt.x;
		    if (pt.x > xmax) xmax = pt.x;
		    if (pt.y < ymin) ymin = pt.y;
		    if (pt.y > ymax) ymax = pt.y;
		    if (pt.z < m_ZMin) m_ZMin = pt.z;
		    if (pt.z > zmax) zmax = pt.z;

		    break;
		}
		
	        default:
	            break;
	    }

	    first = false;

	} while (result != img_BAD && result != img_STOP);

	// Check we've actually loaded some legs or stations!
	if (m_NumLegs == 0 && m_Labels.empty()) {
	    //SetStatusText(wxString(""));
	    wxString m = wxString::Format(msg(/*No survey data in 3d file `%s'*/202), file.c_str());
	    wxGetApp().ReportError(m);
	    return false;
	}

	// Delete any trailing move.
	PointInfo* pt = points.back();
	if (!pt->isLine) {
	    m_NumPoints--;
	    points.pop_back();
	    delete pt;
	}

	m_XExt = xmax - xmin;
	m_YExt = ymax - ymin;
	m_ZExt = zmax - m_ZMin;

	// Sort out depth colouring boundaries (before centering dataset!)
	SortIntoDepthBands(points);

	// Centre the dataset around the origin.
	CentreDataset(xmin, ymin, m_ZMin);

	// Update status bar.
	wxString numlegs_str = wxString::Format(wxString("%d"), m_NumLegs);
	//SetStatusText(numlegs_str + wxString(" legs loaded from ") + file);
	
	// Update window title.
	SetTitle(wxString("Aven - [") + file + wxString("]"));
    }

    return (survey != NULL);
}

void MainFrm::CentreDataset(double xmin, double ymin, double zmin)
{
    // Centre the dataset around the origin.

    double xoff = xmin + (m_XExt / 2.0f);
    double yoff = ymin + (m_YExt / 2.0f);
    double zoff = zmin + (m_ZExt / 2.0f);
    
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

int MainFrm::GetDepthColour(double z)
{
    // Return the (0-based) depth colour band index for a z-coordinate.
    return int(((z - m_ZMin) / (m_ZExt == 0.0f ? 1.0f : m_ZExt)) * (NUM_DEPTH_COLOURS - 1));
}

double MainFrm::GetDepthBoundaryBetweenBands(int a, int b)
{
    // Return the z-coordinate of the depth colour boundary between
    // two adjacent depth colour bands (specified by 0-based indices).

    assert((a == b - 1) || (a == b + 1));

    int band = (a > b) ? a : b; // boundary N lies on the bottom of band N.
    return m_ZMin + (m_ZExt * band / (NUM_DEPTH_COLOURS - 1));
}

void MainFrm::IntersectLineWithPlane(double x0, double y0, double z0,
				     double x1, double y1, double z1,
				     double z, double& x, double& y)
{
    // Find the intersection point of the line (x0, y0, z0) -> (x1, y1, z1) with
    // the plane parallel to the xy-plane with z-axis intersection z.

    double t = (z - z0) / (z1 - z0);
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
	if (prev_point && point->isLine/* && !point->isSurface*/) {
	    int col1 = GetDepthColour(prev_point->z);
	    int col2 = GetDepthColour(point->z);
	    if (col1 != col2) {
	        // The leg does cross at least one boundary, so split it as
		// many times as required...
	        int inc = (col1 > col2) ? -1 : 1;
		for (int band = col1; band != col2; band += inc) {
		    int next_band = band + inc;

		    // Determine the z-coordinate of the boundary being intersected.
		    double split_at_z = GetDepthBoundaryBetweenBands(band, next_band);
		    double split_at_x, split_at_y;

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
		    m_Points[band].push_back(info);

		    // Create a move to this point in the next band.
		    info = new PointInfo;
		    info->x = split_at_x;
		    info->y = split_at_y;
		    info->z = split_at_z;
		    info->isLine = false;
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
	    int band = /*point->isSurface ? NUM_DEPTH_COLOURS :*/ GetDepthColour(point->z);
	    /*	    if (point->isSurface) {
	        if (m_Points[band].empty()) {
		    PointInfo* info = new PointInfo;
		    info->x = 0.0;
		    info->y = 0.0;
		    info->z = 0.0;
		    info->isLine = false;
		    info->isSurface = true;
		    m_Points[band].push_back(info);
		}
		}*/
	    m_Points[band].push_back(point);

	}

	prev_point = point;
    }
}

void MainFrm::OpenFile(const wxString& file, bool delay)
{
    if (m_Gfx) {
        SetCursor(*wxHOURGLASS_CURSOR);
        if (LoadData(file)) {
	    if (!delay) {
                m_Gfx->Initialise();
	    }
	    else {
	        m_Gfx->InitialiseOnNextResize();
	    }
	}
        SetCursor(*wxSTANDARD_CURSOR);
    }
    else {
        m_FileToLoad = file;
    }
}

//
//  UI event handlers
//

void MainFrm::OnOpen(wxCommandEvent&)
{
#ifdef __WXMOTIF__
    wxFileDialog dlg (this, wxString(msg(206)) /* Select a 3D file to view */, "", "",
		      "*.3d", wxOPEN);
#else
    wxFileDialog dlg (this, wxString(msg(206)) /* Select a 3D file to view */, "", "",
		      wxString::Format("%s|*.3d|%s|*.*",
				       msg(207) /* Survex 3d files */,
				       msg(208) /* All files */), wxOPEN);
#endif
    if (dlg.ShowModal() == wxID_OK) {
        OpenFile(dlg.GetPath());
    }
}

void MainFrm::OnQuit(wxCommandEvent&)
{
    // see comment in OnClose()
    exit(0);
}

void MainFrm::OnClose(wxCloseEvent&)
{
  //--wtf doesn't Destroy() work? (on Windows at least)
  //    Destroy();
    exit(0);
}

void MainFrm::OnAbout(wxCommandEvent&)
{
    wxDialog* dlg = new AboutDlg(this);
    dlg->Centre();
    dlg->ShowModal();
}

void MainFrm::OnToggleStatusbar(wxCommandEvent&)
{
    if (!m_StatusBar) {
        m_StatusBar = GetStatusBar();
	SetStatusBar(NULL);
    }
    else {
        SetStatusBar(m_StatusBar);
        m_StatusBar = NULL;
    }

    wxSizeEvent ev(GetSize());
    OnSize(ev);
}

void MainFrm::OnToggleStatusbarUpdate(wxUpdateUIEvent& event)
{
    event.Check(!m_StatusBar);
}
