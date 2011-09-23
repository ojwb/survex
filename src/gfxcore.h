//
//  gfxcore.h
//
//  Core drawing code for Aven.
//
//  Copyright (C) 2000-2001,2002,2005 Mark R. Shinwell.
//  Copyright (C) 2001-2004,2005,2006,2007,2010 Olly Betts
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

#ifndef gfxcore_h
#define gfxcore_h

#include <float.h>

#include "img.h"

#include "guicontrol.h"
#include "vector3.h"
#include "wx.h"
#include "gla.h"

#include <list>
#include <utility>
#include <vector>

using namespace std;

class MainFrm;
class traverse;

extern const int NUM_DEPTH_COLOURS;

// Mac OS X headers pollute the global namespace with generic names like
// "class Point", which clashes with our "class Point".  So for __WXMAC__
// put our class in a namespace and define Point as a macro.
#ifdef __WXMAC__
namespace svx {
#endif

class Point : public Vector3 {
  public:
    Point() {}
    Point(const Vector3 & v) : Vector3(v) { }
    Point(const img_point & pt) : Vector3(pt.x, pt.y, pt.z) { }
    Double GetX() const { return x; }
    Double GetY() const { return y; }
    Double GetZ() const { return z; }
    void Invalidate() { x = DBL_MAX; }
    bool IsValid() const { return x != DBL_MAX; }
};

#ifdef __WXMAC__
}
#define Point svx::Point
#endif

class XSect;
class LabelInfo;
class PointInfo;
class MovieMaker;

class PresentationMark : public Point {
  public:
    Double angle, tilt_angle;
    Double scale;
    Double time;
    PresentationMark() : Point(), angle(0), tilt_angle(0), scale(0), time(0)
	{ }
    PresentationMark(const Vector3 & v, Double angle_, Double tilt_angle_,
		     Double scale_, Double time_ = 0)
	: Point(v), angle(angle_), tilt_angle(tilt_angle_), scale(scale_),
	  time(time_)
	{ }
    bool is_valid() const { return scale > 0; }
};

enum {
    COLOUR_BY_NONE,
    COLOUR_BY_DEPTH,
    COLOUR_BY_DATE,
    COLOUR_BY_ERROR
};

enum {
    UPDATE_NONE,
    UPDATE_BLOBS,
    UPDATE_BLOBS_AND_CROSSES
};

class GfxCore : public GLACanvas {
    Double m_Scale;
    int m_ScaleBarWidth;

    typedef enum {
	LIST_COMPASS,
	LIST_CLINO,
	LIST_CLINO_BACK,
	LIST_DEPTHBAR,
	LIST_DATEBAR,
	LIST_ERRORBAR,
	LIST_UNDERGROUND_LEGS,
	LIST_TUBES,
	LIST_SURFACE_LEGS,
	LIST_BLOBS,
	LIST_CROSSES,
	LIST_GRID,
	LIST_SHADOW
    } drawing_list;

public:
    typedef enum {
	CURSOR_DEFAULT,
	CURSOR_POINTING_HAND,
	CURSOR_DRAGGING_HAND,
	CURSOR_HORIZONTAL_RESIZE,
	CURSOR_ROTATE_HORIZONTALLY,
	CURSOR_ROTATE_VERTICALLY,
	CURSOR_ROTATE_EITHER_WAY,
	CURSOR_ZOOM,
	CURSOR_ZOOM_ROTATE
    } cursor;

private:
    GUIControl* m_Control;
    char* m_LabelGrid;
    MainFrm* m_Parent;
    bool m_DoneFirstShow;
    Double m_TiltAngle;
    Double m_PanAngle;
    bool m_Rotating;
    Double m_RotationStep;
    int m_SwitchingTo;
    bool m_Crosses;
    bool m_Legs;
    bool m_Names;
    bool m_Scalebar;
    bool m_Depthbar;
    bool m_OverlappingNames;
    bool m_Compass;
    bool m_Clino;
    bool m_Tubes;
    int m_XSize;
    int m_YSize;
    int m_ColourBy;

    bool m_HaveData;
    bool m_MouseOutsideCompass;
    bool m_MouseOutsideElev;
    bool m_Surface;
    bool m_Entrances;
    bool m_FixedPts;
    bool m_ExportedPts;
    bool m_Grid;
    bool m_BoundingBox;

    bool m_Degrees;
    bool m_Metric;

    list<LabelInfo*> *m_PointGrid;
    bool m_HitTestGridValid;

    bool m_here_is_temporary;
    Point m_here, m_there;

    wxStopWatch timer;
    long drawtime;

    GLAPen * m_Pens;

#define PLAYING 1
    int presentation_mode; // for now, 0 => off, PLAYING => continuous play
    bool pres_reverse;
    double pres_speed;
    PresentationMark next_mark;
    double next_mark_time;
    double this_mark_total;

    MovieMaker * movie;

    cursor current_cursor;

    void PlaceVertexWithColour(const Vector3 &v, Double factor = 1.0);
    void PlaceVertexWithDepthColour(const Vector3 & v, Double factor = 1.0);

    void SetColourFromDate(int date, Double factor);
    void SetColourFromError(double E, Double factor);

    int GetClinoOffset() const;
    void DrawTick(int angle_cw);
    void DrawArrow(gla_colour col1, gla_colour col2);
    wxString FormatLength(Double, bool scalebar = true);

    void SkinPassage(const vector<XSect> & centreline);

    virtual void GenerateList(unsigned int l);
    void GenerateDisplayList();
    void GenerateDisplayListTubes();
    void GenerateDisplayListSurface();
    void GenerateDisplayListShadow();
    void GenerateBlobsDisplayList();

    void DrawIndicators();

    void TryToFreeArrays();
    void FirstShow();

    void DrawScalebar();
    void DrawDepthbar();
    void DrawDatebar();
    void DrawErrorbar();
    void DrawCompass();
    void DrawClino();
    void DrawClinoBack();
    void Draw2dIndicators();
    void DrawGrid();

    void NattyDrawNames();
    void SimpleDrawNames();

    void DefaultParameters();

    void Repaint();

    void CreateHitTestGrid();

    int GetCompassXPosition() const;
    int GetClinoXPosition() const;
    int GetIndicatorYPosition() const;
    int GetIndicatorRadius() const;

    void ToggleFlag(bool* flag, int update = UPDATE_NONE);

    GLAPen& GetPen(int band) const {
	assert(band >= 0 && band < NUM_DEPTH_COLOURS);
	return m_Pens[band];
    }

    GLAPen& GetSurfacePen() const { return m_Pens[NUM_DEPTH_COLOURS]; }

    int GetNumDepthBands() const { return NUM_DEPTH_COLOURS; }

    void DrawShadowedBoundingBox();
    void DrawBoundingBox();

public:
    GfxCore(MainFrm* parent, wxWindow* parent_window, GUIControl* control);
    ~GfxCore();

    void Initialise(bool same_file);
    void InitialiseTerrain();

    void UpdateBlobs();
    void ForceRefresh();

    void RefreshLine(const Point& a, const Point& b, const Point& c);

    void SetHere();
    void SetHere(const Point &p);
    void SetThere();
    void SetThere(const Point &p);

    void CentreOn(const Point &p);

    void TranslateCave(int dx, int dy);
    void TiltCave(Double tilt_angle);
    void TurnCave(Double angle);
    void TurnCaveTo(Double angle);

    void OnPaint(wxPaintEvent&);
    void OnSize(wxSizeEvent& event);
    void OnIdle(wxIdleEvent& event);

    void OnMouseMove(wxMouseEvent& event) { m_Control->OnMouseMove(event); }
    void OnLeaveWindow(wxMouseEvent& event);

    void OnLButtonDown(wxMouseEvent& event) { SetFocus(); m_Control->OnLButtonDown(event); }
    void OnLButtonUp(wxMouseEvent& event) { m_Control->OnLButtonUp(event); }
    void OnMButtonDown(wxMouseEvent& event) { SetFocus(); m_Control->OnMButtonDown(event); }
    void OnMButtonUp(wxMouseEvent& event) { m_Control->OnMButtonUp(event); }
    void OnRButtonDown(wxMouseEvent& event) { SetFocus(); m_Control->OnRButtonDown(event); }
    void OnRButtonUp(wxMouseEvent& event) { m_Control->OnRButtonUp(event); }
    void OnMouseWheel(wxMouseEvent& event) { SetFocus(); m_Control->OnMouseWheel(event); }
    void OnKeyPress(wxKeyEvent &event) { m_Control->OnKeyPress(event); }

    bool Animate();
    bool Animating() const {
	return m_Rotating || m_SwitchingTo || presentation_mode != 0;
    }

    void ClearCoords();
    void SetCoords(wxPoint);

    // Determine whether the compass is currently shown.
    bool ShowingCompass() const { return m_Compass; }
    // Determine whether the clino is currently shown.
    bool ShowingClino() const { return m_Clino; }

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
    bool IsExtendedElevation() const;
    void ReverseRotation();
    void RotateSlower(bool accel);
    void RotateFaster(bool accel);

    void SwitchToElevation();
    void SwitchToPlan();

    void SetViewTo(Double xmin, Double xmax, Double ymin, Double ymax, Double zmin, Double zmax);

    bool ShowingPlan() const;
    bool ShowingElevation() const;
    bool ShowingMeasuringLine() const;
    bool HereIsReal() const { return m_here.IsValid() && !m_here_is_temporary; }

    bool CanRaiseViewpoint() const;
    bool CanLowerViewpoint() const;

    bool IsRotating() const { return m_Rotating; }
    bool HasData() const { return m_DoneFirstShow && m_HaveData; }
    bool HasDepth() const;
    bool HasErrorInformation() const;
    bool HasDateInformation() const;

    double GetScale() const { return m_Scale; }
    void SetScale(Double scale);

    bool ShowingStationNames() const { return m_Names; }
    bool ShowingOverlappingNames() const { return m_OverlappingNames; }
    bool ShowingCrosses() const { return m_Crosses; }
    bool ShowingGrid() const { return m_Grid; }

    int ColouringBy() const { return m_ColourBy; }

    bool HasUndergroundLegs() const;
    bool HasSurfaceLegs() const;
    bool HasTubes() const;

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

    void ToggleUndergroundLegs() {
	ToggleFlag(&m_Legs, UPDATE_BLOBS_AND_CROSSES);
    }
    void ToggleSurfaceLegs() {
	ToggleFlag(&m_Surface, UPDATE_BLOBS_AND_CROSSES);
    }
    void ToggleCompass() { ToggleFlag(&m_Compass); }
    void ToggleClino() { ToggleFlag(&m_Clino); }
    void ToggleScaleBar() { ToggleFlag(&m_Scalebar); }
    void ToggleEntrances() { ToggleFlag(&m_Entrances, UPDATE_BLOBS); }
    void ToggleFixedPts() { ToggleFlag(&m_FixedPts, UPDATE_BLOBS); }
    void ToggleExportedPts() { ToggleFlag(&m_ExportedPts, UPDATE_BLOBS); }
    void ToggleGrid() { ToggleFlag(&m_Grid); }
    void ToggleCrosses() { ToggleFlag(&m_Crosses); }
    void ToggleStationNames() { ToggleFlag(&m_Names); }
    void ToggleOverlappingNames() { ToggleFlag(&m_OverlappingNames); }
    void ToggleDepthBar() { ToggleFlag(&m_Depthbar); }
    void ToggleMetric() { ToggleFlag(&m_Metric); InvalidateList(LIST_DEPTHBAR); }
    void ToggleDegrees() { ToggleFlag(&m_Degrees); }
    void ToggleTubes() { ToggleFlag(&m_Tubes); }
    void TogglePerspective() { GLACanvas::TogglePerspective(); ForceRefresh(); }
    void ToggleSmoothShading();
    bool DisplayingBoundingBox() const { return m_BoundingBox; }
    void ToggleBoundingBox() { ToggleFlag(&m_BoundingBox); }

    bool GetMetric() const { return m_Metric; }
    bool GetDegrees() const { return m_Degrees; }
    bool GetTubes() const { return m_Tubes; }

    bool CheckHitTestGrid(const wxPoint& point, bool centre);

    void ClearTreeSelection();

    void Defaults();

    void FullScreenMode();

    bool IsFullScreen() const;

    void DragFinished();

    void SplitLineAcrossBands(int band, int band2,
			      const Vector3 &p, const Vector3 &q,
			      Double factor = 1.0);
    int GetDepthColour(Double z) const;
    Double GetDepthBoundaryBetweenBands(int a, int b) const;
    void AddPolyline(const traverse & centreline);
    void AddPolylineDepth(const traverse & centreline);
    void AddPolylineDate(const traverse & centreline);
    void AddPolylineError(const traverse & centreline);
    void AddQuadrilateral(const Vector3 &a, const Vector3 &b,
			  const Vector3 &c, const Vector3 &d);
    void AddPolylineShadow(const traverse & centreline);
    void AddQuadrilateralDepth(const Vector3 &a, const Vector3 &b,
			       const Vector3 &c, const Vector3 &d);
    void AddQuadrilateralDate(const Vector3 &a, const Vector3 &b,
			      const Vector3 &c, const Vector3 &d);
    void AddQuadrilateralError(const Vector3 &a, const Vector3 &b,
			       const Vector3 &c, const Vector3 &d);
    void MoveViewer(double forward, double up, double right);

    void (GfxCore::* AddQuad)(const Vector3 &a, const Vector3 &b,
                              const Vector3 &c, const Vector3 &d);
    void (GfxCore::* AddPoly)(const traverse & centreline);

    PresentationMark GetView() const;
    void SetView(const PresentationMark & p);
    void PlayPres(double speed, bool change_speed = true);
    int GetPresentationMode() const { return presentation_mode; }
    double GetPresentationSpeed() const { return presentation_mode ? pres_speed : 0; }

    void SetColourBy(int colour_by);
    bool ExportMovie(const wxString & fnm);
    void OnPrint(const wxString &filename, const wxString &title,
		 const wxString &datestamp);
    void OnExport(const wxString &filename, const wxString &title);
    void SetCursor(GfxCore::cursor new_cursor);
    bool MeasuringLineActive() const;

    bool HandleRClick(wxPoint point);

private:
    DECLARE_EVENT_TABLE()
};

#endif
