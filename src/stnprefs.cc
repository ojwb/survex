//
//  stnprefs.cc
//
//  Preferences page for stations.
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

#include "stnprefs.h"
#include "message.h"
#include "gfxcore.h"

#include <wx/statline.h>

static const wxWindowID ID_STN_PREFS = 1001;
static const wxWindowID ID_STN_CROSSES = 2000;
static const wxWindowID ID_STN_HI_ENTS = 2001;
static const wxWindowID ID_STN_HI_FIXED = 2002;
static const wxWindowID ID_STN_HI_XPTS = 2003;
static const wxWindowID ID_STN_NAMES = 2004;
static const wxWindowID ID_STN_OVERLAPPING = 2005;
static const wxWindowID ID_STN_LINE1 = 2006;
static const wxWindowID ID_STN_LINE2 = 2006;

BEGIN_EVENT_TABLE(StnPrefs, PanelDlgPage)
    EVT_CHECKBOX(ID_STN_CROSSES, StnPrefs::OnShowCrosses)
    EVT_CHECKBOX(ID_STN_HI_ENTS, StnPrefs::OnHighlightEntrances)
    EVT_CHECKBOX(ID_STN_HI_FIXED, StnPrefs::OnHighlightFixedPts)
    EVT_CHECKBOX(ID_STN_HI_XPTS, StnPrefs::OnHighlightExportedPts)
    EVT_CHECKBOX(ID_STN_NAMES, StnPrefs::OnNames)
    EVT_CHECKBOX(ID_STN_OVERLAPPING, StnPrefs::OnOverlappingNames)

    EVT_UPDATE_UI(ID_STN_CROSSES, StnPrefs::OnShowCrossesUpdate)
    EVT_UPDATE_UI(ID_STN_HI_ENTS, StnPrefs::OnHighlightEntrancesUpdate)
    EVT_UPDATE_UI(ID_STN_HI_FIXED, StnPrefs::OnHighlightFixedPtsUpdate)
    EVT_UPDATE_UI(ID_STN_HI_XPTS, StnPrefs::OnHighlightExportedPtsUpdate)
    EVT_UPDATE_UI(ID_STN_NAMES, StnPrefs::OnNamesUpdate)
    EVT_UPDATE_UI(ID_STN_OVERLAPPING, StnPrefs::OnOverlappingNamesUpdate)
END_EVENT_TABLE()

StnPrefs::StnPrefs(GfxCore* parent, wxWindow* parent_win) : PanelDlgPage(parent_win, ID_STN_PREFS), m_Parent(parent)
{
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
}

StnPrefs::~StnPrefs()
{

}

void StnPrefs::OnShowCrosses(wxCommandEvent&)
{
    m_Parent->ToggleCrosses();
}

void StnPrefs::OnHighlightEntrances(wxCommandEvent&)
{
    m_Parent->ToggleEntrances();
}

void StnPrefs::OnHighlightFixedPts(wxCommandEvent&)
{
    m_Parent->ToggleFixedPts();
}

void StnPrefs::OnHighlightExportedPts(wxCommandEvent&)
{
    m_Parent->ToggleExportedPts();
}

void StnPrefs::OnNames(wxCommandEvent&)
{
    m_Parent->ToggleStationNames();
}

void StnPrefs::OnOverlappingNames(wxCommandEvent&)
{
    m_Parent->ToggleOverlappingNames();
}

void StnPrefs::OnShowCrossesUpdate(wxUpdateUIEvent& ui)
{
    ui.Check(m_Parent->ShowingCrosses());
}

void StnPrefs::OnHighlightEntrancesUpdate(wxUpdateUIEvent& ui)
{
    ui.Check(m_Parent->ShowingEntrances());
}

void StnPrefs::OnHighlightFixedPtsUpdate(wxUpdateUIEvent& ui)
{
    ui.Check(m_Parent->ShowingFixedPts());
}

void StnPrefs::OnHighlightExportedPtsUpdate(wxUpdateUIEvent& ui)
{
    ui.Check(m_Parent->ShowingExportedPts());
}

void StnPrefs::OnNamesUpdate(wxUpdateUIEvent& ui)
{
    ui.Check(m_Parent->ShowingStationNames());
}

void StnPrefs::OnOverlappingNamesUpdate(wxUpdateUIEvent& ui)
{
    ui.Check(m_Parent->ShowingOverlappingNames());
}

const wxString StnPrefs::GetName()
{
    return "Stations";
}

const wxBitmap StnPrefs::GetIcon()
{
    return wxGetApp().LoadPreferencesIcon("stations");
}

