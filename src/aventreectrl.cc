//
//  aventreectrl.cc
//
//  Tree control used for the survey tree.
//
//  Copyright (C) 2001, Mark R. Shinwell.
//  Copyright (C) 2001-2002, Olly Betts
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

#include "aventreectrl.h"
#include "mainfrm.h"

BEGIN_EVENT_TABLE(AvenTreeCtrl, wxTreeCtrl)
    EVT_MOTION(AvenTreeCtrl::OnMouseMove)
    EVT_TREE_SEL_CHANGED(-1, AvenTreeCtrl::OnSelChanged)
    EVT_CHAR(AvenTreeCtrl::OnKeyPress)
END_EVENT_TABLE()

AvenTreeCtrl::AvenTreeCtrl(MainFrm* parent, wxWindow* window_parent) :
    wxTreeCtrl(window_parent, -1, wxDefaultPosition),
    m_LastItem((wxTreeItemId) -1),
    m_Enabled(false),
    m_Parent(parent),
    m_SelValid(false),
    m_BackgroundColour(GetBackgroundColour())
{
}

#define TREE_MASK (wxTREE_HITTEST_ONITEMLABEL | wxTREE_HITTEST_ONITEMRIGHT)

void AvenTreeCtrl::OnMouseMove(wxMouseEvent& event)
{
    if (m_Enabled) {
	int flags;
	wxTreeItemId pos = HitTest(event.GetPosition(), flags);
	if (pos != m_LastItem) {
	    if (m_LastItem != -1) {
		SetItemBackgroundColour(m_LastItem, m_BackgroundColour);
	    }
	    if (flags & TREE_MASK) {
		SetItemBackgroundColour(pos, wxColour(180, 180, 180));
		m_Parent->DisplayTreeInfo(GetItemData(pos));
		m_LastItem = pos;
	    } else {
		m_Parent->DisplayTreeInfo(NULL);
	    }
	}
    }
}

void AvenTreeCtrl::SetEnabled(bool enabled)
{
    m_Enabled = enabled;
}

void AvenTreeCtrl::OnSelChanged(wxTreeEvent& event)
{
    if (m_Enabled) {
	m_Parent->TreeItemSelected(GetItemData(GetSelection()));
    }

    m_SelValid = true;
}

bool AvenTreeCtrl::GetSelectionData(wxTreeItemData** data)
{
    assert(m_Enabled);

    if (!m_SelValid) {
	return false;
    }

    wxTreeItemId id = GetSelection();
    if (id.IsOk()) {
	*data = GetItemData(id);
    }

    return id.IsOk() && *data;
}

void AvenTreeCtrl::UnselectAll()
{
    m_SelValid = false;
    wxTreeCtrl::UnselectAll();
}

void AvenTreeCtrl::DeleteAllItems()
{
    m_LastItem = -1;
    m_SelValid = false;
    wxTreeCtrl::DeleteAllItems();    
}

void AvenTreeCtrl::OnKeyPress(wxKeyEvent &e)
{
    if (e.m_keyCode == WXK_ESCAPE) {
	m_Parent->ClearTreeSelection();
    } else {
	e.Skip();
    }
}
