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

#include <math.h>

#define MAX(a, b)    (((a) > (b)) ? (a) : (b))
#define TEXT_COLOUR  RGB(0, 255, 40)
#define LABEL_COLOUR RGB(160, 255, 0)

static const int FONT_SIZE = 14;
static const int CROSS_SIZE = 5;
static const float COMPASS_SIZE = 24.0f;
static const long COMPASS_OFFSET_X = 60;
static const long COMPASS_OFFSET_Y = 80;
static const int DISPLAY_SHIFT = 50;
static const int TIMER_ID = 0;

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
    EVT_TIMER(TIMER_ID, GfxCore::OnTimer)
END_EVENT_TABLE()

GfxCore::GfxCore(MainFrm* parent) :
    wxWindow(parent, -1), m_Timer(this, TIMER_ID)
{
    m_DraggingLeft = false;
    m_DraggingMiddle = false;
    m_DraggingRight = false;
    m_Parent = parent;
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
    m_Depthbar = true;
    m_Scalebar = true;
    m_ReverseControls = false;
    m_CaverotMouse = true;
    m_LabelGrid = NULL;
    m_Rotating = false;
    m_SwitchingToPlan = false;
    m_SwitchingToElevation = false;

    // Create pens and brushes for drawing.
    m_Pens.black.SetColour(0, 0, 0);
    m_Pens.grey.SetColour(100, 100, 100);
    m_Pens.white.SetColour(255, 255, 255);
    m_Pens.turquoise.SetColour(0, 100, 255);
    m_Pens.green.SetColour(0, 255, 40);

    m_Brushes.black.SetColour(0, 0, 0);
    m_Brushes.white.SetColour(255, 255, 255);
    m_Brushes.grey.SetColour(100, 100, 100);
/*
    // Create the font used on the display.
    m_Font.CreateFont(FONT_SIZE, 0, 0, 0, FW_NORMAL, false, false, 0, ANSI_CHARSET,
		      OUT_DEFAULT_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY,
		      FF_MODERN, "Arial");*/
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
	    delete[] m_PlotData[band].vertices;
	    delete[] m_PlotData[band].num_segs;
	}

	delete[] m_PlotData;
	delete[] m_Polylines;
	delete[] m_CrossData.vertices;
	delete[] m_CrossData.num_segs;
	delete[] m_Labels;
	delete[] m_LabelsLastPlotted;

	if (m_LabelGrid) {
	    delete[] m_LabelGrid;
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

    m_Bands = m_Parent->GetNumDepthBands();
    m_PlotData = new PlotData[m_Bands];
    m_Polylines = new int[m_Bands];
    m_CrossData.vertices = new wxPoint[m_Parent->GetNumCrosses() * 4];
    m_CrossData.num_segs = new int[m_Parent->GetNumCrosses() * 2];
    m_Labels = new wxString[m_Parent->GetNumCrosses()];
    m_LabelsLastPlotted = new LabelFlags[m_Parent->GetNumCrosses()];
    m_LabelCacheNotInvalidated = false;

    for (int band = 0; band < m_Bands; band++) {
        m_PlotData[band].vertices = new wxPoint[m_Parent->GetNumPoints()];
	m_PlotData[band].num_segs = new int[m_Parent->GetNumLegs()];
    }

    // Scale the survey to a reasonable initial size.
    m_InitialScale = double(m_XSize) / (double(MAX(m_Parent->GetXExtent(),
						   m_Parent->GetYExtent())) * 1.1);

    // Apply default settings and redraw.
    Defaults();
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
    //	m_DrawDC.SelectObject(&m_Font);

    m_DoneFirstShow = true;

    RedrawOffscreen();
}

//
//  Recalculating methods
//

void GfxCore::SetScale(double scale)
{
    // Fill the plot data arrays with screen coordinates, scaling the survey
    // to a particular absolute scale.  The input points must be such that
    // the conditions for CDC::PolyPolyLine are satisfied (in particular, all polylines
    // must have >= 2 vertices or nothing appears on the screen).

    m_Params.scale = scale;

    if (m_Params.scale < m_InitialScale / 20.0) {
        m_Params.scale = m_InitialScale / 20.0;
    }

    if (!m_ScaleCrossesOnly) {
        for (int band = 0; band < m_Bands; band++) {
	    wxPoint* pt = m_PlotData[band].vertices;
	    assert(pt);
	    int* count = m_PlotData[band].num_segs;
	    assert(count);
	    count--; // MainFrm guarantees the first point will be a move.

	    float x, y, z;
	    bool is_line;

	    m_Polylines[band] = 0;
	    list<PointInfo*>::iterator pos = m_Parent->GetFirstPoint(band);
	    bool more = true;
	    while (more) {
	        more = m_Parent->GetNextPoint(band, pos, x, y, z, is_line);
		if (!more) break; //--sort this out...

		x += m_Params.translation.x;
		y += m_Params.translation.y;
		z += m_Params.translation.z;

		pt->x = (long) (XToScreen(x, y, z) * scale) + m_Params.display_shift.x;
		pt->y = -(long) (ZToScreen(x, y, z) * scale) + m_Params.display_shift.y;
		pt++;

		if (!is_line) {
		    // new polyline
		    *++count = 1;
		    m_Polylines[band]++;
		}
		else if (is_line) {
		    // still part of the same polyline => increment count of vertices
		    // for this polyline.
		    (*count)++;
		}
	    }
	}
    }
    else {
        m_ScaleCrossesOnly = false;
    }

    if (m_Crosses || m_Names) {
        // Construct polylines for crosses and sort out station names.
        wxPoint* pt = m_CrossData.vertices;
	int* count = m_CrossData.num_segs;
	wxString* labels = m_Labels;
	list<LabelInfo*>::iterator pos = m_Parent->GetFirstLabel();
	float x, y, z;
	wxString text;
	bool more = true;
	while (more) {
	    more = m_Parent->GetNextLabel(pos, x, y, z, text);
	    if (!more) break;

	    x += m_Params.translation.x;
	    y += m_Params.translation.y;
	    z += m_Params.translation.z;

	    long cx = (long) (XToScreen(x, y, z) * scale) + m_Params.display_shift.x;
	    long cy = -(long) (ZToScreen(x, y, z) * scale) + m_Params.display_shift.y;

	    pt->x = cx;
	    pt->y = cy - CROSS_SIZE;
	    pt++;
	    pt->x = cx;
	    pt->y = cy + CROSS_SIZE;
	    pt++;
	    pt->x = cx - CROSS_SIZE;
	    pt->y = cy;
	    pt++;
	    pt->x = cx + CROSS_SIZE;
	    pt->y = cy;
	    pt++;
	    
	    *count++ = 2;
	    *count++ = 2;
	    
	    *labels++ = text;
	}
    }
}

//
//  Repainting methods
//

void GfxCore::RedrawOffscreen()
{
    // Redraw the offscreen bitmap.

    m_DrawDC.BeginDrawing();
  
    if (!m_DoneFirstShow) {
        FirstShow();
    }

    // Clear the background to black.
    m_DrawDC.SetPen(m_Pens.black);
    // m_DrawDC.SetWindowOrg(0, 0);
    m_DrawDC.SetBrush(m_Brushes.black);
    m_DrawDC.DrawRectangle(0, 0, m_XSize, m_YSize);
	
    //m_DrawDC.SetWindowOrg(-m_XCentre, -m_YCentre);

    if (m_PlotData) {
        // Draw the legs.
        if (m_Legs) {
	    for (int band = 0; band < m_Bands; band++) {
	        m_DrawDC.SetPen(m_Parent->GetPen(band));
		int* num_segs = m_PlotData[band].num_segs;
		wxPoint* vertices = m_PlotData[band].vertices;
		for (int polyline = 0; polyline < m_Polylines[band]; polyline++) {
		    m_DrawDC.DrawLines(*num_segs, vertices, m_XCentre, m_YCentre);
		    vertices += *num_segs++;
		}
	    }
	}

#if 0
	// Draw crosses.
	if (m_Crosses) {
	    m_DrawDC.SelectObject(m_Pens.turquoise);
	    m_DrawDC.PolyPolyline(m_CrossData.vertices, m_CrossData.num_segs,
				  m_Parent->GetNumCrosses() * 2);
	}

	// Draw station names.
	if (m_Names) {
	    DrawNames();
	    m_LabelCacheNotRefreshd = false;
	}

	// Draw scalebar.
	if (m_Scalebar) {
	    DrawScalebar();
	}

	// Draw depthbar.
	if (m_Depthbar) {
	    DrawDepthbar();
	}

	// Draw compass.
	if (m_Compass) {
	    DrawCompass();
	}
#endif
    }

    m_DrawDC.EndDrawing();
}

void GfxCore::OnPaint(wxPaintEvent& event) 
{
    // Redraw the window.

    // Make sure we're initialised.
    if (!m_DoneFirstShow) {
        FirstShow();
    }

    // Redraw the offscreen bitmap if it's out of date.
    if (m_RedrawOffscreen) {
        m_RedrawOffscreen = false;
	RedrawOffscreen();
    }

    // Get a graphics context.
    wxPaintDC dc(this);

    dc.BeginDrawing();

    // Get the areas to redraw and update them.
    wxRegionIterator iter(GetUpdateRegion());
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

#if 0
POINT GfxCore::CompassPtToScreen(float x, float y, float z)
{
	POINT pt;
	pt.x = long(-XToScreen(x, y, z)) + m_XSize/2 - COMPASS_OFFSET_X;
	pt.y = long(ZToScreen(x, y, z)) + m_YSize/2 - COMPASS_OFFSET_Y;
	return pt;
}

void GfxCore::DrawCompass()
{
	// Draw the compass.

	POINT pt[12];
	const DWORD count[] = { 2, 3, 2, 2, 3 };
	pt[0] = CompassPtToScreen(0.0, 0.0, -COMPASS_SIZE);
	pt[1] = CompassPtToScreen(0.0, 0.0, COMPASS_SIZE);
	pt[2] = CompassPtToScreen(-COMPASS_SIZE / 3.0f, 0.0, -COMPASS_SIZE * 2.0f / 3.0f);
	pt[3] = pt[0];
	pt[4] = CompassPtToScreen(COMPASS_SIZE / 3.0f, 0.0, -COMPASS_SIZE * 2.0f / 3.0f);

	pt[5] = CompassPtToScreen(-COMPASS_SIZE, 0.0, 0.0);
	pt[6] = CompassPtToScreen(COMPASS_SIZE, 0.0, 0.0);
	pt[7] = CompassPtToScreen(0.0, -COMPASS_SIZE, 0.0);
	pt[8] = CompassPtToScreen(0.0, COMPASS_SIZE, 0.0);
	pt[9] = CompassPtToScreen(-COMPASS_SIZE / 3.0f, -COMPASS_SIZE * 2.0f / 3.0f, 0.0);
	pt[10] = pt[7];
	pt[11] = CompassPtToScreen(COMPASS_SIZE / 3.0f, -COMPASS_SIZE * 2.0f / 3.0f, 0.0);

	m_DrawDC.SelectObject(m_Pens.turquoise);
	m_DrawDC.PolyPolyline(pt, count, 2);

	m_DrawDC.SelectObject(m_Pens.green);
	m_DrawDC.PolyPolyline(&pt[5], &count[2], 3);

	if (!m_FreeRotMode) {
		m_DrawDC.SetBkColor(RGB(0, 0, 0));
		m_DrawDC.SetTextColor(TEXT_COLOUR);

		CString str;
		str.Format("Hdg %03d", int(m_PanAngle * 180.0 / M_PI));
		m_DrawDC.TextOut(int(m_XSize/2 - COMPASS_OFFSET_X - COMPASS_SIZE*3),
			             int(m_YSize/2 - COMPASS_OFFSET_Y - FONT_SIZE*1.5), str);

		str.Format("Elev %03d", int(m_TiltAngle * 180.0 / M_PI));
		m_DrawDC.TextOut(int(m_XSize/2 - COMPASS_OFFSET_X - COMPASS_SIZE*3),
			             int(m_YSize/2 - COMPASS_OFFSET_Y + FONT_SIZE/2.0), str);
	}
}

void GfxCore::DrawNames()
{
	// Draw station names.

	m_DrawDC.SetBkColor(RGB(0, 0, 0));
	m_DrawDC.SetTextColor(LABEL_COLOUR);

	if (m_OverlappingNames || m_LabelCacheNotRefreshd) {
		SimpleDrawNames();
		// Draw names on bits of the survey which weren't visible when
		// the label cache was last generated (happens after a translation...)
		if (m_LabelCacheNotRefreshd) {
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
	const int quantise = FONT_SIZE / dv;
	const int quantised_x = m_XSize / quantise;
	const int quantised_y = m_YSize / quantise;
	const size_t buffer_size = quantised_x * quantised_y;
	if (!m_LabelCacheNotRefreshd) {
		if (m_LabelGrid) {
			delete[] m_LabelGrid;
		}
		m_LabelGrid = new BOOL[buffer_size];
		memset((void*) m_LabelGrid, 0, buffer_size * sizeof(LabelFlags));
	}

	CString* label = m_Labels;
	LabelFlags* last_plot = m_LabelsLastPlotted;
	POINT* pt = m_CrossData.vertices;
	for (int name = 0; name < m_Parent->GetNumCrosses(); name++) {
		// *pt is at (cx, cy - CROSS_SIZE), where (cx, cy) are the coordinates of
		// the actual station.

		long x = pt->x + m_XSize/2;
		long y = pt->y + CROSS_SIZE - FONT_SIZE + m_YSize/2;

		// We may have labels in the cache which are still going to be in the
		// same place: in this case we only consider labels here which are in
		// just about the region of screen exposed since the last label cache generation,
		// and were not plotted last time, together with labels which were in
		// this category last time around but didn't end up plotted (possibly because
		// the newly exposed region was too small, as happens during a drag).
#ifdef _DEBUG
		if (m_LabelCacheNotRefreshd) {
			ASSERT(m_LabelCacheExtend.bottom >= m_LabelCacheExtend.top);
			ASSERT(m_LabelCacheExtend.right >= m_LabelCacheExtend.left);
		}
#endif

		if ((m_LabelCacheNotRefreshd && x >= m_LabelCacheExtend.left &&
			 x <= m_LabelCacheExtend.right && y >= m_LabelCacheExtend.top &&
			 y <= m_LabelCacheExtend.bottom && *last_plot == label_NOT_PLOTTED) ||
			(m_LabelCacheNotRefreshd && *last_plot == label_CHECK_AGAIN) ||
			!m_LabelCacheNotRefreshd) {
		
			CString str = *label;

#ifdef _DEBUG
			if (m_LabelCacheNotRefreshd && *last_plot == label_CHECK_AGAIN) {
				TRACE("Label " + str + " being checked again.\n");
			}
#endif

			int ix = int(x) / quantise;
			int iy = int(y) / quantise;
			int ixshift = m_LabelCacheNotRefreshd ? int(m_LabelShift.x / quantise) : 0;
	//		if (ix + ixshift >= quantised_x) {
	//			ixshift = 
			int iyshift = m_LabelCacheNotRefreshd ? int(m_LabelShift.y / quantise) : 0;

			if (ix >= 0 && ix < quantised_x && iy >= 0 && iy < quantised_y) {
				BOOL* test = &m_LabelGrid[ix + ixshift + (iy + iyshift)*quantised_x];
				int len = str.GetLength()*dv + 1;
				BOOL reject = (ix + len >= quantised_x);
				int i = 0;
				while (!reject && i++ < len) {
					reject = *test++;
				}

				if (!reject) {
					m_DrawDC.TextOut(x - m_XSize/2, y - m_YSize/2, str);
					int ymin = (iy >= 2) ? iy - 2 : iy;
					int ymax = (iy < quantised_y - 2) ? iy + 2 : iy;
					for (int y0 = ymin; y0 <= ymax; y0++) {
						memset((void*) &m_LabelGrid[ix + y0*quantised_x], true,
							   sizeof(LabelFlags) * len);
					}
				}

				if (reject) {
					*last_plot++ = m_LabelCacheNotRefreshd ? label_CHECK_AGAIN :
					                                            label_NOT_PLOTTED;
				}
				else {
					*last_plot++ = label_PLOTTED;
				}
			}
			else {
				*last_plot++ = m_LabelCacheNotRefreshd ? label_CHECK_AGAIN :
				                                            label_NOT_PLOTTED;
			}
		}
		else {
			if (m_LabelCacheNotRefreshd && x >= m_LabelCacheExtend.left - 50 &&
			    x <= m_LabelCacheExtend.right + 50 && y >= m_LabelCacheExtend.top - 50 &&
			    y <= m_LabelCacheExtend.bottom + 50) {
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

	CString* label = m_Labels;
	POINT* pt = m_CrossData.vertices;
	LabelFlags* last_plot = m_LabelsLastPlotted;
	for (int name = 0; name < m_Parent->GetNumCrosses(); name++) {
		// *pt is at (cx, cy - CROSS_SIZE), where (cx, cy) are the coordinates of
		// the actual station.

		if ((m_LabelCacheNotRefreshd && *last_plot == label_PLOTTED) ||
			 !m_LabelCacheNotRefreshd) {
			m_DrawDC.TextOut(pt->x, pt->y + CROSS_SIZE - FONT_SIZE, *label);
		}

		last_plot++;
		label++;
		pt += 4;
	}
}

void GfxCore::DrawDepthbar()
{
	// Draw the depthbar.

	m_DrawDC.SetBkColor(RGB(0, 0, 0));
	m_DrawDC.SetTextColor(TEXT_COLOUR);

	int width = 20;
	int height = 16;
	int x_min = m_XSize/2 - width - 50;
	int y = -m_YSize/2 + (height*m_Bands) + 15;

	for (int band = 0; band <= m_Bands; band++) {
		if (band < m_Bands) {
			m_DrawDC.SelectObject(m_Parent->GetPen(band));
			m_DrawDC.FillRect(CRect(x_min, y - height, x_min + width, y),
							  m_Parent->GetBrush(band));
		}

		float z = m_Parent->GetZMin() + (m_Parent->GetZExtent() * band / m_Bands);
		CString str;
		str.Format("%.0fm", z);
		m_DrawDC.TextOut(x_min + width + 5, y - (FONT_SIZE / 2), str);

		y -= height;
	}
}

void GfxCore::DrawScalebar()
{
	// Draw the scalebar.

	// Calculate the extent of the survey, in metres across the screen plane.
    float m_across_screen = float(m_XSize / m_Params.scale);
	// The scalebar will represent this distance:
    float size_snap = (float) pow(10.0, int(log10(m_across_screen * 0.94)));
	// Actual size of the thing in pixels:
    int size = int(size_snap * m_Params.scale);
    
    // Draw it...
    int end_x = m_XSize/2 - 15;
    int height = 12;
    int end_y = m_YSize/2 - 15 - height;
    int interval = size / 10;

    BOOL solid = true;
    for (int ix = 0; ix < 10; ix++) {
        int x = end_x - size + int(ix * ((float) size / 10.0));
        
		m_DrawDC.SelectObject(solid ? m_Pens.grey : m_Pens.white);
        m_DrawDC.FillRect(CRect(x, end_y, x + interval + 2, end_y + height),
			              solid ? &m_Brushes.grey : &m_Brushes.white);

        solid = !solid;
    }

	// Add labels.
	float dv = size_snap * 100.0f;
	CString str;
	CString str2;
	if (dv < 1.0) {
		str = "0mm";
		if (dv < 0.001) {
			str2.Format("%.02fmm", dv * 10.0);
		}
		else {
			str2.Format("%.fmm", dv * 10.0);
		}
	}
	else if (dv < 100.0) {
		str = "0cm";
		str2.Format("%.02fcm", dv);
	}
	else if (dv < 100000.0) {
		str = "0m";
		str2.Format("%.02fm", dv / 100.0);
	}
	else {
		str = "0km";
		str2.Format("%.02fkm", dv / 100000.0);
	}

	m_DrawDC.SetBkColor(RGB(0, 0, 0));
	m_DrawDC.SetTextColor(TEXT_COLOUR);
	m_DrawDC.TextOut(end_x - size, end_y - FONT_SIZE - 4, str);
	CSize text_size = m_DrawDC.GetTextExtent(str2);
	m_DrawDC.TextOut(end_x - text_size.cx, end_y - FONT_SIZE - 4, str2);
}
#endif

//
//  Mouse event handling methods
//

void GfxCore::OnLButtonDown(wxMouseEvent& event)
{
    m_DraggingLeft = true;
    m_DragStart = wxPoint(event.GetX(), event.GetY());
}

void GfxCore::OnLButtonUp(wxMouseEvent& event)
{
    m_DraggingLeft = false;
}

void GfxCore::OnMButtonDown(wxMouseEvent& event)
{
    if (!m_CaverotMouse) {
        m_DraggingMiddle = true;
	m_DragStart = wxPoint(event.GetX(), event.GetY());
    }
}

void GfxCore::OnMButtonUp(wxMouseEvent& event)
{
    if (m_CaverotMouse) {
        if (m_FreeRotMode) {
	    m_Parent->SetStatusText("Disabled in free rotation mode.  Reset using Delete.");
	}
	else {
	    // plan/elevation toggle
	    if (m_TiltAngle == 0.0) {
	        Plan();
	    }
	    else if (m_TiltAngle == M_PI/2.0) {
	        Elevation();
	    }
	    else {
		// go to whichever of plan or elevation is nearer
	        if (m_TiltAngle > M_PI/4.0) {
		     Plan();
		}
		else {
		     Elevation();
		}
	    }
	}
    }
    else {
        m_DraggingMiddle = false;
    }
}

void GfxCore::OnRButtonDown(wxMouseEvent& event)
{
    m_DragStart = wxPoint(event.GetX(), event.GetY());
    m_DraggingRight = true;
}

void GfxCore::OnRButtonUp(wxMouseEvent& event)
{
    m_DraggingRight = false;
}

void GfxCore::HandleScaleRotate(bool control, wxPoint point)
{
    // Handle a mouse movement during scale/rotate mode.

    int dx = point.x - m_DragStart.x;
    int dy = point.y - m_DragStart.y;

    double pan_angle = M_PI * (double(dx) / 500.0);

    Quaternion q;
    double new_scale = m_Params.scale;
    if (control || m_FreeRotMode) {
        // free rotation starts when Control is down

        if (!m_FreeRotMode) {
	    m_Parent->SetStatusText("Free rotation mode.  Switch back using Delete.");
	    m_FreeRotMode = true;
	}

	double tilt_angle = M_PI * (double(dy) / 500.0);
	q.setFromEulerAngles(tilt_angle, 0.0, pan_angle);
    }
    else {
        // left/right => rotate, up/down => scale
        if (m_ReverseControls) {
	    pan_angle = -pan_angle;
	}
	q.setFromVectorAndAngle(Vector3(XToScreen(0.0, 0.0, 1.0),
					YToScreen(0.0, 0.0, 1.0),
					ZToScreen(0.0, 0.0, 1.0)), -pan_angle);

	m_PanAngle += pan_angle;
	if (m_PanAngle > M_PI*2.0) {
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

void GfxCore::TiltCave(double tilt_angle)
{
    // Tilt the cave by a given angle.

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
        int dx = point.x - m_DragStart.x;
	int dy = point.y - m_DragStart.y;

	TiltCave(M_PI * (double(dy) / 500.0));

	m_DragStart = point;
    }
    else {
        m_Parent->SetStatusText("Disabled in free rotation mode.  Reset using Delete.");
    }
}

void GfxCore::HandleTranslate(wxPoint point)
{
    // Handle a mouse movement during translation mode.

    int dx = point.x - m_DragStart.x;
    int dy = point.y - m_DragStart.y;

    // Find out how far the mouse motion takes us in cave coords.
    float x = float(dx / m_Params.scale);
    float z = float(-dy / m_Params.scale);
    Matrix4 inverse_rotation = m_Params.rotation.asInverseMatrix();
    float cx = float(inverse_rotation.get(0, 0)*x + inverse_rotation.get(0, 2)*z);
    float cy = float(inverse_rotation.get(1, 0)*x + inverse_rotation.get(1, 2)*z);
    float cz = float(inverse_rotation.get(2, 0)*x + inverse_rotation.get(2, 2)*z);
    
    // Update parameters and redraw.
    m_Params.translation.x += cx;
    m_Params.translation.y += cy;
    m_Params.translation.z += cz;

    if (!m_OverlappingNames) {
        //		m_LabelCacheNotRefreshd = true;//--fixme
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
	    HandleScaleRotate(event.ControlDown(), point);
	}
	else if (m_DraggingMiddle) {
	    HandleTilt(point);
	}
	else if (m_DraggingRight) {
	    HandleTranslate(point);
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

    if (m_DoneFirstShow) {
        m_DrawDC.SelectObject(wxNullBitmap);
	m_OffscreenBitmap.Create(m_XSize, m_YSize);
	m_DrawDC.SelectObject(m_OffscreenBitmap);
        RedrawOffscreen();
	Refresh(false);
    }
}

#if 0

void GfxCore::DisplayOverlappingNames() 
{
	m_OverlappingNames = !m_OverlappingNames;
	SetScale(m_Params.scale);
	m_RedrawOffscreen = true;
	Refresh(false);
}

void GfxCore::OnUpdateDisplayOverlappingNamesUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL && m_Names);
	cmd->SetCheck(m_OverlappingNames);
}

void GfxCore::ShowCrosses() 
{
	m_Crosses = !m_Crosses;
	m_RedrawOffscreen = true;
	m_ScaleCrossesOnly = true;
	SetScale(m_Params.scale);
	Refresh(false);
}

void GfxCore::ShowCrossesUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL);
	cmd->SetCheck(m_Crosses);
}

void GfxCore::ShowStationNames() 
{
	m_Names = !m_Names;
	SetScale(m_Params.scale);
	m_RedrawOffscreen = true;
	Refresh(false);
}

void GfxCore::ShowStationNamesUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL);
	cmd->SetCheck(m_Names);
}

void GfxCore::ShowSurveyLegs() 
{
	m_Legs = !m_Legs;
	m_RedrawOffscreen = true;
	Refresh(false);
}

void GfxCore::ShowSurveyLegsUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL);
	cmd->SetCheck(m_Legs);
}

void GfxCore::MoveEast() 
{
	m_Params.translation.x += m_Parent->GetXExtent() / 100.0f;
	m_RedrawOffscreen = true;
	SetScale(m_Params.scale);
	Refresh(false);
}

void GfxCore::MoveEastUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL);
}

void GfxCore::MoveNorth() 
{
	m_Params.translation.y += m_Parent->GetYExtent() / 100.0f;
	m_RedrawOffscreen = true;
	SetScale(m_Params.scale);
	Refresh(false);
}

void GfxCore::MoveNorthUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL);
}

void GfxCore::MoveSouth() 
{
	m_Params.translation.y -= m_Parent->GetYExtent() / 100.0f;
	m_RedrawOffscreen = true;
	SetScale(m_Params.scale);
	Refresh(false);
}

void GfxCore::MoveSouthUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL);
}

void GfxCore::MoveWest() 
{
	m_Params.translation.x -= m_Parent->GetXExtent() / 100.0f;
	m_RedrawOffscreen = true;
	SetScale(m_Params.scale);
	Refresh(false);
}

void GfxCore::MoveWestUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL);
}
#endif

void GfxCore::StartTimer()
{
    m_Timer.Start(50);
}

void GfxCore::StopTimer()
{
    m_Timer.Stop();
}

#if 0
void GfxCore::StartRotation() 
{
    StartTimer();
    m_Rotating = true;
}

void GfxCore::StartRotationUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL && !m_FreeRotMode && !m_Rotating);
}

void GfxCore::StopRotation() 
{
	if (!m_SwitchingToElevation && !m_SwitchingToPlan) {
		StopTimer();
	}

	m_Rotating = false;
}

void GfxCore::StopRotationUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL && !m_FreeRotMode && m_Rotating);
}

void GfxCore::OriginalCaverotMouse() 
{
	m_CaverotMouse = !m_CaverotMouse;
}

void GfxCore::OnUpdateOriginalCaverotMouseUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL && !m_FreeRotMode);
	cmd->SetCheck(m_CaverotMouse);
}

void GfxCore::ReverseControls() 
{
	m_ReverseControls = !m_ReverseControls;
}

void GfxCore::ReverseControlsUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL && !m_FreeRotMode);
	cmd->SetCheck(m_ReverseControls);
}

void GfxCore::ReverseDirectionOfRotation() 
{
	m_RotationStep = -m_RotationStep;
}

void GfxCore::ReverseDirectionOfRotationUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL && m_Rotating);
}

void GfxCore::SlowDown() 
{
	m_RotationStep /= 1.2f;
	if (m_RotationStep < M_PI/2000.0f) {
		m_RotationStep = (float) M_PI/2000.0f;
	}
}

void GfxCore::SlowDownUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL && m_Rotating);
}

void GfxCore::SpeedUp() 
{
	m_RotationStep *= 1.2f;
	if (m_RotationStep > M_PI/8.0f) {
		m_RotationStep = (float) M_PI/8.0f;
	}
}

void GfxCore::SpeedUpUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL && m_Rotating);
}

void GfxCore::StepOnceAnticlockwise() 
{
	TurnCave(M_PI / 18.0);
}

void GfxCore::StepOnceAnticlockwiseUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL && !m_FreeRotMode && !m_Rotating);
}

void GfxCore::StepOnceClockwise() 
{
	TurnCave(-M_PI / 18.0);
}

void GfxCore::StepOnceClockwiseUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL && !m_FreeRotMode && !m_Rotating);
}
#endif
void GfxCore::Defaults() 
{
    // Restore default scale, rotation and translation parameters.

    m_Params.rotation.setFromEulerAngles(0.0, 0.0, 0.0);
    m_RotationMatrix = m_Params.rotation.asMatrix();

    //    m_Parent->SetStatusText("Ready");

    m_Params.translation.x = 0.0;
    m_Params.translation.y = 0.0;
    m_Params.translation.z = 0.0;
    
    m_Params.display_shift.x = 0;
    m_Params.display_shift.y = 0;

    m_ScaleCrossesOnly = false;
    SetScale(m_InitialScale);

    StopTimer();
    m_FreeRotMode = false;
    m_TiltAngle = 0.0;
    m_PanAngle = 0.0;
    m_RotationStep = (float) M_PI / 180.0f;
    m_Rotating = false;
    m_SwitchingToPlan = false;
    m_SwitchingToElevation = false;

    m_RedrawOffscreen = true;
    Refresh(false);
}
#if 0
void GfxCore::DefaultsUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL);
}
#endif
void GfxCore::Elevation() 
{
    // Switch to elevation view.

    m_SwitchingToElevation = true;
    StartTimer();
}
#if 0
void GfxCore::ElevationUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL && !m_FreeRotMode && !m_SwitchingToPlan &&
		        !m_SwitchingToElevation);
}

void GfxCore::HigherViewpoint() 
{
	// Raise the viewpoint.

	TiltCave(M_PI / 18.0);
}

void GfxCore::HigherViewpointUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL && !m_FreeRotMode && m_TiltAngle < M_PI / 2.0);
}

void GfxCore::LowerViewpoint() 
{
	// Lower the viewpoint.

	TiltCave(-M_PI / 18.0);
}

void GfxCore::LowerViewpointUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL && !m_FreeRotMode && m_TiltAngle > -M_PI / 2.0);
}
#endif
void GfxCore::Plan() 
{
    // Switch to plan view.

    m_SwitchingToPlan = true;
    StartTimer();
}
#if 0
void GfxCore::PlanUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL && !m_FreeRotMode && !m_SwitchingToElevation &&
		        !m_SwitchingToPlan);
}

void GfxCore::ShiftDisplayDown() 
{
	m_Params.display_shift.y += DISPLAY_SHIFT;
	SetScale(m_Params.scale);
	m_RedrawOffscreen = true;
	Refresh(false);
}

void GfxCore::ShiftDisplayDownUpdate(CCmdUI* cmd)
{
	cmd->Enable(m_PlotData != NULL);
}

void GfxCore::ShiftDisplayLeft() 
{
	m_Params.display_shift.x -= DISPLAY_SHIFT;
	SetScale(m_Params.scale);
	m_RedrawOffscreen = true;
	Refresh(false);
}

void GfxCore::ShiftDisplayLeftUpdate(CCmdUI* cmd)
{
	cmd->Enable(m_PlotData != NULL);
}

void GfxCore::ShiftDisplayRight() 
{
	m_Params.display_shift.x += DISPLAY_SHIFT;
	SetScale(m_Params.scale);
	m_RedrawOffscreen = true;
	Refresh(false);
}

void GfxCore::ShiftDisplayRightUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL);
}

void GfxCore::ShiftDisplayUp() 
{
	m_Params.display_shift.y -= DISPLAY_SHIFT;
	SetScale(m_Params.scale);
	m_RedrawOffscreen = true;
	Refresh(false);
}

void GfxCore::ShiftDisplayUpUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL);
}

void GfxCore::ZoomIn() 
{
	// Increase the scale.

	m_Params.scale *= 1.06f;
	SetScale(m_Params.scale);
	m_RedrawOffscreen = true;
	Refresh(false);
}

void GfxCore::ZoomInUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL);
}

void GfxCore::ZoomOut() 
{
	// Decrease the scale.

	m_Params.scale /= 1.06f;
	SetScale(m_Params.scale);
	m_RedrawOffscreen = true;
	Refresh(false);
}

void GfxCore::ZoomOutUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL);
}
#endif
void GfxCore::OnTimer(wxTimerEvent& event)
{
    // Handle a timer event.

    // When rotating...
    if (m_Rotating) {
        TurnCave(m_RotationStep);
    }
    // When switching to plan view...
    else if (m_SwitchingToPlan) {
        if (m_TiltAngle == M_PI / 2.0) {
	    if (!m_Rotating) {
	        StopTimer();
	    }
	    m_SwitchingToPlan = false;
	}
	TiltCave(M_PI / 30.0);
    }
    // When switching to elevation view;;;
    else if (m_SwitchingToElevation) {
        if (m_TiltAngle == 0.0) {
	    if (!m_Rotating) {
	        StopTimer();
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
		    StopTimer();
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
		    StopTimer();
		}
		m_SwitchingToElevation = false;
	    }
	}
    }
}
#if 0
void GfxCore::ToggleScalebar() 
{
	m_Scalebar = !m_Scalebar;
	m_RedrawOffscreen = true;
	Refresh(false);
}

void GfxCore::ToggleScalebarUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL);
	cmd->SetCheck(m_Scalebar);
}

void GfxCore::ToggleDepthbar() 
{
	m_Depthbar = !m_Depthbar;
	m_RedrawOffscreen = true;
	Refresh(false);
}

void GfxCore::ToggleDepthbarUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL);
	cmd->SetCheck(m_Depthbar);
}

void GfxCore::ViewCompass() 
{
	m_Compass = !m_Compass;
	m_RedrawOffscreen = true;
	Refresh(false);
}

void GfxCore::ViewCompassUpdate(CCmdUI* cmd) 
{
	cmd->Enable(m_PlotData != NULL);
	cmd->SetCheck(m_Compass);
}
#endif
