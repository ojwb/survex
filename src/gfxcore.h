//
//  gfxcore.h
//
//  Core drawing code for Aven.
//
//  Copyright (C) 2000-2001 Mark R. Shinwell.
//  Copyright (C) 2001-2003 Olly Betts
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
#include "guicontrol.h"
#include "wx.h"
#include "gla.h"
#include <utility>
#include <list>

using std::list;

class MainFrm;

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

class GfxCore : public GLACanvas {
    struct params {
	Quaternion rotation;
	Double scale;
	struct {
	    Double x;
	    Double y;
	    Double z;
	} translation;
    } m_Params;

    struct {
	int offset_x;
	int offset_y;
	int width;
	int drag_start_offset_x;
	int drag_start_offset_y;
    } m_ScaleBar;

    struct {
        glaList underground_legs;
        glaList tubes;
        glaList surface_legs;
        glaList names;
        glaList indicators;
        glaList blobs;
    } m_Lists;

    GUIControl* m_Control;
    double m_MaxExtent; // twice the maximum of the {x,y,z}-extents, in survey coordinates.
    char* m_LabelGrid;
    bool m_RotationOK;
    LockFlags m_Lock;
    MainFrm* m_Parent;
    bool m_DoneFirstShow;
    bool m_RedrawOffscreen;
    int m_Polylines;
    int m_SurfacePolylines;
    int m_Bands;
    Double m_InitialScale;
    Double m_TiltAngle;
    Double m_PanAngle;
    bool m_Rotating;
    Double m_RotationStep;
    int m_SwitchingTo;
    bool m_Crosses;
    bool m_Legs;
    bool m_Names;
    bool m_Scalebar;
    wxFont m_Font;
    bool m_Depthbar;
    bool m_OverlappingNames;
    bool m_Compass;
    bool m_Clino;
    bool m_Tubes;
    int m_XSize;
    int m_YSize;
    int m_XCentre;
    int m_YCentre;

    bool m_HaveData;
    bool m_MouseOutsideCompass;
    bool m_MouseOutsideElev;
    bool m_Surface;
    bool m_Entrances;
    bool m_FixedPts;
    bool m_ExportedPts;
    bool m_Grid;

    bool m_Degrees;
    bool m_Metric;

    list<LabelInfo*> *m_PointGrid;
    bool m_HitTestGridValid;

    Point m_here, m_there;

    wxStopWatch timer;
    long drawtime;
    
    bool clipping;

    GLAPen* m_Pens;

    void SetAvenColour(AvenColour col, bool background = false /* true => foreground */) {
	background = background; // FIXME: suppress warning, but sort out why we're passed a useless parameter...
	assert(col >= (AvenColour) 0 && col < col_LAST);
	SetColour(m_Pens[col]); // FIXME background stuff
    }

    void UpdateNames();
    void UpdateQuaternion();
    void UpdateIndicators();
    void UpdateBlobs();
    
    void SetColourFromHeight(Double z, Double factor);
    void PlaceVertexWithColour(Double x, Double y, Double z,
			       Double factor = 1.0);
   
    Double GridXToScreen(Double x, Double y, Double z) const;
    Double GridYToScreen(Double x, Double y, Double z) const;
    Double GridXToScreen(const Point &p) const {
	return GridXToScreen(p.x, p.y, p.z);
    }
    Double GridYToScreen(const Point &p) const {
	return GridYToScreen(p.x, p.y, p.z);
    }

    glaCoord GetClinoOffset() const;
    wxPoint CompassPtToScreen(Double x, Double y, Double z) const;
    void DrawTick(wxCoord cx, wxCoord cy, int angle_cw);
    wxString FormatLength(Double, bool scalebar = true);

    void DrawPolylines(bool tubes, bool surface);

    void GenerateDisplayList();
    void GenerateDisplayListTubes();
    void GenerateDisplayListSurface();
    void GenerateBlobsDisplayList();
    void GenerateIndicatorDisplayList();

    void TryToFreeArrays();
    void FirstShow();

    void DrawScalebar();
    void DrawDepthbar();
    void DrawCompass();
    void Draw2dIndicators();
    void DrawGrid();

    GLAPoint IndicatorCompassToScreenPan(int angle) const;
    GLAPoint IndicatorCompassToScreenElev(int angle) const;

    void DrawNames();
    void NattyDrawNames();
    void SimpleDrawNames();

    void DefaultParameters();

    void Repaint();

    void CreateHitTestGrid();

    wxCoord GetCompassXPosition() const;
    glaCoord GetClinoXPosition() const;
    wxCoord GetIndicatorYPosition() const;
    wxCoord GetIndicatorRadius() const;

    void ToggleFlag(bool* flag, bool refresh = true);

public:
    GfxCore(MainFrm* parent, wxWindow* parent_window, GUIControl* control);
    ~GfxCore();

    void Initialise();
    void InitialiseTerrain();

    void ForceRefresh();

    void RefreshLine(const Point& a1, const Point& b1, const Point& a2, const Point& b2);
 
    void SetHere();
    void SetHere(Double x, Double y, Double z);
    void SetThere();
    void SetThere(Double x, Double y, Double z);

    void CentreOn(Double x, Double y, Double z);

    void TranslateCave(int dx, int dy);
    void TiltCave(Double tilt_angle);
    void TurnCave(Double angle);
    void TurnCaveTo(Double angle);

    void OnPaint(wxPaintEvent&);
    void OnSize(wxSizeEvent& event);
    void OnIdle(wxIdleEvent &event);

    void OnMouseMove(wxMouseEvent& event) { m_Control->OnMouseMove(event); }
    void OnLButtonDown(wxMouseEvent& event) { m_Control->OnLButtonDown(event); }
    void OnLButtonUp(wxMouseEvent& event) { m_Control->OnLButtonUp(event); }
    void OnMButtonDown(wxMouseEvent& event) { m_Control->OnMButtonDown(event); }
    void OnMButtonUp(wxMouseEvent& event) { m_Control->OnMButtonUp(event); }
    void OnRButtonDown(wxMouseEvent& event) { m_Control->OnRButtonDown(event); }
    void OnRButtonUp(wxMouseEvent& event) { m_Control->OnRButtonUp(event); }
    void OnKeyPress(wxKeyEvent &event) { m_Control->OnKeyPress(event); }

    bool Animate(wxIdleEvent* idle_event = NULL);

    void SetCoords(wxPoint);

    bool ShowingCompass() const;
    bool ShowingClino() const;
    
    bool PointWithinCompass(wxPoint point) const;
    bool PointWithinClino(wxPoint point) const;
    bool PointWithinScaleBar(wxPoint point) const;
    
    void SetCompassFromPoint(wxPoint point);
    void SetClinoFromPoint(wxPoint point);
    void SetScaleBarFromOffset(wxCoord dx);
    
    void RedrawIndicators();
    
    void StartRotation();
    void ToggleRotation();
    void StopRotation();
    bool CanRotate() const;
    void ReverseRotation();
    void RotateSlower(bool accel);
    void RotateFaster(bool accel);
    
    void SwitchToElevation();
    void SwitchToPlan();
    bool ChangingOrientation() const;
    
    bool ShowingPlan() const;
    bool ShowingElevation() const;
    bool ShowingMeasuringLine() const;

    bool CanRaiseViewpoint() const;
    bool CanLowerViewpoint() const;

    LockFlags GetLock() const { return m_Lock; }
    bool IsRotating() const { return m_Rotating; }
    bool HasData() const { return m_HaveData; }

    double GetScale() const { return m_Params.scale; }
    void SetScale(Double scale);
    
    bool ShowingStationNames() const { return m_Names; }
    bool ShowingOverlappingNames() const { return m_OverlappingNames; }
    bool ShowingCrosses() const { return m_Crosses; }

    bool HasUndergroundLegs() const { return true; /* FIXME */ }
    bool HasSurfaceLegs() const { return true; /* FIXME */ }

    bool ShowingUndergroundLegs() const { return m_Legs; }
    bool ShowingSurfaceLegs() const { return m_Surface; }

    bool ShowingDepthBar() const { return m_Depthbar; }
    bool ShowingScaleBar() const { return m_Scalebar; }

    bool ShowingEntrances() const { return m_Entrances; }
    bool ShowingFixedPts() const { return m_FixedPts; }
    bool ShowingExportedPts() const { return m_ExportedPts; }

    int GetNumEntrances() const;
    int GetNumFixedPts() const;
    int GetNumExportedPts() const;

    void ToggleUndergroundLegs() { ToggleFlag(&m_Legs); }
    void ToggleSurfaceLegs() { ToggleFlag(&m_Surface); }
    void ToggleCompass() { ToggleFlag(&m_Compass, false); UpdateIndicators(); ForceRefresh(); }
    void ToggleClino() { ToggleFlag(&m_Clino, false); UpdateIndicators(); ForceRefresh(); }
    void ToggleScaleBar() { ToggleFlag(&m_Scalebar, false); UpdateIndicators(); ForceRefresh(); }
    void ToggleEntrances() { ToggleFlag(&m_Entrances); }
    void ToggleFixedPts() { ToggleFlag(&m_FixedPts); }
    void ToggleExportedPts() { ToggleFlag(&m_ExportedPts); }
    void ToggleGrid() { ToggleFlag(&m_Grid); }
    void ToggleCrosses() { ToggleFlag(&m_Crosses); }
    void ToggleStationNames() { ToggleFlag(&m_Names, false); UpdateNames(); ForceRefresh(); }
    void ToggleOverlappingNames() { ToggleFlag(&m_OverlappingNames); }
    void ToggleDepthBar() { ToggleFlag(&m_Depthbar, false); UpdateIndicators(); ForceRefresh(); }
    void ToggleMetric() { ToggleFlag(&m_Metric, false); UpdateIndicators(); ForceRefresh(); }
    void ToggleDegrees() { ToggleFlag(&m_Degrees, false); UpdateIndicators(); ForceRefresh(); }
    void ToggleTubes() { ToggleFlag(&m_Tubes, false); ForceRefresh(); }

    bool GetMetric() const { return m_Metric; }
    bool GetDegrees() const { return m_Degrees; }
    bool GetTubes() const { return m_Tubes; }
    
    bool CheckHitTestGrid(wxPoint& point, bool centre);

    void ClearTreeSelection();

    void Defaults();

    void FullScreenMode();

    bool IsFullScreen() const;

    void DragFinished();

    void SplitLineAcrossBands(int band, int band2,
	    		      const Vector3 &p, const Vector3 &p2,
			      Double factor = 1.0);
    int GetDepthColour(Double z) const;
    Double GetDepthBoundaryBetweenBands(int a, int b) const;
    void AddQuadrilateral(const Vector3 &a, const Vector3 &b, 
			  const Vector3 &c, const Vector3 &d);
private:
    DECLARE_EVENT_TABLE()
};

#endif

