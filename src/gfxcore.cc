//
//  gfxcore.cc
//
//  Core drawing code for Aven.
//
//  Copyright (C) 2000-2003,2005,2006 Mark R. Shinwell
//  Copyright (C) 2001-2003,2004,2005,2006,2007,2010,2011,2012,2014,2015,2016,2017 Olly Betts
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
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <float.h>

#include "aven.h"
#include "date.h"
#include "filename.h"
#include "gfxcore.h"
#include "mainfrm.h"
#include "message.h"
#include "useful.h"
#include "printing.h"
#include "guicontrol.h"
#include "moviemaker.h"

#include <wx/confbase.h>
#include <wx/wfstream.h>
#include <wx/image.h>
#include <wx/zipstrm.h>

#include <proj_api.h>

const unsigned long DEFAULT_HGT_DIM = 3601;
const unsigned long DEFAULT_HGT_SIZE = sqrd(DEFAULT_HGT_DIM) * 2;

// Values for m_SwitchingTo
#define PLAN 1
#define ELEVATION 2
#define NORTH 3
#define EAST 4
#define SOUTH 5
#define WEST 6

// Any error value higher than this is clamped to this.
#define MAX_ERROR 12.0

// Any length greater than pow(10, LOG_LEN_MAX) will be clamped to this.
const Double LOG_LEN_MAX = 1.5;

// How many bins per letter height to use when working out non-overlapping
// labels.
const unsigned int QUANTISE_FACTOR = 2;

#include "avenpal.h"

static const int INDICATOR_BOX_SIZE = 60;
static const int INDICATOR_GAP = 2;
static const int INDICATOR_MARGIN = 5;
static const int INDICATOR_OFFSET_X = 15;
static const int INDICATOR_OFFSET_Y = 15;
static const int INDICATOR_RADIUS = INDICATOR_BOX_SIZE / 2 - INDICATOR_MARGIN;
static const int KEY_OFFSET_X = 10;
static const int KEY_OFFSET_Y = 10;
static const int KEY_EXTRA_LEFT_MARGIN = 2;
static const int KEY_BLOCK_WIDTH = 20;
static const int KEY_BLOCK_HEIGHT = 16;
static const int TICK_LENGTH = 4;
static const int SCALE_BAR_OFFSET_X = 15;
static const int SCALE_BAR_OFFSET_Y = 12;
static const int SCALE_BAR_HEIGHT = 12;

static const gla_colour TEXT_COLOUR = col_GREEN;
static const gla_colour HERE_COLOUR = col_WHITE;
static const gla_colour NAME_COLOUR = col_GREEN;
static const gla_colour SEL_COLOUR = col_WHITE;

// Number of entries across and down the hit-test grid:
#define HITTEST_SIZE 20

// How close the pointer needs to be to a station to be considered:
#define MEASURE_THRESHOLD 7

// vector for lighting angle
static const Vector3 light(.577, .577, .577);

BEGIN_EVENT_TABLE(GfxCore, GLACanvas)
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
    GLACanvas(parent_win, 100),
    m_Scale(0.0),
    initial_scale(1.0),
    m_ScaleBarWidth(0),
    m_Control(control),
    m_LabelGrid(NULL),
    m_Parent(parent),
    m_DoneFirstShow(false),
    m_TiltAngle(0.0),
    m_PanAngle(0.0),
    m_Rotating(false),
    m_RotationStep(0.0),
    m_SwitchingTo(0),
    m_Crosses(false),
    m_Legs(true),
    m_Splays(SHOW_FADED),
    m_Dupes(SHOW_DASHED),
    m_Names(false),
    m_Scalebar(true),
    m_ColourKey(true),
    m_OverlappingNames(false),
    m_Compass(true),
    m_Clino(true),
    m_Tubes(false),
    m_ColourBy(COLOUR_BY_DEPTH),
    m_HaveData(false),
    m_HaveTerrain(true),
    m_MouseOutsideCompass(false),
    m_MouseOutsideElev(false),
    m_Surface(false),
    m_Entrances(false),
    m_FixedPts(false),
    m_ExportedPts(false),
    m_Grid(false),
    m_BoundingBox(false),
    m_Terrain(false),
    m_Degrees(false),
    m_Metric(false),
    m_Percent(false),
    m_HitTestDebug(false),
    m_RenderStats(false),
    m_PointGrid(NULL),
    m_HitTestGridValid(false),
    m_here(NULL),
    m_there(NULL),
    presentation_mode(0),
    pres_reverse(false),
    pres_speed(0.0),
    movie(NULL),
    current_cursor(GfxCore::CURSOR_DEFAULT),
    sqrd_measure_threshold(sqrd(MEASURE_THRESHOLD)),
    dem(NULL),
    last_time(0),
    n_tris(0)
{
    AddQuad = &GfxCore::AddQuadrilateralDepth;
    AddPoly = &GfxCore::AddPolylineDepth;
    wxConfigBase::Get()->Read(wxT("metric"), &m_Metric, true);
    wxConfigBase::Get()->Read(wxT("degrees"), &m_Degrees, true);
    wxConfigBase::Get()->Read(wxT("percent"), &m_Percent, false);

    for (int pen = 0; pen < NUM_COLOUR_BANDS + 1; ++pen) {
	m_Pens[pen].SetColour(REDS[pen] / 255.0,
			      GREENS[pen] / 255.0,
			      BLUES[pen] / 255.0);
    }

    timer.Start();
}

GfxCore::~GfxCore()
{
    TryToFreeArrays();

    delete[] m_PointGrid;
}

void GfxCore::TryToFreeArrays()
{
    // Free up any memory allocated for arrays.
    delete[] m_LabelGrid;
    m_LabelGrid = NULL;
}

//
//  Initialisation methods
//

void GfxCore::Initialise(bool same_file)
{
    // Initialise the view from the parent holding the survey data.

    TryToFreeArrays();

    m_DoneFirstShow = false;

    m_HitTestGridValid = false;
    m_here = NULL;
    m_there = NULL;

    m_MouseOutsideCompass = m_MouseOutsideElev = false;

    if (!same_file) {
	// Apply default parameters unless reloading the same file.
	DefaultParameters();
    }

    m_HaveData = true;

    // Clear any cached OpenGL lists which depend on the data.
    InvalidateList(LIST_SCALE_BAR);
    InvalidateList(LIST_DEPTH_KEY);
    InvalidateList(LIST_DATE_KEY);
    InvalidateList(LIST_ERROR_KEY);
    InvalidateList(LIST_GRADIENT_KEY);
    InvalidateList(LIST_LENGTH_KEY);
    InvalidateList(LIST_UNDERGROUND_LEGS);
    InvalidateList(LIST_TUBES);
    InvalidateList(LIST_SURFACE_LEGS);
    InvalidateList(LIST_BLOBS);
    InvalidateList(LIST_CROSSES);
    InvalidateList(LIST_GRID);
    InvalidateList(LIST_SHADOW);
    InvalidateList(LIST_TERRAIN);

    // Set diameter of the viewing volume.
    double cave_diameter = sqrt(sqrd(m_Parent->GetXExtent()) +
				sqrd(m_Parent->GetYExtent()) +
				sqrd(m_Parent->GetZExtent()));

    // Allow for terrain.
    double diameter = max(1000.0 * 2, cave_diameter * 2);

    if (!same_file) {
	SetVolumeDiameter(diameter);

	// Set initial scale based on the size of the cave.
	initial_scale = diameter / cave_diameter;
	SetScale(initial_scale);
    } else {
	// Adjust the position when restricting the view to a subsurvey (or
	// expanding the view to show the whole survey).
	AddTranslation(m_Parent->GetOffset() - offsets);

	// Try to keep the same scale, allowing for the
	// cave having grown (or shrunk).
	double rescale = GetVolumeDiameter() / diameter;
	SetVolumeDiameter(diameter);
	SetScale(GetScale() / rescale); // ?
	initial_scale = initial_scale * rescale;
    }

    offsets = m_Parent->GetOffset();

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

    m_DoneFirstShow = true;
}

//
//  Recalculating methods
//

void GfxCore::SetScale(Double scale)
{
    if (scale < 0.05) {
	scale = 0.05;
    } else if (scale > GetVolumeDiameter()) {
	scale = GetVolumeDiameter();
    }

    m_Scale = scale;
    m_HitTestGridValid = false;
    if (m_here && m_here == &temp_here) SetHere();

    GLACanvas::SetScale(scale);
}

bool GfxCore::HasUndergroundLegs() const
{
    return m_Parent->HasUndergroundLegs();
}

bool GfxCore::HasSplays() const
{
    return m_Parent->HasSplays();
}

bool GfxCore::HasDupes() const
{
    return m_Parent->HasDupes();
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

void GfxCore::OnLeaveWindow(wxMouseEvent&) {
    SetHere();
    ClearCoords();
}

void GfxCore::OnIdle(wxIdleEvent& event)
{
    // Handle an idle event.
    if (Animating()) {
	Animate();
	// If still animating, we want more idle events.
	if (Animating())
	    event.RequestMore();
    } else {
	// If we're idle, don't show a bogus FPS next time we render.
	last_time = 0;
    }
}

void GfxCore::OnPaint(wxPaintEvent&)
{
    // Redraw the window.

    // Get a graphics context.
    wxPaintDC dc(this);

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

	if (m_BoundingBox) {
	    DrawShadowedBoundingBox();
	}
	if (m_Grid) {
	    // Draw the grid.
	    DrawList(LIST_GRID);
	}

	DrawList(LIST_BLOBS);

	if (m_Crosses) {
	    DrawList(LIST_CROSSES);
	}

	if (m_Terrain) {
	    // Disable texturing while drawing terrain.
	    bool texturing = GetTextured();
	    if (texturing) GLACanvas::ToggleTextured();

	    // This is needed if blobs and/or crosses are drawn using lines -
	    // otherwise the terrain doesn't appear when they are enabled.
	    SetDataTransform();

	    // We don't want to be able to see the terrain through itself, so
	    // do a "Z-prepass" - plot the terrain once only updating the
	    // Z-buffer, then again with Z-clipping only plotting where the
	    // depth matches the value in the Z-buffer.
	    DrawListZPrepass(LIST_TERRAIN);

	    if (texturing) GLACanvas::ToggleTextured();
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

	if (m_HitTestDebug) {
	    // Show the hit test grid bucket sizes...
	    SetColour(m_HitTestGridValid ? col_LIGHT_GREY : col_DARK_GREY);
	    if (m_PointGrid) {
		for (int i = 0; i != HITTEST_SIZE; ++i) {
		    int x = (GetXSize() + 1) * i / HITTEST_SIZE + 2;
		    for (int j = 0; j != HITTEST_SIZE; ++j) {
			int square = i + j * HITTEST_SIZE;
			unsigned long bucket_size = m_PointGrid[square].size();
			if (bucket_size) {
			    int y = (GetYSize() + 1) * (HITTEST_SIZE - 1 - j) / HITTEST_SIZE;
			    DrawIndicatorText(x, y, wxString::Format(wxT("%lu"), bucket_size));
			}
		    }
		}
	    }

	    EnableDashedLines();
	    BeginLines();
	    for (int i = 0; i != HITTEST_SIZE; ++i) {
		int x = (GetXSize() + 1) * i / HITTEST_SIZE;
		PlaceIndicatorVertex(x, 0);
		PlaceIndicatorVertex(x, GetYSize());
	    }
	    for (int j = 0; j != HITTEST_SIZE; ++j) {
		int y = (GetYSize() + 1) * (HITTEST_SIZE - 1 - j) / HITTEST_SIZE;
		PlaceIndicatorVertex(0, y);
		PlaceIndicatorVertex(GetXSize(), y);
	    }
	    EndLines();
	    DisableDashedLines();
	}

	long now = timer.Time();
	if (m_RenderStats) {
	    // Show stats about rendering.
	    SetColour(col_TURQUOISE);
	    int y = GetYSize() - GetFontSize();
	    if (last_time != 0.0) {
		// timer.Time() measure in milliseconds.
		double fps = 1000.0 / (now - last_time);
		DrawIndicatorText(1, y, wxString::Format(wxT("FPS:% 5.1f"), fps));
	    }
	    y -= GetFontSize();
	    DrawIndicatorText(1, y, wxString::Format(wxT("▲:%lu"), (unsigned long)n_tris));
	}
	last_time = now;

	// Draw indicators.
	//
	// There's no advantage in generating an OpenGL list for the
	// indicators since they change with almost every redraw (and
	// sometimes several times between redraws).  This way we avoid
	// the need to track when to update the indicator OpenGL list,
	// and also avoid indicator update bugs when we don't quite get this
	// right...
	DrawIndicators();

	if (zoombox.active()) {
	    SetColour(SEL_COLOUR);
	    EnableDashedLines();
	    BeginPolyline();
	    glaCoord Y = GetYSize();
	    PlaceIndicatorVertex(zoombox.x1, Y - zoombox.y1);
	    PlaceIndicatorVertex(zoombox.x1, Y - zoombox.y2);
	    PlaceIndicatorVertex(zoombox.x2, Y - zoombox.y2);
	    PlaceIndicatorVertex(zoombox.x2, Y - zoombox.y1);
	    PlaceIndicatorVertex(zoombox.x1, Y - zoombox.y1);
	    EndPolyline();
	    DisableDashedLines();
	} else if (MeasuringLineActive()) {
	    // Draw "here" and "there".
	    double hx, hy;
	    SetColour(HERE_COLOUR);
	    if (m_here) {
		double dummy;
		Transform(*m_here, &hx, &hy, &dummy);
		if (m_here != &temp_here) DrawRing(hx, hy);
	    }
	    if (m_there) {
		double tx, ty;
		double dummy;
		Transform(*m_there, &tx, &ty, &dummy);
		if (m_here) {
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

	FinishDrawing();
    } else {
	dc.SetBackground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWFRAME));
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

    PolygonOffset(true);
    SetColour(col_DARK_GREY);
    BeginQuadrilaterals();
    PlaceVertex(-v.GetX(), -v.GetY(), -v.GetZ());
    PlaceVertex(-v.GetX(), v.GetY(), -v.GetZ());
    PlaceVertex(v.GetX(), v.GetY(), -v.GetZ());
    PlaceVertex(v.GetX(), -v.GetY(), -v.GetZ());
    EndQuadrilaterals();
    PolygonOffset(false);

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
    int result = INDICATOR_OFFSET_X;
    if (m_Compass) {
	result += 6 + GetCompassWidth() + INDICATOR_GAP;
    }
    return result;
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
	DrawList2D(LIST_CLINO, elev_centre_x, centre_y, 90 - m_TiltAngle);
    }

    SetColour(TEXT_COLOUR);

    static int triple_zero_width = 0;
    static int height = 0;
    if (!triple_zero_width) {
	GetTextExtent(wxT("000"), &triple_zero_width, &height);
    }
    const int y_off = INDICATOR_OFFSET_Y + INDICATOR_BOX_SIZE + height / 2;

    if (m_Compass && !m_Parent->IsExtendedElevation()) {
	wxString str;
	int value;
	int brg_unit;
	if (m_Degrees) {
	    value = int(m_PanAngle);
	    /* TRANSLATORS: degree symbol - probably should be translated to
	     * itself. */
	    brg_unit = /*°*/344;
	} else {
	    value = int(m_PanAngle * 200.0 / 180.0);
	    /* TRANSLATORS: symbol for grad (400 grad = 360 degrees = full
	     * circle). */
	    brg_unit = /*ᵍ*/345;
	}
	str.Printf(wxT("%03d"), value);
	str += wmsg(brg_unit);
	DrawIndicatorText(comp_centre_x - triple_zero_width / 2, y_off, str);

	// TRANSLATORS: Used in aven above the compass indicator at the lower
	// right of the display, with a bearing below "Facing".  This indicates the
	// direction the viewer is "facing" in.
	//
	// Try to keep this translation short - ideally at most 10 characters -
	// as otherwise the compass and clino will be moved further apart to
	// make room. */
	str = wmsg(/*Facing*/203);
	int w;
	GetTextExtent(str, &w, NULL);
	DrawIndicatorText(comp_centre_x - w / 2, y_off + height, str);
    }

    if (m_Clino) {
	if (m_TiltAngle == -90.0) {
	    // TRANSLATORS: Label used for "clino" in Aven when the view is
	    // from directly above.
	    //
	    // Try to keep this translation short - ideally at most 10
	    // characters - as otherwise the compass and clino will be moved
	    // further apart to make room. */
	    wxString str = wmsg(/*Plan*/432);
	    static int width = 0;
	    if (!width) {
		GetTextExtent(str, &width, NULL);
	    }
	    int x = elev_centre_x - width / 2;
	    DrawIndicatorText(x, y_off + height / 2, str);
	} else if (m_TiltAngle == 90.0) {
	    // TRANSLATORS: Label used for "clino" in Aven when the view is
	    // from directly below.
	    //
	    // Try to keep this translation short - ideally at most 10
	    // characters - as otherwise the compass and clino will be moved
	    // further apart to make room. */
	    wxString str = wmsg(/*Kiwi Plan*/433);
	    static int width = 0;
	    if (!width) {
		GetTextExtent(str, &width, NULL);
	    }
	    int x = elev_centre_x - width / 2;
	    DrawIndicatorText(x, y_off + height / 2, str);
	} else {
	    int angle;
	    wxString str;
	    int width;
	    int unit;
	    if (m_Percent) {
		static int zero_width = 0;
		if (!zero_width) {
		    GetTextExtent(wxT("0"), &zero_width, NULL);
		}
		width = zero_width;
		if (m_TiltAngle > 89.99) {
		    angle = 1000000;
		} else if (m_TiltAngle < -89.99) {
		    angle = -1000000;
		} else {
		    angle = int(100 * tan(rad(m_TiltAngle)));
		}
		if (angle > 99999 || angle < -99999) {
		    str = angle > 0 ? wxT("+") : wxT("-");
		    /* TRANSLATORS: infinity symbol - used for the percentage gradient on
		     * vertical angles. */
		    str += wmsg(/*∞*/431);
		} else {
		    str = angle ? wxString::Format(wxT("%+03d"), angle) : wxT("0");
		}
		/* TRANSLATORS: symbol for percentage gradient (100% = 45
		 * degrees = 50 grad). */
		unit = /*%*/96;
	    } else if (m_Degrees) {
		static int zero_zero_width = 0;
		if (!zero_zero_width) {
		    GetTextExtent(wxT("00"), &zero_zero_width, NULL);
		}
		width = zero_zero_width;
		angle = int(m_TiltAngle);
		str = angle ? wxString::Format(wxT("%+03d"), angle) : wxT("00");
		unit = /*°*/344;
	    } else {
		width = triple_zero_width;
		angle = int(m_TiltAngle * 200.0 / 180.0);
		str = angle ? wxString::Format(wxT("%+04d"), angle) : wxT("000");
		unit = /*ᵍ*/345;
	    }

	    int sign_offset = 0;
	    if (unit == /*%*/96) {
		// Right align % since the width changes so much.
		GetTextExtent(str, &sign_offset, NULL);
		sign_offset -= width;
	    } else if (angle < 0) {
		// Adjust horizontal position so the left of the first digit is
		// always in the same place.
		static int minus_width = 0;
		if (!minus_width) {
		    GetTextExtent(wxT("-"), &minus_width, NULL);
		}
		sign_offset = minus_width;
	    } else if (angle > 0) {
		// Adjust horizontal position so the left of the first digit is
		// always in the same place.
		static int plus_width = 0;
		if (!plus_width) {
		    GetTextExtent(wxT("+"), &plus_width, NULL);
		}
		sign_offset = plus_width;
	    }

	    str += wmsg(unit);
	    DrawIndicatorText(elev_centre_x - sign_offset - width / 2, y_off, str);

	    // TRANSLATORS: Label used for "clino" in Aven when the view is
	    // neither from directly above nor from directly below.  It is
	    // also used in the dialog for editing a marked position in a
	    // presentation.
	    //
	    // Try to keep this translation short - ideally at most 10
	    // characters - as otherwise the compass and clino will be moved
	    // further apart to make room. */
	    str = wmsg(/*Elevation*/118);
	    static int elevation_width = 0;
	    if (!elevation_width) {
		GetTextExtent(str, &elevation_width, NULL);
	    }
	    int x = elev_centre_x - elevation_width / 2;
	    DrawIndicatorText(x, y_off + height, str);
	}
    }
}

void GfxCore::NattyDrawNames()
{
    // Draw station names, without overlapping.

    const unsigned int quantise(GetFontSize() / QUANTISE_FACTOR);
    const unsigned int quantised_x = GetXSize() / quantise;
    const unsigned int quantised_y = GetYSize() / quantise;
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

	double x, y, z;

	Transform(**label, &x, &y, &z);
	// Check if the label is behind us (in perspective view).
	if (z <= 0.0 || z >= 1.0) continue;

	// Apply a small shift so that translating the view doesn't make which
	// labels are displayed change as the resulting twinkling effect is
	// distracting.
	double tx, ty, tz;
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

	double x, y, z;
	Transform(**label, &x, &y, &z);

	// Check if the label is behind us (in perspective view).
	if (z <= 0) continue;

	x += 3;
	y -= GetFontSize() / 2;
	DrawIndicatorText((int)x, (int)y, (*label)->GetText());
    }
}

void GfxCore::DrawColourKey(int num_bands, const wxString & other, const wxString & units)
{
    int total_block_height =
	KEY_BLOCK_HEIGHT * (num_bands == 1 ? num_bands : num_bands - 1);
    if (!other.empty()) total_block_height += KEY_BLOCK_HEIGHT * 2;
    if (!units.empty()) total_block_height += KEY_BLOCK_HEIGHT;

    const int bottom = -total_block_height;

    int size = 0;
    if (!other.empty()) GetTextExtent(other, &size, NULL);
    int band;
    for (band = 0; band < num_bands; ++band) {
	int x;
	GetTextExtent(key_legends[band], &x, NULL);
	if (x > size) size = x;
    }

    int left = -KEY_BLOCK_WIDTH - size;

    key_lowerleft[m_ColourBy].x = left - KEY_EXTRA_LEFT_MARGIN;
    key_lowerleft[m_ColourBy].y = bottom;

    int y = bottom;
    if (!units.empty()) y += KEY_BLOCK_HEIGHT;

    if (!other.empty()) {
	DrawShadedRectangle(GetSurfacePen(), GetSurfacePen(), left, y,
		KEY_BLOCK_WIDTH, KEY_BLOCK_HEIGHT);
	SetColour(col_BLACK);
	BeginPolyline();
	PlaceIndicatorVertex(left, y);
	PlaceIndicatorVertex(left + KEY_BLOCK_WIDTH, y);
	PlaceIndicatorVertex(left + KEY_BLOCK_WIDTH, y + KEY_BLOCK_HEIGHT);
	PlaceIndicatorVertex(left, y + KEY_BLOCK_HEIGHT);
	PlaceIndicatorVertex(left, y);
	EndPolyline();
	y += KEY_BLOCK_HEIGHT * 2;
    }

    int start = y;
    if (num_bands == 1) {
	DrawShadedRectangle(GetPen(0), GetPen(0), left, y,
			    KEY_BLOCK_WIDTH, KEY_BLOCK_HEIGHT);
	y += KEY_BLOCK_HEIGHT;
    } else {
	for (band = 0; band < num_bands - 1; ++band) {
	    DrawShadedRectangle(GetPen(band), GetPen(band + 1), left, y,
				KEY_BLOCK_WIDTH, KEY_BLOCK_HEIGHT);
	    y += KEY_BLOCK_HEIGHT;
	}
    }

    SetColour(col_BLACK);
    BeginPolyline();
    PlaceIndicatorVertex(left, y);
    PlaceIndicatorVertex(left + KEY_BLOCK_WIDTH, y);
    PlaceIndicatorVertex(left + KEY_BLOCK_WIDTH, start);
    PlaceIndicatorVertex(left, start);
    PlaceIndicatorVertex(left, y);
    EndPolyline();

    SetColour(TEXT_COLOUR);

    y = bottom;
    if (!units.empty()) {
	GetTextExtent(units, &size, NULL);
	DrawIndicatorText(left + (KEY_BLOCK_WIDTH - size) / 2, y, units);
	y += KEY_BLOCK_HEIGHT;
    }
    y -= GetFontSize() / 2;
    left += KEY_BLOCK_WIDTH + 5;

    if (!other.empty()) {
	y += KEY_BLOCK_HEIGHT / 2;
	DrawIndicatorText(left, y, other);
	y += KEY_BLOCK_HEIGHT * 2 - KEY_BLOCK_HEIGHT / 2;
    }

    if (num_bands == 1) {
	y += KEY_BLOCK_HEIGHT / 2;
	DrawIndicatorText(left, y, key_legends[0]);
    } else {
	for (band = 0; band < num_bands; ++band) {
	    DrawIndicatorText(left, y, key_legends[band]);
	    y += KEY_BLOCK_HEIGHT;
	}
    }
}

void GfxCore::DrawDepthKey()
{
    Double z_ext = m_Parent->GetDepthExtent();
    int num_bands = 1;
    int sf = 0;
    if (z_ext > 0.0) {
	num_bands = GetNumColourBands();
	Double z_range = z_ext;
	if (!m_Metric) z_range /= METRES_PER_FOOT;
	sf = max(0, 1 - (int)floor(log10(z_range)));
    }

    Double z_min = m_Parent->GetDepthMin() + m_Parent->GetOffset().GetZ();
    for (int band = 0; band < num_bands; ++band) {
	Double z = z_min;
	if (band)
	    z += z_ext * band / (num_bands - 1);

	if (!m_Metric)
	    z /= METRES_PER_FOOT;

	key_legends[band].Printf(wxT("%.*f"), sf, z);
    }

    DrawColourKey(num_bands, wxString(), wmsg(m_Metric ? /*m*/424: /*ft*/428));
}

void GfxCore::DrawDateKey()
{
    int num_bands;
    if (!HasDateInformation()) {
	num_bands = 0;
    } else {
	int date_ext = m_Parent->GetDateExtent();
	if (date_ext == 0) {
	    num_bands = 1;
	} else {
	    num_bands = GetNumColourBands();
	}
	for (int band = 0; band < num_bands; ++band) {
	    int y, m, d;
	    int days = m_Parent->GetDateMin();
	    if (band)
		days += date_ext * band / (num_bands - 1);
	    ymd_from_days_since_1900(days, &y, &m, &d);
	    key_legends[band].Printf(wxT("%04d-%02d-%02d"), y, m, d);
	}
    }

    wxString other;
    if (!m_Parent->HasCompleteDateInfo()) {
	/* TRANSLATORS: Used in the "colour key" for "colour by date" if there
	 * are surveys without date information.  Try to keep this fairly short.
	 */
	other = wmsg(/*Undated*/221);
    }

    DrawColourKey(num_bands, other, wxString());
}

void GfxCore::DrawErrorKey()
{
    int num_bands;
    if (HasErrorInformation()) {
	// Use fixed colours for each error factor so it's directly visually
	// comparable between surveys.
	num_bands = GetNumColourBands();
	for (int band = 0; band < num_bands; ++band) {
	    double E = MAX_ERROR * band / (num_bands - 1);
	    key_legends[band].Printf(wxT("%.2f"), E);
	}
    } else {
	num_bands = 0;
    }

    // Always show the "Not in loop" legend for now (FIXME).
    /* TRANSLATORS: Used in the "colour key" for "colour by error" for surveys
     * which aren’t part of a loop and so have no error information. Try to keep
     * this fairly short. */
    DrawColourKey(num_bands, wmsg(/*Not in loop*/290), wxString());
}

void GfxCore::DrawGradientKey()
{
    int num_bands;
    // Use fixed colours for each gradient so it's directly visually comparable
    // between surveys.
    num_bands = GetNumColourBands();
    wxString units = wmsg(m_Degrees ? /*°*/344 : /*ᵍ*/345);
    for (int band = 0; band < num_bands; ++band) {
	double gradient = double(band) / (num_bands - 1);
	if (m_Degrees) {
	    gradient *= 90.0;
	} else {
	    gradient *= 100.0;
	}
	key_legends[band].Printf(wxT("%.f%s"), gradient, units);
    }

    DrawColourKey(num_bands, wxString(), wxString());
}

void GfxCore::DrawLengthKey()
{
    int num_bands;
    // Use fixed colours for each length so it's directly visually comparable
    // between surveys.
    num_bands = GetNumColourBands();
    for (int band = 0; band < num_bands; ++band) {
	double len = pow(10, LOG_LEN_MAX * band / (num_bands - 1));
	if (!m_Metric) {
	    len /= METRES_PER_FOOT;
	}
	key_legends[band].Printf(wxT("%.1f"), len);
    }

    DrawColourKey(num_bands, wxString(), wmsg(m_Metric ? /*m*/424: /*ft*/428));
}

void GfxCore::DrawScaleBar()
{
    // Draw the scalebar.
    if (GetPerspective()) return;

    // Calculate how many metres of survey are currently displayed across the
    // screen.
    Double across_screen = SurveyUnitsAcrossViewport();

    double f = double(GetClinoXPosition() - INDICATOR_BOX_SIZE / 2 - SCALE_BAR_OFFSET_X) / GetXSize();
    if (f > 0.75) {
	f = 0.75;
    } else if (f < 0.5) {
	// Stop it getting squeezed to nothing.
	// FIXME: In this case we should probably move the compass and clino up
	// to make room rather than letting stuff overlap.
	f = 0.5;
    }

    // Convert to imperial measurements if required.
    Double multiplier = 1.0;
    if (!m_Metric) {
	across_screen /= METRES_PER_FOOT;
	multiplier = METRES_PER_FOOT;
	if (across_screen >= 5280.0 / f) {
	    across_screen /= 5280.0;
	    multiplier *= 5280.0;
	}
    }

    // Calculate the length of the scale bar.
    Double size_snap = pow(10.0, floor(log10(f * across_screen)));
    Double t = across_screen * f / size_snap;
    if (t >= 5.0) {
	size_snap *= 5.0;
    } else if (t >= 2.0) {
	size_snap *= 2.0;
    }

    if (!m_Metric) size_snap *= multiplier;

    // Actual size of the thing in pixels:
    int size = int((size_snap / SurveyUnitsAcrossViewport()) * GetXSize());
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
    wxString str;
    int units;
    if (m_Metric) {
	Double km = size_snap * 1e-3;
	if (km >= 1.0) {
	    size_snap = km;
	    /* TRANSLATORS: abbreviation for "kilometres" (unit of length),
	     * used e.g.  "5km".
	     *
	     * If there should be a space between the number and this, include
	     * one in the translation. */
	    units = /*km*/423;
	} else if (size_snap >= 1.0) {
	    /* TRANSLATORS: abbreviation for "metres" (unit of length), used
	     * e.g. "10m".
	     *
	     * If there should be a space between the number and this, include
	     * one in the translation. */
	    units = /*m*/424;
	} else {
	    size_snap *= 1e2;
	    /* TRANSLATORS: abbreviation for "centimetres" (unit of length),
	     * used e.g.  "50cm".
	     *
	     * If there should be a space between the number and this, include
	     * one in the translation. */
	    units = /*cm*/425;
	}
    } else {
	size_snap /= METRES_PER_FOOT;
	Double miles = size_snap / 5280.0;
	if (miles >= 1.0) {
	    size_snap = miles;
	    if (size_snap >= 2.0) {
		/* TRANSLATORS: abbreviation for "miles" (unit of length,
		 * plural), used e.g.  "2 miles".
		 *
		 * If there should be a space between the number and this,
		 * include one in the translation. */
		units = /* miles*/426;
	    } else {
		/* TRANSLATORS: abbreviation for "mile" (unit of length,
		 * singular), used e.g.  "1 mile".
		 *
		 * If there should be a space between the number and this,
		 * include one in the translation. */
		units = /* mile*/427;
	    }
	} else if (size_snap >= 1.0) {
	    /* TRANSLATORS: abbreviation for "feet" (unit of length), used e.g.
	     * as "10ft".
	     *
	     * If there should be a space between the number and this, include
	     * one in the translation. */
	    units = /*ft*/428;
	} else {
	    size_snap *= 12.0;
	    /* TRANSLATORS: abbreviation for "inches" (unit of length), used
	     * e.g. as "6in".
	     *
	     * If there should be a space between the number and this, include
	     * one in the translation. */
	    units = /*in*/429;
	}
    }
    if (size_snap >= 1.0) {
	str.Printf(wxT("%.f%s"), size_snap, wmsg(units).c_str());
    } else {
	int sf = -(int)floor(log10(size_snap));
	str.Printf(wxT("%.*f%s"), sf, size_snap, wmsg(units).c_str());
    }

    int text_width, text_height;
    GetTextExtent(str, &text_width, &text_height);
    const int text_y = end_y - text_height + 1;
    SetColour(TEXT_COLOUR);
    DrawIndicatorText(SCALE_BAR_OFFSET_X, text_y, wxT("0"));
    DrawIndicatorText(SCALE_BAR_OFFSET_X + size - text_width, text_y, str);
}

bool GfxCore::CheckHitTestGrid(const wxPoint& point, bool centre)
{
    if (Animating()) return false;

    if (point.x < 0 || point.x >= GetXSize() ||
	point.y < 0 || point.y >= GetYSize()) {
	return false;
    }

    SetDataTransform();

    if (!m_HitTestGridValid) CreateHitTestGrid();

    int grid_x = point.x * HITTEST_SIZE / (GetXSize() + 1);
    int grid_y = point.y * HITTEST_SIZE / (GetYSize() + 1);

    LabelInfo *best = NULL;
    int dist_sqrd = sqrd_measure_threshold;
    int square = grid_x + grid_y * HITTEST_SIZE;
    list<LabelInfo*>::iterator iter = m_PointGrid[square].begin();

    while (iter != m_PointGrid[square].end()) {
	LabelInfo *pt = *iter++;

	double cx, cy, cz;

	Transform(*pt, &cx, &cy, &cz);

	cy = GetYSize() - cy;

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

    if (best) {
	m_Parent->ShowInfo(best, m_there);
	if (centre) {
	    // FIXME: allow Ctrl-Click to not set there or something?
	    CentreOn(*best);
	    WarpPointer(GetXSize() / 2, GetYSize() / 2);
	    SetThere(best);
	    m_Parent->SelectTreeItem(best);
	}
    } else {
	// Left-clicking not on a survey cancels the measuring line.
	if (centre) {
	    ClearTreeSelection();
	} else {
	    m_Parent->ShowInfo(best, m_there);
	    double x, y, z;
	    ReverseTransform(point.x, GetYSize() - point.y, &x, &y, &z);
	    temp_here.assign(Vector3(x, y, z));
	    SetHere(&temp_here);
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
	// 1.0.32 and wxGTK 2.5.2 (load a file from the command line).
	// With 1.1.6 and wxGTK 2.4.2 we only get negative sizes if MainFrm
	// is resized such that the GfxCore window isn't visible.
	//printf("OnSize(%d,%d)\n", size.GetWidth(), size.GetHeight());
	return;
    }

    event.Skip();

    if (m_DoneFirstShow) {
	TryToFreeArrays();

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
	m_TiltAngle = -90.0;
    }

    SetRotation(m_PanAngle, m_TiltAngle);
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

    // Set the initial scale.
    SetScale(initial_scale);
}

void GfxCore::Defaults()
{
    // Restore default scale, rotation and translation parameters.
    DefaultParameters();

    // Invalidate all the cached lists.
    GLACanvas::FirstShow();

    ForceRefresh();
}

void GfxCore::Animate()
{
    // Don't show pointer coordinates while animating.
    // FIXME : only do this when we *START* animating!  Use a static copy
    // of the value of "Animating()" last time we were here to track this?
    // MainFrm now checks if we're trying to clear already cleared labels
    // and just returns, but it might be simpler to check here!
    ClearCoords();
    m_Parent->ShowInfo();

    long t;
    if (movie) {
	ReadPixels(movie->GetWidth(), movie->GetHeight(), movie->GetBuffer());
	if (!movie->AddFrame()) {
	    wxGetApp().ReportError(wxString(movie->get_error_string(), wxConvUTF8));
	    delete movie;
	    movie = NULL;
	    presentation_mode = 0;
	    return;
	}
	t = 1000 / 25; // 25 frames per second
    } else {
	static long t_prev = 0;
	t = timer.Time();
	// Avoid redrawing twice in the same frame.
	long delta_t = (t_prev == 0 ? 1000 / MAX_FRAMERATE : t - t_prev);
	if (delta_t < 1000 / MAX_FRAMERATE)
	    return;
	t_prev = t;
	if (presentation_mode == PLAYING && pres_speed != 0.0)
	    t = delta_t;
    }

    if (presentation_mode == PLAYING && pres_speed != 0.0) {
	// FIXME: It would probably be better to work relative to the time we
	// passed the last mark, but that's complicated by the speed
	// potentially changing (or even the direction of playback reversing)
	// at any point during playback.
	Double tick = t * 0.001 * fabs(pres_speed);
	while (tick >= next_mark_time) {
	    tick -= next_mark_time;
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
		if (movie && !movie->Close()) {
		    wxGetApp().ReportError(wxString(movie->get_error_string(), wxConvUTF8));
		}
		delete movie;
		movie = NULL;
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
	    double p = tick / next_mark_time;
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
	    this_mark_total += tick;
	    next_mark_time -= tick;
	}

	ForceRefresh();
	return;
    }

    // When rotating...
    if (m_Rotating) {
	Double step = base_pan + (t - base_pan_time) * 1e-3 * m_RotationStep - m_PanAngle;
	TurnCave(step);
    }

    if (m_SwitchingTo == PLAN) {
	// When switching to plan view...
	Double step = base_tilt - (t - base_tilt_time) * 1e-3 * 90.0 - m_TiltAngle;
	TiltCave(step);
	if (m_TiltAngle == -90.0) {
	    m_SwitchingTo = 0;
	}
    } else if (m_SwitchingTo == ELEVATION) {
	// When switching to elevation view...
	Double step;
	if (m_TiltAngle > 0.0) {
	    step = base_tilt - (t - base_tilt_time) * 1e-3 * 90.0 - m_TiltAngle;
	} else {
	    step = base_tilt + (t - base_tilt_time) * 1e-3 * 90.0 - m_TiltAngle;
	}
	if (fabs(step) >= fabs(m_TiltAngle)) {
	    m_SwitchingTo = 0;
	    step = -m_TiltAngle;
	}
	TiltCave(step);
    } else if (m_SwitchingTo) {
	// Rotate the shortest way around to the destination angle.  If we're
	// 180 off, we favour turning anticlockwise, as auto-rotation does by
	// default.
	Double target = (m_SwitchingTo - NORTH) * 90;
	Double diff = target - m_PanAngle;
	diff = fmod(diff, 360);
	if (diff <= -180)
	    diff += 360;
	else if (diff > 180)
	    diff -= 360;
	if (m_RotationStep < 0 && diff == 180.0)
	    diff = -180.0;
	Double step = base_pan - m_PanAngle;
	Double delta = (t - base_pan_time) * 1e-3 * fabs(m_RotationStep);
	if (diff > 0) {
	    step += delta;
	} else {
	    step -= delta;
	}
	step = fmod(step, 360);
	if (step <= -180)
	    step += 360;
	else if (step > 180)
	    step -= 360;
	if (fabs(step) >= fabs(diff)) {
	    m_SwitchingTo = 0;
	    step = diff;
	}
	TurnCave(step);
    }

    ForceRefresh();
}

// How much to allow around the box - this is because of the ring shape
// at one end of the line.
static const int HIGHLIGHTED_PT_SIZE = 2; // FIXME: tie in to blob and ring size
#define MARGIN (HIGHLIGHTED_PT_SIZE * 2 + 1)
void GfxCore::RefreshLine(const Point *a, const Point *b, const Point *c)
{
#ifdef __WXMSW__
    (void)a;
    (void)b;
    (void)c;
    // FIXME: We get odd redraw artifacts if we just update the line, and
    // redrawing the whole scene doesn't actually seem to be measurably
    // slower.  That may not be true with software rendering though...
    ForceRefresh();
#else
    // Best of all might be to copy the window contents before we draw the
    // line, then replace each time we redraw.

    // Calculate the minimum rectangle which includes the old and new
    // measuring lines to minimise the redraw time
    int l = INT_MAX, r = INT_MIN, u = INT_MIN, d = INT_MAX;
    double X, Y, Z;
    if (a) {
	if (!Transform(*a, &X, &Y, &Z)) {
	    printf("oops\n");
	} else {
	    int x = int(X);
	    int y = GetYSize() - 1 - int(Y);
	    l = x;
	    r = x;
	    u = y;
	    d = y;
	}
    }
    if (b) {
	if (!Transform(*b, &X, &Y, &Z)) {
	    printf("oops\n");
	} else {
	    int x = int(X);
	    int y = GetYSize() - 1 - int(Y);
	    l = min(l, x);
	    r = max(r, x);
	    u = max(u, y);
	    d = min(d, y);
	}
    }
    if (c) {
	if (!Transform(*c, &X, &Y, &Z)) {
	    printf("oops\n");
	} else {
	    int x = int(X);
	    int y = GetYSize() - 1 - int(Y);
	    l = min(l, x);
	    r = max(r, x);
	    u = max(u, y);
	    d = min(d, y);
	}
    }
    l -= MARGIN;
    r += MARGIN;
    u += MARGIN;
    d -= MARGIN;
    RefreshRect(wxRect(l, d, r - l, u - d), false);
#endif
}

void GfxCore::SetHereFromTree(const LabelInfo * p)
{
    SetHere(p);
    m_Parent->ShowInfo(m_here, m_there);
}

void GfxCore::SetHere(const LabelInfo *p)
{
    if (p == m_here) return;
    bool line_active = MeasuringLineActive();
    const LabelInfo * old = m_here;
    m_here = p;
    if (line_active || MeasuringLineActive())
	RefreshLine(old, m_there, m_here);
}

void GfxCore::SetThere(const LabelInfo * p)
{
    if (p == m_there) return;
    const LabelInfo * old = m_there;
    m_there = p;
    RefreshLine(m_here, old, m_there);
}

void GfxCore::CreateHitTestGrid()
{
    if (!m_PointGrid) {
	// Initialise hit-test grid.
	m_PointGrid = new list<LabelInfo*>[HITTEST_SIZE * HITTEST_SIZE];
    } else {
	// Clear hit-test grid.
	for (int i = 0; i < HITTEST_SIZE * HITTEST_SIZE; i++) {
	    m_PointGrid[i].clear();
	}
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
	double cx, cy, cz;
	Transform(*label, &cx, &cy, &cz);
	if (cx < 0 || cx >= GetXSize()) continue;
	if (cy < 0 || cy >= GetYSize()) continue;

	cy = GetYSize() - cy;

	// On-screen, so add to hit-test grid...
	int grid_x = int(cx * HITTEST_SIZE / (GetXSize() + 1));
	int grid_y = int(cy * HITTEST_SIZE / (GetYSize() + 1));

	m_PointGrid[grid_x + grid_y * HITTEST_SIZE].push_back(label);
    }

    m_HitTestGridValid = true;
}

//
//  Methods for controlling the orientation of the survey
//

void GfxCore::TurnCave(Double angle)
{
    // Turn the cave around its z-axis by a given angle.

    m_PanAngle += angle;
    // Wrap to range [0, 360):
    m_PanAngle = fmod(m_PanAngle, 360.0);
    if (m_PanAngle < 0.0) {
	m_PanAngle += 360.0;
    }

    m_HitTestGridValid = false;
    if (m_here && m_here == &temp_here) SetHere();

    SetRotation(m_PanAngle, m_TiltAngle);
}

void GfxCore::TurnCaveTo(Double angle)
{
    if (m_Rotating) {
	// If we're rotating, jump to the specified angle.
	TurnCave(angle - m_PanAngle);
	SetPanBase();
	return;
    }

    int new_switching_to = ((int)angle) / 90 + NORTH;
    if (new_switching_to == m_SwitchingTo) {
	// A second order to switch takes us there right away
	TurnCave(angle - m_PanAngle);
	m_SwitchingTo = 0;
	ForceRefresh();
    } else {
	SetPanBase();
	m_SwitchingTo = new_switching_to;
    }
}

void GfxCore::TiltCave(Double tilt_angle)
{
    // Tilt the cave by a given angle.
    if (m_TiltAngle + tilt_angle > 90.0) {
	m_TiltAngle = 90.0;
    } else if (m_TiltAngle + tilt_angle < -90.0) {
	m_TiltAngle = -90.0;
    } else {
	m_TiltAngle += tilt_angle;
    }

    m_HitTestGridValid = false;
    if (m_here && m_here == &temp_here) SetHere();

    SetRotation(m_PanAngle, m_TiltAngle);
}

void GfxCore::TranslateCave(int dx, int dy)
{
    AddTranslationScreenCoordinates(dx, dy);
    m_HitTestGridValid = false;

    if (m_here && m_here == &temp_here) SetHere();

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

    double cx, cy, cz;

    SetDataTransform();
    ReverseTransform(point.x, GetYSize() - 1 - point.y, &cx, &cy, &cz);

    if (ShowingPlan()) {
	m_Parent->SetCoords(cx + m_Parent->GetOffset().GetX(),
			    cy + m_Parent->GetOffset().GetY(),
			    m_there);
    } else if (ShowingElevation()) {
	m_Parent->SetAltitude(cz + m_Parent->GetOffset().GetZ(),
			      m_there);
    } else {
	m_Parent->ClearCoords();
    }
}

int GfxCore::GetCompassWidth() const
{
    static int result = 0;
    if (result == 0) {
	result = INDICATOR_BOX_SIZE;
	int width;
	const wxString & msg = wmsg(/*Facing*/203);
	GetTextExtent(msg, &width, NULL);
	if (width > result) result = width;
    }
    return result;
}

int GfxCore::GetClinoWidth() const
{
    static int result = 0;
    if (result == 0) {
	result = INDICATOR_BOX_SIZE;
	int width;
	const wxString & msg1 = wmsg(/*Plan*/432);
	GetTextExtent(msg1, &width, NULL);
	if (width > result) result = width;
	const wxString & msg2 = wmsg(/*Kiwi Plan*/433);
	GetTextExtent(msg2, &width, NULL);
	if (width > result) result = width;
	const wxString & msg3 = wmsg(/*Elevation*/118);
	GetTextExtent(msg3, &width, NULL);
	if (width > result) result = width;
    }
    return result;
}

int GfxCore::GetCompassXPosition() const
{
    // Return the x-coordinate of the centre of the compass in window
    // coordinates.
    return GetXSize() - INDICATOR_OFFSET_X - GetCompassWidth() / 2;
}

int GfxCore::GetClinoXPosition() const
{
    // Return the x-coordinate of the centre of the compass in window
    // coordinates.
    return GetXSize() - GetClinoOffset() - GetClinoWidth() / 2;
}

int GfxCore::GetIndicatorYPosition() const
{
    // Return the y-coordinate of the centre of the indicators in window
    // coordinates.
    return GetYSize() - INDICATOR_OFFSET_Y - INDICATOR_BOX_SIZE / 2;
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
	    point.y <= GetYSize() - SCALE_BAR_OFFSET_Y - SCALE_BAR_HEIGHT &&
	    point.y >= GetYSize() - SCALE_BAR_OFFSET_Y - SCALE_BAR_HEIGHT*2);
}

bool GfxCore::PointWithinColourKey(wxPoint point) const
{
    // Determine whether a point (in window coordinates) lies within the key.
    point.x -= GetXSize() - KEY_OFFSET_X;
    point.y = KEY_OFFSET_Y - point.y;
    return (point.x >= key_lowerleft[m_ColourBy].x && point.x <= 0 &&
	    point.y >= key_lowerleft[m_ColourBy].y && point.y <= 0);
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
	TiltCave(-deg(atan2(double(dy), double(dx))) - m_TiltAngle);
	m_MouseOutsideElev = false;
    } else if (dy >= INDICATOR_MARGIN) {
	TiltCave(-90.0 - m_TiltAngle);
	m_MouseOutsideElev = true;
    } else if (dy <= -INDICATOR_MARGIN) {
	TiltCave(90.0 - m_TiltAngle);
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

    int total_width = GetCompassWidth() + INDICATOR_GAP + GetClinoWidth();
    RefreshRect(wxRect(GetXSize() - INDICATOR_OFFSET_X - total_width,
		       GetYSize() - INDICATOR_OFFSET_Y - INDICATOR_BOX_SIZE,
		       total_width,
		       INDICATOR_BOX_SIZE), false);
}

void GfxCore::StartRotation()
{
    // Start the survey rotating.

    if (m_SwitchingTo >= NORTH)
	m_SwitchingTo = 0;
    m_Rotating = true;
    SetPanBase();
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
    if (m_Rotating)
	SetPanBase();
}

void GfxCore::RotateSlower(bool accel)
{
    // Decrease the speed of rotation, optionally by an increased amount.
    if (fabs(m_RotationStep) == 1.0)
	return;

    m_RotationStep *= accel ? (1 / 1.44) : (1 / 1.2);

    if (fabs(m_RotationStep) < 1.0) {
	m_RotationStep = (m_RotationStep > 0 ? 1.0 : -1.0);
    }
    if (m_Rotating)
	SetPanBase();
}

void GfxCore::RotateFaster(bool accel)
{
    // Increase the speed of rotation, optionally by an increased amount.
    if (fabs(m_RotationStep) == 180.0)
	return;

    m_RotationStep *= accel ? 1.44 : 1.2;
    if (fabs(m_RotationStep) > 180.0) {
	m_RotationStep = (m_RotationStep > 0 ? 180.0 : -180.0);
    }
    if (m_Rotating)
	SetPanBase();
}

void GfxCore::SwitchToElevation()
{
    // Perform an animated switch to elevation view.

    if (m_SwitchingTo != ELEVATION) {
	SetTiltBase();
	m_SwitchingTo = ELEVATION;
    } else {
	// A second order to switch takes us there right away
	TiltCave(-m_TiltAngle);
	m_SwitchingTo = 0;
	ForceRefresh();
    }
}

void GfxCore::SwitchToPlan()
{
    // Perform an animated switch to plan view.

    if (m_SwitchingTo != PLAN) {
	SetTiltBase();
	m_SwitchingTo = PLAN;
    } else {
	// A second order to switch takes us there right away
	TiltCave(-90.0 - m_TiltAngle);
	m_SwitchingTo = 0;
	ForceRefresh();
    }
}

void GfxCore::SetViewTo(Double xmin, Double xmax, Double ymin, Double ymax, Double zmin, Double zmax)
{

    SetTranslation(-Vector3((xmin + xmax) / 2, (ymin + ymax) / 2, (zmin + zmax) / 2));
    Double scale = HUGE_VAL;
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
    if (scale != HUGE_VAL) SetScale(scale);
    ForceRefresh();
}

bool GfxCore::CanRaiseViewpoint() const
{
    // Determine if the survey can be viewed from a higher angle of elevation.

    return GetPerspective() ? (m_TiltAngle < 90.0) : (m_TiltAngle > -90.0);
}

bool GfxCore::CanLowerViewpoint() const
{
    // Determine if the survey can be viewed from a lower angle of elevation.

    return GetPerspective() ? (m_TiltAngle > -90.0) : (m_TiltAngle < 90.0);
}

bool GfxCore::HasDepth() const
{
    return m_Parent->GetDepthExtent() == 0.0;
}

bool GfxCore::HasErrorInformation() const
{
    return m_Parent->HasErrorInformation();
}

bool GfxCore::HasDateInformation() const
{
    return m_Parent->GetDateMin() >= 0;
}

bool GfxCore::ShowingPlan() const
{
    // Determine if the survey is in plan view.

    return (m_TiltAngle == -90.0);
}

bool GfxCore::ShowingElevation() const
{
    // Determine if the survey is in elevation view.

    return (m_TiltAngle == 0.0);
}

bool GfxCore::ShowingMeasuringLine() const
{
    // Determine if the measuring line is being shown.  Only check if "there"
    // is valid, since that means the measuring line anchor is out.

    return m_there;
}

void GfxCore::ToggleFlag(bool* flag, int update)
{
    *flag = !*flag;
    if (update == UPDATE_BLOBS) {
	UpdateBlobs();
    } else if (update == UPDATE_BLOBS_AND_CROSSES) {
	UpdateBlobs();
	InvalidateList(LIST_CROSSES);
	m_HitTestGridValid = false;
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

void GfxCore::ToggleTerrain()
{
    if (!m_Terrain && !dem) {
	// OnOpenTerrain() calls us if a file is selected.
	wxCommandEvent dummy;
	m_Parent->OnOpenTerrain(dummy);
	return;
    }
    ToggleFlag(&m_Terrain);
}

void GfxCore::ToggleFatFinger()
{
    if (sqrd_measure_threshold == sqrd(MEASURE_THRESHOLD)) {
	sqrd_measure_threshold = sqrd(5 * MEASURE_THRESHOLD);
	wxMessageBox(wxT("Fat finger enabled"), wxT("Aven Debug"), wxOK | wxICON_INFORMATION);
    } else {
	sqrd_measure_threshold = sqrd(MEASURE_THRESHOLD);
	wxMessageBox(wxT("Fat finger disabled"), wxT("Aven Debug"), wxOK | wxICON_INFORMATION);
    }
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
	case LIST_SCALE_BAR:
	    DrawScaleBar();
	    break;
	case LIST_DEPTH_KEY:
	    DrawDepthKey();
	    break;
	case LIST_DATE_KEY:
	    DrawDateKey();
	    break;
	case LIST_ERROR_KEY:
	    DrawErrorKey();
	    break;
	case LIST_GRADIENT_KEY:
	    DrawGradientKey();
	    break;
	case LIST_LENGTH_KEY:
	    DrawLengthKey();
	    break;
	case LIST_UNDERGROUND_LEGS:
	    GenerateDisplayList(false);
	    break;
	case LIST_TUBES:
	    GenerateDisplayListTubes();
	    break;
	case LIST_SURFACE_LEGS:
	    GenerateDisplayList(true);
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
	case LIST_TERRAIN:
	    DrawTerrain();
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

void GfxCore::GenerateDisplayList(bool surface)
{
    unsigned surf_or_not = surface ? img_FLAG_SURFACE : 0;
    // Generate the display list for the surface or underground legs.
    for (int f = 0; f != 8; ++f) {
	if ((f & img_FLAG_SURFACE) != surf_or_not) continue;
	const unsigned SHOW_DASHED_AND_FADED = unsigned(-1);
	unsigned style = SHOW_NORMAL;
	if ((f & img_FLAG_SPLAY) && m_Splays != SHOW_NORMAL) {
	    style = m_Splays;
	} else if (f & img_FLAG_DUPLICATE) {
	    style = m_Dupes;
	}
	if (f & img_FLAG_SURFACE) {
	    if (style == SHOW_FADED) {
		style = SHOW_DASHED_AND_FADED;
	    } else {
		style = SHOW_DASHED;
	    }
	}

	switch (style) {
	    case SHOW_HIDE:
		continue;
	    case SHOW_FADED:
		SetAlpha(0.4);
		break;
	    case SHOW_DASHED:
		EnableDashedLines();
		break;
	    case SHOW_DASHED_AND_FADED:
		SetAlpha(0.4);
		EnableDashedLines();
		break;
	}

	void (GfxCore::* add_poly)(const traverse&);
	if (surface) {
	    if (m_ColourBy == COLOUR_BY_ERROR) {
		add_poly = &GfxCore::AddPolylineError;
	    } else {
		add_poly = &GfxCore::AddPolyline;
	    }
	} else {
	    add_poly = AddPoly;
	}

	list<traverse>::const_iterator trav = m_Parent->traverses_begin(f);
	list<traverse>::const_iterator tend = m_Parent->traverses_end(f);
	while (trav != tend) {
	    (this->*add_poly)(*trav);
	    ++trav;
	}

	switch (style) {
	    case SHOW_FADED:
		SetAlpha(1.0);
		break;
	    case SHOW_DASHED:
		DisableDashedLines();
		break;
	    case SHOW_DASHED_AND_FADED:
		DisableDashedLines();
		SetAlpha(1.0);
		break;
	}
    }
}

void GfxCore::GenerateDisplayListTubes()
{
    // Generate the display list for the tubes.
    list<vector<XSect> >::iterator trav = m_Parent->tubes_begin();
    list<vector<XSect> >::iterator tend = m_Parent->tubes_end();
    while (trav != tend) {
	SkinPassage(*trav);
	++trav;
    }
}

void GfxCore::GenerateDisplayListShadow()
{
    SetColour(col_BLACK);
    for (int f = 0; f != 8; ++f) {
	// Only include underground legs in the shadow.
	if ((f & img_FLAG_SURFACE) != 0) continue;
	list<traverse>::const_iterator trav = m_Parent->traverses_begin(f);
	list<traverse>::const_iterator tend = m_Parent->traverses_end(f);
	while (trav != tend) {
	    AddPolylineShadow(*trav);
	    ++trav;
	}
    }
}

void
GfxCore::parse_hgt_filename(const wxString & lc_name)
{
    char * leaf = leaf_from_fnm(lc_name.utf8_str());
    const char * p = leaf;
    char * q;
    char dirn = *p++;
    o_y = strtoul(p, &q, 10);
    p = q;
    if (dirn == 's')
	o_y = -o_y;
    ++o_y;
    dirn = *p++;
    o_x = strtoul(p, &q, 10);
    if (dirn == 'w')
	o_x = -o_x;
    bigendian = true;
    nodata_value = -32768;
    osfree(leaf);
}

size_t
GfxCore::parse_hdr(wxInputStream & is, unsigned long & skipbytes)
{
    // ESRI docs say NBITS defaults to 8.
    unsigned long nbits = 8;
    // ESRI docs say NBANDS defaults to 1.
    unsigned long nbands = 1;
    unsigned long bandrowbytes = 0;
    unsigned long totalrowbytes = 0;
    // ESRI docs say ULXMAP defaults to 0.
    o_x = 0.0;
    // ESRI docs say ULYMAP defaults to NROWS - 1.
    o_y = HUGE_VAL;
    // ESRI docs say XDIM and YDIM default to 1.
    step_x = step_y = 1.0;
    while (!is.Eof()) {
	wxString line;
	int ch;
	while ((ch = is.GetC()) != wxEOF) {
	    if (ch == '\n' || ch == '\r') break;
	    line += wxChar(ch);
	}
#define CHECK(X, COND) \
} else if (line.StartsWith(wxT(X " "))) { \
size_t v = line.find_first_not_of(wxT(' '), sizeof(X)); \
if (v == line.npos || !(COND)) { \
err += wxT("Unexpected value for " X); \
}
	wxString err;
	if (false) {
	// I = little-endian; M = big-endian
	CHECK("BYTEORDER", (bigendian = (line[v] == 'M')) || line[v] == 'I')
	// ESRI docs say LAYOUT defaults to BIL if not specified.
	CHECK("LAYOUT", line.substr(v) == wxT("BIL"))
	CHECK("NROWS", line.substr(v).ToCULong(&dem_height))
	CHECK("NCOLS", line.substr(v).ToCULong(&dem_width))
	// ESRI docs say NBANDS defaults to 1 if not specified.
	CHECK("NBANDS", line.substr(v).ToCULong(&nbands) && nbands == 1)
	CHECK("NBITS", line.substr(v).ToCULong(&nbits) && nbits == 16)
	CHECK("BANDROWBYTES", line.substr(v).ToCULong(&bandrowbytes))
	CHECK("TOTALROWBYTES", line.substr(v).ToCULong(&totalrowbytes))
	// PIXELTYPE is a GDAL extension, so may not be present.
	CHECK("PIXELTYPE", line.substr(v) == wxT("SIGNEDINT"))
	CHECK("ULXMAP", line.substr(v).ToCDouble(&o_x))
	CHECK("ULYMAP", line.substr(v).ToCDouble(&o_y))
	CHECK("XDIM", line.substr(v).ToCDouble(&step_x))
	CHECK("YDIM", line.substr(v).ToCDouble(&step_y))
	CHECK("NODATA", line.substr(v).ToCLong(&nodata_value))
	CHECK("SKIPBYTES", line.substr(v).ToCULong(&skipbytes))
	}
	if (!err.empty()) {
	    wxMessageBox(err);
	}
    }
    if (o_y == HUGE_VAL) {
	o_y = dem_height - 1;
    }
    if (bandrowbytes != 0) {
	if (nbits * dem_width != bandrowbytes * 8) {
	    wxMessageBox("BANDROWBYTES setting indicates unused bits after each band - not currently supported");
	}
    }
    if (totalrowbytes != 0) {
	// This is the ESRI default for BIL, for BIP it would be
	// nbands * bandrowbytes.
	if (nbands * nbits * dem_width != totalrowbytes * 8) {
	    wxMessageBox("TOTALROWBYTES setting indicates unused bits after "
			 "each row - not currently supported");
	}
    }
    return ((nbits * dem_width + 7) / 8) * dem_height;
}

bool
GfxCore::read_bil(wxInputStream & is, size_t size, unsigned long skipbytes)
{
    bool know_size = true;
    if (!size) {
	// If the stream doesn't know its size, GetSize() returns 0.
	size = is.GetSize();
	if (!size) {
	    size = DEFAULT_HGT_SIZE;
	    know_size = false;
	}
    }
    dem = new unsigned short[size / 2];
    if (skipbytes) {
	if (is.SeekI(skipbytes, wxFromStart) == ::wxInvalidOffset) {
	    while (skipbytes) {
		unsigned long to_read = skipbytes;
		if (size < to_read) to_read = size;
		is.Read(reinterpret_cast<char *>(dem), to_read);
		size_t c = is.LastRead();
		if (c == 0) {
		    wxMessageBox(wxT("Failed to skip terrain data header"));
		    break;
		}
		skipbytes -= c;
	    }
	}
    }

#if wxCHECK_VERSION(2,9,5)
    if (!is.ReadAll(dem, size)) {
	if (know_size) {
	    // FIXME: On __WXMSW__ currently we fail to
	    // read any data from files in zips.
	    delete [] dem;
	    dem = NULL;
	    wxMessageBox(wxT("Failed to read terrain data"));
	    return false;
	}
	size = is.LastRead();
    }
#else
    char * p = reinterpret_cast<char *>(dem);
    while (size) {
	is.Read(p, size);
	size_t c = is.LastRead();
	if (c == 0) {
	    if (!know_size) {
		size = DEFAULT_HGT_SIZE - size;
		if (size)
		    break;
	    }
	    delete [] dem;
	    dem = NULL;
	    wxMessageBox(wxT("Failed to read terrain data"));
	    return false;
	}
	p += c;
	size -= c;
    }
#endif

    if (dem_width == 0 && dem_height == 0) {
	dem_width = dem_height = sqrt(size / 2);
	if (dem_width * dem_height * 2 != size) {
	    delete [] dem;
	    dem = NULL;
	    wxMessageBox(wxT("HGT format data doesn't form a square"));
	    return false;
	}
	step_x = step_y = 1.0 / dem_width;
    }

    return true;
}

bool GfxCore::LoadDEM(const wxString & file)
{
    if (m_Parent->m_cs_proj.empty()) {
	wxMessageBox(wxT("No coordinate system specified in survey data"));
	return false;
    }

    delete [] dem;
    dem = NULL;

    size_t size = 0;
    // Default is to not skip any bytes.
    unsigned long skipbytes = 0;
    // For .hgt files, default to using filesize to determine.
    dem_width = dem_height = 0;
    // ESRI say "The default byte order is the same as that of the host machine
    // executing the software", but that's stupid so we default to
    // little-endian.
    bigendian = false;

    wxFileInputStream fs(file);
    if (!fs.IsOk()) {
	wxMessageBox(wxT("Failed to open DEM file"));
	return false;
    }

    const wxString & lc_file = file.Lower();
    if (lc_file.EndsWith(wxT(".hgt"))) {
	parse_hgt_filename(lc_file);
	read_bil(fs, size, skipbytes);
    } else if (lc_file.EndsWith(wxT(".bil"))) {
	wxString hdr_file = file;
	hdr_file.replace(file.size() - 4, 4, wxT(".hdr"));
	wxFileInputStream hdr_is(hdr_file);
	if (!hdr_is.IsOk()) {
	    wxMessageBox(wxT("Failed to open HDR file '") + hdr_file + wxT("'"));
	    return false;
	}
	size = parse_hdr(hdr_is, skipbytes);
	read_bil(fs, size, skipbytes);
    } else if (lc_file.EndsWith(wxT(".zip"))) {
	wxZipEntry * ze_data = NULL;
	wxZipInputStream zs(fs);
	wxZipEntry * ze;
	while ((ze = zs.GetNextEntry()) != NULL) {
	    if (!ze->IsDir()) {
		const wxString & lc_name = ze->GetName().Lower();
		if (!ze_data && lc_name.EndsWith(wxT(".hgt"))) {
		    // SRTM .hgt files are raw binary data, with the filename
		    // encoding the coordinates.
		    parse_hgt_filename(lc_name);
		    read_bil(zs, size, skipbytes);
		    delete ze;
		    break;
		}

		if (!ze_data && lc_name.EndsWith(wxT(".bil"))) {
		    if (size) {
			read_bil(zs, size, skipbytes);
			break;
		    }
		    ze_data = ze;
		    continue;
		}

		if (lc_name.EndsWith(wxT(".hdr"))) {
		    size = parse_hdr(zs, skipbytes);
		    if (ze_data) {
			if (!zs.OpenEntry(*ze_data)) {
			    wxMessageBox(wxT("Couldn't read DEM data from .zip file"));
			    break;
			}
			read_bil(zs, size, skipbytes);
		    }
		} else if (lc_name.EndsWith(wxT(".prj"))) {
		    //FIXME: check this matches the datum string we use
		    //Projection    GEOGRAPHIC
		    //Datum         WGS84
		    //Zunits        METERS
		    //Units         DD
		    //Spheroid      WGS84
		    //Xshift        0.0000000000
		    //Yshift        0.0000000000
		    //Parameters
		}
	    }
	    delete ze;
	}
	delete ze_data;
    }

    if (!dem) {
	return false;
    }

    InvalidateList(LIST_TERRAIN);
    ForceRefresh();
    return true;
}

void GfxCore::DrawTerrainTriangle(const Vector3 & a, const Vector3 & b, const Vector3 & c)
{
    Vector3 n = (b - a) * (c - a);
    n.normalise();
    Double factor = dot(n, light) * .95 + .05;
    SetColour(col_WHITE, factor);
    PlaceVertex(a);
    PlaceVertex(b);
    PlaceVertex(c);
    ++n_tris;
}

// Like wxBusyCursor, but you can cancel it early.
class AvenBusyCursor {
    bool active;

  public:
    AvenBusyCursor() : active(true) {
	wxBeginBusyCursor();
    }

    void stop() {
	if (active) {
	    active = false;
	    wxEndBusyCursor();
	}
    }

    ~AvenBusyCursor() {
	stop();
    }
};

void GfxCore::DrawTerrain()
{
    if (!dem) return;

    AvenBusyCursor hourglass;

    // Draw terrain to twice the extent, or at least 1km.
    double r_sqrd = sqrd(max(m_Parent->GetExtent().magnitude(), 1000.0));
#define WGS84_DATUM_STRING "+proj=longlat +ellps=WGS84 +datum=WGS84"
    static projPJ pj_in = pj_init_plus(WGS84_DATUM_STRING);
    if (!pj_in) {
	ToggleTerrain();
	delete [] dem;
	dem = NULL;
	hourglass.stop();
	error(/*Failed to initialise input coordinate system “%s”*/287, WGS84_DATUM_STRING);
	return;
    }
    static projPJ pj_out = pj_init_plus(m_Parent->m_cs_proj.c_str());
    if (!pj_out) {
	ToggleTerrain();
	delete [] dem;
	dem = NULL;
	hourglass.stop();
	error(/*Failed to initialise output coordinate system “%s”*/288, (const char *)m_Parent->m_cs_proj.c_str());
	return;
    }
    n_tris = 0;
    SetAlpha(0.3);
    BeginTriangles();
    const Vector3 & off = m_Parent->GetOffset();
    vector<Vector3> prevcol(dem_height + 1);
    for (size_t x = 0; x < dem_width; ++x) {
	double X_ = (o_x + x * step_x) * DEG_TO_RAD;
	Vector3 prev;
	for (size_t y = 0; y < dem_height; ++y) {
	    unsigned short elev = dem[x + y * dem_width];
#ifdef WORDS_BIGENDIAN
	    const bool MACHINE_BIGENDIAN = true;
#else
	    const bool MACHINE_BIGENDIAN = false;
#endif
	    if (bigendian != MACHINE_BIGENDIAN) {
#if defined __GNUC__ && (__GNUC__ * 100 + __GNUC_MINOR__ >= 408)
		elev = __builtin_bswap16(elev);
#else
		elev = (elev >> 8) | (elev << 8);
#endif
	    }
	    double Z = (short)elev;
	    Vector3 pt;
	    if (Z == nodata_value) {
		pt = Vector3(DBL_MAX, DBL_MAX, DBL_MAX);
	    } else {
		double X = X_;
		double Y = (o_y - y * step_y) * DEG_TO_RAD;
		pj_transform(pj_in, pj_out, 1, 1, &X, &Y, &Z);
		pt = Vector3(X, Y, Z) - off;
		double dist_2 = sqrd(pt.GetX()) + sqrd(pt.GetY());
		if (dist_2 > r_sqrd) {
		    pt = Vector3(DBL_MAX, DBL_MAX, DBL_MAX);
		}
	    }
	    if (x > 0 && y > 0) {
		const Vector3 & a = prevcol[y - 1];
		const Vector3 & b = prevcol[y];
		// If all points are valid, split the quadrilateral into
		// triangles along the shorter 3D diagonal, which typically
		// looks better:
		//
		//               ----->
		//     prev---a    x     prev---a
		//   |   |P  /|            |\  S|
		// y |   |  / |    or      | \  |
		//   V   | /  |            |  \ |
		//       |/  Q|            |R  \|
		//       b----pt           b----pt
		//
		//       FORWARD           BACKWARD
		enum { NONE = 0, P = 1, Q = 2, R = 4, S = 8, ALL = P|Q|R|S };
		int valid =
		    ((prev.GetZ() != DBL_MAX)) |
		    ((a.GetZ() != DBL_MAX) << 1) |
		    ((b.GetZ() != DBL_MAX) << 2) |
		    ((pt.GetZ() != DBL_MAX) << 3);
		static const int tris_map[16] = {
		    NONE, // nothing valid
		    NONE, // prev
		    NONE, // a
		    NONE, // a, prev
		    NONE, // b
		    NONE, // b, prev
		    NONE, // b, a
		    P, // b, a, prev
		    NONE, // pt
		    NONE, // pt, prev
		    NONE, // pt, a
		    S, // pt, a, prev
		    NONE, // pt, b
		    R, // pt, b, prev
		    Q, // pt, b, a
		    ALL, // pt, b, a, prev
		};
		int tris = tris_map[valid];
		if (tris == ALL) {
		    // All points valid.
		    if ((a - b).magnitude() < (prev - pt).magnitude()) {
			tris = P | Q;
		    } else {
			tris = R | S;
		    }
		}
		if (tris & P)
		    DrawTerrainTriangle(a, prev, b);
		if (tris & Q)
		    DrawTerrainTriangle(a, b, pt);
		if (tris & R)
		    DrawTerrainTriangle(pt, prev, b);
		if (tris & S)
		    DrawTerrainTriangle(a, prev, pt);
	    }
	    prev = prevcol[y];
	    prevcol[y].assign(pt);
	}
    }
    EndTriangles();
    SetAlpha(1.0);
    if (n_tris == 0) {
	ToggleTerrain();
	delete [] dem;
	dem = NULL;
	hourglass.stop();
	/* TRANSLATORS: Aven shows a circle of terrain covering the area
	 * of the survey plus a bit, but the terrain data file didn't
	 * contain any data inside that circle.
	 */
	error(/*No terrain data near area of survey*/161);
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
    // Draw colour key.
    if (m_ColourKey) {
	drawing_list key_list = LIST_LIMIT_;
	switch (m_ColourBy) {
	    case COLOUR_BY_DEPTH:
		key_list = LIST_DEPTH_KEY; break;
	    case COLOUR_BY_DATE:
		key_list = LIST_DATE_KEY; break;
	    case COLOUR_BY_ERROR:
		key_list = LIST_ERROR_KEY; break;
	    case COLOUR_BY_GRADIENT:
		key_list = LIST_GRADIENT_KEY; break;
	    case COLOUR_BY_LENGTH:
		key_list = LIST_LENGTH_KEY; break;
	}
	if (key_list != LIST_LIMIT_) {
	    DrawList2D(key_list, GetXSize() - KEY_OFFSET_X,
		       GetYSize() - KEY_OFFSET_Y, 0);
	}
    }

    // Draw compass or elevation/heading indicators.
    if (m_Compass || m_Clino) {
	if (!m_Parent->IsExtendedElevation()) Draw2dIndicators();
    }

    // Draw scalebar.
    if (m_Scalebar) {
	DrawList2D(LIST_SCALE_BAR, 0, 0, 0);
    }
}

void GfxCore::PlaceVertexWithColour(const Vector3 & v,
				    glaTexCoord tex_x, glaTexCoord tex_y,
				    Double factor)
{
    SetColour(col_WHITE, factor);
    PlaceVertex(v, tex_x, tex_y);
}

void GfxCore::SetDepthColour(Double z, Double factor) {
    // Set the drawing colour based on the altitude.
    Double z_ext = m_Parent->GetDepthExtent();

    z -= m_Parent->GetDepthMin();
    // points arising from tubes may be slightly outside the limits...
    if (z < 0) z = 0;
    if (z > z_ext) z = z_ext;

    if (z == 0) {
	SetColour(GetPen(0), factor);
	return;
    }

    assert(z_ext > 0.0);
    Double how_far = z / z_ext;
    assert(how_far >= 0.0);
    assert(how_far <= 1.0);

    int band = int(floor(how_far * (GetNumColourBands() - 1)));
    GLAPen pen1 = GetPen(band);
    if (band < GetNumColourBands() - 1) {
	const GLAPen& pen2 = GetPen(band + 1);

	Double interval = z_ext / (GetNumColourBands() - 1);
	Double into_band = z / interval - band;

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

void GfxCore::PlaceVertexWithDepthColour(const Vector3 &v, Double factor)
{
    SetDepthColour(v.GetZ(), factor);
    PlaceVertex(v);
}

void GfxCore::PlaceVertexWithDepthColour(const Vector3 &v,
					 glaTexCoord tex_x, glaTexCoord tex_y,
					 Double factor)
{
    SetDepthColour(v.GetZ(), factor);
    PlaceVertex(v, tex_x, tex_y);
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

void GfxCore::SplitPolyAcrossBands(vector<vector<Split> >& splits,
				   int band, int band2,
				   const Vector3 &p, const Vector3 &q,
				   glaTexCoord ptx, glaTexCoord pty,
				   glaTexCoord w, glaTexCoord h)
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
	glaTexCoord tx = ptx, ty = pty;
	if (w) tx += t * w;
	if (h) ty += t * h;

	splits[i].push_back(Split(Vector3(x, y, z), tx, ty));
	splits[i + step].push_back(Split(Vector3(x, y, z), tx, ty));
    }
}

int GfxCore::GetDepthColour(Double z) const
{
    // Return the (0-based) depth colour band index for a z-coordinate.
    Double z_ext = m_Parent->GetDepthExtent();
    z -= m_Parent->GetDepthMin();
    // We seem to get rounding differences causing z to sometimes be slightly
    // less than GetDepthMin() here, and it can certainly be true for passage
    // tubes, so just clamp the value to 0.
    if (z <= 0) return 0;
    // We seem to get rounding differences causing z to sometimes exceed z_ext
    // by a small amount here (see: https://trac.survex.com/ticket/26) and it
    // can certainly be true for passage tubes, so just clamp the value.
    if (z >= z_ext) return GetNumColourBands() - 1;
    return int(z / z_ext * (GetNumColourBands() - 1));
}

Double GfxCore::GetDepthBoundaryBetweenBands(int a, int b) const
{
    // Return the z-coordinate of the depth colour boundary between
    // two adjacent depth colour bands (specified by 0-based indices).

    assert((a == b - 1) || (a == b + 1));
    if (GetNumColourBands() == 1) return 0;

    int band = (a > b) ? a : b; // boundary N lies on the bottom of band N.
    Double z_ext = m_Parent->GetDepthExtent();
    return (z_ext * band / (GetNumColourBands() - 1)) + m_Parent->GetDepthMin();
}

void GfxCore::AddPolyline(const traverse & centreline)
{
    BeginPolyline();
    SetColour(col_WHITE);
    vector<PointInfo>::const_iterator i = centreline.begin();
    PlaceVertex(*i);
    ++i;
    while (i != centreline.end()) {
	PlaceVertex(*i);
	++i;
    }
    EndPolyline();
}

void GfxCore::AddPolylineShadow(const traverse & centreline)
{
    BeginPolyline();
    const double z = -0.5 * m_Parent->GetZExtent();
    vector<PointInfo>::const_iterator i = centreline.begin();
    PlaceVertex(i->GetX(), i->GetY(), z);
    ++i;
    while (i != centreline.end()) {
	PlaceVertex(i->GetX(), i->GetY(), z);
	++i;
    }
    EndPolyline();
}

void GfxCore::AddPolylineDepth(const traverse & centreline)
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
    glaTexCoord w(((b - a).magnitude() + (d - c).magnitude()) * .5);
    glaTexCoord h(((b - c).magnitude() + (d - a).magnitude()) * .5);
    // FIXME: should plot triangles instead to avoid rendering glitches.
    BeginQuadrilaterals();
    PlaceVertexWithColour(a, 0, 0, factor);
    PlaceVertexWithColour(b, w, 0, factor);
    PlaceVertexWithColour(c, w, h, factor);
    PlaceVertexWithColour(d, 0, h, factor);
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
    a_band = min(max(a_band, 0), GetNumColourBands());
    b_band = GetDepthColour(b.GetZ());
    b_band = min(max(b_band, 0), GetNumColourBands());
    c_band = GetDepthColour(c.GetZ());
    c_band = min(max(c_band, 0), GetNumColourBands());
    d_band = GetDepthColour(d.GetZ());
    d_band = min(max(d_band, 0), GetNumColourBands());
    glaTexCoord w(((b - a).magnitude() + (d - c).magnitude()) * .5);
    glaTexCoord h(((b - c).magnitude() + (d - a).magnitude()) * .5);
    int min_band = min(min(a_band, b_band), min(c_band, d_band));
    int max_band = max(max(a_band, b_band), max(c_band, d_band));
    if (min_band == max_band) {
	// Simple case - the polygon is entirely within one band.
	BeginPolygon();
////	PlaceNormal(normal);
	PlaceVertexWithDepthColour(a, 0, 0, factor);
	PlaceVertexWithDepthColour(b, w, 0, factor);
	PlaceVertexWithDepthColour(c, w, h, factor);
	PlaceVertexWithDepthColour(d, 0, h, factor);
	EndPolygon();
    } else {
	// We need to make a separate polygon for each depth band...
	vector<vector<Split> > splits;
	splits.resize(max_band + 1);
	splits[a_band].push_back(Split(a, 0, 0));
	if (a_band != b_band) {
	    SplitPolyAcrossBands(splits, a_band, b_band, a, b, 0, 0, w, 0);
	}
	splits[b_band].push_back(Split(b, w, 0));
	if (b_band != c_band) {
	    SplitPolyAcrossBands(splits, b_band, c_band, b, c, w, 0, 0, h);
	}
	splits[c_band].push_back(Split(c, w, h));
	if (c_band != d_band) {
	    SplitPolyAcrossBands(splits, c_band, d_band, c, d, w, h, -w, 0);
	}
	splits[d_band].push_back(Split(d, 0, h));
	if (d_band != a_band) {
	    SplitPolyAcrossBands(splits, d_band, a_band, d, a, 0, h, 0, -h);
	}
	for (int band = min_band; band <= max_band; ++band) {
	    BeginPolygon();
	    for (auto&& item : splits[band]) {
		PlaceVertexWithDepthColour(item.vec, item.tx, item.ty, factor);
	    }
	    EndPolygon();
	}
    }
}

void GfxCore::SetColourFromDate(int date, Double factor)
{
    // Set the drawing colour based on a date.

    if (date == -1) {
	// Undated.
	SetColour(col_WHITE, factor);
	return;
    }

    int date_offset = date - m_Parent->GetDateMin();
    if (date_offset == 0) {
	// Earliest date - handle as a special case for the single date case.
	SetColour(GetPen(0), factor);
	return;
    }

    int date_ext = m_Parent->GetDateExtent();
    Double how_far = (Double)date_offset / date_ext;
    assert(how_far >= 0.0);
    assert(how_far <= 1.0);
    SetColourFrom01(how_far, factor);
}

void GfxCore::AddPolylineDate(const traverse & centreline)
{
    BeginPolyline();
    vector<PointInfo>::const_iterator i, prev_i;
    i = centreline.begin();
    int date = i->GetDate();
    SetColourFromDate(date, 1.0);
    PlaceVertex(*i);
    prev_i = i;
    while (++i != centreline.end()) {
	int newdate = i->GetDate();
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

static int static_date_hack; // FIXME

void GfxCore::AddQuadrilateralDate(const Vector3 &a, const Vector3 &b,
				   const Vector3 &c, const Vector3 &d)
{
    Vector3 normal = (a - c) * (d - b);
    normal.normalise();
    Double factor = dot(normal, light) * .3 + .7;
    glaTexCoord w(((b - a).magnitude() + (d - c).magnitude()) * .5);
    glaTexCoord h(((b - c).magnitude() + (d - a).magnitude()) * .5);
    // FIXME: should plot triangles instead to avoid rendering glitches.
    BeginQuadrilaterals();
////    PlaceNormal(normal);
    SetColourFromDate(static_date_hack, factor);
    PlaceVertex(a, 0, 0);
    PlaceVertex(b, w, 0);
    PlaceVertex(c, w, h);
    PlaceVertex(d, 0, h);
    EndQuadrilaterals();
}

static double static_E_hack; // FIXME

void GfxCore::SetColourFromError(double E, Double factor)
{
    // Set the drawing colour based on an error value.

    if (E < 0) {
	SetColour(col_WHITE, factor);
	return;
    }

    Double how_far = E / MAX_ERROR;
    assert(how_far >= 0.0);
    if (how_far > 1.0) how_far = 1.0;
    SetColourFrom01(how_far, factor);
}

void GfxCore::AddQuadrilateralError(const Vector3 &a, const Vector3 &b,
				    const Vector3 &c, const Vector3 &d)
{
    Vector3 normal = (a - c) * (d - b);
    normal.normalise();
    Double factor = dot(normal, light) * .3 + .7;
    glaTexCoord w(((b - a).magnitude() + (d - c).magnitude()) * .5);
    glaTexCoord h(((b - c).magnitude() + (d - a).magnitude()) * .5);
    // FIXME: should plot triangles instead to avoid rendering glitches.
    BeginQuadrilaterals();
////    PlaceNormal(normal);
    SetColourFromError(static_E_hack, factor);
    PlaceVertex(a, 0, 0);
    PlaceVertex(b, w, 0);
    PlaceVertex(c, w, h);
    PlaceVertex(d, 0, h);
    EndQuadrilaterals();
}

void GfxCore::AddPolylineError(const traverse & centreline)
{
    BeginPolyline();
    SetColourFromError(centreline.E, 1.0);
    vector<PointInfo>::const_iterator i;
    for(i = centreline.begin(); i != centreline.end(); ++i) {
	PlaceVertex(*i);
    }
    EndPolyline();
}

// gradient is in *radians*.
void GfxCore::SetColourFromGradient(double gradient, Double factor)
{
    // Set the drawing colour based on the gradient of the leg.

    const Double GRADIENT_MAX = M_PI_2;
    gradient = fabs(gradient);
    Double how_far = gradient / GRADIENT_MAX;
    SetColourFrom01(how_far, factor);
}

void GfxCore::AddPolylineGradient(const traverse & centreline)
{
    vector<PointInfo>::const_iterator i, prev_i;
    i = centreline.begin();
    prev_i = i;
    while (++i != centreline.end()) {
	BeginPolyline();
	SetColourFromGradient((*i - *prev_i).gradient(), 1.0);
	PlaceVertex(*prev_i);
	PlaceVertex(*i);
	prev_i = i;
	EndPolyline();
    }
}

static double static_gradient_hack; // FIXME

void GfxCore::AddQuadrilateralGradient(const Vector3 &a, const Vector3 &b,
				       const Vector3 &c, const Vector3 &d)
{
    Vector3 normal = (a - c) * (d - b);
    normal.normalise();
    Double factor = dot(normal, light) * .3 + .7;
    glaTexCoord w(((b - a).magnitude() + (d - c).magnitude()) * .5);
    glaTexCoord h(((b - c).magnitude() + (d - a).magnitude()) * .5);
    // FIXME: should plot triangles instead to avoid rendering glitches.
    BeginQuadrilaterals();
////    PlaceNormal(normal);
    SetColourFromGradient(static_gradient_hack, factor);
    PlaceVertex(a, 0, 0);
    PlaceVertex(b, w, 0);
    PlaceVertex(c, w, h);
    PlaceVertex(d, 0, h);
    EndQuadrilaterals();
}

void GfxCore::SetColourFromLength(double length, Double factor)
{
    // Set the drawing colour based on log(length_of_leg).

    Double log_len = log10(length);
    Double how_far = log_len / LOG_LEN_MAX;
    how_far = max(how_far, 0.0);
    how_far = min(how_far, 1.0);
    SetColourFrom01(how_far, factor);
}

void GfxCore::SetColourFrom01(double how_far, Double factor)
{
    double b;
    double into_band = modf(how_far * (GetNumColourBands() - 1), &b);
    int band(b);
    GLAPen pen1 = GetPen(band);
    // With 24bit colour, interpolating by less than this can have no effect.
    if (into_band >= 1.0 / 512.0) {
	const GLAPen& pen2 = GetPen(band + 1);
	pen1.Interpolate(pen2, into_band);
    }
    SetColour(pen1, factor);
}

void GfxCore::AddPolylineLength(const traverse & centreline)
{
    vector<PointInfo>::const_iterator i, prev_i;
    i = centreline.begin();
    prev_i = i;
    while (++i != centreline.end()) {
	BeginPolyline();
	SetColourFromLength((*i - *prev_i).magnitude(), 1.0);
	PlaceVertex(*prev_i);
	PlaceVertex(*i);
	prev_i = i;
	EndPolyline();
    }
}

static double static_length_hack; // FIXME

void GfxCore::AddQuadrilateralLength(const Vector3 &a, const Vector3 &b,
				     const Vector3 &c, const Vector3 &d)
{
    Vector3 normal = (a - c) * (d - b);
    normal.normalise();
    Double factor = dot(normal, light) * .3 + .7;
    glaTexCoord w(((b - a).magnitude() + (d - c).magnitude()) * .5);
    glaTexCoord h(((b - c).magnitude() + (d - a).magnitude()) * .5);
    // FIXME: should plot triangles instead to avoid rendering glitches.
    BeginQuadrilaterals();
////    PlaceNormal(normal);
    SetColourFromLength(static_length_hack, factor);
    PlaceVertex(a, 0, 0);
    PlaceVertex(b, w, 0);
    PlaceVertex(c, w, h);
    PlaceVertex(d, 0, h);
    EndQuadrilaterals();
}

void
GfxCore::SkinPassage(vector<XSect> & centreline, bool draw)
{
    assert(centreline.size() > 1);
    Vector3 U[4];
    XSect prev_pt_v;
    Vector3 last_right(1.0, 0.0, 0.0);

//  FIXME: it's not simple to set the colour of a tube based on error...
//    static_E_hack = something...
    vector<XSect>::iterator i = centreline.begin();
    vector<XSect>::size_type segment = 0;
    while (i != centreline.end()) {
	// get the coordinates of this vertex
	XSect & pt_v = *i++;

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
	    } else {
		up = up_v;
	    }
	    last_right = right;
	    static_date_hack = pt_v.GetDate();
	}

	// Scale to unit vectors in the LRUD plane.
	right.normalise();
	up.normalise();

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

	if (draw) {
	    const Vector3 & delta = pt_v - prev_pt_v;
	    static_length_hack = delta.magnitude();
	    static_gradient_hack = delta.gradient();
	    if (segment > 0) {
		(this->*AddQuad)(v[0], v[1], U[1], U[0]);
		(this->*AddQuad)(v[2], v[3], U[3], U[2]);
		(this->*AddQuad)(v[1], v[2], U[2], U[1]);
		(this->*AddQuad)(v[3], v[0], U[0], U[3]);
	    }

	    if (cover_end) {
		if (segment == 0) {
		    (this->*AddQuad)(v[0], v[1], v[2], v[3]);
		} else {
		    (this->*AddQuad)(v[3], v[2], v[1], v[0]);
		}
	    }
	}

	prev_pt_v = pt_v;
	U[0] = v[0];
	U[1] = v[1];
	U[2] = v[2];
	U[3] = v[3];

	pt_v.set_right_bearing(deg(atan2(right.GetY(), right.GetX())));

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

bool GfxCore::FullScreenModeShowingMenus() const
{
    return m_Parent->FullScreenModeShowingMenus();
}

void GfxCore::FullScreenModeShowMenus(bool show)
{
    m_Parent->FullScreenModeShowMenus(show);
}

void
GfxCore::MoveViewer(double forward, double up, double right)
{
    double cT = cos(rad(m_TiltAngle));
    double sT = sin(rad(m_TiltAngle));
    double cP = cos(rad(m_PanAngle));
    double sP = sin(rad(m_PanAngle));
    Vector3 v_forward(cT * sP, cT * cP, sT);
    Vector3 v_up(sT * sP, sT * cP, -cT);
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
			    m_PanAngle, -m_TiltAngle, m_Scale);
}

void GfxCore::SetView(const PresentationMark & p)
{
    m_SwitchingTo = 0;
    SetTranslation(p - m_Parent->GetOffset());
    m_PanAngle = p.angle;
    m_TiltAngle = -p.tilt_angle; // FIXME: nasty reversed sense (and above)
    SetRotation(m_PanAngle, m_TiltAngle);
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
	case COLOUR_BY_ERROR:
	    AddQuad = &GfxCore::AddQuadrilateralError;
	    AddPoly = &GfxCore::AddPolylineError;
	    break;
	case COLOUR_BY_GRADIENT:
	    AddQuad = &GfxCore::AddQuadrilateralGradient;
	    AddPoly = &GfxCore::AddPolylineGradient;
	    break;
	case COLOUR_BY_LENGTH:
	    AddQuad = &GfxCore::AddQuadrilateralLength;
	    AddPoly = &GfxCore::AddPolylineLength;
	    break;
	default: // case COLOUR_BY_NONE:
	    AddQuad = &GfxCore::AddQuadrilateral;
	    AddPoly = &GfxCore::AddPolyline;
	    break;
    }

    InvalidateList(LIST_UNDERGROUND_LEGS);
    InvalidateList(LIST_SURFACE_LEGS);
    InvalidateList(LIST_TUBES);

    ForceRefresh();
}

bool GfxCore::ExportMovie(const wxString & fnm)
{
    FILE* fh = wxFopen(fnm.fn_str(), wxT("wb"));
    if (fh == NULL) {
	wxGetApp().ReportError(wxString::Format(wmsg(/*Failed to open output file “%s”*/47), fnm.c_str()));
	return false;
    }

    wxString ext;
    wxFileName::SplitPath(fnm, NULL, NULL, NULL, &ext, wxPATH_NATIVE);

    int width;
    int height;
    GetSize(&width, &height);
    // Round up to next multiple of 2 (required by ffmpeg).
    width += (width & 1);
    height += (height & 1);

    movie = new MovieMaker();

    // movie takes ownership of fh.
    if (!movie->Open(fh, ext.utf8_str(), width, height)) {
	wxGetApp().ReportError(wxString(movie->get_error_string(), wxConvUTF8));
	delete movie;
	movie = NULL;
	return false;
    }

    PlayPres(1);
    return true;
}

void
GfxCore::OnPrint(const wxString &filename, const wxString &title,
		 const wxString &datestamp, time_t datestamp_numeric,
		 const wxString &cs_proj,
		 bool close_after_print)
{
    svxPrintDlg * p;
    p = new svxPrintDlg(m_Parent, filename, title, cs_proj,
			datestamp, datestamp_numeric,
			m_PanAngle, m_TiltAngle,
			m_Names, m_Crosses, m_Legs, m_Surface, m_Splays,
			m_Tubes, m_Entrances, m_FixedPts, m_ExportedPts,
			true, close_after_print);
    p->Show(true);
}

void
GfxCore::OnExport(const wxString &filename, const wxString &title,
		  const wxString &datestamp, time_t datestamp_numeric,
		  const wxString &cs_proj)
{
    // Fill in "right_bearing" for each cross-section.
    list<vector<XSect> >::iterator trav = m_Parent->tubes_begin();
    list<vector<XSect> >::iterator tend = m_Parent->tubes_end();
    while (trav != tend) {
	SkinPassage(*trav, false);
	++trav;
    }

    svxPrintDlg * p;
    p = new svxPrintDlg(m_Parent, filename, title, cs_proj,
			datestamp, datestamp_numeric,
			m_PanAngle, m_TiltAngle,
			m_Names, m_Crosses, m_Legs, m_Surface, m_Splays,
			m_Tubes, m_Entrances, m_FixedPts, m_ExportedPts,
			false);
    p->Show(true);
}

static wxCursor
make_cursor(const unsigned char * bits, const unsigned char * mask,
	    int hotx, int hoty)
{
#if defined __WXGTK__ && !defined __WXGTK3__
    // Use this code for GTK < 3 only - it doesn't work properly with GTK3
    // (reported and should be fixed in wxWidgets 3.0.4 and 3.1.1, see:
    // https://trac.wxwidgets.org/ticket/17916)
    return wxCursor((const char *)bits, 32, 32, hotx, hoty,
		    (const char *)mask, wxBLACK, wxWHITE);
#else
# ifdef __WXMAC__
    // The default Mac cursor is black with a white edge, so
    // invert our custom cursors to match.
    char b[128];
    for (int i = 0; i < 128; ++i)
	b[i] = bits[i] ^ 0xff;
# else
    const char * b = reinterpret_cast<const char *>(bits);
# endif
    wxBitmap cursor_bitmap(b, 32, 32);
    wxBitmap mask_bitmap(reinterpret_cast<const char *>(mask), 32, 32);
    cursor_bitmap.SetMask(new wxMask(mask_bitmap, *wxWHITE));
    wxImage cursor_image = cursor_bitmap.ConvertToImage();
    cursor_image.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, hotx);
    cursor_image.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, hoty);
    return wxCursor(cursor_image);
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
GfxCore::UpdateCursor(GfxCore::cursor new_cursor)
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

bool GfxCore::MeasuringLineActive() const
{
    if (Animating()) return false;
    return HereIsReal() || m_there;
}

bool GfxCore::HandleRClick(wxPoint point)
{
    if (PointWithinCompass(point)) {
	// Pop up menu.
	wxMenu menu;
	/* TRANSLATORS: View *looking* North */
	menu.Append(menu_ORIENT_MOVE_NORTH, wmsg(/*View &North*/240));
	/* TRANSLATORS: View *looking* East */
	menu.Append(menu_ORIENT_MOVE_EAST, wmsg(/*View &East*/241));
	/* TRANSLATORS: View *looking* South */
	menu.Append(menu_ORIENT_MOVE_SOUTH, wmsg(/*View &South*/242));
	/* TRANSLATORS: View *looking* West */
	menu.Append(menu_ORIENT_MOVE_WEST, wmsg(/*View &West*/243));
	menu.AppendSeparator();
	/* TRANSLATORS: Menu item which turns off the "north arrow" in aven. */
	menu.AppendCheckItem(menu_IND_COMPASS, wmsg(/*&Hide Compass*/387));
	/* TRANSLATORS: tickable menu item in View menu.
	 *
	 * Degrees are the angular measurement where there are 360 in a full
	 * circle. */
	menu.AppendCheckItem(menu_CTL_DEGREES, wmsg(/*&Degrees*/343));
	menu.Connect(wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&wxEvtHandler::ProcessEvent, NULL, m_Parent->GetEventHandler());
	PopupMenu(&menu);
	return true;
    }

    if (PointWithinClino(point)) {
	// Pop up menu.
	wxMenu menu;
	menu.Append(menu_ORIENT_PLAN, wmsg(/*&Plan View*/248));
	menu.Append(menu_ORIENT_ELEVATION, wmsg(/*Ele&vation*/249));
	menu.AppendSeparator();
	/* TRANSLATORS: Menu item which turns off the tilt indicator in aven. */
	menu.AppendCheckItem(menu_IND_CLINO, wmsg(/*&Hide Clino*/384));
	/* TRANSLATORS: tickable menu item in View menu.
	 *
	 * Degrees are the angular measurement where there are 360 in a full
	 * circle. */
	menu.AppendCheckItem(menu_CTL_DEGREES, wmsg(/*&Degrees*/343));
	/* TRANSLATORS: tickable menu item in View menu.
	 *
	 * Show the tilt of the survey as a percentage gradient (100% = 45
	 * degrees = 50 grad). */
	menu.AppendCheckItem(menu_CTL_PERCENT, wmsg(/*&Percent*/430));
	menu.Connect(wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&wxEvtHandler::ProcessEvent, NULL, m_Parent->GetEventHandler());
	PopupMenu(&menu);
	return true;
    }

    if (PointWithinScaleBar(point)) {
	// Pop up menu.
	wxMenu menu;
	/* TRANSLATORS: Menu item which turns off the scale bar in aven. */
	menu.AppendCheckItem(menu_IND_SCALE_BAR, wmsg(/*&Hide scale bar*/385));
	/* TRANSLATORS: tickable menu item in View menu.
	 *
	 * "Metric" here means metres, km, etc (rather than feet, miles, etc)
	 */
	menu.AppendCheckItem(menu_CTL_METRIC, wmsg(/*&Metric*/342));
	menu.Connect(wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&wxEvtHandler::ProcessEvent, NULL, m_Parent->GetEventHandler());
	PopupMenu(&menu);
	return true;
    }

    if (PointWithinColourKey(point)) {
	// Pop up menu.
	wxMenu menu;
	menu.AppendCheckItem(menu_COLOUR_BY_DEPTH, wmsg(/*Colour by &Depth*/292));
	menu.AppendCheckItem(menu_COLOUR_BY_DATE, wmsg(/*Colour by D&ate*/293));
	menu.AppendCheckItem(menu_COLOUR_BY_ERROR, wmsg(/*Colour by &Error*/289));
	menu.AppendCheckItem(menu_COLOUR_BY_GRADIENT, wmsg(/*Colour by &Gradient*/85));
	menu.AppendCheckItem(menu_COLOUR_BY_LENGTH, wmsg(/*Colour by &Length*/82));
	menu.AppendSeparator();
	/* TRANSLATORS: Menu item which turns off the colour key.
	 * The "Colour Key" is the thing in aven showing which colour
	 * corresponds to which depth, date, survey closure error, etc. */
	menu.AppendCheckItem(menu_IND_COLOUR_KEY, wmsg(/*&Hide colour key*/386));
	if (m_ColourBy == COLOUR_BY_DEPTH || m_ColourBy == COLOUR_BY_LENGTH)
	    menu.AppendCheckItem(menu_CTL_METRIC, wmsg(/*&Metric*/342));
	else if (m_ColourBy == COLOUR_BY_GRADIENT)
	    menu.AppendCheckItem(menu_CTL_DEGREES, wmsg(/*&Degrees*/343));
	menu.Connect(wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&wxEvtHandler::ProcessEvent, NULL, m_Parent->GetEventHandler());
	PopupMenu(&menu);
	return true;
    }

    return false;
}

void GfxCore::SetZoomBox(wxPoint p1, wxPoint p2, bool centred, bool aspect)
{
    if (centred) {
	p1.x = p2.x + (p1.x - p2.x) * 2;
	p1.y = p2.y + (p1.y - p2.y) * 2;
    }
    if (aspect) {
#if 0 // FIXME: This needs more work.
	int sx = GetXSize();
	int sy = GetYSize();
	int dx = p1.x - p2.x;
	int dy = p1.y - p2.y;
	int dy_new = dx * sy / sx;
	if (abs(dy_new) >= abs(dy)) {
	    p1.y += (dy_new - dy) / 2;
	    p2.y -= (dy_new - dy) / 2;
	} else {
	    int dx_new = dy * sx / sy;
	    p1.x += (dx_new - dx) / 2;
	    p2.x -= (dx_new - dx) / 2;
	}
#endif
    }
    zoombox.set(p1, p2);
    ForceRefresh();
}

void GfxCore::ZoomBoxGo()
{
    if (!zoombox.active()) return;

    int width = GetXSize();
    int height = GetYSize();

    TranslateCave(-0.5 * (zoombox.x1 + zoombox.x2 - width),
		  -0.5 * (zoombox.y1 + zoombox.y2 - height));
    int box_w = abs(zoombox.x1 - zoombox.x2);
    int box_h = abs(zoombox.y1 - zoombox.y2);

    double factor = min(double(width) / box_w, double(height) / box_h);

    zoombox.unset();

    SetScale(GetScale() * factor);
}
