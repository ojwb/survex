//
//  aboutdlg.cc
//
//  About box handling for Aven.
//
//  Copyright (C) 2001-2003 Mark R. Shinwell.
//  Copyright (C) 2001,2002,2003,2004,2005,2006,2010 Olly Betts
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

#include "aboutdlg.h"
#include "aven.h"
#include "gla.h"
#include "message.h"

#include <wx/confbase.h>
#include <wx/image.h>

BEGIN_EVENT_TABLE(AboutDlg, wxDialog)
    EVT_TIMER(about_TIMER, AboutDlg::OnTimer)
END_EVENT_TABLE()

void
AboutDlg::OnTimer(wxTimerEvent &e)
{
    wxImage::AddHandler(new wxJPEGHandler);
    bitmap.LoadFile(icon_path + "osterei.jpg", wxBITMAP_TYPE_JPEG);
    ((wxStaticBitmap*)FindWindowById(501, this))->SetBitmap(bitmap);
}

AboutDlg::AboutDlg(wxWindow* parent, const wxString & icon_path_) :
    wxDialog(parent, 500, wxString::Format(msg(/*About %s*/205), APP_NAME)),
    icon_path(icon_path_), timer(this, about_TIMER)
{
    wxBoxSizer* horiz = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* vert = new wxBoxSizer(wxVERTICAL);

    if (!bitmap.Ok()) {
	bitmap.LoadFile(icon_path + APP_ABOUT_IMAGE, wxBITMAP_TYPE_PNG);
	bitmap_icon.LoadFile(icon_path + APP_IMAGE, wxBITMAP_TYPE_PNG);
    }
    if (bitmap.Ok()) {
	wxStaticBitmap* static_bitmap = new wxStaticBitmap(this, 501, bitmap);
	horiz->Add(static_bitmap, 0 /* horizontally unstretchable */, wxALL,
		   2 /* border width */);
    }
    horiz->Add(vert, 0, wxALL, 2);

    wxString id(APP_NAME" "VERSION"\n");
    id += msg(/*Survey visualisation tool*/209);
    wxBoxSizer* title = new wxBoxSizer(wxHORIZONTAL);
    if (bitmap_icon.Ok()) {
	title->Add(new wxStaticBitmap(this, 599, bitmap_icon), 0, wxALIGN_CENTRE_VERTICAL|wxRIGHT, 8);
    }
    title->Add(new wxStaticText(this, 502, id), 0, wxALL, 2);

    wxString copyright_msg(COPYRIGHT_MSG"\n"AVEN_COPYRIGHT_MSG);
    const char * csign = msg(/*&copy;*/0);
    if (strcmp(csign, "(C)") != 0) {
	size_t csign_len = strlen(csign);
	size_t i = 0;
	while ((i = copyright_msg.find("(C)", i)) != wxString::npos) {
	    copyright_msg.replace(i, 3, csign, csign_len);
	    i += csign_len;
	}
    }
    wxStaticText* copyright = new wxStaticText(this, 503, copyright_msg);

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

    wxStaticText* licence = new wxStaticText(this, 504, licence_str);
    wxButton* ok = new wxButton(this, wxID_OK, wxGetTranslation("OK"));
    ok->SetDefault();

    vert->Add(10, 5, 0, wxTOP, 5);
    vert->Add(title, 0, wxLEFT | wxRIGHT, 20);
    vert->Add(10, 5, 0, wxTOP, 5);

    vert->Add(copyright, 0, wxLEFT | wxRIGHT, 20);
    vert->Add(10, 5, 0, wxTOP, 5);

    vert->Add(new wxStaticText(this, 505, msg(/*System Information:*/390)),
	      0, wxLEFT | wxRIGHT, 20);

    wxString info(wxGetOsDescription());
    info += "\n" wxVERSION_STRING;
#ifdef __WXGTK__
# if defined __WXGTK26__
	" (GTK+ >= 2.6)\n";
# elif defined __WXGTK24__
	" (GTK+ >= 2.4)\n";
# elif defined __WXGTK20__
	" (GTK+ >= 2.0)\n";
# elif defined __WXGTK12__
	" (GTK+ >= 1.2)\n";
# else
	" (GTK+ < 1.2)\n";
# endif
#elif defined __WXMOTIF__
# if defined __WXMOTIF20__
	" (Motif >= 2.0)\n";
# else
	" (Motif < 2.0)\n";
# endif
#elif defined __WXX11__
	" (X11)\n";
#else
	"\n";
#endif
    int bpp = wxDisplayDepth();
    info += wxString::Format("Display Depth: %d bpp", bpp);
    if (wxColourDisplay()) info += " (colour)";
    info += '\n';
    info += GetGLSystemDescription();

    // Use a readonly multiline text edit for the system info so users can
    // easily cut and paste it into an email when reporting bugs.
    vert->Add(new wxTextCtrl(this, 506, info, wxDefaultPosition,
			     wxSize(360, 64), wxTE_MULTILINE|wxTE_READONLY),
	      0, wxLEFT | wxRIGHT, 20);

    vert->Add(10, 5, 0, wxTOP, 15);

    vert->Add(licence, 0, wxLEFT | wxRIGHT, 20);
    vert->Add(10, 5, 1, wxALIGN_BOTTOM | wxTOP, 5);

    wxBoxSizer* bottom = new wxBoxSizer(wxHORIZONTAL);
    bottom->Add(5, 5, 1);
    bottom->Add(ok, 0, wxRIGHT | wxBOTTOM, 15);
    vert->Add(bottom, 0, wxEXPAND | wxLEFT | wxRIGHT, 0);
    vert->SetMinSize(0, bitmap.GetHeight());

    SetSizer(horiz);

    horiz->Fit(this);
    horiz->SetSizeHints(this);

    timer.Start(42000);
}
