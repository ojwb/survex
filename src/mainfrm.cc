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

#include "img.h"
#include "mainfrm.h"

#include "aven.h"

#include <float.h>

static const int NUM_DEPTH_COLOURS = 13;
static const float REDS[]   = {190, 155, 110, 18, 0, 124, 48, 117, 163, 182, 224, 237, 255};
static const float GREENS[] = {218, 205, 177, 153, 178, 211, 219, 224, 224, 193, 190, 117, 0};
static const float BLUES[]  = {247, 255, 244, 237, 169, 175, 139, 40, 40, 17, 40, 18, 0};

BEGIN_EVENT_TABLE(MainFrm, wxFrame)
    EVT_MENU(menu_FILE_OPEN, MainFrm::OnOpen)
    EVT_MENU(menu_FILE_QUIT, MainFrm::OnQuit)
END_EVENT_TABLE()

MainFrm::MainFrm(const wxString& title, const wxPoint& pos, const wxSize& size) :
    wxFrame(NULL, -1, title, pos, size)
{
    m_Points = new list<PointInfo*>[NUM_DEPTH_COLOURS];
    m_Pens = new wxPen[NUM_DEPTH_COLOURS];
    for (int pen = 0; pen < NUM_DEPTH_COLOURS; pen++) {
	m_Pens[pen].SetColour(REDS[pen], GREENS[pen], BLUES[pen]);
    }

    wxMenu* filemenu = new wxMenu;
    filemenu->Append(menu_FILE_OPEN, "&Open...\tCtrl+O", "");
    filemenu->AppendSeparator();
    filemenu->Append(menu_FILE_QUIT, "&Exit", "Quits Aven");

    wxMenu* rotmenu = new wxMenu;
    rotmenu->Append(menu_ROTATION_START, "&Start Rotation\tReturn", "");
    rotmenu->Append(menu_ROTATION_STOP, "S&top Rotation\tSpace", "");
    rotmenu->AppendSeparator();
    rotmenu->Append(menu_ROTATION_SPEED_UP, "Speed &Up\tZ", "");
    rotmenu->Append(menu_ROTATION_SLOW_DOWN, "S&low Down\tX", "");
    rotmenu->AppendSeparator();
    rotmenu->Append(menu_ROTATION_REVERSE, "&Reverse Direction\tR", "");
    rotmenu->AppendSeparator();
    rotmenu->Append(menu_ROTATION_STEP_CCW, "Step Once &Anticlockwise\tC", "");
    rotmenu->Append(menu_ROTATION_STEP_CW, "Step Once &Clockwise\tV", "");

    wxMenu* orientmenu = new wxMenu;
    orientmenu->Append(menu_ORIENT_MOVE_NORTH, "Move &North\tN", "");
    orientmenu->Append(menu_ORIENT_MOVE_EAST, "Move &East\tE", "");
    orientmenu->Append(menu_ORIENT_MOVE_SOUTH, "Move S&outh\tS", "");
    orientmenu->Append(menu_ORIENT_MOVE_WEST, "Move &West\tW", "");
    orientmenu->AppendSeparator();
    orientmenu->Append(menu_ORIENT_SHIFT_LEFT, "Shift Survey &Left\tLeft Arrow", "");
    orientmenu->Append(menu_ORIENT_SHIFT_RIGHT, "Shift Survey &Right\tRight Arrow", "");
    orientmenu->Append(menu_ORIENT_SHIFT_UP, "Shift Survey &Up\tUp Arrow", "");
    orientmenu->Append(menu_ORIENT_SHIFT_DOWN, "Shift Survey &Down\tDown Arrow", "");
    orientmenu->AppendSeparator();
    orientmenu->Append(menu_ORIENT_PLAN, "&Plan View\tP", "");
    orientmenu->Append(menu_ORIENT_ELEVATION, "Ele&vation View\tL", "");
    orientmenu->AppendSeparator();
    orientmenu->Append(menu_ORIENT_HIGHER_VP, "&Higher Viewpoint\t'", "");
    orientmenu->Append(menu_ORIENT_LOWER_VP, "Lo&wer Viewpoint\t/", "");
    orientmenu->AppendSeparator();
    orientmenu->Append(menu_ORIENT_ZOOM_IN, "&Zoom In\t]", "");
    orientmenu->Append(menu_ORIENT_ZOOM_OUT, "Zoo&m Out\t[", "");
    orientmenu->AppendSeparator();
    orientmenu->Append(menu_ORIENT_DEFAULTS, "Restore De&fault Settings\tDelete", "");

    wxMenuBar* menubar = new wxMenuBar;
    menubar->Append(filemenu, "&File");
    menubar->Append(rotmenu, "&Rotation");
    menubar->Append(orientmenu, "&Orientation");
    SetMenuBar(menubar);

    CreateStatusBar(2);
    SetStatusText("Ready");

    m_Gfx = new GfxCore(this);
}

MainFrm::~MainFrm()
{
    ClearPointLists();
    delete[] m_Points;
}

int MainFrm::GetNumDepthBands()
{
    return NUM_DEPTH_COLOURS;
}

wxPen MainFrm::GetPen(int band)
{
    assert(band >= 0 && band < NUM_DEPTH_COLOURS);
    return m_Pens[band];
}

void MainFrm::ClearPointLists()
{
    // Free memory occupied by the contents of the point and label lists.

    for (int band = 0; band < NUM_DEPTH_COLOURS; band++) {
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
    SetStatusText(wxString("Attempting to open 3D file ") + file);

    img* survey = img_open(file, NULL, NULL);
    if (!survey) {
        SetStatusText("");
	wxString svxerr;
	switch (img_error()) {
	    case IMG_FILENOTFOUND: svxerr = "file not found"; break;
	    case IMG_OUTOFMEMORY:  svxerr = "out of memory"; break;
	    case IMG_DIRECTORY:    svxerr = "it's a directory!"; break;
	    case IMG_BADFORMAT:    svxerr = "incorrect 3D file format"; break;
	    default: assert(0);
	}
	wxString msg = wxString("Couldn't open 3D file '") + file + wxString("' (") + svxerr +
	               wxString(").");
	wxGetApp().ReportError(msg);
	SetStatusText("Ready");
    }
    else {
        // Create a list of all the leg vertices, counting them and finding the
        // extent of the survey at the same time.
        SetStatusText(wxString("Parsing ") + file);
 
	m_NumLegs = 0;
	m_NumPoints = 0;
	m_NumExtraLegs = 0;
	m_NumCrosses = 0;

	// Delete any existing list entries.
	ClearPointLists();

	float xmin = FLT_MAX;
	float xmax = FLT_MIN;
	float ymin = FLT_MAX;
	float ymax = FLT_MIN;
	m_ZMin = FLT_MAX;
	float zmax = FLT_MIN;

	bool first = true;
	bool last_was_move = false;

	list<PointInfo*> points;

	int result;
	do {
	    float x, y, z;
	    char buffer[256];
	    switch (result = img_read_datum(survey, buffer, &x, &y, &z)) {
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
		    if (x < xmin) xmin = x;
		    if (x > xmax) xmax = x;
		    if (y < ymin) ymin = y;
		    if (y > ymax) ymax = y;
		    if (z < m_ZMin) m_ZMin = z;
		    if (z > zmax) zmax = z;

		    PointInfo* info = new PointInfo;
		    info->x = x;
		    info->y = y;
		    info->z = z;

		    // Set flags if this datum is a line rather than a move; update number of
		    // legs if it's a line.
		    if (result == img_LINE) {
		        m_NumLegs++;
		        info->isLine = true;
		    }
		    else {
		        info->isLine = false;
		    }

		    // Store this point in the list.
		    points.push_back(info);
		    
		    break;
		}

	        case img_LABEL:
		{
		    LabelInfo* label = new LabelInfo;
		    label->text = buffer;
		    label->x = x;
		    label->y = y;
		    label->z = z;
		    m_Labels.push_back(label);
		    m_NumCrosses++;

		    // Update survey extents (just in case there are no legs).
		    if (x < xmin) xmin = x;
		    if (x > xmax) xmax = x;
		    if (y < ymin) ymin = y;
		    if (y > ymax) ymax = y;
		    if (z < m_ZMin) m_ZMin = z;
		    if (z > zmax) zmax = z;

		    break;
		}
		
	        default:
	            break;
	    }

	    first = false;

	} while (result != img_BAD && result != img_STOP);

	// Check we've actually loaded some legs or stations!
	if (m_NumLegs == 0 || m_Labels.size() == 0) {
	    SetStatusText(wxString(""));
	    wxString msg = wxString("No legs or stations found in 3D file '") + file + wxString("'.");
	    wxGetApp().ReportError(msg);
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
	SetStatusText(numlegs_str + wxString(" legs loaded from ") + file);

	// Update window title.
	SetTitle(wxString("Aven - [") + file + wxString("]"));
    }

    return (survey != NULL);
}

void MainFrm::CentreDataset(float xmin, float ymin, float zmin)
{
    // Centre the dataset around the origin.

    float xoff = xmin + (m_XExt / 2.0f);
    float yoff = ymin + (m_YExt / 2.0f);
    float zoff = zmin + (m_ZExt / 2.0f);
    
    for (int band = 0; band < NUM_DEPTH_COLOURS; band++) {
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

int MainFrm::GetDepthColour(float z)
{
    // Return the (0-based) depth colour band index for a z-coordinate.
    return int(((z - m_ZMin) / (m_ZExt == 0.0f ? 1.0f : m_ZExt)) * (NUM_DEPTH_COLOURS - 1));
}

float MainFrm::GetDepthBoundaryBetweenBands(int a, int b)
{
    // Return the z-coordinate of the depth colour boundary between
    // two adjacent depth colour bands (specified by 0-based indices).

    assert((a == b - 1) || (a == b + 1));

    int band = (a > b) ? a : b; // boundary N lies on the bottom of band N.
    return m_ZMin + (m_ZExt * band / (NUM_DEPTH_COLOURS - 1));
}

void MainFrm::IntersectLineWithPlane(float x0, float y0, float z0,
				     float x1, float y1, float z1,
				     float z, float& x, float& y)
{
    // Find the intersection point of the line (x0, y0, z0) -> (x1, y1, z1) with
    // the plane parallel to the xy-plane with z-axis intersection z.

    float t = (z - z0) / (z1 - z0);
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
	if (prev_point && (point->isLine)) {
	    int col1 = GetDepthColour(prev_point->z);
	    int col2 = GetDepthColour(point->z);
	    if (col1 != col2) {
	        // The leg does cross at least one boundary, so split it as
		// many times as required...
	        int inc = (col1 > col2) ? -1 : 1;
		for (int band = col1; band != col2; band += inc) {
		    int next_band = band + inc;

		    // Determine the z-coordinate of the boundary being intersected.
		    float split_at_z = GetDepthBoundaryBetweenBands(band, next_band);
		    float split_at_x, split_at_y;

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
	    // The first point, or another move: put it in the correct list
	    // according to depth.
	    assert(!point->isLine);
	    int band = GetDepthColour(point->z);
	    m_Points[band].push_back(point);

	}

	prev_point = point;
    }
}

list<PointInfo*>::iterator MainFrm::GetFirstPoint(int band)
{
    assert(band >= 0 && band < NUM_DEPTH_COLOURS);
    return m_Points[band].begin();
}

bool MainFrm::GetNextPoint(int band, list<PointInfo*>::iterator& pos, float& x, float& y, float& z,
			   bool& isLine)
{
    assert(band >= 0 && band < NUM_DEPTH_COLOURS);
    if (pos == m_Points[band].end()) {
        return false;
    }

    PointInfo* pt = *pos++;
    x = pt->x;
    y = pt->y;
    z = pt->z;
    isLine = pt->isLine;

    return (pos != m_Points[band].end());
}

list<LabelInfo*>::iterator MainFrm::GetFirstLabel()
{
    return m_Labels.begin();
}

bool MainFrm::GetNextLabel(list<LabelInfo*>::iterator& pos, float& x, float& y, float& z,
			   wxString& text)
{
    if (pos == m_Labels.end()) return false;

    LabelInfo* label = *pos++;
    x = label->x;
    y = label->y;
    z = label->z;
    text = label->text;

    return (pos != m_Labels.end());
}

//
//  UI event handlers
//

void MainFrm::OnOpen(wxCommandEvent&)
{
    wxFileDialog dlg (this, "Select a 3D file to view", "", "",
		      "Survex 3D files|*.3d|All files|*.*", wxOPEN);
    if (dlg.ShowModal() == wxID_OK) {
        SetCursor(*wxHOURGLASS_CURSOR);
        LoadData(dlg.GetPath());
	m_Gfx->Initialise();
        SetCursor(*wxSTANDARD_CURSOR);
    }
}

void MainFrm::OnQuit(wxCommandEvent&)
{
    Close(true);
}
