//
//  aventreectrl.cc
//
//  Tree control used for the survey tree.
//
//  Copyright (C) 2001, Mark R. Shinwell.
//  Copyright (C) 2001-2003,2005,2006,2016,2018 Olly Betts
//  Copyright (C) 2005 Martin Green
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "aventreectrl.h"
#include "mainfrm.h"

enum { STATE_NONE = 0, STATE_VISIBLE };

/* XPM */
static const char *none_xpm[] = {
/* columns rows colors chars-per-pixel */
"15 15 1 1",
"  c None",
/* pixels */
"               ",
"               ",
"               ",
"               ",
"               ",
"               ",
"               ",
"               ",
"               ",
"               ",
"               ",
"               ",
"               ",
"               ",
"               "
};

/* XPM */
static const char *visible_xpm[] = {
/* columns rows colors chars-per-pixel */
"15 15 2 1",
"X c #007F28",
"  c None",
/* pixels */
"               ",
"             XX",
"            XXX",
"           XXXX",
"          XXXX ",
"         XXXX  ",
"        XXXX   ",
"XX     XXXX    ",
"XXX   XXXX     ",
"XXXX XXXX      ",
" XXXXXXX       ",
"  XXXXX        ",
"   XXX         ",
"    X          ",
"               "
};

BEGIN_EVENT_TABLE(AvenTreeCtrl, wxTreeCtrl)
    EVT_MOTION(AvenTreeCtrl::OnMouseMove)
    EVT_LEAVE_WINDOW(AvenTreeCtrl::OnLeaveWindow)
    EVT_TREE_SEL_CHANGED(-1, AvenTreeCtrl::OnSelChanged)
    EVT_TREE_ITEM_ACTIVATED(-1, AvenTreeCtrl::OnItemActivated)
    EVT_CHAR(AvenTreeCtrl::OnKeyPress)
    EVT_TREE_ITEM_MENU(-1, AvenTreeCtrl::OnMenu)
    EVT_MENU(menu_SURVEY_SHOW_ALL, AvenTreeCtrl::OnRestrict)
    EVT_MENU(menu_SURVEY_RESTRICT, AvenTreeCtrl::OnRestrict)
    EVT_MENU(menu_SURVEY_HIDE, AvenTreeCtrl::OnHide)
    EVT_MENU(menu_SURVEY_SHOW, AvenTreeCtrl::OnShow)
    EVT_MENU(menu_SURVEY_HIDE_SIBLINGS, AvenTreeCtrl::OnHideSiblings)
    EVT_TREE_STATE_IMAGE_CLICK(-1, AvenTreeCtrl::OnStateClick)
END_EVENT_TABLE()

AvenTreeCtrl::AvenTreeCtrl(MainFrm* parent, wxWindow* window_parent) :
    wxTreeCtrl(window_parent, -1),
    m_Parent(parent),
    m_Enabled(false),
    m_LastItem(),
    m_BackgroundColour(),
    m_SelValid(false),
    menu_data(NULL)
{
    wxImageList* img_list = new wxImageList(15, 15, 2);
    img_list->Add(wxBitmap(none_xpm));
    img_list->Add(wxBitmap(visible_xpm));
    AssignStateImageList(img_list);
}

#define TREE_MASK (wxTREE_HITTEST_ONITEMLABEL | wxTREE_HITTEST_ONITEMRIGHT)

void AvenTreeCtrl::OnMouseMove(wxMouseEvent& event)
{
    if (!m_Enabled || m_Parent->Animating())
	return;

    int flags;
    wxTreeItemId pos = HitTest(event.GetPosition(), flags);
    if (!(flags & TREE_MASK)) {
	pos = wxTreeItemId();
    }
    if (pos == m_LastItem) return;
    if (pos.IsOk()) {
	m_Parent->DisplayTreeInfo(GetItemData(pos));
    } else {
	m_Parent->DisplayTreeInfo();
    }
}

void AvenTreeCtrl::SetHere(wxTreeItemId pos)
{
    if (pos == m_LastItem) return;

    if (m_LastItem.IsOk()) {
	SetItemBackgroundColour(m_LastItem, m_BackgroundColour);
    }
    if (pos.IsOk()) {
	m_BackgroundColour = GetItemBackgroundColour(pos);
	SetItemBackgroundColour(pos, wxColour(180, 180, 180));
    }
    m_LastItem = pos;
}

void AvenTreeCtrl::OnLeaveWindow(wxMouseEvent&)
{
    if (m_LastItem.IsOk()) {
	SetItemBackgroundColour(m_LastItem, m_BackgroundColour);
	m_LastItem = wxTreeItemId();
    }
    m_Parent->DisplayTreeInfo();
}

void AvenTreeCtrl::OnSelChanged(wxTreeEvent& e)
{
    m_SelValid = true;
}

void AvenTreeCtrl::OnItemActivated(wxTreeEvent& e)
{
    if (!m_Enabled) return;

    m_Parent->TreeItemSelected(GetItemData(e.GetItem()));
}

void AvenTreeCtrl::OnMenu(wxTreeEvent& e)
{
    if (!m_Enabled) return;

    const TreeData* data = static_cast<const TreeData*>(GetItemData(e.GetItem()));
    menu_data = data;
    menu_item = e.GetItem();
    if (!data) {
	// Root:
	wxMenu menu;
	/* TRANSLATORS: In aven's survey tree, right-clicking on the root
	 * gives a pop-up menu and this is an option (but only enabled if
	 * the view is restricted to a subsurvey). It reloads the current
	 * survey file with the who survey visible.
	 */
	menu.Append(menu_SURVEY_SHOW_ALL, wmsg(/*Show all*/245));
	if (m_Parent->GetSurvey().empty())
	    menu.Enable(menu_SURVEY_SHOW_ALL, false);
	PopupMenu(&menu);
    } else if (data->GetLabel()) {
	// Station: name is data->GetLabel()->GetText()
    } else {
	// Survey:
	wxMenu menu;
	/* TRANSLATORS: In aven's survey tree, right-clicking on a survey
	 * name gives a pop-up menu and this is an option.  It reloads the
	 * current survey file with the view restricted to the survey
	 * clicked upon.
	 */
	menu.Append(menu_SURVEY_RESTRICT, wmsg(/*Hide others*/246));
	menu.AppendSeparator();
	menu.Append(menu_SURVEY_HIDE, wmsg(/*&Hide*/407));
	menu.Append(menu_SURVEY_SHOW, wmsg(/*&Show*/409));
	menu.Append(menu_SURVEY_HIDE_SIBLINGS, wmsg(/*Hide si&blings*/388));
	switch (GetItemState(menu_item)) {
	    case STATE_VISIBLE: // Currently shown.
		menu.Enable(menu_SURVEY_SHOW, false);
		break;
#if 0
	    case STATE_HIDDEN: // Currently hidden.
		menu.Enable(menu_SURVEY_RESTRICT, false);
		menu.Enable(menu_SURVEY_HIDE, false);
		menu.Enable(menu_SURVEY_HIDE_SIBLINGS, false);
		break;
#endif
	    case STATE_NONE:
		menu.Enable(menu_SURVEY_HIDE, false);
		menu.Enable(menu_SURVEY_HIDE_SIBLINGS, false);
		break;
	}
	PopupMenu(&menu);
    }
    menu_data = NULL;
    e.Skip();
}

bool AvenTreeCtrl::GetSelectionData(wxTreeItemData** data) const
{
    assert(m_Enabled);
    assert(data);

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
    m_Enabled = false;
    m_LastItem = wxTreeItemId();
    m_SelValid = false;
    wxTreeCtrl::DeleteAllItems();
    filter.clear();
    filter.SetSeparator(m_Parent->GetSeparator());
}

void AvenTreeCtrl::OnKeyPress(wxKeyEvent &e)
{
    switch (e.GetKeyCode()) {
	case WXK_ESCAPE:
	    m_Parent->ClearTreeSelection();
	    break;
	case WXK_RETURN: {
	    wxTreeItemId id = GetSelection();
	    if (id.IsOk()) {
		if (ItemHasChildren(id)) {
		    // If on a branch, expand/contract it.
		    if (IsExpanded(id)) {
			Collapse(id);
		    } else {
			Expand(id);
		    }
		} else {
		    // If on a station, centre on it by selecting it twice.
		    m_Parent->TreeItemSelected(GetItemData(id));
		    m_Parent->TreeItemSelected(GetItemData(id));
		}
	    }
	    break;
	}
	case WXK_LEFT: case WXK_RIGHT: case WXK_UP: case WXK_DOWN:
	case WXK_HOME: case WXK_END: case WXK_PAGEUP: case WXK_PAGEDOWN:
	    e.Skip();
	    break;
	default:
	    // Pass key event to MainFrm which will pass to GfxCore which will
	    // pass to GUIControl.
	    m_Parent->OnKeyPress(e);
	    break;
    }
}

void AvenTreeCtrl::OnRestrict(wxCommandEvent& e)
{
    m_Parent->RestrictTo(menu_data ? menu_data->GetSurvey() : wxString());
}

void AvenTreeCtrl::OnHide(wxCommandEvent& e)
{
    // Shouldn't be available for the root item.
    wxASSERT(menu_data);
    // Hide should be disabled unless the item is explicitly shown.
    wxASSERT(GetItemState(menu_item) == STATE_VISIBLE);
    SetItemState(menu_item, STATE_NONE);
    filter.remove(menu_data->GetSurvey());
#if 0
    Freeze();
    // Show siblings if not already shown or hidden.
    wxTreeItemId i = menu_item;
    while ((i = GetPrevSibling(i)).IsOk()) {
	if (GetItemState(i) == wxTREE_ITEMSTATE_NONE)
	    SetItemState(i, 1);
    }
    i = menu_item;
    while ((i = GetNextSibling(i)).IsOk()) {
	if (GetItemState(i) == wxTREE_ITEMSTATE_NONE)
	    SetItemState(i, 1);
    }
    Thaw();
#endif
    m_Parent->ForceFullRedraw();
}

void AvenTreeCtrl::OnShow(wxCommandEvent& e)
{
    // Shouldn't be available for the root item.
    wxASSERT(menu_data);
    // Show should be disabled for an explicitly shown item.
    wxASSERT(GetItemState(menu_item) != STATE_VISIBLE);
    Freeze();
    SetItemState(menu_item, STATE_VISIBLE);
    filter.add(menu_data->GetSurvey());
    // Hide siblings if not already shown or hidden.
    wxTreeItemId i = menu_item;
    while ((i = GetPrevSibling(i)).IsOk()) {
	if (GetItemState(i) == wxTREE_ITEMSTATE_NONE)
	    SetItemState(i, STATE_NONE);
    }
    i = menu_item;
    while ((i = GetNextSibling(i)).IsOk()) {
	if (GetItemState(i) == wxTREE_ITEMSTATE_NONE)
	    SetItemState(i, STATE_NONE);
    }
    Thaw();
    m_Parent->ForceFullRedraw();
}

void AvenTreeCtrl::OnHideSiblings(wxCommandEvent& e)
{
    // Shouldn't be available for the root item.
    wxASSERT(menu_data);
    Freeze();
    SetItemState(menu_item, STATE_VISIBLE);
    filter.add(menu_data->GetSurvey());

    wxTreeItemId i = menu_item;
    while ((i = GetPrevSibling(i)).IsOk()) {
	const TreeData* data = static_cast<const TreeData*>(GetItemData(i));
	filter.remove(data->GetSurvey());
	SetItemState(i, STATE_NONE);
    }
    i = menu_item;
    while ((i = GetNextSibling(i)).IsOk()) {
	const TreeData* data = static_cast<const TreeData*>(GetItemData(i));
	filter.remove(data->GetSurvey());
	SetItemState(i, STATE_NONE);
    }
    Thaw();
    m_Parent->ForceFullRedraw();
}

void AvenTreeCtrl::OnStateClick(wxTreeEvent& e)
{
    auto item = e.GetItem();
    const TreeData* data = static_cast<const TreeData*>(GetItemData(item));
    if (GetItemState(item) == STATE_VISIBLE) {
	filter.remove(data->GetSurvey());
	SetItemState(item, STATE_NONE);
    } else {
	filter.add(data->GetSurvey());
	SetItemState(item, STATE_VISIBLE);
    }
    e.Skip();
    m_Parent->ForceFullRedraw();
}
