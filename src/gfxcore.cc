//
//  gfxcore.cxx
//
//  Core drawing code for Aven.
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

#include "gfxcore.h"
#include "mainfrm.h"
#include "message.h"

#include <math.h>

// This is for mingw32/Visual C++:
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MAX(a, b)    (((a) > (b)) ? (a) : (b))
#define TEXT_COLOUR  wxColour(0, 255, 40)
#define LABEL_COLOUR wxColour(160, 255, 0)
//#define LABEL_COLOUR wxColour(175, 4, 214)

#ifdef _WIN32
static const int FONT_SIZE = 8;
#else
static const int FONT_SIZE = 10;
#endif
static const int CROSS_SIZE = 5;
static const double COMPASS_SIZE = 24.0f;
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
static const int TIMER_ID = 0;
static const int SCALE_BAR_OFFSET_X = 15;
static const int SCALE_BAR_OFFSET_Y = 12;
static const int SCALE_BAR_HEIGHT = 12;
static const int HIGHLIGHTED_PT_SIZE = 2;

#define DELETE_ARRAY(x) { assert(x); delete[] x; }

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
    EVT_IDLE(GfxCore::OnTimer)
END_EVENT_TABLE()

GfxCore::GfxCore(MainFrm* parent) :
    wxWindow(parent, 100, wxDefaultPosition, wxSize(640, 480)),
    m_Font(FONT_SIZE, wxSWISS, wxNORMAL, wxNORMAL, FALSE, "Helvetica",
	   wxFONTENCODING_ISO8859_1),
    m_InitialisePending(false)
{
    m_LastDrag = drag_NONE;
    m_ScaleBar.offset_x = SCALE_BAR_OFFSET_X;
    m_ScaleBar.offset_y = SCALE_BAR_OFFSET_Y;
    m_ScaleBar.width = 0;
    m_DraggingLeft = false;
    m_DraggingMiddle = false;
    m_DraggingRight = false;
    m_Parent = parent;
    m_DepthbarOff = false;
    m_ScalebarOff = false;
    m_IndicatorsOff = false;
    m_DoneFirstShow = false;
    m_PlotData = NULL;
    m_RedrawOffscreen = false;
    m_Params.display_shift.x = 0;
    m_Params.display_shift.y = 0;
    m_LabelsLastPlotted = NULL;
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
    m_SwitchingToPlan = false;
    m_SwitchingToElevation = false;
    m_Entrances = false;
    m_FixedPts = false;
    m_ExportedPts = false;

    // Create pens and brushes for drawing.
    m_Pens.black.SetColour(0, 0, 0);
    m_Pens.grey.SetColour(100, 100, 100);
    m_Pens.lgrey.SetColour(180, 180, 180);
    m_Pens.lgrey2.SetColour(140, 140, 140);
    m_Pens.white.SetColour(255, 255, 255);
    m_Pens.turquoise.SetColour(0, 100, 255);
    m_Pens.green.SetColour(0, 255, 40);
    m_Pens.indicator1.SetColour(150, 205, 224);
    m_Pens.indicator2.SetColour(114, 149, 160);
    m_Pens.yellow.SetColour(255, 255, 0);
    m_Pens.red.SetColour(255, 0, 0);
    m_Pens.cyan.SetColour(0, 100, 255);

    m_Brushes.black.SetColour(0, 0, 0);
    m_Brushes.white.SetColour(255, 255, 255);
    m_Brushes.grey.SetColour(100, 100, 100);
    m_Brushes.dgrey.SetColour(90, 90, 90);
    m_Brushes.indicator1.SetColour(150, 205, 224);
    m_Brushes.indicator2.SetColour(114, 149, 160);
    m_Brushes.yellow.SetColour(255, 255, 0);
    m_Brushes.red.SetColour(255, 0, 0);
    m_Brushes.cyan.SetColour(0, 100, 255);

    SetBackgroundColour(wxColour(0, 0, 0));
}

GfxCore::~GfxCore()
{
    TryToFreeArrays();
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
	DELETE_ARRAY(m_HighlightedPts);
	DELETE_ARRAY(m_Polylines);
	DELETE_ARRAY(m_SurfacePolylines);
	DELETE_ARRAY(m_CrossData.vertices);
	DELETE_ARRAY(m_CrossData.num_segs);
	DELETE_ARRAY(m_Labels);
	DELETE_ARRAY(m_LabelsLastPlotted);

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

    GetSize(&m_XSize, &m_YSize);

    m_Bands = m_Parent->GetNumDepthBands(); // last band is surface data
    m_PlotData = new PlotData[m_Bands];
    m_Polylines = new int[m_Bands];
    m_SurfacePolylines = new int[m_Bands];
    m_CrossData.vertices = new wxPoint[m_Parent->GetNumCrosses() * 4];
    m_CrossData.num_segs = new int[m_Parent->GetNumCrosses() * 2];
    m_HighlightedPts = new HighlightedPt[m_Parent->GetNumCrosses()];
    m_Labels = new wxString[m_Parent->GetNumCrosses()];
    m_LabelsLastPlotted = new LabelFlags[m_Parent->GetNumCrosses()];
    m_LabelCacheNotInvalidated = false;
    
    for (int band = 0; band < m_Bands; band++) {
        m_PlotData[band].vertices = new wxPoint[m_Parent->GetNumPoints()];
	m_PlotData[band].num_segs = new int[m_Parent->GetNumLegs()];
        m_PlotData[band].surface_vertices = new wxPoint[m_Parent->GetNumPoints()];
	m_PlotData[band].surface_num_segs = new int[m_Parent->GetNumLegs()];
    }

    m_UndergroundLegs = false;
    m_SurfaceLegs = false;

    // Apply default parameters.
    DefaultParameters();

    // Check for flat/linear/point surveys.
    m_Lock = lock_NONE;
    m_IndicatorsOff = false;
    m_DepthbarOff = false;
    m_ScalebarOff = false;
    
    if (m_Parent->GetXExtent() == 0.0) {
        m_Lock = LockFlags(m_Lock | lock_X);
    }
    if (m_Parent->GetYExtent() == 0.0) {
        m_Lock = LockFlags(m_Lock | lock_Y);
    }
    if (m_Parent->GetZExtent() == 0.0) {
        m_Lock = LockFlags(m_Lock | lock_Z);
    }
    switch (m_Lock) {
        case lock_X:
	{
	    // elevation looking along X axis
	    m_PanAngle = M_PI * 1.5;

	    Quaternion q;
	    q.setFromEulerAngles(0.0, 0.0, m_PanAngle);

	    m_Params.rotation = q * m_Params.rotation;
	    m_RotationMatrix = m_Params.rotation.asMatrix();
	    m_IndicatorsOff = true;
	    break;
	}

        case lock_Y:
  	    // elevation looking along Y axis
 	    m_Params.rotation.setFromEulerAngles(0.0, 0.0, 0.0);
	    m_RotationMatrix = m_Params.rotation.asMatrix();
	    m_TiltAngle = 0.0;
 	    m_IndicatorsOff = true;
	    break;

        case lock_Z:
        case lock_XZ: // linear survey parallel to Y axis
        case lock_YZ: // linear survey parallel to X axis
	{
	    // flat survey (zero height range) => go into plan view (default orientation).
	    m_Clino = false;
	    break;
	}

        case lock_POINT:
  	    m_DepthbarOff = true;
	    m_ScalebarOff = true;
	    m_IndicatorsOff = true;
	    m_Crosses = true;
	    break;

        case lock_XY:
	{
	    // survey is linear and parallel to the Z axis => display in elevation.
	    m_PanAngle = M_PI * 1.5;

	    Quaternion q;
	    q.setFromEulerAngles(0.0, 0.0, m_PanAngle);

	    m_Params.rotation = q * m_Params.rotation;
	    m_RotationMatrix = m_Params.rotation.asMatrix();
	    m_IndicatorsOff = true;
	    break;
	}

        case lock_NONE:
            break;
    }

    // Scale the survey to a reasonable initial size.
    m_InitialScale = m_Lock == lock_POINT ? 1.0 :
      m_Lock == lock_XY ? double(m_YSize) / double(m_Parent->GetZExtent()) :
      double(m_XSize) / (double(MAX(m_Parent->GetXExtent(), m_Parent->GetYExtent())) * 1.1);

    // Calculate screen coordinates and redraw.
    m_ScaleCrossesOnly = false;
    m_ScaleHighlightedPtsOnly = false;
    SetScale(m_InitialScale);
    m_RedrawOffscreen = true;
    Refresh(false);
}

void GfxCore::FirstShow()
{
    // Update our record of the client area size and centre.
    GetClientSize(&m_XSize, &m_YSize);
    m_XCentre = m_XSize / 2;
    m_YCentre = m_YSize / 2;

    // Create the offscreen bitmap.
    m_OffscreenBitmap.Create(m_XSize, m_YSize);

    m_DrawDC.SelectObject(m_OffscreenBitmap);

    // Set the font.
    m_DrawDC.SetFont(m_Font);

    m_DoneFirstShow = true;

    RedrawOffscreen();
}

//
//  Recalculating methods
//

void GfxCore::SetScale(double scale)
{
    // Fill the plot data arrays with screen coordinates, scaling the survey
    // to a particular absolute scale.

    m_Params.scale = scale;

    if (m_Params.scale < m_InitialScale / 20.0) {
        m_Params.scale = m_InitialScale / 20.0;
    }

    if (!m_ScaleCrossesOnly && !m_ScaleHighlightedPtsOnly) {
        for (int band = 0; band < m_Bands; band++) {
	    wxPoint* pt = m_PlotData[band].vertices;
	    assert(pt);
	    int* count = m_PlotData[band].num_segs;
	    assert(count);
	    wxPoint* spt = m_PlotData[band].surface_vertices;
	    assert(spt);
	    int* scount = m_PlotData[band].surface_num_segs;
	    assert(scount);
	    count--;
	    scount--;
	    double current_x = 0.0;
	    double current_y = 0.0;
	    double current_z = 0.0;

	    m_Polylines[band] = 0;
	    m_SurfacePolylines[band] = 0;
	    list<PointInfo*>::const_iterator pos = m_Parent->GetPoints(band);
	    list<PointInfo*>::const_iterator end = m_Parent->GetPointsEnd(band);
	    bool first_point = true;
	    bool last_was_move = true;
	    bool current_polyline_is_surface = false;
	    while (pos != end) {
 	        const PointInfo* pti = *pos++;

		double x = pti->GetX();
		double y = pti->GetY();
		double z = pti->GetZ();
		
		if (pti->IsLine()) {
		    // We have a leg.

		    assert(!first_point); // The first point must always be a move.

		    // Determine if we're switching from an underground polyline to a
		    // surface polyline, or vice-versa.
		    bool changing_ug_state = (current_polyline_is_surface != pti->IsSurface());

		    // Record new underground/surface state.
		    current_polyline_is_surface = pti->IsSurface();

		    if (changing_ug_state || last_was_move) {
		        // Start a new polyline if we're switching underground/surface state
		        // or if the previous point was a move.

		        wxPoint** dest;

		        if (current_polyline_is_surface) {
			    m_SurfacePolylines[band]++;
			    *(++scount) = 1; // initialise number of vertices for next polyline
			    dest = &spt;
			}
			else {
			    m_Polylines[band]++;
			    *(++count) = 1; // initialise number of vertices for next polyline
			    dest = &pt;
			}

			double xp = current_x + m_Params.translation.x;
			double yp = current_y + m_Params.translation.y;
			double zp = current_z + m_Params.translation.z;

			(*dest)->x = (long) (XToScreen(xp, yp, zp) * scale) +
			             m_Params.display_shift.x;
			(*dest)->y = -(long) (ZToScreen(xp, yp, zp) * scale) +
			             m_Params.display_shift.y;

			// Advance the relevant coordinate pointer to the next position.
			(*dest)++;
		    }

		    // Add the leg onto the current polyline.
		    wxPoint** dest = &(current_polyline_is_surface ? spt : pt);

		    // Final coordinate transformations and storage of coordinates.
		    double xp = x + m_Params.translation.x;
		    double yp = y + m_Params.translation.y;
		    double zp = z + m_Params.translation.z;

		    (*dest)->x = (long) (XToScreen(xp, yp, zp) * scale) + m_Params.display_shift.x;
		    (*dest)->y = -(long) (ZToScreen(xp, yp, zp) * scale) + m_Params.display_shift.y;

		    // Advance the relevant coordinate pointer to the next position.
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
		}

		// Save the current coordinates for the next time around the loop.
		current_x = x;
		current_y = y;
		current_z = z;
	    }

	    if (!m_UndergroundLegs) {
	        m_UndergroundLegs = (m_Polylines[band] > 0);
	    }
	    if (!m_SurfaceLegs) {
	        m_SurfaceLegs = (m_SurfacePolylines[band] > 0);
	    }
	}
    }

    if (m_Crosses || m_Names || m_Entrances || m_FixedPts || m_ExportedPts) {
        // Construct polylines for crosses, sort out station names and deal with highlighted points.

        m_NumHighlightedPts = 0;
	HighlightedPt* hpt = m_HighlightedPts;
        wxPoint* pt = m_CrossData.vertices;
	int* count = m_CrossData.num_segs;
	wxString* labels = m_Labels;
	list<LabelInfo*>::const_iterator pos = m_Parent->GetLabels();
	list<LabelInfo*>::const_iterator end = m_Parent->GetLabelsEnd();
	wxString text;
	while (pos != end) {
	    LabelInfo* label = *pos++;
	    double x = label->GetX();
	    double y = label->GetY();
	    double z = label->GetZ();

	    x += m_Params.translation.x;
	    y += m_Params.translation.y;
	    z += m_Params.translation.z;

	    int cx = (int) (XToScreen(x, y, z) * scale) + m_Params.display_shift.x;
	    int cy = -(int) (ZToScreen(x, y, z) * scale) + m_Params.display_shift.y;

	    if (m_Crosses || m_Names) {
	        pt->x = cx - CROSS_SIZE;
		pt->y = cy - CROSS_SIZE;
		
		pt++;
		pt->x = cx + CROSS_SIZE;
		pt->y = cy + CROSS_SIZE;
		pt++;
		pt->x = cx - CROSS_SIZE;
		pt->y = cy + CROSS_SIZE;
		pt++;
		pt->x = cx + CROSS_SIZE;
		pt->y = cy - CROSS_SIZE;
		pt++;
		
		*count++ = 2;
		*count++ = 2;
	    
		*labels++ = label->GetText();
	    }

	    if (m_FixedPts || m_Entrances || m_ExportedPts) {
		hpt->x = cx;
		hpt->y = cy;
		hpt->flags = hl_NONE;

		if (label->IsFixedPt()) {
		    hpt->flags = HighlightFlags(hpt->flags | hl_FIXED);
		}

		if (label->IsEntrance()) {
		    hpt->flags = HighlightFlags(hpt->flags | hl_ENTRANCE);
		}

		if (label->IsExportedPt()) {
		    hpt->flags = HighlightFlags(hpt->flags | hl_EXPORTED);
		}

		if (hpt->flags != hl_NONE) {
		    hpt++;
		    m_NumHighlightedPts++;
		}
	    }
	}
    }

    m_ScaleHighlightedPtsOnly = false;
    m_ScaleCrossesOnly = false;
}

//
//  Repainting methods
//

void GfxCore::RedrawOffscreen()
{
    // Redraw the offscreen bitmap.

    m_DrawDC.BeginDrawing();

    // Clear the background to black.
    m_DrawDC.SetPen(m_Pens.black);
    m_DrawDC.SetBrush(m_Brushes.black);
    m_DrawDC.DrawRectangle(0, 0, m_XSize, m_YSize);

    if (m_PlotData) {
        // Draw underground legs.
        if (m_Legs) {
	    for (int band = 0; band < m_Bands; band++) {
	        m_DrawDC.SetPen(m_Parent->GetPen(band));
		int* num_segs = m_PlotData[band].num_segs; //-- sort out the polyline stuff!!
		wxPoint* vertices = m_PlotData[band].vertices;
		for (int polyline = 0; polyline < m_Polylines[band]; polyline++) {
       		    m_DrawDC.DrawLines(*num_segs, vertices, m_XCentre, m_YCentre);
		    vertices += *num_segs++;
		}
	    }
	}

	// Draw surface legs.
        if (m_Surface) {
	    for (int band = 0; band < m_Bands; band++) {
	        wxPen pen = m_SurfaceDepth ? m_Parent->GetPen(band) : m_Parent->GetSurfacePen();
		if (m_SurfaceDashed) {
#ifdef _WIN32
		    pen.SetStyle(wxDOT);
#else
		    pen.SetStyle(wxSHORT_DASH);
#endif
		}
		m_DrawDC.SetPen(pen);

		int* num_segs = m_PlotData[band].surface_num_segs; //-- sort out the polyline stuff!!
		wxPoint* vertices = m_PlotData[band].surface_vertices;
		for (int polyline = 0; polyline < m_SurfacePolylines[band]; polyline++) {
       		    m_DrawDC.DrawLines(*num_segs, vertices, m_XCentre, m_YCentre);
		    vertices += *num_segs++;
		}
		if (m_SurfaceDashed) {
		    pen.SetStyle(wxSOLID);
		}
	    }
	}

	// Draw crosses.
	if (m_Crosses) {
	    m_DrawDC.SetPen(m_Pens.turquoise);
	    int* num_segs = m_CrossData.num_segs; //-- sort out the polyline stuff!!
	    wxPoint* vertices = m_CrossData.vertices;
	    for (int polyline = 0; polyline < m_Parent->GetNumCrosses() * 2; polyline++) {
	        m_DrawDC.DrawLines(*num_segs, vertices, m_XCentre, m_YCentre);
		vertices += *num_segs++;
	    }
	}

	// Plot highlighted points.
	if (m_Entrances || m_FixedPts || m_ExportedPts) {
	    for (int count = 0; count < m_NumHighlightedPts; count++) {
	        HighlightedPt* pt = &m_HighlightedPts[count];

		bool draw = true;

		// When more than one flag is set on a point:
		// entrance highlighting takes priority over fixed point highlighting, which in turn
		// takes priority over exported point highlighting.

		if (m_Entrances && (pt->flags & hl_ENTRANCE)) {
		    m_DrawDC.SetPen(m_Pens.yellow);
		    m_DrawDC.SetBrush(m_Brushes.yellow);
		}
		else if (m_FixedPts && (pt->flags & hl_FIXED)) {
		    m_DrawDC.SetPen(m_Pens.red);
		    m_DrawDC.SetBrush(m_Brushes.red);
		}
		else if (m_ExportedPts && (pt->flags & hl_EXPORTED)) {
		    m_DrawDC.SetPen(m_Pens.cyan);
		    m_DrawDC.SetBrush(m_Brushes.cyan);
		}
		else {
		    draw = false;
		}

		if (draw) {
		    m_DrawDC.DrawEllipse(pt->x - HIGHLIGHTED_PT_SIZE + m_XCentre,
					 pt->y - HIGHLIGHTED_PT_SIZE + m_YCentre,
					 HIGHLIGHTED_PT_SIZE*2, HIGHLIGHTED_PT_SIZE*2);
		}
	    }
	}

	// Draw station names.
	if (m_Names) {
	    DrawNames();
	    m_LabelCacheNotInvalidated = false;
	}

	// Draw scalebar.
	if (m_Scalebar && !m_ScalebarOff) {
	    DrawScalebar();
	}

	// Draw depthbar.
	if (m_Depthbar && !m_DepthbarOff) {
	    DrawDepthbar();
	}

	// Draw compass or elevation/heading indicators.
	if ((m_Compass || m_Clino) && !m_IndicatorsOff) {
	    if (m_FreeRotMode) {
	        DrawCompass();
	    }
	    else {
	        Draw2dIndicators();
	    }
	}
    }

    m_DrawDC.EndDrawing();
}

void GfxCore::OnPaint(wxPaintEvent& event) 
{
    // Redraw the window.

    // Get a graphics context.
    wxPaintDC dc(this);

    const wxRegion& region = GetUpdateRegion();

    // Make sure we're initialised.
    if (!m_DoneFirstShow) {
        FirstShow();
    }

    // Redraw the offscreen bitmap if it's out of date.
    if (m_RedrawOffscreen) {
        m_RedrawOffscreen = false;
	RedrawOffscreen();
    }

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

    dc.EndDrawing();
}

wxCoord GfxCore::GetClinoOffset()
{
    return m_Compass ? CLINO_OFFSET_X : INDICATOR_OFFSET_X;
}

wxPoint GfxCore::CompassPtToScreen(double x, double y, double z)
{
    return wxPoint(long(-XToScreen(x, y, z)) + m_XSize - COMPASS_OFFSET_X,
	  	   long(ZToScreen(x, y, z)) + m_YSize - COMPASS_OFFSET_Y);
}

wxPoint GfxCore::IndicatorCompassToScreenPan(int angle)
{
    double theta = (angle * M_PI / 180.0) + m_PanAngle;
    wxCoord length = (INDICATOR_BOX_SIZE - INDICATOR_MARGIN*2) / 2;
    wxCoord x = wxCoord(length * sin(theta));
    wxCoord y = wxCoord(length * cos(theta));

    return wxPoint(m_XSize - INDICATOR_OFFSET_X - INDICATOR_BOX_SIZE/2 - x,
		   m_YSize - INDICATOR_OFFSET_Y - INDICATOR_BOX_SIZE/2 - y);
}

wxPoint GfxCore::IndicatorCompassToScreenElev(int angle)
{
    double theta = (angle * M_PI / 180.0) + m_TiltAngle + M_PI/2.0;
    wxCoord length = (INDICATOR_BOX_SIZE - INDICATOR_MARGIN*2) / 2;
    wxCoord x = wxCoord(length * sin(-theta));
    wxCoord y = wxCoord(length * cos(-theta));

    return wxPoint(m_XSize - GetClinoOffset() - INDICATOR_BOX_SIZE/2 - x,
		   m_YSize - INDICATOR_OFFSET_Y - INDICATOR_BOX_SIZE/2 - y);
}

void GfxCore::DrawTick(wxCoord cx, wxCoord cy, int angle_cw)
{
    double theta = angle_cw * M_PI / 180.0;
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
    m_DrawDC.SetBrush(m_Brushes.grey);
    m_DrawDC.SetPen(m_Pens.lgrey2);
    if (m_Compass) {
        m_DrawDC.DrawEllipse(m_XSize - INDICATOR_OFFSET_X - INDICATOR_BOX_SIZE + INDICATOR_MARGIN,
			     m_YSize - INDICATOR_OFFSET_Y - INDICATOR_BOX_SIZE + INDICATOR_MARGIN,
			     INDICATOR_BOX_SIZE - INDICATOR_MARGIN*2,
			     INDICATOR_BOX_SIZE - INDICATOR_MARGIN*2);
    }
    if (m_Clino) {
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
    if (m_Compass) {
        int deg_pan = (int) (m_PanAngle * 180.0 / M_PI);
	// FIXME: bodge by Olly to stop wrong tick highlighting
	if (deg_pan) deg_pan = 360 - deg_pan;
	for (int angle = deg_pan; angle <= 315 + deg_pan; angle += 45) {
	    if (deg_pan == angle) {
	        m_DrawDC.SetPen(m_Pens.green);
	    }
	    else {
	        m_DrawDC.SetPen(white ? m_Pens.white : m_Pens.lgrey2);
	    }
	    DrawTick(pan_centre_x, centre_y, angle);
	}
    }
    if (m_Clino) {
        white = m_DraggingLeft && m_LastDrag == drag_ELEV && m_MouseOutsideElev;
	int deg_elev = (int) (m_TiltAngle * 180.0 / M_PI);
	for (int angle = 0; angle <= 180; angle += 90) {
	    if (deg_elev == angle - 90) {
	        m_DrawDC.SetPen(m_Pens.green);
	    }
	    else {
	        m_DrawDC.SetPen(white ? m_Pens.white : m_Pens.lgrey2);
	    }
	    DrawTick(elev_centre_x, centre_y, angle);
	}
    }

    // Pan arrow
    if (m_Compass) {
        wxPoint p1 = IndicatorCompassToScreenPan(0);
	wxPoint p2 = IndicatorCompassToScreenPan(150);
	wxPoint p3 = IndicatorCompassToScreenPan(210);
	wxPoint pc(pan_centre_x, centre_y);
	wxPoint pts1[3] = { p2, p1, pc };
	wxPoint pts2[3] = { p3, p1, pc };
	m_DrawDC.SetPen(m_Pens.lgrey);
	m_DrawDC.SetBrush(m_Brushes.indicator1);
	m_DrawDC.DrawPolygon(3, pts1);
	m_DrawDC.SetBrush(m_Brushes.indicator2);
	m_DrawDC.DrawPolygon(3, pts2);
    }

    // Elevation arrow
    if (m_Clino) {
        wxPoint p1e = IndicatorCompassToScreenElev(0);
	wxPoint p2e = IndicatorCompassToScreenElev(150);
	wxPoint p3e = IndicatorCompassToScreenElev(210);
	wxPoint pce(elev_centre_x, centre_y);
	wxPoint pts1e[3] = { p2e, p1e, pce };
	wxPoint pts2e[3] = { p3e, p1e, pce };
	m_DrawDC.SetPen(m_Pens.lgrey);
	m_DrawDC.SetBrush(m_Brushes.indicator2);
	m_DrawDC.DrawPolygon(3, pts1e);
	m_DrawDC.SetBrush(m_Brushes.indicator1);
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
   
    if (m_Compass) {
	str = wxString::Format("%03d", int(m_PanAngle * 180.0 / M_PI));
	m_DrawDC.GetTextExtent(str, &w, &h);
	m_DrawDC.DrawText(str, pan_centre_x + width / 2 - w, height);
        str = wxString(msg(/*Facing*/203));
	m_DrawDC.GetTextExtent(str, &w, &h);
	m_DrawDC.DrawText(str, pan_centre_x - w / 2, height - h);
    }

    if (m_Clino) {
        int angle = int(-m_TiltAngle * 180.0 / M_PI);
	str = angle ? wxString::Format("%+03d", angle) : wxString("00");
	m_DrawDC.GetTextExtent(str, &w, &h);
	m_DrawDC.DrawText(str, elev_centre_x + width / 2 - w, height);
        str = wxString(msg(/*Elevation*/118));
	m_DrawDC.GetTextExtent(str, &w, &h);
	m_DrawDC.DrawText(str, elev_centre_x - w / 2, height - h);
    }
}

void GfxCore::DrawCompass()
{
    // Draw the 3d compass.

    wxPoint pt[3];

    m_DrawDC.SetPen(m_Pens.turquoise);
    m_DrawDC.DrawLine(CompassPtToScreen(0.0, 0.0, -COMPASS_SIZE),
		      CompassPtToScreen(0.0, 0.0, COMPASS_SIZE));

    pt[0] = CompassPtToScreen(-COMPASS_SIZE / 3.0f, 0.0, -COMPASS_SIZE * 2.0f / 3.0f);
    pt[1] = CompassPtToScreen(0.0, 0.0, -COMPASS_SIZE);
    pt[2] = CompassPtToScreen(COMPASS_SIZE / 3.0f, 0.0, -COMPASS_SIZE * 2.0f / 3.0f);
    m_DrawDC.DrawLines(3, pt);

    m_DrawDC.DrawLine(CompassPtToScreen(-COMPASS_SIZE, 0.0, 0.0),
		      CompassPtToScreen(COMPASS_SIZE, 0.0, 0.0));

    m_DrawDC.SetPen(m_Pens.green);
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

    if (m_OverlappingNames || m_LabelCacheNotInvalidated) {
        SimpleDrawNames();
	// Draw names on bits of the survey which weren't visible when
	// the label cache was last generated (happens after a translation...)
	if (m_LabelCacheNotInvalidated) {
	    NattyDrawNames();
	}
    }
    else {
        NattyDrawNames();
    }
}

void GfxCore::NattyDrawNames()
{
    // Draw station names, without overlapping.

    const int dv = 2;
    const int quantise = int(FONT_SIZE / dv);
    const int quantised_x = m_XSize / quantise;
    const int quantised_y = m_YSize / quantise;
    const size_t buffer_size = quantised_x * quantised_y;
    if (!m_LabelCacheNotInvalidated) {
        if (m_LabelGrid) {
	    delete[] m_LabelGrid;
	}
	m_LabelGrid = new LabelFlags[buffer_size];
	memset((void*) m_LabelGrid, 0, buffer_size * sizeof(LabelFlags));
    }

    wxString* label = m_Labels;
    LabelFlags* last_plot = m_LabelsLastPlotted;
    wxPoint* pt = m_CrossData.vertices;
	
    for (int name = 0; name < m_Parent->GetNumCrosses(); name++) {
        // *pt is at (cx, cy - CROSS_SIZE), where (cx, cy) are the coordinates of
        // the actual station.
 
        wxCoord x = pt->x + m_XSize/2;
	wxCoord y = pt->y + CROSS_SIZE - FONT_SIZE + m_YSize/2;

	// We may have labels in the cache which are still going to be in the
	// same place: in this case we only consider labels here which are in
	// just about the region of screen exposed since the last label cache generation,
	// and were not plotted last time, together with labels which were in
	// this category last time around but didn't end up plotted (possibly because
	// the newly exposed region was too small, as happens during a drag).
#ifdef _DEBUG
	if (m_LabelCacheNotInvalidated) {
	    assert(m_LabelCacheExtend.bottom >= m_LabelCacheExtend.top);
	    assert(m_LabelCacheExtend.right >= m_LabelCacheExtend.left);
	}
#endif
	
	if ((m_LabelCacheNotInvalidated && x >= m_LabelCacheExtend.GetLeft() &&
	     x <= m_LabelCacheExtend.GetRight() && y >= m_LabelCacheExtend.GetTop() &&
	     y <= m_LabelCacheExtend.GetBottom() && *last_plot == label_NOT_PLOTTED) ||
	    (m_LabelCacheNotInvalidated && *last_plot == label_CHECK_AGAIN) ||
	    !m_LabelCacheNotInvalidated) {
		
	    wxString str = *label;

#ifdef _DEBUG
	    if (m_LabelCacheNotInvalidated && *last_plot == label_CHECK_AGAIN) {
	        TRACE("Label " + str + " being checked again.\n");
	    }
#endif

	    int ix = int(x) / quantise;
	    int iy = int(y) / quantise;
	    int ixshift = m_LabelCacheNotInvalidated ? int(m_LabelShift.x / quantise) : 0;
	    //		if (ix + ixshift >= quantised_x) {
	//			ixshift = 
	    int iyshift = m_LabelCacheNotInvalidated ? int(m_LabelShift.y / quantise) : 0;

	    if (ix >= 0 && ix < quantised_x && iy >= 0 && iy < quantised_y) {
	          LabelFlags* test = &m_LabelGrid[ix + ixshift + (iy + iyshift)*quantised_x];
		  int len = str.Length()*dv + 1;
		  bool reject = (ix + len >= quantised_x);
		  int i = 0;
		  while (!reject && i++ < len) {
		      reject = *test++ != label_NOT_PLOTTED;
		  }

		  if (!reject) {
		      m_DrawDC.DrawText(str, x, y);
		      int ymin = (iy >= 2) ? iy - 2 : iy;
		      int ymax = (iy < quantised_y - 2) ? iy + 2 : iy;
		      for (int y0 = ymin; y0 <= ymax; y0++) {
			memset((void*) &m_LabelGrid[ix + y0*quantised_x], true,
			       sizeof(LabelFlags) * len);
		      }
		  }
		  
		  if (reject) {
		      *last_plot++ = m_LabelCacheNotInvalidated ? label_CHECK_AGAIN :
			                                          label_NOT_PLOTTED;
		  }
		  else {
		      *last_plot++ = label_PLOTTED;
		  }
	    }
	    else {
	        *last_plot++ = m_LabelCacheNotInvalidated ? label_CHECK_AGAIN :
		                                            label_NOT_PLOTTED;
	    }
	}
	else {
	    if (m_LabelCacheNotInvalidated && x >= m_LabelCacheExtend.GetLeft() - 50 &&
		x <= m_LabelCacheExtend.GetRight() + 50 &&
		y >= m_LabelCacheExtend.GetTop() - 50 &&
		y <= m_LabelCacheExtend.GetBottom() + 50) {
	        *last_plot++ = label_CHECK_AGAIN;
	    }
	    else {
	        last_plot++; // leave the cache alone
	    }
	}

	label++;
	pt += 4;
    }
}

void GfxCore::SimpleDrawNames()
{
    // Draw station names, possibly overlapping; or use previously-cached info
    // from NattyDrawNames() to draw names known not to overlap.

    wxString* label = m_Labels;
    wxPoint* pt = m_CrossData.vertices;

    LabelFlags* last_plot = m_LabelsLastPlotted;
    for (int name = 0; name < m_Parent->GetNumCrosses(); name++) {
        // *pt is at (cx, cy - CROSS_SIZE), where (cx, cy) are the coordinates of
        // the actual station.

        if ((m_LabelCacheNotInvalidated && *last_plot == label_PLOTTED) ||
	    !m_LabelCacheNotInvalidated) {
	    m_DrawDC.DrawText(*label, pt->x + m_XCentre,
			      pt->y + m_YCentre + CROSS_SIZE - FONT_SIZE);
	}
	
	last_plot++;
	label++;
	pt += 4;
    }
}

void GfxCore::DrawDepthbar()
{
    // Draw the depthbar.

    m_DrawDC.SetTextBackground(wxColour(0, 0, 0));
    m_DrawDC.SetTextForeground(TEXT_COLOUR);

    int bands = (m_Lock == lock_NONE || m_Lock == lock_X || m_Lock == lock_Y ||
		 m_Lock == lock_XY) ? m_Bands-1 : 1;
    int y = DEPTH_BAR_BLOCK_HEIGHT*bands + DEPTH_BAR_OFFSET_Y;
    int size = 0;

    wxString* strs = new wxString[bands + 1];
    for (int band = 0; band <= bands; band++) {
	double z = m_Parent->GetZMin() + (m_Parent->GetZExtent() * band / bands);
	strs[band] = FormatLength(z, false);
	int x, y;
	m_DrawDC.GetTextExtent(strs[band], &x, &y);
	if (x > size) {
	    size = x;
	}
    }

    int x_min = m_XSize - DEPTH_BAR_OFFSET_X - DEPTH_BAR_BLOCK_WIDTH - DEPTH_BAR_MARGIN - size;

    m_DrawDC.SetPen(m_Pens.black);
    m_DrawDC.SetBrush(m_Brushes.dgrey);
    m_DrawDC.DrawRectangle(x_min - DEPTH_BAR_MARGIN - DEPTH_BAR_EXTRA_LEFT_MARGIN,
			   DEPTH_BAR_OFFSET_Y - DEPTH_BAR_MARGIN*2,
			   DEPTH_BAR_BLOCK_WIDTH + size + DEPTH_BAR_MARGIN*3 +
			     DEPTH_BAR_EXTRA_LEFT_MARGIN,
			   DEPTH_BAR_BLOCK_HEIGHT*bands + DEPTH_BAR_MARGIN*4);

    for (int band = (bands == 1 ? 1 : 0); band <= bands; band++) {
        if (band < bands || bands == 1) {
	    m_DrawDC.SetPen(m_Parent->GetPen(band));
	    m_DrawDC.SetBrush(m_Parent->GetBrush(band));
	    m_DrawDC.DrawRectangle(x_min, y - DEPTH_BAR_BLOCK_HEIGHT,
				   DEPTH_BAR_BLOCK_WIDTH, DEPTH_BAR_BLOCK_HEIGHT);
	}

	m_DrawDC.DrawText(strs[band], x_min + DEPTH_BAR_BLOCK_WIDTH + 5,
			  y - (FONT_SIZE / 2) - 1 - (bands == 1 ? DEPTH_BAR_BLOCK_HEIGHT/2 : 0));

	y -= DEPTH_BAR_BLOCK_HEIGHT;
    }

    delete[] strs;
}

wxString GfxCore::FormatLength(double size_snap, bool scalebar)
{
    wxString str;
    bool negative = (size_snap < 0.0);

    if (negative) {
        size_snap = -size_snap;
    }

    if (size_snap == 0.0) {
        str = "0";
    }
    else {
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
    }

    return negative ? wxString("-") + str : str;
}

void GfxCore::DrawScalebar()
{
    // Draw the scalebar.

    // Calculate the extent of the survey, in metres across the screen plane.
    double m_across_screen = double(m_XSize / m_Params.scale);

    // Calculate the length of the scale bar in metres.
    double size_snap = pow(10.0, floor(log10(0.75 * m_across_screen)));
    double t = m_across_screen * 0.75 / size_snap;
    if (t >= 5.0) {
        size_snap *= 5.0;
    }
    else if (t >= 2.0) {
        size_snap *= 2.0;
    }

    // Actual size of the thing in pixels:
    int size = int(size_snap * m_Params.scale);
    m_ScaleBar.width = size;
    
    // Draw it...
    int end_x = m_ScaleBar.offset_x;
    int height = SCALE_BAR_HEIGHT;
    int end_y = m_YSize - m_ScaleBar.offset_y - height;
    int interval = size / 10;

    bool solid = true;
    for (int ix = 0; ix < 10; ix++) {
        int x = end_x + int(ix * ((double) size / 10.0));
        
	m_DrawDC.SetPen(solid ? m_Pens.grey : m_Pens.white);
	m_DrawDC.SetBrush(solid ? m_Brushes.grey : m_Brushes.white);
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
    if (m_PlotData && (m_Lock != lock_POINT)) {
        m_DraggingLeft = true;
	m_ScaleBar.drag_start_offset_x = m_ScaleBar.offset_x;
	m_ScaleBar.drag_start_offset_y = m_ScaleBar.offset_y;
	m_DragStart = wxPoint(event.GetX(), event.GetY());
    }
}

void GfxCore::OnLButtonUp(wxMouseEvent& event)
{
    if (m_PlotData && (m_Lock != lock_POINT)) {
        m_LastDrag = drag_NONE;
	m_DraggingLeft = false;
	const wxRect r(m_XSize - INDICATOR_OFFSET_X - INDICATOR_BOX_SIZE*2 - INDICATOR_GAP,
		       m_YSize - INDICATOR_OFFSET_Y - INDICATOR_BOX_SIZE,
		       INDICATOR_BOX_SIZE*2 + INDICATOR_GAP,
		       INDICATOR_BOX_SIZE);
	m_RedrawOffscreen = true;
	//	SetCursor(m_StdCursor);
	Refresh(false, &r);
    }
}

void GfxCore::OnMButtonDown(wxMouseEvent& event)
{
    if (m_PlotData && m_Lock == lock_NONE) {
        m_DraggingMiddle = true;
	m_DragStart = wxPoint(event.GetX(), event.GetY());
    }
}

void GfxCore::OnMButtonUp(wxMouseEvent& event)
{
    if (m_PlotData && m_Lock == lock_NONE) {
	m_DraggingMiddle = false;
    }
}

void GfxCore::OnRButtonDown(wxMouseEvent& event)
{
    if (m_PlotData) {
        m_DragStart = wxPoint(event.GetX(), event.GetY());
	m_ScaleBar.drag_start_offset_x = m_ScaleBar.offset_x;
	m_ScaleBar.drag_start_offset_y = m_ScaleBar.offset_y;
        m_DraggingRight = true;
    }
}

void GfxCore::OnRButtonUp(wxMouseEvent& event)
{
    m_DraggingRight = false;
    m_LastDrag = drag_NONE;
}

void GfxCore::HandleScaleRotate(bool control, wxPoint point)
{
    // Handle a mouse movement during scale/rotate mode.

    int dx = point.x - m_DragStart.x;
    int dy = point.y - m_DragStart.y;

    double pan_angle = (m_Lock == lock_NONE || m_Lock == lock_Z || m_Lock == lock_XZ ||
			m_Lock == lock_YZ) ? -M_PI * (double(dx) / 500.0) : 0.0;

    Quaternion q;
    double new_scale = m_Params.scale;
    if (control || m_FreeRotMode) {
        // free rotation starts when Control is down

        if (!m_FreeRotMode) {
	  //	    m_Parent->SetStatusText("Free rotation mode.  Switch back using Delete.");
	    m_FreeRotMode = true;
	}

	double tilt_angle = M_PI * (double(dy) / 500.0);
	q.setFromEulerAngles(tilt_angle, 0.0, pan_angle);
    }
    else {
        // left/right => rotate, up/down => scale

      //	SetCursor(m_ScaleRotateCursor);

        if (m_ReverseControls) {
	    pan_angle = -pan_angle;
	}
	q.setFromVectorAndAngle(Vector3(XToScreen(0.0, 0.0, 1.0),
					YToScreen(0.0, 0.0, 1.0),
					ZToScreen(0.0, 0.0, 1.0)), pan_angle);

	m_PanAngle += pan_angle;
	if (m_PanAngle >= M_PI*2.0) {
	    m_PanAngle -= M_PI*2.0;
	}
	if (m_PanAngle < 0.0) {
	    m_PanAngle += M_PI*2.0;
	}
	new_scale *= pow(1.06, 0.08 * dy * (m_ReverseControls ? -1.0 : 1.0));
    }

    m_Params.rotation = q * m_Params.rotation;
    m_RotationMatrix = m_Params.rotation.asMatrix();

    SetScale(new_scale);

    m_RedrawOffscreen = true;
    Refresh(false);

    m_DragStart = point;
}

void GfxCore::TurnCave(double angle)
{
    // Turn the cave around its z-axis by a given angle.

    Vector3 v(XToScreen(0.0, 0.0, 1.0), YToScreen(0.0, 0.0, 1.0), ZToScreen(0.0, 0.0, 1.0));
    Quaternion q(v, angle);

    m_Params.rotation = q * m_Params.rotation;
    m_RotationMatrix = m_Params.rotation.asMatrix();

    m_PanAngle += angle;
    if (m_PanAngle > M_PI*2.0) {
        m_PanAngle -= M_PI*2.0;
    }
    if (m_PanAngle < 0.0) {
        m_PanAngle += M_PI*2.0;
    }
    
    SetScale(m_Params.scale);

    m_RedrawOffscreen = true;
    Refresh(false);
}

void GfxCore::TurnCaveTo(double angle)
{
    // Turn the cave to a particular pan angle.

    TurnCave(angle - m_PanAngle);
}

void GfxCore::TiltCave(double tilt_angle)
{
    // Tilt the cave by a given angle.

    if (m_ReverseControls) {
        tilt_angle = -tilt_angle;
    }

    if (m_TiltAngle + tilt_angle > M_PI / 2.0) {
        tilt_angle = (M_PI / 2.0) - m_TiltAngle;
    }

    if (m_TiltAngle + tilt_angle < -M_PI / 2.0) {
        tilt_angle = (-M_PI / 2.0) - m_TiltAngle;
    }

    m_TiltAngle += tilt_angle;

    Quaternion q;
    q.setFromEulerAngles(tilt_angle, 0.0, 0.0);

    m_Params.rotation = q * m_Params.rotation;
    m_RotationMatrix = m_Params.rotation.asMatrix();
    
    SetScale(m_Params.scale);
    
    m_RedrawOffscreen = true;
    Refresh(false);
}

void GfxCore::HandleTilt(wxPoint point)
{
    // Handle a mouse movement during tilt mode.

    if (!m_FreeRotMode) {
	int dy = point.y - m_DragStart.y;

	TiltCave(M_PI * (double(-dy) / 500.0));

	m_DragStart = point;
    }
}

void GfxCore::HandleTranslate(wxPoint point)
{
    // Handle a mouse movement during translation mode.

    int dx = point.x - m_DragStart.x;
    int dy = point.y - m_DragStart.y;

    // Find out how far the mouse motion takes us in cave coords.
    double x = double(dx / m_Params.scale);
    double z = double(-dy / m_Params.scale);
    Matrix4 inverse_rotation = m_Params.rotation.asInverseMatrix();
    double cx = double(inverse_rotation.get(0, 0)*x + inverse_rotation.get(0, 2)*z);
    double cy = double(inverse_rotation.get(1, 0)*x + inverse_rotation.get(1, 2)*z);
    double cz = double(inverse_rotation.get(2, 0)*x + inverse_rotation.get(2, 2)*z);
    
    // Update parameters and redraw.
    m_Params.translation.x += cx;
    m_Params.translation.y += cy;
    m_Params.translation.z += cz;

    if (!m_OverlappingNames) {
        //		m_LabelCacheNotInvalidated = true;//--fixme
        m_LabelShift.x = dx;
	m_LabelShift.y = dy;
	//	m_LabelCacheExtend.left = (dx < 0) ? m_XSize + dx : 0;
	//	m_LabelCacheExtend.right = (dx < 0) ? m_XSize : dx;
	//	m_LabelCacheExtend.top = (dy < 0) ? m_YSize + dy : 0;
	//	m_LabelCacheExtend.bottom = (dy < 0) ? m_YSize : dy;
    }

    SetScale(m_Params.scale);

    m_RedrawOffscreen = true;
    Refresh(false);

    m_DragStart = point;
}

void GfxCore::OnMouseMove(wxMouseEvent& event)
{
    // Mouse motion event handler.

    wxPoint point = wxPoint(event.GetX(), event.GetY());

    if (!m_SwitchingToPlan && !m_SwitchingToElevation) {
        if (m_DraggingLeft) {
	  if (!m_FreeRotMode) {
	      wxCoord x0 = m_XSize - INDICATOR_OFFSET_X - INDICATOR_BOX_SIZE/2;
	      wxCoord x1 = wxCoord(m_XSize - GetClinoOffset() - INDICATOR_BOX_SIZE/2);
	      wxCoord y = m_YSize - INDICATOR_OFFSET_Y - INDICATOR_BOX_SIZE/2;

	      wxCoord dx0 = point.x - x0;
	      wxCoord dx1 = point.x - x1;
	      wxCoord dy = point.y - y;

	      wxCoord radius = (INDICATOR_BOX_SIZE - INDICATOR_MARGIN*2) / 2;

	      if (m_Compass && sqrt(dx0*dx0 + dy*dy) <= radius && m_LastDrag == drag_NONE ||
		  m_LastDrag == drag_COMPASS) {
		  // drag in heading indicator
		  if (sqrt(dx0*dx0 + dy*dy) <= radius) {
		      TurnCaveTo(atan2(dx0, dy) - M_PI);
		      m_MouseOutsideCompass = false;
		  }
		  else {
		      TurnCaveTo(int(int((atan2(dx0, dy) - M_PI) * 180.0 / M_PI) / 45) *
				 M_PI/4.0);
		      m_MouseOutsideCompass = true;
		  }
		  m_LastDrag = drag_COMPASS;
	      }
	      else if (m_Clino && sqrt(dx1*dx1 + dy*dy) <= radius && m_LastDrag == drag_NONE ||
		  m_LastDrag == drag_ELEV) {
		  // drag in elevation indicator
		  m_LastDrag = drag_ELEV;
		  if (dx1 >= 0 && sqrt(dx1*dx1 + dy*dy) <= radius) {
		      TiltCave(atan2(dy, dx1) - m_TiltAngle);
		      m_MouseOutsideElev = false;
		  }
		  else if (dy >= INDICATOR_MARGIN) {
		      TiltCave(M_PI/2.0 - m_TiltAngle);
		      m_MouseOutsideElev = true;
		  }
		  else if (dy <= -INDICATOR_MARGIN) {
		      TiltCave(-M_PI/2.0 - m_TiltAngle);
		      m_MouseOutsideElev = true;
		  }
		  else {
		      TiltCave(-m_TiltAngle);
		      m_MouseOutsideElev = true;
		  }
	      }
	      else if ((m_LastDrag == drag_NONE &&
		       point.x >= m_ScaleBar.offset_x &&
		       point.x <= m_ScaleBar.offset_x + m_ScaleBar.width &&
		       point.y <= m_YSize - m_ScaleBar.offset_y &&
		       point.y >= m_YSize - m_ScaleBar.offset_y - SCALE_BAR_HEIGHT) ||
		       m_LastDrag == drag_SCALE) {
 		  if (point.x >= 0 && point.x <= m_XSize) {
		      m_LastDrag = drag_SCALE;
		      SetScale(m_Params.scale * pow(1.06, 0.01 *
						    (-m_DragStart.x + point.x)));
		      m_RedrawOffscreen = true;
		      Refresh(false);
		  }
	      }
	      else if (m_LastDrag == drag_NONE || m_LastDrag == drag_MAIN) {
		  m_LastDrag = drag_MAIN;
		  HandleScaleRotate(event.ControlDown(), point);
	      }
	  }
	  else {
	      HandleScaleRotate(event.ControlDown(), point);
	  }
	}
	else if (m_DraggingMiddle) {
	    HandleTilt(point);
	}
	else if (m_DraggingRight) {
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
		  m_RedrawOffscreen = true;
		  Refresh(false);
	    }
	    else {
	        m_LastDrag = drag_MAIN;
	        HandleTranslate(point);
	    }
	}
    }
}

void GfxCore::OnSize(wxSizeEvent& event) 
{
    // Handle a change in window size.

    wxSize size = event.GetSize();

    m_XSize = size.GetWidth();
    m_YSize = size.GetHeight();
    m_XCentre = m_XSize / 2;
    m_YCentre = m_YSize / 2;

    if (m_InitialisePending) {
        Initialise();
	m_InitialisePending = false;
	m_DoneFirstShow = true;
    }

    if (m_DoneFirstShow) {
#ifndef __WXMOTIF__
        m_DrawDC.SelectObject(wxNullBitmap);
#endif
	m_OffscreenBitmap.Create(m_XSize, m_YSize);
	m_DrawDC.SelectObject(m_OffscreenBitmap);
        RedrawOffscreen();
	Refresh(false);
    }
}

void GfxCore::OnDisplayOverlappingNames(wxCommandEvent&)
{
    
    m_OverlappingNames = !m_OverlappingNames;
    SetScale(m_Params.scale);
    m_RedrawOffscreen = true;
    Refresh(false);
}

void GfxCore::OnDisplayOverlappingNamesUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && m_Names);
    cmd.Check(m_OverlappingNames);
}

void GfxCore::OnShowCrosses(wxCommandEvent&)
{
    m_Crosses = !m_Crosses;
    m_RedrawOffscreen = true;
    m_ScaleCrossesOnly = true;
    SetScale(m_Params.scale);
    Refresh(false);
}

void GfxCore::OnShowCrossesUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && m_Lock != lock_POINT);
    cmd.Check(m_Crosses);
}

void GfxCore::OnShowStationNames(wxCommandEvent&)
{
    m_Names = !m_Names;
    SetScale(m_Params.scale);
    m_RedrawOffscreen = true;
    Refresh(false);
}

void GfxCore::OnShowStationNamesUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL);
    cmd.Check(m_Names);
}

void GfxCore::OnShowSurveyLegs(wxCommandEvent&) 
{
    m_Legs = !m_Legs;
    m_RedrawOffscreen = true;
    Refresh(false);
}

void GfxCore::OnShowSurveyLegsUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && m_Lock != lock_POINT && m_UndergroundLegs);
    cmd.Check(m_Legs);
}

void GfxCore::OnMoveEast(wxCommandEvent&) 
{
    TurnCaveTo(M_PI / 2.0);
}

void GfxCore::OnMoveEastUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && !m_FreeRotMode && m_Lock != lock_POINT &&
	       m_Lock != lock_Y && m_Lock != lock_XY);
}

void GfxCore::OnMoveNorth(wxCommandEvent&) 
{
    TurnCaveTo(0.0);
}

void GfxCore::OnMoveNorthUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && !m_FreeRotMode && m_Lock != lock_POINT &&
	       m_Lock != lock_X && m_Lock != lock_XY);
}

void GfxCore::OnMoveSouth(wxCommandEvent&) 
{
    TurnCaveTo(M_PI);
}

void GfxCore::OnMoveSouthUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && !m_FreeRotMode && m_Lock != lock_POINT &&
	       m_Lock != lock_X && m_Lock != lock_XY);
}

void GfxCore::OnMoveWest(wxCommandEvent&)
{
    TurnCaveTo(M_PI * 1.5);
}

void GfxCore::OnMoveWestUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && !m_FreeRotMode && m_Lock != lock_POINT &&
	       m_Lock != lock_Y && m_Lock != lock_XY);
}

void GfxCore::StartTimer()
{
    m_Timer.Start(100);
}

void GfxCore::StopTimer()
{
    m_Timer.Stop();
}

void GfxCore::OnStartRotation(wxCommandEvent&) 
{
  //    StartTimer();
    m_Rotating = true;
}

void GfxCore::OnStartRotationUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && !m_FreeRotMode && !m_Rotating &&
	       (m_Lock == lock_NONE || m_Lock == lock_Z || m_Lock == lock_XZ ||
		m_Lock == lock_YZ));
}

void GfxCore::OnStopRotation(wxCommandEvent&) 
{
    if (!m_SwitchingToElevation && !m_SwitchingToPlan) {
      //        StopTimer();
    }

    m_Rotating = false;
}

void GfxCore::OnStopRotationUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && !m_FreeRotMode && m_Rotating);
}

void GfxCore::OnReverseControls(wxCommandEvent&) 
{
    m_ReverseControls = !m_ReverseControls;
}

void GfxCore::OnReverseControlsUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && !m_FreeRotMode);
    cmd.Check(m_ReverseControls);
}

void GfxCore::OnReverseDirectionOfRotation(wxCommandEvent&)
{
    m_RotationStep = -m_RotationStep;
}

void GfxCore::OnReverseDirectionOfRotationUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && m_Rotating);
}

void GfxCore::OnSlowDown(wxCommandEvent&) 
{
    m_RotationStep /= 1.2f;
    if (m_RotationStep < M_PI/2000.0f) {
        m_RotationStep = (double) M_PI/2000.0f;
    }
}

void GfxCore::OnSlowDownUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && m_Rotating);
}

void GfxCore::OnSpeedUp(wxCommandEvent&) 
{
    m_RotationStep *= 1.2f;
    if (m_RotationStep > M_PI/8.0f) {
        m_RotationStep = (double) M_PI/8.0f;
    }
}

void GfxCore::OnSpeedUpUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && m_Rotating);
}

void GfxCore::OnStepOnceAnticlockwise(wxCommandEvent&) 
{
    TurnCave(M_PI / 18.0);
}

void GfxCore::OnStepOnceAnticlockwiseUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && !m_FreeRotMode && !m_Rotating && m_Lock != lock_POINT);
}

void GfxCore::OnStepOnceClockwise(wxCommandEvent&) 
{
    TurnCave(-M_PI / 18.0);
}

void GfxCore::OnStepOnceClockwiseUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && !m_FreeRotMode && !m_Rotating && m_Lock != lock_POINT);
}

void GfxCore::OnDefaults(wxCommandEvent&) 
{
    Defaults();
}

void GfxCore::DefaultParameters()
{
    m_TiltAngle = M_PI / 2.0;
    m_PanAngle = 0.0;

    m_Params.rotation.setFromEulerAngles(m_TiltAngle, 0.0, m_PanAngle);
    m_RotationMatrix = m_Params.rotation.asMatrix();

    m_Params.translation.x = 0.0;
    m_Params.translation.y = 0.0;
    m_Params.translation.z = 0.0;
    
    m_Params.display_shift.x = 0;
    m_Params.display_shift.y = 0;

    m_ScaleCrossesOnly = false;
    m_Surface = false;
    m_SurfaceDepth = false;
    m_SurfaceDashed = false;
    m_FreeRotMode = false;
    m_RotationStep = M_PI / 180.0;
    m_Rotating = false;
    m_SwitchingToPlan = false;
    m_SwitchingToElevation = false;
    m_Entrances = false;
    m_FixedPts = false;
    m_ExportedPts = false;
}

void GfxCore::Defaults()
{
    // Restore default scale, rotation and translation parameters.

    DefaultParameters();
    SetScale(m_InitialScale);
    m_RedrawOffscreen = true;
    Refresh(false);
}

void GfxCore::OnDefaultsUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL);
}

void GfxCore::OnElevation(wxCommandEvent&)
{
    Elevation();
}

void GfxCore::Elevation() 
{
    // Switch to elevation view.

    m_SwitchingToElevation = true;
    m_SwitchingToPlan = false;
}

void GfxCore::OnElevationUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && !m_FreeRotMode && !m_SwitchingToPlan &&
		!m_SwitchingToElevation && m_Lock == lock_NONE);
}

void GfxCore::OnHigherViewpoint(wxCommandEvent&) 
{
    // Raise the viewpoint.

    TiltCave(M_PI / 18.0);
}

void GfxCore::OnHigherViewpointUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && !m_FreeRotMode && m_TiltAngle < M_PI / 2.0 &&
	       m_Lock == lock_NONE);
}

void GfxCore::OnLowerViewpoint(wxCommandEvent&) 
{
    // Lower the viewpoint.

    TiltCave(-M_PI / 18.0);
}

void GfxCore::OnLowerViewpointUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && !m_FreeRotMode && m_TiltAngle > -M_PI / 2.0 &&
	       m_Lock == lock_NONE);
}

void GfxCore::OnPlan(wxCommandEvent&)
{
    Plan();
}

void GfxCore::Plan() 
{
    // Switch to plan view.

    m_SwitchingToPlan = true;
    m_SwitchingToElevation = false;
}

void GfxCore::OnPlanUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && !m_FreeRotMode && !m_SwitchingToElevation &&
		!m_SwitchingToPlan && m_Lock == lock_NONE);
}

void GfxCore::OnShiftDisplayDown(wxCommandEvent&) 
{
    m_Params.display_shift.y += DISPLAY_SHIFT;
    SetScale(m_Params.scale);
    m_RedrawOffscreen = true;
    Refresh(false);
}

void GfxCore::OnShiftDisplayDownUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL);
}

void GfxCore::OnShiftDisplayLeft(wxCommandEvent&) 
{
    m_Params.display_shift.x -= DISPLAY_SHIFT;
    SetScale(m_Params.scale);
    m_RedrawOffscreen = true;
    Refresh(false);
}

void GfxCore::OnShiftDisplayLeftUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData != NULL);
}

void GfxCore::OnShiftDisplayRight(wxCommandEvent&) 
{
    m_Params.display_shift.x += DISPLAY_SHIFT;
    SetScale(m_Params.scale);
    m_RedrawOffscreen = true;
    Refresh(false);
}

void GfxCore::OnShiftDisplayRightUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL);
}

void GfxCore::OnShiftDisplayUp(wxCommandEvent&) 
{
    m_Params.display_shift.y -= DISPLAY_SHIFT;
    SetScale(m_Params.scale);
    m_RedrawOffscreen = true;
    Refresh(false);
}

void GfxCore::OnShiftDisplayUpUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL);
}

void GfxCore::OnZoomIn(wxCommandEvent&) 
{
    // Increase the scale.

    m_Params.scale *= 1.06f;
    SetScale(m_Params.scale);
    m_RedrawOffscreen = true;
    Refresh(false);
}

void GfxCore::OnZoomInUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && m_Lock != lock_POINT);
}

void GfxCore::OnZoomOut(wxCommandEvent&) 
{
    // Decrease the scale.

    m_Params.scale /= 1.06f;
    SetScale(m_Params.scale);
    m_RedrawOffscreen = true;
    Refresh(false);
}

void GfxCore::OnZoomOutUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && m_Lock != lock_POINT);
}

void GfxCore::OnTimer(wxTimerEvent& event)
{
    // Handle a timer event.

    // When rotating...
    if (m_Rotating) {
        TurnCave(m_RotationStep);
    }
    // When switching to plan view...
    if (m_SwitchingToPlan) {
        if (m_TiltAngle == M_PI / 2.0) {
	    if (!m_Rotating) {
	      //	        StopTimer();
	    }
	    m_SwitchingToPlan = false;
	}
	TiltCave(M_PI / 30.0);
    }
    // When switching to elevation view;;;
    if (m_SwitchingToElevation) {
        if (m_TiltAngle == 0.0) {
	    if (!m_Rotating) {
	      //	        StopTimer();
	    }
	    m_SwitchingToElevation = false;
	}
	else if (m_TiltAngle < 0.0) {
	    if (m_TiltAngle > (-M_PI / 30.0)) {
	        TiltCave(-m_TiltAngle);
	    }
	    else {
	        TiltCave(M_PI / 30.0);
	    }
	    if (m_TiltAngle >= 0.0) {
	        if (!m_Rotating) {
		  //		    StopTimer();
		}
		m_SwitchingToElevation = false;
	    }
	}
	else {
	    if (m_TiltAngle < (M_PI / 30.0)) {
	        TiltCave(-m_TiltAngle);
	    }
	    else {
	        TiltCave(-M_PI / 30.0);
	    }
	    
	    if (m_TiltAngle <= 0.0) {
	        if (!m_Rotating) {
		  //		    StopTimer();
		}
		m_SwitchingToElevation = false;
	    }
	}
    }
}

void GfxCore::OnToggleScalebar(wxCommandEvent&) 
{
    m_Scalebar = !m_Scalebar;
    m_RedrawOffscreen = true;
    Refresh(false);
}

void GfxCore::OnToggleScalebarUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && !m_ScalebarOff);
    cmd.Check(m_Scalebar);
}

void GfxCore::OnToggleDepthbar(wxCommandEvent&) 
{
    m_Depthbar = !m_Depthbar;
    m_RedrawOffscreen = true;
    Refresh(false);
}

void GfxCore::OnToggleDepthbarUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && !m_DepthbarOff);
    cmd.Check(m_Depthbar);
}

void GfxCore::OnViewCompass(wxCommandEvent&) 
{
    m_Compass = !m_Compass;
    m_RedrawOffscreen = true;
    Refresh(false);
}

void GfxCore::OnViewCompassUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && !m_FreeRotMode && !m_IndicatorsOff);
    cmd.Check(m_Compass);
}

void GfxCore::OnViewClino(wxCommandEvent&) 
{
    m_Clino = !m_Clino;
    m_RedrawOffscreen = true;
    Refresh(false);
}

void GfxCore::OnViewClinoUpdate(wxUpdateUIEvent& cmd) 
{
    cmd.Enable(m_PlotData != NULL && !m_FreeRotMode && !m_IndicatorsOff &&
	       m_Lock == lock_NONE);
    cmd.Check(m_Clino);
}

void GfxCore::OnShowSurface(wxCommandEvent& cmd)
{
    m_Surface = !m_Surface;
    m_RedrawOffscreen = true;
    Refresh(false);
}

void GfxCore::OnShowSurfaceDepth(wxCommandEvent& cmd)
{
    m_SurfaceDepth = !m_SurfaceDepth;
    m_RedrawOffscreen = true;
    Refresh(false);
}

void GfxCore::OnShowSurfaceDashed(wxCommandEvent& cmd)
{
    m_SurfaceDashed = !m_SurfaceDashed;
    m_RedrawOffscreen = true;
    Refresh(false);
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

void GfxCore::OnShowEntrances(wxCommandEvent&)
{
    m_Entrances = !m_Entrances;
    m_RedrawOffscreen = true;
    m_ScaleHighlightedPtsOnly = true;
    SetScale(m_Params.scale);
    Refresh(false);
}

void GfxCore::OnShowEntrancesUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData && (m_Parent->GetNumEntrances() > 0));
    cmd.Check(m_Entrances);
}

void GfxCore::OnShowFixedPts(wxCommandEvent&)
{
    m_FixedPts = !m_FixedPts;
    m_RedrawOffscreen = true;
    m_ScaleHighlightedPtsOnly = true;
    SetScale(m_Params.scale);
    Refresh(false);
}

void GfxCore::OnShowFixedPtsUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData && (m_Parent->GetNumFixedPts() > 0));
    cmd.Check(m_FixedPts);
}

void GfxCore::OnShowExportedPts(wxCommandEvent&)
{
    m_ExportedPts = !m_ExportedPts;
    m_RedrawOffscreen = true;
    m_ScaleHighlightedPtsOnly = true;
    SetScale(m_Params.scale);
    Refresh(false);
}

void GfxCore::OnShowExportedPtsUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_PlotData && (m_Parent->GetNumExportedPts() > 0));
    cmd.Check(m_ExportedPts);
}
