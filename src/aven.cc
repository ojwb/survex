//
//  aven.cxx
//
//  Main class for Aven.
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

#include "aven.h"
#include "mainfrm.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <wx/image.h>
#include <wx/confbase.h>

IMPLEMENT_APP(Aven)

Aven::Aven() :
    m_Frame(NULL)
{
}

bool Aven::OnInit()
{
    wxImage::AddHandler(new wxPNGHandler);
    //--need to sort this!
    m_AboutBitmap.LoadFile(wxString(DATADIR) + wxCONFIG_PATH_SEPARATOR + 
			   wxString("survex") + wxCONFIG_PATH_SEPARATOR +
			   wxString("aven-about.png"), wxBITMAP_TYPE_PNG);

    m_Frame = new MainFrm("Aven", wxPoint(50, 50), wxSize(640, 480));
    m_Frame->Show(true);

    return true;
}

void Aven::ReportError(const wxString& msg)
{
    wxMessageBox(msg, "Aven", wxOK | wxCENTRE | wxICON_EXCLAMATION);
}
