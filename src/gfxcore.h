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

class MainFrm;

class GfxCore : public wxWindow {
    struct params {
        Quaternion rotation;
        double scale;
        struct {
	    float x;
	    float y;
	    float z;
	} translation;
        struct {
	    int x;
	    int y;
	} display_shift;
    } m_Params;

    enum LabelFlags {
        label_NOT_PLOTTED,
	label_PLOTTED,
	label_CHECK_AGAIN
    };


    struct {
        wxPoint* vertices;
        int* num_segs;
    } m_CrossData;

    struct PlotData {
        wxPoint* vertices;
        int* num_segs;
    };

    bool* m_LabelGrid;
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
    int m_Bands;
    double m_InitialScale;
    bool m_FreeRotMode;
    double m_TiltAngle;
    double m_PanAngle;
    bool m_CaverotMouse;
    bool m_Rotating;
    float m_RotationStep;
    bool m_ReverseControls;
    //UINT m_Timer;
    bool m_SwitchingToPlan;
    bool m_SwitchingToElevation;
    bool m_ScaleCrossesOnly;
    bool m_Crosses;
    bool m_Legs;
    bool m_Names;
    bool m_Scalebar;
    //CFont m_Font;
    bool m_Depthbar;
    bool m_OverlappingNames;
    bool m_LabelCacheNotInvalidated;
    LabelFlags* m_LabelsLastPlotted;
    wxRect m_LabelCacheExtend;
    bool m_Compass;
    int m_XSize;
    int m_YSize;
    int m_XCentre;
    int m_YCentre;
    wxPoint m_LabelShift;
    wxTimer m_Timer;
    PlotData* m_PlotData;
    wxString* m_Labels;

    struct pens {
        wxPen black;
        wxPen turquoise;
        wxPen green;
        wxPen white;
        wxPen grey;
    } m_Pens;

    struct brushes {
        wxBrush black;
        wxBrush grey;
        wxBrush white;
    } m_Brushes;

    float XToScreen(float x, float y, float z) {
        return float(x*m_RotationMatrix.get(0, 0) + y*m_RotationMatrix.get(0, 1) +
		     z*m_RotationMatrix.get(0, 2));
    }

    float YToScreen(float x, float y, float z) {
        return float(x*m_RotationMatrix.get(1, 0) + y*m_RotationMatrix.get(1, 1) +
		     z*m_RotationMatrix.get(1, 2));
    }

    float ZToScreen(float x, float y, float z) {
        return float(x*m_RotationMatrix.get(2, 0) + y*m_RotationMatrix.get(2, 1) +
		     z*m_RotationMatrix.get(2, 2));
    }

    wxPoint CompassPtToScreen(float x, float y, float z);

    void SetScale(double scale);
    void RedrawOffscreen();
    void TryToFreeArrays();
    void FirstShow();
    void Defaults();

    void HandleScaleRotate(bool, wxPoint);
    void HandleTilt(wxPoint);
    void HandleTranslate(wxPoint);

    void TiltCave(double tilt_angle);
    void TurnCave(double angle);
    
    void Plan();
    void Elevation();

    void StartTimer();
    void StopTimer();
    
    //void DrawScalebar();
    //void DrawDepthbar();
    //void DrawNames();
    //void NattyDrawNames();
    //void SimpleDrawNames();
    //void DrawCompass();

public:
    GfxCore(MainFrm* parent);
    ~GfxCore();

    void Initialise();

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

private:
    DECLARE_EVENT_TABLE();
};

#endif
