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
#include "guicontrol.h"
#include "wx.h"
#include <utility>
#include <list>
#ifdef AVENGL
#ifndef wxUSE_GLCANVAS
#define wxUSE_GLCANVAS
#endif
#include <wx/glcanvas.h>
#endif

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

#ifdef AVENGL
class GfxCore : public wxGLCanvas {
#else
class GfxCore : public wxWindow {
#endif
    struct params {
	Quaternion rotation;
	Double scale;
	struct {
	    Double x;
	    Double y;
	    Double z;
	} translation;
    } m_Params;

#ifdef AVENPRES
    struct PresData {
	struct {
	    Double x, y, z;
	} translation;
	Double scale;
	Double pan_angle;
	Double tilt_angle;
	bool solid_surface;
    };
#endif

#ifdef AVENGL
    struct {
	// OpenGL display lists.
	GLint survey; // all underground data
	GLint surface; // all surface data in uniform colour
	GLint surface_depth; // all surface data in depth bands
	GLint grid; // the grid
	GLint terrain; // surface terrain
	GLint flat_terrain; // flat surface terrain - used for drape effect
	GLint map; // map overlay
    } m_Lists;

    bool m_AntiAlias;
#endif

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

#ifdef AVENGL
    struct {
	// Viewing volume parameters: these are all negative!
	Double left;
	Double bottom;
	Double nearface;
    } m_Volume;

    struct {
	GLuint surface;
	GLuint map;
    } m_Textures;

    bool m_SolidSurface;

    Double floor_alt;
    bool terrain_rising;
#endif

#ifdef AVENPRES
    list<pair<PresData, Quaternion> > m_Presentation;
    list<pair<PresData, Quaternion> >::iterator m_PresIterator;
#endif
    GUIControl* m_Control;
    double m_MaxExtent; // twice the maximum of the {x,y,z}-extents, in survey coordinates.
    char *m_LabelGrid;
    bool m_RotationOK;
    LockFlags m_Lock;
    Matrix4 m_RotationMatrix;
    MainFrm* m_Parent;
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
#ifdef AVENGL
    bool m_TerrainLoaded;
#endif

#ifdef AVENPRES
    Double m_DoingPresStep;
    struct step_params {
	Double pan_angle;
	Double tilt_angle;
	Quaternion rotation;
	Double scale;
	struct {
	    Double x;
	    Double y;
	    Double z;
	} translation;
    };

    struct {
	step_params from;
	step_params to;
    } m_PresStep;
#endif

    Point m_here, m_there;

    wxStopWatch timer;
    long drawtime;
    
    bool clipping;

#ifndef AVENGL
    wxPen* m_Pens;
    wxBrush* m_Brushes;
#endif

    void SetColour(AvenColour col, bool background = false /* true => foreground */) {
	assert(col >= (AvenColour) 0 && col < col_LAST);
#ifdef AVENGL
	glColor3f(GLfloat(COLOURS[col].r) / 256.0,
		  GLfloat(COLOURS[col].g) / 256.0,
		  GLfloat(COLOURS[col].b) / 256.0);
#else
	if (background) {
	    assert(m_Brushes[col].Ok());
	    m_DrawDC.SetBrush(m_Brushes[col]);
	}
	else {
	    assert(m_Pens[col].Ok());
	    m_DrawDC.SetPen(m_Pens[col]);
	}
#endif
    }

#ifdef AVENGL
    void SetGLProjection();
    void SetModellingTransformation();
    void ClearBackgroundAndBuffers();
    void SetGLAntiAliasing();
    void CheckGLError(const wxString& where);
    void SetTerrainColour(Double);
    void LoadTexture(const wxString& file, GLuint* texture);
    void RenderMap();
    void SetSolidSurface(bool);
    void RenderTerrain(Double floor_alt);
#endif

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

    wxCoord GetClinoOffset();
    wxPoint CompassPtToScreen(Double x, Double y, Double z);
    void DrawTick(wxCoord cx, wxCoord cy, int angle_cw);
    wxString FormatLength(Double, bool scalebar = true);

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

    void DrawNames();
    void NattyDrawNames();
    void SimpleDrawNames();

    void DefaultParameters();

    void Repaint();

    void CreateHitTestGrid();

    wxCoord GetCompassXPosition();
    wxCoord GetClinoXPosition();
    wxCoord GetIndicatorYPosition();
    wxCoord GetIndicatorRadius();

    void ToggleFlag(bool* flag);

public:
    bool m_Degrees;
    bool m_Metric;

    GfxCore(MainFrm* parent, wxWindow* parent_window, GUIControl* control);
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

    void TranslateCave(int dx, int dy);
    void TiltCave(Double tilt_angle);
    void TurnCave(Double angle);
    void TurnCaveTo(Double angle);

#ifdef AVENPRES
    void RecordPres(FILE* fp);
    void LoadPres(FILE* fp);
    void PresGo();
    void PresGoBack();
    void RestartPres();

    bool AtStartOfPres();
    bool AtEndOfPres();

    void PresGoto(PresData& d, Quaternion& q);
#endif

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

#ifdef AVENGL
    void OnAntiAlias();
    void OnSolidSurface();
#endif
    bool Animate(wxIdleEvent *idle_event = NULL);
#ifdef AVENGL
    void OnAntiAliasUpdate(wxUpdateUIEvent&);
    void OnSolidSurfaceUpdate(wxUpdateUIEvent&);
#endif

    void SetCoords(wxPoint);

    bool ShowingCompass();
    bool ShowingClino();
    
    bool PointWithinCompass(wxPoint point);
    bool PointWithinClino(wxPoint point);
    bool PointWithinScaleBar(wxPoint point);
    
    void SetCompassFromPoint(wxPoint point);
    void SetClinoFromPoint(wxPoint point);
    void SetScaleBarFromOffset(wxCoord dx);
    
    void RedrawIndicators();
    
    void StartRotation();
    void ToggleRotation();
    void StopRotation();
    bool CanRotate();
    void ReverseRotation();
    void RotateSlower(bool accel);
    void RotateFaster(bool accel);
    
    void SwitchToElevation();
    void SwitchToPlan();
    bool ChangingOrientation();
    
    bool ShowingPlan();
    bool ShowingElevation();
    bool ShowingMeasuringLine();

    bool CanRaiseViewpoint();
    bool CanLowerViewpoint();

    LockFlags GetLock() { return m_Lock; }
    bool IsRotating() { return m_Rotating; }
    bool HasData() { return m_PlotData != NULL; }

    double GetScale() { return m_Params.scale; }
    void SetScale(Double scale);
    
    bool ShowingStationNames() { return m_Names; }
    bool ShowingOverlappingNames() { return m_OverlappingNames; }
    bool ShowingCrosses() { return m_Crosses; }

    bool HasUndergroundLegs() { return m_UndergroundLegs; }
    bool HasSurfaceLegs() { return m_SurfaceLegs; }

    bool ShowingUndergroundLegs() { return m_Legs; }
    bool ShowingSurfaceLegs() { return m_Surface; }
    bool ShowingSurfaceDepth() { return m_SurfaceDepth; }
    bool ShowingSurfaceDashed() { return m_SurfaceDashed; }

    bool ShowingDepthBar() { return m_Depthbar; }
    bool ShowingScaleBar() { return m_Scalebar; }

    bool ShowingEntrances() { return m_Entrances; }
    bool ShowingFixedPts() { return m_FixedPts; }
    bool ShowingExportedPts() { return m_ExportedPts; }

    int GetNumEntrances();
    int GetNumFixedPts();
    int GetNumExportedPts();

    void ToggleUndergroundLegs() { ToggleFlag(&m_Legs); }
    void ToggleSurfaceLegs() { ToggleFlag(&m_Surface); }
    void ToggleCompass() { ToggleFlag(&m_Compass); }
    void ToggleClino() { ToggleFlag(&m_Clino); }
    void ToggleScaleBar() { ToggleFlag(&m_Scalebar); }
    void ToggleEntrances() { ToggleFlag(&m_Entrances); }
    void ToggleFixedPts() { ToggleFlag(&m_FixedPts); }
    void ToggleExportedPts() { ToggleFlag(&m_ExportedPts); }
    void ToggleGrid() { ToggleFlag(&m_Grid); }
    void ToggleCrosses() { ToggleFlag(&m_Crosses); }
    void ToggleStationNames() { ToggleFlag(&m_Names); }
    void ToggleOverlappingNames() { ToggleFlag(&m_OverlappingNames); }
    void ToggleDepthBar() { ToggleFlag(&m_Depthbar); }
    void ToggleSurfaceDepth() { ToggleFlag(&m_SurfaceDepth); }
    void ToggleSurfaceDashed() { ToggleFlag(&m_SurfaceDashed); }
    void ToggleMetric() { ToggleFlag(&m_Metric); }
    void ToggleDegrees() { ToggleFlag(&m_Degrees); }

    bool GetMetric() { return m_Metric; }
    bool GetDegrees() { return m_Degrees; }
    
    void CheckHitTestGrid(wxPoint& point, bool centre);

    void ClearTreeSelection();

    void Defaults();

private:
    DECLARE_EVENT_TABLE()
};

#endif

