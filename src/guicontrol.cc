//
//  guicontrol.cc
//
//  Handlers for events relating to the display of a survey.
//
//  Copyright (C) 2000-2002,2005 Mark R. Shinwell
//  Copyright (C) 2001,2003,2004,2005,2006 Olly Betts
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "guicontrol.h"
#include "gfxcore.h"
#include <wx/confbase.h>

const int DISPLAY_SHIFT = 10;
const double FLYFREE_SHIFT = 0.2;
const double ROTATE_STEP = 2.0;

GUIControl::GUIControl()
    : dragging(NO_DRAG)
{
    m_View = NULL;
    m_ReverseControls = false;
    m_LastDrag = drag_NONE;
}

GUIControl::~GUIControl()
{
    // no action
}

void GUIControl::SetView(GfxCore* view)
{
    m_View = view;
}

bool GUIControl::MouseDown() const
{
    return (dragging != NO_DRAG);
}

void GUIControl::HandleTilt(wxPoint point)
{
    // Handle a mouse movement during tilt mode.

    // wxGTK (at least) fails to update the cursor while dragging.
    m_View->SetCursor(GfxCore::CURSOR_ROTATE_VERTICALLY);

    int dy = point.y - m_DragStart.y;

    if (m_ReverseControls != m_View->GetPerspective()) dy = -dy;

    m_View->TiltCave(Double(-dy) * 0.36);

    m_DragStart = point;

    m_View->ForceRefresh();
}

void GUIControl::HandleTranslate(wxPoint point)
{
    // Handle a mouse movement during translation mode.

    // wxGTK (at least) fails to update the cursor while dragging.
    m_View->SetCursor(GfxCore::CURSOR_DRAGGING_HAND);

    int dx = point.x - m_DragStart.x;
    int dy = point.y - m_DragStart.y;

    if (m_ReverseControls) {
	dx = -dx;
	dy = -dy;
    }

    if (m_View->GetPerspective())
	m_View->MoveViewer(0, -dy * .1, dx * .1);
    else
	m_View->TranslateCave(dx, dy);

    m_DragStart = point;
}

void GUIControl::HandleScaleRotate(wxPoint point)
{
    // Handle a mouse movement during scale/rotate mode.

    // wxGTK (at least) fails to update the cursor while dragging.
    m_View->SetCursor(GfxCore::CURSOR_ZOOM_ROTATE);

    int dx, dy;
    int threshold;
    if (m_ScaleRotateLock == NONE) {
	// Dragging to scale or rotate but we've not decided which yet.
	dx = point.x - m_DragRealStart.x;
	dy = point.y - m_DragRealStart.y;
	threshold = 8 * 8;
    } else {
	dx = point.x - m_DragStart.x;
	dy = point.y - m_DragStart.y;
	threshold = 5;
    }
    int dx2 = dx * dx;
    int dy2 = dy * dy;
    if (dx2 + dy2 < threshold) return;

    switch (m_ScaleRotateLock) {
	case NONE:
	    if (dx2 > dy2) {
		m_ScaleRotateLock = ROTATE;
//		m_View->SetCursor(GfxCore::CURSOR_ROTATE_HORIZONTALLY);
	    } else {
		m_ScaleRotateLock = SCALE;
//		m_View->SetCursor(GfxCore::CURSOR_ZOOM);
	    }
	    break;
	case SCALE:
	    if (dx2 >= 8 * dy2) {
		m_ScaleRotateLock = ROTATE;
//		m_View->SetCursor(GfxCore::CURSOR_ROTATE_HORIZONTALLY);
	    }
	    break;
	case ROTATE:
	    if (dy2 >= 8 * dx2) {
		m_ScaleRotateLock = SCALE;
//		m_View->SetCursor(GfxCore::CURSOR_ZOOM);
	    }
	    break;
    }

    if (m_ScaleRotateLock == ROTATE) {
	dy = 0;
    } else {
	dx = 0;
    }

    if (m_ReverseControls) {
	dx = -dx;
	dy = -dy;
    }

    if (m_View->GetPerspective()) {
	if (dy) m_View->MoveViewer(-dy * .1, 0, 0);
    } else {
	// up/down => scale.
	if (dy) m_View->SetScale(m_View->GetScale() * pow(1.06, 0.08 * dy));
	// left/right => rotate.
	if (dx) m_View->TurnCave(Double(dx) * -0.36);
	if (dx || dy) m_View->ForceRefresh();
    }

    m_DragStart = point;
}

void GUIControl::HandleTiltRotate(wxPoint point)
{
    // Handle a mouse movement during tilt/rotate mode.
    if (m_View->IsExtendedElevation()) return;

    // wxGTK (at least) fails to update the cursor while dragging.
    m_View->SetCursor(GfxCore::CURSOR_ROTATE_EITHER_WAY);

    int dx = point.x - m_DragStart.x;
    int dy = point.y - m_DragStart.y;

    if (m_ReverseControls != m_View->GetPerspective()) {
	dx = -dx;
	dy = -dy;
    }

    // left/right => rotate, up/down => tilt.
    // Make tilt less sensitive than rotate as that feels better.
    m_View->TurnCave(Double(dx) * -0.36);
    m_View->TiltCave(Double(dy) * -0.18);

    m_View->ForceRefresh();

    m_DragStart = point;
}

void GUIControl::HandleRotate(wxPoint point)
{
    // Handle a mouse movement during rotate mode.
    if (m_View->IsExtendedElevation()) return;

    // wxGTK (at least) fails to update the cursor while dragging.
    m_View->SetCursor(GfxCore::CURSOR_ROTATE_HORIZONTALLY);

    int dx = point.x - m_DragStart.x;
    int dy = point.y - m_DragStart.y;

    if (m_ReverseControls != m_View->GetPerspective()) {
	dx = -dx;
	dy = -dy;
    }

    // left/right => rotate.
    m_View->TurnCave(Double(dx) * -0.36);

    m_View->ForceRefresh();

    m_DragStart = point;
}

void GUIControl::RestoreCursor()
{
    if (m_View->ShowingMeasuringLine()) {
        m_View->SetCursor(GfxCore::CURSOR_POINTING_HAND);
    } else {
        m_View->SetCursor(GfxCore::CURSOR_DEFAULT);
    }
}

//
//  Mouse event handling methods
//

void GUIControl::OnMouseMove(wxMouseEvent& event)
{
    // Mouse motion event handler.
    if (!m_View->HasData()) return;

    // Ignore moves which don't change the position.
    if (event.GetX() == m_DragStart.x && event.GetY() == m_DragStart.y) {
	return;
    }

    static long timestamp = LONG_MIN;
    if (dragging != NO_DRAG && m_ScaleRotateLock != NONE && timestamp != LONG_MIN) {
	// If no motion for a second, reset the direction lock.
	if (event.GetTimestamp() - timestamp >= 1000) {
	    m_ScaleRotateLock = NONE;
	    m_DragRealStart = m_DragStart;
	    RestoreCursor();
	}
    }
    timestamp = event.GetTimestamp();

    wxPoint point(event.GetX(), event.GetY());

    // Check hit-test grid (only if no buttons are pressed).
    if (!event.LeftIsDown() && !event.MiddleIsDown() && !event.RightIsDown()) {
	if (m_View->CheckHitTestGrid(point, false)) {
	    m_View->SetCursor(GfxCore::CURSOR_POINTING_HAND);
        } else if (m_View->PointWithinScaleBar(point)) {
	    m_View->SetCursor(GfxCore::CURSOR_HORIZONTAL_RESIZE);
        } else if (m_View->PointWithinCompass(point)) {
	    m_View->SetCursor(GfxCore::CURSOR_ROTATE_HORIZONTALLY);
        } else if (m_View->PointWithinClino(point)) {
	    m_View->SetCursor(GfxCore::CURSOR_ROTATE_VERTICALLY);
	} else {
	    RestoreCursor();
	}
    }

    // Update coordinate display if in plan view,
    // or altitude if in elevation view.
    m_View->SetCoords(point);

    switch (dragging) {
	case LEFT_DRAG:
	    switch (m_LastDrag) {
		case drag_COMPASS:
		    // Drag in heading indicator.
		    m_View->SetCompassFromPoint(point);
		    break;
		case drag_ELEV:
		    // Drag in clinometer.
		    m_View->SetClinoFromPoint(point);
		    break;
		case drag_SCALE:
		    m_View->SetScaleBarFromOffset(point.x - m_DragLast.x);
		    break;
		case drag_MAIN:
		    if (event.ControlDown()) {
			HandleTiltRotate(point);
		    } else {
			HandleScaleRotate(point);
		    }
		    break;
		case drag_NONE:
		    // Shouldn't happen?!  FIXME: assert or something.
		    break;
	    }
	    break;
	case MIDDLE_DRAG:
	    HandleTilt(point);
	    break;
	case RIGHT_DRAG:
	    HandleTranslate(point);
	    break;
	case NO_DRAG:
	    break;
    }

    m_DragLast = point;
}

void GUIControl::OnLButtonDown(wxMouseEvent& event)
{
    if (m_View->HasData()) {
	dragging = LEFT_DRAG;

	m_DragStart = m_DragRealStart = wxPoint(event.GetX(), event.GetY());

	if (m_View->PointWithinCompass(m_DragStart)) {
	    m_LastDrag = drag_COMPASS;
	} else if (m_View->PointWithinClino(m_DragStart)) {
	    m_LastDrag = drag_ELEV;
	} else if (m_View->PointWithinScaleBar(m_DragStart)) {
	    m_LastDrag = drag_SCALE;
	} else {
	    if (event.ControlDown()) {
		if (m_View->IsExtendedElevation()) {
		    dragging = NO_DRAG;
		    return;
		}
		m_View->SetCursor(GfxCore::CURSOR_ROTATE_EITHER_WAY);
	    } else {
		m_View->SetCursor(GfxCore::CURSOR_ZOOM_ROTATE);
	    }

	    m_LastDrag = drag_MAIN;
	    m_ScaleRotateLock = NONE;
	}

	m_View->CaptureMouse();
    }
}

void GUIControl::OnLButtonUp(wxMouseEvent& event)
{
    if (m_View->HasData()) {
	if (event.GetPosition() == m_DragRealStart) {
	    // Just a "click"...
	    m_View->CheckHitTestGrid(m_DragStart, true);
	} else if (dragging == NO_DRAG) {
	    return;
	}

//	m_View->RedrawIndicators();
	m_View->ReleaseMouse();

	m_LastDrag = drag_NONE;
	dragging = NO_DRAG;

        m_View->DragFinished();

        RestoreCursor();
    }
}

void GUIControl::OnMButtonDown(wxMouseEvent& event)
{
    if (m_View->HasData() && !m_View->IsExtendedElevation()) {
	dragging = MIDDLE_DRAG;
	m_DragStart = wxPoint(event.GetX(), event.GetY());

	m_View->SetCursor(GfxCore::CURSOR_ROTATE_VERTICALLY);

	m_View->CaptureMouse();
    }
}

void GUIControl::OnMButtonUp(wxMouseEvent&)
{
    if (m_View->HasData()) {
	dragging = NO_DRAG;
	m_View->ReleaseMouse();
        m_View->DragFinished();

        RestoreCursor();
    }
}

void GUIControl::OnRButtonDown(wxMouseEvent& event)
{
    if (m_View->HasData()) {
	m_DragStart = wxPoint(event.GetX(), event.GetY());

	dragging = RIGHT_DRAG;

        m_View->SetCursor(GfxCore::CURSOR_DRAGGING_HAND);
	m_View->CaptureMouse();
    }
}

void GUIControl::OnRButtonUp(wxMouseEvent&)
{
    m_LastDrag = drag_NONE;
    m_View->ReleaseMouse();

    dragging = NO_DRAG;

    RestoreCursor();

    m_View->DragFinished();
}

void GUIControl::OnMouseWheel(wxMouseEvent& event) {
    int dy = event.GetWheelRotation();
    if (m_View->GetPerspective()) {
	m_View->MoveViewer(-dy, 0, 0);
    } else {
	m_View->SetScale(m_View->GetScale() * pow(1.06, -0.04 * dy));
	m_View->ForceRefresh();
    }
}

void GUIControl::OnDisplayOverlappingNames()
{
    m_View->ToggleOverlappingNames();
}

void GUIControl::OnDisplayOverlappingNamesUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && m_View->ShowingStationNames());
    cmd.Check(m_View->ShowingOverlappingNames());
}

void GUIControl::OnColourByDepth()
{
    if (m_View->ColouringBy() == COLOUR_BY_DEPTH) {
	m_View->SetColourBy(COLOUR_BY_NONE);
    } else {
	m_View->SetColourBy(COLOUR_BY_DEPTH);
    }
}

void GUIControl::OnColourByDate()
{
    if (m_View->ColouringBy() == COLOUR_BY_DATE) {
	m_View->SetColourBy(COLOUR_BY_NONE);
    } else {
	m_View->SetColourBy(COLOUR_BY_DATE);
    }
}

void GUIControl::OnColourByDepthUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && m_View->HasUndergroundLegs() && !m_View->IsFlat());
    cmd.Check(m_View->ColouringBy() == COLOUR_BY_DEPTH);
}

void GUIControl::OnColourByDateUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && m_View->HasUndergroundLegs() && m_View->HasRangeOfDates());
    cmd.Check(m_View->ColouringBy() == COLOUR_BY_DATE);
}

void GUIControl::OnShowCrosses()
{
    m_View->ToggleCrosses();
}

void GUIControl::OnShowCrossesUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
    cmd.Check(m_View->ShowingCrosses());
}

void GUIControl::OnShowStationNames()
{
    m_View->ToggleStationNames();
}

void GUIControl::OnShowStationNamesUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
    cmd.Check(m_View->ShowingStationNames());
}

void GUIControl::OnShowSurveyLegs()
{
    m_View->ToggleUndergroundLegs();
}

void GUIControl::OnShowSurveyLegsUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && m_View->HasUndergroundLegs());
    cmd.Check(m_View->ShowingUndergroundLegs());
}

void GUIControl::OnMoveEast()
{
    m_View->TurnCaveTo(90.0);
    m_View->ForceRefresh();
}

void GUIControl::OnMoveEastUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && !m_View->IsExtendedElevation());
}

void GUIControl::OnMoveNorth()
{
    m_View->TurnCaveTo(0.0);
    m_View->ForceRefresh();
}

void GUIControl::OnMoveNorthUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
}

void GUIControl::OnMoveSouth()
{
    m_View->TurnCaveTo(180.0);
    m_View->ForceRefresh();
}

void GUIControl::OnMoveSouthUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
}

void GUIControl::OnMoveWest()
{
    m_View->TurnCaveTo(270.0);
    m_View->ForceRefresh();
}

void GUIControl::OnMoveWestUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && !m_View->IsExtendedElevation());
}

void GUIControl::OnToggleRotation()
{
    m_View->ToggleRotation();
}

void GUIControl::OnToggleRotationUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && !m_View->IsExtendedElevation());
    cmd.Check(m_View->HasData() && m_View->IsRotating());
}

void GUIControl::OnReverseControls()
{
    m_ReverseControls = !m_ReverseControls;
}

void GUIControl::OnReverseControlsUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
    cmd.Check(m_ReverseControls);
}

void GUIControl::OnReverseDirectionOfRotation()
{
    m_View->ReverseRotation();
}

void GUIControl::OnReverseDirectionOfRotationUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && !m_View->IsExtendedElevation());
}

void GUIControl::OnSlowDown(bool accel)
{
    m_View->RotateSlower(accel);
}

void GUIControl::OnSlowDownUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && !m_View->IsExtendedElevation());
}

void GUIControl::OnSpeedUp(bool accel)
{
    m_View->RotateFaster(accel);
}

void GUIControl::OnSpeedUpUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && !m_View->IsExtendedElevation());
}

void GUIControl::OnStepOnceAnticlockwise(bool accel)
{
    if (m_View->GetPerspective()) {
	m_View->TurnCave(accel ? -5.0 * ROTATE_STEP : -ROTATE_STEP);
    } else {
	m_View->TurnCave(accel ? 5.0 * ROTATE_STEP : ROTATE_STEP);
    }
    m_View->ForceRefresh();
}

void GUIControl::OnStepOnceAnticlockwiseUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && !m_View->IsExtendedElevation() && !m_View->IsRotating());
}

void GUIControl::OnStepOnceClockwise(bool accel)
{
    if (m_View->GetPerspective()) {
	m_View->TurnCave(accel ? 5.0 * ROTATE_STEP : ROTATE_STEP);
    } else {
	m_View->TurnCave(accel ? -5.0 * ROTATE_STEP : -ROTATE_STEP);
    }
    m_View->ForceRefresh();
}

void GUIControl::OnStepOnceClockwiseUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && !m_View->IsExtendedElevation() && !m_View->IsRotating());
}

void GUIControl::OnDefaults()
{
    m_View->Defaults();
}

void GUIControl::OnDefaultsUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
}

void GUIControl::OnElevation()
{
    // Switch to elevation view.

    m_View->SwitchToElevation();
}

void GUIControl::OnElevationUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && !m_View->IsExtendedElevation() && !m_View->ShowingElevation());
}

void GUIControl::OnHigherViewpoint(bool accel)
{
    // Raise the viewpoint.
    if (m_View->GetPerspective()) {
	m_View->TiltCave(accel ? -5.0 * ROTATE_STEP : -ROTATE_STEP);
    } else {
	m_View->TiltCave(accel ? 5.0 * ROTATE_STEP : ROTATE_STEP);
    }
    m_View->ForceRefresh();
}

void GUIControl::OnHigherViewpointUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && m_View->CanRaiseViewpoint() && !m_View->IsExtendedElevation());
}

void GUIControl::OnLowerViewpoint(bool accel)
{
    // Lower the viewpoint.
    if (m_View->GetPerspective()) {
	m_View->TiltCave(accel ? 5.0 * ROTATE_STEP : ROTATE_STEP);
    } else {
	m_View->TiltCave(accel ? -5.0 * ROTATE_STEP : -ROTATE_STEP);
    }
    m_View->ForceRefresh();
}

void GUIControl::OnLowerViewpointUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && m_View->CanLowerViewpoint() && !m_View->IsExtendedElevation());
}

void GUIControl::OnPlan()
{
    // Switch to plan view.
    m_View->SwitchToPlan();
}

void GUIControl::OnPlanUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && !m_View->IsExtendedElevation() && !m_View->ShowingPlan());
}

void GUIControl::OnShiftDisplayDown(bool accel)
{
    if (m_View->GetPerspective())
	m_View->MoveViewer(0, accel ? 5 * FLYFREE_SHIFT : FLYFREE_SHIFT, 0);
    else
	m_View->TranslateCave(0, accel ? 5 * DISPLAY_SHIFT : DISPLAY_SHIFT);
}

void GUIControl::OnShiftDisplayDownUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
}

void GUIControl::OnShiftDisplayLeft(bool accel)
{
    if (m_View->GetPerspective())
	m_View->MoveViewer(0, 0, accel ? 5 * FLYFREE_SHIFT : FLYFREE_SHIFT);
    else
	m_View->TranslateCave(accel ? -5 * DISPLAY_SHIFT : -DISPLAY_SHIFT, 0);
}

void GUIControl::OnShiftDisplayLeftUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
}

void GUIControl::OnShiftDisplayRight(bool accel)
{
    if (m_View->GetPerspective())
	m_View->MoveViewer(0, 0, accel ? -5 * FLYFREE_SHIFT : -FLYFREE_SHIFT);
    else
	m_View->TranslateCave(accel ? 5 * DISPLAY_SHIFT : DISPLAY_SHIFT, 0);
}

void GUIControl::OnShiftDisplayRightUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
}

void GUIControl::OnShiftDisplayUp(bool accel)
{
    if (m_View->GetPerspective())
	m_View->MoveViewer(0, accel ? -5 * FLYFREE_SHIFT : -FLYFREE_SHIFT, 0);
    else
	m_View->TranslateCave(0, accel ? -5 * DISPLAY_SHIFT : -DISPLAY_SHIFT);
}

void GUIControl::OnShiftDisplayUpUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
}

void GUIControl::OnZoomIn(bool accel)
{
    // Increase the scale.

    if (m_View->GetPerspective()) {
	m_View->MoveViewer(accel ? 5 * FLYFREE_SHIFT : FLYFREE_SHIFT, 0, 0);
    } else {
	m_View->SetScale(m_View->GetScale() * (accel ? 1.1236 : 1.06));
	m_View->ForceRefresh();
    }
}

void GUIControl::OnZoomInUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
}

void GUIControl::OnZoomOut(bool accel)
{
    // Decrease the scale.

    if (m_View->GetPerspective()) {
	m_View->MoveViewer(accel ? -5 * FLYFREE_SHIFT : -FLYFREE_SHIFT, 0, 0);
    } else {
	m_View->SetScale(m_View->GetScale() / (accel ? 1.1236 : 1.06));
	m_View->ForceRefresh();
    }
}

void GUIControl::OnZoomOutUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
}

void GUIControl::OnToggleScalebar()
{
    m_View->ToggleScaleBar();
}

void GUIControl::OnToggleScalebarUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
    cmd.Check(m_View->ShowingScaleBar());
}

void GUIControl::OnToggleDepthbar() /* FIXME naming */
{
    m_View->ToggleDepthBar();
}

void GUIControl::OnToggleDepthbarUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && m_View->ColouringBy() == COLOUR_BY_DEPTH);
    cmd.Check(m_View->ShowingDepthBar());
}

void GUIControl::OnViewCompass()
{
    m_View->ToggleCompass();
}

void GUIControl::OnViewCompassUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && !m_View->IsExtendedElevation());
    cmd.Check(m_View->ShowingCompass());
}

void GUIControl::OnViewClino()
{
    m_View->ToggleClino();
}

void GUIControl::OnViewClinoUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && !m_View->IsExtendedElevation());
    cmd.Check(m_View->ShowingClino());
}

void GUIControl::OnShowSurface()
{
    m_View->ToggleSurfaceLegs();
}

void GUIControl::OnShowSurfaceUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && m_View->HasSurfaceLegs());
    cmd.Check(m_View->ShowingSurfaceLegs());
}

void GUIControl::OnShowEntrances()
{
    m_View->ToggleEntrances();
}

void GUIControl::OnShowEntrancesUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && (m_View->GetNumEntrances() > 0));
    cmd.Check(m_View->ShowingEntrances());
}

void GUIControl::OnShowFixedPts()
{
    m_View->ToggleFixedPts();
}

void GUIControl::OnShowFixedPtsUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && (m_View->GetNumFixedPts() > 0));
    cmd.Check(m_View->ShowingFixedPts());
}

void GUIControl::OnShowExportedPts()
{
    m_View->ToggleExportedPts();
}

void GUIControl::OnShowExportedPtsUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && (m_View->GetNumExportedPts() > 0));
    cmd.Check(m_View->ShowingExportedPts());
}

void GUIControl::OnViewGrid()
{
    m_View->ToggleGrid();
}

void GUIControl::OnViewGridUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
    cmd.Check(m_View->ShowingGrid());
}

void GUIControl::OnIndicatorsUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
}

void GUIControl::OnViewPerspective()
{
    m_View->TogglePerspective();
    // Force update of coordinate display.
    if (m_View->GetPerspective()) {
	m_View->MoveViewer(0, 0, 0);
    } else {
	m_View->ClearCoords();
    }
}

void GUIControl::OnViewPerspectiveUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && !m_View->IsExtendedElevation());
    cmd.Check(m_View->GetPerspective());
}

void GUIControl::OnViewSmoothShading()
{
    m_View->ToggleSmoothShading();
}

void GUIControl::OnViewSmoothShadingUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
    cmd.Check(m_View->GetSmoothShading());
}

void GUIControl::OnViewTextured()
{
    m_View->ToggleTextured();
}

void GUIControl::OnViewTexturedUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
    cmd.Check(m_View->GetTextured());
}

void GUIControl::OnViewFog()
{
    m_View->ToggleFog();
}

void GUIControl::OnViewFogUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
    cmd.Check(m_View->GetFog());
}

void GUIControl::OnViewSmoothLines()
{
    m_View->ToggleAntiAlias();
}

void GUIControl::OnViewSmoothLinesUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
    cmd.Check(m_View->GetAntiAlias());
}

void GUIControl::OnToggleMetric()
{
    m_View->ToggleMetric();

    wxConfigBase::Get()->Write("metric", m_View->GetMetric());
    wxConfigBase::Get()->Flush();
}

void GUIControl::OnToggleMetricUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
    cmd.Check(m_View->GetMetric());
}

void GUIControl::OnToggleDegrees()
{
    m_View->ToggleDegrees();

    wxConfigBase::Get()->Write("degrees", m_View->GetDegrees());
    wxConfigBase::Get()->Flush();
}

void GUIControl::OnToggleDegreesUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
    cmd.Check(m_View->GetDegrees());
}

void GUIControl::OnToggleTubes()
{
    m_View->ToggleTubes();
}

void GUIControl::OnToggleTubesUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && m_View->HasTubes());
    cmd.Check(m_View->GetTubes());
}

void GUIControl::OnCancelDistLine()
{
    m_View->ClearTreeSelection();
}

void GUIControl::OnCancelDistLineUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->ShowingMeasuringLine());
}

void GUIControl::OnKeyPress(wxKeyEvent &e)
{
    if (!m_View->HasData()) {
	e.Skip();
	return;
    }

    // The changelog says this is meant to keep animation going while keys are
    // pressed, but that happens anyway (on linux at least - perhaps it helps
    // on windows?)  FIXME : check!
    //bool refresh = m_View->Animate();

    switch (e.m_keyCode) {
	case '/': case '?':
	    if (m_View->CanLowerViewpoint() && !m_View->IsExtendedElevation())
		OnLowerViewpoint(e.m_shiftDown);
	    break;
	case '\'': case '@': case '"': // both shifted forms - US and UK kbd
	    if (m_View->CanRaiseViewpoint() && !m_View->IsExtendedElevation())
		OnHigherViewpoint(e.m_shiftDown);
	    break;
	case 'C': case 'c':
	    if (!m_View->IsExtendedElevation() && !m_View->IsRotating())
		OnStepOnceAnticlockwise(e.m_shiftDown);
	    break;
	case 'V': case 'v':
	    if (!m_View->IsExtendedElevation() && !m_View->IsRotating())
		OnStepOnceClockwise(e.m_shiftDown);
	    break;
	case ']': case '}':
	    OnZoomIn(e.m_shiftDown);
	    break;
	case '[': case '{':
	    OnZoomOut(e.m_shiftDown);
	    break;
	case 'N': case 'n':
	    OnMoveNorth();
	    break;
	case 'S': case 's':
	    OnMoveSouth();
	    break;
	case 'E': case 'e':
	    if (!m_View->IsExtendedElevation())
		OnMoveEast();
	    break;
	case 'W': case 'w':
	    if (!m_View->IsExtendedElevation())
		OnMoveWest();
	    break;
	case 'Z': case 'z':
	    if (!m_View->IsExtendedElevation())
		OnSpeedUp(e.m_shiftDown);
	    break;
	case 'X': case 'x':
	    if (!m_View->IsExtendedElevation())
		OnSlowDown(e.m_shiftDown);
	    break;
	case 'R': case 'r':
	    if (!m_View->IsExtendedElevation())
		OnReverseDirectionOfRotation();
	    break;
	case 'P': case 'p':
	    if (!m_View->IsExtendedElevation() && !m_View->ShowingPlan())
		OnPlan();
	    break;
	case 'L': case 'l':
	    if (!m_View->IsExtendedElevation() && !m_View->ShowingElevation())
		OnElevation();
	    break;
	case 'O': case 'o':
	    OnDisplayOverlappingNames();
	    break;
	case WXK_DELETE:
	    OnDefaults();
	    break;
	case WXK_RETURN:
            // For compatibility with older versions.
	    if (!m_View->IsExtendedElevation() && !m_View->IsRotating())
                m_View->StartRotation();
	    break;
	case WXK_SPACE:
	    if (!m_View->IsExtendedElevation())
		OnToggleRotation();
	    break;
	case WXK_LEFT:
	    if (e.m_controlDown) {
		if (!m_View->IsExtendedElevation() && !m_View->IsRotating())
		    OnStepOnceAnticlockwise(e.m_shiftDown);
	    } else {
		OnShiftDisplayLeft(e.m_shiftDown);
	    }
	    break;
	case WXK_RIGHT:
	    if (e.m_controlDown) {
		if (!m_View->IsExtendedElevation() && !m_View->IsRotating())
		    OnStepOnceClockwise(e.m_shiftDown);
	    } else {
		OnShiftDisplayRight(e.m_shiftDown);
	    }
	    break;
	case WXK_UP:
	    if (e.m_controlDown) {
		if (m_View->CanRaiseViewpoint() && !m_View->IsExtendedElevation())
		    OnHigherViewpoint(e.m_shiftDown);
	    } else {
		OnShiftDisplayUp(e.m_shiftDown);
	    }
	    break;
	case WXK_DOWN:
	    if (e.m_controlDown) {
		if (m_View->CanLowerViewpoint() && !m_View->IsExtendedElevation())
		    OnLowerViewpoint(e.m_shiftDown);
	    } else {
		OnShiftDisplayDown(e.m_shiftDown);
	    }
	    break;
	case WXK_ESCAPE:
	    if (m_View->ShowingMeasuringLine()) {
		OnCancelDistLine();
	    }
	    break;
	default:
	    e.Skip();
    }

    //if (refresh) m_View->ForceRefresh();
}

void GUIControl::OnViewFullScreenUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
    cmd.Check(m_View->IsFullScreen());
}

void GUIControl::OnViewFullScreen()
{
    m_View->FullScreenMode();
}

void GUIControl::OnViewBoundingBoxUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
    cmd.Check(m_View->DisplayingBoundingBox());
}

void GUIControl::OnViewBoundingBox()
{
    m_View->ToggleBoundingBox();
}
