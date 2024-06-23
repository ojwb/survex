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

#include <config.h>

#include "aventreectrl.h"
#include "mainfrm.h"

#include <stack>

using namespace std;

// STATE_BLANK is used for stations which are siblings of surveys which have
// select checkboxes.
enum { STATE_BLANK = 0, STATE_OFF, STATE_ON };

/* XPM */
static const char *blank_xpm[] = {
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
static const char *off_xpm[] = {
/* columns rows colors chars-per-pixel */
"15 15 2 1",
". c #000000",
"  c None",
/* pixels */
"               ",
"               ",
" ............  ",
" .          .  ",
" .          .  ",
" .          .  ",
" .          .  ",
" .          .  ",
" .          .  ",
" .          .  ",
" .          .  ",
" .          .  ",
" .          .  ",
" ............  ",
"               "
};

/* XPM */
static const char *on_xpm[] = {
/* columns rows colors chars-per-pixel */
"15 15 3 1",
". c #000000",
"X c #007F28",
"  c None",
/* pixels */
"               ",
"               ",
" ............XX",
" .          XXX",
" .         XXXX",
" .        XXXX ",
" .       XXXX  ",
" .      XXXX.  ",
" . XX  XXXX .  ",
" . XXXXXXX  .  ",
" .  XXXXX   .  ",
" .   XXX    .  ",
" .    X     .  ",
" ............  ",
"               "
};

BEGIN_EVENT_TABLE(AvenTreeCtrl, wxTreeCtrl)
    EVT_MOTION(AvenTreeCtrl::OnMouseMove)
    EVT_LEAVE_WINDOW(AvenTreeCtrl::OnLeaveWindow)
    EVT_TREE_SEL_CHANGED(wxID_ANY, AvenTreeCtrl::OnSelChanged)
    EVT_TREE_ITEM_ACTIVATED(wxID_ANY, AvenTreeCtrl::OnItemActivated)
    EVT_CHAR(AvenTreeCtrl::OnKeyPress)
    EVT_TREE_ITEM_MENU(wxID_ANY, AvenTreeCtrl::OnMenu)
    EVT_MENU(menu_SURVEY_SHOW_ALL, AvenTreeCtrl::OnRestrict)
    EVT_MENU(menu_SURVEY_RESTRICT, AvenTreeCtrl::OnRestrict)
    EVT_MENU(menu_SURVEY_HIDE, AvenTreeCtrl::OnHide)
    EVT_MENU(menu_SURVEY_SHOW, AvenTreeCtrl::OnShow)
    EVT_MENU(menu_SURVEY_HIDE_SIBLINGS, AvenTreeCtrl::OnHideSiblings)
    EVT_TREE_STATE_IMAGE_CLICK(wxID_ANY, AvenTreeCtrl::OnStateClick)
END_EVENT_TABLE()

AvenTreeCtrl::AvenTreeCtrl(MainFrm* parent, wxWindow* window_parent) :
    wxTreeCtrl(window_parent, wxID_ANY),
    m_Parent(parent),
    m_Enabled(false),
    m_LastItem(),
    m_BackgroundColour(),
    m_SelValid(false),
    menu_data(NULL)
{
    wxImageList* img_list = new wxImageList(15, 15, 2);
    img_list->Add(wxBitmap(blank_xpm));
    img_list->Add(wxBitmap(off_xpm));
    img_list->Add(wxBitmap(on_xpm));
    AssignStateImageList(img_list);
}

void AvenTreeCtrl::FillTree(const wxString& root_name)
{
    Freeze();
    m_Enabled = false;
    m_LastItem = wxTreeItemId();
    m_SelValid = false;
    DeleteAllItems();

    const wxChar separator = m_Parent->GetSeparator();
    filter.clear();
    filter.SetSeparator(separator);

    // Create the root of the tree.
    wxTreeItemId treeroot = AddRoot(root_name);

    // Fill the tree of stations and prefixes.
    stack<wxTreeItemId> previous_ids;
    wxString current_prefix;
    wxTreeItemId current_id = treeroot;

    list<LabelInfo*>::const_iterator pos = m_Parent->GetLabels();
    while (pos != m_Parent->GetLabelsEnd()) {
	LabelInfo* label = *pos++;

	if (label->IsAnon()) continue;

	// Determine the current prefix.
	wxString prefix = label->GetText().BeforeLast(separator);

	// Determine if we're still on the same prefix.
	if (prefix == current_prefix) {
	    // no need to fiddle with branches...
	}
	// If not, then see if we've descended to a new prefix.
	else if (prefix.length() > current_prefix.length() &&
		 prefix.StartsWith(current_prefix) &&
		 (prefix[current_prefix.length()] == separator ||
		  current_prefix.empty())) {
	    // We have, so start as many new branches as required.
	    int current_prefix_length = current_prefix.length();
	    current_prefix = prefix;
	    size_t next_dot = current_prefix_length;
	    if (!next_dot) --next_dot;
	    do {
		size_t prev_dot = next_dot + 1;

		// Extract the next bit of prefix.
		next_dot = prefix.find(separator, prev_dot + 1);

		wxString bit = prefix.substr(prev_dot, next_dot - prev_dot);
		// Sigh, therion can produce files with empty components in
		// station names!
		// assert(!bit.empty());

		// Add the current tree ID to the stack.
		previous_ids.push(current_id);

		// Append the new item to the tree and set this as the current branch.
		current_id = AppendItem(current_id, bit);
		SetItemData(current_id, new TreeData(prefix.substr(0, next_dot)));
	    } while (next_dot != wxString::npos);
	}
	// Otherwise, we must have moved up, and possibly then down again.
	else {
	    size_t count = 0;
	    bool ascent_only = (prefix.length() < current_prefix.length() &&
				current_prefix.StartsWith(prefix) &&
				(current_prefix[prefix.length()] == separator ||
				 prefix.empty()));
	    if (!ascent_only) {
		// Find out how much of the current prefix and the new prefix
		// are the same.
		// Note that we require a match of a whole number of parts
		// between dots!
		size_t n = min(prefix.length(), current_prefix.length());
		size_t i;
		for (i = 0; i < n && prefix[i] == current_prefix[i]; ++i) {
		    if (prefix[i] == separator) count = i + 1;
		}
	    } else {
		count = prefix.length() + 1;
	    }

	    // Extract the part of the current prefix after the bit (if any)
	    // which has matched.
	    // This gives the prefixes to ascend over.
	    wxString prefixes_ascended = current_prefix.substr(count);

	    // Count the number of prefixes to ascend over.
	    int num_prefixes = prefixes_ascended.Freq(separator);

	    // Reverse up over these prefixes.
	    for (int i = 1; i <= num_prefixes; i++) {
		previous_ids.pop();
	    }
	    current_id = previous_ids.top();
	    previous_ids.pop();

	    if (!ascent_only) {
		// Add branches for this new part.
		size_t next_dot = count - 1;
		do {
		    size_t prev_dot = next_dot + 1;

		    // Extract the next bit of prefix.
		    next_dot = prefix.find(separator, prev_dot + 1);

		    wxString bit = prefix.substr(prev_dot, next_dot - prev_dot);
		    // Sigh, therion can produce files with empty components in
		    // station names!
		    // assert(!bit.empty());

		    // Add the current tree ID to the stack.
		    previous_ids.push(current_id);

		    // Append the new item to the tree and set this as the current branch.
		    current_id = AppendItem(current_id, bit);
		    SetItemData(current_id, new TreeData(prefix.substr(0, next_dot)));
		} while (next_dot != wxString::npos);
	    }

	    current_prefix = prefix;
	}

	// Now add the leaf.
	wxString bit = label->GetText().AfterLast(separator);
	// Sigh, therion can produce files with empty components in station
	// names!
	// assert(!bit.empty());
	wxTreeItemId id = AppendItem(current_id, bit);
	SetItemData(id, new TreeData(label));
	label->tree_id = id;
	// Set the colour for an item in the survey tree.
	if (label->IsEntrance()) {
	    // Entrances are green (like entrance blobs).
	    SetItemTextColour(id, wxColour(0, 255, 40));
	} else if (label->IsSurface()) {
	    // Surface stations are dark green.
	    SetItemTextColour(id, wxColour(49, 158, 79));
	}
    }

    Expand(treeroot);
    m_Enabled = true;
    Thaw();
}

constexpr auto TREE_MASK = wxTREE_HITTEST_ONITEMLABEL |
			   wxTREE_HITTEST_ONITEMRIGHT |
			   wxTREE_HITTEST_ONITEMSTATEICON;

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
	const TreeData* data = static_cast<const TreeData*>(GetItemData(pos));
	m_Parent->DisplayTreeInfo(data);
	if (data && !data->IsStation()) {
	    // For stations, MainFrm calls back to SetHere(), but for surveys
	    // we need to do that ourselves.
	    SetHere(pos);
	}
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

void AvenTreeCtrl::OnSelChanged(wxTreeEvent&)
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
	//menu.Append(menu_SURVEY_HIDE, wmsg(/*&Hide*/407));
	menu.Append(menu_SURVEY_SHOW, wmsg(/*&Show*/409));
	//menu.Append(menu_SURVEY_HIDE_SIBLINGS, wmsg(/*Hide si&blings*/388));
	switch (GetItemState(menu_item)) {
	    case STATE_ON: // Currently shown.
		menu.Enable(menu_SURVEY_SHOW, false);
		break;
#if 0
	    case STATE_HIDDEN: // Currently hidden.
		menu.Enable(menu_SURVEY_RESTRICT, false);
		menu.Enable(menu_SURVEY_HIDE, false);
		menu.Enable(menu_SURVEY_HIDE_SIBLINGS, false);
		break;
	    case STATE_OFF:
		menu.Enable(menu_SURVEY_HIDE, false);
		menu.Enable(menu_SURVEY_HIDE_SIBLINGS, false);
		break;
#endif
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

void AvenTreeCtrl::OnRestrict(wxCommandEvent&)
{
    m_Parent->RestrictTo(menu_data ? menu_data->GetSurvey() : wxString());
}

void AvenTreeCtrl::OnHide(wxCommandEvent&)
{
    // Shouldn't be available for the root item.
    wxASSERT(menu_data);
    // Hide should be disabled unless the item is explicitly shown.
    wxASSERT(GetItemState(menu_item) == STATE_ON);
    SetItemState(menu_item, STATE_OFF);
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

void AvenTreeCtrl::OnShow(wxCommandEvent&)
{
    // Shouldn't be available for the root item.
    wxASSERT(menu_data);
    auto old_state = GetItemState(menu_item);
    // Show should be disabled for an explicitly shown item.
    wxASSERT(old_state != STATE_ON);
    Freeze();
    SetItemState(menu_item, STATE_ON);
    filter.add(menu_data->GetSurvey());
    if (old_state == wxTREE_ITEMSTATE_NONE) {
	// Hide siblings if not already shown or hidden.
	wxTreeItemId i = menu_item;
	while ((i = GetPrevSibling(i)).IsOk()) {
	    if (GetItemState(i) == wxTREE_ITEMSTATE_NONE) {
		const TreeData* data = static_cast<const TreeData*>(GetItemData(i));
		SetItemState(i, data->IsStation() ? STATE_BLANK : STATE_OFF);
	    }
	}
	i = menu_item;
	while ((i = GetNextSibling(i)).IsOk()) {
	    if (GetItemState(i) == wxTREE_ITEMSTATE_NONE) {
		const TreeData* data = static_cast<const TreeData*>(GetItemData(i));
		SetItemState(i, data->IsStation() ? STATE_BLANK : STATE_OFF);
	    }
	}
    }
    Thaw();
    m_Parent->ForceFullRedraw();
}

void AvenTreeCtrl::OnHideSiblings(wxCommandEvent&)
{
    // Shouldn't be available for the root item.
    wxASSERT(menu_data);
    Freeze();
    SetItemState(menu_item, STATE_ON);
    filter.add(menu_data->GetSurvey());

    wxTreeItemId i = menu_item;
    while ((i = GetPrevSibling(i)).IsOk()) {
	const TreeData* data = static_cast<const TreeData*>(GetItemData(i));
	filter.remove(data->GetSurvey());
	SetItemState(i, data->IsStation() ? STATE_BLANK : STATE_OFF);
    }
    i = menu_item;
    while ((i = GetNextSibling(i)).IsOk()) {
	const TreeData* data = static_cast<const TreeData*>(GetItemData(i));
	filter.remove(data->GetSurvey());
	SetItemState(i, data->IsStation() ? STATE_BLANK : STATE_OFF);
    }
    Thaw();
    m_Parent->ForceFullRedraw();
}

void AvenTreeCtrl::OnStateClick(wxTreeEvent& e)
{
    auto item = e.GetItem();
    const TreeData* data = static_cast<const TreeData*>(GetItemData(item));
    switch (GetItemState(item)) {
	case STATE_BLANK:
	    // Click on blank state icon for a station - let the tree handle
	    // this in the same way as a click on the label.
	    return;
	case STATE_ON:
	    filter.remove(data->GetSurvey());
	    SetItemState(item, STATE_OFF);
	    break;
	case STATE_OFF:
	    filter.add(data->GetSurvey());
	    SetItemState(item, STATE_ON);
	    break;
    }
    e.Skip();
    m_Parent->ForceFullRedraw();
}
