//
//  aboutdlg.cxx
//
//  About box handling for Aven.
//
//  Copyright (C) 2001-2002, Mark R. Shinwell.
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

#ifdef AVENGL
#include <GL/gl.h>
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/utsname.h>

BEGIN_EVENT_TABLE(AboutDlg, wxDialog)
END_EVENT_TABLE()

AboutDlg::AboutDlg(wxWindow* parent) :
    wxDialog(parent, 500, wxString::Format(msg(/*About %s*/205), "Aven"))
{
    wxBoxSizer* horiz = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* vert = new wxBoxSizer(wxVERTICAL);

#ifdef AVENGL
    wxStaticText* title = new wxStaticText(this, 502,
                              wxString("Aven "VERSION" (OpenGL enabled)"));
    const GLubyte* gl_vendor = glGetString(GL_VENDOR);
    const GLubyte* gl_renderer = glGetString(GL_RENDERER);
    const GLubyte* gl_version = glGetString(GL_VERSION);
    wxStaticText* vendor = new wxStaticText(this, 520,
                           wxString(msg(/*OpenGL renderer:*/384)) +
                           wxString(" ") + wxString(gl_vendor) +
                           wxString(" ") + wxString(gl_renderer));
    wxStaticText* version = new wxStaticText(this, 521,
                            wxString(msg(/*OpenGL version:*/385)) +
                            wxString(" ") + wxString(gl_version));
#else
    wxStaticText* title = new wxStaticText(this, 502,
                                           wxString("Aven "VERSION));
#endif
    wxStaticText* purpose = new wxStaticText(this, 505,
	wxString(msg(/*Visualisation of Survex 3D files*/209)));
    wxStaticText* copyright1 = new wxStaticText(this, 503,
	    wxString::Format(AVEN_COPYRIGHT_MSG, msg(/*&copy;*/0)));
    wxStaticText* copyright2 = new wxStaticText(this, 504,
	    wxString::Format(COPYRIGHT_MSG, msg(/*&copy;*/0)));

//FIXME windows version
    struct utsname buf;
    int fail = uname(&buf);
    
    wxStaticText* os = new wxStaticText(this, 506,
                       fail ? wxString(msg(/*(unavailable)*/387))
                            : msg(/*Host system type:*/386) + wxString(" ") +
                              wxString(buf.sysname) + wxString(" ") +
                              wxString(buf.release));
    wxStaticText* depth = new wxStaticText(this, 507,
                          msg(/*Colour depth:*/388) + wxString(" ") +
                          wxString::Format("%d-%s", wxDisplayDepth(),
                                           msg(/*bit*/389)));
    
    wxString licence_str;
    wxString l(msg(/*This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public Licence as published by the Free Software Foundation; either version 2 of the Licence, or (at your option) any later version.*/219));
    wxClientDC dc(this);
    dc.SetFont(this->GetFont());
    do {
	unsigned int a = 72;
	if (a >= l.length()) {
	    a = l.length();
	} else {
	    while (a > 1 && l[a] != ' ') --a;
	}

	while (a > 1) {
	    wxCoord w, h;
	    dc.GetTextExtent(l.substr(0, a), &w, &h);
	    if (w <= 380) break;
	    do { --a; } while (a > 1 && l[a] != ' ');
	}

	if (!licence_str.empty()) licence_str += '\n';
	licence_str += l.substr(0, a);
	if (a < l.length() && l[a] == ' ') ++a;
	l = l.substr(a);
    } while (!l.empty());

    wxStaticText* licence = new wxStaticText(this, 506, licence_str);
    wxButton* close = new wxButton(this, wxID_OK, wxString(msg(/*Close*/204)));
    close->SetDefault();

    wxBitmap& bm = wxGetApp().GetAboutBitmap();
    if (bm.Ok()) {
	wxStaticBitmap* bitmap = new wxStaticBitmap(this, 501, bm);
	horiz->Add(bitmap, 0 /* horizontally unstretchable */, wxALL, 2 /* border width */);
    }
    horiz->Add(vert, 0, wxALL, 2);

    vert->Add(10, 5, 0, wxTOP, 5);
    vert->Add(title, 0, wxLEFT | wxRIGHT, 20);
    vert->Add(10, 5, 0, wxTOP, 5);
    vert->Add(purpose, 0, wxLEFT | wxRIGHT, 20);
    vert->Add(10, 5, 0, wxTOP, 5);

    vert->Add(copyright1, 0, wxLEFT | wxRIGHT, 20);
    vert->Add(copyright2, 0, wxLEFT | wxRIGHT, 20);
    vert->Add(10, 5, 0, wxTOP, 5);

    vert->Add(os, 0, wxLEFT | wxRIGHT, 20);
    vert->Add(depth, 0, wxLEFT | wxRIGHT, 20);
#ifdef AVENGL
    vert->Add(vendor, 0, wxLEFT | wxRIGHT, 20);
    vert->Add(version, 0, wxLEFT | wxRIGHT, 20);
#endif
    vert->Add(10, 5, 0, wxTOP, 15);
    
    vert->Add(licence, 0, wxLEFT | wxRIGHT, 20);
    vert->Add(10, 5, 1, wxALIGN_BOTTOM | wxTOP, 5);

    wxBoxSizer* bottom = new wxBoxSizer(wxHORIZONTAL);
    bottom->Add(5, 5, 1);
    bottom->Add(close, 0, wxRIGHT | wxBOTTOM, 15);
    vert->Add(bottom, 0, wxEXPAND | wxLEFT | wxRIGHT, 0);
    vert->SetMinSize(0, bm.GetHeight());

    SetAutoLayout(true);
    SetSizer(horiz);

    horiz->Fit(this);
    horiz->SetSizeHints(this);
}
