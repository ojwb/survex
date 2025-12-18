//
//  aboutdlg.h
//
//  About box handling for Aven.
//
//  Copyright (C) 2001, Mark R. Shinwell.
//  Copyright (C) 2004,2005,2010,2015 Olly Betts
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

#ifndef aboutdlg_h
#define aboutdlg_h

#include <wx/wx.h>

enum {
    about_TIMER = 1000
};

class AboutDlg : public wxDialog {
public:
    AboutDlg(wxWindow* parent, const wxIcon & app_icon);
    void OnTimer(wxTimerEvent &e);
    void OnCopy(wxCommandEvent &e);

private:
    wxBitmap bitmap;
    wxString img_path;
    wxTimer timer;
    wxString info;

    DECLARE_EVENT_TABLE()
};

#endif
