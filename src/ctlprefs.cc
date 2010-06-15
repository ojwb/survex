//
//  ctlprefs.cc
//
//  Preferences page for mouse control options.
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
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ctlprefs.h"
#include "message.h"

#include <wx/statline.h>

static const wxWindowID ID_CTL_PREFS = 5004;
static const wxWindowID ID_CTL_REVERSE = 2000;

CtlPrefs::CtlPrefs(wxWindow* parent) : PanelDlgPage(parent, ID_CTL_PREFS)
{
    wxCheckBox* reverse = new wxCheckBox(this, ID_CTL_REVERSE, msg(/*Reverse the sense of the controls*/368));

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    sizer->Add(reverse, 0 /* not vertically stretchable */, wxALIGN_TOP | wxBOTTOM, 0);

    SetAutoLayout(true);
    SetSizer(sizer);
}

CtlPrefs::~CtlPrefs()
{

}

const wxString CtlPrefs::GetName()
{
    return "Control";
}

const wxBitmap CtlPrefs::GetIcon()
{
    return GetParent()->LoadPreferencesIcon("ctl");
}

