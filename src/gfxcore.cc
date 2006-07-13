//
//  gfxcore.cc
//
//  Core drawing code for Aven.
//
//  Copyright (C) 2000-2003,2005 Mark R. Shinwell
//  Copyright (C) 2001-2003,2004,2005,2006 Olly Betts
//  Copyright (C) 2005 Martin Green
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <float.h>

#include "aven.h"
#include "gfxcore.h"
#include "mainfrm.h"
#include "message.h"
#include "useful.h"
#include "printwx.h"
#include "guicontrol.h"
#include "moviemaker.h"
#include "export.h"

#include <wx/confbase.h>
#include <wx/image.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAX3(a, b, c) ((a) > (b) ? MAX(a, c) : MAX(b, c))

// Values for m_SwitchingTo
#define PLAN 1
#define ELEVATION 2
#define NORTH 3
#define EAST 4
#define SOUTH 5
#define WEST 6

// How many bins per letter height to use when working out non-overlapping
// labels.
const unsigned int QUANTISE_FACTOR = 2;

const int NUM_DEPTH_COLOURS = 13;

#include "avenpal.h"

static const int COMPASS_OFFSET_X = 60;
static const int COMPASS_OFFSET_Y = 80;
static const int INDICATOR_BOX_SIZE = 60;
static const int INDICATOR_GAP = 2;
static const int INDICATOR_MARGIN = 5;
static const int INDICATOR_OFFSET_X = 15;
static const int INDICATOR_OFFSET_Y = 15;
static const int INDICATOR_RADIUS = INDICATOR_BOX_SIZE / 2 - INDICATOR_MARGIN;
static const int CLINO_OFFSET_X = 6 + INDICATOR_OFFSET_X +
				  INDICATOR_BOX_SIZE + INDICATOR_GAP;
static const int DEPTH_BAR_OFFSET_X = 16;
static const int DEPTH_BAR_EXTRA_LEFT_MARGIN = 2;
static const int DEPTH_BAR_BLOCK_WIDTH = 20;
static const int DEPTH_BAR_BLOCK_HEIGHT = 15;
static const int DEPTH_BAR_MARGIN = 6;
static const int DEPTH_BAR_OFFSET_Y = 16 + DEPTH_BAR_MARGIN;
static const int TICK_LENGTH = 4;
static const int SCALE_BAR_OFFSET_X = 15;
static const int SCALE_BAR_OFFSET_Y = 12;
static const int SCALE_BAR_HEIGHT = 12;

static const gla_colour TEXT_COLOUR = col_GREEN;
static const gla_colour HERE_COLOUR = col_WHITE;
static const gla_colour NAME_COLOUR = col_GREEN;

#define HITTEST_SIZE 20

// vector for lighting angle
static const Vector3 light(.577, .577, .577);

BEGIN_EVENT_TABLE(GfxCore, wxGLCanvas)
    EVT_PAINT(GfxCore::OnPaint)
    EVT_LEFT_DOWN(GfxCore::OnLButtonDown)
    EVT_LEFT_UP(GfxCore::OnLButtonUp)
    EVT_MIDDLE_DOWN(GfxCore::OnMButtonDown)
    EVT_MIDDLE_UP(GfxCore::OnMButtonUp)
    EVT_RIGHT_DOWN(GfxCore::OnRButtonDown)
    EVT_RIGHT_UP(GfxCore::OnRButtonUp)
    EVT_MOUSEWHEEL(GfxCore::OnMouseWheel)
    EVT_MOTION(GfxCore::OnMouseMove)
    EVT_LEAVE_WINDOW(GfxCore::OnLeaveWindow)
    EVT_SIZE(GfxCore::OnSize)
    EVT_IDLE(GfxCore::OnIdle)
    EVT_CHAR(GfxCore::OnKeyPress)
END_EVENT_TABLE()

GfxCore::GfxCore(MainFrm* parent, wxWindow* parent_win, GUIControl* control) :
    GLACanvas(parent_win, 100), m_HaveData(false)
{
    m_Control = control;
    m_ScaleBarWidth = 0;
    m_Parent = parent;
    m_DoneFirstShow = false;
    m_Crosses = false;
    m_Legs = true;
    m_Names = false;
    m_OverlappingNames = false;
    m_Compass = true;
    m_Clino = true;
    m_Depthbar = true;
    m_Scalebar = true;
    m_LabelGrid = NULL;
    m_Rotating = false;
    m_SwitchingTo = 0;
    m_Entrances = false;
    m_FixedPts = false;
    m_ExportedPts = false;
    m_Tubes = false;
    m_Grid = false;
    m_BoundingBox = false;
    m_ColourBy = COLOUR_BY_DEPTH;
    AddQuad = &GfxCore::AddQuadrilateralDepth;
    AddPoly = &GfxCore::AddPolylineDepth;
    wxConfigBase::Get()->Read("metric", &m_Metric, true);
    wxConfigBase::Get()->Read("degrees", &m_Degrees, true);
    m_here.Invalidate();
    m_there.Invalidate();
    presentation_mode = 0;
    pres_speed = 0.0;
    pres_reverse = false;
    mpeg = NULL;
    current_cursor = GfxCore::CURSOR_DEFAULT;

    // Initialise grid for hit testing.
    m_PointGrid = new list<LabelInfo*>[HITTEST_SIZE * HITTEST_SIZE];

    m_Pens = new GLAPen[NUM_DEPTH_COLOURS + 1];
    for (int pen = 0; pen < NUM_DEPTH_COLOURS + 1; ++pen) {
	m_Pens[pen].SetColour(REDS[pen] / 255.0,
			      GREENS[pen] / 255.0,
			      BLUES[pen] / 255.0);
    }
}

GfxCore::~GfxCore()
{
    TryToFreeArrays();

    delete[] m_Pens;
    delete[] m_PointGrid;
}

void GfxCore::TryToFreeArrays()
{
    // Free up any memory allocated for arrays.

    if (m_LabelGrid) {
	delete[] m_LabelGrid;
	m_LabelGrid = NULL;
    }
}

//
//  Initialisation methods
//

void GfxCore::Initialise()
{
    // Initialise the view from the parent holding the survey data.

    TryToFreeArrays();

    m_DoneFirstShow = false;

    m_HitTestGridValid = false;
    m_here.Invalidate();
    m_there.Invalidate();

    // Apply default parameters.
    DefaultParameters();

    switch (m_ColourBy) {
	case COLOUR_BY_DEPTH:
	    if (m_Parent->GetZExtent() == 0.0) {
		SetColourBy(COLOUR_BY_NONE);
	    }
	    break;
	case COLOUR_BY_DATE:
	    if (m_Parent->GetDateExtent() == 0) {
		SetColourBy(COLOUR_BY_NONE);
	    }
	    break;
    }

    m_HaveData = true;

    ForceRefresh();
}

void GfxCore::FirstShow()
{
    GLACanvas::FirstShow();

    const unsigned int quantise(GetFontSize() / QUANTISE_FACTOR);
    list<LabelInfo*>::iterator pos = m_Parent->GetLabelsNC();
    while (pos != m_Parent->GetLabelsNCEnd()) {
	LabelInfo* label = *pos++;
	// Calculate and set the label width for use when plotting
	// none-overlapping labels.
	int ext_x;
	GLACanvas::GetTextExtent(label->GetText(), &ext_x, NULL);
	label->set_width(unsigned(ext_x) / quantise + 1);
    }

    SetBackgroundColour(0.0, 0.0, 0.0);

    // Set diameter of the viewing volume.
    SetVolumeDiameter(sqrt(sqrd(m_Parent->GetXExtent()) +
			   sqrd(m_Parent->GetYExtent()) +
			   sqrd(m_Parent->GetZExtent())));

    // Update our record of the client area size and centre.
    GetClientSize(&m_XSize, &m_YSize);

    // Set the initial scale.
    SetScale(1.0);

    m_DoneFirstShow = true;
}

//
//  Recalculating methods
//

void GfxCore::SetScale(Double scale)
{
    // Fill the plot data arrays with screen coordinates, scaling the survey
    // to a particular absolute scale.

    if (scale < 0.05) {
	scale = 0.05;
    } else {
	if (scale > 1000.0) scale = 1000.0;
    }

    m_Scale = scale;
    m_HitTestGridValid = false;

    GLACanvas::SetScale(scale);
}

bool GfxCore::HasUndergroundLegs() const
{
    return m_Parent->HasUndergroundLegs();
}

bool GfxCore::HasSurfaceLegs() const
{
    return m_Parent->HasSurfaceLegs();
}

bool GfxCore::HasTubes() const
{
    return m_Parent->HasTubes();
}

void GfxCore::UpdateBlobs()
{
    InvalidateList(LIST_BLOBS);
}

//
//  Event handlers
//

void GfxCore::OnLeaveWindow(wxMouseEvent& event) {
    SetHere();
    ClearCoords();
}

void GfxCore::OnIdle(wxIdleEvent& event)
{
    // Handle an idle event.
    if (Animate()) ForceRefresh();
}

void GfxCore::OnPaint(wxPaintEvent&)
{
    // Redraw the window.

    // Get a graphics context.
    wxPaintDC dc(this);

    assert(GetContext());

    if (m_HaveData) {
	// Make sure we're initialised.
	bool first_time = !m_DoneFirstShow;
	if (first_time) {
	    FirstShow();
	}

	StartDrawing();

	// Clear the background.
	Clear();

	// Set up model transformation matrix.
	SetDataTransform();

	timer.Start(); // reset timer

	if (m_Legs || m_Tubes) {
	    if (m_Tubes) {
		EnableSmoothPolygons(true); // FIXME: allow false for wireframe view
		DrawList(LIST_TUBES);
		DisableSmoothPolygons();
	    }

	    // Draw the underground legs.  Do this last so that anti-aliasing
	    // works over polygons.
	    SetColour(col_GREEN);
	    DrawList(LIST_UNDERGROUND_LEGS);
	}

	if (m_Surface) {
	    // Draw the surface legs.
	    DrawList(LIST_SURFACE_LEGS);
	}

	DrawList(LIST_BLOBS);

	if (m_BoundingBox) {
	    DrawShadowedBoundingBox();
	}
	if (m_Grid) {
	    // Draw the grid.
	    DrawList(LIST_GRID);
	}

	if (m_Crosses) {
	    DrawList(LIST_CROSSES);
	}

	SetIndicatorTransform();

	// Draw station names.
	if (m_Names /*&& !m_Control->MouseDown() && !Animating()*/) {
	    SetColour(NAME_COLOUR);

	    if (m_OverlappingNames) {
		SimpleDrawNames();
	    } else {
		NattyDrawNames();
	    }
	}

	if (!Animating() && (m_here.IsValid() || m_there.IsValid())) {
	    // Draw "here" and "there".
	    Double hx, hy;
	    SetColour(HERE_COLOUR);
	    if (m_here.IsValid()) {
		Double dummy;
		Transform(m_here, &hx, &hy, &dummy);
		DrawRing(hx, hy);
	    }
	    if (m_there.IsValid()) {
		Double tx, ty;
		Double dummy;
		Transform(m_there, &tx, &ty, &dummy);
		if (m_here.IsValid()) {
		    BeginLines();
		    PlaceIndicatorVertex(hx, hy);
		    PlaceIndicatorVertex(tx, ty);
		    EndLines();
		}
		BeginBlobs();
		DrawBlob(tx, ty);
		EndBlobs();
	    }
	}

	// Draw indicators.
	//
	// There's no advantage in generating an OpenGL list for the
	// indicators since they change with almost every redraw (and
	// sometimes several times between redraws).  This way we avoid
	// the need to track when to update the indicator OpenGL list,
	// and also avoid indicator update bugs when we don't quite get this
	// right...
	DrawIndicators();

	FinishDrawing();

	drawtime = timer.Time();
    } else {
#if wxCHECK_VERSION(2,6,0)
	dc.SetBackground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWFRAME));
#else
	dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWFRAME), wxSOLID));
#endif
	dc.Clear();
    }
}

void GfxCore::DrawBoundingBox()
{
    const Vector3 v = 0.5 * m_Parent->GetExtent();

    SetColour(col_BLUE);
    EnableDashedLines();
    BeginPolyline();
    PlaceVertex(-v.GetX(), -v.GetY(), v.GetZ());
    PlaceVertex(-v.GetX(), v.GetY(), v.GetZ());
    PlaceVertex(v.GetX(), v.GetY(), v.GetZ());
    PlaceVertex(v.GetX(), -v.GetY(), v.GetZ());
    PlaceVertex(-v.GetX(), -v.GetY(), v.GetZ());
    EndPolyline();
    BeginPolyline();
    PlaceVertex(-v.GetX(), -v.GetY(), -v.GetZ());
    PlaceVertex(-v.GetX(), v.GetY(), -v.GetZ());
    PlaceVertex(v.GetX(), v.GetY(), -v.GetZ());
    PlaceVertex(v.GetX(), -v.GetY(), -v.GetZ());
    PlaceVertex(-v.GetX(), -v.GetY(), -v.GetZ());
    EndPolyline();
    BeginLines();
    PlaceVertex(-v.GetX(), -v.GetY(), v.GetZ());
    PlaceVertex(-v.GetX(), -v.GetY(), -v.GetZ());
    PlaceVertex(-v.GetX(), v.GetY(), v.GetZ());
    PlaceVertex(-v.GetX(), v.GetY(), -v.GetZ());
    PlaceVertex(v.GetX(), v.GetY(), v.GetZ());
    PlaceVertex(v.GetX(), v.GetY(), -v.GetZ());
    PlaceVertex(v.GetX(), -v.GetY(), v.GetZ());
    PlaceVertex(v.GetX(), -v.GetY(), -v.GetZ());
    EndLines();
    DisableDashedLines();
}

void GfxCore::DrawShadowedBoundingBox()
{
    const Vector3 v = 0.5 * m_Parent->GetExtent();

    DrawBoundingBox();

    // FIXME: these gl* calls should be in gla-gl.cc
    glPolygonOffset(1.0, 1.0);
    glEnable(GL_POLYGON_OFFSET_FILL);

    SetColour(col_DARK_GREY);
    BeginQuadrilaterals();
    PlaceVertex(-v.GetX(), -v.GetY(), -v.GetZ());
    PlaceVertex(-v.GetX(), v.GetY(), -v.GetZ());
    PlaceVertex(v.GetX(), v.GetY(), -v.GetZ());
    PlaceVertex(v.GetX(), -v.GetY(), -v.GetZ());
    EndQuadrilaterals();

    glDisable(GL_POLYGON_OFFSET_FILL);

    DrawList(LIST_SHADOW);
}

void GfxCore::DrawGrid()
{
    // Draw the grid.
    SetColour(col_RED);

    // Calculate the extent of the survey, in metres across the screen plane.
    Double m_across_screen = SurveyUnitsAcrossViewport();
    // Calculate the length of the scale bar in metres.
    //--move this elsewhere
    Double size_snap = pow(10.0, floor(log10(0.75 * m_across_screen)));
    Double t = m_across_screen * 0.75 / size_snap;
    if (t >= 5.0) {
	size_snap *= 5.0;
    }
    else if (t >= 2.0) {
	size_snap *= 2.0;
    }

    Double grid_size = size_snap * 0.1;
    Double edge = grid_size * 2.0;
    Double grid_z = -m_Parent->GetZExtent() * 0.5 - grid_size;
    Double left = -m_Parent->GetXExtent() * 0.5 - edge;
    Double right = m_Parent->GetXExtent() * 0.5 + edge;
    Double bottom = -m_Parent->GetYExtent() * 0.5 - edge;
    Double top = m_Parent->GetYExtent() * 0.5 + edge;
    int count_x = (int) ceil((right - left) / grid_size);
    int count_y = (int) ceil((top - bottom) / grid_size);
    Double actual_right = left + count_x*grid_size;
    Double actual_top = bottom + count_y*grid_size;

    BeginLines();

    for (int xc = 0; xc <= count_x; xc++) {
	Double x = left + xc*grid_size;

	PlaceVertex(x, bottom, grid_z);
	PlaceVertex(x, actual_top, grid_z);
    }

    for (int yc = 0; yc <= count_y; yc++) {
	Double y = bottom + yc*grid_size;
	PlaceVertex(left, y, grid_z);
	PlaceVertex(actual_right, y, grid_z);
    }

    EndLines();
}

int GfxCore::GetClinoOffset() const
{
    return m_Compass ? CLINO_OFFSET_X : INDICATOR_OFFSET_X;
}

void GfxCore::DrawTick(int angle_cw)
{
    const Double theta = rad(angle_cw);
    const wxCoord length1 = INDICATOR_RADIUS;
    const wxCoord length0 = length1 + TICK_LENGTH;
    wxCoord x0 = wxCoord(length0 * sin(theta));
    wxCoord y0 = wxCoord(length0 * cos(theta));
    wxCoord x1 = wxCoord(length1 * sin(theta));
    wxCoord y1 = wxCoord(length1 * cos(theta));

    PlaceIndicatorVertex(x0, y0);
    PlaceIndicatorVertex(x1, y1);
}

void GfxCore::DrawArrow(gla_colour col1, gla_colour col2) {
    Vector3 p1(0, INDICATOR_RADIUS, 0);
    Vector3 p2(INDICATOR_RADIUS/2, INDICATOR_RADIUS*-.866025404, 0); // 150deg
    Vector3 p3(-INDICATOR_RADIUS/2, INDICATOR_RADIUS*-.866025404, 0); // 210deg
    Vector3 pc(0, 0, 0);

    DrawTriangle(col_LIGHT_GREY, col1, p2, p1, pc);
    DrawTriangle(col_LIGHT_GREY, col2, p3, p1, pc);
}

void GfxCore::DrawCompass() {
    // Ticks.
    BeginLines();
    for (int angle = 315; angle > 0; angle -= 45) {
	DrawTick(angle);
    }
    SetColour(col_GREEN);
    DrawTick(0);
    EndLines();

    // Compass background.
    DrawCircle(col_LIGHT_GREY_2, col_GREY, 0, 0, INDICATOR_RADIUS);

    // Compass arrow.
    DrawArrow(col_INDICATOR_1, col_INDICATOR_2);
}

// Draw the non-rotating background to the clino.
void GfxCore::DrawClinoBack() {
    BeginLines();
    for (int angle = 0; angle <= 180; angle += 90) {
	DrawTick(angle);
    }

    SetColour(col_GREY);
    PlaceIndicatorVertex(0, INDICATOR_RADIUS);
    PlaceIndicatorVertex(0, -INDICATOR_RADIUS);
    PlaceIndicatorVertex(0, 0);
    PlaceIndicatorVertex(INDICATOR_RADIUS, 0);

    EndLines();
}

void GfxCore::DrawClino() {
    // Ticks.
    SetColour(col_GREEN);
    BeginLines();
    DrawTick(0);
    EndLines();

    // Clino background.
    DrawSemicircle(col_LIGHT_GREY_2, col_GREY, 0, 0, INDICATOR_RADIUS, 0);

    // Elevation arrow.
    DrawArrow(col_INDICATOR_2, col_INDICATOR_1);
}

void GfxCore::Draw2dIndicators()
{
    // Draw the compass and elevation indicators.

    const int centre_y = INDICATOR_BOX_SIZE / 2 + INDICATOR_OFFSET_Y;

    const int comp_centre_x = GetCompassXPosition();

    if (m_Compass && !m_Parent->IsExtendedElevation()) {
	// If the user is dragging the compass with the pointer outside the
	// compass, we snap to 45 degree multiples, and the ticks go white.
	SetColour(m_MouseOutsideCompass ? col_WHITE : col_LIGHT_GREY_2);
	DrawList2D(LIST_COMPASS, comp_centre_x, centre_y, -m_PanAngle);
    }

    const int elev_centre_x = GetClinoXPosition();

    if (m_Clino) {
	// If the user is dragging the clino with the pointer outside the
	// clino, we snap to 90 degree multiples, and the ticks go white.
	SetColour(m_MouseOutsideElev ? col_WHITE : col_LIGHT_GREY_2);
	DrawList2D(LIST_CLINO_BACK, elev_centre_x, centre_y, 0);
	DrawList2D(LIST_CLINO, elev_centre_x, centre_y, 90 + m_TiltAngle);
    }

    SetColour(TEXT_COLOUR);

    int w, h;
    int width, height;
    wxString str;

    GetTextExtent(wxString("000"), &width, &h);
    height = INDICATOR_OFFSET_Y + INDICATOR_BOX_SIZE + INDICATOR_GAP + h;

    if (m_Compass && !m_Parent->IsExtendedElevation()) {
	if (m_Degrees) {
	    str = wxString::Format("%03d", int(m_PanAngle));
	} else {
	    str = wxString::Format("%03d", int(m_PanAngle * 200.0 / 180.0));
	}
	GetTextExtent(str, &w, NULL);
	DrawIndicatorText(comp_centre_x + width / 2 - w, height, str);
	str = wxString(msg(/*Facing*/203));
	for (size_t i = 0; i < str.size(); ++i) {
	    unsigned char ch = str[i];
	    if (i + 1 < str.size() && ch >= 0xc0 && ch < 0xe0) {
		char buf[2];
		buf[0] = ((ch & 0x1f) << 6) | (str[i + 1] & 0x3f);
		buf[1] = '\0';
		str.replace(i, 2, buf);
	    }
	}
	GetTextExtent(str, &w, NULL);
	DrawIndicatorText(comp_centre_x - w / 2, height + h, str);
    }

    if (m_Clino) {
	int angle;
	if (m_Degrees) {
	    angle = int(-m_TiltAngle);
	} else {
	    angle = int(-m_TiltAngle * 200.0 / 180.0);
	}
	str = angle ? wxString::Format("%+03d", angle) : wxString("00");
	GetTextExtent(str, &w, NULL);
	DrawIndicatorText(elev_centre_x + width / 2 - w, height, str);
	str = wxString(msg(/*Elevation*/118));
	for (size_t i = 0; i < str.size(); ++i) {
	    unsigned char ch = str[i];
	    if (i + 1 < str.size() && ch >= 0xc0 && ch < 0xe0) {
		char buf[2];
		buf[0] = ((ch & 0x1f) << 6) | (str[i + 1] & 0x3f);
		buf[1] = '\0';
		str.replace(i, 2, buf);
	    }
	}
	GetTextExtent(str, &w, NULL);
	DrawIndicatorText(elev_centre_x - w / 2, height + h, str);
    }
}

void GfxCore::NattyDrawNames()
{
    // Draw station names, without overlapping.

    const unsigned int quantise(GetFontSize() / QUANTISE_FACTOR);
    const unsigned int quantised_x = m_XSize / quantise;
    const unsigned int quantised_y = m_YSize / quantise;
    const size_t buffer_size = quantised_x * quantised_y;

    if (!m_LabelGrid) m_LabelGrid = new char[buffer_size];

    memset((void*) m_LabelGrid, 0, buffer_size);

    list<LabelInfo*>::const_iterator label = m_Parent->GetLabels();
    for ( ; label != m_Parent->GetLabelsEnd(); ++label) {
	if (!((m_Surface && (*label)->IsSurface()) ||
	      (m_Legs && (*label)->IsUnderground()) ||
	      (!(*label)->IsSurface() && !(*label)->IsUnderground()))) {
	    // if this station isn't to be displayed, skip to the next
	    // (last case is for stns with no legs attached)
	    continue;
	}

	Double x, y, z;

	Transform(**label, &x, &y, &z);
	// Check if the label is behind us (in perspective view).
	if (z <= 0.0 || z >= 1.0) continue;

	// Apply a small shift so that translating the view doesn't make which
	// labels are displayed change as the resulting twinkling effect is
	// distracting.
	Double tx, ty, tz;
	Transform(Vector3(), &tx, &ty, &tz);
	tx -= floor(tx / quantise) * quantise;
	ty -= floor(ty / quantise) * quantise;

	tx = x - tx;
	if (tx < 0) continue;

	ty = y - ty;
	if (ty < 0) continue;

	unsigned int iy = unsigned(ty) / quantise;
	if (iy >= quantised_y) continue;
	unsigned int width = (*label)->get_width();
	unsigned int ix = unsigned(tx) / quantise;
	if (ix + width >= quantised_x) continue;

	char * test = m_LabelGrid + ix + iy * quantised_x;
	if (memchr(test, 1, width)) continue;

	x += 3;
	y -= GetFontSize() / 2;
	DrawIndicatorText((int)x, (int)y, (*label)->GetText());

	if (iy > QUANTISE_FACTOR) iy = QUANTISE_FACTOR;
	test -= quantised_x * iy;
	iy += 4;
	while (--iy && test < m_LabelGrid + buffer_size) {
	    memset(test, 1, width);
	    test += quantised_x;
	}
    }
}

void GfxCore::SimpleDrawNames()
{
    // Draw all station names, without worrying about overlaps
    list<LabelInfo*>::const_iterator label = m_Parent->GetLabels();
    for ( ; label != m_Parent->GetLabelsEnd(); ++label) {
	if (!((m_Surface && (*label)->IsSurface()) ||
	      (m_Legs && (*label)->IsUnderground()) ||
	      (!(*label)->IsSurface() && !(*label)->IsUnderground()))) {
	    // if this station isn't to be displayed, skip to the next
	    // (last case is for stns with no legs attached)
	    continue;
	}

	Double x, y, z;
	Transform(**label, &x, &y, &z);

	// Check if the label is behind us (in perspective view).
	if (z <= 0) continue;

	x += 3;
	y -= GetFontSize() / 2;
	DrawIndicatorText((int)x, (int)y, (*label)->GetText());
    }
}

void GfxCore::DrawDepthbar()
{
    const int total_block_height =
	DEPTH_BAR_BLOCK_HEIGHT * (GetNumDepthBands() - 1);
    const int top = -(total_block_height + DEPTH_BAR_OFFSET_Y);
    int size = 0;

    wxString* strs = new wxString[GetNumDepthBands()];
    int band;
    for (band = 0; band < GetNumDepthBands(); band++) {
	Double z = m_Parent->GetZMin() +
		   m_Parent->GetZExtent() * band / (GetNumDepthBands() - 1);

	strs[band] = FormatLength(z, false);

	int x, dummy;
	GetTextExtent(strs[band], &x, &dummy);
	if (x > size) size = x;
    }

    int left = -DEPTH_BAR_OFFSET_X - DEPTH_BAR_BLOCK_WIDTH
		- DEPTH_BAR_MARGIN - size;

    DrawRectangle(col_BLACK, col_DARK_GREY,
		  left - DEPTH_BAR_MARGIN - DEPTH_BAR_EXTRA_LEFT_MARGIN,
		  top - DEPTH_BAR_MARGIN * 2,
		  DEPTH_BAR_BLOCK_WIDTH + size + DEPTH_BAR_MARGIN * 3 +
		      DEPTH_BAR_EXTRA_LEFT_MARGIN,
		  total_block_height + DEPTH_BAR_MARGIN*4);

    int y = top;
    for (band = 0; band < GetNumDepthBands() - 1; band++) {
	DrawShadedRectangle(GetPen(band), GetPen(band + 1), left, y,
			    DEPTH_BAR_BLOCK_WIDTH, DEPTH_BAR_BLOCK_HEIGHT);
	y += DEPTH_BAR_BLOCK_HEIGHT;
    }

    y = top - GetFontSize() / 2 - 1;
    left += DEPTH_BAR_BLOCK_WIDTH + 5;

    SetColour(TEXT_COLOUR);
    for (band = 0; band < GetNumDepthBands(); band++) {
	DrawIndicatorText(left, y, strs[band]);
	y += DEPTH_BAR_BLOCK_HEIGHT;
    }

    delete[] strs;
}

void GfxCore::DrawDatebar()
{
    const int total_block_height =
	DEPTH_BAR_BLOCK_HEIGHT * (GetNumDepthBands() - 1);
    const int top = -(total_block_height + DEPTH_BAR_OFFSET_Y);
    int size = 0;

    wxString* strs = new wxString[GetNumDepthBands()];
    char buf[128];
    int band;
    for (band = 0; band < GetNumDepthBands(); band++) {
	time_t date = m_Parent->GetDateMin() +
		   time_t((double)m_Parent->GetDateExtent() * band / (GetNumDepthBands() - 1));
	size_t res = strftime(buf, sizeof(buf), "%Y-%m-%d", gmtime(&date));
	// Insert extra "" to avoid trigraphs issues.
	if (res == 0 || res == sizeof(buf)) strcpy(buf, "?""?""?""?-?""?-?""?");
	strs[band] = buf;

	int x, dummy;
	GetTextExtent(strs[band], &x, &dummy);
	if (x > size) size = x;
    }

    int left = -DEPTH_BAR_OFFSET_X - DEPTH_BAR_BLOCK_WIDTH
		- DEPTH_BAR_MARGIN - size;

    DrawRectangle(col_BLACK, col_DARK_GREY,
		  left - DEPTH_BAR_MARGIN - DEPTH_BAR_EXTRA_LEFT_MARGIN,
		  top - DEPTH_BAR_MARGIN * 2,
		  DEPTH_BAR_BLOCK_WIDTH + size + DEPTH_BAR_MARGIN * 3 +
		      DEPTH_BAR_EXTRA_LEFT_MARGIN,
		  total_block_height + DEPTH_BAR_MARGIN*4);

    int y = top;
    for (band = 0; band < GetNumDepthBands() - 1; band++) {
	DrawShadedRectangle(GetPen(band), GetPen(band + 1), left, y,
			    DEPTH_BAR_BLOCK_WIDTH, DEPTH_BAR_BLOCK_HEIGHT);
	y += DEPTH_BAR_BLOCK_HEIGHT;
    }

    y = top - GetFontSize() / 2 - 1;
    left += DEPTH_BAR_BLOCK_WIDTH + 5;

    SetColour(TEXT_COLOUR);
    for (band = 0; band < GetNumDepthBands(); band++) {
	DrawIndicatorText(left, y, strs[band]);
	y += DEPTH_BAR_BLOCK_HEIGHT;
    }

    delete[] strs;
}

wxString GfxCore::FormatLength(Double size_snap, bool scalebar)
{
    wxString str;
    bool negative = (size_snap < 0.0);

    if (negative) {
	size_snap = -size_snap;
    }

    if (size_snap == 0.0) {
	str = "0";
    } else if (m_Metric) {
#ifdef SILLY_UNITS
	if (size_snap < 1e-12) {
	    str = wxString::Format("%.3gpm", size_snap * 1e12);
	} else if (size_snap < 1e-9) {
	    str = wxString::Format("%.fpm", size_snap * 1e12);
	} else if (size_snap < 1e-6) {
	    str = wxString::Format("%.fnm", size_snap * 1e9);
	} else if (size_snap < 1e-3) {
	    str = wxString::Format("%.fum", size_snap * 1e6);
#else
	if (size_snap < 1e-3) {
	    str = wxString::Format("%.3gmm", size_snap * 1e3);
#endif
	} else if (size_snap < 1e-2) {
	    str = wxString::Format("%.fmm", size_snap * 1e3);
	} else if (size_snap < 1.0) {
	    str = wxString::Format("%.fcm", size_snap * 100.0);
	} else if (size_snap < 1e3) {
	    str = wxString::Format("%.fm", size_snap);
#ifdef SILLY_UNITS
	} else if (size_snap < 1e6) {
	    str = wxString::Format("%.fkm", size_snap * 1e-3);
	} else if (size_snap < 1e9) {
	    str = wxString::Format("%.fMm", size_snap * 1e-6);
	} else {
	    str = wxString::Format("%.fGm", size_snap * 1e-9);
#else
	} else {
	    str = wxString::Format(scalebar ? "%.fkm" : "%.2fkm", size_snap * 1e-3);
#endif
	}
    } else {
	size_snap /= METRES_PER_FOOT;
	if (scalebar) {
	    Double inches = size_snap * 12;
	    if (inches < 1.0) {
		str = wxString::Format("%.3gin", inches);
	    } else if (size_snap < 1.0) {
		str = wxString::Format("%.fin", inches);
	    } else if (size_snap < 5279.5) {
		str = wxString::Format("%.fft", size_snap);
	    } else {
		str = wxString::Format("%.f miles", size_snap / 5280.0);
	    }
	} else {
	    str = wxString::Format("%.fft", size_snap);
	}
    }

    return negative ? wxString("-") + str : str;
}

void GfxCore::DrawScalebar()
{
    // Draw the scalebar.
    if (GetPerspective()) return;

    // Calculate how many metres of survey are currently displayed across the
    // screen.
    Double across_screen = SurveyUnitsAcrossViewport();

    // Convert to imperial measurements if required.
    Double multiplier = 1.0;
    if (!m_Metric) {
	across_screen /= METRES_PER_FOOT;
	multiplier = METRES_PER_FOOT;
	if (across_screen >= 5280.0 / 0.75) {
	    across_screen /= 5280.0;
	    multiplier *= 5280.0;
	}
    }

    // Calculate the length of the scale bar.
    Double size_snap = pow(10.0, floor(log10(0.65 * across_screen)));
    Double t = across_screen * 0.65 / size_snap;
    if (t >= 5.0) {
	size_snap *= 5.0;
    } else if (t >= 2.0) {
	size_snap *= 2.0;
    }

    if (!m_Metric) size_snap *= multiplier;

    // Actual size of the thing in pixels:
    int size = int((size_snap / SurveyUnitsAcrossViewport()) * m_XSize);
    m_ScaleBarWidth = size;

    // Draw it...
    const int end_y = SCALE_BAR_OFFSET_Y + SCALE_BAR_HEIGHT;
    int interval = size / 10;

    gla_colour col = col_WHITE;
    for (int ix = 0; ix < 10; ix++) {
	int x = SCALE_BAR_OFFSET_X + int(ix * ((Double) size / 10.0));

	DrawRectangle(col, col, x, end_y, interval + 2, SCALE_BAR_HEIGHT);

	col = (col == col_WHITE) ? col_GREY : col_WHITE;
    }

    // Add labels.
    wxString str = FormatLength(size_snap);

    int text_width, text_height;
    GetTextExtent(str, &text_width, &text_height);
    const int text_y = end_y - text_height + 1;
    SetColour(TEXT_COLOUR);
    DrawIndicatorText(SCALE_BAR_OFFSET_X, text_y, "0");
    DrawIndicatorText(SCALE_BAR_OFFSET_X + size - text_width, text_y, str);
}

bool GfxCore::CheckHitTestGrid(const wxPoint& point, bool centre)
{
    if (Animating()) return false;

    if (point.x < 0 || point.x >= m_XSize ||
	point.y < 0 || point.y >= m_YSize) {
	return false;
    }

    SetDataTransform();

    if (!m_HitTestGridValid) CreateHitTestGrid();

    int grid_x = (point.x * (HITTEST_SIZE - 1)) / m_XSize;
    int grid_y = (point.y * (HITTEST_SIZE - 1)) / m_YSize;

    LabelInfo *best = NULL;
    int dist_sqrd = 25;
    int square = grid_x + grid_y * HITTEST_SIZE;
    list<LabelInfo*>::iterator iter = m_PointGrid[square].begin();

    while (iter != m_PointGrid[square].end()) {
	LabelInfo *pt = *iter++;

	Double cx, cy, cz;

	Transform(*pt, &cx, &cy, &cz);

	cy = m_YSize - cy;

	int dx = point.x - int(cx);
	int ds = dx * dx;
	if (ds >= dist_sqrd) continue;
	int dy = point.y - int(cy);

	ds += dy * dy;
	if (ds >= dist_sqrd) continue;

	dist_sqrd = ds;
	best = pt;

	if (ds == 0) break;
    }

    m_Parent->ShowInfo(best);
    if (best) {
	if (centre) {
	    // FIXME: allow Ctrl-Click to not set there or something?
	    CentreOn(*best);
	    WarpPointer(m_XSize / 2, m_YSize / 2);
	    SetThere(*best);
	    m_Parent->SelectTreeItem(best);
	}
    } else {
	// Left-clicking not on a survey cancels the measuring line.
	if (centre) {
	    ClearTreeSelection();
	} else {
	    Double x, y, z;
	    ReverseTransform(point.x, m_YSize - point.y, &x, &y, &z);
	    SetHere(Point(Vector3(x, y, z)));
	}
    }

    return best;
}

void GfxCore::OnSize(wxSizeEvent& event)
{
    // Handle a change in window size.

    wxSize size = event.GetSize();

    if (size.GetWidth() <= 0 || size.GetHeight() <= 0) {
	// Before things are fully initialised, we sometimes get a bogus
	// resize message...
	// FIXME have changes in MainFrm cured this?  It still happens with
	// 1.0.32 and wxGtk 2.5.2 (load a file from the command line).
	// With 1.1.6 and wxGtk 2.4.2 we only get negative sizes if MainFrm
	// is resized such that the GfxCore window isn't visible.
	//printf("OnSize(%d,%d)\n", size.GetWidth(), size.GetHeight());
	return;
    }
    m_XSize = size.GetWidth();
    m_YSize = size.GetHeight();

    if (m_DoneFirstShow) {
	if (m_LabelGrid) {
	    delete[] m_LabelGrid;
	    m_LabelGrid = NULL;
	}

	m_HitTestGridValid = false;

	ForceRefresh();
    }
}

void GfxCore::DefaultParameters()
{
    // Set default viewing parameters.

    m_Surface = false;
    if (!m_Parent->HasUndergroundLegs()) {
	if (m_Parent->HasSurfaceLegs()) {
	    // If there are surface legs, but no underground legs, turn
	    // surface surveys on.
	    m_Surface = true;
	} else {
	    // If there are no legs (e.g. after loading a .pos file), turn
	    // crosses on.
	    m_Crosses = true;
	}
    }

    m_PanAngle = 0.0;
    if (m_Parent->IsExtendedElevation()) {
	m_TiltAngle = 0.0;
    } else {
	m_TiltAngle = 90.0;
    }

    UpdateQuaternion();
    SetTranslation(Vector3());

    m_RotationStep = 30.0;
    m_Rotating = false;
    m_SwitchingTo = 0;
    m_Entrances = false;
    m_FixedPts = false;
    m_ExportedPts = false;
    m_Grid = false;
    m_BoundingBox = false;
    m_Tubes = false;
    if (GetPerspective()) TogglePerspective();
}

void GfxCore::Defaults()
{
    // Restore default scale, rotation and translation parameters.

    DefaultParameters();
    SetScale(1.0);
    ForceRefresh();
}

// return: true if animation occured (and ForceRefresh() needs to be called)
bool GfxCore::Animate()
{
    if (!Animating()) return false;

    // Don't show pointer coordinates while animating.
    // FIXME : only do this when we *START* animating!  Use a static copy
    // of the value of "Animating()" last time we were here to track this?
    // MainFrm now checks if we're trying to clear already cleared labels
    // and just returns, but it might be simpler to check here!
    ClearCoords();
    m_Parent->ShowInfo(NULL);

    static double last_t = 0;
    double t;
    if (mpeg) {
	// FIXME: this glReadPixels call should be in gla-gl.cc
	glReadPixels(0, 0, mpeg->GetWidth(), mpeg->GetHeight(), GL_RGB,
		     GL_UNSIGNED_BYTE, (GLvoid *)mpeg->GetBuffer());
	mpeg->AddFrame();
	t = 1.0 / 25.0; // 25 frames per second
    } else {
	t = timer.Time() * 1.0e-3;
	if (t == 0) t = 0.001;
	else if (t > 1.0) t = 1.0;
	if (last_t > 0) t = (t + last_t) / 2;
	last_t = t;
    }

    if (presentation_mode == PLAYING && pres_speed != 0.0) {
	t *= fabs(pres_speed);
	while (t >= next_mark_time) {
	    t -= next_mark_time;
	    this_mark_total = 0;
	    PresentationMark prev_mark = next_mark;
	    if (prev_mark.angle < 0) prev_mark.angle += 360.0;
	    else if (prev_mark.angle >= 360.0) prev_mark.angle -= 360.0;
	    if (pres_reverse)
		next_mark = m_Parent->GetPresMark(MARK_PREV);
	    else
		next_mark = m_Parent->GetPresMark(MARK_NEXT);
	    if (!next_mark.is_valid()) {
		SetView(prev_mark);
		presentation_mode = 0;
		if (mpeg) {
		    delete mpeg;
		    mpeg = 0;
		}
		break;
	    }

	    double tmp = (pres_reverse ? prev_mark.time : next_mark.time);
	    if (tmp > 0) {
		next_mark_time = tmp;
	    } else {
		double d = (next_mark - prev_mark).magnitude();
		// FIXME: should ignore component of d which is unseen in
		// non-perspective mode?
		next_mark_time = sqrd(d / 30.0);
		double a = next_mark.angle - prev_mark.angle;
		if (a > 180.0) {
		    next_mark.angle -= 360.0;
		    a = 360.0 - a;
		} else if (a < -180.0) {
		    next_mark.angle += 360.0;
		    a += 360.0;
		} else {
		    a = fabs(a);
		}
		next_mark_time += sqrd(a / 60.0);
		double ta = fabs(next_mark.tilt_angle - prev_mark.tilt_angle);
		next_mark_time += sqrd(ta / 60.0);
		double s = fabs(log(next_mark.scale) - log(prev_mark.scale));
		next_mark_time += sqrd(s / 2.0);
		next_mark_time = sqrt(next_mark_time);
		// was: next_mark_time = max(max(d / 30, s / 2), max(a, ta) / 60);
		//printf("*** %.6f from (\nd: %.6f\ns: %.6f\na: %.6f\nt: %.6f )\n",
		//       next_mark_time, d/30.0, s/2.0, a/60.0, ta/60.0);
		if (tmp < 0) next_mark_time /= -tmp;
	    }
	}

	if (presentation_mode) {
	    // Advance position towards next_mark
	    double p = t / next_mark_time;
	    double q = 1 - p;
	    PresentationMark here = GetView();
	    if (next_mark.angle < 0) {
		if (here.angle >= next_mark.angle + 360.0)
		    here.angle -= 360.0;
	    } else if (next_mark.angle >= 360.0) {
		if (here.angle <= next_mark.angle - 360.0)
		    here.angle += 360.0;
	    }
	    here.assign(q * here + p * next_mark);
	    here.angle = q * here.angle + p * next_mark.angle;
	    if (here.angle < 0) here.angle += 360.0;
	    else if (here.angle >= 360.0) here.angle -= 360.0;
	    here.tilt_angle = q * here.tilt_angle + p * next_mark.tilt_angle;
	    here.scale = exp(q * log(here.scale) + p * log(next_mark.scale));
	    SetView(here);
	    this_mark_total += t;
	    next_mark_time -= t;
	}
    }

    // When rotating...
    if (m_Rotating) {
	TurnCave(m_RotationStep * t);
    }

    if (m_SwitchingTo == PLAN) {
	// When switching to plan view...
	TiltCave(90.0 * t);
	if (m_TiltAngle == 90.0) {
	    m_SwitchingTo = 0;
	}
    } else if (m_SwitchingTo == ELEVATION) {
	// When switching to elevation view...
	if (fabs(m_TiltAngle) < 90.0 * t) {
	    m_SwitchingTo = 0;
	    TiltCave(-m_TiltAngle);
	} else if (m_TiltAngle < 0.0) {
	    TiltCave(90.0 * t);
	} else {
	    TiltCave(-90.0 * t);
	}
    } else if (m_SwitchingTo) {
	int angle = (m_SwitchingTo - NORTH) * 90;
	double diff = angle - m_PanAngle;
	if (diff < -180) diff += 360;
	if (diff > 180) diff -= 360;
	double move = 90.0 * t;
	if (move >= fabs(diff)) {
	    TurnCave(diff);
	    m_SwitchingTo = 0;
	} else if (diff < 0) {
	    TurnCave(-move);
	} else {
	    TurnCave(move);
	}
    }

    return true;
}

// How much to allow around the box - this is because of the ring shape
// at one end of the line.
static const int HIGHLIGHTED_PT_SIZE = 2; // FIXME: tie in to blob and ring size
#define MARGIN (HIGHLIGHTED_PT_SIZE * 2 + 1)
void GfxCore::RefreshLine(const Point &a, const Point &b, const Point &c)
{
    // Best of all might be to copy the window contents before we draw the
    // line, then replace each time we redraw.

    // Calculate the minimum rectangle which includes the old and new
    // measuring lines to minimise the redraw time
    int l = INT_MAX, r = INT_MIN, u = INT_MIN, d = INT_MAX;
    Double X, Y, Z;
    if (a.IsValid()) {
	if (!Transform(a, &X, &Y, &Z)) {
	    printf("oops\n");
	} else {
	    int x = int(X);
	    int y = m_YSize - 1 - int(Y);
	    l = x - MARGIN;
	    r = x + MARGIN;
	    u = y + MARGIN;
	    d = y - MARGIN;
	}
    }
    if (b.IsValid()) {
	if (!Transform(b, &X, &Y, &Z)) {
	    printf("oops\n");
	} else {
	    int x = int(X);
	    int y = m_YSize - 1 - int(Y);
	    l = min(l, x - MARGIN);
	    r = max(r, x + MARGIN);
	    u = max(u, y + MARGIN);
	    d = min(d, y - MARGIN);
	}
    }
    if (c.IsValid()) {
	if (!Transform(c, &X, &Y, &Z)) {
	    printf("oops\n");
	} else {
	    int x = int(X);
	    int y = m_YSize - 1 - int(Y);
	    l = min(l, x - MARGIN);
	    r = max(r, x + MARGIN);
	    u = max(u, y + MARGIN);
	    d = min(d, y - MARGIN);
	}
    }
    const wxRect R(l, d, r - l, u - d);
    Refresh(false, &R);
}

void GfxCore::SetHere()
{
    if (!m_here.IsValid()) return;
    Point old = m_here;
    m_here.Invalidate();
    RefreshLine(old, m_there, m_here);
}

void GfxCore::SetHere(const Point &p)
{
    Point old = m_here;
    m_here = p;
    RefreshLine(old, m_there, m_here);
}

void GfxCore::SetThere()
{
    if (!m_there.IsValid()) return;
    Point old = m_there;
    m_there.Invalidate();
    RefreshLine(m_here, old, m_there);
}

void GfxCore::SetThere(const Point &p)
{
    Point old = m_there;
    m_there = p;
    RefreshLine(m_here, old, m_there);
}

void GfxCore::CreateHitTestGrid()
{
    // Clear hit-test grid.
    for (int i = 0; i < HITTEST_SIZE * HITTEST_SIZE; i++) {
	m_PointGrid[i].clear();
    }

    // Fill the grid.
    list<LabelInfo*>::const_iterator pos = m_Parent->GetLabels();
    list<LabelInfo*>::const_iterator end = m_Parent->GetLabelsEnd();
    while (pos != end) {
	LabelInfo* label = *pos++;

	if (!((m_Surface && label->IsSurface()) ||
	      (m_Legs && label->IsUnderground()) ||
	      (!label->IsSurface() && !label->IsUnderground()))) {
	    // if this station isn't to be displayed, skip to the next
	    // (last case is for stns with no legs attached)
	    continue;
	}

	// Calculate screen coordinates.
	Double cx, cy, cz;
	Transform(*label, &cx, &cy, &cz);
	if (cx < 0 || cx >= m_XSize) continue;
	if (cy < 0 || cy >= m_YSize) continue;

	cy = m_YSize - cy;

	// On-screen, so add to hit-test grid...
	int grid_x = int((cx * (HITTEST_SIZE - 1)) / m_XSize);
	int grid_y = int((cy * (HITTEST_SIZE - 1)) / m_YSize);

	m_PointGrid[grid_x + grid_y * HITTEST_SIZE].push_back(label);
    }

    m_HitTestGridValid = true;
}

void GfxCore::UpdateQuaternion()
{
    // Produce the rotation quaternion from the pan and tilt angles.
    Vector3 v1(0.0, 0.0, 1.0);
    Vector3 v2(1.0, 0.0, 0.0);
    Quaternion q1(v1, rad(m_PanAngle));
    Quaternion q2(v2, rad(m_TiltAngle));

    // care: quaternion multiplication is not commutative!
    Quaternion rotation = q2 * q1;
    SetRotation(rotation);
}

//
//  Methods for controlling the orientation of the survey
//

void GfxCore::TurnCave(Double angle)
{
    // Turn the cave around its z-axis by a given angle.

    m_PanAngle += angle;
    if (m_PanAngle >= 360.0) {
	m_PanAngle -= 360.0;
    } else if (m_PanAngle < 0.0) {
	m_PanAngle += 360.0;
    }

    m_HitTestGridValid = false;

    UpdateQuaternion();
}

void GfxCore::TurnCaveTo(Double angle)
{
    timer.Start(drawtime);
    int new_switching_to = ((int)angle) / 90 + NORTH;
    if (new_switching_to == m_SwitchingTo) {
	// A second order to switch takes us there right away
	TurnCave(angle - m_PanAngle);
	m_SwitchingTo = 0;
	ForceRefresh();
    } else {
	m_SwitchingTo = new_switching_to;
    }
}

void GfxCore::TiltCave(Double tilt_angle)
{
    // Tilt the cave by a given angle.
    if (m_TiltAngle + tilt_angle > 90.0) {
	tilt_angle = 90.0 - m_TiltAngle;
    } else if (m_TiltAngle + tilt_angle < -90.0) {
	tilt_angle = -90.0 - m_TiltAngle;
    }

    m_TiltAngle += tilt_angle;

    m_HitTestGridValid = false;

    UpdateQuaternion();
}

void GfxCore::TranslateCave(int dx, int dy)
{
    AddTranslationScreenCoordinates(dx, dy);
    m_HitTestGridValid = false;

    ForceRefresh();
}

void GfxCore::DragFinished()
{
    m_MouseOutsideCompass = m_MouseOutsideElev = false;
    ForceRefresh();
}

void GfxCore::ClearCoords()
{
    m_Parent->ClearCoords();
}

void GfxCore::SetCoords(wxPoint point)
{
    // We can't work out 2D coordinates from a perspective view, and it
    // doesn't really make sense to show coordinates while we're animating.
    if (GetPerspective() || Animating()) return;

    // Update the coordinate or altitude display, given the (x, y) position in
    // window coordinates.  The relevant display is updated depending on
    // whether we're in plan or elevation view.

    Double cx, cy, cz;

    SetDataTransform();
    ReverseTransform(point.x, m_YSize - 1 - point.y, &cx, &cy, &cz);

    if (ShowingPlan()) {
	m_Parent->SetCoords(cx + m_Parent->GetOffset().GetX(),
			    cy + m_Parent->GetOffset().GetY());
    } else if (ShowingElevation()) {
	m_Parent->SetAltitude(cz + m_Parent->GetOffset().GetZ());
    } else {
	m_Parent->ClearCoords();
    }
}

int GfxCore::GetCompassXPosition() const
{
    // Return the x-coordinate of the centre of the compass in window
    // coordinates.
    return m_XSize - INDICATOR_OFFSET_X - INDICATOR_BOX_SIZE / 2;
}

int GfxCore::GetClinoXPosition() const
{
    // Return the x-coordinate of the centre of the compass in window
    // coordinates.
    return m_XSize - GetClinoOffset() - INDICATOR_BOX_SIZE / 2;
}

int GfxCore::GetIndicatorYPosition() const
{
    // Return the y-coordinate of the centre of the indicators in window
    // coordinates.
    return m_YSize - INDICATOR_OFFSET_Y - INDICATOR_BOX_SIZE / 2;
}

int GfxCore::GetIndicatorRadius() const
{
    // Return the radius of each indicator.
    return (INDICATOR_BOX_SIZE - INDICATOR_MARGIN * 2) / 2;
}

bool GfxCore::PointWithinCompass(wxPoint point) const
{
    // Determine whether a point (in window coordinates) lies within the
    // compass.
    if (!ShowingCompass()) return false;

    glaCoord dx = point.x - GetCompassXPosition();
    glaCoord dy = point.y - GetIndicatorYPosition();
    glaCoord radius = GetIndicatorRadius();

    return (dx * dx + dy * dy <= radius * radius);
}

bool GfxCore::PointWithinClino(wxPoint point) const
{
    // Determine whether a point (in window coordinates) lies within the clino.
    if (!ShowingClino()) return false;

    glaCoord dx = point.x - GetClinoXPosition();
    glaCoord dy = point.y - GetIndicatorYPosition();
    glaCoord radius = GetIndicatorRadius();

    return (dx * dx + dy * dy <= radius * radius);
}

bool GfxCore::PointWithinScaleBar(wxPoint point) const
{
    // Determine whether a point (in window coordinates) lies within the scale
    // bar.
    if (!ShowingScaleBar()) return false;

    return (point.x >= SCALE_BAR_OFFSET_X &&
	    point.x <= SCALE_BAR_OFFSET_X + m_ScaleBarWidth &&
	    point.y <= m_YSize - SCALE_BAR_OFFSET_Y - SCALE_BAR_HEIGHT &&
	    point.y >= m_YSize - SCALE_BAR_OFFSET_Y - SCALE_BAR_HEIGHT*2);
}

void GfxCore::SetCompassFromPoint(wxPoint point)
{
    // Given a point in window coordinates, set the heading of the survey.  If
    // the point is outside the compass, it snaps to 45 degree intervals;
    // otherwise it operates as normal.

    wxCoord dx = point.x - GetCompassXPosition();
    wxCoord dy = point.y - GetIndicatorYPosition();
    wxCoord radius = GetIndicatorRadius();

    double angle = deg(atan2(double(dx), double(dy))) - 180.0;
    if (dx * dx + dy * dy <= radius * radius) {
	TurnCave(angle - m_PanAngle);
	m_MouseOutsideCompass = false;
    } else {
	TurnCave(int(angle / 45.0) * 45.0 - m_PanAngle);
	m_MouseOutsideCompass = true;
    }

    ForceRefresh();
}

void GfxCore::SetClinoFromPoint(wxPoint point)
{
    // Given a point in window coordinates, set the elevation of the survey.
    // If the point is outside the clino, it snaps to 90 degree intervals;
    // otherwise it operates as normal.

    glaCoord dx = point.x - GetClinoXPosition();
    glaCoord dy = point.y - GetIndicatorYPosition();
    glaCoord radius = GetIndicatorRadius();

    if (dx >= 0 && dx * dx + dy * dy <= radius * radius) {
	TiltCave(deg(atan2(double(dy), double(dx))) - m_TiltAngle);
	m_MouseOutsideElev = false;
    } else if (dy >= INDICATOR_MARGIN) {
	TiltCave(90.0 - m_TiltAngle);
	m_MouseOutsideElev = true;
    } else if (dy <= -INDICATOR_MARGIN) {
	TiltCave(-90.0 - m_TiltAngle);
	m_MouseOutsideElev = true;
    } else {
	TiltCave(-m_TiltAngle);
	m_MouseOutsideElev = true;
    }

    ForceRefresh();
}

void GfxCore::SetScaleBarFromOffset(wxCoord dx)
{
    // Set the scale of the survey, given an offset as to how much the mouse has
    // been dragged over the scalebar since the last scale change.

    SetScale((m_ScaleBarWidth + dx) * m_Scale / m_ScaleBarWidth);
    ForceRefresh();
}

void GfxCore::RedrawIndicators()
{
    // Redraw the compass and clino indicators.

    const wxRect r(m_XSize - INDICATOR_OFFSET_X - INDICATOR_BOX_SIZE*2 -
		     INDICATOR_GAP,
		   m_YSize - INDICATOR_OFFSET_Y - INDICATOR_BOX_SIZE,
		   INDICATOR_BOX_SIZE*2 + INDICATOR_GAP,
		   INDICATOR_BOX_SIZE);

    Refresh(false, &r);
}

void GfxCore::StartRotation()
{
    // Start the survey rotating.

    m_Rotating = true;
    timer.Start(drawtime);
}

void GfxCore::ToggleRotation()
{
    // Toggle the survey rotation on/off.

    if (m_Rotating) {
	StopRotation();
    } else {
	StartRotation();
    }
}

void GfxCore::StopRotation()
{
    // Stop the survey rotating.

    m_Rotating = false;
    ForceRefresh();
}

bool GfxCore::IsExtendedElevation() const
{
    return m_Parent->IsExtendedElevation();
}

void GfxCore::ReverseRotation()
{
    // Reverse the direction of rotation.

    m_RotationStep = -m_RotationStep;
}

void GfxCore::RotateSlower(bool accel)
{
    // Decrease the speed of rotation, optionally by an increased amount.

    m_RotationStep /= accel ? 1.44 : 1.2;
    if (m_RotationStep < 1.0) {
	m_RotationStep = 1.0;
    }
}

void GfxCore::RotateFaster(bool accel)
{
    // Increase the speed of rotation, optionally by an increased amount.

    m_RotationStep *= accel ? 1.44 : 1.2;
    if (m_RotationStep > 450.0) {
	m_RotationStep = 450.0;
    }
}

void GfxCore::SwitchToElevation()
{
    // Perform an animated switch to elevation view.

    switch (m_SwitchingTo) {
	case 0:
	    timer.Start(drawtime);
	    m_SwitchingTo = ELEVATION;
	    break;

	case PLAN:
	    m_SwitchingTo = ELEVATION;
	    break;

	case ELEVATION:
	    // a second order to switch takes us there right away
	    TiltCave(-m_TiltAngle);
	    m_SwitchingTo = 0;
	    ForceRefresh();
    }
}

void GfxCore::SwitchToPlan()
{
    // Perform an animated switch to plan view.

    switch (m_SwitchingTo) {
	case 0:
	    timer.Start(drawtime);
	    m_SwitchingTo = PLAN;
	    break;

	case ELEVATION:
	    m_SwitchingTo = PLAN;
	    break;

	case PLAN:
	    // A second order to switch takes us there right away
	    TiltCave(90.0 - m_TiltAngle);
	    m_SwitchingTo = 0;
	    ForceRefresh();
    }
}

void GfxCore::SetViewTo(Double xmin, Double xmax, Double ymin, Double ymax, Double zmin, Double zmax)
{

    SetTranslation(-Vector3((xmin + xmax) / 2, (ymin + ymax) / 2, (zmin + zmax) / 2));
    Double scale = 1000.0;
    const Vector3 ext = m_Parent->GetExtent();
    if (xmax > xmin) {
	Double s = ext.GetX() / (xmax - xmin);
	if (s < scale) scale = s;
    }
    if (ymax > ymin) {
	Double s = ext.GetY() / (ymax - ymin);
	if (s < scale) scale = s;
    }
    if (!ShowingPlan() && zmax > zmin) {
	Double s = ext.GetZ() / (zmax - zmin);
	if (s < scale) scale = s;
    }
    SetScale(scale);
    ForceRefresh();
}

bool GfxCore::CanRaiseViewpoint() const
{
    // Determine if the survey can be viewed from a higher angle of elevation.

    return GetPerspective() ? (m_TiltAngle > -90.0) : (m_TiltAngle < 90.0);
}

bool GfxCore::CanLowerViewpoint() const
{
    // Determine if the survey can be viewed from a lower angle of elevation.

    return GetPerspective() ? (m_TiltAngle < 90.0) : (m_TiltAngle > -90.0);
}

bool GfxCore::IsFlat() const
{
    return m_Parent->GetZExtent() == 0.0;
}

bool GfxCore::HasRangeOfDates() const
{
    return m_Parent->GetDateExtent() > 0;
}

bool GfxCore::ShowingPlan() const
{
    // Determine if the survey is in plan view.

    return (m_TiltAngle == 90.0);
}

bool GfxCore::ShowingElevation() const
{
    // Determine if the survey is in elevation view.

    return (m_TiltAngle == 0.0);
}

bool GfxCore::ShowingMeasuringLine() const
{
    // Determine if the measuring line is being shown.

    return (m_there.IsValid());
}

void GfxCore::ToggleFlag(bool* flag, int update)
{
    *flag = !*flag;
    if (update == UPDATE_BLOBS) {
	UpdateBlobs();
    } else if (update == UPDATE_BLOBS_AND_CROSSES) {
	UpdateBlobs();
	InvalidateList(LIST_CROSSES);
    }
    ForceRefresh();
}

int GfxCore::GetNumEntrances() const
{
    return m_Parent->GetNumEntrances();
}

int GfxCore::GetNumFixedPts() const
{
    return m_Parent->GetNumFixedPts();
}

int GfxCore::GetNumExportedPts() const
{
    return m_Parent->GetNumExportedPts();
}

void GfxCore::ClearTreeSelection()
{
    m_Parent->ClearTreeSelection();
}

void GfxCore::CentreOn(const Point &p)
{
    SetTranslation(-p);
    m_HitTestGridValid = false;

    ForceRefresh();
}

void GfxCore::ForceRefresh()
{
    Refresh(false);
}

void GfxCore::GenerateList(unsigned int l)
{
    assert(m_HaveData);

    switch (l) {
	case LIST_COMPASS:
	    DrawCompass();
	    break;
	case LIST_CLINO:
	    DrawClino();
	    break;
	case LIST_CLINO_BACK:
	    DrawClinoBack();
	    break;
	case LIST_DEPTHBAR:
	    DrawDepthbar();
	    break;
	case LIST_DATEBAR:
	    DrawDatebar();
	    break;
	case LIST_UNDERGROUND_LEGS:
	    GenerateDisplayList();
	    break;
	case LIST_TUBES:
	    GenerateDisplayListTubes();
	    break;
	case LIST_SURFACE_LEGS:
	    GenerateDisplayListSurface();
	    break;
	case LIST_BLOBS:
	    GenerateBlobsDisplayList();
	    break;
	case LIST_CROSSES: {
	    BeginCrosses();
	    SetColour(col_LIGHT_GREY);
	    list<LabelInfo*>::const_iterator pos = m_Parent->GetLabels();
	    while (pos != m_Parent->GetLabelsEnd()) {
		const LabelInfo* label = *pos++;

		if ((m_Surface && label->IsSurface()) ||
		    (m_Legs && label->IsUnderground()) ||
		    (!label->IsSurface() && !label->IsUnderground())) {
		    // Check if this station should be displayed
		    // (last case is for stns with no legs attached)
		    DrawCross(label->GetX(), label->GetY(), label->GetZ());
		}
	    }
	    EndCrosses();
	    break;
	}
	case LIST_GRID:
	    DrawGrid();
	    break;
	case LIST_SHADOW:
	    GenerateDisplayListShadow();
	    break;
	default:
	    assert(false);
	    break;
    }
}

void GfxCore::ToggleSmoothShading()
{
    GLACanvas::ToggleSmoothShading();
    InvalidateList(LIST_TUBES);
    ForceRefresh();
}

void GfxCore::GenerateDisplayList()
{
    // Generate the display list for the underground legs.
    list<vector<PointInfo> >::const_iterator trav = m_Parent->traverses_begin();
    list<vector<PointInfo> >::const_iterator tend = m_Parent->traverses_end();
    while (trav != tend) {
	(this->*AddPoly)(*trav);
	++trav;
    }
}

void GfxCore::GenerateDisplayListTubes()
{
    // Generate the display list for the tubes.
    list<vector<XSect> >::const_iterator trav = m_Parent->tubes_begin();
    list<vector<XSect> >::const_iterator tend = m_Parent->tubes_end();
    while (trav != tend) {
	SkinPassage(*trav);
	++trav;
    }
}

void GfxCore::GenerateDisplayListSurface()
{
    // Generate the display list for the surface legs.
    EnableDashedLines();
    list<vector<PointInfo> >::const_iterator trav = m_Parent->surface_traverses_begin();
    list<vector<PointInfo> >::const_iterator tend = m_Parent->surface_traverses_end();
    while (trav != tend) {
	AddPolyline(*trav);
	++trav;
    }
    DisableDashedLines();
}

void GfxCore::GenerateDisplayListShadow()
{
    SetColour(col_BLACK);
    list<vector<PointInfo> >::const_iterator trav = m_Parent->traverses_begin();
    list<vector<PointInfo> >::const_iterator tend = m_Parent->traverses_end();
    while (trav != tend) {
	AddPolylineShadow(*trav);
	++trav;
    }
}

// Plot blobs.
void GfxCore::GenerateBlobsDisplayList()
{
    if (!(m_Entrances || m_FixedPts || m_ExportedPts ||
	  m_Parent->GetNumHighlightedPts()))
	return;

    // Plot blobs.
    gla_colour prev_col = col_BLACK; // not a colour used for blobs
    list<LabelInfo*>::const_iterator pos = m_Parent->GetLabels();
    BeginBlobs();
    while (pos != m_Parent->GetLabelsEnd()) {
	const LabelInfo* label = *pos++;

	// When more than one flag is set on a point:
	// search results take priority over entrance highlighting
	// which takes priority over fixed point
	// highlighting, which in turn takes priority over exported
	// point highlighting.

	if (!((m_Surface && label->IsSurface()) ||
	      (m_Legs && label->IsUnderground()) ||
	      (!label->IsSurface() && !label->IsUnderground()))) {
	    // if this station isn't to be displayed, skip to the next
	    // (last case is for stns with no legs attached)
	    continue;
	}

	gla_colour col;

	if (label->IsHighLighted()) {
	    col = col_YELLOW;
	} else if (m_Entrances && label->IsEntrance()) {
	    col = col_GREEN;
	} else if (m_FixedPts && label->IsFixedPt()) {
	    col = col_RED;
	} else if (m_ExportedPts && label->IsExportedPt()) {
	    col = col_TURQUOISE;
	} else {
	    continue;
	}

	// Stations are sorted by blob type, so colour changes are infrequent.
	if (col != prev_col) {
	    SetColour(col);
	    prev_col = col;
	}
	DrawBlob(label->GetX(), label->GetY(), label->GetZ());
    }
    EndBlobs();
}

void GfxCore::DrawIndicators()
{
    // Draw depthbar.
    if (m_Depthbar) {
       if (m_ColourBy == COLOUR_BY_DEPTH && m_Parent->GetZExtent() != 0.0) {
	   DrawList2D(LIST_DEPTHBAR, m_XSize, m_YSize, 0);
       } else if (m_ColourBy == COLOUR_BY_DATE && m_Parent->GetDateExtent()) {
	   DrawList2D(LIST_DATEBAR, m_XSize, m_YSize, 0);
       }
    }

    // Draw compass or elevation/heading indicators.
    if (m_Compass || m_Clino) {
	if (!m_Parent->IsExtendedElevation()) Draw2dIndicators();
    }

    // Draw scalebar.
    if (m_Scalebar) {
	DrawScalebar();
    }
}

void GfxCore::PlaceVertexWithColour(const Vector3 & v, Double factor)
{
    SetColour(GetSurfacePen(), factor); // FIXME : assumes surface pen is white!
    PlaceVertex(v);
}

void GfxCore::PlaceVertexWithDepthColour(const Vector3 &v, Double factor)
{
    // Set the drawing colour based on the altitude.
    Double z_ext = m_Parent->GetZExtent();
    assert(z_ext > 0);

    Double z = v.GetZ();
    // points arising from tubes may be slightly outside the limits...
    if (z < -z_ext * 0.5) z = -z_ext * 0.5;
    if (z > z_ext * 0.5) z = z_ext * 0.5;

    Double z_offset = z + z_ext * 0.5;

    Double how_far = z_offset / z_ext;
    assert(how_far >= 0.0);
    assert(how_far <= 1.0);

    int band = int(floor(how_far * (GetNumDepthBands() - 1)));
    GLAPen pen1 = GetPen(band);
    if (band < GetNumDepthBands() - 1) {
	const GLAPen& pen2 = GetPen(band + 1);

	Double interval = z_ext / (GetNumDepthBands() - 1);
	Double into_band = z_offset / interval - band;

//	printf("%g z_offset=%g interval=%g band=%d\n", into_band,
//	       z_offset, interval, band);
	// FIXME: why do we need to clamp here?  Is it because the walls can
	// extend further up/down than the centre-line?
	if (into_band < 0.0) into_band = 0.0;
	if (into_band > 1.0) into_band = 1.0;
	assert(into_band >= 0.0);
	assert(into_band <= 1.0);

	pen1.Interpolate(pen2, into_band);
    }
    SetColour(pen1, factor);

    PlaceVertex(v);
}

void GfxCore::SplitLineAcrossBands(int band, int band2,
				   const Vector3 &p, const Vector3 &q,
				   Double factor)
{
    const int step = (band < band2) ? 1 : -1;
    for (int i = band; i != band2; i += step) {
	const Double z = GetDepthBoundaryBetweenBands(i, i + step);

	// Find the intersection point of the line p -> q
	// with the plane parallel to the xy-plane with z-axis intersection z.
	assert(q.GetZ() - p.GetZ() != 0.0);

	const Double t = (z - p.GetZ()) / (q.GetZ() - p.GetZ());
//	assert(0.0 <= t && t <= 1.0);		FIXME: rounding problems!

	const Double x = p.GetX() + t * (q.GetX() - p.GetX());
	const Double y = p.GetY() + t * (q.GetY() - p.GetY());

	PlaceVertexWithDepthColour(Vector3(x, y, z), factor);
    }
}

int GfxCore::GetDepthColour(Double z) const
{
    // Return the (0-based) depth colour band index for a z-coordinate.
    Double z_ext = m_Parent->GetZExtent();
    assert(z_ext > 0);
    z += z_ext / 2;
    return int(z / z_ext * (GetNumDepthBands() - 1));
}

Double GfxCore::GetDepthBoundaryBetweenBands(int a, int b) const
{
    // Return the z-coordinate of the depth colour boundary between
    // two adjacent depth colour bands (specified by 0-based indices).

    assert((a == b - 1) || (a == b + 1));
    if (GetNumDepthBands() == 1) return 0;

    int band = (a > b) ? a : b; // boundary N lies on the bottom of band N.
    Double z_ext = m_Parent->GetZExtent();
    return (z_ext * band / (GetNumDepthBands() - 1)) - z_ext / 2;
}

void GfxCore::AddPolyline(const vector<PointInfo> & centreline)
{
    BeginPolyline();
    SetColour(GetSurfacePen());
    vector<PointInfo>::const_iterator i = centreline.begin();
    PlaceVertex(*i);
    ++i;
    while (i != centreline.end()) {
	PlaceVertex(*i);
	++i;
    }
    EndPolyline();
}

void GfxCore::AddPolylineShadow(const vector<PointInfo> & centreline)
{
    BeginPolyline();
    vector<PointInfo>::const_iterator i = centreline.begin();
    PlaceVertex(i->GetX(), i->GetY(), -0.5 * m_Parent->GetZExtent());
    ++i;
    while (i != centreline.end()) {
	PlaceVertex(i->GetX(), i->GetY(), -0.5 * m_Parent->GetZExtent());
	++i;
    }
    EndPolyline();
}

void GfxCore::AddPolylineDepth(const vector<PointInfo> & centreline)
{
    BeginPolyline();
    vector<PointInfo>::const_iterator i, prev_i;
    i = centreline.begin();
    int band0 = GetDepthColour(i->GetZ());
    PlaceVertexWithDepthColour(*i);
    prev_i = i;
    ++i;
    while (i != centreline.end()) {
	int band = GetDepthColour(i->GetZ());
	if (band != band0) {
	    SplitLineAcrossBands(band0, band, *prev_i, *i);
	    band0 = band;
	}
	PlaceVertexWithDepthColour(*i);
	prev_i = i;
	++i;
    }
    EndPolyline();
}

void GfxCore::AddQuadrilateral(const Vector3 &a, const Vector3 &b,
			       const Vector3 &c, const Vector3 &d)
{
    Vector3 normal = (a - c) * (d - b);
    normal.normalise();
    Double factor = dot(normal, light) * .3 + .7;
    int w = int(ceil(((b - a).magnitude() + (d - c).magnitude()) / 2));
    int h = int(ceil(((b - c).magnitude() + (d - a).magnitude()) / 2));
    // FIXME: should plot triangles instead to avoid rendering glitches.
    BeginQuadrilaterals();
    // FIXME: these glTexCoord2i calls should be in gla-gl.cc
    glTexCoord2i(0, 0);
    PlaceVertexWithColour(a, factor);
    glTexCoord2i(w, 0);
    PlaceVertexWithColour(b, factor);
    glTexCoord2i(w, h);
    PlaceVertexWithColour(c, factor);
    glTexCoord2i(0, h);
    PlaceVertexWithColour(d, factor);
    EndQuadrilaterals();
}

void GfxCore::AddQuadrilateralDepth(const Vector3 &a, const Vector3 &b,
				    const Vector3 &c, const Vector3 &d)
{
    Vector3 normal = (a - c) * (d - b);
    normal.normalise();
    Double factor = dot(normal, light) * .3 + .7;
    int a_band, b_band, c_band, d_band;
    a_band = GetDepthColour(a.GetZ());
    a_band = min(max(a_band, 0), GetNumDepthBands());
    b_band = GetDepthColour(b.GetZ());
    b_band = min(max(b_band, 0), GetNumDepthBands());
    c_band = GetDepthColour(c.GetZ());
    c_band = min(max(c_band, 0), GetNumDepthBands());
    d_band = GetDepthColour(d.GetZ());
    d_band = min(max(d_band, 0), GetNumDepthBands());
    // All this splitting is incorrect - we need to make a separate polygon
    // for each depth band...
    int w = int(ceil(((b - a).magnitude() + (d - c).magnitude()) / 2));
    int h = int(ceil(((b - c).magnitude() + (d - a).magnitude()) / 2));
    // FIXME: should plot triangles instead to avoid rendering glitches.
    BeginPolygon();
////    PlaceNormal(normal);
    // FIXME: these glTexCoord2i calls should be in gla-gl.cc
    glTexCoord2i(0, 0);
    PlaceVertexWithDepthColour(a, factor);
    if (a_band != b_band) {
	SplitLineAcrossBands(a_band, b_band, a, b, factor);
    }
    glTexCoord2i(w, 0);
    PlaceVertexWithDepthColour(b, factor);
    if (b_band != c_band) {
	SplitLineAcrossBands(b_band, c_band, b, c, factor);
    }
    glTexCoord2i(w, h);
    PlaceVertexWithDepthColour(c, factor);
    if (c_band != d_band) {
	SplitLineAcrossBands(c_band, d_band, c, d, factor);
    }
    glTexCoord2i(0, h);
    PlaceVertexWithDepthColour(d, factor);
    if (d_band != a_band) {
	SplitLineAcrossBands(d_band, a_band, d, a, factor);
    }
    EndPolygon();
}

void GfxCore::SetColourFromDate(time_t date, Double factor)
{
    // Set the drawing colour based on a date.

    if (date == 0) {
	SetColour(GetSurfacePen(), factor);
	return;
    }

    time_t date_ext = m_Parent->GetDateExtent();
    assert(date_ext > 0);
    time_t date_offset = date - m_Parent->GetDateMin();

    Double how_far = (Double)date_offset / date_ext;
    assert(how_far >= 0.0);
    assert(how_far <= 1.0);

    int band = int(floor(how_far * (GetNumDepthBands() - 1)));
    GLAPen pen1 = GetPen(band);
    if (band < GetNumDepthBands() - 1) {
	const GLAPen& pen2 = GetPen(band + 1);

	Double interval = date_ext / (GetNumDepthBands() - 1);
	Double into_band = date_offset / interval - band;

//	printf("%g z_offset=%g interval=%g band=%d\n", into_band,
//	       z_offset, interval, band);
	// FIXME: why do we need to clamp here?  Is it because the walls can
	// extend further up/down than the centre-line?
	if (into_band < 0.0) into_band = 0.0;
	if (into_band > 1.0) into_band = 1.0;
	assert(into_band >= 0.0);
	assert(into_band <= 1.0);

	pen1.Interpolate(pen2, into_band);
    }
    SetColour(pen1, factor);
}

void GfxCore::AddPolylineDate(const vector<PointInfo> & centreline)
{
    BeginPolyline();
    vector<PointInfo>::const_iterator i, prev_i;
    i = centreline.begin();
    time_t date = i->GetDate();
    SetColourFromDate(date, 1.0);
    PlaceVertex(*i);
    prev_i = i;
    while (++i != centreline.end()) {
	time_t newdate = i->GetDate();
	if (newdate != date) {
	    EndPolyline();
	    BeginPolyline();
	    date = newdate;
	    SetColourFromDate(date, 1.0);
	    PlaceVertex(*prev_i);
	}
	PlaceVertex(*i);
	prev_i = i;
    }
    EndPolyline();
}

static time_t static_date_hack; // FIXME

void GfxCore::AddQuadrilateralDate(const Vector3 &a, const Vector3 &b,
				   const Vector3 &c, const Vector3 &d)
{
    Vector3 normal = (a - c) * (d - b);
    normal.normalise();
    Double factor = dot(normal, light) * .3 + .7;
    int w = int(ceil(((b - a).magnitude() + (d - c).magnitude()) / 2));
    int h = int(ceil(((b - c).magnitude() + (d - a).magnitude()) / 2));
    // FIXME: should plot triangles instead to avoid rendering glitches.
    BeginPolygon();
////    PlaceNormal(normal);
    SetColourFromDate(static_date_hack, factor);
    // FIXME: these glTexCoord2i calls should be in gla-gl.cc
    glTexCoord2i(0, 0);
    PlaceVertex(a);
    glTexCoord2i(w, 0);
    PlaceVertex(b);
    glTexCoord2i(w, h);
    PlaceVertex(c);
    glTexCoord2i(0, h);
    PlaceVertex(d);
    EndPolygon();
}

void
GfxCore::SkinPassage(const vector<XSect> & centreline)
{
    assert(centreline.size() > 1);
    Vector3 U[4];
    XSect prev_pt_v;
    Vector3 last_right(1.0, 0.0, 0.0);

    vector<XSect>::const_iterator i = centreline.begin();
    vector<XSect>::size_type segment = 0;
    while (i != centreline.end()) {
	// get the coordinates of this vertex
	const XSect & pt_v = *i++;

	double z_pitch_adjust = 0.0;
	bool cover_end = false;

	Vector3 right, up;

	const Vector3 up_v(0.0, 0.0, 1.0);

	if (segment == 0) {
	    assert(i != centreline.end());
	    // first segment

	    // get the coordinates of the next vertex
	    const XSect & next_pt_v = *i;

	    // calculate vector from this pt to the next one
	    Vector3 leg_v = next_pt_v - pt_v;

	    // obtain a vector in the LRUD plane
	    right = leg_v * up_v;
	    if (right.magnitude() == 0) {
		right = last_right;
		// Obtain a second vector in the LRUD plane,
		// perpendicular to the first.
		//up = right * leg_v;
		up = up_v;
	    } else {
		last_right = right;
		up = up_v;
	    }

	    cover_end = true;
	    static_date_hack = next_pt_v.GetDate();
	} else if (segment + 1 == centreline.size()) {
	    // last segment

	    // Calculate vector from the previous pt to this one.
	    Vector3 leg_v = pt_v - prev_pt_v;

	    // Obtain a horizontal vector in the LRUD plane.
	    right = leg_v * up_v;
	    if (right.magnitude() == 0) {
		right = Vector3(last_right.GetX(), last_right.GetY(), 0.0);
		// Obtain a second vector in the LRUD plane,
		// perpendicular to the first.
		//up = right * leg_v;
		up = up_v;
	    } else {
		last_right = right;
		up = up_v;
	    }

	    cover_end = true;
	    static_date_hack = pt_v.GetDate();
	} else {
	    assert(i != centreline.end());
	    // Intermediate segment.

	    // Get the coordinates of the next vertex.
	    const XSect & next_pt_v = *i;

	    // Calculate vectors from this vertex to the
	    // next vertex, and from the previous vertex to
	    // this one.
	    Vector3 leg1_v = pt_v - prev_pt_v;
	    Vector3 leg2_v = next_pt_v - pt_v;

	    // Obtain horizontal vectors perpendicular to
	    // both legs, then normalise and average to get
	    // a horizontal bisector.
	    Vector3 r1 = leg1_v * up_v;
	    Vector3 r2 = leg2_v * up_v;
	    r1.normalise();
	    r2.normalise();
	    right = r1 + r2;
	    if (right.magnitude() == 0) {
		// This is the "mid-pitch" case...
		right = last_right;
	    }
	    if (r1.magnitude() == 0) {
		Vector3 n = leg1_v;
		n.normalise();
		z_pitch_adjust = n.GetZ();
		//up = Vector3(0, 0, leg1_v.GetZ());
		//up = right * up;
		up = up_v;

		// Rotate pitch section to minimise the
		// "tortional stress" - FIXME: use
		// triangles instead of rectangles?
		int shift = 0;
		Double maxdotp = 0;

		// Scale to unit vectors in the LRUD plane.
		right.normalise();
		up.normalise();
		Vector3 vec = up - right;
		for (int orient = 0; orient <= 3; ++orient) {
		    Vector3 tmp = U[orient] - prev_pt_v;
		    tmp.normalise();
		    Double dotp = dot(vec, tmp);
		    if (dotp > maxdotp) {
			maxdotp = dotp;
			shift = orient;
		    }
		}
		if (shift) {
		    if (shift != 2) {
			Vector3 temp(U[0]);
			U[0] = U[shift];
			U[shift] = U[2];
			U[2] = U[shift ^ 2];
			U[shift ^ 2] = temp;
		    } else {
			swap(U[0], U[2]);
			swap(U[1], U[3]);
		    }
		}
#if 0
		// Check that the above code actually permuted
		// the vertices correctly.
		shift = 0;
		maxdotp = 0;
		for (int j = 0; j <= 3; ++j) {
		    Vector3 tmp = U[j] - prev_pt_v;
		    tmp.normalise();
		    Double dotp = dot(vec, tmp);
		    if (dotp > maxdotp) {
			maxdotp = dotp + 1e-6; // Add small tolerance to stop 45 degree offset cases being flagged...
			shift = j;
		    }
		}
		if (shift) {
		    printf("New shift = %d!\n", shift);
		    shift = 0;
		    maxdotp = 0;
		    for (int j = 0; j <= 3; ++j) {
			Vector3 tmp = U[j] - prev_pt_v;
			tmp.normalise();
			Double dotp = dot(vec, tmp);
			printf("    %d : %.8f\n", j, dotp);
		    }
		}
#endif
	    } else if (r2.magnitude() == 0) {
		Vector3 n = leg2_v;
		n.normalise();
		z_pitch_adjust = n.GetZ();
		//up = Vector3(0, 0, leg2_v.GetZ());
		//up = right * up;
		up = up_v;
	    } else {
		up = up_v;
	    }
	    last_right = right;
	    static_date_hack = pt_v.GetDate();
	}

	// Scale to unit vectors in the LRUD plane.
	right.normalise();
	up.normalise();

	if (z_pitch_adjust != 0) up += Vector3(0, 0, fabs(z_pitch_adjust));

	Double l = fabs(pt_v.GetL());
	Double r = fabs(pt_v.GetR());
	Double u = fabs(pt_v.GetU());
	Double d = fabs(pt_v.GetD());

	// Produce coordinates of the corners of the LRUD "plane".
	Vector3 v[4];
	v[0] = pt_v - right * l + up * u;
	v[1] = pt_v + right * r + up * u;
	v[2] = pt_v + right * r - up * d;
	v[3] = pt_v - right * l - up * d;

	if (segment > 0) {
	    (this->*AddQuad)(v[0], v[1], U[1], U[0]);
	    (this->*AddQuad)(v[2], v[3], U[3], U[2]);
	    (this->*AddQuad)(v[1], v[2], U[2], U[1]);
	    (this->*AddQuad)(v[3], v[0], U[0], U[3]);
	}

	if (cover_end) {
	    (this->*AddQuad)(v[3], v[2], v[1], v[0]);
	}

	prev_pt_v = pt_v;
	U[0] = v[0];
	U[1] = v[1];
	U[2] = v[2];
	U[3] = v[3];

	++segment;
    }
}

void GfxCore::FullScreenMode()
{
    m_Parent->ViewFullScreen();
}

bool GfxCore::IsFullScreen() const
{
    return m_Parent->IsFullScreen();
}

void
GfxCore::MoveViewer(double forward, double up, double right)
{
    double cT = cos(rad(m_TiltAngle));
    double sT = sin(rad(m_TiltAngle));
    double cP = cos(rad(m_PanAngle));
    double sP = sin(rad(m_PanAngle));
    Vector3 v_forward(cT * sP, cT * cP, -sT);
    Vector3 v_up(-sT * sP, -sT * cP, -cT);
    Vector3 v_right(-cP, sP, 0);
    assert(fabs(dot(v_forward, v_up)) < 1e-6);
    assert(fabs(dot(v_forward, v_right)) < 1e-6);
    assert(fabs(dot(v_right, v_up)) < 1e-6);
    Vector3 move = v_forward * forward + v_up * up + v_right * right;
    AddTranslation(-move);
    // Show current position.
    m_Parent->SetCoords(m_Parent->GetOffset() - GetTranslation());
    ForceRefresh();
}

PresentationMark GfxCore::GetView() const
{
    return PresentationMark(GetTranslation() + m_Parent->GetOffset(),
			    m_PanAngle, m_TiltAngle, m_Scale);
}

void GfxCore::SetView(const PresentationMark & p)
{
    m_SwitchingTo = 0;
    SetTranslation(p - m_Parent->GetOffset());
    m_PanAngle = p.angle;
    m_TiltAngle = p.tilt_angle;
    UpdateQuaternion();
    SetScale(p.scale);
    ForceRefresh();
}

void GfxCore::PlayPres(double speed, bool change_speed) {
    if (!change_speed || presentation_mode == 0) {
	if (speed == 0.0) {
	    presentation_mode = 0;
	    return;
	}
	presentation_mode = PLAYING;
	next_mark = m_Parent->GetPresMark(MARK_FIRST);
	SetView(next_mark);
	next_mark_time = 0; // There already!
	this_mark_total = 0;
	pres_reverse = (speed < 0);
    }

    if (change_speed) pres_speed = speed;

    if (speed != 0.0) {
	bool new_pres_reverse = (speed < 0);
	if (new_pres_reverse != pres_reverse) {
	    pres_reverse = new_pres_reverse;
	    if (pres_reverse) {
		next_mark = m_Parent->GetPresMark(MARK_PREV);
	    } else {
		next_mark = m_Parent->GetPresMark(MARK_NEXT);
	    }
	    swap(this_mark_total, next_mark_time);
	}
    }
}

void GfxCore::SetColourBy(int colour_by) {
    m_ColourBy = colour_by;
    switch (colour_by) {
	case COLOUR_BY_DEPTH:
	    AddQuad = &GfxCore::AddQuadrilateralDepth;
	    AddPoly = &GfxCore::AddPolylineDepth;
	    break;
	case COLOUR_BY_DATE:
	    AddQuad = &GfxCore::AddQuadrilateralDate;
	    AddPoly = &GfxCore::AddPolylineDate;
	    break;
	default: // case COLOUR_BY_NONE:
	    AddQuad = &GfxCore::AddQuadrilateral;
	    AddPoly = &GfxCore::AddPolyline;
	    break;
    }

    InvalidateList(LIST_UNDERGROUND_LEGS);
    InvalidateList(LIST_TUBES);

    ForceRefresh();
}

bool GfxCore::ExportMovie(const wxString & fnm)
{
    int width;
    int height;
    GetSize(&width, &height);
    // Round up to next multiple of 2 (required by ffmpeg).
    width += (width & 1);
    height += (height & 1);

    mpeg = new MovieMaker();

    if (!mpeg->Open(fnm.c_str(), width, height)) {
	// FIXME : sort out reporting actual errors from ffmpeg library
	wxGetApp().ReportError(wxString::Format(msg(/*Error writing to file `%s'*/110), fnm.c_str()));
	delete mpeg;
	mpeg = 0;
	return false;
    }

    PlayPres(1, false);
    return true;
}

void
GfxCore::OnPrint(const wxString &filename, const wxString &title,
		 const wxString &datestamp)
{
    svxPrintDlg * p;
    p = new svxPrintDlg(m_Parent, filename, title, datestamp,
			m_PanAngle, m_TiltAngle,
			m_Names, m_Crosses, m_Legs, m_Surface);
    p->Show(TRUE);
}

bool
GfxCore::OnExport(const wxString &filename, const wxString &title)
{
    return Export(filename, title, m_Parent,
	   m_PanAngle, m_TiltAngle, m_Names, m_Crosses, m_Legs, m_Surface);
}

static wxCursor
make_cursor(const unsigned char * bits, const unsigned char * mask,
	    int hotx, int hoty)
{
#ifdef __WXMSW__
    wxBitmap cursor_bitmap(reinterpret_cast<const char *>(bits), 32, 32);
    wxBitmap mask_bitmap(reinterpret_cast<const char *>(mask), 32, 32);
    cursor_bitmap.SetMask(new wxMask(mask_bitmap));
    wxImage cursor_image = cursor_bitmap.ConvertToImage();
    cursor_image.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, hotx);
    cursor_image.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, hoty);
    return wxCursor(cursor_image);
#else
    return wxCursor((const char *)bits, 32, 32, hotx, hoty,
		    (const char *)mask, wxWHITE, wxBLACK);
#endif
}

const
#include "hand.xbm"
const
#include "handmask.xbm"

const
#include "brotate.xbm"
const
#include "brotatemask.xbm"

const
#include "vrotate.xbm"
const
#include "vrotatemask.xbm"

const
#include "rotate.xbm"
const
#include "rotatemask.xbm"

const
#include "rotatezoom.xbm"
const
#include "rotatezoommask.xbm"

void
GfxCore::SetCursor(GfxCore::cursor new_cursor)
{
    // Check if we're already showing that cursor.
    if (current_cursor == new_cursor) return;

    current_cursor = new_cursor;
    switch (current_cursor) {
	case GfxCore::CURSOR_DEFAULT:
	    GLACanvas::SetCursor(wxNullCursor);
	    break;
	case GfxCore::CURSOR_POINTING_HAND:
	    GLACanvas::SetCursor(wxCursor(wxCURSOR_HAND));
	    break;
	case GfxCore::CURSOR_DRAGGING_HAND:
	    GLACanvas::SetCursor(make_cursor(hand_bits, handmask_bits, 12, 18));
	    break;
	case GfxCore::CURSOR_HORIZONTAL_RESIZE:
	    GLACanvas::SetCursor(wxCursor(wxCURSOR_SIZEWE));
	    break;
	case GfxCore::CURSOR_ROTATE_HORIZONTALLY:
	    GLACanvas::SetCursor(make_cursor(rotate_bits, rotatemask_bits, 15, 15));
	    break;
	case GfxCore::CURSOR_ROTATE_VERTICALLY:
	    GLACanvas::SetCursor(make_cursor(vrotate_bits, vrotatemask_bits, 15, 15));
	    break;
	case GfxCore::CURSOR_ROTATE_EITHER_WAY:
	    GLACanvas::SetCursor(make_cursor(brotate_bits, brotatemask_bits, 15, 15));
	    break;
	case GfxCore::CURSOR_ZOOM:
	    GLACanvas::SetCursor(wxCursor(wxCURSOR_MAGNIFIER));
	    break;
	case GfxCore::CURSOR_ZOOM_ROTATE:
	    GLACanvas::SetCursor(make_cursor(rotatezoom_bits, rotatezoommask_bits, 15, 15));
	    break;
    }
}
