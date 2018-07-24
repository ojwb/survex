//
//  guicontrol.h
//
//  Handlers for events relating to the display of a survey.
//
//  Copyright (C) 2000-2002,2005 Mark R. Shinwell
//  Copyright (C) 2001-2004,2006,2014,2015 Olly Betts
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
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

#ifndef guicontrol_h
#define guicontrol_h

#include "wx.h"

class GfxCore;

class GUIControl {
    GfxCore* m_View;

    enum { NO_DRAG = 0, LEFT_DRAG, MIDDLE_DRAG, RIGHT_DRAG } dragging;

    wxPoint m_DragStart;
    wxPoint m_DragRealStart;
    wxPoint m_DragLast;

    enum {
	drag_NONE,
	drag_MAIN,
	drag_COMPASS,
	drag_ELEV,
	drag_SCALE,
	drag_ZOOM
    } m_LastDrag;

    enum { lock_NONE, lock_ROTATE, lock_SCALE } m_ScaleRotateLock;

    bool m_ReverseControls;

    void HandleRotate(wxPoint);
    void HandleTilt(wxPoint);
    void HandleTranslate(wxPoint);
    void HandleScaleRotate(wxPoint);
    void HandleTiltRotate(wxPoint);

    void HandCursor();
    void RestoreCursor();

    void HandleNonDrag(const wxPoint & point);

public:
    GUIControl();

    void SetView(GfxCore* view);

    bool MouseDown() const;

    void OnDefaults();
    void OnPlan();
    void OnElevation();
    void OnDisplayOverlappingNames();
    void OnColourByDepth();
    void OnColourByDate();
    void OnColourByError();
    void OnColourByGradient();
    void OnColourByLength();
    void OnColourBySurvey();
    void OnShowCrosses();
    void OnShowStationNames();
    void OnShowSurveyLegs();
    void OnHideSplays();
    void OnShowSplaysDashed();
    void OnShowSplaysFaded();
    void OnShowSplaysNormal();
    void OnHideDupes();
    void OnShowDupesDashed();
    void OnShowDupesFaded();
    void OnShowDupesNormal();
    void OnShowSurface();
    void OnMoveEast();
    void OnMoveNorth();
    void OnMoveSouth();
    void OnMoveWest();
    void OnToggleRotation();
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
    void OnToggleColourKey();
    void OnViewCompass();
    void OnViewClino();
    void OnViewGrid();
    void OnReverseDirectionOfRotation();
    void OnShowEntrances();
    void OnShowFixedPts();
    void OnShowExportedPts();
    void OnCancelDistLine();
    void OnMouseMove(wxMouseEvent& event);
    void OnLButtonDown(wxMouseEvent& event);
    void OnLButtonUp(wxMouseEvent& event);
    void OnMButtonDown(wxMouseEvent& event);
    void OnMButtonUp(wxMouseEvent& event);
    void OnRButtonDown(wxMouseEvent& event);
    void OnRButtonUp(wxMouseEvent& event);
    void OnMouseWheel(wxMouseEvent& event);
    void OnKeyPress(wxKeyEvent &e);

    void OnDisplayOverlappingNamesUpdate(wxUpdateUIEvent&);
    void OnColourByUpdate(wxUpdateUIEvent&);
    void OnColourByDepthUpdate(wxUpdateUIEvent&);
    void OnColourByDateUpdate(wxUpdateUIEvent&);
    void OnColourByErrorUpdate(wxUpdateUIEvent&);
    void OnColourByGradientUpdate(wxUpdateUIEvent&);
    void OnColourByLengthUpdate(wxUpdateUIEvent&);
    void OnColourBySurveyUpdate(wxUpdateUIEvent&);
    void OnShowCrossesUpdate(wxUpdateUIEvent&);
    void OnShowStationNamesUpdate(wxUpdateUIEvent&);
    void OnShowSurveyLegsUpdate(wxUpdateUIEvent&);
    void OnSplaysUpdate(wxUpdateUIEvent&);
    void OnHideSplaysUpdate(wxUpdateUIEvent&);
    void OnShowSplaysDashedUpdate(wxUpdateUIEvent&);
    void OnShowSplaysFadedUpdate(wxUpdateUIEvent&);
    void OnShowSplaysNormalUpdate(wxUpdateUIEvent&);
    void OnDupesUpdate(wxUpdateUIEvent&);
    void OnHideDupesUpdate(wxUpdateUIEvent&);
    void OnShowDupesDashedUpdate(wxUpdateUIEvent&);
    void OnShowDupesFadedUpdate(wxUpdateUIEvent&);
    void OnShowDupesNormalUpdate(wxUpdateUIEvent&);
    void OnShowSurfaceUpdate(wxUpdateUIEvent&);
    void OnMoveEastUpdate(wxUpdateUIEvent&);
    void OnMoveNorthUpdate(wxUpdateUIEvent&);
    void OnMoveSouthUpdate(wxUpdateUIEvent&);
    void OnMoveWestUpdate(wxUpdateUIEvent&);
    void OnToggleRotationUpdate(wxUpdateUIEvent&);
    void OnReverseControlsUpdate(wxUpdateUIEvent&);
    void OnReverseDirectionOfRotationUpdate(wxUpdateUIEvent&);
    void OnDefaultsUpdate(wxUpdateUIEvent&);
    void OnElevationUpdate(wxUpdateUIEvent&);
    void OnPlanUpdate(wxUpdateUIEvent&);
    void OnToggleScalebarUpdate(wxUpdateUIEvent&);
    void OnToggleColourKeyUpdate(wxUpdateUIEvent&);
    void OnViewCompassUpdate(wxUpdateUIEvent&);
    void OnViewClinoUpdate(wxUpdateUIEvent&);
    void OnViewGridUpdate(wxUpdateUIEvent&);
    void OnShowEntrancesUpdate(wxUpdateUIEvent&);
    void OnShowExportedPtsUpdate(wxUpdateUIEvent&);
    void OnShowFixedPtsUpdate(wxUpdateUIEvent&);

    void OnIndicatorsUpdate(wxUpdateUIEvent&);
    void OnCancelDistLineUpdate(wxUpdateUIEvent&);

    void OnViewPerspective();
    void OnViewPerspectiveUpdate(wxUpdateUIEvent& cmd);

    void OnViewSmoothShading();
    void OnViewSmoothShadingUpdate(wxUpdateUIEvent& cmd);

    void OnViewTextured();
    void OnViewTexturedUpdate(wxUpdateUIEvent& cmd);

    void OnViewFog();
    void OnViewFogUpdate(wxUpdateUIEvent& cmd);

    void OnViewSmoothLines();
    void OnViewSmoothLinesUpdate(wxUpdateUIEvent& cmd);

    void OnToggleMetric();
    void OnToggleMetricUpdate(wxUpdateUIEvent& cmd);

    void OnToggleDegrees();
    void OnToggleDegreesUpdate(wxUpdateUIEvent& cmd);

    void OnTogglePercent();
    void OnTogglePercentUpdate(wxUpdateUIEvent& cmd);

    void OnToggleTubes();
    void OnToggleTubesUpdate(wxUpdateUIEvent& cmd);

    void OnViewFullScreenUpdate(wxUpdateUIEvent&);
    void OnViewFullScreen();

    void OnViewBoundingBoxUpdate(wxUpdateUIEvent&);
    void OnViewBoundingBox();

    void OnViewTerrainUpdate(wxUpdateUIEvent&);
    void OnViewTerrain();
};

#endif
