//
//  aboutdlg.cxx
//
//  About box handling for Aven.
//
//  Copyright (C) 2001, Mark R. Shinwell.
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

#include "aboutdlg.h"
#include "aven.h"
#include "message.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

BEGIN_EVENT_TABLE(AboutDlg, wxDialog)
END_EVENT_TABLE()

AboutDlg::AboutDlg(wxWindow* parent) :
    wxDialog(parent, 500, wxString::Format("%s Aven", msg(512) /* About */))
{
    wxBoxSizer* horiz = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* vert = new wxBoxSizer(wxVERTICAL);

    wxStaticText* title = new wxStaticText(this, 502, wxString("Aven ") + wxString(VERSION));
    wxStaticText* purpose = new wxStaticText(this, 505, wxString(msg(509)));
    wxStaticText* copyright1 = new wxStaticText(this, 503, AVEN_COPYRIGHT_MSG);
    wxStaticText* copyright2 = new wxStaticText(this, 504, COPYRIGHT_MSG);
    wxString licence_str(msg(510)); /* This is free software... */
    licence_str.Replace("#", "\n");
    wxStaticText* licence = new wxStaticText(this, 506, licence_str);
    wxButton* close = new wxButton(this, wxID_OK, wxString(msg(511)) /* Close */);
    close->SetDefault();

    wxBitmap& bm = wxGetApp().GetAboutBitmap();
    if (bm.Ok()) {
        wxStaticBitmap* bitmap = new wxStaticBitmap(this, 501, bm);
        horiz->Add(bitmap, 0, wxALL, 2);
    }
    horiz->Add(vert, 1, wxEXPAND | wxALL, 2);

    vert->Add(title, 0, wxLEFT | wxRIGHT | wxTOP, 20);
    vert->Add(10, 5, 0, wxTOP, 5);
    vert->Add(purpose, 0, wxLEFT | wxRIGHT, 20);
    vert->Add(10, 5, 0, wxTOP, 5);
    vert->Add(copyright1, 0, wxLEFT | wxRIGHT, 20);
    vert->Add(copyright2, 0, wxLEFT | wxBOTTOM | wxRIGHT, 20);
    vert->Add(10, 5, 0, wxTOP, 5);
    vert->Add(licence, 0, wxLEFT | wxRIGHT | wxBOTTOM, 20);
    vert->Add(10, 5, 1, wxEXPAND | wxGROW | wxTOP, 5);

    wxBoxSizer* bottom = new wxBoxSizer(wxHORIZONTAL);
    bottom->Add(5, 5, 1);
    bottom->Add(close, 0, wxRIGHT | wxBOTTOM, 15);
    vert->Add(bottom, 0, wxEXPAND | wxLEFT | wxRIGHT, 0);

    horiz->Fit(this);
    horiz->SetSizeHints(this);

    SetAutoLayout(true);
    SetSizer(horiz);
}
