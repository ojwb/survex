//
//  stnprefs.h
//
//  Preferences page for stations.
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

#ifndef stnprefs_h
#define stnprefs_h

#include "paneldlgpage.h"
#include "aven.h"

class GfxCore;

class StnPrefs : public PanelDlgPage {
    GfxCore* m_Parent;
    
    DECLARE_EVENT_TABLE()

public:
    StnPrefs(GfxCore* parent, wxWindow* parent_win);
    virtual ~StnPrefs();

    const wxString GetName();
    const wxBitmap GetIcon();

    void OnShowCrosses(wxCommandEvent&);
    void OnHighlightEntrances(wxCommandEvent&);
    void OnHighlightFixedPts(wxCommandEvent&);
    void OnHighlightExportedPts(wxCommandEvent&);
    void OnNames(wxCommandEvent&);
    void OnOverlappingNames(wxCommandEvent&);

    void OnShowCrossesUpdate(wxUpdateUIEvent&);
    void OnHighlightEntrancesUpdate(wxUpdateUIEvent&);
    void OnHighlightFixedPtsUpdate(wxUpdateUIEvent&);
    void OnHighlightExportedPtsUpdate(wxUpdateUIEvent&);
    void OnNamesUpdate(wxUpdateUIEvent&);
    void OnOverlappingNamesUpdate(wxUpdateUIEvent&);
};

#endif
