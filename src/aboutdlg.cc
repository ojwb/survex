//
//  aboutdlg.cc
//
//  About box handling for Aven.
//
//  Copyright (C) 2001-2003 Mark R. Shinwell.
//  Copyright (C) 2001,2002,2003,2004,2005,2006,2010,2014,2015,2017 Olly Betts
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
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

#include <config.h>

#include "aboutdlg.h"
#include "aven.h"
#include "gla.h"
#include "message.h"

#include <wx/clipbrd.h>
#include <wx/confbase.h>

BEGIN_EVENT_TABLE(AboutDlg, wxDialog)
    EVT_TIMER(about_TIMER, AboutDlg::OnTimer)
    EVT_BUTTON(wxID_COPY, AboutDlg::OnCopy)
END_EVENT_TABLE()

void
AboutDlg::OnTimer(wxTimerEvent &)
{
    bitmap.LoadFile(img_path + wxT("osterei.png"), wxBITMAP_TYPE_PNG);
    ((wxStaticBitmap*)FindWindowById(501, this))->SetBitmap(bitmap);
}

void
AboutDlg::OnCopy(wxCommandEvent &)
{
    if (wxTheClipboard->Open()) {
	wxTheClipboard->SetData(new wxTextDataObject(info));
	wxTheClipboard->Close();
	// (Try to) make the selection persist after aven exits.
	(void)wxTheClipboard->Flush();
    }
}

AboutDlg::AboutDlg(wxWindow* parent, const wxIcon & app_icon) :
    /* TRANSLATORS: for the title of the About box */
    wxDialog(parent, 500, wxString::Format(wmsg(/*About %s*/205), APP_NAME)),
    timer(this, about_TIMER)
{
    img_path = wxString(wmsg_cfgpth());
    img_path += wxCONFIG_PATH_SEPARATOR;
    img_path += wxT("images");
    img_path += wxCONFIG_PATH_SEPARATOR;

    wxBoxSizer* horiz = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* vert = new wxBoxSizer(wxVERTICAL);

    if (!bitmap.Ok()) {
	bitmap.LoadFile(img_path + APP_ABOUT_IMAGE, wxBITMAP_TYPE_PNG);
    }
    if (bitmap.Ok()) {
	wxStaticBitmap* static_bitmap = new wxStaticBitmap(this, 501, bitmap);
	horiz->Add(static_bitmap, 0 /* horizontally unstretchable */, wxALL,
		   2 /* border width */);
    }
    horiz->Add(vert, 0, wxALL, 2);

    wxString id(APP_NAME wxT(" " VERSION "\n"));
    /* TRANSLATORS: Here "survey" is a "cave map" rather than list of questions
     * - it should be translated to the terminology that cavers using the
     * language would use.
     *
     * This string is used in the about box (summarising the purpose of aven).
     */
    id += wmsg(/*Survey visualisation tool*/209);
    wxBoxSizer* title = new wxBoxSizer(wxHORIZONTAL);
    wxStaticBitmap* static_bitmap = new wxStaticBitmap(this, 599, wxBitmap());
    static_bitmap->SetIcon(app_icon);
    title->Add(static_bitmap, 0, wxALIGN_CENTRE_VERTICAL|wxRIGHT, 8);
    title->Add(new wxStaticText(this, 502, id), 0, wxALL, 2);

    wxStaticText* copyright = new wxStaticText(this, 503,
					wxT(COPYRIGHT_MSG_UTF8 "\n" AVEN_COPYRIGHT_MSG_UTF8));

    wxString licence_str;
    /* TRANSLATORS: Summary paragraph for the GPLv2 - there are translations for
     * some languages here:
     * https://www.gnu.org/licenses/old-licenses/gpl-2.0-translations.html */
    wxString l(wmsg(/*This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public Licence as published by the Free Software Foundation; either version 2 of the Licence, or (at your option) any later version.*/219));
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
	l.erase(0, a);
    } while (!l.empty());

    wxStaticText* licence = new wxStaticText(this, 504, licence_str);

    vert->Add(10, 5, 0, wxTOP, 5);
    vert->Add(title, 0, wxLEFT | wxRIGHT, 20);
    vert->Add(10, 5, 0, wxTOP, 5);

    vert->Add(copyright, 0, wxLEFT | wxRIGHT, 20);
    vert->Add(10, 5, 0, wxTOP, 5);

    vert->Add(licence, 0, wxLEFT | wxRIGHT, 20);
    vert->Add(10, 5, 0, wxTOP, 5);

    // TRANSLATORS: for about box:
    vert->Add(new wxStaticText(this, 505, wmsg(/*System Information:*/390)),
	      0, wxLEFT | wxRIGHT, 20);

    info = wxGetOsDescription();
    info += wxT("\n");
    wxString version = wxGetLibraryVersionInfo().GetVersionString();
    info += version;
    if (version != wxVERSION_STRING)
	info += wxT(" (built with ") wxVERSION_STRING wxT(")");
    info +=
#ifdef __WXGTK__
# if defined __WXGTK3__
	wxT(" (GTK+ 3)\n");
# elif defined __WXGTK20__
	wxT(" (GTK+ 2)\n");
# elif defined __WXGTK12__
	wxT(" (GTK+ 1.2)\n");
# else
	wxT(" (GTK+ < 1.2)\n");
# endif
#elif defined __WXMOTIF__
# if defined __WXMOTIF20__
	wxT(" (Motif >= 2.0)\n");
# else
	wxT(" (Motif < 2.0)\n");
# endif
#elif defined __WXX11__
	wxT(" (X11)\n");
#else
	wxT("\n");
#endif
    int bpp = wxDisplayDepth();
    /* TRANSLATORS: bpp is "Bits Per Pixel" */
    info += wxString::Format(wmsg(/*Display Depth: %d bpp*/196), bpp);
    /* TRANSLATORS: appended to previous message if the display is colour */
    if (wxColourDisplay()) info += wmsg(/* (colour)*/197);
    info += wxT('\n');
    info += wxString(GetGLSystemDescription().c_str(), wxConvUTF8);

    // Use a readonly multiline text edit for the system info so users can
    // easily cut and paste it into an email when reporting bugs.
    vert->Add(new wxTextCtrl(this, 506, info, wxDefaultPosition,
			     wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY),
	      1, wxLEFT | wxRIGHT | wxEXPAND, 20);

    vert->Add(10, 5, 0, wxTOP, 5);

    wxBoxSizer* bottom = new wxBoxSizer(wxHORIZONTAL);
    bottom->Add(5, 5, 1);
    bottom->Add(new wxButton(this, wxID_COPY), 0, wxRIGHT | wxBOTTOM, 6);
    wxButton* close = new wxButton(this, wxID_OK);
    bottom->Add(close, 0, wxRIGHT | wxBOTTOM, 15);
    vert->Add(bottom, 0, wxEXPAND | wxLEFT | wxRIGHT, 0);
    if (bitmap.Ok()) {
	vert->SetMinSize(0, bitmap.GetHeight());
    }

    SetSizer(horiz);
    close->SetDefault();

    horiz->SetSizeHints(this);

    timer.Start(42000);
}
