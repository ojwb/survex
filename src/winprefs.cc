//
//  winprefs.cc
//
//  Preferences page for window-related options.
//
//  Copyright (C) 2002 Mark R. Shinwell
//  Copyright (C) 2004 Olly Betts
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

#include "winprefs.h"
#include "message.h"
#include "mainfrm.h"

static const wxWindowID ID_WIN_PREFS = 5011;
static const wxWindowID ID_WIN_SIDE_PANEL = 4000;

BEGIN_EVENT_TABLE(WinPrefs, PanelDlgPage)
    EVT_CHECKBOX(ID_WIN_SIDE_PANEL, WinPrefs::OnSidePanel)
    EVT_UPDATE_UI(ID_WIN_SIDE_PANEL, WinPrefs::OnSidePanelUpdate)
END_EVENT_TABLE()

WinPrefs::WinPrefs(MainFrm* parent, wxWindow* parent_win) : PanelDlgPage(parent_win, ID_WIN_PREFS), m_Parent(parent)
{
    wxCheckBox* side_panel = new wxCheckBox(this, ID_WIN_SIDE_PANEL, msg(/*Display side panel*/373));

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    sizer->Add(side_panel, 0 /* not vertically stretchable */, wxALIGN_TOP);

    SetAutoLayout(true);
    SetSizer(sizer);
}

WinPrefs::~WinPrefs()
{

}

const wxString WinPrefs::GetName()
{
    return "Windows";
}

const wxBitmap WinPrefs::GetIcon()
{
    return GetParent()->LoadPreferencesIcon("window");
}

void WinPrefs::OnSidePanel(wxCommandEvent&)
{
    m_Parent->ToggleSidePanel();
}

void WinPrefs::OnSidePanelUpdate(wxUpdateUIEvent& ui)
{
    ui.Check(m_Parent->ShowingSidePanel());
}

