//
//  avenview.cc
//
//  View handling for Aven.
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

#include "avendoc.h"
#include "avenview.h"
#include "mainfrm.h"
#include "childfrm.h"

IMPLEMENT_DYNAMIC_CLASS(AvenView, wxView)

bool AvenView::m_First = true;

AvenView::AvenView() : m_Gfx(NULL)
{

}

AvenView::~AvenView()
{

}

bool AvenView::OnCreate(wxDocument* doc, long flags)
{
    // Called when a new view is created.

    if (m_First) {
        // The first survey opened should sit inside the "parent" frame window.
        // Unfortunately, wxWindows seems to hinder this.

        wxWindow* parent = wxGetApp().GetMainFrame();
	m_Gfx = new GfxCore((AvenDoc*) doc, parent);
	int x;
	int y;
	m_Gfx->GetSize(&x, &y);
	m_Gfx->SetSize(-1, -1, x, y);
	wxGetApp().GetMainFrame()->SetFullMenuBar();
    }
    else {
        // Create a new child frame.
        m_Frame = new ChildFrm(doc, this, wxGetApp().GetMainFrame(), -1, "Aven");

#ifdef __X__
	// X seems to require a forced resize.
	int x;
	int y;
	m_Frame->GetSize(&x, &y);
	m_Frame->SetSize(-1, -1, x, y);
#endif

	m_Frame->Show(true);
    }

    m_First = false;

    return true;
}

void AvenView::OnDraw(wxDC* dc)
{

}

void AvenView::OnUpdate(wxView*, wxObject*)
{
  //   static bool inited = false;
    
    if (m_Gfx) {
        m_Gfx->Initialise();
    }
    else {
        m_Frame->InitialiseGfx();
    }
}
