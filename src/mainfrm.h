//
//  mainfrm.h
//
//  Main frame handling for Aven.
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

#ifndef mainfrm_h
#define mainfrm_h

#include "wx.h"
#include "gfxcore.h"
#include <list>

class PointInfo {
    friend class MainFrm;
    float x, y, z;
    bool isLine; // false => move, true => draw line
};

class LabelInfo {
    friend class MainFrm;
    float x, y, z;
    wxString text;
};

class MainFrm : public wxFrame {
    list<PointInfo*>* m_Points;
    list<LabelInfo*> m_Labels;
    float m_XExt;
    float m_YExt;
    float m_ZExt;
    float m_ZMin;
    int m_NumLegs;
    int m_NumPoints;
    int m_NumCrosses;
    int m_NumExtraLegs;
    GfxCore* m_Gfx;
    wxPen* m_Pens;

    void ClearPointLists();
    bool LoadData(const wxString& file);
    void SortIntoDepthBands(list<PointInfo*>& points);
    void IntersectLineWithPlane(float x0, float y0, float z0,
				float x1, float y1, float z1,
				float z, float& x, float& y);
    float GetDepthBoundaryBetweenBands(int a, int b);
    int GetDepthColour(float z);
    void CentreDataset(float xmin, float ymin, float zmin);

public:
    MainFrm(const wxString& title, const wxPoint& pos, const wxSize& size);
    ~MainFrm();

    void OnOpen(wxCommandEvent& event);
    void OnQuit(wxCommandEvent& event);

    float GetXExtent() { return m_XExt; }
    float GetYExtent() { return m_YExt; }
    float GetZExtent() { return m_ZExt; }
    float GetZMin()    { return m_ZMin; }

    int GetNumLegs()   { return m_NumLegs; }
    int GetNumPoints() { return m_NumPoints; }
    int GetNumCrosses() { return m_NumCrosses; }

    int GetNumDepthBands();

    wxPen GetPen(int band);

    list<PointInfo*>::iterator GetFirstPoint(int band);
    bool GetNextPoint(int band, list<PointInfo*>::iterator& pos, float& x, float& y, float& z,
		      bool& isLine);

    list<LabelInfo*>::iterator GetFirstLabel();
    bool GetNextLabel(list<LabelInfo*>::iterator& pos, float& x, float& y, float& z, wxString& text);

private:
    DECLARE_EVENT_TABLE();
};

enum {
    menu_FILE_OPEN,
    menu_FILE_QUIT,
    menu_ROTATION_START,
    menu_ROTATION_STOP,
    menu_ROTATION_SPEED_UP,
    menu_ROTATION_SLOW_DOWN,
    menu_ROTATION_REVERSE,
    menu_ROTATION_STEP_CCW,
    menu_ROTATION_STEP_CW,
    menu_ORIENT_MOVE_NORTH,
    menu_ORIENT_MOVE_EAST,
    menu_ORIENT_MOVE_SOUTH,
    menu_ORIENT_MOVE_WEST,
    menu_ORIENT_SHIFT_LEFT,
    menu_ORIENT_SHIFT_RIGHT,
    menu_ORIENT_SHIFT_UP,
    menu_ORIENT_SHIFT_DOWN,
    menu_ORIENT_PLAN,
    menu_ORIENT_ELEVATION,
    menu_ORIENT_HIGHER_VP,
    menu_ORIENT_LOWER_VP,
    menu_ORIENT_ZOOM_IN,
    menu_ORIENT_ZOOM_OUT,
    menu_ORIENT_DEFAULTS
};

#endif
