//
//  splash.cc
//
//  Splash screen for Aven.
//
//  Copyright (C) 2002, Mark R. Shinwell.
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

#include "splash.h"
#include <wx/image.h>
#include <wx/confbase.h>
#include "message.h"

Splash::Splash(bool loading_file) :
    wxFrame(NULL, 101, "Aven", wxDefaultPosition, wxDefaultSize,
            wxSTAY_ON_TOP | wxSIMPLE_BORDER)
{
    m_Bitmap.LoadFile(wxString(msg_cfgpth()) + wxCONFIG_PATH_SEPARATOR +
	              wxString("splash.png"), wxBITMAP_TYPE_PNG);
    if (m_Bitmap.Ok()) {
        int width = m_Bitmap.GetWidth();
        int height = m_Bitmap.GetHeight();
        const int prog_size = 24;
        int win_height = height + prog_size;
        wxStaticBitmap* sbm = new wxStaticBitmap(this, 100, m_Bitmap,
                                                 wxPoint(0, 0),
                                                 wxSize(width, height));
        if (loading_file) {
            m_Gauge = new wxGauge(this, 102, 100, wxPoint(0, height),
                                  wxSize(width, prog_size));
            SetSizeHints(width, win_height, width, win_height);
            Show(true);
        }
        else {
            m_Gauge = NULL;
        }

        CentreOnScreen();
        wxYield();
    }
    else {
        Hide();
    }
}

Splash::~Splash()
{

}

void Splash::SetProgress(int pos)
{
    assert(pos >= 0 && pos <= 100);

    if (m_Gauge) {
        m_Gauge->SetValue(pos);
        m_Gauge->Refresh(false);
    }
}
