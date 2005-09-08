//
//  gfxcore.h
//
//  Core drawing code for Aven.
//
//  Copyright (C) 2000-2001,2002 Mark R. Shinwell.
//  Copyright (C) 2001-2004,2005 Olly Betts
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

#include <list>
#include <utility>
#include <vector>

using namespace std;

class MainFrm;

extern const int NUM_DEPTH_COLOURS;

class Point {
    friend class GfxCore;
    Double x, y, z;
public:
    Point() {}
    Point(Double x_, Double y_, Double z_) : x(x_), y(y_), z(z_) {}
};

class PointInfo;
class XSect;
class LabelInfo;
class MovieMaker;

class PresentationMark {
    public:
    Double x, y, z;
    Double angle, tilt_angle;
    Double scale;
    Double time;
    PresentationMark()
	: x(0), y(0), z(0), angle(0), tilt_angle(0), scale(0), time(0)
	{ }
    PresentationMark(Double x_, Double y_, Double z_, Double angle_,
		     Double tilt_angle_, Double scale_, Double time_ = 0)
	: x(x_), y(y_), z(z_), angle(angle_), tilt_angle(tilt_angle_),
	  scale(scale_), time(time_)
	{ }
    bool is_valid() const { return scale > 0; }
};

#define COLOUR_BY_NONE 0
#define COLOUR_BY_DEPTH 1
#define COLOUR_BY_DATE 2
#define COLOUR_BY_ERROR 3

#define UPDATE_NONE 0
#define UPDATE_BLOBS 1

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

class GfxCore : public GLACanvas {
    struct params {
	Double scale;
    } m_Params;

    int m_ScaleBarWidth;

    struct {
        glaList underground_legs;
        glaList tubes;
        glaList surface_legs;
        glaList blobs;
    } m_Lists;

public:
    typedef enum {
	CURSOR_DEFAULT,
	CURSOR_POINTING_HAND,
	CURSOR_DRAGGING_HAND,
	CURSOR_HORIZONTAL_RESIZE,
	CURSOR_ROTATE_HORIZONTALLY,
	CURSOR_ROTATE_VERTICALLY,
	CURSOR_ROTATE_EITHER_WAY,
	CURSOR_ZOOM
    } cursor;

private:
    GUIControl* m_Control;
    char* m_LabelGrid;
    bool m_RotationOK;
    LockFlags m_Lock;
    MainFrm* m_Parent;
    bool m_DoneFirstShow;
    int m_Polylines;
    int m_SurfacePolylines;
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
    int m_ColourBy;

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

    GLAPen * m_Pens;

#define PLAYING 1
    int presentation_mode; // for now, 0 => off, PLAYING => continuous play
    bool pres_reverse;
    double pres_speed;
    PresentationMark next_mark;
    double next_mark_time;
    double this_mark_total;

    MovieMaker * mpeg;

    cursor current_cursor;

    void UpdateQuaternion();

    void SetColourFromHeight(Double z, Double factor);
    void PlaceVertexWithColour(Double x, Double y, Double z,
			       Double factor = 1.0);
    void PlaceVertexWithDepthColour(Double x, Double y, Double z,
				    Double factor = 1.0);

    Double GridXToScreen(Double x, Double y, Double z) const;
    Double GridYToScreen(Double x, Double y, Double z) const;
    Double GridXToScreen(const Point &p) const {
	return GridXToScreen(p.x, p.y, p.z);
    }
    Double GridYToScreen(const Point &p) const {
	return GridYToScreen(p.x, p.y, p.z);
    }

    int GetClinoOffset() const;
    wxPoint CompassPtToScreen(Double x, Double y, Double z) const;
    void DrawTick(wxCoord cx, wxCoord cy, int angle_cw);
    wxString FormatLength(Double, bool scalebar = true);

    void SkinPassage(const vector<XSect> & centreline);

    void GenerateDisplayList();
    void GenerateDisplayListTubes();
    void GenerateDisplayListSurface();
    void GenerateBlobsDisplayList();

    void DrawIndicators();

    void TryToFreeArrays();
    void FirstShow();

    void DrawScalebar();
    void DrawDepthbar();
    void Draw2dIndicators();
    void DrawGrid();

    GLAPoint IndicatorCompassToScreenPan(int angle) const;
    GLAPoint IndicatorCompassToScreenElev(int angle) const;

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

public:
    GfxCore(MainFrm* parent, wxWindow* parent_window, GUIControl* control);
    ~GfxCore();

    void Initialise();
    void InitialiseTerrain();

    void UpdateBlobs();
    void ForceRefresh();

    void RefreshLine(const Point& a, const Point& b, const Point& c);

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
    void OnMouseWheel(wxMouseEvent& event) { m_Control->OnMouseWheel(event); }
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
    bool CanRotate() const;
    void ReverseRotation();
    void RotateSlower(bool accel);
    void RotateFaster(bool accel);

    void SwitchToElevation();
    void SwitchToPlan();

    bool ShowingPlan() const;
    bool ShowingElevation() const;
    bool ShowingMeasuringLine() const;

    bool CanRaiseViewpoint() const;
    bool CanLowerViewpoint() const;

    LockFlags GetLock() const { return m_Lock; }
    bool IsRotating() const { return m_Rotating; }
    bool HasData() const { return m_DoneFirstShow && m_HaveData; }

    double GetScale() const { return m_Params.scale; }
    void SetScale(Double scale);

    bool ShowingStationNames() const { return m_Names; }
    bool ShowingOverlappingNames() const { return m_OverlappingNames; }
    bool ShowingCrosses() const { return m_Crosses; }
    bool ShowingGrid() const { return m_Grid; }

    bool ColouringBy() const { return m_ColourBy; }

    bool HasUndergroundLegs() const;
    bool HasSurfaceLegs() const;

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
    void ToggleMetric() { ToggleFlag(&m_Metric); }
    void ToggleDegrees() { ToggleFlag(&m_Degrees); }
    void ToggleTubes() { ToggleFlag(&m_Tubes); }
    void TogglePerspective() { GLACanvas::TogglePerspective(); ForceRefresh(); }

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
    void AddPolyline(const vector<PointInfo> & centreline);
    void AddPolylineDepth(const vector<PointInfo> & centreline);
    void AddQuadrilateral(const Vector3 &a, const Vector3 &b,
			  const Vector3 &c, const Vector3 &d);
    void AddQuadrilateralDepth(const Vector3 &a, const Vector3 &b,
			       const Vector3 &c, const Vector3 &d);
    void MoveViewer(double forward, double up, double right);

    void (GfxCore::* AddQuad)(const Vector3 &a, const Vector3 &b,
                              const Vector3 &c, const Vector3 &d);
    void (GfxCore::* AddPoly)(const vector<PointInfo> & centreline);

    PresentationMark GetView() const;
    void SetView(const PresentationMark & p);
    void PlayPres(double speed, bool change_speed = true);
    int GetPresentationMode() const { return presentation_mode; }
    double GetPresentationSpeed() const { return pres_speed; }

    void SetColourBy(int colour_by);
    bool ExportMovie(const wxString & fnm);
    void OnPrint(const wxString &filename, const wxString &title,
		 const wxString &datestamp);
    bool OnExport(const wxString &filename, const wxString &title);
    void SetCursor(GfxCore::cursor new_cursor);

private:
    DECLARE_EVENT_TABLE()
};

#endif
