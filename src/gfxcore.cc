//
//  gfxcore.cc
//
//  Core drawing code for Aven, with both standard 2D and OpenGL functionality.
//
//  Copyright (C) 2000-2001, Mark R. Shinwell.
//  Copyright (C) 2001-2002 Olly Betts
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

#include <float.h>

#include "aven.h"
#include "gfxcore.h"
#include "mainfrm.h"
#include "message.h"
#include "useful.h"

#include <wx/confbase.h>
#include <wx/image.h>

#define HEAVEN 5000.0 // altitude of heaven
#define INTERPOLATE(a, b, t) ((a) + (((b) - (a)) * Double(t) / 100.0))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAX3(a, b, c) ((a) > (b) ? MAX(a, c) : MAX(b, c))
#define TEXT_COLOUR  wxColour(0, 255, 40)
#define LABEL_COLOUR wxColour(160, 255, 0)

// Values for m_SwitchingTo
#define PLAN 1
#define ELEVATION 2

#ifdef _WIN32
static const int FONT_SIZE = 8;
#else
static const int FONT_SIZE = 9;
#endif
static const int CROSS_SIZE = 3;
static const Double COMPASS_SIZE = 24.0;
static const int COMPASS_OFFSET_X = 60;
static const int COMPASS_OFFSET_Y = 80;
static const int INDICATOR_BOX_SIZE = 60;
static const int INDICATOR_GAP = 2;
static const int INDICATOR_MARGIN = 5;
static const int INDICATOR_OFFSET_X = 15;
static const int INDICATOR_OFFSET_Y = 15;
static const int CLINO_OFFSET_X = 6 + INDICATOR_OFFSET_X + INDICATOR_BOX_SIZE + INDICATOR_GAP;
static const int DEPTH_BAR_OFFSET_X = 16;
static const int DEPTH_BAR_EXTRA_LEFT_MARGIN = 2;
static const int DEPTH_BAR_BLOCK_WIDTH = 20;
static const int DEPTH_BAR_BLOCK_HEIGHT = 15;
static const int DEPTH_BAR_MARGIN = 6;
static const int DEPTH_BAR_OFFSET_Y = 16 + DEPTH_BAR_MARGIN;
static const int TICK_LENGTH = 4;
static const int DISPLAY_SHIFT = 50;
static const int SCALE_BAR_OFFSET_X = 15;
static const int SCALE_BAR_OFFSET_Y = 12;
static const int SCALE_BAR_HEIGHT = 12;
static const int HIGHLIGHTED_PT_SIZE = 2;

const ColourTriple COLOURS[] = {
    { 0, 0, 0 },       // black
    { 100, 100, 100 }, // grey
    { 180, 180, 180 }, // light grey
    { 140, 140, 140 }, // light grey 2
    { 90, 90, 90 },    // dark grey
    { 255, 255, 255 }, // white
    { 0, 100, 255},    // turquoise
    { 0, 255, 40 },    // green
    { 150, 205, 224 }, // indicator 1
    { 114, 149, 160 }, // indicator 2
    { 255, 255, 0 },   // yellow
    { 255, 0, 0 },     // red
};

#define DELETE_ARRAY(A) do { assert((A)); delete[] (A); } while (0)

#define HITTEST_SIZE 20

BEGIN_EVENT_TABLE(GfxCore, wxWindow)
    EVT_PAINT(GfxCore::OnPaint)
    EVT_LEFT_DOWN(GfxCore::OnLButtonDown)
    EVT_LEFT_UP(GfxCore::OnLButtonUp)
    EVT_MIDDLE_DOWN(GfxCore::OnMButtonDown)
    EVT_MIDDLE_UP(GfxCore::OnMButtonUp)
    EVT_RIGHT_DOWN(GfxCore::OnRButtonDown)
    EVT_RIGHT_UP(GfxCore::OnRButtonUp)
    EVT_MOTION(GfxCore::OnMouseMove)
    EVT_SIZE(GfxCore::OnSize)
    EVT_IDLE(GfxCore::OnIdle)
    EVT_CHAR(GfxCore::OnKeyPress)
END_EVENT_TABLE()

GfxCore::GfxCore(MainFrm* parent, wxWindow* parent_win) :
    wxWindow(parent_win, 100, wxDefaultPosition, wxSize(640, 480)),
    m_Font(FONT_SIZE, wxSWISS, wxNORMAL, wxNORMAL, FALSE, "Helvetica",
	   wxFONTENCODING_ISO8859_1),
    m_InitialisePending(false)
{
    tiltangle_intial = M_PI_2;
    panangle_initial = 0.0;
    m_OffscreenBitmap = NULL;
    m_LastDrag = drag_NONE;
    m_ScaleBar.offset_x = SCALE_BAR_OFFSET_X;
    m_ScaleBar.offset_y = SCALE_BAR_OFFSET_Y;
    m_ScaleBar.width = 0;
    m_DraggingLeft = false;
    m_DraggingMiddle = false;
    m_DraggingRight = false;
    m_Parent = parent;
    m_RotationOK = true;
    m_DoneFirstShow = false;
    m_PlotData = NULL;
    m_RedrawOffscreen = false;
    m_Crosses = false;
    m_Legs = true;
    m_Names = false;
    m_OverlappingNames = false;
    m_Compass = true;
    m_Clino = true;
    m_Depthbar = true;
    m_Scalebar = true;
    m_ReverseControls = false;
    m_LabelGrid = NULL;
    m_Rotating = false;
    m_SwitchingTo = 0;
    m_Entrances = false;
    m_FixedPts = false;
    m_ExportedPts = false;
    m_Grid = false;
    wxConfigBase::Get()->Read("metric", &m_Metric, true);
    wxConfigBase::Get()->Read("degrees", &m_Degrees, true);
    m_here.x = DBL_MAX;
    m_there.x = DBL_MAX;
    clipping = false;

    // Create pens and brushes for drawing.
    int num_colours = (int) col_LAST;
    m_Pens = new wxPen[num_colours];
    m_Brushes = new wxBrush[num_colours];
    for (int col = 0; col < num_colours; col++) {
	m_Pens[col].SetColour(COLOURS[col].r, COLOURS[col].g, COLOURS[col].b);
	assert(m_Pens[col].Ok());
	m_Brushes[col].SetColour(COLOURS[col].r, COLOURS[col].g, COLOURS[col].b);
	assert(m_Brushes[col].Ok());
    }

    SetBackgroundColour(wxColour(0, 0, 0));

    // Initialise grid for hit testing.
    m_PointGrid = new list<LabelInfo*>[HITTEST_SIZE * HITTEST_SIZE];
}

GfxCore::~GfxCore()
{
    TryToFreeArrays();

    if (m_OffscreenBitmap) {
        delete m_OffscreenBitmap;
    }

    DELETE_ARRAY(m_Pens);
    DELETE_ARRAY(m_Brushes);

    delete[] m_PointGrid;
}

void GfxCore::TryToFreeArrays()
{
    // Free up any memory allocated for arrays.

    if (m_PlotData) {
	for (int band = 0; band < m_Bands; band++) {
	    DELETE_ARRAY(m_PlotData[band].vertices);
	    DELETE_ARRAY(m_PlotData[band].num_segs);
	    DELETE_ARRAY(m_PlotData[band].surface_vertices);
	    DELETE_ARRAY(m_PlotData[band].surface_num_segs);
	}

	DELETE_ARRAY(m_PlotData);
	DELETE_ARRAY(m_Polylines);
	DELETE_ARRAY(m_SurfacePolylines);

	if (m_LabelGrid) {
	    DELETE_ARRAY(m_LabelGrid);
	    m_LabelGrid = NULL;
	}

	m_PlotData = NULL;
    }
}

//
//  Initialisation methods
//

void GfxCore::Initialise()
{
    // Initialise the view from the parent holding the survey data.

    TryToFreeArrays();

    if (!m_InitialisePending) {
	GetSize(&m_XSize, &m_YSize);
    }

    m_Bands = m_Parent->GetNumDepthBands(); // last band is surface data
    m_PlotData = new PlotData[m_Bands];
    m_Polylines = new int[m_Bands];
    m_SurfacePolylines = new int[m_Bands];

    for (int band = 0; band < m_Bands; band++) {
	m_PlotData[band].vertices = new Point[m_Parent->GetNumPoints()];
	m_PlotData[band].num_segs = new int[m_Parent->GetNumLegs()];
	m_PlotData[band].surface_vertices = new Point[m_Parent->GetNumPoints()];
	m_PlotData[band].surface_num_segs = new int[m_Parent->GetNumLegs()];
    }

    m_UndergroundLegs = false;
    m_SurfaceLegs = false;

    m_HitTestGridValid = false;
    m_here.x = DBL_MAX;
    m_there.x = DBL_MAX;

    // If there are no legs (e.g. after loading a .pos file), turn crosses on.
    if (m_Parent->GetNumLegs() == 0) {
	m_Crosses = true;
    }

    // Check for flat/linear/point surveys.
    m_RotationOK = true;

    m_Lock = lock_NONE;
    if (m_Parent->GetXExtent() == 0.0) m_Lock = LockFlags(m_Lock | lock_X);
    if (m_Parent->GetYExtent() == 0.0) m_Lock = LockFlags(m_Lock | lock_Y);
    if (m_Parent->GetZExtent() == 0.0) m_Lock = LockFlags(m_Lock | lock_Z);

    // Scale the survey to a reasonable initial size.
    switch (m_Lock) {
     case lock_POINT:
       m_InitialScale = 1.0;
       break;
     case lock_YZ:
       m_InitialScale = Double(m_XSize) / m_Parent->GetXExtent();
       break;
     case lock_XZ:
       m_InitialScale = Double(m_YSize) / m_Parent->GetYExtent();
       break;
     case lock_XY:
       m_InitialScale = Double(m_YSize) / m_Parent->GetZExtent();
       break;
     case lock_X:
       m_InitialScale = min(Double(m_YSize) / m_Parent->GetZExtent(),
			    Double(m_XSize) / m_Parent->GetYExtent());
       break;
     case lock_Y:
       m_InitialScale = min(Double(m_YSize) / m_Parent->GetZExtent(),
			    Double(m_XSize) / m_Parent->GetXExtent());
       break;
     default:
       m_InitialScale = min(Double(m_XSize) / m_Parent->GetXExtent(),
			    Double(m_YSize) / m_Parent->GetYExtent());
    }
    m_InitialScale *= .85;

    // Apply default parameters.
    OnDefaults();
}

void GfxCore::FirstShow()
{
    // Update our record of the client area size and centre.
    GetClientSize(&m_XSize, &m_YSize);
    m_XCentre = m_XSize / 2;
    m_YCentre = m_YSize / 2;

    // Create the offscreen bitmap.
    m_OffscreenBitmap = new wxBitmap;
    m_OffscreenBitmap->Create(m_XSize, m_YSize);

    m_DrawDC.SelectObject(*m_OffscreenBitmap);

    m_DoneFirstShow = true;

    RedrawOffscreen();
}

//
//  Recalculating methods
//

void GfxCore::SetScaleInitial(Double scale)
{
    SetScale(scale);

    // Invalidate hit-test grid.
    m_HitTestGridValid = false;

    for (int band = 0; band < m_Bands; band++) {
	Point* pt = m_PlotData[band].vertices;
	assert(pt);
	int* count = m_PlotData[band].num_segs;
	assert(count);
	Point* spt = m_PlotData[band].surface_vertices;
	assert(spt);
	int* scount = m_PlotData[band].surface_num_segs;
	assert(scount);
	count--;
	scount--;

	m_Polylines[band] = 0;
	m_SurfacePolylines[band] = 0;
	Double x, y, z;

	list<PointInfo*>::iterator pos = m_Parent->GetPointsNC(band);
	list<PointInfo*>::iterator end = m_Parent->GetPointsEndNC(band);
	bool first_point = true;
	bool last_was_move = true;
	bool current_polyline_is_surface = false;
	while (pos != end) {
	    PointInfo* pti = *pos++;

	    if (pti->IsLine()) {
		// We have a leg.

		assert(!first_point); // The first point must always be a move.
		bool changing_ug_state = (current_polyline_is_surface != pti->IsSurface());
		// Record new underground/surface state.
		current_polyline_is_surface = pti->IsSurface();

		if (changing_ug_state || last_was_move) {
		    // Start a new polyline if we're switching
		    // underground/surface state or if the previous point
		    // was a move.
		    Point** dest;

		    if (current_polyline_is_surface) {
			m_SurfacePolylines[band]++;
			// initialise number of vertices for next polyline
			*(++scount) = 1;
			dest = &spt;
		    }
		    else {
			m_Polylines[band]++;
			// initialise number of vertices for next polyline
			*(++count) = 1;
			dest = &pt;
		    }

		    (*dest)->x = x;
		    (*dest)->y = y;
		    (*dest)->z = z;

		    // Advance the relevant coordinate pointer to the next
		    // position.
		    (*dest)++;
		}

		// Add the leg onto the current polyline.
		Point** dest = &(current_polyline_is_surface ? spt : pt);

		(*dest)->x = x = pti->GetX();
		(*dest)->y = y = pti->GetY();
		(*dest)->z = z = pti->GetZ();

		// Advance the relevant coordinate pointer to the next
		// position.
		(*dest)++;

		// Increment the relevant vertex count.
		if (current_polyline_is_surface) {
		    (*scount)++;
		}
		else {
		    (*count)++;
		}
		last_was_move = false;
	    }
	    else {
		first_point = false;
		last_was_move = true;

		// Save the current coordinates for the next time around
		// the loop.
		x = pti->GetX();
		y = pti->GetY();
		z = pti->GetZ();
	    }
	}
	if (!m_UndergroundLegs) {
	    m_UndergroundLegs = (m_Polylines[band] > 0);
	}
	if (!m_SurfaceLegs) {
	    m_SurfaceLegs = (m_SurfacePolylines[band] > 0);
	}
    }
}

void GfxCore::SetScale(Double scale)
{
    // Fill the plot data arrays with screen coordinates, scaling the survey
    // to a particular absolute scale.

    clipping = false;
    if (scale < m_InitialScale / 20) {
	scale = m_InitialScale / 20;
    } else {
	if (scale > 1000.0) scale = 1000.0;
	Double max_scale = 32767.0 / MAX(m_Parent->GetXExtent(), m_Parent->GetYExtent());
	if (scale > max_scale) clipping = true;
    }

    m_Params.scale = scale;
}

//
//  Repainting methods
//

void
GfxCore::DrawBand(int num_polylines, const int *num_segs, const Point *vertices,
		  Double m_00, Double m_01, Double m_02,
		  Double m_20, Double m_21, Double m_22)
{
    if (clipping) {
	while (num_polylines--) {
	    double X = vertices->x + m_Params.translation.x;
	    double Y = vertices->y + m_Params.translation.y;
	    double Z = vertices->z + m_Params.translation.z;
	    ++vertices;

	    int x = int(X * m_00 + Y * m_01 + Z * m_02) + m_XCentre;
	    int y = -int(X * m_20 + Y * m_21 + Z * m_22) + m_YCentre;
#define U 1
#define D 2
#define L 4
#define R 8
	    int mask = 0;
	    if (x < 0)
		mask = L;
	    else if (x > m_XSize)
		mask = R;

	    if (y < 0)
		mask |= D;
	    else if (y > m_YSize)
		mask |= U;

	    for (size_t n = *num_segs; n > 1; --n) {
		X = vertices->x + m_Params.translation.x;
		Y = vertices->y + m_Params.translation.y;
		Z = vertices->z + m_Params.translation.z;
		++vertices;

		int x2 = x;
		int y2 = y;
		int mask2 = mask;

		x = int(X * m_00 + Y * m_01 + Z * m_02) + m_XCentre;
		y = -int(X * m_20 + Y * m_21 + Z * m_22) + m_YCentre;

		mask = 0;
		if (x < 0)
		    mask = L;
		else if (x > m_XSize)
		    mask = R;

		if (y < 0)
		    mask |= D;
		else if (y > m_YSize)
		    mask |= U;

		// See if whole line is above, left, right, or below
		// screen.
		if (mask & mask2) continue;
		//printf("(%d,%d) - (%d,%d)\n", x, y, x2, y2);

		int a = x, b = y;
		int c = x2, d = y2;

		// Actually need to clip line to screen...
		if (mask & U) {
		    b = m_YSize;
		    a = ((x - x2) * b + x2 * y - x * y2) / (y - y2);
		} else if (mask & D) {
		    b = 0;
		    a = (x2 * y - x * y2) / (y - y2);
		}

		//printf("(%d,%d) -\n", a, b);
		if (a < 0 || a > m_XSize) {
		    if (!(mask & (L|R))) continue;
		    if (mask & L) {
			a = 0;
			b = (x * y2 - x2 * y) / (x - x2);
		    } else if (mask & R) {
			a = m_XSize;
			b = ((y - y2) * a + x * y2 - x2 * y) / (x - x2);
		    }
		    //printf("(%d,%d) -\n", a, b);
		    if (b < 0 || b > m_YSize) continue;
		}

		if (mask2 & U) {
		    d = m_YSize;
		    c = ((x - x2) * d + x2 * y - x * y2) / (y - y2);
		} else if (mask2 & D) {
		    d = 0;
		    c = (x2 * y - x * y2) / (y - y2);
		}

		//printf(" - (%d,%d)\n", c, d);
		if (c < 0 || c > m_XSize) {
		    if (!(mask2 & (L|R))) continue;
		    if (mask2 & L) {
			c = 0;
			d = (x * y2 - x2 * y) / (x - x2);
		    } else if (mask2 & R) {
			c = m_XSize;
			d = ((y - y2) * c + x * y2 - x2 * y) / (x - x2);
		    }
		    //printf(" - (%d,%d)\n", c, d);
		    if (d < 0 || d > m_YSize) continue;
		}
#undef U
#undef D
#undef L
#undef R
		m_DrawDC.DrawLine(a, b, c, d);
	    }
	    ++num_segs;
	}
    } else {
#define MAX_SEGS 64 // Longest run in all.3d is 44 segments - set dynamically?
	wxPoint tmp[MAX_SEGS];
	while (num_polylines--) {
	    double X = vertices->x + m_Params.translation.x;
	    double Y = vertices->y + m_Params.translation.y;
	    double Z = vertices->z + m_Params.translation.z;
	    ++vertices;

	    int x = int(X * m_00 + Y * m_01 + Z * m_02);
	    int y = -int(X * m_20 + Y * m_21 + Z * m_22);
	    tmp[0].x = x;
	    tmp[0].y = y;
	    size_t count = 1;
	    size_t n = *num_segs;
	    while (1) {
		X = vertices->x + m_Params.translation.x;
		Y = vertices->y + m_Params.translation.y;
		Z = vertices->z + m_Params.translation.z;
		++vertices;

		x = int(X * m_00 + Y * m_01 + Z * m_02);
		y = -int(X * m_20 + Y * m_21 + Z * m_22);
		tmp[count].x = x;
		tmp[count].y = y;
		++count;
		--n;
		if (count == MAX_SEGS || n == 1) {
		    m_DrawDC.DrawLines(count, tmp, m_XCentre, m_YCentre);
		    if (n == 1) break;
		    // printf("had to split traverse with %d segs\n", *num_segs);
		    tmp[0].x = x;
		    tmp[0].y = y;
		    count = 1;
		}
	    }
	    ++num_segs;
	}
    }
}

void GfxCore::RedrawOffscreen()
{
    static wxPoint blob[10] = {
	wxPoint(-1,  2),
	wxPoint( 1,  2),
	wxPoint( 2,  1),
	wxPoint(-2,  1),
	wxPoint(-2,  0),
	wxPoint( 2,  0),
	wxPoint( 2, -1),
	wxPoint(-2, -1),
	wxPoint(-1, -2),
	wxPoint( 2, -2) // (1, -2) for any platform which draws the last dot
    };
    static wxPoint cross1[2] = {
	wxPoint(-CROSS_SIZE, -CROSS_SIZE),
	wxPoint(CROSS_SIZE + 1, CROSS_SIZE + 1) // remove +1 if last dot drawn
    };
    static wxPoint cross2[2] = {
	wxPoint(-CROSS_SIZE, CROSS_SIZE),
	wxPoint(CROSS_SIZE + 1, -CROSS_SIZE - 1) // remove +/-1 if last dot
    };
#if 0
    static int ignore = 10; // Ignore the first few redraws before averaging
    static double total = 0.0;
    static int count = 0;
    double t = timer.Time() * 1.0e-3;
    if (ignore) {
	--ignore;
    } else {
	total += t;
	count++;
	cout << count / total << " average fps; " << 1.0 / t << " fps (" << t << " sec)\n";
    }
#endif
    timer.Start(); // reset timer

    // Redraw the offscreen bitmap

    // Invalidate hit-test grid.
    m_HitTestGridValid = false;

    m_DrawDC.BeginDrawing();

    // Set the font.
    m_DrawDC.SetFont(m_Font);

    // Clear the background to black.
    SetColour(col_BLACK);
    SetColour(col_BLACK, true);
    m_DrawDC.DrawRectangle(0, 0, m_XSize, m_YSize);

    if (m_PlotData) {
	Double m_00 = m_RotationMatrix.get(0, 0) * m_Params.scale;
	Double m_01 = m_RotationMatrix.get(0, 1) * m_Params.scale;
	Double m_02 = m_RotationMatrix.get(0, 2) * m_Params.scale;
	Double m_20 = m_RotationMatrix.get(2, 0) * m_Params.scale;
	Double m_21 = m_RotationMatrix.get(2, 1) * m_Params.scale;
	Double m_22 = m_RotationMatrix.get(2, 2) * m_Params.scale;

	bool grid_first = (m_TiltAngle >= 0.0);

	if (m_Grid && grid_first) {
	    DrawGrid();
	}

	// Draw underground legs.
	if (m_Legs) {
	    int start;
	    int end;
	    int inc;

	    if (m_TiltAngle >= 0.0) {
		start = 0;
		end = m_Bands;
		inc = 1;
	    } else {
		start = m_Bands - 1;
		end = -1;
		inc = -1;
	    }

	    for (int band = start; band != end; band += inc) {
		m_DrawDC.SetPen(m_Parent->GetPen(band));
		DrawBand(m_Polylines[band], m_PlotData[band].num_segs,
			 m_PlotData[band].vertices,
			 m_00, m_01, m_02, m_20, m_21, m_22);
	    }
	}
	    
	// Draw surface legs.
	if (m_Surface) {
	    int start;
	    int end;
	    int inc;

	    if (m_TiltAngle >= 0.0) {
		start = 0;
		end = m_Bands;
		inc = 1;
	    } else {
		start = m_Bands - 1;
		end = -1;
		inc = -1;
	    }

	    for (int band = start; band != end; band += inc) {
		wxPen pen = m_SurfaceDepth ? m_Parent->GetPen(band) : m_Parent->GetSurfacePen();
		if (m_SurfaceDashed) {
#ifdef _WIN32
		    pen.SetStyle(wxDOT);
#else
		    pen.SetStyle(wxSHORT_DASH);
#endif
		}
		m_DrawDC.SetPen(pen);
		DrawBand(m_SurfacePolylines[band],
			 m_PlotData[band].surface_num_segs,
			 m_PlotData[band].surface_vertices,
			 m_00, m_01, m_02, m_20, m_21, m_22);
		if (m_SurfaceDashed) {
		    pen.SetStyle(wxSOLID);
		}
	    }
	}

	// Plot crosses and/or blobs.
	if (true || // FIXME : replace true with test for there being highlighted points...
		m_Crosses || m_Entrances || m_FixedPts || m_ExportedPts) {
	    list<LabelInfo*>::const_reverse_iterator pos =
		m_Parent->GetRevLabels();
	    while (pos != m_Parent->GetRevLabelsEnd()) {
		LabelInfo* label = *pos++;

		// When more than one flag is set on a point:
		// search results take priority over entrance highlighting
		// which takes priority over fixed point
		// highlighting, which in turn takes priority over exported
		// point highlighting.

		enum AvenColour col;
		enum {BLOB, CROSS} shape = BLOB;
		if (!((m_Surface && label->IsSurface()) ||
		      (m_Legs && label->IsUnderground()) ||
		      (!label->IsSurface() && !label->IsUnderground()))) {
		    // if this station isn't to be displayed, skip to the next
		    // (last case is for stns with no legs attached)
		    continue;
		}

		if (label->IsHighLighted()) {
		    col = col_YELLOW;
		} else if (m_Entrances && label->IsEntrance()) {
		    col = col_GREEN;
		} else if (m_FixedPts && label->IsFixedPt()) {
		    col = col_RED;
		} else if (m_ExportedPts && label->IsExportedPt()) {
		    col = col_TURQUOISE;
		} else if (m_Crosses) {
		    col = col_LIGHT_GREY;
		    shape = CROSS;
		} else {
		    continue;
		}

		Double x3 = label->GetX() + m_Params.translation.x;
		Double y3 = label->GetY() + m_Params.translation.y;
		Double z3 = label->GetZ() + m_Params.translation.z;

		// Calculate screen coordinates, and check if the point is
		// visible - this is faster, and avoids coordinate
		// wrap-around problems
		int x = (int) (x3 * m_00 + y3 * m_01 + z3 * m_02) + m_XCentre;
		if (x < -CROSS_SIZE || x >= m_XSize + CROSS_SIZE) continue;
		int y = -(int) (x3 * m_20 + y3 * m_21 + z3 * m_22) + m_YCentre;
		if (y < -CROSS_SIZE || y >= m_YSize + CROSS_SIZE) continue;

		SetColour(col);
		if (shape == CROSS) {
		    m_DrawDC.DrawLines(2, cross1, x, y);
		    m_DrawDC.DrawLines(2, cross2, x, y);
		} else {
		    SetColour(col, true);
#if 0
		    m_DrawDC.DrawEllipse(x - HIGHLIGHTED_PT_SIZE,
					 y - HIGHLIGHTED_PT_SIZE,
					 HIGHLIGHTED_PT_SIZE * 2,
					 HIGHLIGHTED_PT_SIZE * 2);
#else
		    m_DrawDC.DrawLines(10, blob, x, y);
		}
	    }
	}

	if (m_Grid && !grid_first) DrawGrid();

	// Draw station names.
	if (m_Names) DrawNames();

	// Draw scalebar.
	if (m_Scalebar) DrawScalebar();

	// Draw depthbar.
	if (m_Depthbar) DrawDepthbar();

	// Draw compass or elevation/heading indicators.
	if ((m_Compass && m_RotationOK) || (m_Clino && m_Lock == lock_NONE)) {
	    Draw2dIndicators();
	}
    }

    m_DrawDC.EndDrawing();
#endif
    
    drawtime = timer.Time();
}

void GfxCore::OnPaint(wxPaintEvent& event)
{
    // Redraw the window.

    // Get a graphics context.
    wxPaintDC dc(this);

    // Make sure we're initialised.
    if (!m_DoneFirstShow) {
	FirstShow();
    }

    // Redraw the offscreen bitmap if it's out of date.
    if (m_RedrawOffscreen) {
	m_RedrawOffscreen = false;
	RedrawOffscreen();
    }

    const wxRegion& region = GetUpdateRegion();

    dc.BeginDrawing();

    // Get the areas to redraw and update them.
    wxRegionIterator iter(region);
    while (iter) {
	// Blit the bitmap onto the window.

	int x = iter.GetX();
	int y = iter.GetY();
	int width = iter.GetW();
	int height = iter.GetH();

	dc.Blit(x, y, width, height, &m_DrawDC, x, y);

	iter++;
    }
    
    if (!m_Rotating && !m_SwitchingTo) {
	int here_x = INT_MAX, here_y;
	// Draw "here" and "there".
	if (m_here.x != DBL_MAX) {
	    dc.SetPen(*wxWHITE_PEN);
	    dc.SetBrush(*wxTRANSPARENT_BRUSH);
	    here_x = (int)GridXToScreen(m_here);
	    here_y = (int)GridYToScreen(m_here);
	    dc.DrawEllipse(here_x - HIGHLIGHTED_PT_SIZE * 2,
			   here_y - HIGHLIGHTED_PT_SIZE * 2,
			   HIGHLIGHTED_PT_SIZE * 4,
			   HIGHLIGHTED_PT_SIZE * 4);
	}
	if (m_there.x != DBL_MAX) {
	    if (here_x == INT_MAX) dc.SetPen(*wxWHITE_PEN);
	    dc.SetBrush(*wxWHITE_BRUSH);
	    int there_x = (int)GridXToScreen(m_there);
	    int there_y = (int)GridYToScreen(m_there);
	    if (here_x != INT_MAX) {
		dc.DrawLine(here_x, here_y, there_x, there_y);
	    }
	    dc.DrawEllipse(there_x - HIGHLIGHTED_PT_SIZE,
		    	   there_y - HIGHLIGHTED_PT_SIZE,
			   HIGHLIGHTED_PT_SIZE * 2,
			   HIGHLIGHTED_PT_SIZE * 2);
	}
    }

    dc.EndDrawing();
}

Double GfxCore::GridXToScreen(Double x, Double y, Double z)
{
    x += m_Params.translation.x;
    y += m_Params.translation.y;
    z += m_Params.translation.z;

    return (XToScreen(x, y, z) * m_Params.scale) + m_XCentre;
}

Double GfxCore::GridYToScreen(Double x, Double y, Double z)
{
    x += m_Params.translation.x;
    y += m_Params.translation.y;
    z += m_Params.translation.z;

    return m_YCentre - ((ZToScreen(x, y, z) * m_Params.scale));
}

void GfxCore::DrawGrid()
{
    // Draw the grid.
    SetColour(col_RED);

    // Calculate the extent of the survey, in metres across the screen plane.
    Double m_across_screen = Double(m_XSize / m_Params.scale);
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

    for (int xc = 0; xc <= count_x; xc++) {
	Double x = left + xc*grid_size;
	m_DrawDC.DrawLine((int) GridXToScreen(x, bottom, grid_z), (int) GridYToScreen(x, bottom, grid_z),
			  (int) GridXToScreen(x, actual_top, grid_z), (int) GridYToScreen(x, actual_top, grid_z));
    }

    for (int yc = 0; yc <= count_y; yc++) {
	Double y = bottom + yc*grid_size;
	m_DrawDC.DrawLine((int) GridXToScreen(left, y, grid_z), (int) GridYToScreen(left, y, grid_z),
			  (int) GridXToScreen(actual_right, y, grid_z),
			  (int) GridYToScreen(actual_right, y, grid_z));
    }
}

wxCoord GfxCore::GetClinoOffset()
{
    return m_Compass ? CLINO_OFFSET_X : INDICATOR_OFFSET_X;
}

wxPoint GfxCore::CompassPtToScreen(Double x, Double y, Double z)
{
    return wxPoint(long(-XToScreen(x, y, z)) + m_XSize - COMPASS_OFFSET_X,
		   long(ZToScreen(x, y, z)) + m_YSize - COMPASS_OFFSET_Y);
}

wxPoint GfxCore::IndicatorCompassToScreenPan(int angle)
{
    Double theta = (angle * M_PI / 180.0) + m_PanAngle;
    wxCoord length = (INDICATOR_BOX_SIZE - INDICATOR_MARGIN*2) / 2;
    wxCoord x = wxCoord(length * sin(theta));
    wxCoord y = wxCoord(length * cos(theta));

    return wxPoint(m_XSize - INDICATOR_OFFSET_X - INDICATOR_BOX_SIZE/2 - x,
		   m_YSize - INDICATOR_OFFSET_Y - INDICATOR_BOX_SIZE/2 - y);
}

wxPoint GfxCore::IndicatorCompassToScreenElev(int angle)
{
    Double theta = (angle * M_PI / 180.0) + m_TiltAngle + M_PI_2;
    wxCoord length = (INDICATOR_BOX_SIZE - INDICATOR_MARGIN*2) / 2;
    wxCoord x = wxCoord(length * sin(-theta));
    wxCoord y = wxCoord(length * cos(-theta));

    return wxPoint(m_XSize - GetClinoOffset() - INDICATOR_BOX_SIZE/2 - x,
		   m_YSize - INDICATOR_OFFSET_Y - INDICATOR_BOX_SIZE/2 - y);
}

void GfxCore::DrawTick(wxCoord cx, wxCoord cy, int angle_cw)
{
    Double theta = angle_cw * M_PI / 180.0;
    wxCoord length1 = (INDICATOR_BOX_SIZE - INDICATOR_MARGIN*2) / 2;
    wxCoord length0 = length1 + TICK_LENGTH;
    wxCoord x0 = wxCoord(length0 * sin(theta));
    wxCoord y0 = wxCoord(length0 * -cos(theta));
    wxCoord x1 = wxCoord(length1 * sin(theta));
    wxCoord y1 = wxCoord(length1 * -cos(theta));

    m_DrawDC.DrawLine(cx + x0, cy + y0, cx + x1, cy + y1);
}

void GfxCore::Draw2dIndicators()
{
    // Draw the "traditional" elevation and compass indicators.

    //-- code is a bit messy...

    // Indicator backgrounds
    SetColour(col_GREY, true);
    SetColour(col_LIGHT_GREY_2);

    if (m_Compass && m_RotationOK) {
	m_DrawDC.DrawEllipse(m_XSize - INDICATOR_OFFSET_X - INDICATOR_BOX_SIZE + INDICATOR_MARGIN,
			     m_YSize - INDICATOR_OFFSET_Y - INDICATOR_BOX_SIZE + INDICATOR_MARGIN,
			     INDICATOR_BOX_SIZE - INDICATOR_MARGIN*2,
			     INDICATOR_BOX_SIZE - INDICATOR_MARGIN*2);
    }
    if (m_Clino && m_Lock == lock_NONE) {
	int tilt = (int) (m_TiltAngle * 180.0 / M_PI);
	m_DrawDC.DrawEllipticArc(m_XSize - GetClinoOffset() - INDICATOR_BOX_SIZE +
				 INDICATOR_MARGIN,
				 m_YSize - INDICATOR_OFFSET_Y - INDICATOR_BOX_SIZE +
				 INDICATOR_MARGIN,
				 INDICATOR_BOX_SIZE - INDICATOR_MARGIN*2,
				 INDICATOR_BOX_SIZE - INDICATOR_MARGIN*2,
				 -180 - tilt, -tilt); // do not change the order of these two
						      // or the code will fail on Windows

	m_DrawDC.DrawLine(m_XSize - GetClinoOffset() - INDICATOR_BOX_SIZE/2,
			  m_YSize - INDICATOR_OFFSET_Y - INDICATOR_MARGIN,
			  m_XSize - GetClinoOffset() - INDICATOR_BOX_SIZE/2,
			  m_YSize - INDICATOR_OFFSET_Y - INDICATOR_BOX_SIZE + INDICATOR_MARGIN);

	m_DrawDC.DrawLine(m_XSize - GetClinoOffset() - INDICATOR_BOX_SIZE/2,
			  m_YSize - INDICATOR_OFFSET_Y - INDICATOR_BOX_SIZE/2,
			  m_XSize - GetClinoOffset() - INDICATOR_MARGIN,
			  m_YSize - INDICATOR_OFFSET_Y - INDICATOR_BOX_SIZE/2);
    }

    // Ticks
    bool white = m_DraggingLeft && m_LastDrag == drag_COMPASS && m_MouseOutsideCompass;
    wxCoord pan_centre_x = m_XSize - INDICATOR_OFFSET_X - INDICATOR_BOX_SIZE/2;
    wxCoord centre_y = m_YSize - INDICATOR_OFFSET_Y - INDICATOR_BOX_SIZE/2;
    wxCoord elev_centre_x = m_XSize - GetClinoOffset() - INDICATOR_BOX_SIZE/2;
    if (m_Compass && m_RotationOK) {
	int deg_pan = (int) (m_PanAngle * 180.0 / M_PI);
	//--FIXME: bodge by Olly to stop wrong tick highlighting
	if (deg_pan) deg_pan = 360 - deg_pan;
	for (int angle = deg_pan; angle <= 315 + deg_pan; angle += 45) {
	    if (deg_pan == angle) {
		SetColour(col_GREEN);
	    }
	    else {
		SetColour(white ? col_WHITE : col_LIGHT_GREY_2);
	    }
	    DrawTick(pan_centre_x, centre_y, angle);
	}
    }
    if (m_Clino && m_Lock == lock_NONE) {
	white = m_DraggingLeft && m_LastDrag == drag_ELEV && m_MouseOutsideElev;
	int deg_elev = (int) (m_TiltAngle * 180.0 / M_PI);
	for (int angle = 0; angle <= 180; angle += 90) {
	    if (deg_elev == angle - 90) {
		SetColour(col_GREEN);
	    }
	    else {
		SetColour(white ? col_WHITE : col_LIGHT_GREY_2);
	    }
	    DrawTick(elev_centre_x, centre_y, angle);
	}
    }

    // Pan arrow
    if (m_Compass && m_RotationOK) {
	wxPoint p1 = IndicatorCompassToScreenPan(0);
	wxPoint p2 = IndicatorCompassToScreenPan(150);
	wxPoint p3 = IndicatorCompassToScreenPan(210);
	wxPoint pc(pan_centre_x, centre_y);
	wxPoint pts1[3] = { p2, p1, pc };
	wxPoint pts2[3] = { p3, p1, pc };
	SetColour(col_LIGHT_GREY);
	SetColour(col_INDICATOR_1, true);
	m_DrawDC.DrawPolygon(3, pts1);
	SetColour(col_INDICATOR_2, true);
	m_DrawDC.DrawPolygon(3, pts2);
    }

    // Elevation arrow
    if (m_Clino && m_Lock == lock_NONE) {
	wxPoint p1e = IndicatorCompassToScreenElev(0);
	wxPoint p2e = IndicatorCompassToScreenElev(150);
	wxPoint p3e = IndicatorCompassToScreenElev(210);
	wxPoint pce(elev_centre_x, centre_y);
	wxPoint pts1e[3] = { p2e, p1e, pce };
	wxPoint pts2e[3] = { p3e, p1e, pce };
	SetColour(col_LIGHT_GREY);
	SetColour(col_INDICATOR_2, true);
	m_DrawDC.DrawPolygon(3, pts1e);
	SetColour(col_INDICATOR_1, true);
	m_DrawDC.DrawPolygon(3, pts2e);
    }

    // Text
    m_DrawDC.SetTextBackground(wxColour(0, 0, 0));
    m_DrawDC.SetTextForeground(TEXT_COLOUR);

    wxCoord w, h;
    wxCoord width, height;
    wxString str;

    m_DrawDC.GetTextExtent(wxString("000"), &width, &h);
    height = m_YSize - INDICATOR_OFFSET_Y - INDICATOR_BOX_SIZE - INDICATOR_GAP - h;

    if (m_Compass && m_RotationOK) {
	if (m_Degrees) {
	    str = wxString::Format("%03d", int(m_PanAngle * 180.0 / M_PI));
	} else {
	    str = wxString::Format("%03d", int(m_PanAngle * 200.0 / M_PI));
	}	
	m_DrawDC.GetTextExtent(str, &w, &h);
	m_DrawDC.DrawText(str, pan_centre_x + width / 2 - w, height);
	str = wxString(msg(/*Facing*/203));
	m_DrawDC.GetTextExtent(str, &w, &h);
	m_DrawDC.DrawText(str, pan_centre_x - w / 2, height - h);
    }

    if (m_Clino && m_Lock == lock_NONE) {
	int angle;
	if (m_Degrees) {
	    angle = int(-m_TiltAngle * 180.0 / M_PI);
	} else {
	    angle = int(-m_TiltAngle * 200.0 / M_PI);
	}	
	str = angle ? wxString::Format("%+03d", angle) : wxString("00");
	m_DrawDC.GetTextExtent(str, &w, &h);
	m_DrawDC.DrawText(str, elev_centre_x + width / 2 - w, height);
	str = wxString(msg(/*Elevation*/118));
	m_DrawDC.GetTextExtent(str, &w, &h);
	m_DrawDC.DrawText(str, elev_centre_x - w / 2, height - h);
    }
}

// FIXME: either remove this, or make it an option...
void GfxCore::DrawCompass()
{
    // Draw the 3d compass.

    wxPoint pt[3];

    SetColour(col_TURQUOISE);
    m_DrawDC.DrawLine(CompassPtToScreen(0.0, 0.0, -COMPASS_SIZE),
		      CompassPtToScreen(0.0, 0.0, COMPASS_SIZE));

    pt[0] = CompassPtToScreen(-COMPASS_SIZE / 3.0f, 0.0, -COMPASS_SIZE * 2.0f / 3.0f);
    pt[1] = CompassPtToScreen(0.0, 0.0, -COMPASS_SIZE);
    pt[2] = CompassPtToScreen(COMPASS_SIZE / 3.0f, 0.0, -COMPASS_SIZE * 2.0f / 3.0f);
    m_DrawDC.DrawLines(3, pt);

    m_DrawDC.DrawLine(CompassPtToScreen(-COMPASS_SIZE, 0.0, 0.0),
		      CompassPtToScreen(COMPASS_SIZE, 0.0, 0.0));

    SetColour(col_GREEN);
    m_DrawDC.DrawLine(CompassPtToScreen(0.0, -COMPASS_SIZE, 0.0),
		      CompassPtToScreen(0.0, COMPASS_SIZE, 0.0));

    pt[0] = CompassPtToScreen(-COMPASS_SIZE / 3.0f, -COMPASS_SIZE * 2.0f / 3.0f, 0.0);
    pt[1] = CompassPtToScreen(0.0, -COMPASS_SIZE, 0.0);
    pt[2] = CompassPtToScreen(COMPASS_SIZE / 3.0f, -COMPASS_SIZE * 2.0f / 3.0f, 0.0);
    m_DrawDC.DrawLines(3, pt);
}

void GfxCore::DrawNames()
{
    // Draw station names.
    m_DrawDC.SetTextBackground(wxColour(0, 0, 0));
    m_DrawDC.SetTextForeground(LABEL_COLOUR);

    if (m_OverlappingNames) {
	SimpleDrawNames();
    } else {
	NattyDrawNames();
    }
}

void GfxCore::NattyDrawNames()
{
    // Draw station names, without overlapping.
    // FIXME: copied to OnSize()
    const int dv = 2;
    const int quantise = int(FONT_SIZE / dv);
    const int quantised_x = m_XSize / quantise;
    const int quantised_y = m_YSize / quantise;
    const size_t buffer_size = quantised_x * quantised_y;
    if (!m_LabelGrid) m_LabelGrid = new char[buffer_size];
    memset((void*)m_LabelGrid, 0, buffer_size);

    list<LabelInfo*>::const_iterator label = m_Parent->GetLabels();

    while (label != m_Parent->GetLabelsEnd()) {
	Double x = GridXToScreen((*label)->x, (*label)->y, (*label)->z);
	Double y = GridYToScreen((*label)->x, (*label)->y, (*label)->z)
	    + CROSS_SIZE - FONT_SIZE;

	wxString str = (*label)->GetText();

	Double t = GridXToScreen(0, 0, 0);
	t -= floor(t / quantise) * quantise;
	int ix = int(x - t) / quantise;
	t = GridYToScreen(0, 0, 0);
	t -= floor(t / quantise) * quantise;
	int iy = int(y - t) / quantise;

	bool reject = true;

	if (ix >= 0 && ix < quantised_x && iy >= 0 && iy < quantised_y) {
	    char *test = &m_LabelGrid[ix + iy * quantised_x];
	    int len = str.Length() * dv + 1;
	    reject = (ix + len >= quantised_x);
	    int i = 0;
	    while (!reject && i++ < len) {
		reject = *test++;
	    }

	    if (!reject) {
		m_DrawDC.DrawText(str, (wxCoord)x, (wxCoord)y);

		int ymin = (iy >= 2) ? iy - 2 : iy;
		int ymax = (iy < quantised_y - 2) ? iy + 2 : iy;
		for (int y0 = ymin; y0 <= ymax; y0++) {
		    memset((void*) &m_LabelGrid[ix + y0 * quantised_x], 1, len);
		}
	    }
	}
	++label;
    }
}

void GfxCore::SimpleDrawNames()
{
    // Draw all station names, without worrying about overlaps
    list<LabelInfo*>::const_iterator label = m_Parent->GetLabels();
    while (label != m_Parent->GetLabelsEnd()) {
	wxCoord x = (wxCoord)GridXToScreen((*label)->x, (*label)->y, (*label)->z);
	wxCoord y = (wxCoord)GridYToScreen((*label)->x, (*label)->y, (*label)->z);
	m_DrawDC.DrawText((*label)->GetText(), x, y + CROSS_SIZE - FONT_SIZE);
	++label;
    }
}

void GfxCore::DrawDepthbar()
{
    if (m_Parent->GetZExtent() == 0.0) return;

    m_DrawDC.SetTextBackground(wxColour(0, 0, 0));
    m_DrawDC.SetTextForeground(TEXT_COLOUR);

    int y = DEPTH_BAR_BLOCK_HEIGHT * (m_Bands - 1) + DEPTH_BAR_OFFSET_Y;
    int size = 0;

    wxString* strs = new wxString[m_Bands];
    for (int band = 0; band < m_Bands; band++) {
	Double z = m_Parent->GetZMin() + m_Parent->GetZExtent() * band
		/ (m_Bands - 1);
	strs[band] = FormatLength(z, false);
	int x, y;
	m_DrawDC.GetTextExtent(strs[band], &x, &y);
	if (x > size) {
	    size = x;
	}
    }

    int x_min = m_XSize - DEPTH_BAR_OFFSET_X - DEPTH_BAR_BLOCK_WIDTH
	    - DEPTH_BAR_MARGIN - size;

    SetColour(col_BLACK);
    SetColour(col_DARK_GREY, true);
    m_DrawDC.DrawRectangle(x_min - DEPTH_BAR_MARGIN
		    	     - DEPTH_BAR_EXTRA_LEFT_MARGIN,
			   DEPTH_BAR_OFFSET_Y - DEPTH_BAR_MARGIN*2,
			   DEPTH_BAR_BLOCK_WIDTH + size + DEPTH_BAR_MARGIN*3 +
			     DEPTH_BAR_EXTRA_LEFT_MARGIN,
			   DEPTH_BAR_BLOCK_HEIGHT*(m_Bands - 1) + DEPTH_BAR_MARGIN*4);

    for (int band = 0; band < m_Bands; band++) {
	if (band < m_Bands - 1) {
	    m_DrawDC.SetPen(m_Parent->GetPen(band));
	    m_DrawDC.SetBrush(m_Parent->GetBrush(band));
	    m_DrawDC.DrawRectangle(x_min,
			           y - DEPTH_BAR_BLOCK_HEIGHT,
				   DEPTH_BAR_BLOCK_WIDTH,
				   DEPTH_BAR_BLOCK_HEIGHT);
	}

	m_DrawDC.DrawText(strs[band], x_min + DEPTH_BAR_BLOCK_WIDTH + 5,
			  y - (FONT_SIZE / 2) - 1);

	y -= DEPTH_BAR_BLOCK_HEIGHT;
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
    if (m_Lock == lock_POINT) return;

    // Draw the scalebar.

    // Calculate the extent of the survey, in metres across the screen plane.
    int x_size = m_XSize;

    Double across_screen = Double(x_size / m_Params.scale);
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
    Double size_snap = pow(10.0, floor(log10(0.75 * across_screen)));
    Double t = across_screen * 0.75 / size_snap;
    if (t >= 5.0) {
	size_snap *= 5.0;
    } else if (t >= 2.0) {
	size_snap *= 2.0;
    }

    if (!m_Metric) size_snap *= multiplier;

    // Actual size of the thing in pixels:
    int size = int(size_snap * m_Params.scale);
    m_ScaleBar.width = (int) size; //FIXME

    // Draw it...
    //--FIXME: improve this
    int end_x = m_ScaleBar.offset_x;
    int height = SCALE_BAR_HEIGHT;
    int end_y = m_YSize - m_ScaleBar.offset_y - height;
    int interval = size / 10;

    bool solid = true;
    for (int ix = 0; ix < 10; ix++) {
	int x = end_x + int(ix * ((Double) size / 10.0));

	SetColour(solid ? col_GREY : col_WHITE);
	SetColour(solid ? col_GREY : col_WHITE, true);

	m_DrawDC.DrawRectangle(x, end_y, interval + 2, height);

	solid = !solid;
    }

    // Add labels.
    wxString str = FormatLength(size_snap);

    m_DrawDC.SetTextBackground(wxColour(0, 0, 0));
    m_DrawDC.SetTextForeground(TEXT_COLOUR);
    m_DrawDC.DrawText("0", end_x, end_y - FONT_SIZE - 4);

    int text_width, text_height;
    m_DrawDC.GetTextExtent(str, &text_width, &text_height);
    m_DrawDC.DrawText(str, end_x + size - text_width, end_y - FONT_SIZE - 4);
}

//
//  Mouse event handling methods
//

void GfxCore::OnLButtonDown(wxMouseEvent& event)
{
    if (m_PlotData && m_Lock != lock_POINT) {
	m_DraggingLeft = true;
	m_ScaleBar.drag_start_offset_x = m_ScaleBar.offset_x;
	m_ScaleBar.drag_start_offset_y = m_ScaleBar.offset_y;
	m_DragStart = m_DragRealStart = wxPoint(event.GetX(), event.GetY());

	CaptureMouse();
    }
}

void GfxCore::OnLButtonUp(wxMouseEvent& event)
{
    if (m_PlotData && m_Lock != lock_POINT) {
	if (event.GetPosition() == m_DragRealStart) {
	    // just a "click"...
	    CheckHitTestGrid(m_DragStart, true);
	}

	m_LastDrag = drag_NONE;
	m_DraggingLeft = false;
	const wxRect r(m_XSize - INDICATOR_OFFSET_X - INDICATOR_BOX_SIZE*2 - INDICATOR_GAP,
		       m_YSize - INDICATOR_OFFSET_Y - INDICATOR_BOX_SIZE,
		       INDICATOR_BOX_SIZE*2 + INDICATOR_GAP,
		       INDICATOR_BOX_SIZE);
	m_RedrawOffscreen = true;
	Refresh(false, &r);
	ReleaseMouse();
    }
}

void GfxCore::OnMButtonDown(wxMouseEvent& event)
{
    if (m_PlotData && m_Lock == lock_NONE) {
	m_DraggingMiddle = true;
	m_DragStart = wxPoint(event.GetX(), event.GetY());

	CaptureMouse();
    }
}

void GfxCore::OnMButtonUp(wxMouseEvent& event)
{
    if (m_PlotData && m_Lock == lock_NONE) {
	m_DraggingMiddle = false;
	ReleaseMouse();
    }
}

void GfxCore::OnRButtonDown(wxMouseEvent& event)
{
    if (m_PlotData) {
	m_DragStart = wxPoint(event.GetX(), event.GetY());
	m_ScaleBar.drag_start_offset_x = m_ScaleBar.offset_x;
	m_ScaleBar.drag_start_offset_y = m_ScaleBar.offset_y;
	m_DraggingRight = true;

	CaptureMouse();
    }
}

void GfxCore::OnRButtonUp(wxMouseEvent& event)
{
    m_DraggingRight = false;
    m_LastDrag = drag_NONE;
    ReleaseMouse();
}

void GfxCore::HandleScaleRotate(bool control, wxPoint point)
{
    // Handle a mouse movement during scale/rotate mode.
    int dx = point.x - m_DragStart.x;
    int dy = point.y - m_DragStart.y;

    if (m_ReverseControls) {
	dx = -dx;
	dy = -dy;
    }

    Double pan_angle = m_RotationOK ? (Double(dx) * (-M_PI / 500.0)) : 0.0;

    // left/right => rotate, up/down => scale
    TurnCave(pan_angle);

    if (control) {
	// For now...
	TiltCave(Double(-dy) * M_PI / 500.0);
    } else {
	SetScale(m_Params.scale *= pow(1.06, 0.08 * dy));
    }

    ForceRefresh();

    m_DragStart = point;
}

void GfxCore::TurnCave(Double angle)
{
    // Turn the cave around its z-axis by a given angle.
    m_PanAngle += angle;
    if (m_PanAngle >= M_PI * 2.0) {
	m_PanAngle -= M_PI * 2.0;
    } else if (m_PanAngle < 0.0) {
	m_PanAngle += M_PI * 2.0;
    }
    m_Params.rotation.setFromEulerAngles(0.0, 0.0, m_PanAngle);
    Quaternion q;
    q.setFromEulerAngles(m_TiltAngle, 0.0, 0.0);
    m_Params.rotation = q * m_Params.rotation;
    m_RotationMatrix = m_Params.rotation.asMatrix();
}

void GfxCore::TurnCaveTo(Double angle)
{
    // Turn the cave to a particular pan angle.
    TurnCave(angle - m_PanAngle);
}

void GfxCore::TiltCave(Double tilt_angle)
{
    // Tilt the cave by a given angle.
    if (m_TiltAngle + tilt_angle > M_PI_2) {
	tilt_angle = M_PI_2 - m_TiltAngle;
    } else if (m_TiltAngle + tilt_angle < -M_PI_2) {
	tilt_angle = -M_PI_2 - m_TiltAngle;
    }

    m_TiltAngle += tilt_angle;

    m_Params.rotation.setFromEulerAngles(0.0, 0.0, m_PanAngle);
    Quaternion q;
    q.setFromEulerAngles(m_TiltAngle, 0.0, 0.0);
    m_Params.rotation = q * m_Params.rotation;
    m_RotationMatrix = m_Params.rotation.asMatrix();
}

void GfxCore::HandleTilt(wxPoint point)
{
    // Handle a mouse movement during tilt mode.
    int dy = point.y - m_DragStart.y;

    if (m_ReverseControls) dy = -dy;

    TiltCave(Double(-dy) * M_PI / 500.0);

    m_DragStart = point;

    ForceRefresh();
}

void GfxCore::HandleTranslate(wxPoint point)
{
    // Handle a mouse movement during translation mode.
    int dx = point.x - m_DragStart.x;
    int dy = point.y - m_DragStart.y;
    
    if (m_ReverseControls) {
	dx = -dx;
	dy = -dy;
    }

    TranslateCave(dx, dy);
    m_DragStart = point;
}

void GfxCore::TranslateCave(int dx, int dy)
{
    // Find out how far the screen movement takes us in cave coords.
    Double x = Double(dx / m_Params.scale);
    Double z = Double(-dy / m_Params.scale);

    Matrix4 inverse_rotation = m_Params.rotation.asInverseMatrix();

    Double cx = Double(inverse_rotation.get(0, 0)*x + inverse_rotation.get(0, 2)*z);
    Double cy = Double(inverse_rotation.get(1, 0)*x + inverse_rotation.get(1, 2)*z);
    Double cz = Double(inverse_rotation.get(2, 0)*x + inverse_rotation.get(2, 2)*z);

    // Update parameters and redraw.
    m_Params.translation.x += cx;
    m_Params.translation.y += cy;
    m_Params.translation.z += cz;

    ForceRefresh();
}

void GfxCore::CheckHitTestGrid(wxPoint& point, bool centre)
{
    if (point.x < 0 || point.x >= m_XSize || point.y < 0 || point.y >= m_YSize) {
	return;
    }

    if (!m_HitTestGridValid) CreateHitTestGrid();

    int grid_x = (point.x * (HITTEST_SIZE - 1)) / m_XSize;
    int grid_y = (point.y * (HITTEST_SIZE - 1)) / m_YSize;

    LabelInfo *best = NULL;
    int dist_sqrd = 25;
    int square = grid_x + grid_y * HITTEST_SIZE;
    list<LabelInfo*>::iterator iter = m_PointGrid[square].begin();
    while (iter != m_PointGrid[square].end()) {
	LabelInfo *pt = *iter++;

	int dx = point.x -
		(int)GridXToScreen(pt->GetX(), pt->GetY(), pt->GetZ());
	int ds = dx * dx;
	if (ds >= dist_sqrd) continue;
	int dy = point.y -
		(int)GridYToScreen(pt->GetX(), pt->GetY(), pt->GetZ());

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
}

void GfxCore::OnMouseMove(wxMouseEvent& event)
{
    // Mouse motion event handler.
    if (!m_PlotData) return;

    wxPoint point = wxPoint(event.GetX(), event.GetY());

    // Check hit-test grid (only if no buttons are pressed).
    if (!event.LeftIsDown() && !event.MiddleIsDown() && !event.RightIsDown()) {
	CheckHitTestGrid(point, false);
    }

    // Update coordinate display if in plan view, or altitude if in elevation
    // view.
    if (m_TiltAngle == M_PI_2) {
	int x = event.GetX() - m_XCentre;
	int y = -(event.GetY() - m_YCentre);
	Matrix4 inverse_rotation = m_Params.rotation.asInverseMatrix();

	//--TODO: GL version
	Double cx = Double(inverse_rotation.get(0, 0)*x + inverse_rotation.get(0, 2)*y);
	Double cy = Double(inverse_rotation.get(1, 0)*x + inverse_rotation.get(1, 2)*y);

	m_Parent->SetCoords(cx / m_Params.scale - m_Params.translation.x + m_Parent->GetXOffset(),
			    cy / m_Params.scale - m_Params.translation.y + m_Parent->GetYOffset());
    } else if (m_TiltAngle == 0.0) {
	int z = -(event.GetY() - m_YCentre);
	m_Parent->SetAltitude(z / m_Params.scale - m_Params.translation.z + m_Parent->GetZOffset());
    } else {
	m_Parent->ClearCoords();
    }

    if (!m_SwitchingTo) {
	if (m_DraggingLeft) {
	    wxCoord x0 = m_XSize - INDICATOR_OFFSET_X - INDICATOR_BOX_SIZE/2;
	    wxCoord x1 = wxCoord(m_XSize - GetClinoOffset() - INDICATOR_BOX_SIZE/2);
	    wxCoord y = m_YSize - INDICATOR_OFFSET_Y - INDICATOR_BOX_SIZE/2;

	    wxCoord dx0 = point.x - x0;
	    wxCoord dx1 = point.x - x1;
	    wxCoord dy = point.y - y;

	    wxCoord radius = (INDICATOR_BOX_SIZE - INDICATOR_MARGIN*2) / 2;

	    if (m_LastDrag == drag_NONE) {
		if (m_Compass && dx0 * dx0 + dy * dy <= radius * radius)
		    m_LastDrag = drag_COMPASS;
		else if (m_Clino && dx1 * dx1 + dy * dy <= radius * radius)
		    m_LastDrag = drag_ELEV;
		else if (point.x >= m_ScaleBar.offset_x &&
			 point.x <= m_ScaleBar.offset_x + m_ScaleBar.width &&
			 point.y <= m_YSize - m_ScaleBar.offset_y &&
			 point.y >= m_YSize - m_ScaleBar.offset_y - SCALE_BAR_HEIGHT)
		    m_LastDrag = drag_SCALE;
	    }
	    if (m_LastDrag == drag_COMPASS) {
		// drag in heading indicator
		if (dx0 * dx0 + dy * dy <= radius * radius) {
		    TurnCaveTo(atan2(dx0, dy) - M_PI);
		    m_MouseOutsideCompass = false;
		}
		else {
		    TurnCaveTo(int(int((atan2(dx0, dy) - M_PI) * 180.0 / M_PI) / 45) *
			       M_PI_4);
		    m_MouseOutsideCompass = true;
		}
		ForceRefresh();
	    }
	    else if (m_LastDrag == drag_ELEV) {
		// drag in elevation indicator
		if (dx1 >= 0 && dx1 * dx1 + dy * dy <= radius * radius) {
		    TiltCave(atan2(dy, dx1) - m_TiltAngle);
		    m_MouseOutsideElev = false;
		}
		else if (dy >= INDICATOR_MARGIN) {
		    TiltCave(M_PI_2 - m_TiltAngle);
		    m_MouseOutsideElev = true;
		}
		else if (dy <= -INDICATOR_MARGIN) {
		    TiltCave(-M_PI_2 - m_TiltAngle);
		    m_MouseOutsideElev = true;
		}
		else {
		    TiltCave(-m_TiltAngle);
		    m_MouseOutsideElev = true;
		}
		ForceRefresh();
	    }
	    else if (m_LastDrag == drag_SCALE) {
		if (point.x >= 0 && point.x <= m_XSize) {
		    //--FIXME: GL fix needed

		    Double size_snap = Double(m_ScaleBar.width) / m_Params.scale;
		    int dx = point.x - m_DragLast.x;

		    SetScale((m_ScaleBar.width + dx) / size_snap);
		    ForceRefresh();
		}
	    }
	    else if (m_LastDrag == drag_NONE || m_LastDrag == drag_MAIN) {
		m_LastDrag = drag_MAIN;
		HandleScaleRotate(event.ControlDown(), point);
	    }
	}
	else if (m_DraggingMiddle) {
	    HandleTilt(point);
	}
	else if (m_DraggingRight) {
	  //FIXME: this needs sorting for GL
	    if ((m_LastDrag == drag_NONE &&
		 point.x >= m_ScaleBar.offset_x &&
		 point.x <= m_ScaleBar.offset_x + m_ScaleBar.width &&
		 point.y <= m_YSize - m_ScaleBar.offset_y &&
		 point.y >= m_YSize - m_ScaleBar.offset_y - SCALE_BAR_HEIGHT) ||
		 m_LastDrag == drag_SCALE) {
		  if (point.x < 0) point.x = 0;
		  if (point.y < 0) point.y = 0;
		  if (point.x > m_XSize) point.x = m_XSize;
		  if (point.y > m_YSize) point.y = m_YSize;
		  m_LastDrag = drag_SCALE;
		  int x_inside_bar = m_DragStart.x - m_ScaleBar.drag_start_offset_x;
		  int y_inside_bar = m_YSize - m_ScaleBar.drag_start_offset_y - m_DragStart.y;
		  m_ScaleBar.offset_x = point.x - x_inside_bar;
		  m_ScaleBar.offset_y = (m_YSize - point.y) - y_inside_bar;
		  ForceRefresh();
	    }
	    else {
		m_LastDrag = drag_MAIN;
		HandleTranslate(point);
	    }
	}
    }

    m_DragLast = point;
}

void GfxCore::OnSize(wxSizeEvent& event)
{
    // Handle a change in window size.

    wxSize size = event.GetSize();

    m_XSize = size.GetWidth();
    m_YSize = size.GetHeight();
    if (m_XSize < 0 || m_YSize < 0) { //-- FIXME
	m_XSize = 640;
	m_YSize = 480;
    }
    m_XCentre = m_XSize / 2;
    m_YCentre = m_YSize / 2;

    if (m_InitialisePending) {
	Initialise();
	m_InitialisePending = false;
	m_DoneFirstShow = true;
    }

    if (m_DoneFirstShow) {
	// FIXME: copied from NattyDrawNames()
	const int dv = 2;
	const int quantise = int(FONT_SIZE / dv);
	const int quantised_x = m_XSize / quantise;
	const int quantised_y = m_YSize / quantise;
	const size_t buffer_size = quantised_x * quantised_y;
	if (m_LabelGrid) delete[] m_LabelGrid;

	m_LabelGrid = new char[buffer_size];
	CreateHitTestGrid();

#ifndef __WXMOTIF__
	m_DrawDC.SelectObject(wxNullBitmap);
#endif
	if (m_OffscreenBitmap) {
	    delete m_OffscreenBitmap;
	}
	m_OffscreenBitmap = new wxBitmap;
	m_OffscreenBitmap->Create(m_XSize, m_YSize);
	m_DrawDC.SelectObject(*m_OffscreenBitmap);
	RedrawOffscreen();
	Refresh(false);
    }
}

void GfxCore::OnDisplayOverlappingNames()
{
    m_OverlappingNames = !m_OverlappingNames;
    ForceRefresh();
}

void GfxCore::OnDisplayOverlappingNamesUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL && m_Names);
    cmd.Check(m_OverlappingNames);
}

void GfxCore::OnShowCrosses()
{
    m_Crosses = !m_Crosses;
    ForceRefresh();
}

void GfxCore::OnShowCrossesUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL);
    cmd.Check(m_Crosses);
}

void GfxCore::OnShowStationNames()
{
    m_Names = !m_Names;
    ForceRefresh();
}

void GfxCore::OnShowStationNamesUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL);
    cmd.Check(m_Names);
}

void GfxCore::OnShowSurveyLegs()
{
    m_Legs = !m_Legs;
    ForceRefresh();
}

void GfxCore::OnShowSurveyLegsUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL && m_Lock != lock_POINT && m_UndergroundLegs);
    cmd.Check(m_Legs);
}

void GfxCore::OnMoveEast()
{
    TurnCaveTo(M_PI_2);
    ForceRefresh();
}

void GfxCore::OnMoveEastUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL && !(m_Lock & lock_Y));
}

void GfxCore::OnMoveNorth()
{
    TurnCaveTo(0.0);
    ForceRefresh();
}

void GfxCore::OnMoveNorthUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL && !(m_Lock & lock_X));
}

void GfxCore::OnMoveSouth()
{
    TurnCaveTo(M_PI);
    ForceRefresh();
}

void GfxCore::OnMoveSouthUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL && !(m_Lock & lock_X));
}

void GfxCore::OnMoveWest()
{
    TurnCaveTo(M_PI * 1.5);
    ForceRefresh();
}

void GfxCore::OnMoveWestUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL && !(m_Lock & lock_Y));
}

void GfxCore::OnStartRotation()
{
    m_Rotating = true;
    timer.Start(drawtime);
}

void GfxCore::OnStartRotationUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL && !m_Rotating && m_RotationOK);
}

void GfxCore::OnToggleRotation()
{
    m_Rotating = !m_Rotating;
    if (m_Rotating) timer.Start(drawtime);
}

void GfxCore::OnToggleRotationUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL && m_RotationOK);
    cmd.Check(m_PlotData != NULL && m_Rotating);
}

void GfxCore::OnStopRotation()
{
    m_Rotating = false;
}

void GfxCore::OnStopRotationUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL && m_Rotating);
}

void GfxCore::OnReverseControls()
{
    m_ReverseControls = !m_ReverseControls;
}

void GfxCore::OnReverseControlsUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL);
    cmd.Check(m_ReverseControls);
}

void GfxCore::OnReverseDirectionOfRotation()
{
    m_RotationStep = -m_RotationStep;
}

void GfxCore::OnReverseDirectionOfRotationUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL && m_RotationOK);
}

void GfxCore::OnSlowDown(bool accel)
{
    m_RotationStep /= accel ? 1.44 : 1.2;
    if (m_RotationStep < M_PI / 180.0) {
	m_RotationStep = (Double) M_PI / 180.0;
    }
}

void GfxCore::OnSlowDownUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL && m_RotationOK);
}

void GfxCore::OnSpeedUp(bool accel)
{
    m_RotationStep *= accel ? 1.44 : 1.2;
    if (m_RotationStep > 2.5 * M_PI) {
	m_RotationStep = (Double) 2.5 * M_PI;
    }
}

void GfxCore::OnSpeedUpUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL && m_RotationOK);
}

void GfxCore::OnStepOnceAnticlockwise(bool accel)
{
    TurnCave(accel ? 5.0 * M_PI / 18.0 : M_PI / 18.0);
    ForceRefresh();
}

void GfxCore::OnStepOnceAnticlockwiseUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL && m_RotationOK && !m_Rotating);
}

void GfxCore::OnStepOnceClockwise(bool accel)
{
    TurnCave(accel ? 5.0 * -M_PI / 18.0 : -M_PI / 18.0);
    ForceRefresh();
}

void GfxCore::OnStepOnceClockwiseUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL && m_RotationOK && !m_Rotating);
}

void GfxCore::OnDefaults()
{
    m_TiltAngle = M_PI_2;
    m_PanAngle = 0.0;
    switch (m_Lock) {
	case lock_X:
	{
	    // elevation looking along X axis (East)
	    m_PanAngle = M_PI * 1.5;
	    m_TiltAngle = 0.0;
	    m_RotationOK = false;
	    break;
	}

	case lock_Y:
	case lock_XY: // survey is linearface and parallel to the Z axis => display in elevation.
	    // elevation looking along Y axis (North)
	    m_TiltAngle = 0.0;
	    m_RotationOK = false;
	    break;

	case lock_Z:
	case lock_XZ: // linearface survey parallel to Y axis
	case lock_YZ: // linearface survey parallel to X axis
	{
	    // flat survey (zero height range) => go into plan view (default orientation).
	    break;
	}

	case lock_POINT:
	    m_RotationOK = false;
	    m_Crosses = true;
	    break;

	case lock_NONE:
	    break;
    }

    m_Params.rotation.setFromEulerAngles(m_TiltAngle, 0.0, m_PanAngle);
    m_RotationMatrix = m_Params.rotation.asMatrix();

    m_Params.translation.x = 0.0;
    m_Params.translation.y = 0.0;
    m_Params.translation.z = 0.0;

    m_Surface = false;
    m_SurfaceDepth = false;
    m_SurfaceDashed = true;
    m_RotationStep = M_PI / 6.0;
    m_Rotating = false;
    m_SwitchingTo = 0;
    m_Entrances = false;
    m_FixedPts = false;
    m_ExportedPts = false;
    m_Grid = false;
    SetScale(m_InitialScale);
    DefaultParameters();
    ForceRefresh();
}

void GfxCore::OnDefaultsUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL);
}

void GfxCore::OnElevation()
{
    // Switch to elevation view.
    switch (m_SwitchingTo) {
	case 0:
	    timer.Start(drawtime);
	    m_SwitchingTo = ELEVATION;
	    break;
	case PLAN:
	    m_SwitchingTo = ELEVATION;
	    break;
	case ELEVATION:
	    // A second order to switch takes us there right away
	    TiltCave(-m_TiltAngle);
	    m_SwitchingTo = 0;
	    ForceRefresh();
    } 
}

void GfxCore::OnElevationUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL && m_Lock == lock_NONE && m_TiltAngle != 0.0);
}

void GfxCore::OnHigherViewpoint(bool accel)
{
    // Raise the viewpoint.
    TiltCave(accel ? 5.0 * M_PI / 18.0 : M_PI / 18.0);
    ForceRefresh();
}

void GfxCore::OnHigherViewpointUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL && m_TiltAngle < M_PI_2 &&
	       m_Lock == lock_NONE);
}

void GfxCore::OnLowerViewpoint(bool accel)
{
    // Lower the viewpoint.
    TiltCave(accel ? 5.0 * -M_PI / 18.0 : -M_PI / 18.0);
    ForceRefresh();
}

void GfxCore::OnLowerViewpointUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL && m_TiltAngle > -M_PI_2 &&
	       m_Lock == lock_NONE);
}

void GfxCore::OnPlan()
{
    // Switch to plan view.
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
	    TiltCave(M_PI_2 - m_TiltAngle);
	    m_SwitchingTo = 0;
	    ForceRefresh();
    } 
}

void GfxCore::OnPlanUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL && m_Lock == lock_NONE &&
	       m_TiltAngle != M_PI_2);
}

void GfxCore::OnShiftDisplayDown(bool accel)
{
    TranslateCave(0, accel ? 5 * DISPLAY_SHIFT : DISPLAY_SHIFT);
}

void GfxCore::OnShiftDisplayDownUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL);
}

void GfxCore::OnShiftDisplayLeft(bool accel)
{
    TranslateCave(accel ? -5 * DISPLAY_SHIFT : -DISPLAY_SHIFT, 0);
}

void GfxCore::OnShiftDisplayLeftUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL);
}

void GfxCore::OnShiftDisplayRight(bool accel)
{
    TranslateCave(accel ? 5 * DISPLAY_SHIFT : DISPLAY_SHIFT, 0);
}

void GfxCore::OnShiftDisplayRightUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL);
}

void GfxCore::OnShiftDisplayUp(bool accel)
{
    TranslateCave(0, accel ? -5 * DISPLAY_SHIFT : -DISPLAY_SHIFT);
}

void GfxCore::OnShiftDisplayUpUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL);
}

void GfxCore::OnZoomIn(bool accel)
{
    // Increase the scale.

    //--GL fixes needed
    SetScale(m_Params.scale * (accel ? 1.1236 : 1.06));
    ForceRefresh();
}

void GfxCore::OnZoomInUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL && m_Lock != lock_POINT);
}

void GfxCore::OnZoomOut(bool accel)
{
    // Decrease the scale.

    SetScale(m_Params.scale / (accel ? 1.1236 : 1.06));
    ForceRefresh();
}

void GfxCore::OnZoomOutUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL && m_Lock != lock_POINT);
}

void GfxCore::OnIdle(wxIdleEvent& event)
{
    // Handle an idle event.
    if (Animate(&event)) ForceRefresh();
}

// idle_event is a pointer to and idle event (to call RequestMore()) or NULL
// return: true if animation occured (and ForceRefresh() needs to be called)
bool GfxCore::Animate(wxIdleEvent *idle_event)
{
    idle_event = idle_event; // Suppress "not used" warning

    if (!m_Rotating && !m_SwitchingTo) {
	return false;
    }

    static double last_t = 0;
    double t = timer.Time() * 1.0e-3;
//    cout << 1.0 / t << " fps (i.e. " << t << " sec)\n";
    if (t == 0) t = 0.001;
    else if (t > 1.0) t = 1.0;
    if (last_t > 0) t = (t + last_t) / 2;
    last_t = t;

    // When rotating...
    if (m_Rotating) {
	TurnCave(m_RotationStep * t);
    }

    if (m_SwitchingTo == PLAN) {
	// When switching to plan view...
	TiltCave(M_PI_2 * t);
	if (m_TiltAngle == M_PI_2) m_SwitchingTo = 0;
    } else if (m_SwitchingTo == ELEVATION) {
	// When switching to elevation view...
	if (fabs(m_TiltAngle) < M_PI_2 * t) {
	    TiltCave(-m_TiltAngle);
	    m_SwitchingTo = 0;
	} else if (m_TiltAngle < 0.0) {
	    TiltCave(M_PI_2 * t);
	} else {
	    TiltCave(-M_PI_2 * t);
	}
    }

    return true;
}

void GfxCore::OnToggleScalebar()
{
    m_Scalebar = !m_Scalebar;
    ForceRefresh();
}

void GfxCore::OnToggleScalebarUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL && m_Lock != lock_POINT);
    cmd.Check(m_Scalebar);
}

void GfxCore::OnToggleDepthbar()
{
    m_Depthbar = !m_Depthbar;
    ForceRefresh();
}

void GfxCore::OnToggleDepthbarUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL && !(m_Lock && lock_Z));
    cmd.Check(m_Depthbar);
}

void GfxCore::OnViewCompass()
{
    m_Compass = !m_Compass;
    ForceRefresh();
}

void GfxCore::OnViewCompassUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL && m_RotationOK);
    cmd.Check(m_Compass);
}

void GfxCore::OnViewClino()
{
    m_Clino = !m_Clino;
    ForceRefresh();
}

void GfxCore::OnViewClinoUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL && m_Lock == lock_NONE);
    cmd.Check(m_Clino);
}

void GfxCore::OnShowSurface()
{
    m_Surface = !m_Surface;
    ForceRefresh();
}

void GfxCore::OnShowSurfaceDepth()
{
    m_SurfaceDepth = !m_SurfaceDepth;
    ForceRefresh();
}

void GfxCore::OnShowSurfaceDashed()
{
    m_SurfaceDashed = !m_SurfaceDashed;
    ForceRefresh();
}

void GfxCore::OnShowSurfaceUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData && m_SurfaceLegs);
    cmd.Check(m_Surface);
}

void GfxCore::OnShowSurfaceDepthUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData && m_Surface);
    cmd.Check(m_SurfaceDepth);
}

void GfxCore::OnShowSurfaceDashedUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData && m_SurfaceLegs && m_Surface);
    cmd.Check(m_SurfaceDashed);
}

void GfxCore::OnShowEntrances()
{
    m_Entrances = !m_Entrances;
    ForceRefresh();
}

void GfxCore::OnShowEntrancesUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData && (m_Parent->GetNumEntrances() > 0));
    cmd.Check(m_Entrances);
}

void GfxCore::OnShowFixedPts()
{
    m_FixedPts = !m_FixedPts;
    ForceRefresh();
}

void GfxCore::OnShowFixedPtsUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData && (m_Parent->GetNumFixedPts() > 0));
    cmd.Check(m_FixedPts);
}

void GfxCore::OnShowExportedPts()
{
    m_ExportedPts = !m_ExportedPts;
    ForceRefresh();
}

void GfxCore::OnShowExportedPtsUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData && (m_Parent->GetNumExportedPts() > 0));
    cmd.Check(m_ExportedPts);
}

void GfxCore::OnViewGrid()
{
    m_Grid = !m_Grid;
    ForceRefresh();
}

void GfxCore::OnViewGridUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData);
}

void GfxCore::OnIndicatorsUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData);
}

void GfxCore::OnToggleMetric()
{
    m_Metric = !m_Metric;
    wxConfigBase::Get()->Write("metric", m_Metric);
    wxConfigBase::Get()->Flush();
    ForceRefresh();
}

void GfxCore::OnToggleMetricUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData);
    cmd.Check(m_Metric);
}

void GfxCore::OnToggleDegrees()
{
    m_Degrees = !m_Degrees;
    wxConfigBase::Get()->Write("degrees", m_Degrees);
    wxConfigBase::Get()->Flush();
    ForceRefresh();
}

void GfxCore::OnToggleDegreesUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData);
    cmd.Check(m_Degrees);
}

//
//  OpenGL-specific methods
//

void GfxCore::CentreOn(Double x, Double y, Double z)
{
    m_Params.translation.x = -x;
    m_Params.translation.y = -y;
    m_Params.translation.z = -z;
    ForceRefresh();
}

void GfxCore::ForceRefresh()
{
    // Redraw the offscreen bitmap
    m_RedrawOffscreen = true;
    Refresh(false);
}

void GfxCore::RefreshLine(const Point &a1, const Point &b1,
			  const Point &a2, const Point &b2)
{
    // Calculate the minimum rectangle which includes the old and new
    // measuring lines to minimise the redraw time
    int l = INT_MAX, r = INT_MIN, u = INT_MIN, d = INT_MAX;
    if (a1.x != DBL_MAX) {
	int x = (int)GridXToScreen(a1);
	int y = (int)GridYToScreen(a1);
	l = x - HIGHLIGHTED_PT_SIZE * 4;
	r = x + HIGHLIGHTED_PT_SIZE * 4;
	u = y + HIGHLIGHTED_PT_SIZE * 4;
	d = y - HIGHLIGHTED_PT_SIZE * 4;
    }
    if (a2.x != DBL_MAX) {
	int x = (int)GridXToScreen(a2);
	int y = (int)GridYToScreen(a2);
	l = min(l, x - HIGHLIGHTED_PT_SIZE * 4);
	r = max(r, x + HIGHLIGHTED_PT_SIZE * 4);
	u = max(u, y + HIGHLIGHTED_PT_SIZE * 4);
	d = min(d, y - HIGHLIGHTED_PT_SIZE * 4);
    }
    if (b1.x != DBL_MAX) {
	int x = (int)GridXToScreen(b1);
	int y = (int)GridYToScreen(b1);
	l = min(l, x - HIGHLIGHTED_PT_SIZE * 4);
	r = max(r, x + HIGHLIGHTED_PT_SIZE * 4);
	u = max(u, y + HIGHLIGHTED_PT_SIZE * 4);
	d = min(d, y - HIGHLIGHTED_PT_SIZE * 4);
    }
    if (b2.x != DBL_MAX) {
	int x = (int)GridXToScreen(b2);
	int y = (int)GridYToScreen(b2);
	l = min(l, x - HIGHLIGHTED_PT_SIZE * 4);
	r = max(r, x + HIGHLIGHTED_PT_SIZE * 4);
	u = max(u, y + HIGHLIGHTED_PT_SIZE * 4);
	d = min(d, y - HIGHLIGHTED_PT_SIZE * 4);
    }
    const wxRect R(l, d, r - l, u - d);
    Refresh(false, &R);
}

void GfxCore::SetHere()
{
//    if (m_here.x == DBL_MAX) return;
    Point old = m_here;
    m_here.x = DBL_MAX;
    RefreshLine(old, m_there, m_here, m_there);
}

void GfxCore::SetHere(Double x, Double y, Double z)
{
    Point old = m_here;
    m_here.x = x;
    m_here.y = y;
    m_here.z = z;
    RefreshLine(old, m_there, m_here, m_there);
}

void GfxCore::SetThere()
{
//    if (m_there.x == DBL_MAX) return;
    Point old = m_there;
    m_there.x = DBL_MAX;
    RefreshLine(m_here, old, m_here, m_there);
}

void GfxCore::SetThere(Double x, Double y, Double z)
{
    Point old = m_there;
    m_there.x = x;
    m_there.y = y;
    m_there.z = z;
    RefreshLine(m_here, old, m_here, m_there);
}

void GfxCore::OnCancelDistLine()
{
    m_Parent->ClearTreeSelection();
}

void GfxCore::OnCancelDistLineUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_there.x != DBL_MAX);
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
	LabelInfo *label = *pos++;

	if (!((m_Surface && label->IsSurface()) ||
	      (m_Legs && label->IsUnderground()) ||
	      (!label->IsSurface() && !label->IsUnderground()))) {
	    // if this station isn't to be displayed, skip to the next
	    // (last case is for stns with no legs attached)
	    continue;
	}

	// Calculate screen coordinates.
	int cx = (int)GridXToScreen(label->GetX(), label->GetY(), label->GetZ());
 	if (cx < 0 || cx >= m_XSize) continue;
	int cy = (int)GridYToScreen(label->GetX(), label->GetY(), label->GetZ());
 	if (cy < 0 || cy >= m_YSize) continue;

	// On-screen, so add to hit-test grid...
	int grid_x = (cx * (HITTEST_SIZE - 1)) / m_XSize;
	int grid_y = (cy * (HITTEST_SIZE - 1)) / m_YSize;

	m_PointGrid[grid_x + grid_y * HITTEST_SIZE].push_back(label);
    }

    m_HitTestGridValid = true;
}

void GfxCore::OnKeyPress(wxKeyEvent &e)
{
    if (!m_PlotData) {
	e.Skip();
	return;
    }

    m_RedrawOffscreen = Animate();

    switch (e.m_keyCode) {
	case '/': case '?':
	    if (m_TiltAngle > -M_PI_2 && m_Lock == lock_NONE)
		OnLowerViewpoint(e.m_shiftDown);
	    break;
	case '\'': case '@': case '"': // both shifted forms - US and UK kbd
	    if (m_TiltAngle < M_PI_2 && m_Lock == lock_NONE)
		OnHigherViewpoint(e.m_shiftDown);
	    break;
	case 'C': case 'c':
	    if (m_RotationOK && !m_Rotating)
		OnStepOnceAnticlockwise(e.m_shiftDown);
	    break;
	case 'V': case 'v':
	    if (m_RotationOK && !m_Rotating)
		OnStepOnceClockwise(e.m_shiftDown);
	    break;
	case ']': case '}':
	    if (m_Lock != lock_POINT)
		OnZoomIn(e.m_shiftDown);
	    break;
	case '[': case '{':
	    if (m_Lock != lock_POINT)
		OnZoomOut(e.m_shiftDown);
	    break;
	case 'N': case 'n':
	    if (!(m_Lock & lock_X))
		OnMoveNorth();
	    break;
	case 'S': case 's':
	    if (!(m_Lock & lock_X))
		OnMoveSouth();
	    break;
	case 'E': case 'e':
	    if (!(m_Lock & lock_Y))
		OnMoveEast();
	    break;
	case 'W': case 'w':
	    if (!(m_Lock & lock_Y))
		OnMoveWest();
	    break;
	case 'Z': case 'z':
	    if (m_RotationOK)
		OnSpeedUp(e.m_shiftDown);
	    break;
	case 'X': case 'x':
	    if (m_RotationOK)
		OnSlowDown(e.m_shiftDown);
	    break;
	case 'R': case 'r':
	    if (m_RotationOK)
		OnReverseDirectionOfRotation();
	    break;
	case 'P': case 'p':
	    if (m_Lock == lock_NONE && m_TiltAngle != M_PI_2)
		OnPlan();
	    break;
	case 'L': case 'l':
	    if (m_Lock == lock_NONE && m_TiltAngle != 0.0)
		OnElevation();
	    break;
	case 'O': case 'o':
	    OnDisplayOverlappingNames();
	    break;
	case WXK_DELETE:
	    OnDefaults();
	    break;
	case WXK_RETURN:
	    if (m_RotationOK && !m_Rotating)
		OnStartRotation();
	    break;
	case WXK_SPACE:
	    if (m_Rotating)
		OnStopRotation();
	    break;
	case WXK_LEFT:
	    if (e.m_controlDown) {
		if (m_RotationOK && !m_Rotating)
		    OnStepOnceAnticlockwise(e.m_shiftDown);
	    } else {
		OnShiftDisplayLeft(e.m_shiftDown);
	    }
	    break;
	case WXK_RIGHT:
	    if (e.m_controlDown) {
		if (m_RotationOK && !m_Rotating)
		    OnStepOnceClockwise(e.m_shiftDown);
	    } else {
		OnShiftDisplayRight(e.m_shiftDown);
	    }
	    break;
	case WXK_UP:
	    if (e.m_controlDown) {
		if (m_TiltAngle < M_PI_2 && m_Lock == lock_NONE)
		    OnHigherViewpoint(e.m_shiftDown);
	    } else {
		OnShiftDisplayUp(e.m_shiftDown);
	    }
	    break;
	case WXK_DOWN:
	    if (e.m_controlDown) {
		if (m_TiltAngle > -M_PI_2 && m_Lock == lock_NONE)
		    OnLowerViewpoint(e.m_shiftDown);
	    } else {
		OnShiftDisplayDown(e.m_shiftDown);
	    }
	    break;
	case WXK_ESCAPE:
	    if (m_there.x != DBL_MAX)
		OnCancelDistLine();
	    break;
	default:
	    e.Skip();
    }
 
    // OnPaint clears m_RedrawOffscreen so it'll only still be set if we need
    // to redraw
    if (m_RedrawOffscreen) ForceRefresh();
}
