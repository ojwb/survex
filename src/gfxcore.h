//
//  gfxcore.h
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

#ifndef gfxcore_h
#define gfxcore_h

#include <float.h>

#include "quaternion.h"
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

#ifdef AVENGL
struct Double3 {
    Double x;
    Double y;
    Double z;
};
#endif

struct ColourTriple {
    // RGB triple: values are from 0-255 inclusive for each component.
    int r;
    int g;
    int b;
};

// Colours for drawing.
// These must be in the same order as the entries in the array COLOURS in gfxcore.cc.
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
    col_CYAN,
    col_LAST // must be the last entry here
};

class SpecialPoint {
    friend class GfxCore;

    Double x, y, z;
    AvenColour colour;
    int size;
#ifndef AVENGL
    int screen_x, screen_y;
#endif
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
        struct {
	    int x;
	    int y;
	    int z;
	} display_shift;
    } m_Params;

#ifdef AVENPRES
    struct PresData {
        struct {
	    Double x, y, z;
	} translation;
        struct {
	    int x, y, z;
	} display_shift;
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

    enum LabelFlags {
        label_NOT_PLOTTED,
	label_PLOTTED,
	label_CHECK_AGAIN
    };

    enum HighlightFlags {
        hl_NONE = 0,
        hl_ENTRANCE = 1,
        hl_EXPORTED = 2,
        hl_FIXED = 4
    };

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

    struct {
#ifdef AVENGL
        // For the OpenGL version we store the real (x, y, z) coordinates of
	// each station.
        Double3* vertices;
#else
        // For the non-OpenGL version we store the screen coordinates of
	// each station after the transformation has been applied.
        wxPoint* vertices;
#endif
        int* num_segs;
    } m_CrossData;

    struct PlotData {
        wxPoint* vertices;
        wxPoint* surface_vertices;
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

    struct HighlightedPt {
        int x;
        int y;
        HighlightFlags flags;
    };

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
#endif

    Double floor_alt;
    bool terrain_rising;

    struct GridPointInfo {
        long x, y; // screen coordinates
	LabelInfo* label;
    };

    list<SpecialPoint> m_SpecialPoints;
#ifdef AVENPRES
    list<pair<PresData, Quaternion> > m_Presentation;
    list<pair<PresData, Quaternion> >::iterator m_PresIterator;
#endif
    double m_MaxExtent; // twice the maximum of the {x,y,z}-extents, in survey coordinates.
    LabelFlags* m_LabelGrid;
    HighlightedPt* m_HighlightedPts;
    int m_NumHighlightedPts;
    bool m_ScaleHighlightedPtsOnly;
    bool m_DepthbarOff;
    bool m_ScalebarOff;
    bool m_IndicatorsOff;
    LockFlags m_Lock;
    Matrix4 m_RotationMatrix;
    bool m_DraggingLeft;
    bool m_DraggingMiddle;
    bool m_DraggingRight;
    MainFrm* m_Parent;
    wxPoint m_DragStart;
    wxPoint m_DragRealStart;
    wxBitmap m_OffscreenBitmap;
    wxMemoryDC m_DrawDC;
    bool m_DoneFirstShow;
    bool m_RedrawOffscreen;
    int* m_Polylines;
    int* m_SurfacePolylines;
    int m_Bands;
    Double m_InitialScale;
    bool m_FreeRotMode;
    Double m_TiltAngle;
    Double m_PanAngle;
    bool m_Rotating;
    Double m_RotationStep;
    bool m_ReverseControls;
    bool m_SwitchingToPlan;
    bool m_SwitchingToElevation;
    bool m_ScaleCrossesOnly;
    bool m_Crosses;
    bool m_Legs;
    bool m_Names;
    bool m_Scalebar;
    wxFont m_Font;
    bool m_Depthbar;
    bool m_OverlappingNames;
    bool m_LabelCacheNotInvalidated;
    LabelFlags* m_LabelsLastPlotted;
    wxRect m_LabelCacheExtend;
    bool m_Compass;
    bool m_Clino;
    int m_XSize;
    int m_YSize;
    int m_XCentre;
    int m_YCentre;
    wxPoint m_LabelShift;
    wxTimer m_Timer;
    PlotData* m_PlotData;
    LabelInfo** m_Labels;
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
    int m_NumCrosses;
    list<GridPointInfo>* m_PointGrid;
    bool m_HitTestGridValid;
    bool m_TerrainLoaded;
    list<PointInfo> m_PointCache;

#ifdef AVENPRES
    int m_DoingPresStep;
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
        struct {
	    int x;
	    int y;
	    int z;
	} display_shift;
    };

    struct {
        step_params from;
        step_params to;
    } m_PresStep;
#endif

    struct {
        Double x, y, z;
    } m_here;

    struct {
        Double x, y, z;
    } m_there;

    bool m_ScaleSpecialPtsOnly;
    bool m_DrawDistLine;

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

    void CheckHitTestGrid(wxPoint& point, bool centre);

    wxCoord GetClinoOffset();
    wxPoint CompassPtToScreen(Double x, Double y, Double z);
    void DrawTick(wxCoord cx, wxCoord cy, int angle_cw);
    wxString FormatLength(Double, bool scalebar = true);

    void SetScale(Double scale);
    void SetScaleInitial(Double scale);
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

    void TiltCave(Double tilt_angle);
    void TurnCave(Double angle);
    void TurnCaveTo(Double angle);

    void StartTimer();
    void StopTimer();
    
    void DrawNames();
    void NattyDrawNames();
    void SimpleDrawNames();

    void Defaults();
    void DefaultParameters();

    void Repaint();

#ifdef AVENPRES
    void PresGoto(PresData& d, Quaternion& q);
#endif

    void CreateHitTestGrid();
    
    void RenderTerrain(Double floor_alt);

public:
    GfxCore(MainFrm* parent, wxWindow* parent_window);
    ~GfxCore();

    void Initialise();
    void InitialiseOnNextResize() { m_InitialisePending = true; }
    void InitialiseTerrain();

    void ClearSpecialPoints();
    void AddSpecialPoint(Double x, Double y, Double z);
    void DisplaySpecialPoints();

    void SetHere();
    void SetHere(Double x, Double y, Double z);
    void SetThere();
    void SetThere(Double x, Double y, Double z);

    void CentreOn(Double x, Double y, Double z);

#ifdef AVENPRES
    void RecordPres(FILE* fp);
    void LoadPres(FILE* fp);
    void PresGo();
    void PresGoBack();
    void RestartPres();

    bool AtStartOfPres();
    bool AtEndOfPres();
#endif

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
    void OnSlowDown();
    void OnSpeedUp();
    void OnStepOnceAnticlockwise();
    void OnStepOnceClockwise();
    void OnHigherViewpoint();
    void OnLowerViewpoint();
    void OnShiftDisplayDown();
    void OnShiftDisplayLeft();
    void OnShiftDisplayRight();
    void OnShiftDisplayUp();
    void OnZoomIn();
    void OnZoomOut();
    void OnToggleScalebar();
    void OnToggleDepthbar();
    void OnViewCompass();
    void OnViewClino();
    void OnViewGrid();
    void OnReverseDirectionOfRotation();
    void OnShowEntrances();
    void OnShowFixedPts();
    void OnShowExportedPts();
#ifdef AVENGL
    void OnAntiAlias();
    void OnSolidSurface();
#endif
    void OnCancelDistLine(wxCommandEvent&);

    void OnPaint(wxPaintEvent&);
    void OnMouseMove(wxMouseEvent& event);
    void OnLButtonDown(wxMouseEvent& event);
    void OnLButtonUp(wxMouseEvent& event);
    void OnMButtonDown(wxMouseEvent& event);
    void OnMButtonUp(wxMouseEvent& event);
    void OnRButtonDown(wxMouseEvent& event);
    void OnRButtonUp(wxMouseEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnTimer(wxIdleEvent& event);

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
#ifdef AVENGL
    void OnAntiAliasUpdate(wxUpdateUIEvent&);
    void OnSolidSurfaceUpdate(wxUpdateUIEvent&);
#endif
    void OnIndicatorsUpdate(wxUpdateUIEvent&);
    void OnCancelDistLineUpdate(wxUpdateUIEvent&);

private:
    DECLARE_EVENT_TABLE()
};

#endif
