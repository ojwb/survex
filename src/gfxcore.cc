//
//  gfxcore.cc
//
//  Core drawing control code for Aven.
//
//  Copyright (C) 2000-2002 Mark R. Shinwell
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

#include <float.h>

#include "aven.h"
#include "gfxcore.h"
#include "mainfrm.h"
#include "message.h"
#include "useful.h"
#include "guicontrol.h"

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

#ifdef AVENGL
static void* LABEL_FONT = GLUT_BITMAP_HELVETICA_10;
static const Double SURFACE_ALPHA = 0.6;
#endif

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

#ifdef AVENGL
BEGIN_EVENT_TABLE(GfxCore, wxGLCanvas)
#else
BEGIN_EVENT_TABLE(GfxCore, wxWindow)
#endif
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

GfxCore::GfxCore(MainFrm* parent, wxWindow* parent_win, GUIControl* control) :
#ifdef AVENGL
    wxGLCanvas(parent_win, 100, wxDefaultPosition, wxSize(640, 480)),
#else
    wxWindow(parent_win, 100, wxDefaultPosition, wxSize(640, 480)),
#endif
    m_Font(FONT_SIZE, wxSWISS, wxNORMAL, wxNORMAL, FALSE, "Helvetica",
	   wxFONTENCODING_ISO8859_1),
    m_InitialisePending(false)
{
    m_Control = control;
    m_OffscreenBitmap = NULL;
    m_ScaleBar.offset_x = SCALE_BAR_OFFSET_X;
    m_ScaleBar.offset_y = SCALE_BAR_OFFSET_Y;
    m_ScaleBar.width = 0;
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
#ifdef AVENPRES
    m_DoingPresStep = -1;
#endif
    clipping = false;

#ifdef AVENGL
    m_TerrainLoaded = false;
    m_AntiAlias = false;
    m_SolidSurface = false;
#else
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
#endif

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

#ifndef AVENGL
    DELETE_ARRAY(m_Pens);
    DELETE_ARRAY(m_Brushes);
#endif

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

#ifdef AVENGL
    m_TerrainLoaded = false;
#endif

#ifdef AVENPRES
    m_DoingPresStep = -1; //--Pres: FIXME: delete old lists
#endif

    // Apply default parameters.
    DefaultParameters();

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

    switch (m_Lock) {
	case lock_X:
	{
	    // elevation looking along X axis (East)
	    m_PanAngle = M_PI * 1.5;

	    Quaternion q;
	    q.setFromEulerAngles(0.0, 0.0, m_PanAngle);

	    m_Params.rotation = q * m_Params.rotation;
	    m_RotationMatrix = m_Params.rotation.asMatrix();
	    m_RotationOK = false;
	    break;
	}

	case lock_Y:
	case lock_XY: // survey is linearface and parallel to the Z axis => display in elevation.
	    // elevation looking along Y axis (North)
	    m_Params.rotation.setFromEulerAngles(0.0, 0.0, 0.0);
	    m_RotationMatrix = m_Params.rotation.asMatrix();
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

    // Scale the survey to a reasonable initial size.
#ifdef AVENGL
    m_InitialScale = 1.0;
#else
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
#endif

    // Calculate screen coordinates and redraw.
    SetScaleInitial(m_InitialScale);
    ForceRefresh();
}

void GfxCore::FirstShow()
{
    // Update our record of the client area size and centre.
    GetClientSize(&m_XSize, &m_YSize);
    m_XCentre = m_XSize / 2;
    m_YCentre = m_YSize / 2;

#ifdef AVENGL
    glEnable(GL_DEPTH_TEST);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    CheckGLError("enabling features for survey legs");
#else
    // Create the offscreen bitmap.
    m_OffscreenBitmap = new wxBitmap;
    m_OffscreenBitmap->Create(m_XSize, m_YSize);

    m_DrawDC.SelectObject(*m_OffscreenBitmap);
#endif

    m_DoneFirstShow = true;

    RedrawOffscreen();
}

//
//  Recalculating methods
//

void GfxCore::SetScaleInitial(Double scale)
{
    SetScale(scale);

#ifdef AVENGL
    DrawGrid();
#endif

    if (true) {
	// Invalidate hit-test grid.
	m_HitTestGridValid = false;

#ifdef AVENGL
	// With OpenGL we have to make three passes, as OpenGL lists are
	// immutable and we need the surface and underground data in different
	// lists.  The third pass is so we get different lists for surface data
	// split into depth bands, and not split like that.  This isn't a
	// problem as this routine is only called once in the OpenGL version
	// and it contains very little in the way of calculations for this
	// version.
	for (int pass = 0; pass < 3; pass++) {
	    // 1st pass -> u/g data; 2nd pass -> surface (uniform);
	    // 3rd pass -> surface (w/depth colouring)
	    //--should delete any old GL list. (only a prob on reinit I think)
	    if (pass == 0) {
		CheckGLError("before allocating survey list");
		m_Lists.survey = glGenLists(1);
		CheckGLError("immediately after allocating survey list");
		glNewList(m_Lists.survey, GL_COMPILE);
		CheckGLError("creating survey list");
	    }
	    else if (pass == 1) {
		m_Lists.surface = glGenLists(1);
		glNewList(m_Lists.surface, GL_COMPILE);
		CheckGLError("creating surface-nodepth list");
	    }
	    else {
		m_Lists.surface_depth = glGenLists(1);
		glNewList(m_Lists.surface_depth, GL_COMPILE);
		CheckGLError("creating surface-depth list");
	    }
#endif
	for (int band = 0; band < m_Bands; band++) {
#ifdef AVENGL
	    Double r, g, b;
	    if (pass == 0 || pass == 2) {
		m_Parent->GetColour(band, r, g, b);
		glColor3d(r, g, b);
		CheckGLError("setting survey colour");
	    }
	    else {
		glColor3d(1.0, 1.0, 1.0);
		CheckGLError("setting surface survey colour");
	    }
#else
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
#endif
	    Double x, y, z;

	    list<PointInfo*>::iterator pos = m_Parent->GetPointsNC(band);
	    list<PointInfo*>::iterator end = m_Parent->GetPointsEndNC(band);
	    bool first_point = true;
	    bool last_was_move = true;
	    bool current_polyline_is_surface = false;
#ifdef AVENGL
	    bool line_open = false;
#endif
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
#ifdef AVENGL
			if ((current_polyline_is_surface && pass > 0) ||
			    (!current_polyline_is_surface && pass == 0)) {
			    line_open = true;
			    glBegin(GL_LINE_STRIP);
			    glVertex3d(x, y, z);
			    CheckGLError("survey leg vertex");
			}
#else
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
#endif
		    }

#ifdef AVENGL
		    if ((current_polyline_is_surface && pass > 0) ||
			(!current_polyline_is_surface && pass == 0)) {
			assert(line_open);
			glVertex3d(x, y, z);
			CheckGLError("survey leg vertex");
			if (pass == 0) {
			    m_UndergroundLegs = true;
			}
			else {
			    m_SurfaceLegs = true;
			}
		    }
#else
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
#endif
		    last_was_move = false;
		}
		else {
#ifdef AVENGL
		    if (line_open) {
			//glVertex3d(x, y, z);
			glEnd();
			CheckGLError("closing survey leg strip");
			line_open = false;
		    }
#endif
		    first_point = false;
		    last_was_move = true;

		    // Save the current coordinates for the next time around
		    // the loop.
		    x = pti->GetX();
		    y = pti->GetY();
		    z = pti->GetZ();
		}
	    }
#ifndef AVENGL
	    if (!m_UndergroundLegs) {
		m_UndergroundLegs = (m_Polylines[band] > 0);
	    }
	    if (!m_SurfaceLegs) {
		m_SurfaceLegs = (m_SurfacePolylines[band] > 0);
	    }
#else
	    if (line_open) {
		glEnd();
		CheckGLError("closing survey leg strip (2)");
	    }
	}

	glEndList();
	CheckGLError("ending survey leg list");
#endif
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
//  Event handlers
//

void GfxCore::OnIdle(wxIdleEvent& event)
{
    // Handle an idle event.

    if (Animate(&event)) {
	ForceRefresh();
    }
}

void GfxCore::OnPaint(wxPaintEvent& event)
{
    // Redraw the window.

    // Get a graphics context.
    wxPaintDC dc(this);

#ifdef AVENGL
    SetCurrent();
#endif

    // Make sure we're initialised.
    if (!m_DoneFirstShow) {
	FirstShow();
    }

    // Redraw the offscreen bitmap if it's out of date.
    if (m_RedrawOffscreen) {
	m_RedrawOffscreen = false;
	RedrawOffscreen();
    }

#ifdef AVENGL
    if (m_PlotData) {
	// Clear the background.
	ClearBackgroundAndBuffers();

	// Set up projection matrix.
	SetGLProjection();

	// Set up model transformation matrix.
	SetModellingTransformation();

	if (m_Legs) {
	    // Draw the underground legs.
	    glCallList(m_Lists.survey);
	}

	if (m_Surface) {
	    // Draw the surface legs.

	    if (m_SurfaceDashed) {
		glLineStipple(1, 0xaaaa);
		glEnable(GL_LINE_STIPPLE);
	    }

	    glCallList(m_SurfaceDepth ? m_Lists.surface_depth : m_Lists.surface);

	    if (m_SurfaceDashed) {
		glDisable(GL_LINE_STIPPLE);
	    }
	}

	if (m_Grid) {
	    // Draw the grid.
	    glCallList(m_Lists.grid);
	}

	if (m_TerrainLoaded && m_SolidSurface) {
	    // Draw the terrain.

	    // Underside...
	    glDisable(GL_BLEND);
	    glEnable(GL_CULL_FACE);
	    glCullFace(GL_FRONT);
	    if (floor_alt + m_Parent->GetZOffset() < m_Parent->GetTerrainMaxZ()) {
		glCallList(m_Lists.terrain);
	    } else {
		glTranslated(0, 0, floor_alt + m_Parent->GetZOffset() - m_Parent->GetTerrainMaxZ());
		glCallList(m_Lists.flat_terrain);
	    }
	    glEnable(GL_BLEND);

	    // Topside...
	    glCullFace(GL_BACK);
	    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	    glEnable(GL_TEXTURE_2D);
	    if (floor_alt + m_Parent->GetZOffset() < m_Parent->GetTerrainMaxZ()) {
		glCallList(m_Lists.terrain);
	    } else {
		glTranslated(0, 0, floor_alt + m_Parent->GetZOffset() - m_Parent->GetTerrainMaxZ());
		glCallList(m_Lists.flat_terrain);
	    }
	    glDisable(GL_CULL_FACE);
	    glDisable(GL_BLEND);
	    glDisable(GL_TEXTURE_2D);
/*
	    // Map...
	    glCallList(m_Lists.map);
	    glDisable(GL_TEXTURE_2D);
	    glDisable(GL_BLEND);*/
	}

	//--FIXME: share with above

	// Draw station names.
	if (m_Names) {
	    glDisable(GL_DEPTH_TEST);
	    DrawNames();
	    glEnable(GL_DEPTH_TEST);
	}

	// Draw scalebar.
	if (m_Scalebar) {
	    DrawScalebar();
	}

	// Flush pipeline and swap buffers.
	glFlush();
	SwapBuffers();
    }
#else
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
    
    if (!m_Rotating && !m_SwitchingTo
#ifdef AVENPRES
	&& !(m_DoingPresStep >= 0 && m_DoingPresStep <= 100)
#endif
#ifdef AVENGL
	&& !(m_TerrainLoaded && floor_alt > -DBL_MAX && floor_alt <= HEAVEN)
#endif
	) {
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
#endif
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

#ifdef AVENGL
return;
  //    m_Lists.grid = glGenLists(1);
  //glNewList(m_Lists.grid, GL_COMPILE);
#endif

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
#ifdef AVENGL
	glBegin(GL_LINES);
	glVertex3d(x, bottom, grid_z);
	glVertex3d(x, actual_top, grid_z);
	glEnd();
#else
	m_DrawDC.DrawLine((int) GridXToScreen(x, bottom, grid_z), (int) GridYToScreen(x, bottom, grid_z),
			  (int) GridXToScreen(x, actual_top, grid_z), (int) GridYToScreen(x, actual_top, grid_z));
#endif
    }

    for (int yc = 0; yc <= count_y; yc++) {
	Double y = bottom + yc*grid_size;
#ifdef AVENGL
	glBegin(GL_LINES);
	glVertex3d(left, y, grid_z);
	glVertex3d(actual_right, y, grid_z);
	glEnd();
#else
	m_DrawDC.DrawLine((int) GridXToScreen(left, y, grid_z), (int) GridYToScreen(left, y, grid_z),
			  (int) GridXToScreen(actual_right, y, grid_z),
			  (int) GridYToScreen(actual_right, y, grid_z));
#endif
    }
#ifdef AVENGL
    glEndList();
#endif
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
    bool white = false;
    /* FIXME m_Control->DraggingMouseOutsideCompass();
    m_DraggingLeft && m_LastDrag == drag_COMPASS && m_MouseOutsideCompass; */
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
	white = false; /* FIXME m_DraggingLeft && m_LastDrag == drag_ELEV && m_MouseOutsideElev; */
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

#ifdef AVENGL

#else
    m_DrawDC.SetTextBackground(wxColour(0, 0, 0));
    m_DrawDC.SetTextForeground(LABEL_COLOUR);
#endif

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

#ifdef AVENGL
    // Get transformation matrices, etc. for gluProject().
    GLdouble modelview_matrix[16];
    GLdouble projection_matrix[16];
    GLint viewport[4];
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview_matrix);
    glGetDoublev(GL_PROJECTION_MATRIX, projection_matrix);
    glGetIntegerv(GL_VIEWPORT, viewport);
    glColor3f(0.0, 1.0, 0.0);//--FIXME
#endif

    while (label != m_Parent->GetLabelsEnd()) {
#ifdef AVENGL
	// Project the label's position onto the window.
	GLdouble x, y, z;
	int code = gluProject((*label)->x, (*label)->y, (*label)->z,
			      modelview_matrix, projection_matrix,
			      viewport, &x, &y, &z);
#else
	Double x = GridXToScreen((*label)->x, (*label)->y, (*label)->z);
	Double y = GridYToScreen((*label)->x, (*label)->y, (*label)->z)
	    + CROSS_SIZE - FONT_SIZE;
#endif

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
#ifdef AVENGL
		glRasterPos3f((*label)->x, (*label)->y, (*label)->z);
		for (int pos = 0; pos < str.Length(); pos++) {
		    glutBitmapCharacter(LABEL_FONT, (int) (str[pos]));
		}
#else
		m_DrawDC.DrawText(str, (wxCoord)x, (wxCoord)y);
#endif

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
    // Draw station names, possibly overlapping; or use previously-cached info
    // from NattyDrawNames() to draw names known not to overlap.

#ifndef AVENGL
    list<LabelInfo*>::const_iterator label = m_Parent->GetLabels();
    while (label != m_Parent->GetLabelsEnd()) {
	wxCoord x = (wxCoord)GridXToScreen((*label)->x, (*label)->y, (*label)->z);
	wxCoord y = (wxCoord)GridYToScreen((*label)->x, (*label)->y, (*label)->z);
	m_DrawDC.DrawText((*label)->GetText(), x, y + CROSS_SIZE - FONT_SIZE);
	++label;
    }
#endif
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
#ifdef AVENGL
    Double x_size = -m_Volume.left * 2.0;
#else
    int x_size = m_XSize;
#endif

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
#ifdef AVENGL
    Double size = size_snap * m_Params.scale;
#else
    int size = int(size_snap * m_Params.scale);
#endif
    m_ScaleBar.width = (int) size; //FIXME

    // Draw it...
    //--FIXME: improve this
#ifdef AVENGL
    Double end_x = m_Volume.left + m_ScaleBar.offset_x;
    Double height = (-m_Volume.bottom * 2.0) / 40.0;
    Double gl_z = m_Volume.nearface + 1.0; //-- is this OK??
    Double end_y = m_Volume.bottom + m_ScaleBar.offset_y - height;
    Double interval = size / 10.0;
#else
    int end_x = m_ScaleBar.offset_x;
    int height = SCALE_BAR_HEIGHT;
    int end_y = m_YSize - m_ScaleBar.offset_y - height;
    int interval = size / 10;
#endif

    bool solid = true;
#ifdef AVENGL
    glLoadIdentity();
    glBegin(GL_QUADS);
#endif
    for (int ix = 0; ix < 10; ix++) {
#ifdef AVENGL
	Double x = end_x + ix * ((Double) size / 10.0);
#else
	int x = end_x + int(ix * ((Double) size / 10.0));
#endif

	SetColour(solid ? col_GREY : col_WHITE);
	SetColour(solid ? col_GREY : col_WHITE, true);

#ifdef AVENGL
	glVertex3d(x, end_y, gl_z);
	glVertex3d(x + interval, end_y, gl_z);
	glVertex3d(x + interval, end_y + height, gl_z);
	glVertex3d(x, end_y + height, gl_z);
#else
	m_DrawDC.DrawRectangle(x, end_y, interval + 2, height);
#endif

	solid = !solid;
    }

    // Add labels.
    wxString str = FormatLength(size_snap);

#ifdef AVENGL
    glEnd();
#else
    m_DrawDC.SetTextBackground(wxColour(0, 0, 0));
    m_DrawDC.SetTextForeground(TEXT_COLOUR);
    m_DrawDC.DrawText("0", end_x, end_y - FONT_SIZE - 4);

    int text_width, text_height;
    m_DrawDC.GetTextExtent(str, &text_width, &text_height);
    m_DrawDC.DrawText(str, end_x + size - text_width, end_y - FONT_SIZE - 4);
#endif
}
void GfxCore::CheckHitTestGrid(wxPoint& point, bool centre)
{
#ifndef AVENGL
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
#endif
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

#ifdef AVENGL
	if (GetContext()) {
	    SetCurrent();
	    glViewport(0, 0, m_XSize, m_YSize);
	    SetGLProjection();
	}
#else
#ifndef __WXMOTIF__
	m_DrawDC.SelectObject(wxNullBitmap);
#endif
	if (m_OffscreenBitmap) {
	    delete m_OffscreenBitmap;
	}
	m_OffscreenBitmap = new wxBitmap;
	m_OffscreenBitmap->Create(m_XSize, m_YSize);
	m_DrawDC.SelectObject(*m_OffscreenBitmap);
#endif
	RedrawOffscreen();
	Refresh(false);
    }
}

void GfxCore::DefaultParameters()
{
    m_TiltAngle = M_PI_2;
    m_PanAngle = 0.0;

#ifdef AVENGL
    m_Params.rotation.setFromEulerAngles(m_TiltAngle - M_PI_2, 0.0, m_PanAngle);
    m_AntiAlias = false;
    m_SolidSurface = false;
    SetGLAntiAliasing();
#else
    m_Params.rotation.setFromEulerAngles(m_TiltAngle, 0.0, m_PanAngle);
#endif
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
}

void GfxCore::Defaults()
{
    // Restore default scale, rotation and translation parameters.

    DefaultParameters();
    SetScale(m_InitialScale);
    ForceRefresh();
}

// idle_event is a pointer to and idle event (to call RequestMore()) or NULL
// return: true if animation occured (and ForceRefresh() needs to be called)
bool GfxCore::Animate(wxIdleEvent *idle_event)
{
#if !defined(AVENPRES) && !defined(AVENGL)
    idle_event = idle_event; // Suppress "not used" warning
#endif

    if (!m_Rotating && !m_SwitchingTo
#ifdef AVENPRES
	&& !(m_DoingPresStep >= 0 && m_DoingPresStep <= 100)
#endif
#ifdef AVENGL
        && !(m_TerrainLoaded && floor_alt > -DBL_MAX && floor_alt <= HEAVEN)
#endif
       ) {
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

#ifdef AVENPRES
    if (m_DoingPresStep >= 0 && m_DoingPresStep <= 100) {
	m_Params.scale = INTERPOLATE(m_PresStep.from.scale, m_PresStep.to.scale, m_DoingPresStep);

	m_Params.translation.x = INTERPOLATE(m_PresStep.from.translation.x, m_PresStep.to.translation.x,
					     m_DoingPresStep);
	m_Params.translation.y = INTERPOLATE(m_PresStep.from.translation.y, m_PresStep.to.translation.y,
					     m_DoingPresStep);
	m_Params.translation.z = INTERPOLATE(m_PresStep.from.translation.z, m_PresStep.to.translation.z,
					     m_DoingPresStep);

	Double c = dot(m_PresStep.from.rotation.getVector(), m_PresStep.to.rotation.getVector()) +
		       m_PresStep.from.rotation.getScalar() * m_PresStep.to.rotation.getScalar();

	// adjust signs (if necessary)
	if (c < 0.0) {
	    c = -c;
	    m_PresStep.to.rotation = -m_PresStep.to.rotation;
	}

	Double p = Double(m_DoingPresStep) / 100.0;
	Double scale0;
	Double scale1;

	if ((1.0 - c) > 0.000001) {
	    Double omega = acos(c);
	    Double s = sin(omega);
	    scale0 = sin((1.0 - p) * omega) / s;
	    scale1 = sin(p * omega) / s;
	}
	else {
	    scale0 = 1.0 - p;
	    scale1 = p;
	}

	m_Params.rotation = scale0 * m_PresStep.from.rotation + scale1 * m_PresStep.to.rotation;
	m_RotationMatrix = m_Params.rotation.asMatrix();

	m_DoingPresStep += 30.0 * t;
	if (m_DoingPresStep <= 100.0) {
	    idle_event->RequestMore();
	}
	else {
	    m_PanAngle = m_PresStep.to.pan_angle;
	    m_TiltAngle = m_PresStep.to.tilt_angle;
	}
    }
#endif

#ifdef AVENGL
    if (m_TerrainLoaded && floor_alt > -DBL_MAX && floor_alt <= HEAVEN) {
	// FIXME: check scaling on t - this is an educated guess...
	if (terrain_rising) {
	    floor_alt += 200.0 * t;
	} else {
	    floor_alt -= 200.0 * t;
	}
	InitialiseTerrain();
	idle_event->RequestMore();
    }
#endif

    return true;
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

//
//  Methods for controlling the orientation of the survey
//

void GfxCore::TurnCave(Double angle)
{
    // Turn the cave around its z-axis by a given angle.
    Vector3 v(XToScreen(0.0, 0.0, 1.0),
	      YToScreen(0.0, 0.0, 1.0),
	      ZToScreen(0.0, 0.0, 1.0));
    Quaternion q(v, angle);

    m_Params.rotation = q * m_Params.rotation;
    m_RotationMatrix = m_Params.rotation.asMatrix();

    m_PanAngle += angle;
    if (m_PanAngle >= M_PI * 2.0) {
	m_PanAngle -= M_PI * 2.0;
    } else if (m_PanAngle < 0.0) {
	m_PanAngle += M_PI * 2.0;
    }
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

    Quaternion q;
    q.setFromEulerAngles(tilt_angle, 0.0, 0.0);

    m_Params.rotation = q * m_Params.rotation;
    m_RotationMatrix = m_Params.rotation.asMatrix();
}

void GfxCore::TranslateCave(int dx, int dy)
{
    // Find out how far the screen movement takes us in cave coords.
    Double x = Double(dx / m_Params.scale);
    Double z = Double(-dy / m_Params.scale);

#ifdef AVENGL
    x *= (m_MaxExtent / m_XSize);
    z *= (m_MaxExtent * 0.75 / m_YSize);
#endif

    Matrix4 inverse_rotation = m_Params.rotation.asInverseMatrix();

#ifdef AVENGL
    Double cx = Double(inverse_rotation.get(0, 0)*x + inverse_rotation.get(0, 1)*z);
    Double cy = Double(inverse_rotation.get(1, 0)*x + inverse_rotation.get(1, 1)*z);
    Double cz = Double(inverse_rotation.get(2, 0)*x + inverse_rotation.get(2, 1)*z);
#else
    Double cx = Double(inverse_rotation.get(0, 0)*x + inverse_rotation.get(0, 2)*z);
    Double cy = Double(inverse_rotation.get(1, 0)*x + inverse_rotation.get(1, 2)*z);
    Double cz = Double(inverse_rotation.get(2, 0)*x + inverse_rotation.get(2, 2)*z);
#endif

    // Update parameters and redraw.
    m_Params.translation.x += cx;
    m_Params.translation.y += cy;
    m_Params.translation.z += cz;

    ForceRefresh();
}

void GfxCore::SetCoords(wxPoint point)
{
    // Update the coordinate or altitude display, given the (x, y) position in
    // window coordinates.  The relevant display is updated depending on
    // whether we're in plan or elevation view.

    wxCoord x = point.x - m_XCentre;
    wxCoord y = -(point.y - m_YCentre);

    if (m_TiltAngle == M_PI_2) {
        Matrix4 inverse_rotation = m_Params.rotation.asInverseMatrix();

        Double cx = Double(inverse_rotation.get(0, 0)*x + inverse_rotation.get(0, 2)*y);
        Double cy = Double(inverse_rotation.get(1, 0)*x + inverse_rotation.get(1, 2)*y);

        m_Parent->SetCoords(cx / m_Params.scale - m_Params.translation.x + m_Parent->GetXOffset(),
			    cy / m_Params.scale - m_Params.translation.y + m_Parent->GetYOffset());
    } else if (m_TiltAngle == 0.0) {
	m_Parent->SetAltitude(y / m_Params.scale - m_Params.translation.z + m_Parent->GetZOffset());
    } else {
	m_Parent->ClearCoords();
    }
}

bool GfxCore::ChangingOrientation()
{
    // Determine whether the cave is currently moving between orientations.

    return (m_SwitchingTo != 0);
}

bool GfxCore::ShowingCompass()
{
    // Determine whether the compass is currently shown.

    return m_Compass;
}

bool GfxCore::ShowingClino()
{
    // Determine whether the clino is currently shown.

    return m_Clino;
}

wxCoord GfxCore::GetCompassXPosition()
{
    // Return the x-coordinate of the centre of the compass.
    
    return m_XSize - INDICATOR_OFFSET_X - INDICATOR_BOX_SIZE/2;
}

wxCoord GfxCore::GetClinoXPosition()
{
    // Return the x-coordinate of the centre of the compass.

    return m_XSize - GetClinoOffset() - INDICATOR_BOX_SIZE/2;
}

wxCoord GfxCore::GetIndicatorYPosition()
{
    // Return the y-coordinate of the centre of the indicators.

    return m_YSize - INDICATOR_OFFSET_Y - INDICATOR_BOX_SIZE/2;
}

wxCoord GfxCore::GetIndicatorRadius()
{
    // Return the radius of each indicator.
    
    return (INDICATOR_BOX_SIZE - INDICATOR_MARGIN*2) / 2;
}

bool GfxCore::PointWithinCompass(wxPoint point)
{
    // Determine whether a point (in window coordinates) lies within the compass.

    wxCoord dx = point.x - GetCompassXPosition();
    wxCoord dy = point.y - GetIndicatorYPosition();
    wxCoord radius = GetIndicatorRadius();
    
    return (dx * dx + dy * dy <= radius * radius);
}

bool GfxCore::PointWithinClino(wxPoint point)
{
    // Determine whether a point (in window coordinates) lies within the clino.

    wxCoord dx = point.x - GetClinoXPosition();
    wxCoord dy = point.y - GetIndicatorYPosition();
    wxCoord radius = GetIndicatorRadius();
    
    return (dx * dx + dy * dy <= radius * radius);
}

bool GfxCore::PointWithinScaleBar(wxPoint point)
{
    // Determine whether a point (in window coordinates) lies within the scale bar.

    return (point.x >= m_ScaleBar.offset_x &&
	    point.x <= m_ScaleBar.offset_x + m_ScaleBar.width &&
	    point.y <= m_YSize - m_ScaleBar.offset_y &&
	    point.y >= m_YSize - m_ScaleBar.offset_y - SCALE_BAR_HEIGHT);
}

void GfxCore::SetCompassFromPoint(wxPoint point)
{
    // Given a point in window coordinates, set the heading of the survey.  If the point
    // is outside the compass, it snaps to 45 degree intervals; otherwise it operates as normal.

    wxCoord dx = point.x - GetCompassXPosition();
    wxCoord dy = point.y - GetIndicatorYPosition();
    wxCoord radius = GetIndicatorRadius();

    if (dx * dx + dy * dy <= radius * radius) {
	TurnCaveTo(atan2(dx, dy) - M_PI);
	m_MouseOutsideCompass = false;
    }
    else {
	TurnCaveTo(int(int((atan2(dx, dy) - M_PI) * 180.0 / M_PI) / 45) * M_PI_4);
        m_MouseOutsideCompass = true;
    }

    ForceRefresh();
}

void GfxCore::SetClinoFromPoint(wxPoint point)
{
    // Given a point in window coordinates, set the elevation of the survey.  If the point
    // is outside the clino, it snaps to 90 degree intervals; otherwise it operates as normal.

    wxCoord dx = point.x - GetClinoXPosition();
    wxCoord dy = point.y - GetIndicatorYPosition();
    wxCoord radius = GetIndicatorRadius();
    
    if (dx >= 0 && dx * dx + dy * dy <= radius * radius) {
	TiltCave(atan2(dy, dx) - m_TiltAngle);
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

void GfxCore::SetScaleBarFromOffset(wxCoord dx)
{
    // Set the scale of the survey, given an offset as to how much the mouse has
    // been dragged over the scalebar since the last scale change.

    Double size_snap = Double(m_ScaleBar.width) / m_Params.scale;

    SetScale((m_ScaleBar.width + dx) / size_snap);
    ForceRefresh();
}

void GfxCore::RedrawIndicators()
{
    // Redraw the compass and clino indicators.

    const wxRect r(m_XSize - INDICATOR_OFFSET_X - INDICATOR_BOX_SIZE*2 - INDICATOR_GAP,
		   m_YSize - INDICATOR_OFFSET_Y - INDICATOR_BOX_SIZE,
		   INDICATOR_BOX_SIZE*2 + INDICATOR_GAP,
		   INDICATOR_BOX_SIZE);
		   
    m_RedrawOffscreen = true;
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
    
    m_Rotating = !m_Rotating;

    if (m_Rotating) {
	timer.Start(drawtime);
    }
}

void GfxCore::StopRotation()
{
    // Stop the survey rotating.

    m_Rotating = false;
}

bool GfxCore::CanRotate()
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
    if (m_RotationStep < M_PI / 180.0) {
	m_RotationStep = (Double) M_PI / 180.0;
    }
}

void GfxCore::RotateFaster(bool accel)
{
    // Increase the speed of rotation, optionally by an increased amount.

    m_RotationStep *= accel ? 1.44 : 1.2;
    if (m_RotationStep > 2.5 * M_PI) {
	m_RotationStep = (Double) 2.5 * M_PI;
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
	    // a second order to switch takes us there right away
	    TiltCave(M_PI_2 - m_TiltAngle);
	    m_SwitchingTo = 0;
	    ForceRefresh();
    }
}

bool GfxCore::CanRaiseViewpoint()
{
    // Determine if the survey can be viewed from a higher angle of elevation.
    
    return (m_TiltAngle < M_PI_2);
}

bool GfxCore::CanLowerViewpoint()
{
    // Determine if the survey can be viewed from a lower angle of elevation.

    return (m_TiltAngle > -M_PI_2);
}

bool GfxCore::ShowingPlan()
{
    // Determine if the survey is in plan view.
    
    return (m_TiltAngle == M_PI_2);
}

bool GfxCore::ShowingElevation()
{
    // Determine if the survey is in elevation view.
    
    return (m_TiltAngle == 0.0);
}

bool GfxCore::ShowingMeasuringLine()
{
    // Determine if the measuring line is being shown.
    
    return (m_there.x != DBL_MAX);
}

void GfxCore::ToggleFlag(bool* flag)
{
    *flag = !*flag;
    ForceRefresh();
}

int GfxCore::GetNumEntrances()
{
    return m_Parent->GetNumEntrances();
}

int GfxCore::GetNumFixedPts()
{
    return m_Parent->GetNumFixedPts();
}

int GfxCore::GetNumExportedPts()
{
    return m_Parent->GetNumExportedPts();
}

void GfxCore::ClearTreeSelection()
{
    m_Parent->ClearTreeSelection();
}

void GfxCore::CentreOn(Double x, Double y, Double z)
{
    m_Params.translation.x = -x;
    m_Params.translation.y = -y;
    m_Params.translation.z = -z;
    ForceRefresh();
}

