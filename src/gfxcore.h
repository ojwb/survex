//
//  gfxcore.h
//
//  Core drawing code for Aven.
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

#ifndef gfxcore_h
#define gfxcore_h

#include <float.h>

#include "quaternion.h"
#include "wx.h"
#include <utility>
#include <list>

using std::list;

class MainFrm;
class PointInfo;

struct ColourTriple {
    // RGB triple: values are from 0-255 inclusive for each component.
    int r;
    int g;
    int b;
};

// Colours for drawing.
// These must be in the same order as the entries in COLOURS[] in gfxcore.cc.
enum AvenColour {
    col_BLACK = 0,
    col_GREY,
    col_LIGHT_GREY,
    col_LIGHT_GREY_2,
    col_DARK_GREY,
    col_WHITE,
    col_TURQUOISE,
    col_GREEN,
    col_INDICATOR_1,
    col_INDICATOR_2,
    col_YELLOW,
    col_RED,
    col_LAST // must be the last entry here
};

class Point {
    friend class GfxCore;
    Double x, y, z;
public:
    Point() {}
    Point(Double x_, Double y_, Double z_) : x(x_), y(y_), z(z_) {}
};

class LabelInfo;

extern const ColourTriple COLOURS[]; // defined in gfxcore.cc

class GfxCore : public wxWindow {
    struct params {
	Quaternion rotation;
	Double scale;
	struct {
	    Double x;
	    Double y;
	    Double z;
	} translation;
    } m_Params;

    enum LockFlags {
	lock_NONE = 0,
	lock_X = 1,
	lock_Y = 2,
	lock_Z = 4,
	lock_POINT = lock_X | lock_Y | lock_Z,
	lock_XZ = lock_X | lock_Z,
	lock_YZ = lock_Y | lock_Z,
	lock_XY = lock_X | lock_Y
    };

    struct PlotData {
	Point *vertices;
	Point *surface_vertices;
	int* num_segs;
	int* surface_num_segs;
    };

    struct {
	int offset_x;
	int offset_y;
	int width;
	int drag_start_offset_x;
	int drag_start_offset_y;
    } m_ScaleBar;

    double m_MaxExtent; // twice the maximum of the {x,y,z}-extents, in survey coordinates.
    char *m_LabelGrid;
    bool m_RotationOK;
    LockFlags m_Lock;
    Matrix4 m_RotationMatrix;
    bool m_DraggingLeft;
    bool m_DraggingMiddle;
    bool m_DraggingRight;
    MainFrm* m_Parent;
    wxPoint m_DragStart;
    wxPoint m_DragRealStart;
    wxPoint m_DragLast;
    wxBitmap* m_OffscreenBitmap;
    wxMemoryDC m_DrawDC;
    bool m_DoneFirstShow;
    bool m_RedrawOffscreen;
    int* m_Polylines;
    int* m_SurfacePolylines;
    int m_Bands;
    Double m_InitialScale;
    Double m_TiltAngle;
    Double m_PanAngle;
    bool m_Rotating;
    Double m_RotationStep;
    int m_SwitchingTo;
    bool m_ReverseControls;
    bool m_Crosses;
    bool m_Legs;
    bool m_Names;
    bool m_Scalebar;
    wxFont m_Font;
    bool m_Depthbar;
    bool m_OverlappingNames;
    bool m_Compass;
    bool m_Clino;
    int m_XSize;
    int m_YSize;
    int m_XCentre;
    int m_YCentre;
    PlotData* m_PlotData;
    bool m_InitialisePending;
    enum { drag_NONE, drag_MAIN, drag_COMPASS, drag_ELEV, drag_SCALE } m_LastDrag;
    bool m_MouseOutsideCompass;
    bool m_MouseOutsideElev;
    bool m_UndergroundLegs;
    bool m_SurfaceLegs;
    bool m_Surface;
    bool m_SurfaceDashed;
    bool m_SurfaceDepth;
    bool m_Entrances;
    bool m_FixedPts;
    bool m_ExportedPts;
    bool m_Grid;

    list<LabelInfo*> *m_PointGrid;
    bool m_HitTestGridValid;

    Point m_here, m_there;

    wxStopWatch timer;
    long drawtime;
    
    bool clipping;

    wxPen* m_Pens;
    wxBrush* m_Brushes;

    void SetColour(AvenColour col, bool background = false /* true => foreground */) {
	assert(col >= (AvenColour) 0 && col < col_LAST);
	if (background) {
	    assert(m_Brushes[col].Ok());
	    m_DrawDC.SetBrush(m_Brushes[col]);
	}
	else {
	    assert(m_Pens[col].Ok());
	    m_DrawDC.SetPen(m_Pens[col]);
	}
    }

    Double XToScreen(Double x, Double y, Double z) {
	return Double(x * m_RotationMatrix.get(0, 0) +
		      y * m_RotationMatrix.get(0, 1) +
		      z * m_RotationMatrix.get(0, 2));
    }

    Double YToScreen(Double x, Double y, Double z) {
	return Double(x * m_RotationMatrix.get(1, 0) +
		      y * m_RotationMatrix.get(1, 1) +
		      z * m_RotationMatrix.get(1, 2));
    }

    Double ZToScreen(Double x, Double y, Double z) {
	return Double(x * m_RotationMatrix.get(2, 0) +
		      y * m_RotationMatrix.get(2, 1) +
		      z * m_RotationMatrix.get(2, 2));
    }
    
    Double GridXToScreen(Double x, Double y, Double z);
    Double GridYToScreen(Double x, Double y, Double z);
    Double GridXToScreen(const Point &p) {
	return GridXToScreen(p.x, p.y, p.z);
    }
    Double GridYToScreen(const Point &p) {
	return GridYToScreen(p.x, p.y, p.z);
    }

    void CheckHitTestGrid(wxPoint& point, bool centre);

    wxCoord GetClinoOffset();
    wxPoint CompassPtToScreen(Double x, Double y, Double z);
    void DrawTick(wxCoord cx, wxCoord cy, int angle_cw);
    wxString FormatLength(Double, bool scalebar = true);

    void SetScale(Double scale);
    void SetScaleInitial(Double scale);
    void DrawBand(int num_polylines, const int *num_segs, const Point *vertices,
		  Double m_00, Double m_01, Double m_02,
		  Double m_20, Double m_21, Double m_22);
    void RedrawOffscreen();
    void TryToFreeArrays();
    void FirstShow();

    void DrawScalebar();
    void DrawDepthbar();
    void DrawCompass();
    void Draw2dIndicators();
    void DrawGrid();

    wxPoint IndicatorCompassToScreenPan(int angle);
    wxPoint IndicatorCompassToScreenElev(int angle);

    void HandleScaleRotate(bool, wxPoint);
    void HandleTilt(wxPoint);
    void HandleTranslate(wxPoint);

    void TranslateCave(int dx, int dy);
    void TiltCave(Double tilt_angle);
    void TurnCave(Double angle);
    void TurnCaveTo(Double angle);

    void DrawNames();
    void NattyDrawNames();
    void SimpleDrawNames();

    void Defaults();
    void DefaultParameters();

    void Repaint();

    void CreateHitTestGrid();

public:
    bool m_Degrees;
    bool m_Metric;

    GfxCore(MainFrm* parent, wxWindow* parent_window);
    ~GfxCore();

    void Initialise();
    void InitialiseOnNextResize() { m_InitialisePending = true; }
    void InitialiseTerrain();

    void ForceRefresh();

    void RefreshLine(const Point &a1, const Point &b1,
		     const Point &a2, const Point &b2);
 
    void SetHere();
    void SetHere(Double x, Double y, Double z);
    void SetThere();
    void SetThere(Double x, Double y, Double z);

    void CentreOn(Double x, Double y, Double z);

    void OnDefaults();
    void OnPlan();
    void OnElevation();
    void OnDisplayOverlappingNames();
    void OnShowCrosses();
    void OnShowStationNames();
    void OnShowSurveyLegs();
    void OnShowSurface();
    void OnShowSurfaceDepth();
    void OnShowSurfaceDashed();
    void OnMoveEast();
    void OnMoveNorth();
    void OnMoveSouth();
    void OnMoveWest();
    void OnStartRotation();
    void OnToggleRotation();
    void OnStopRotation();
    void OnReverseControls();
    void OnSlowDown(bool accel = false);
    void OnSpeedUp(bool accel = false);
    void OnStepOnceAnticlockwise(bool accel = false);
    void OnStepOnceClockwise(bool accel = false);
    void OnHigherViewpoint(bool accel = false);
    void OnLowerViewpoint(bool accel = false);
    void OnShiftDisplayDown(bool accel = false);
    void OnShiftDisplayLeft(bool accel = false);
    void OnShiftDisplayRight(bool accel = false);
    void OnShiftDisplayUp(bool accel = false);
    void OnZoomIn(bool accel = false);
    void OnZoomOut(bool accel = false);
    void OnToggleScalebar();
    void OnToggleDepthbar();
    void OnViewCompass();
    void OnViewClino();
    void OnViewGrid();
    void OnReverseDirectionOfRotation();
    void OnShowEntrances();
    void OnShowFixedPts();
    void OnShowExportedPts();
    void OnCancelDistLine();

    void OnPaint(wxPaintEvent&);
    void OnMouseMove(wxMouseEvent& event);
    void OnLButtonDown(wxMouseEvent& event);
    void OnLButtonUp(wxMouseEvent& event);
    void OnMButtonDown(wxMouseEvent& event);
    void OnMButtonUp(wxMouseEvent& event);
    void OnRButtonDown(wxMouseEvent& event);
    void OnRButtonUp(wxMouseEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnIdle(wxIdleEvent& event);
    bool Animate(wxIdleEvent *idle_event = NULL);

    void OnKeyPress(wxKeyEvent &e);

    void OnDisplayOverlappingNamesUpdate(wxUpdateUIEvent&);
    void OnShowCrossesUpdate(wxUpdateUIEvent&);
    void OnShowStationNamesUpdate(wxUpdateUIEvent&);
    void OnShowSurveyLegsUpdate(wxUpdateUIEvent&);
    void OnShowSurfaceUpdate(wxUpdateUIEvent&);
    void OnShowSurfaceDepthUpdate(wxUpdateUIEvent&);
    void OnShowSurfaceDashedUpdate(wxUpdateUIEvent&);
    void OnMoveEastUpdate(wxUpdateUIEvent&);
    void OnMoveNorthUpdate(wxUpdateUIEvent&);
    void OnMoveSouthUpdate(wxUpdateUIEvent&);
    void OnMoveWestUpdate(wxUpdateUIEvent&);
    void OnStartRotationUpdate(wxUpdateUIEvent&);
    void OnToggleRotationUpdate(wxUpdateUIEvent&);
    void OnStopRotationUpdate(wxUpdateUIEvent&);
    void OnReverseControlsUpdate(wxUpdateUIEvent&);
    void OnReverseDirectionOfRotationUpdate(wxUpdateUIEvent&);
    void OnSlowDownUpdate(wxUpdateUIEvent&);
    void OnSpeedUpUpdate(wxUpdateUIEvent&);
    void OnStepOnceAnticlockwiseUpdate(wxUpdateUIEvent&);
    void OnStepOnceClockwiseUpdate(wxUpdateUIEvent&);
    void OnDefaultsUpdate(wxUpdateUIEvent&);
    void OnElevationUpdate(wxUpdateUIEvent&);
    void OnHigherViewpointUpdate(wxUpdateUIEvent&);
    void OnLowerViewpointUpdate(wxUpdateUIEvent&);
    void OnPlanUpdate(wxUpdateUIEvent&);
    void OnShiftDisplayDownUpdate(wxUpdateUIEvent&);
    void OnShiftDisplayLeftUpdate(wxUpdateUIEvent&);
    void OnShiftDisplayRightUpdate(wxUpdateUIEvent&);
    void OnShiftDisplayUpUpdate(wxUpdateUIEvent&);
    void OnZoomInUpdate(wxUpdateUIEvent&);
    void OnZoomOutUpdate(wxUpdateUIEvent&);
    void OnToggleScalebarUpdate(wxUpdateUIEvent&);
    void OnToggleDepthbarUpdate(wxUpdateUIEvent&);
    void OnViewCompassUpdate(wxUpdateUIEvent&);
    void OnViewClinoUpdate(wxUpdateUIEvent&);
    void OnViewGridUpdate(wxUpdateUIEvent&);
    void OnShowEntrancesUpdate(wxUpdateUIEvent&);
    void OnShowExportedPtsUpdate(wxUpdateUIEvent&);
    void OnShowFixedPtsUpdate(wxUpdateUIEvent&);
    void OnIndicatorsUpdate(wxUpdateUIEvent&);
    void OnCancelDistLineUpdate(wxUpdateUIEvent&);

    void OnToggleMetric();
    void OnToggleMetricUpdate(wxUpdateUIEvent& cmd);
	
    void OnToggleDegrees();
    void OnToggleDegreesUpdate(wxUpdateUIEvent& cmd);

private:
    DECLARE_EVENT_TABLE()
};

#endif
