//
//  ctlprefs.cc
//
//  Preferences page for mouse control options.
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

#include "ctlprefs.h"
#include "message.h"

#include <wx/statline.h>

static const wxWindowID ID_CTL_PREFS = 1004;

CtlPrefs::CtlPrefs(wxWindow* parent) : PanelDlgPage(parent, ID_CTL_PREFS)
{
#if 0
    wxCheckBox* show_crosses = new wxCheckBox(this, ID_STN_CROSSES, msg(/*Mark survey stations with crosses*/350));
    wxCheckBox* hi_ents = new wxCheckBox(this, ID_STN_HI_ENTS, msg(/*Highlight stations marked as entrances*/351));
    wxCheckBox* hi_fixed = new wxCheckBox(this, ID_STN_HI_FIXED,
                                          msg(/*Highlight stations marked as fixed points*/352));
    wxCheckBox* hi_xpts = new wxCheckBox(this, ID_STN_HI_XPTS, msg(/*Highlight stations which are exported*/353));
    wxCheckBox* names = new wxCheckBox(this, ID_STN_NAMES, msg(/*Mark survey stations with their names*/354));
    wxCheckBox* overlapping = new wxCheckBox(this, ID_STN_OVERLAPPING,
                                             msg(/*Allow names to overlap on the display (faster)*/355));

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    sizer->Add(show_crosses, 0 /* not vertically stretchable */, wxALIGN_TOP | wxBOTTOM, 0);
    sizer->Add(10, 8);
    sizer->Add(new wxStaticLine(this, ID_STN_LINE1), 0, wxEXPAND | wxRIGHT, 16);
    sizer->Add(10, 8);
    sizer->Add(hi_ents, 0, wxALIGN_TOP | wxBOTTOM, 4);
    sizer->Add(hi_fixed, 0, wxALIGN_TOP | wxBOTTOM, 4);
    sizer->Add(hi_xpts, 0, wxALIGN_TOP | wxBOTTOM, 0);
    sizer->Add(10, 8);
    sizer->Add(new wxStaticLine(this, ID_STN_LINE2), 0, wxEXPAND | wxRIGHT, 16);
    sizer->Add(10, 8);
    sizer->Add(names, 0, wxALIGN_TOP | wxBOTTOM, 4);
    sizer->Add(overlapping, 0, wxALIGN_TOP | wxLEFT, 32);

    SetAutoLayout(true);
    SetSizer(sizer);
#endif
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
    return wxGetApp().LoadPreferencesIcon("ctl");
}

