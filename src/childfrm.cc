//
//  childfrm.cc
//
//  Child (view) frame handling for Aven.
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

#include "childfrm.h"
#include "aven.h"
#include "avendefs.h"
#include "gfxcore.h"

#include "message.h"
#include "img.h"

#include <float.h>

BEGIN_EVENT_TABLE(ChildFrm, wxDocChildFrame)
    EVT_MENU_RANGE(aven_COMMAND_START, aven_COMMAND_END, ChildFrm::OnCommand)
    EVT_UPDATE_UI_RANGE(aven_COMMAND_START, aven_COMMAND_END, ChildFrm::OnUpdateUI)
END_EVENT_TABLE()

ChildFrm::ChildFrm(wxDocument* doc, wxView* view, wxDocParentFrame* parent, wxWindowID id,
		   const wxString& title) :
    wxDocChildFrame(doc, view, parent, id, title, wxDefaultPosition, wxSize(640, 480)),
    m_Gfx((AvenDoc*) doc, this)
{
    BuildMenuBar();
    SetAccelerators();

    // Create the drawing area.
    int width;
    int height;
    parent->GetClientSize(&width, &height);
    m_GfxInited = false;
}

ChildFrm::~ChildFrm()
{
}

void ChildFrm::InitialiseGfx()
{
    m_Gfx.Initialise();
    m_GfxInited = true;
}

//
//  Construction of menus and accelerator tables
//

wxMenu* ChildFrm::BuildFileMenu()
{
    wxMenu* menu = new wxMenu;

    menu->Append(wxID_OPEN, wxGetApp().GetTabMsg(/*@Open...##Ctrl+O*/220), "");
    menu->AppendSeparator();
    menu->Append(wxID_EXIT, wxGetApp().GetTabMsg(/*@Exit*/221), "");

    return menu;
}

wxMenu* ChildFrm::BuildRotationMenu()
{
    wxMenu* menu = new wxMenu;

    menu->Append(menu_ROTATION_START, wxGetApp().GetTabMsg(/*@Start Rotation##Return*/230), "");
    menu->Append(menu_ROTATION_STOP, wxGetApp().GetTabMsg(/*S@top Rotation##Space*/231), "");
    menu->AppendSeparator();
    menu->Append(menu_ROTATION_SPEED_UP, wxGetApp().GetTabMsg(/*Speed @Up##Z*/232), "");
    menu->Append(menu_ROTATION_SLOW_DOWN, wxGetApp().GetTabMsg(/*S@low Down##X*/233), "");
    menu->AppendSeparator();
    menu->Append(menu_ROTATION_REVERSE, wxGetApp().GetTabMsg(/*@Reverse Direction##R*/234), "");
    menu->AppendSeparator();
    menu->Append(menu_ROTATION_STEP_CCW, wxGetApp().GetTabMsg(/*Step Once @Anticlockwise##C*/235), "");
    menu->Append(menu_ROTATION_STEP_CW, wxGetApp().GetTabMsg(/*Step Once @Clockwise##V*/236), "");

    return menu;
}

wxMenu* ChildFrm::BuildOrientationMenu()
{
    wxMenu* menu = new wxMenu;

    menu->Append(menu_ORIENT_MOVE_NORTH, wxGetApp().GetTabMsg(/*View @North##N*/240), "");
    menu->Append(menu_ORIENT_MOVE_EAST, wxGetApp().GetTabMsg(/*View @East##E*/241), "");
    menu->Append(menu_ORIENT_MOVE_SOUTH, wxGetApp().GetTabMsg(/*View @South##S*/242), "");
    menu->Append(menu_ORIENT_MOVE_WEST, wxGetApp().GetTabMsg(/*View @West##W*/243), "");
    menu->AppendSeparator();
    menu->Append(menu_ORIENT_SHIFT_LEFT, wxGetApp().GetTabMsg(/*Shift Survey @Left##Left Arrow*/244), "");
    menu->Append(menu_ORIENT_SHIFT_RIGHT, wxGetApp().GetTabMsg(/*Shift Survey @Right##Right Arrow*/245), "");
    menu->Append(menu_ORIENT_SHIFT_UP, wxGetApp().GetTabMsg(/*Shift Survey @Up##Up Arrow*/246), "");
    menu->Append(menu_ORIENT_SHIFT_DOWN, wxGetApp().GetTabMsg(/*Shift Survey @Down##Down Arrow*/247), "");
    menu->AppendSeparator();
    menu->Append(menu_ORIENT_PLAN, wxGetApp().GetTabMsg(/*@Plan View##P*/248), "");
    menu->Append(menu_ORIENT_ELEVATION, wxGetApp().GetTabMsg(/*Ele@vation View##L*/249), "");
    menu->AppendSeparator();
    menu->Append(menu_ORIENT_HIGHER_VP, wxGetApp().GetTabMsg(/*@Higher Viewpoint##'*/250), "");
    menu->Append(menu_ORIENT_LOWER_VP, wxGetApp().GetTabMsg(/*Lo@wer Viewpoint##/*/251), "");
    menu->AppendSeparator();
    menu->Append(menu_ORIENT_ZOOM_IN, wxGetApp().GetTabMsg(/*@Zoom In##]*/252), "");
    menu->Append(menu_ORIENT_ZOOM_OUT, wxGetApp().GetTabMsg(/*Zoo@m Out##[*/253), "");
    menu->AppendSeparator();
    menu->Append(menu_ORIENT_DEFAULTS, wxGetApp().GetTabMsg(/*Restore De@fault Settings##Delete*/254), "");

    return menu;
}

wxMenu* ChildFrm::BuildViewMenu()
{
    wxMenu* menu = new wxMenu;

    menu->Append(menu_VIEW_SHOW_NAMES, wxGetApp().GetTabMsg(/*Station @Names##Ctrl+N*/270), "", true);
    menu->Append(menu_VIEW_SHOW_CROSSES, wxGetApp().GetTabMsg(/*@Crosses##Ctrl+X*/271), "", true);
    menu->AppendSeparator();
    menu->Append(menu_VIEW_SHOW_LEGS, wxGetApp().GetTabMsg(/*Underground Survey @Legs##Ctrl+L*/272), "", true);
    menu->Append(menu_VIEW_SHOW_SURFACE, wxGetApp().GetTabMsg(/*Sur@face Survey Legs##Ctrl+F*/291), "", true);
    menu->AppendSeparator();
    menu->Append(menu_VIEW_SURFACE_DEPTH, wxGetApp().GetTabMsg(/*Depth Colours on Surface Surveys*/292), "", true);
    menu->Append(menu_VIEW_SURFACE_DASHED, wxGetApp().GetTabMsg(/*Dashed Surface Surveys*/293), "", true);
    menu->AppendSeparator();
    menu->Append(menu_VIEW_SHOW_OVERLAPPING_NAMES, wxGetApp().GetTabMsg(/*@Overlapping Names##O*/273), "", true);
    menu->AppendSeparator();
    menu->Append(menu_VIEW_COMPASS, wxGetApp().GetTabMsg(/*Co@mpass*/274), "", true);
    menu->Append(menu_VIEW_CLINO, wxGetApp().GetTabMsg(/*Cl@inometer*/275), "", true);
    menu->Append(menu_VIEW_DEPTH_BAR, wxGetApp().GetTabMsg(/*@Depth Bar*/276), "", true);
    menu->Append(menu_VIEW_SCALE_BAR, wxGetApp().GetTabMsg(/*Sc@ale Bar*/277), "", true);

    return menu;
}

wxMenu* ChildFrm::BuildControlsMenu()
{
    wxMenu* menu = new wxMenu;

    menu->Append(menu_CTL_REVERSE, wxGetApp().GetTabMsg(/*@Reverse Sense##Ctrl+R*/280), "", true);

    return menu;
}

wxMenu* ChildFrm::BuildHelpMenu()
{
    wxMenu* menu = new wxMenu;

    menu->Append(menu_HELP_ABOUT, wxGetApp().GetTabMsg(/*@About Aven...*/290), "");

    return menu;
}

void ChildFrm::BuildMenuBar()
{
    wxMenuBar* menubar = new wxMenuBar(wxMB_DOCKABLE);

    menubar->Append(BuildFileMenu(), wxGetApp().GetTabMsg(/*@File*/210));
    menubar->Append(BuildRotationMenu(), wxGetApp().GetTabMsg(/*@Rotation*/211));
    menubar->Append(BuildOrientationMenu(), wxGetApp().GetTabMsg(/*@Orientation*/212));
    menubar->Append(BuildViewMenu(), wxGetApp().GetTabMsg(/*@View*/213));
    menubar->Append(BuildControlsMenu(), wxGetApp().GetTabMsg(/*@Controls*/214));
    menubar->Append(BuildHelpMenu(), wxGetApp().GetTabMsg(/*@Help*/215));

    SetMenuBar(menubar);
}

void ChildFrm::SetAccelerators()
{
    wxAcceleratorEntry entries[11];
    entries[0].Set(wxACCEL_NORMAL, WXK_DELETE, menu_ORIENT_DEFAULTS);
    entries[1].Set(wxACCEL_NORMAL, WXK_UP, menu_ORIENT_SHIFT_UP);
    entries[2].Set(wxACCEL_NORMAL, WXK_DOWN, menu_ORIENT_SHIFT_DOWN);
    entries[3].Set(wxACCEL_NORMAL, WXK_LEFT, menu_ORIENT_SHIFT_LEFT);
    entries[4].Set(wxACCEL_NORMAL, WXK_RIGHT, menu_ORIENT_SHIFT_RIGHT);
    entries[5].Set(wxACCEL_NORMAL, (int) '\'', menu_ORIENT_HIGHER_VP);
    entries[6].Set(wxACCEL_NORMAL, (int) '/', menu_ORIENT_LOWER_VP);
    entries[7].Set(wxACCEL_NORMAL, (int) ']', menu_ORIENT_ZOOM_IN);
    entries[8].Set(wxACCEL_NORMAL, (int) '[', menu_ORIENT_ZOOM_OUT);
    entries[9].Set(wxACCEL_NORMAL, WXK_RETURN, menu_ROTATION_START);
    entries[10].Set(wxACCEL_NORMAL, WXK_SPACE, menu_ROTATION_STOP);

    wxAcceleratorTable accel(11, entries);
    SetAcceleratorTable(accel);
}

void ChildFrm::OnCommand(wxCommandEvent& event)
{
    m_Gfx.ProcessEvent(event);
}

void ChildFrm::OnUpdateUI(wxUpdateUIEvent& event)
{
  //    m_Gfx.ProcessEvent(event);
}
