//
//  indicatorprefs.cc
//
//  Preferences page for indicators.
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

#include "indicatorprefs.h"
#include "message.h"

static const wxWindowID ID_IND_PREFS = 5010;
static const wxWindowID ID_IND_SCALE = 4000;
static const wxWindowID ID_IND_DEPTH = 4001;
static const wxWindowID ID_IND_COMPASS = 4002;
static const wxWindowID ID_IND_CLINO = 4003;

IndicatorPrefs::IndicatorPrefs(wxWindow* parent) : PanelDlgPage(parent, ID_IND_PREFS)
{
    wxCheckBox* scalebar = new wxCheckBox(this, ID_IND_SCALE, msg(/*Display scale bar*/369));
    wxCheckBox* depthbar = new wxCheckBox(this, ID_IND_DEPTH, msg(/*Display depth bar*/370));
    wxCheckBox* compass = new wxCheckBox(this, ID_IND_COMPASS, msg(/*Display compass*/371));
    wxCheckBox* clino = new wxCheckBox(this, ID_IND_CLINO, msg(/*Display clinometer*/372));

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    sizer->Add(scalebar, 0 /* not vertically stretchable */, wxALIGN_TOP | wxBOTTOM, 4);
    sizer->Add(depthbar, 0, wxALIGN_TOP | wxBOTTOM, 4);
    sizer->Add(compass, 0, wxALIGN_TOP | wxBOTTOM, 4);
    sizer->Add(clino, 0, wxALIGN_TOP | wxBOTTOM, 4);

    SetAutoLayout(true);
    SetSizer(sizer);
}

IndicatorPrefs::~IndicatorPrefs()
{

}

const wxString IndicatorPrefs::GetName()
{
    return "Indicators";
}

const wxBitmap IndicatorPrefs::GetIcon()
{
    return GetParent()->LoadPreferencesIcon("indicator");
}

