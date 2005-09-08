//
//  gfxcore.cc
//
//  Core drawing code for Aven.
//
//  Copyright (C) 2000-2003 Mark R. Shinwell
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

//#define DrawList(L) do { printf("DrawList(%s)\n", #L); (DrawList)(L); } while (0)

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAX3(a, b, c) ((a) > (b) ? MAX(a, c) : MAX(b, c))

// Values for m_SwitchingTo
#define PLAN 1
#define ELEVATION 2

// How many bins per letter height to use when working out non-overlapping
// labels.
const unsigned int QUANTISE_FACTOR = 2;

const int NUM_DEPTH_COLOURS = 13;

#include "avenpal.h"

#ifdef _WIN32
static const int FONT_SIZE = 8;
#else
static const int FONT_SIZE = 9;
#endif
static const int CROSS_SIZE = 3;
static const int COMPASS_OFFSET_X = 60;
static const int COMPASS_OFFSET_Y = 80;
static const int INDICATOR_BOX_SIZE = 60;
static const int INDICATOR_GAP = 2;
static const int INDICATOR_MARGIN = 5;
static const int INDICATOR_OFFSET_X = 15;
static const int INDICATOR_OFFSET_Y = 15;
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
    EVT_SIZE(GfxCore::OnSize)
    EVT_IDLE(GfxCore::OnIdle)
    EVT_CHAR(GfxCore::OnKeyPress)
END_EVENT_TABLE()

GfxCore::GfxCore(MainFrm* parent, wxWindow* parent_win, GUIControl* control) :
    GLACanvas(parent_win, 100, wxDefaultPosition, wxSize(640, 480)),
    m_Font(FONT_SIZE, wxSWISS, wxNORMAL, wxNORMAL, FALSE, "Helvetica",
	   wxFONTENCODING_ISO8859_1),
    m_HaveData(false)
{
    m_Control = control;
    m_ScaleBarWidth = 0;
    m_Parent = parent;
    m_RotationOK = true;
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
    m_ColourBy = COLOUR_BY_DEPTH;
    AddQuad = &GfxCore::AddQuadrilateralDepth;
    AddPoly = &GfxCore::AddPolylineDepth;
    wxConfigBase::Get()->Read("metric", &m_Metric, true);
    wxConfigBase::Get()->Read("degrees", &m_Degrees, true);
    m_here.x = DBL_MAX;
    m_there.x = DBL_MAX;
    m_Lists.underground_legs = 0;
    m_Lists.tubes = 0;
    m_Lists.surface_legs = 0;
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
    m_here.x = DBL_MAX;
    m_there.x = DBL_MAX;

    // Check for flat/linear/point surveys.
    m_RotationOK = true;

    m_Lock = lock_NONE;
    if (m_Parent->GetXExtent() == 0.0) m_Lock = LockFlags(m_Lock | lock_X);
    if (m_Parent->GetYExtent() == 0.0) m_Lock = LockFlags(m_Lock | lock_Y);
    if (m_Parent->GetZExtent() == 0.0) m_Lock = LockFlags(m_Lock | lock_Z);

    // Apply default parameters.
    DefaultParameters();

    m_HaveData = true;
}

void GfxCore::FirstShow()
{
    GLACanvas::FirstShow();

    const unsigned int quantise(GLACanvas::GetFontSize() / QUANTISE_FACTOR);
    list<LabelInfo*>::iterator pos = m_Parent->GetLabelsNC();
    while (pos != m_Parent->GetLabelsNCEnd()) {
	LabelInfo* label = *pos++;
	// Calculate and set the label width for use when plotting
	// none-overlapping labels.
	int ext_x;
	GLACanvas::GetTextExtent(label->GetText(), &ext_x, NULL);
	label->width = unsigned(ext_x) / quantise + 1;
    }

    SetBackgroundColour(0.0, 0.0, 0.0);

    // Set diameter of the viewing volume.
    SetVolumeDiameter(sqrt(sqrd(m_Parent->GetXExtent()) +
			   sqrd(m_Parent->GetYExtent()) +
			   sqrd(m_Parent->GetZExtent())));

    // Update our record of the client area size and centre.
    GetClientSize(&m_XSize, &m_YSize);
    m_XCentre = m_XSize / 2;
    m_YCentre = m_YSize / 2;

    switch (m_Lock) {
	case lock_POINT:
	case lock_XY:
	case lock_X:
	case lock_Y:
	    m_RotationOK = false;
	    break;
	case lock_YZ:
	case lock_XZ:
	default:
	    break;
    }

    // Set the initial scale.
    m_InitialScale = 1.0;
    SetScale(m_InitialScale);

    m_DoneFirstShow = true;
}

//
//  Recalculating methods
//

void GfxCore::SetScale(Double scale)
{
    // Fill the plot data arrays with screen coordinates, scaling the survey
    // to a particular absolute scale.

    if (scale < m_InitialScale / 20) {
	scale = m_InitialScale / 20;
    } else {
	if (scale > 1000.0) scale = 1000.0;
    }

    m_Params.scale = scale;
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

void GfxCore::UpdateBlobs()
{
    DeleteList(m_Lists.blobs);
    m_Lists.blobs = CreateList(this, &GfxCore::GenerateBlobsDisplayList);
}

//
//  Event handlers
//

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

	if (first_time) {
	    m_Lists.underground_legs =
		CreateList(this, &GfxCore::GenerateDisplayList);

	    m_Lists.tubes =
		CreateList(this, &GfxCore::GenerateDisplayListTubes);

	    m_Lists.surface_legs =
		CreateList(this, &GfxCore::GenerateDisplayListSurface);

	    m_Lists.blobs =
		CreateList(this, &GfxCore::GenerateBlobsDisplayList);
	}

	timer.Start(); // reset timer

	if (m_Legs || m_Tubes) {
	    if (m_Tubes) {
		EnableSmoothPolygons();
		DrawList(m_Lists.tubes);
		DisableSmoothPolygons();
	    }

	    // Draw the underground legs (draw them last so that anti-aliasing
	    // works over polygons.
	    SetColour(col_GREEN);
	    DrawList(m_Lists.underground_legs);
	}

	if (m_Surface) {
	    // Draw the surface legs.
	    DrawList(m_Lists.surface_legs);
	}

	DrawList(m_Lists.blobs);

	if (m_Grid) {
	    // Draw the grid.
	    // FIXME: draw grid as opengl list?  tracking when to invalidate
	    // it requires care...
	    // DrawList(m_Lists.grid);
	    DrawGrid();
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

	if (m_Crosses) {
	    // Plot crosses.  To get the crosses to appear at a constant
	    // size and orientation on screen, we plot them in the Indicator
	    // transform coordinates (and this means we can't sensibly put
	    // them in an opengl display list).
	    list<LabelInfo*>::const_iterator pos = m_Parent->GetLabels();
	    SetColour(col_LIGHT_GREY);
	    BeginLines();
	    while (pos != m_Parent->GetLabelsEnd()) {
		const LabelInfo* label = *pos++;

		if (!((m_Surface && label->IsSurface()) ||
			    (m_Legs && label->IsUnderground()) ||
			    (!label->IsSurface() && !label->IsUnderground()))) {
		    // if this station isn't to be displayed, skip to the next
		    // (last case is for stns with no legs attached)
		    continue;
		}

		Double x, y, z;
		if (!Transform(label->GetX(), label->GetY(), label->GetZ(),
			       &x, &y, &z)) {
		    printf("bad transform on: %s\n", label->GetText().c_str());
		    continue;
		}
		// Stuff in behind us (in perspective view) will get clipped,
		// but we can save effort with a cheap check here.
		if (z > 0) {
		    // Round to integers before adding on the offsets for the
		    // cross arms to avoid uneven crosses.
		    x = rint(x);
		    y = rint(y);
		    PlaceVertex(x - 2, y - 2, z);
		    PlaceVertex(x + 2, y + 2, z);
		    PlaceVertex(x - 2, y + 2, z);
		    PlaceVertex(x + 2, y - 2, z);
		}
	    }
	    EndLines();
	}

	if (!Animating() && (m_here.x != DBL_MAX || m_there.x != DBL_MAX)) {
	    // Draw "here" and "there".
	    Double hx, hy;
	    SetColour(HERE_COLOUR);
	    if (m_here.x != DBL_MAX) {
		Double dummy;
		Transform(m_here.x, m_here.y, m_here.z, &hx, &hy, &dummy);
		DrawRing(hx, hy);
	    }
	    if (m_there.x != DBL_MAX) {
		Double tx, ty;
		Double dummy;
		Transform(m_there.x, m_there.y, m_there.z, &tx, &ty, &dummy);
		if (m_here.x != DBL_MAX) {
		    BeginLines();
		    PlaceIndicatorVertex(hx, hy);
		    PlaceIndicatorVertex(tx, ty);
		    EndLines();
		}
		BeginBlobs();
		DrawBlob(tx, ty, 0.0);
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
    }
}

Double GfxCore::GridXToScreen(Double x, Double y, Double z) const
{
    x = x; y = y; z = z;
    return 0.0;

    /*
    x += m_Params.translation.x;
    y += m_Params.translation.y;
    z += m_Params.translation.z;

    return (XToScreen(x, y, z) * m_Params.scale) + m_XCentre;*/
}

Double GfxCore::GridYToScreen(Double x, Double y, Double z) const
{
    x = x; y = y; z = z;
    return 0.0;

    /*
    x += m_Params.translation.x;
    y += m_Params.translation.y;
    z += m_Params.translation.z;

    return m_YCentre - ((ZToScreen(x, y, z) * m_Params.scale));*/
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

    Double grid_size = size_snap / 10.0;
    Double edge = grid_size * 2.0;
    Double grid_z = -m_Parent->GetZExtent()/2.0 - grid_size;
    Double left = -m_Parent->GetXExtent()/2.0 - edge;
    Double right = m_Parent->GetXExtent()/2.0 + edge;
    Double bottom = -m_Parent->GetYExtent()/2.0 - edge;
    Double top = m_Parent->GetYExtent()/2.0 + edge;
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

wxPoint GfxCore::CompassPtToScreen(Double x, Double y, Double z) const
{
    x = x; y = y; z = z;
    /*return wxPoint(long(-XToScreen(x, y, z)) + m_XSize - COMPASS_OFFSET_X,
		   long(ZToScreen(x, y, z)) + m_YSize - COMPASS_OFFSET_Y);*/

    return wxPoint(0, 0);
}

GLAPoint GfxCore::IndicatorCompassToScreenPan(int angle) const
{
    Double theta = rad(angle + m_PanAngle);
    glaCoord length = (INDICATOR_BOX_SIZE - INDICATOR_MARGIN*2) / 2;
    glaCoord x = glaCoord(length * sin(theta));
    glaCoord y = glaCoord(length * cos(theta));

    return GLAPoint(m_XSize - INDICATOR_OFFSET_X - INDICATOR_BOX_SIZE/2 - x,
		    INDICATOR_OFFSET_Y + INDICATOR_BOX_SIZE/2 + y, 0.0);
}

GLAPoint GfxCore::IndicatorCompassToScreenElev(int angle) const
{
    Double theta = rad(angle + m_TiltAngle + 90.0);
    glaCoord length = (INDICATOR_BOX_SIZE - INDICATOR_MARGIN*2) / 2;
    glaCoord x = glaCoord(length * sin(-theta));
    glaCoord y = glaCoord(length * cos(-theta));

    return GLAPoint(m_XSize - GetClinoOffset() - INDICATOR_BOX_SIZE/2 - x,
		    INDICATOR_OFFSET_Y + INDICATOR_BOX_SIZE/2 + y, 0.0);
}

void GfxCore::DrawTick(wxCoord cx, wxCoord cy, int angle_cw)
{
    Double theta = rad(angle_cw);
    wxCoord length1 = (INDICATOR_BOX_SIZE - INDICATOR_MARGIN*2) / 2;
    wxCoord length0 = length1 + TICK_LENGTH;
    wxCoord x0 = wxCoord(length0 * sin(theta));
    wxCoord y0 = wxCoord(length0 * -cos(theta));
    wxCoord x1 = wxCoord(length1 * sin(theta));
    wxCoord y1 = wxCoord(length1 * -cos(theta));

    BeginLines();
    PlaceIndicatorVertex(cx + x0, cy - y0);
    PlaceIndicatorVertex(cx + x1, cy - y1);
    EndLines();
}

void GfxCore::Draw2dIndicators()
{
    // Draw the "traditional" elevation and compass indicators.

    // Indicator backgrounds.
    if (m_Compass && m_RotationOK) {
	DrawCircle(col_LIGHT_GREY_2, col_GREY,
		   m_XSize - INDICATOR_OFFSET_X - INDICATOR_BOX_SIZE + INDICATOR_MARGIN,
		   INDICATOR_OFFSET_Y + INDICATOR_BOX_SIZE - INDICATOR_MARGIN,
		   INDICATOR_BOX_SIZE / 2 - INDICATOR_MARGIN);
    }
    if (m_Clino && m_Lock == lock_NONE) {
	glaCoord tilt = (glaCoord)m_TiltAngle;
	glaCoord start = tilt + 90;
	DrawSemicircle(col_LIGHT_GREY_2, col_GREY,
		       m_XSize - GetClinoOffset() - INDICATOR_BOX_SIZE + INDICATOR_MARGIN,
		       INDICATOR_OFFSET_Y + INDICATOR_BOX_SIZE - INDICATOR_MARGIN,
		       INDICATOR_BOX_SIZE / 2 - INDICATOR_MARGIN, start);

	SetColour(col_GREY);
	BeginLines();
	PlaceIndicatorVertex(m_XSize - GetClinoOffset() - INDICATOR_BOX_SIZE/2,
			     INDICATOR_OFFSET_Y + INDICATOR_MARGIN);

	PlaceIndicatorVertex(m_XSize - GetClinoOffset() - INDICATOR_BOX_SIZE/2,
			     INDICATOR_OFFSET_Y + INDICATOR_BOX_SIZE - INDICATOR_MARGIN);

	PlaceIndicatorVertex(m_XSize - GetClinoOffset() - INDICATOR_BOX_SIZE/2,
			     INDICATOR_OFFSET_Y + INDICATOR_BOX_SIZE/2);

	PlaceIndicatorVertex(m_XSize - GetClinoOffset() - INDICATOR_MARGIN,
			     INDICATOR_OFFSET_Y + INDICATOR_BOX_SIZE/2);
	EndLines();
    }


    // Ticks
    bool white = false;
    /* FIXME m_Control->DraggingMouseOutsideCompass();
    m_DraggingLeft && m_LastDrag == drag_COMPASS && m_MouseOutsideCompass; */
    int pan_centre_x = m_XSize - INDICATOR_BOX_SIZE / 2 - INDICATOR_OFFSET_X;
    int centre_y = INDICATOR_BOX_SIZE / 2 + INDICATOR_OFFSET_Y;
    int elev_centre_x = m_XSize - INDICATOR_BOX_SIZE / 2 - GetClinoOffset();

    if (m_Compass && m_RotationOK) {
	int deg_pan = (int) m_PanAngle;
	//--FIXME: bodge by Olly to stop wrong tick highlighting
	if (deg_pan) deg_pan = 360 - deg_pan;
	for (int angle = deg_pan; angle <= 315 + deg_pan; angle += 45) {
	    if (deg_pan == angle) {
		SetColour(col_GREEN);
	    } else {
		SetColour(white ? col_WHITE : col_LIGHT_GREY_2);
	    }
	    DrawTick((int) pan_centre_x, (int) centre_y, angle);
	}
    }

    if (m_Clino && m_Lock == lock_NONE) {
	white = false; /* FIXME m_DraggingLeft && m_LastDrag == drag_ELEV && m_MouseOutsideElev; */
	int deg_elev = int(m_TiltAngle);
	for (int angle = 0; angle <= 180; angle += 90) {
	    if (deg_elev == angle - 90) {
		SetColour(col_GREEN);
	    } else {
		SetColour(white ? col_WHITE : col_LIGHT_GREY_2);
	    }
	    DrawTick((int) elev_centre_x, (int) centre_y, angle);
	}
    }

    // Pan arrow
    if (m_Compass && m_RotationOK) {
	GLAPoint p1 = IndicatorCompassToScreenPan(0);
	GLAPoint p2 = IndicatorCompassToScreenPan(150);
	GLAPoint p3 = IndicatorCompassToScreenPan(210);
	GLAPoint pc(pan_centre_x, centre_y, 0.0);
	GLAPoint pts1[3] = { p2, p1, pc };
	GLAPoint pts2[3] = { p3, p1, pc };

	DrawTriangle(col_LIGHT_GREY, col_INDICATOR_1, pts1);
	DrawTriangle(col_LIGHT_GREY, col_INDICATOR_2, pts2);
    }

    // Elevation arrow
    if (m_Clino && m_Lock == lock_NONE) {
	GLAPoint p1e = IndicatorCompassToScreenElev(0);
	GLAPoint p2e = IndicatorCompassToScreenElev(150);
	GLAPoint p3e = IndicatorCompassToScreenElev(210);
	GLAPoint pce(elev_centre_x, centre_y, 0.0);
	GLAPoint pts1e[3] = { p2e, p1e, pce };
	GLAPoint pts2e[3] = { p3e, p1e, pce };
	DrawTriangle(col_LIGHT_GREY, col_INDICATOR_2, pts1e);
	DrawTriangle(col_LIGHT_GREY, col_INDICATOR_1, pts2e);
    }

    // Text
    SetColour(TEXT_COLOUR);

    int w, h;
    int width, height;
    wxString str;

    GetTextExtent(wxString("000"), &width, &h);
    height = INDICATOR_OFFSET_Y + INDICATOR_BOX_SIZE + INDICATOR_GAP + h;

    if (m_Compass && m_RotationOK) {
	if (m_Degrees) {
	    str = wxString::Format("%03d", int(m_PanAngle));
	} else {
	    str = wxString::Format("%03d", int(m_PanAngle * 200.0 / 180.0));
	}
	GetTextExtent(str, &w, NULL);
	DrawIndicatorText(pan_centre_x + width / 2 - w, height, str);
	str = wxString(msg(/*Facing*/203));
	GetTextExtent(str, &w, NULL);
	DrawIndicatorText(pan_centre_x - w / 2, height + h, str);
    }

    if (m_Clino && m_Lock == lock_NONE) {
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

	Transform((*label)->x, (*label)->y, (*label)->z, &x, &y, &z);
	// Check if the label is behind us (in perspective view).
	if (z <= 0.0 || z >= 1.0) continue;

	// Apply a small shift so that translating the view doesn't make which
	// labels are displayed change as the resulting twinkling effect is
	// distracting.
	Double tx, ty, tz;
	Transform(0, 0, 0, &tx, &ty, &tz);
	tx -= floor(tx / quantise) * quantise;
	ty -= floor(ty / quantise) * quantise;

	tx = x - tx;
	if (tx < 0) continue;

	ty = y - ty;
	if (ty < 0) continue;

	unsigned int iy = unsigned(ty) / quantise;
	if (iy >= quantised_y) continue;
	unsigned int width = (*label)->width;
	unsigned int ix = unsigned(tx) / quantise;
	if (ix + width >= quantised_x) continue;

	char * test = m_LabelGrid + ix + iy * quantised_x;
	if (memchr(test, 1, width)) continue;

	y += CROSS_SIZE - GetFontSize();
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
	Transform((*label)->x, (*label)->y, (*label)->z, &x, &y, &z);

	// Check if the label is behind us (in perspective view).
	if (z <= 0) continue;

	y += CROSS_SIZE - GetFontSize();

	DrawIndicatorText((int)x, (int)y, (*label)->GetText());
    }
}

void GfxCore::DrawDepthbar()
{
    if (m_ColourBy != COLOUR_BY_DEPTH || m_Parent->GetZExtent() == 0.0)
	return;

    int y = m_YSize -
	    (DEPTH_BAR_BLOCK_HEIGHT * (GetNumDepthBands() - 1)
							+ DEPTH_BAR_OFFSET_Y);
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

    int x_min = m_XSize - DEPTH_BAR_OFFSET_X -
		     DEPTH_BAR_BLOCK_WIDTH - DEPTH_BAR_MARGIN - size;

    DrawRectangle(col_BLACK, col_DARK_GREY,
		  x_min - DEPTH_BAR_MARGIN - DEPTH_BAR_EXTRA_LEFT_MARGIN,
		  m_YSize - DEPTH_BAR_BLOCK_HEIGHT * (GetNumDepthBands() - 1) -
		      DEPTH_BAR_OFFSET_Y - DEPTH_BAR_MARGIN * 2,
		  DEPTH_BAR_BLOCK_WIDTH + size + DEPTH_BAR_MARGIN * 3 +
		      DEPTH_BAR_EXTRA_LEFT_MARGIN,
		  DEPTH_BAR_BLOCK_HEIGHT * (GetNumDepthBands() - 1) +
		      DEPTH_BAR_MARGIN*4);

    for (band = 0; band < GetNumDepthBands(); band++) {
	if (band < GetNumDepthBands() - 1) {
	    DrawShadedRectangle(GetPen(band), GetPen(band + 1),
				x_min, y,
				DEPTH_BAR_BLOCK_WIDTH, DEPTH_BAR_BLOCK_HEIGHT);
	}

	SetColour(TEXT_COLOUR);

	DrawIndicatorText(x_min + DEPTH_BAR_BLOCK_WIDTH + 5,
			  y - (FONT_SIZE / 2) - 1, strs[band]);

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

    if (m_Lock == lock_POINT || GetPerspective()) return;

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

    SetColour(TEXT_COLOUR);
    DrawIndicatorText(SCALE_BAR_OFFSET_X, end_y - FONT_SIZE - 4, "0");

    int text_width, text_height;
    GetTextExtent(str, &text_width, &text_height);
    DrawIndicatorText(SCALE_BAR_OFFSET_X + size - text_width, end_y - FONT_SIZE - 4, str);
}

bool GfxCore::CheckHitTestGrid(wxPoint& point, bool centre)
{
    if (Animating()) return false;

    if (point.x < 0 || point.x >= m_XSize ||
	point.y < 0 || point.y >= m_YSize) {
	return false;
    }

//    SetDataTransform();

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

	Transform(pt->GetX(), pt->GetY(), pt->GetZ(), &cx, &cy, &cz);

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

    if (best) {
	m_Parent->SetMouseOverStation(best);
	if (centre) {
	    CentreOn(best->GetX(), best->GetY(), best->GetZ());
	    SetThere(best->GetX(), best->GetY(), best->GetZ());
	    m_Parent->SelectTreeItem(best);
	}
    } else {
	m_Parent->SetMouseOverStation(NULL);
    }

    return best;
}

void GfxCore::OnSize(wxSizeEvent& event)
{
    // Handle a change in window size.

    wxSize size = event.GetSize();

    if (size.GetWidth() < 0 || size.GetHeight() < 0) {
	// Before things are fully initialised, we sometimes get a bogus
	// resize message...
	// FIXME have changes in mainfrm cured this?  It still happens with
	// 1.0.32 and wxGtk 2.5.2 (load a file from the command line).
	return;
    }
    m_XSize = size.GetWidth();
    m_YSize = size.GetHeight();
    m_XCentre = m_XSize / 2;
    m_YCentre = m_YSize / 2;

    if (m_DoneFirstShow) {
	if (m_LabelGrid) {
	    delete[] m_LabelGrid;
	    m_LabelGrid = NULL;
	}

	m_HitTestGridValid = false;

	Refresh(false);
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
    m_TiltAngle = 90.0;
    switch (m_Lock) {
	case lock_X:
	{
	    // elevation looking along X axis (East)
	    m_PanAngle = 270.0;
	    m_TiltAngle = 0.0;
	    break;
	}

	case lock_Y:
	case lock_XY: // survey is linearface and parallel to the Z axis => display in elevation.
	    // elevation looking along Y axis (North)
	    m_TiltAngle = 0.0;
	    break;

	case lock_Z:
	case lock_XZ: // linearface survey parallel to Y axis
	case lock_YZ: // linearface survey parallel to X axis
	{
	    // flat survey (zero height range) => go into plan view (default orientation).
	    break;
	}

	case lock_POINT:
	    m_Crosses = true;
	    break;

	case lock_NONE:
	    break;
    }

    UpdateQuaternion();
    SetTranslation(0.0, 0.0, 0.0);

    m_RotationStep = 30.0;
    m_Rotating = false;
    m_SwitchingTo = 0;
    m_Entrances = false;
    m_FixedPts = false;
    m_ExportedPts = false;
    m_Grid = false;
    m_Tubes = false;
    if (GetPerspective()) TogglePerspective();
}

void GfxCore::Defaults()
{
    // Restore default scale, rotation and translation parameters.

    DefaultParameters();
    SetScale(m_InitialScale);
    ForceRefresh();
}

// return: true if animation occured (and ForceRefresh() needs to be called)
bool GfxCore::Animate()
{
    if (!Animating()) return false;

    // Don't show pointer coordinates while animating.
    ClearCoords();
    m_Parent->ClearInfo();

    static double last_t = 0;
    double t;
    if (mpeg) {
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
		double d = sqrt(sqrd(next_mark.x - prev_mark.x) +
				sqrd(next_mark.y - prev_mark.y) +
				sqrd(next_mark.z - prev_mark.z));
		// FIXME: should ignore component of d which is unseen in
		// non-perspective mode?
		next_mark_time = sqrd(d / 100);
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
		next_mark_time += sqrd(s / 2);
		next_mark_time = sqrt(next_mark_time);
		// was: next_mark_time = max(max(d, s / 2), max(a, ta) / 60);
		//printf("*** %.6f from (\nd: %.6f\ns: %.6f\na: %.6f\nt: %.6f )\n",
		//       next_mark_time, d/100, s/2, a/60, ta/60);
		if (tmp < 0) next_mark_time *= -tmp;
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
	    here.x = q * here.x + p * next_mark.x;
	    here.y = q * here.y + p * next_mark.y;
	    here.z = q * here.z + p * next_mark.z;
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
    }

    return true;
}

// How much to allow around the box - this is because of the ring shape
// at one end of the line.
static const int HIGHLIGHTED_PT_SIZE = 2; // FIXME: tie in to blob and ring size
#define MARGIN (HIGHLIGHTED_PT_SIZE * 2 + 1)
void GfxCore::RefreshLine(const Point &a, const Point &b, const Point &c)
{
#if 0
    (void)a; (void)b; (void)c;
    ForceRefresh(); //--FIXME
#else
    // Calculate the minimum rectangle which includes the old and new
    // measuring lines to minimise the redraw time
    int l = INT_MAX, r = INT_MIN, u = INT_MIN, d = INT_MAX;
    if (a.x != DBL_MAX) {
	int x = (int)GridXToScreen(a);
	int y = (int)GridYToScreen(a);
	l = x - MARGIN;
	r = x + MARGIN;
	u = y + MARGIN;
	d = y - MARGIN;
    }
    if (b.x != DBL_MAX) {
	int x = (int)GridXToScreen(b);
	int y = (int)GridYToScreen(b);
	l = min(l, x - MARGIN);
	r = max(r, x + MARGIN);
	u = max(u, y + MARGIN);
	d = min(d, y - MARGIN);
    }
    if (c.x != DBL_MAX) {
	int x = (int)GridXToScreen(c);
	int y = (int)GridYToScreen(c);
	l = min(l, x - MARGIN);
	r = max(r, x + MARGIN);
	u = max(u, y + MARGIN);
	d = min(d, y - MARGIN);
    }
    const wxRect R(l, d, r - l, u - d);
    Refresh(false, &R);
#endif
}

void GfxCore::SetHere()
{
    if (m_here.x == DBL_MAX) return;
    Point old = m_here;
    m_here.x = DBL_MAX;
    RefreshLine(old, m_there, m_here);
}

void GfxCore::SetHere(Double x, Double y, Double z)
{
    Point old = m_here;
    m_here.x = x;
    m_here.y = y;
    m_here.z = z;
    RefreshLine(old, m_there, m_here);
}

void GfxCore::SetThere()
{
    if (m_there.x == DBL_MAX) return;
    Point old = m_there;
    m_there.x = DBL_MAX;
    RefreshLine(m_here, old, m_there);
}

void GfxCore::SetThere(Double x, Double y, Double z)
{
    Point old = m_there;
    m_there.x = x;
    m_there.y = y;
    m_there.z = z;
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
	Transform(label->GetX(), label->GetY(), label->GetZ(), &cx, &cy, &cz);
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
    // Turn the cave to a particular pan angle.
    TurnCave(angle - m_PanAngle);
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
    ReverseTransform(point.x, m_YSize - point.y, &cx, &cy, &cz);

    if (m_TiltAngle == 90.0) {
	m_Parent->SetCoords(cx + m_Parent->GetXOffset(),
			    cy + m_Parent->GetYOffset());
    } else if (m_TiltAngle == 0.0) {
	m_Parent->SetAltitude(cz + m_Parent->GetZOffset());
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
	TurnCaveTo(angle);
	m_MouseOutsideCompass = false;
    } else {
	TurnCaveTo(int(angle / 45.0) * 45.0);
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

    SetScale((m_ScaleBarWidth + dx) * m_Params.scale / m_ScaleBarWidth);
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

bool GfxCore::CanRotate() const
{
    // Determine if the survey may be rotated.

    return m_RotationOK;
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

    return (m_there.x != DBL_MAX);
}

void GfxCore::ToggleFlag(bool* flag, int update)
{
    *flag = !*flag;
    if (update == UPDATE_BLOBS) {
	UpdateBlobs();
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

void GfxCore::CentreOn(Double x, Double y, Double z)
{
    SetTranslation(-x, -y, -z);
    m_HitTestGridValid = false;

    ForceRefresh();
}

void GfxCore::ForceRefresh()
{
    Refresh(false);
}

void GfxCore::GenerateDisplayList()
{
    // Generate the display list for the underground legs.
    assert(m_HaveData);

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
    assert(m_HaveData);

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
    assert(m_HaveData);

    EnableDashedLines();
    list<vector<PointInfo> >::const_iterator trav = m_Parent->surface_traverses_begin();
    list<vector<PointInfo> >::const_iterator tend = m_Parent->surface_traverses_end();
    while (trav != tend) {
	AddPolyline(*trav);
	++trav;
    }
    DisableDashedLines();
}

// Plot crosses and/or blobs.
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
	DrawDepthbar();
    }

    // Draw compass or elevation/heading indicators.
    if ((m_Compass && m_RotationOK) || (m_Clino && m_Lock == lock_NONE)) {
	Draw2dIndicators();
    }

    // Draw scalebar.
    if (m_Scalebar) {
	DrawScalebar();
    }
}

void GfxCore::PlaceVertexWithColour(Double x, Double y, Double z, Double factor)
{
    SetColour(GetSurfacePen(), factor); // FIXME : assumes surface pen is white!
    PlaceVertex(x, y, z);
}

void GfxCore::PlaceVertexWithDepthColour(Double x, Double y, Double z,
					 Double factor)
{
    // Set the drawing colour based on the altitude.
    Double z_ext = m_Parent->GetZExtent();

    // points arising from tubes may be slightly outside the limits...
    if (z < -z_ext * 0.5) z = -z_ext * 0.5;
    if (z > z_ext * 0.5) z = z_ext * 0.5;

    Double z_offset = z + z_ext * 0.5;

    Double how_far = z_offset / z_ext;
    assert(how_far >= 0.0);
    assert(how_far <= 1.0);

    int band = int(floor(how_far * (GetNumDepthBands() - 1)));
    int next_band = (band == (GetNumDepthBands() - 1)) ? band : band + 1;

    GLAPen pen1 = GetPen(band);
    const GLAPen& pen2 = GetPen(next_band);

    Double interval = z_ext / (GetNumDepthBands() - 1);
    Double into_band = z_offset / interval - band;

//    printf("%g z_offset=%g interval=%g band=%d\n", into_band,
//	    z_offset, interval, band);
    // FIXME: why do we need to clamp here?  Is it because the walls can
    // extend further up/down than the centre-line?
    if (into_band < 0.0) into_band = 0.0;
    if (into_band > 1.0) into_band = 1.0;
    assert(into_band >= 0.0);
    assert(into_band <= 1.0);

    pen1.Interpolate(pen2, into_band);

    SetColour(pen1, factor);

    PlaceVertex(x, y, z);
}

static void IntersectLineWithPlane(Double x0, Double y0, Double z0,
				   Double x1, Double y1, Double z1,
				   Double z, Double& x, Double& y)
{
    // Find the intersection point of the line (x0, y0, z0) -> (x1, y1, z1)
    // with the plane parallel to the xy-plane with z-axis intersection z.
    assert(z1 - z0 != 0.0);

    Double t = (z - z0) / (z1 - z0);
//    assert(0.0 <= t && t <= 1.0);		FIXME: rounding problems!

    x = x0 + t * (x1 - x0);
    y = y0 + t * (y1 - y0);
}

void GfxCore::SplitLineAcrossBands(int band, int band2,
				   const Vector3 &p, const Vector3 &p2,
				   Double factor)
{
    int step = (band < band2) ? 1 : -1;
    for (int i = band; i != band2; i += step) {
	Double x, y, z;
	z = GetDepthBoundaryBetweenBands(i, i + step);
	IntersectLineWithPlane(p.getX(), p.getY(), p.getZ(),
			       p2.getX(), p2.getY(), p2.getZ(),
			       z, x, y);
	PlaceVertexWithDepthColour(x, y, z, factor);
    }
}

int GfxCore::GetDepthColour(Double z) const
{
    // Return the (0-based) depth colour band index for a z-coordinate.
    Double z_ext = m_Parent->GetZExtent();
    z += z_ext / 2;
    return int((z / (z_ext == 0.0 ? 1.0 : z_ext)) * (GetNumDepthBands() - 1));
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
    PlaceVertex(i->GetX(), i->GetY(), i->GetZ());
    ++i;
    while (i != centreline.end()) {
	PlaceVertex(i->GetX(), i->GetY(), i->GetZ());
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
    PlaceVertexWithDepthColour(i->GetX(), i->GetY(), i->GetZ());
    prev_i = i;
    ++i;
    while (i != centreline.end()) {
	int band = GetDepthColour(i->GetZ());
	if (band != band0) {
	    SplitLineAcrossBands(band0, band, prev_i->vec(), i->vec());
	    band0 = band;
	}
	PlaceVertexWithDepthColour(i->GetX(), i->GetY(), i->GetZ());
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
    glTexCoord2i(0, 0);
    PlaceVertexWithColour(a.getX(), a.getY(), a.getZ(), factor);
    glTexCoord2i(w, 0);
    PlaceVertexWithColour(b.getX(), b.getY(), b.getZ(), factor);
    glTexCoord2i(w, h);
    PlaceVertexWithColour(c.getX(), c.getY(), c.getZ(), factor);
    glTexCoord2i(0, h);
    PlaceVertexWithColour(d.getX(), d.getY(), d.getZ(), factor);
    EndQuadrilaterals();
}

void GfxCore::AddQuadrilateralDepth(const Vector3 &a, const Vector3 &b,
				    const Vector3 &c, const Vector3 &d)
{
    Vector3 normal = (a - c) * (d - b);
    normal.normalise();
    Double factor = dot(normal, light) * .3 + .7;
    int a_band, b_band, c_band, d_band;
    a_band = GetDepthColour(a.getZ());
    a_band = min(max(a_band, 0), GetNumDepthBands());
    b_band = GetDepthColour(b.getZ());
    b_band = min(max(b_band, 0), GetNumDepthBands());
    c_band = GetDepthColour(c.getZ());
    c_band = min(max(c_band, 0), GetNumDepthBands());
    d_band = GetDepthColour(d.getZ());
    d_band = min(max(d_band, 0), GetNumDepthBands());
    // All this splitting is incorrect - we need to make a separate polygon
    // for each depth band...
    int w = int(ceil(((b - a).magnitude() + (d - c).magnitude()) / 2));
    int h = int(ceil(((b - c).magnitude() + (d - a).magnitude()) / 2));
    // FIXME: should plot triangles instead to avoid rendering glitches.
    BeginPolygon();
////    PlaceNormal(normal.getX(), normal.getY(), normal.getZ());
    glTexCoord2i(0, 0);
    PlaceVertexWithDepthColour(a.getX(), a.getY(), a.getZ(), factor);
    if (a_band != b_band) {
	SplitLineAcrossBands(a_band, b_band, a, b, factor);
    }
    glTexCoord2i(w, 0);
    PlaceVertexWithDepthColour(b.getX(), b.getY(), b.getZ(), factor);
    if (b_band != c_band) {
	SplitLineAcrossBands(b_band, c_band, b, c, factor);
    }
    glTexCoord2i(w, h);
    PlaceVertexWithDepthColour(c.getX(), c.getY(), c.getZ(), factor);
    if (c_band != d_band) {
	SplitLineAcrossBands(c_band, d_band, c, d, factor);
    }
    glTexCoord2i(0, h);
    PlaceVertexWithDepthColour(d.getX(), d.getY(), d.getZ(), factor);
    if (d_band != a_band) {
	SplitLineAcrossBands(d_band, a_band, d, a, factor);
    }
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
	    Vector3 leg_v = next_pt_v.vec() - pt_v.vec();

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
	} else if (segment + 1 == centreline.size()) {
	    // last segment

	    // Calculate vector from the previous pt to this one.
	    Vector3 leg_v = pt_v.vec() - prev_pt_v.vec();

	    // Obtain a horizontal vector in the LRUD plane.
	    right = leg_v * up_v;
	    if (right.magnitude() == 0) {
		right = Vector3(last_right.getX(), last_right.getY(), 0.0);
		// Obtain a second vector in the LRUD plane,
		// perpendicular to the first.
		//up = right * leg_v;
		up = up_v;
	    } else {
		last_right = right;
		up = up_v;
	    }

	    cover_end = true;
	} else {
	    assert(i != centreline.end());
	    // Intermediate segment.

	    // Get the coordinates of the next vertex.
	    const XSect & next_pt_v = *i;

	    // Calculate vectors from this vertex to the
	    // next vertex, and from the previous vertex to
	    // this one.
	    Vector3 leg1_v = pt_v.vec() - prev_pt_v.vec();
	    Vector3 leg2_v = next_pt_v.vec() - pt_v.vec();

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
		z_pitch_adjust = n.getZ();
		//up = Vector3(0, 0, leg1_v.getZ());
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
		    Vector3 tmp = U[orient] - prev_pt_v.vec();
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
		    Vector3 tmp = U[j] - prev_pt_v.vec();
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
			Vector3 tmp = U[j] - prev_pt_v.vec();
			tmp.normalise();
			Double dotp = dot(vec, tmp);
			printf("    %d : %.8f\n", j, dotp);
		    }
		}
#endif
	    } else if (r2.magnitude() == 0) {
		Vector3 n = leg2_v;
		n.normalise();
		z_pitch_adjust = n.getZ();
		//up = Vector3(0, 0, leg2_v.getZ());
		//up = right * up;
		up = up_v;
	    } else {
		up = up_v;
	    }
	    last_right = right;
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
	v[0] = pt_v.vec() - right * l + up * u;
	v[1] = pt_v.vec() + right * r + up * u;
	v[2] = pt_v.vec() + right * r - up * d;
	v[3] = pt_v.vec() - right * l - up * d;

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
    AddTranslation(-move.getX(), -move.getY(), -move.getZ());
    // Show current position.
    m_Parent->SetCoords(m_Parent->GetXOffset() - m_Translation.x,
			m_Parent->GetYOffset() - m_Translation.y,
			m_Parent->GetZOffset() - m_Translation.z);
    ForceRefresh();
}

PresentationMark GfxCore::GetView() const
{
    return PresentationMark(m_Translation.x + m_Parent->GetXOffset(),
			    m_Translation.y + m_Parent->GetYOffset(),
			    m_Translation.z + m_Parent->GetZOffset(),
			    m_PanAngle,
			    m_TiltAngle,
			    m_Params.scale);
}

void GfxCore::SetView(const PresentationMark & p)
{
    m_SwitchingTo = 0;
    SetTranslation(p.x - m_Parent->GetXOffset(),
		   p.y - m_Parent->GetYOffset(),
		   p.z - m_Parent->GetZOffset());
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
    pres_speed = speed;
}

void GfxCore::SetColourBy(int colour_by) {
    m_ColourBy = colour_by;
    switch (colour_by) {
	case COLOUR_BY_DEPTH:
	    AddQuad = &GfxCore::AddQuadrilateralDepth;
	    AddPoly = &GfxCore::AddPolylineDepth;
	    break;
	default: // case COLOUR_BY_NONE:
	    AddQuad = &GfxCore::AddQuadrilateral;
	    AddPoly = &GfxCore::AddPolyline;
	    break;
    }

    DeleteList(m_Lists.underground_legs);
    m_Lists.underground_legs =
	CreateList(this, &GfxCore::GenerateDisplayList);

    DeleteList(m_Lists.tubes);
    m_Lists.tubes =
	CreateList(this, &GfxCore::GenerateDisplayListTubes);

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
    }
}
