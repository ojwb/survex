//
//  winprefs.h
//
//  Preferences page for window-related options.
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
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

#ifndef winprefs_h
#define winprefs_h

#include "paneldlgpage.h"
#include "aven.h"

class MainFrm;

class WinPrefs : public PanelDlgPage {
    MainFrm* m_Parent;
    
    DECLARE_EVENT_TABLE()

public:
    WinPrefs(MainFrm*, wxWindow*);
    virtual ~WinPrefs();

    const wxString GetName();
    const wxBitmap GetIcon();

    void OnSidePanel(wxCommandEvent&);
    void OnSidePanelUpdate(wxUpdateUIEvent&);
};

#endif
