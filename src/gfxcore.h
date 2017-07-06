//
//  gfxcore.h
//
//  Core drawing code for Aven.
//
//  Copyright (C) 2000-2001,2002,2005 Mark R. Shinwell.
//  Copyright (C) 2001-2004,2005,2006,2007,2010,2011,2012,2013,2014,2015,2016,2017 Olly Betts
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
#include <limits.h>
#include <time.h>

#include "img_hosted.h"

#include "guicontrol.h"
#include "labelinfo.h"
#include "vector3.h"
#include "wx.h"
#include "gla.h"

#include <list>
#include <utility>
#include <vector>

using namespace std;

class MainFrm;
class traverse;

class XSect;
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

struct ZoomBox {
  public:
    int x1, y1, x2, y2;

    ZoomBox()
	: x1(INT_MAX) { }

    bool active() const { return x1 != INT_MAX; }

    void set(const wxPoint & p1, const wxPoint & p2) {
	x1 = p1.x;
	y1 = p1.y;
	x2 = p2.x;
	y2 = p2.y;
    }

    void unset() {
	x1 = INT_MAX;
    }
};

enum {
    COLOUR_BY_NONE,
    COLOUR_BY_DEPTH,
    COLOUR_BY_DATE,
    COLOUR_BY_ERROR,
    COLOUR_BY_GRADIENT,
    COLOUR_BY_LENGTH,
    COLOUR_BY_LIMIT_ // Leave this last.
};

enum {
    UPDATE_NONE,
    UPDATE_BLOBS,
    UPDATE_BLOBS_AND_CROSSES
};

enum {
    SHOW_HIDE,
    SHOW_DASHED,
    SHOW_FADED,
    SHOW_NORMAL,
};

struct Split {
    Vector3 vec;
    glaCoord tx, ty;

    Split(const Vector3& vec_, glaCoord tx_, glaCoord ty_)
	: vec(vec_), tx(tx_), ty(ty_) { }
};

// It's pointless to redraw the screen as often as we can on a fast machine,
// since the display hardware will only update so many times per second.
// This is the maximum framerate we'll redraw at.
const int MAX_FRAMERATE = 50;

class GfxCore : public GLACanvas {
    Double m_Scale;
    Double initial_scale;
    int m_ScaleBarWidth;

    typedef enum {
	LIST_COMPASS,
	LIST_CLINO,
	LIST_CLINO_BACK,
	LIST_SCALE_BAR,
	LIST_DEPTH_KEY,
	LIST_DATE_KEY,
	LIST_ERROR_KEY,
	LIST_GRADIENT_KEY,
	LIST_LENGTH_KEY,
	LIST_UNDERGROUND_LEGS,
	LIST_TUBES,
	LIST_SURFACE_LEGS,
	LIST_BLOBS,
	LIST_CROSSES,
	LIST_GRID,
	LIST_SHADOW,
	LIST_TERRAIN,
	LIST_LIMIT_ // Leave this last.
    } drawing_list;

    static const int NUM_COLOUR_BANDS = 13;

    void SetPanBase() {
	base_pan = m_PanAngle;
	base_pan_time = timer.Time() - (1000 / MAX_FRAMERATE);
    }

    void SetTiltBase() {
	base_tilt = m_TiltAngle;
	base_tilt_time = timer.Time() - (1000 / MAX_FRAMERATE);
    }

    int GetCompassWidth() const;
    int GetClinoWidth() const;

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
    int m_Splays;
    int m_Dupes;
    bool m_Names;
    bool m_Scalebar;
    bool m_ColourKey;
    bool m_OverlappingNames;
    bool m_Compass;
    bool m_Clino;
    bool m_Tubes;
    int m_ColourBy;

    bool m_HaveData;
    bool m_HaveTerrain;
    bool m_MouseOutsideCompass;
    bool m_MouseOutsideElev;
    bool m_Surface;
    bool m_Entrances;
    bool m_FixedPts;
    bool m_ExportedPts;
    bool m_Grid;
    bool m_BoundingBox;
    bool m_Terrain;

    bool m_Degrees;
    bool m_Metric;
    bool m_Percent;

    bool m_HitTestDebug;
    bool m_RenderStats;

    list<LabelInfo*> *m_PointGrid;
    bool m_HitTestGridValid;

    LabelInfo temp_here;
    const LabelInfo * m_here;
    const LabelInfo * m_there;

    wxStopWatch timer;
    long base_tilt_time;
    long base_pan_time;
    Double base_tilt;
    Double base_pan;

    GLAPen m_Pens[NUM_COLOUR_BANDS + 1];

#define PLAYING 1
    int presentation_mode; // for now, 0 => off, PLAYING => continuous play
    bool pres_reverse;
    double pres_speed;
    PresentationMark next_mark;
    double next_mark_time;
    double this_mark_total;

    MovieMaker * movie;

    cursor current_cursor;

    int sqrd_measure_threshold;

    // The legends for each entry in the colour key.
    wxString key_legends[NUM_COLOUR_BANDS];

    wxPoint key_lowerleft[COLOUR_BY_LIMIT_];

    ZoomBox zoombox;

    // Copied from parent, so we can adjust view when reloading the same
    // file with the view restricted.
    Vector3 offsets;

    // DEM:
    unsigned short * dem;
    unsigned long dem_width, dem_height;
    double o_x, o_y, step_x, step_y;
    long nodata_value;
    bool bigendian;
    long last_time;
    size_t n_tris;

    void PlaceVertexWithColour(const Vector3 &v, Double factor = 1.0);
    void PlaceVertexWithColour(const Vector3 & v,
			       glaTexCoord tex_x, glaTexCoord tex_y,
			       Double factor);
    void SetDepthColour(Double z, Double factor);
    void PlaceVertexWithDepthColour(const Vector3 & v, Double factor = 1.0);
    void PlaceVertexWithDepthColour(const Vector3 & v,
				    glaTexCoord tex_x, glaTexCoord tex_y,
				    Double factor);

    void SetColourFrom01(double how_far, Double factor);

    void SetColourFromDate(int date, Double factor);
    void SetColourFromError(double E, Double factor);
    void SetColourFromGradient(double angle, Double factor);
    void SetColourFromLength(double len, Double factor);

    int GetClinoOffset() const;
    void DrawTick(int angle_cw);
    void DrawArrow(gla_colour col1, gla_colour col2);

    void SkinPassage(vector<XSect> & centreline, bool draw = true);

    virtual void GenerateList(unsigned int l);
    void GenerateDisplayList(bool surface);
    void GenerateDisplayListTubes();
    void DrawTerrainTriangle(const Vector3 & a, const Vector3 & b, const Vector3 & c);
    void DrawTerrain();
    void GenerateDisplayListShadow();
    void GenerateBlobsDisplayList();

    void DrawIndicators();

    void TryToFreeArrays();
    void FirstShow();

    void DrawScaleBar();
    void DrawColourKey(int num_bands, const wxString & other, const wxString & units);
    void DrawDepthKey();
    void DrawDateKey();
    void DrawErrorKey();
    void DrawGradientKey();
    void DrawLengthKey();
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

    const GLAPen& GetPen(int band) const {
	assert(band >= 0 && band < NUM_COLOUR_BANDS);
	return m_Pens[band];
    }

    const GLAPen& GetSurfacePen() const { return m_Pens[NUM_COLOUR_BANDS]; }

    int GetNumColourBands() const { return NUM_COLOUR_BANDS; }

    void DrawShadowedBoundingBox();
    void DrawBoundingBox();

public:
    GfxCore(MainFrm* parent, wxWindow* parent_window, GUIControl* control);
    ~GfxCore();

    void Initialise(bool same_file);

    void UpdateBlobs();
    void ForceRefresh();

    void RefreshLine(const Point* a, const Point* b, const Point* c);

    void SetHereFromTree(const LabelInfo * p);

    void SetHere(const LabelInfo * p = NULL);
    void SetThere(const LabelInfo * p = NULL);

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

    void Animate();
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
    bool PointWithinColourKey(wxPoint point) const;

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

    double GetCompassValue() const { return m_PanAngle; }
    bool ShowingPlan() const;
    bool ShowingElevation() const;
    bool ShowingMeasuringLine() const;
    bool HereIsReal() const { return m_here && m_here != &temp_here; }

    bool CanRaiseViewpoint() const;
    bool CanLowerViewpoint() const;

    bool IsRotating() const { return m_Rotating; }
    bool HasData() const { return m_DoneFirstShow && m_HaveData; }
    bool HasTerrain() const { return m_DoneFirstShow && m_HaveTerrain; }
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
    bool HasSplays() const;
    bool HasDupes() const;
    bool HasSurfaceLegs() const;
    bool HasTubes() const;

    bool ShowingUndergroundLegs() const { return m_Legs; }
    int ShowingSplaysMode() const { return m_Splays; }
    int ShowingDupesMode() const { return m_Dupes; }
    bool ShowingSurfaceLegs() const { return m_Surface; }

    bool ShowingColourKey() const { return m_ColourKey; }
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
    void SetSplaysMode(int mode) {
	m_Splays = mode;
	UpdateBlobs();
	InvalidateList(LIST_SURFACE_LEGS);
	InvalidateList(LIST_UNDERGROUND_LEGS);
	ForceRefresh();
    }
    void SetDupesMode(int mode) {
	m_Dupes = mode;
	UpdateBlobs();
	InvalidateList(LIST_SURFACE_LEGS);
	InvalidateList(LIST_UNDERGROUND_LEGS);
	ForceRefresh();
    }
    void ToggleSurfaceLegs() {
	ToggleFlag(&m_Surface, UPDATE_BLOBS_AND_CROSSES);
    }
    void ToggleCompass() {
	ToggleFlag(&m_Compass);
	InvalidateList(LIST_SCALE_BAR);
    }
    void ToggleClino() {
	ToggleFlag(&m_Clino);
	InvalidateList(LIST_SCALE_BAR);
    }
    void ToggleScaleBar() { ToggleFlag(&m_Scalebar); }
    void ToggleEntrances() { ToggleFlag(&m_Entrances, UPDATE_BLOBS); }
    void ToggleFixedPts() { ToggleFlag(&m_FixedPts, UPDATE_BLOBS); }
    void ToggleExportedPts() { ToggleFlag(&m_ExportedPts, UPDATE_BLOBS); }
    void ToggleGrid() { ToggleFlag(&m_Grid); }
    void ToggleCrosses() { ToggleFlag(&m_Crosses); }
    void ToggleStationNames() { ToggleFlag(&m_Names); }
    void ToggleOverlappingNames() { ToggleFlag(&m_OverlappingNames); }
    void ToggleColourKey() { ToggleFlag(&m_ColourKey); }
    void ToggleMetric() {
	ToggleFlag(&m_Metric);
	InvalidateList(LIST_DEPTH_KEY);
	InvalidateList(LIST_LENGTH_KEY);
	InvalidateList(LIST_SCALE_BAR);
    }
    void ToggleHitTestDebug() {
	ToggleFlag(&m_HitTestDebug);
    }
    void ToggleRenderStats() {
	ToggleFlag(&m_RenderStats);
    }
    void ToggleDegrees() {
	ToggleFlag(&m_Degrees);
	InvalidateList(LIST_GRADIENT_KEY);
    }
    void TogglePercent() { ToggleFlag(&m_Percent); }
    void ToggleTubes() { ToggleFlag(&m_Tubes); }
    void TogglePerspective() { GLACanvas::TogglePerspective(); ForceRefresh(); }
    void ToggleSmoothShading();
    bool DisplayingBoundingBox() const { return m_BoundingBox; }
    void ToggleBoundingBox() { ToggleFlag(&m_BoundingBox); }
    bool DisplayingTerrain() const { return m_Terrain; }
    void ToggleTerrain();
    void ToggleFatFinger();
    void ToggleTextured() {
	GLACanvas::ToggleTextured();
	ForceRefresh();
    }

    bool GetMetric() const { return m_Metric; }
    bool GetDegrees() const { return m_Degrees; }
    bool GetPercent() const { return m_Percent; }
    bool GetTubes() const { return m_Tubes; }

    bool CheckHitTestGrid(const wxPoint& point, bool centre);

    void ClearTreeSelection();

    void Defaults();

    void FullScreenMode();

    bool IsFullScreen() const;

    bool FullScreenModeShowingMenus() const;

    void FullScreenModeShowMenus(bool show);

    void DragFinished();

    void SplitLineAcrossBands(int band, int band2,
			      const Vector3 &p, const Vector3 &q,
			      Double factor = 1.0);
    void SplitPolyAcrossBands(vector<vector<Split> >& splits,
			      int band, int band2,
			      const Vector3 &p, const Vector3 &q,
			      glaTexCoord ptx, glaTexCoord pty,
			      glaTexCoord w, glaTexCoord h);
    int GetDepthColour(Double z) const;
    Double GetDepthBoundaryBetweenBands(int a, int b) const;
    void AddPolyline(const traverse & centreline);
    void AddPolylineDepth(const traverse & centreline);
    void AddPolylineDate(const traverse & centreline);
    void AddPolylineError(const traverse & centreline);
    void AddPolylineGradient(const traverse & centreline);
    void AddPolylineLength(const traverse & centreline);
    void AddQuadrilateral(const Vector3 &a, const Vector3 &b,
			  const Vector3 &c, const Vector3 &d);
    void AddPolylineShadow(const traverse & centreline);
    void AddQuadrilateralDepth(const Vector3 &a, const Vector3 &b,
			       const Vector3 &c, const Vector3 &d);
    void AddQuadrilateralDate(const Vector3 &a, const Vector3 &b,
			      const Vector3 &c, const Vector3 &d);
    void AddQuadrilateralError(const Vector3 &a, const Vector3 &b,
			       const Vector3 &c, const Vector3 &d);
    void AddQuadrilateralGradient(const Vector3 &a, const Vector3 &b,
				  const Vector3 &c, const Vector3 &d);
    void AddQuadrilateralLength(const Vector3 &a, const Vector3 &b,
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
		 const wxString &datestamp, time_t datestamp_numeric,
		 const wxString &cs_proj,
		 bool close_after_print = false);
    void OnExport(const wxString &filename, const wxString &title,
		  const wxString &datestamp, time_t datestamp_numeric,
		  const wxString &cs_proj);
    void UpdateCursor(GfxCore::cursor new_cursor);
    bool MeasuringLineActive() const;

    bool HandleRClick(wxPoint point);

    void InvalidateAllLists() {
	for (int i = 0; i < LIST_LIMIT_; ++i) {
	    InvalidateList(i);
	}
    }

    void SetZoomBox(wxPoint p1, wxPoint p2, bool centred, bool aspect);

    void UnsetZoomBox() {
	if (!zoombox.active()) return;
	zoombox.unset();
	ForceRefresh();
    }

    void ZoomBoxGo();

    void parse_hgt_filename(const wxString & lc_name);
    size_t parse_hdr(wxInputStream & is, unsigned long & skipbytes);
    bool read_bil(wxInputStream & is, size_t size, unsigned long skipbytes);
    bool LoadDEM(const wxString & file);

private:
    DECLARE_EVENT_TABLE()
};

#endif
