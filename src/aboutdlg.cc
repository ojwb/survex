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
    wxDialog(parent, 500, wxString::Format(msg(/*About %s*/205), "Aven"))
{
    wxBoxSizer* horiz = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* vert = new wxBoxSizer(wxVERTICAL);

#ifdef AVENGL
    wxStaticText* title = new wxStaticText(this, 502, wxString("Aven "VERSION" (OpenGL enabled)"));
#else
    wxStaticText* title = new wxStaticText(this, 502, wxString("Aven "VERSION));
#endif
    wxStaticText* purpose = new wxStaticText(this, 505,
        wxString(msg(/*Visualisation of Survex 3D files*/209)));
    wxStaticText* copyright1 = new wxStaticText(this, 503, AVEN_COPYRIGHT_MSG);
    wxStaticText* copyright2 = new wxStaticText(this, 504, COPYRIGHT_MSG);

    wxString licence_str(msg(/*This is free software.  Aven is licenced under the terms of the GNU General Public Licence version 2, or (at your option) any later version.*/200));
    unsigned int wrap = 42;
    while (wrap < licence_str.length()) {
	if (licence_str[wrap] == ' ') {
	   licence_str.SetChar(wrap, '\n');
	   wrap += 42;
	}
	wrap++;  
    }

    wxStaticText* licence = new wxStaticText(this, 506, licence_str);
    wxButton* close = new wxButton(this, wxID_OK, wxString(msg(/*Close*/204)));
    close->SetDefault();

    wxBitmap& bm = wxGetApp().GetAboutBitmap();
    if (bm.Ok()) {
        wxStaticBitmap* bitmap = new wxStaticBitmap(this, 501, bm);
        horiz->Add(bitmap, 0 /* horizontally unstretchable */, wxALL, 2 /* border width */);
    }
    horiz->Add(vert, 0, wxALL, 2);

    vert->Add(title, 0, wxLEFT | wxRIGHT | wxTOP, 20);
    vert->Add(10, 5, 0, wxTOP, 5);
    vert->Add(purpose, 0, wxLEFT | wxRIGHT, 20);
    vert->Add(10, 5, 0, wxTOP, 5);
    vert->Add(copyright1, 0, wxLEFT | wxRIGHT, 20);
    vert->Add(copyright2, 0, wxLEFT | wxBOTTOM | wxRIGHT, 20);
    vert->Add(10, 5, 0, wxTOP, 5);
    vert->Add(licence, 0, wxLEFT | wxRIGHT | wxBOTTOM, 20);
    vert->Add(10, 5, 1, wxALIGN_BOTTOM | wxTOP, 5);

    wxBoxSizer* bottom = new wxBoxSizer(wxHORIZONTAL);
    bottom->Add(5, 5, 1);
    bottom->Add(close, 0, wxRIGHT | wxBOTTOM, 15);
    vert->Add(bottom, 0, wxEXPAND | wxLEFT | wxRIGHT, 0);

    SetAutoLayout(true);
    SetSizer(horiz);

    horiz->Fit(this);
    horiz->SetSizeHints(this);
}
