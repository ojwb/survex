//
//  guicontrol.cc
//
//  Handlers for events relating to the display of a survey.
//
//  Copyright (C) 2000-2002 Mark R. Shinwell
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "guicontrol.h"
#include "gfxcore.h"
#include <wx/confbase.h>

static const int DISPLAY_SHIFT = 50;

GUIControl::GUIControl()
{
    m_View = NULL;
    m_DraggingLeft = false;
    m_DraggingMiddle = false;
    m_DraggingRight = false;
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

bool GUIControl::MouseDown()
{
    return m_DraggingLeft || m_DraggingMiddle || m_DraggingRight;
}

void GUIControl::HandleTilt(wxPoint point)
{
    // Handle a mouse movement during tilt mode.
    int dy = point.y - m_DragStart.y;

    if (m_ReverseControls) dy = -dy;

    m_View->TiltCave(Double(-dy) * M_PI / 500.0);

    m_DragStart = point;

    m_View->ForceRefresh();
}

void GUIControl::HandleTranslate(wxPoint point)
{
    // Handle a mouse movement during translation mode.
    int dx = point.x - m_DragStart.x;
    int dy = point.y - m_DragStart.y;
    
    if (m_ReverseControls) {
	dx = -dx;
	dy = -dy;
    }

    m_View->TranslateCave(dx, dy);
    m_DragStart = point;
}

void GUIControl::HandleScale(wxPoint point)
{
    // Handle a mouse movement during scale mode.

    int dx = point.x - m_DragStart.x;
    int dy = point.y - m_DragStart.y;

    if (m_ReverseControls) {
	dx = -dx;
	dy = -dy;
    }

    m_View->SetScale(m_View->GetScale() * pow(1.06, 0.08 * dy));
    m_View->ForceRefresh();

    m_DragStart = point;
}

void GUIControl::HandleTiltRotate(wxPoint point)
{
    // Handle a mouse movement during tilt/rotate mode.

    int dx = point.x - m_DragStart.x;
    int dy = point.y - m_DragStart.y;

    if (m_ReverseControls) {
	dx = -dx;
	dy = -dy;
    }

    // left/right => rotate, up/down => tilt
    m_View->TurnCave(m_View->CanRotate() ?
                     (Double(dx) * (M_PI / -500.0)) : 0.0);
    m_View->TiltCave(Double(-dy) * M_PI / 1000.0);

    m_View->ForceRefresh();

    m_DragStart = point;
}

void GUIControl::HandleScaleRotate(wxPoint point)
{
    // Handle a mouse movement during scale/rotate mode.
    int dx = point.x - m_DragStart.x;
    int dy = point.y - m_DragStart.y;

    if (m_ReverseControls) {
	dx = -dx;
	dy = -dy;
    }

    Double pan_angle = m_View->CanRotate() ? (Double(dx) * (-M_PI / 500.0)) : 0.0;

    // left/right => rotate, up/down => scale
    m_View->TurnCave(pan_angle);

    m_View->SetScale(m_View->GetScale() * pow(1.06, 0.08 * dy));

#ifdef AVENGL
    //glDeleteLists(m_Lists.grid, 1);
    //    DrawGrid();
#endif
    m_View->ForceRefresh();

    m_DragStart = point;
}

void GUIControl::HandCursor()
{
    const wxCursor HAND_CURSOR(wxCURSOR_HAND);
    m_View->SetCursor(HAND_CURSOR);
}

void GUIControl::RestoreCursor()
{
    if (m_View->ShowingMeasuringLine()) {
        HandCursor();
    }
    else {
        m_View->SetCursor(wxNullCursor);
    }
}

//
//  Mouse event handling methods
//

void GUIControl::OnMouseMove(wxMouseEvent& event)
{
    // Mouse motion event handler.
    if (!m_View->HasData()) return;

    wxPoint point = wxPoint(event.GetX(), event.GetY());

    // Check hit-test grid (only if no buttons are pressed).
    if (!event.LeftIsDown() && !event.MiddleIsDown() && !event.RightIsDown()) {
	if (m_View->CheckHitTestGrid(point, false)) {
            HandCursor();
        }
        else {            
            if (m_View->ShowingScaleBar() &&
                m_View->PointWithinScaleBar(point)) {

                const wxCursor CURSOR(wxCURSOR_SIZEWE);
                m_View->SetCursor(CURSOR);
            }
            else {
                m_View->SetCursor(wxNullCursor);
            }
        }
    }

    // Update coordinate display if in plan view,
    // or altitude if in elevation view.
    m_View->SetCoords(point);

    if (!m_View->ChangingOrientation()) {
	if (m_DraggingLeft) {
	    if (m_LastDrag == drag_NONE) {
		if (m_View->ShowingCompass() &&
                    m_View->PointWithinCompass(point)) {
		    m_LastDrag = drag_COMPASS;
		}
		else if (m_View->ShowingClino() &&
                         m_View->PointWithinClino(point)) {
		    m_LastDrag = drag_ELEV;
		}
		else if (m_View->ShowingScaleBar() &&
                         m_View->PointWithinScaleBar(point)) {
		    m_LastDrag = drag_SCALE;
		}
	    }

	    if (m_LastDrag == drag_COMPASS) {
		// drag in heading indicator
		m_View->SetCompassFromPoint(point);
	    }
	    else if (m_LastDrag == drag_ELEV) {
		// drag in clinometer
		m_View->SetClinoFromPoint(point);
	    }
	    else if (m_LastDrag == drag_SCALE) {
		// FIXME: check why there was a check here for x being inside
		// the window
	        m_View->SetScaleBarFromOffset(point.x - m_DragLast.x);
	    }
	    else if (m_LastDrag == drag_NONE || m_LastDrag == drag_MAIN) {
		m_LastDrag = drag_MAIN;
                if (event.ShiftDown()) {
		    HandleScaleRotate(point);
                }
                else {
		    HandleTiltRotate(point);
                }
	    }
	}
	else if (m_DraggingMiddle) {
            if (event.ShiftDown()) {
	        HandleTilt(point);
            }
            else {
		HandleScale(point);
            }
	}
	else if (m_DraggingRight) {
	    if ((m_LastDrag == drag_NONE && m_View->PointWithinScaleBar(point)) || m_LastDrag == drag_SCALE) {
	    /* FIXME
		  if (point.x < 0) point.x = 0;
		  if (point.y < 0) point.y = 0;
		  if (point.x > m_XSize) point.x = m_XSize;
		  if (point.y > m_YSize) point.y = m_YSize;
		  m_LastDrag = drag_SCALE;
		  int x_inside_bar = m_DragStart.x - m_ScaleBar.drag_start_offset_x;
		  int y_inside_bar = m_YSize - m_ScaleBar.drag_start_offset_y - m_DragStart.y;
		  m_ScaleBar.offset_x = point.x - x_inside_bar;
		  m_ScaleBar.offset_y = (m_YSize - point.y) - y_inside_bar;
		  m_View->ForceRefresh(); */
	    }
	    else {
		m_LastDrag = drag_MAIN;
		HandleTranslate(point);
	    }
	}
    }

    m_DragLast = point;
}

void GUIControl::OnLButtonDown(wxMouseEvent& event)
{
    if (m_View->HasData() && m_View->GetLock() != lock_POINT) {
	m_DraggingLeft = true;
	
	/* FIXME
	m_ScaleBar.drag_start_offset_x = m_ScaleBar.offset_x;
	m_ScaleBar.drag_start_offset_y = m_ScaleBar.offset_y; */

	m_DragStart = m_DragRealStart = wxPoint(event.GetX(), event.GetY());
        
//        const wxCursor CURSOR(wxCURSOR_MAGNIFIER);
//        m_View->SetCursor(CURSOR);
	m_View->CaptureMouse();
    }
}

void GUIControl::OnLButtonUp(wxMouseEvent& event)
{
    if (m_View->HasData() && m_View->GetLock() != lock_POINT) {
	if (event.GetPosition() == m_DragRealStart) {
	    // just a "click"...
	    m_View->CheckHitTestGrid(m_DragStart, true);
	}

//	m_View->RedrawIndicators();
	m_View->ReleaseMouse();

	m_LastDrag = drag_NONE;
	m_DraggingLeft = false;

        m_View->DragFinished();
        
        RestoreCursor();
    }
}

void GUIControl::OnMButtonDown(wxMouseEvent& event)
{
    if (m_View->HasData() && m_View->GetLock() == lock_NONE) {
	m_DraggingMiddle = true;
	m_DragStart = wxPoint(event.GetX(), event.GetY());

        const wxCursor CURSOR(wxCURSOR_MAGNIFIER);
        m_View->SetCursor(CURSOR);
	m_View->CaptureMouse();
    }
}

void GUIControl::OnMButtonUp(wxMouseEvent& event)
{
    if (m_View->HasData() && m_View->GetLock() == lock_NONE) {
	m_DraggingMiddle = false;
	m_View->ReleaseMouse();
        m_View->DragFinished();

        RestoreCursor();
    }
}

void GUIControl::OnRButtonDown(wxMouseEvent& event)
{
    if (m_View->HasData()) {
	m_DragStart = wxPoint(event.GetX(), event.GetY());
	
/* FIXME	m_ScaleBar.drag_start_offset_x = m_ScaleBar.offset_x;
	m_ScaleBar.drag_start_offset_y = m_ScaleBar.offset_y; */

	m_DraggingRight = true;

      //  const wxCursor CURSOR(wxCURSOR_HAND);
      //  m_View->SetCursor(CURSOR);
	m_View->CaptureMouse();
    }
}

void GUIControl::OnRButtonUp(wxMouseEvent& event)
{
    m_LastDrag = drag_NONE;
    m_View->ReleaseMouse();

    m_DraggingRight = false;
    
    RestoreCursor();
    
    m_View->DragFinished();
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
    cmd.Enable(m_View->HasData() && m_View->GetLock() != lock_POINT && m_View->HasUndergroundLegs());
    cmd.Check(m_View->ShowingUndergroundLegs());
}

void GUIControl::OnMoveEast()
{
    m_View->TurnCaveTo(M_PI_2);
    m_View->ForceRefresh();
}

void GUIControl::OnMoveEastUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && !(m_View->GetLock() & lock_Y));
}

void GUIControl::OnMoveNorth()
{
    m_View->TurnCaveTo(0.0);
    m_View->ForceRefresh();
}

void GUIControl::OnMoveNorthUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && !(m_View->GetLock() & lock_X));
}

void GUIControl::OnMoveSouth()
{
    m_View->TurnCaveTo(M_PI);
    m_View->ForceRefresh();
}

void GUIControl::OnMoveSouthUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && !(m_View->GetLock() & lock_X));
}

void GUIControl::OnMoveWest()
{
    m_View->TurnCaveTo(M_PI * 1.5);
    m_View->ForceRefresh();
}

void GUIControl::OnMoveWestUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && !(m_View->GetLock() & lock_Y));
}

void GUIControl::OnStartRotation()
{
    m_View->StartRotation();
}

void GUIControl::OnStartRotationUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && !m_View->IsRotating() && m_View->CanRotate());
}

void GUIControl::OnToggleRotation()
{
    m_View->ToggleRotation();
}

void GUIControl::OnToggleRotationUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && m_View->CanRotate());
    cmd.Check(m_View->HasData() && m_View->IsRotating());
}

void GUIControl::OnStopRotation()
{
    m_View->StopRotation();
}

void GUIControl::OnStopRotationUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && m_View->IsRotating());
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
    cmd.Enable(m_View->HasData() && m_View->CanRotate());
}

void GUIControl::OnSlowDown(bool accel)
{
    m_View->RotateSlower(accel);
}

void GUIControl::OnSlowDownUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && m_View->CanRotate());
}

void GUIControl::OnSpeedUp(bool accel)
{
    m_View->RotateFaster(accel);
}

void GUIControl::OnSpeedUpUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && m_View->CanRotate());
}

void GUIControl::OnStepOnceAnticlockwise(bool accel)
{
    m_View->TurnCave(accel ? 5.0 * M_PI / 18.0 : M_PI / 18.0);
    m_View->ForceRefresh();
}

void GUIControl::OnStepOnceAnticlockwiseUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && m_View->CanRotate() && !m_View->IsRotating());
}

void GUIControl::OnStepOnceClockwise(bool accel)
{
    m_View->TurnCave(accel ? 5.0 * -M_PI / 18.0 : -M_PI / 18.0);
    m_View->ForceRefresh();
}

void GUIControl::OnStepOnceClockwiseUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && m_View->CanRotate() && !m_View->IsRotating());
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
    cmd.Enable(m_View->HasData() && m_View->GetLock() == lock_NONE && !m_View->ShowingElevation());
}

void GUIControl::OnHigherViewpoint(bool accel)
{
    // Raise the viewpoint.
    m_View->TiltCave(accel ? 5.0 * M_PI / 18.0 : M_PI / 18.0);
    m_View->ForceRefresh();
}

void GUIControl::OnHigherViewpointUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && m_View->CanRaiseViewpoint() && m_View->GetLock() == lock_NONE);
}

void GUIControl::OnLowerViewpoint(bool accel)
{
    // Lower the viewpoint.
    m_View->TiltCave(accel ? 5.0 * -M_PI / 18.0 : -M_PI / 18.0);
    m_View->ForceRefresh(); /* FIXME check if necessary */
}

void GUIControl::OnLowerViewpointUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && m_View->CanLowerViewpoint() && m_View->GetLock() == lock_NONE);
}

void GUIControl::OnPlan()
{
    // Switch to plan view.
    m_View->SwitchToPlan();
}

void GUIControl::OnPlanUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && m_View->GetLock() == lock_NONE && !m_View->ShowingPlan());
}

void GUIControl::OnShiftDisplayDown(bool accel)
{
    m_View->TranslateCave(0, accel ? 5 * DISPLAY_SHIFT : DISPLAY_SHIFT);
}

void GUIControl::OnShiftDisplayDownUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
}

void GUIControl::OnShiftDisplayLeft(bool accel)
{
    m_View->TranslateCave(accel ? -5 * DISPLAY_SHIFT : -DISPLAY_SHIFT, 0);
}

void GUIControl::OnShiftDisplayLeftUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
}

void GUIControl::OnShiftDisplayRight(bool accel)
{
    m_View->TranslateCave(accel ? 5 * DISPLAY_SHIFT : DISPLAY_SHIFT, 0);
}

void GUIControl::OnShiftDisplayRightUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
}

void GUIControl::OnShiftDisplayUp(bool accel)
{
    m_View->TranslateCave(0, accel ? -5 * DISPLAY_SHIFT : -DISPLAY_SHIFT);
}

void GUIControl::OnShiftDisplayUpUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
}

void GUIControl::OnZoomIn(bool accel)
{
    // Increase the scale.

    m_View->SetScale(m_View->GetScale() * (accel ? 1.1236 : 1.06));
    m_View->ForceRefresh();
}

void GUIControl::OnZoomInUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && m_View->GetLock() != lock_POINT);
}

void GUIControl::OnZoomOut(bool accel)
{
    // Decrease the scale.

    m_View->SetScale(m_View->GetScale() / (accel ? 1.1236 : 1.06));
    m_View->ForceRefresh();
}

void GUIControl::OnZoomOutUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && m_View->GetLock() != lock_POINT);
}

void GUIControl::OnToggleScalebar()
{
    m_View->ToggleScaleBar();
}

void GUIControl::OnToggleScalebarUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && m_View->GetLock() != lock_POINT);
    cmd.Check(m_View->ShowingScaleBar());
}

void GUIControl::OnToggleDepthbar() /* FIXME naming */
{
    m_View->ToggleDepthBar();
}

void GUIControl::OnToggleDepthbarUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && !(m_View->GetLock() && lock_Z));
    cmd.Check(m_View->ShowingDepthBar());
}

void GUIControl::OnViewCompass()
{
    m_View->ToggleCompass();
}

void GUIControl::OnViewCompassUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && m_View->CanRotate());
    cmd.Check(m_View->ShowingCompass());
}

void GUIControl::OnViewClino()
{
    m_View->ToggleClino();
}

void GUIControl::OnViewClinoUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData() && m_View->GetLock() == lock_NONE);
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
}

void GUIControl::OnIndicatorsUpdate(wxUpdateUIEvent& cmd)
{
    cmd.Enable(m_View->HasData());
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
    cmd.Enable(m_View->HasData());
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

/*     m_RedrawOffscreen = Animate();  FIXME */

    switch (e.m_keyCode) {
	case '/': case '?':
	    if (m_View->CanLowerViewpoint() && m_View->GetLock() == lock_NONE)
		OnLowerViewpoint(e.m_shiftDown);
	    break;
	case '\'': case '@': case '"': // both shifted forms - US and UK kbd
	    if (m_View->CanRaiseViewpoint() && m_View->GetLock() == lock_NONE)
		OnHigherViewpoint(e.m_shiftDown);
	    break;
	case 'C': case 'c':
	    if (m_View->CanRotate() && !m_View->IsRotating())
		OnStepOnceAnticlockwise(e.m_shiftDown);
	    break;
	case 'V': case 'v':
	    if (m_View->CanRotate() && !m_View->IsRotating())
		OnStepOnceClockwise(e.m_shiftDown);
	    break;
	case ']': case '}':
	    if (m_View->GetLock() != lock_POINT)
		OnZoomIn(e.m_shiftDown);
	    break;
	case '[': case '{':
	    if (m_View->GetLock() != lock_POINT)
		OnZoomOut(e.m_shiftDown);
	    break;
	case 'N': case 'n':
	    if (!(m_View->GetLock() & lock_X))
		OnMoveNorth();
	    break;
	case 'S': case 's':
	    if (!(m_View->GetLock() & lock_X))
		OnMoveSouth();
	    break;
	case 'E': case 'e':
	    if (!(m_View->GetLock() & lock_Y))
		OnMoveEast();
	    break;
	case 'W': case 'w':
	    if (!(m_View->GetLock() & lock_Y))
		OnMoveWest();
	    break;
	case 'Z': case 'z':
	    if (m_View->CanRotate())
		OnSpeedUp(e.m_shiftDown);
	    break;
	case 'X': case 'x':
	    if (m_View->CanRotate())
		OnSlowDown(e.m_shiftDown);
	    break;
	case 'R': case 'r':
	    if (m_View->CanRotate())
		OnReverseDirectionOfRotation();
	    break;
	case 'P': case 'p':
	    if (m_View->GetLock() == lock_NONE && !m_View->ShowingPlan())
		OnPlan();
	    break;
	case 'L': case 'l':
	    if (m_View->GetLock() == lock_NONE && !m_View->ShowingElevation())
		OnElevation();
	    break;
	case 'O': case 'o':
	    OnDisplayOverlappingNames();
	    break;
	case WXK_DELETE:
	    OnDefaults();
	    break;
	case WXK_RETURN:
	    if (m_View->CanRotate() && !m_View->IsRotating())
		OnStartRotation();
	    break;
	case WXK_SPACE:
	    if (m_View->IsRotating())
		OnStopRotation();
	    break;
	case WXK_LEFT:
	    if (e.m_controlDown) {
		if (m_View->CanRotate() && !m_View->IsRotating())
		    OnStepOnceAnticlockwise(e.m_shiftDown);
	    } else {
		OnShiftDisplayLeft(e.m_shiftDown);
	    }
	    break;
	case WXK_RIGHT:
	    if (e.m_controlDown) {
		if (m_View->CanRotate() && !m_View->IsRotating())
		    OnStepOnceClockwise(e.m_shiftDown);
	    } else {
		OnShiftDisplayRight(e.m_shiftDown);
	    }
	    break;
	case WXK_UP:
	    if (e.m_controlDown) {
		if (m_View->CanRaiseViewpoint() && m_View->GetLock() == lock_NONE)
		    OnHigherViewpoint(e.m_shiftDown);
	    } else {
		OnShiftDisplayUp(e.m_shiftDown);
	    }
	    break;
	case WXK_DOWN:
	    if (e.m_controlDown) {
		if (m_View->CanLowerViewpoint() && m_View->GetLock() == lock_NONE)
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
 
    // OnPaint clears m_RedrawOffscreen so it'll only still be set if we need
    // to redraw
/* FIXME    if (m_RedrawOffscreen) */ m_View->ForceRefresh();
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
