//
//  childfrm.h
//
//  Child (view) frame handling for Aven.
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

#ifndef childfrm_h
#define childfrm_h

#include "wx.h"
#include "gfxcore.h"

class ChildFrm : public wxDocChildFrame {
    GfxCore m_Gfx;
    bool m_GfxInited;

    void SetAccelerators();

public:
    ChildFrm(wxDocument* doc, wxView* view, wxDocParentFrame* parent, wxWindowID id,
	     const wxString& title);
    ~ChildFrm();

    //-- this *must* be sorted out after the doc/view fiasco is done
    static wxMenu* BuildFileMenu();
    static wxMenu* BuildRotationMenu();
    static wxMenu* BuildOrientationMenu();
    static wxMenu* BuildViewMenu();
    static wxMenu* BuildControlsMenu();
    static wxMenu* BuildHelpMenu();

    static wxMenuBar* BuildMenuBar();

    void InitialiseGfx(); // call only after loading survey data into the document
    void OnCommand(wxCommandEvent& event);
    void OnUpdateUI(wxUpdateUIEvent& event);

private:
    DECLARE_EVENT_TABLE()
};

#endif
