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

#include "quaternion.h"
#include "wx.h"
#ifdef AVENGL
#ifndef wxUSE_GLCANVAS
#define wxUSE_GLCANVAS
#endif
#include <wx/glcanvas.h>
#endif

class MainFrm;

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
	} display_shift;
    } m_Params;

#ifdef AVENGL
    struct {
        // OpenGL display lists.
        GLint survey; // all underground data
        GLint surface; // all surface data in uniform colour
        GLint surface_depth; // all surface data in depth bands
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
        wxPoint* vertices;
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
    wxString* m_Labels;
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
    // wxCursor m_StdCursor;
    // wxCursor m_ScaleRotateCursor;
    bool m_Grid;

    struct pens {
        wxPen black;
        wxPen yellow;
        wxPen red;
        wxPen cyan;
        wxPen turquoise;
        wxPen green;
        wxPen white;
        wxPen grey;
        wxPen lgrey;
        wxPen lgrey2;
        wxPen indicator1;
        wxPen indicator2;
    } m_Pens;

    struct brushes {
        wxBrush black;
        wxBrush yellow;
        wxBrush red;
        wxBrush cyan;
        wxBrush grey;
        wxBrush dgrey;
        wxBrush white;
        wxBrush indicator1;
        wxBrush indicator2;
    } m_Brushes;

    Double XToScreen(Double x, Double y, Double z) {
        return Double(x*m_RotationMatrix.get(0, 0) + y*m_RotationMatrix.get(0, 1) +
		     z*m_RotationMatrix.get(0, 2));
    }

    Double YToScreen(Double x, Double y, Double z) {
        return Double(x*m_RotationMatrix.get(1, 0) + y*m_RotationMatrix.get(1, 1) +
		     z*m_RotationMatrix.get(1, 2));
    }

    Double ZToScreen(Double x, Double y, Double z) {
        return Double(x*m_RotationMatrix.get(2, 0) + y*m_RotationMatrix.get(2, 1) +
		     z*m_RotationMatrix.get(2, 2));
    }

    Double GridXToScreen(Double x, Double y, Double z);
    Double GridYToScreen(Double x, Double y, Double z);

    wxCoord GetClinoOffset();
    wxPoint CompassPtToScreen(Double x, Double y, Double z);
    void DrawTick(wxCoord cx, wxCoord cy, int angle_cw);
    wxString FormatLength(Double, bool scalebar = true);

    void SetScale(Double scale);
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
    void Plan();
    void Elevation();

    void Repaint();

public:
    GfxCore(MainFrm* parent);
    ~GfxCore();

    void Initialise();
    void InitialiseOnNextResize() { m_InitialisePending = true; }

    void OnDefaults(wxCommandEvent&);
    void OnPlan(wxCommandEvent&);
    void OnElevation(wxCommandEvent&);
    void OnDisplayOverlappingNames(wxCommandEvent&);
    void OnShowCrosses(wxCommandEvent&);
    void OnShowStationNames(wxCommandEvent&);
    void OnShowSurveyLegs(wxCommandEvent&);
    void OnShowSurface(wxCommandEvent&);
    void OnShowSurfaceDepth(wxCommandEvent&);
    void OnShowSurfaceDashed(wxCommandEvent&);
    void OnMoveEast(wxCommandEvent&);
    void OnMoveNorth(wxCommandEvent&);
    void OnMoveSouth(wxCommandEvent&);
    void OnMoveWest(wxCommandEvent&);
    void OnStartRotation(wxCommandEvent&);
    void OnStopRotation(wxCommandEvent&);
    void OnReverseControls(wxCommandEvent&);
    void OnSlowDown(wxCommandEvent&);
    void OnSpeedUp(wxCommandEvent&);
    void OnStepOnceAnticlockwise(wxCommandEvent&);
    void OnStepOnceClockwise(wxCommandEvent&);
    void OnHigherViewpoint(wxCommandEvent&);
    void OnLowerViewpoint(wxCommandEvent&);
    void OnShiftDisplayDown(wxCommandEvent&);
    void OnShiftDisplayLeft(wxCommandEvent&);
    void OnShiftDisplayRight(wxCommandEvent&);
    void OnShiftDisplayUp(wxCommandEvent&);
    void OnZoomIn(wxCommandEvent&);
    void OnZoomOut(wxCommandEvent&);
    void OnToggleScalebar(wxCommandEvent&);
    void OnToggleDepthbar(wxCommandEvent&);
    void OnViewCompass(wxCommandEvent&);
    void OnViewClino(wxCommandEvent&);
    void OnViewGrid(wxCommandEvent&);
    void OnReverseDirectionOfRotation(wxCommandEvent&);
    void OnShowEntrances(wxCommandEvent&);
    void OnShowFixedPts(wxCommandEvent&);
    void OnShowExportedPts(wxCommandEvent&);
#ifdef AVENGL
    void OnAntiAlias(wxCommandEvent&);
#endif

    void OnPaint(wxPaintEvent&);
    void OnMouseMove(wxMouseEvent& event);
    void OnLButtonDown(wxMouseEvent& event);
    void OnLButtonUp(wxMouseEvent& event);
    void OnMButtonDown(wxMouseEvent& event);
    void OnMButtonUp(wxMouseEvent& event);
    void OnRButtonDown(wxMouseEvent& event);
    void OnRButtonUp(wxMouseEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnTimer(wxTimerEvent& event);

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
#endif
    void OnIndicatorsUpdate(wxUpdateUIEvent&);

private:
    DECLARE_EVENT_TABLE()
};

#endif
