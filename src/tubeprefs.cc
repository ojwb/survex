//
//  tubeprefs.cc
//
//  Preferences page for tubes.
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

#include "tubeprefs.h"
#include "message.h"

static const wxWindowID ID_TUBE_PREFS = 1001;
static const wxWindowID ID_SHOW_TUBES = 5000;
static const wxWindowID ID_ESTIMATE_LRUD = 5001;

TubePrefs::TubePrefs(wxWindow* parent) : PanelDlgPage(parent, ID_TUBE_PREFS)
{
    wxCheckBox* show_tubes = new wxCheckBox(this, ID_SHOW_TUBES, msg(/*Draw passage walls*/348));
    wxCheckBox* estimate_lrud = new wxCheckBox(this, ID_ESTIMATE_LRUD,
                                               msg(/*Estimate LRUD readings based on heuristics*/349));

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    sizer->Add(show_tubes, 0 /* not vertically stretchable */, wxALIGN_TOP | wxBOTTOM, 4);
    sizer->Add(estimate_lrud, 0 /* not vertically stretchable */, wxALIGN_TOP | wxLEFT, 32);

    SetAutoLayout(true);
    SetSizer(sizer);

    Hide();
}

TubePrefs::~TubePrefs()
{

}

const wxString TubePrefs::GetName()
{
    return "Tubes";
}

const wxBitmap TubePrefs::GetIcon()
{
    return GetParent()->LoadPreferencesIcon("tubes");
}

