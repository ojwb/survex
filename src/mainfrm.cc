//
//  mainfrm.cc
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

#include "mainfrm.h"
#include "aven.h"
#include "aboutdlg.h"

#include "message.h"
#include "img.h"

#include <float.h>

BEGIN_EVENT_TABLE(MainFrm, wxDocParentFrame)
    EVT_MENU(menu_HELP_ABOUT, MainFrm::OnAbout)
END_EVENT_TABLE()

MainFrm::MainFrm(wxDocManager* manager, wxFrame* parent, wxWindowID id, const wxString& title) :
    wxDocParentFrame(manager, parent, id, title, wxPoint(50, 50), wxSize(640, 480))
{
    BuildMenuBar();
}

MainFrm::~MainFrm()
{
}

//
//  Construction of menus
//

wxMenu* MainFrm::BuildFileMenu()
{
    wxMenu* menu = new wxMenu;

    menu->Append(wxID_OPEN, wxGetApp().GetTabMsg(/*@Open...##Ctrl+O*/220), "");
    menu->AppendSeparator();
    menu->Append(wxID_EXIT, wxGetApp().GetTabMsg(/*@Exit*/221), "");

    return menu;
}

wxMenu* MainFrm::BuildHelpMenu()
{
    wxMenu* menu = new wxMenu;

    menu->Append(menu_HELP_ABOUT, wxGetApp().GetTabMsg(/*@About Aven...*/290), "");

    return menu;
}

void MainFrm::BuildMenuBar()
{
    wxMenuBar* menubar = new wxMenuBar(wxMB_DOCKABLE);

    menubar->Append(BuildFileMenu(), wxGetApp().GetTabMsg(/*@File*/210));
    menubar->Append(BuildHelpMenu(), wxGetApp().GetTabMsg(/*@Help*/215));

    SetMenuBar(menubar);
}

void MainFrm::OnAbout(wxCommandEvent& event)
{
    wxGetApp().OnAbout(this);
}
