//
//  mainfrm.h
//
//  Main frame handling for Aven.
//
//  Copyright (C) 2000-2001, Mark R. Shinwell.
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

#ifndef mainfrm_h
#define mainfrm_h

#include "wx.h"

class MainFrm : public wxDocParentFrame {
    wxMenu* BuildFileMenu();
    wxMenu* BuildHelpMenu();

    void BuildMenuBar();

public:
    MainFrm(wxDocManager* manager, wxFrame* parent, wxWindowID id, const wxString& title);
    ~MainFrm();

    void OnOpen(wxCommandEvent& event);
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);

private:
    DECLARE_EVENT_TABLE()
};

#endif
