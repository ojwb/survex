//
//  legprefs.cc
//
//  Preferences page for survey legs.
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

#include "legprefs.h"
#include "message.h"
#include <wx/statline.h>

static const wxWindowID ID_LEG_PREFS = 1003;
static const wxWindowID ID_LEG_UG_LEGS = 2000;
static const wxWindowID ID_LEG_SURF_LEGS = 2001;
static const wxWindowID ID_LEG_LINE = 2002;

LegPrefs::LegPrefs(wxWindow* parent) : PanelDlgPage(parent, ID_LEG_PREFS)
{
    wxCheckBox* ug_legs = new wxCheckBox(this, ID_LEG_UG_LEGS, msg(/*Display underground survey legs*/357));
    wxCheckBox* surf_legs = new wxCheckBox(this, ID_LEG_SURF_LEGS, msg(/*Display surface survey legs*/358));

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    sizer->Add(ug_legs, 0 /* not vertically stretchable */, wxALIGN_TOP | wxBOTTOM, 0);
    sizer->Add(10, 8);
    sizer->Add(new wxStaticLine(this, ID_LEG_LINE), 0, wxEXPAND | wxRIGHT, 16);
    sizer->Add(10, 8);
    sizer->Add(surf_legs, 0 /* not vertically stretchable */, wxALIGN_TOP | wxBOTTOM, 4);
    sizer->Add(col_surface, 0, wxALIGN_TOP | wxLEFT, 32);
    sizer->Add(10, 4);
    sizer->Add(dashed_surface, 0, wxALIGN_TOP | wxLEFT, 32);

    SetAutoLayout(true);
    SetSizer(sizer);
}

LegPrefs::~LegPrefs()
{

}

const wxString LegPrefs::GetName()
{
    return "Legs";
}

const wxBitmap LegPrefs::GetIcon()
{
    return GetParent()->LoadPreferencesIcon("legs");
}

