//
//  unitsprefs.cc
//
//  Preferences page for measurement unit options.
//
//  Copyright (C) 2002 Mark R. Shinwell
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

#include "unitsprefs.h"
#include "message.h"

#include <wx/statline.h>

static const wxWindowID ID_UNITS_PREFS = 5005;
static const wxWindowID ID_UNITS_METRIC = 2000;
static const wxWindowID ID_UNITS_IMPERIAL = 2001;
static const wxWindowID ID_UNITS_DEGREES = 2002;
static const wxWindowID ID_UNITS_GRADS = 2003;
static const wxWindowID ID_UNITS_LINE1 = 2004;
static const wxWindowID ID_UNITS_LABEL1 = 2005;
static const wxWindowID ID_UNITS_LABEL2 = 2006;

UnitsPrefs::UnitsPrefs(wxWindow* parent) : PanelDlgPage(parent, ID_UNITS_PREFS)
{
    wxRadioButton* metric = new wxRadioButton(this, ID_UNITS_METRIC, msg(/*metric units*/362),
                                              wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    wxRadioButton* imperial = new wxRadioButton(this, ID_UNITS_IMPERIAL, msg(/*imperial units*/363));
    wxRadioButton* degrees = new wxRadioButton(this, ID_UNITS_DEGREES, msg(/*degrees*/364),
                                               wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    wxRadioButton* grads = new wxRadioButton(this, ID_UNITS_GRADS, msg(/*grads*/365));

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    sizer->Add(new wxStaticText(this, ID_UNITS_LABEL1, msg(/*Display measurements in*/366)),
               0 /* not vertically stretchable */, wxALIGN_TOP | wxBOTTOM, 4);
    sizer->Add(metric, 0, wxALIGN_TOP | wxLEFT, 32);
    sizer->Add(10, 4);
    sizer->Add(imperial, 0, wxALIGN_TOP | wxLEFT, 32);
    sizer->Add(10, 8);
    sizer->Add(new wxStaticLine(this, ID_UNITS_LINE1), 0, wxEXPAND | wxRIGHT, 16);
    sizer->Add(10, 8);
    sizer->Add(new wxStaticText(this, ID_UNITS_LABEL2, msg(/*Display angles in*/367)),
               0 /* not vertically stretchable */, wxALIGN_TOP | wxBOTTOM, 4);
    sizer->Add(degrees, 0, wxALIGN_TOP | wxLEFT, 32);
    sizer->Add(10, 4);
    sizer->Add(grads, 0, wxALIGN_TOP | wxLEFT, 32);

    SetAutoLayout(true);
    SetSizer(sizer);
}

UnitsPrefs::~UnitsPrefs()
{

}

const wxString UnitsPrefs::GetName()
{
    return "Units";
}

const wxBitmap UnitsPrefs::GetIcon()
{
    return wxGetApp().LoadPreferencesIcon("units");
}

