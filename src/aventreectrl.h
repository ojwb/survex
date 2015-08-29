//
//  aventreectrl.h
//
//  Tree control used for the survey tree.
//
//  Copyright (C) 2001, Mark R. Shinwell.
//  Copyright (C) 2002,2006 Olly Betts
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

#ifndef aventreectrl_h
#define aventreectrl_h

#include "wx.h"

class MainFrm;
class LabelInfo;

class TreeData : public wxTreeItemData {
    const LabelInfo* m_Label;
    wxString survey;

public:
    explicit TreeData(const LabelInfo* label) : m_Label(label) {}
    explicit TreeData(const wxString & survey_)
	: m_Label(NULL), survey(survey_) {}
    const LabelInfo* GetLabel() const { return m_Label; }
    const wxString & GetSurvey() const { return survey; }
    bool IsStation() const { return m_Label != NULL; }
};

class AvenTreeCtrl : public wxTreeCtrl {
    MainFrm* m_Parent;
    bool m_Enabled;
    wxTreeItemId m_LastItem;
    wxColour m_BackgroundColour;
    bool m_SelValid;

public:
    AvenTreeCtrl(MainFrm* parent, wxWindow* window_parent);

    void SetEnabled(bool enabled = true);
    void UnselectAll();

    void OnMouseMove(wxMouseEvent& event);
    void OnLeaveWindow(wxMouseEvent& event);
    void OnSelChanged(wxTreeEvent& event);
    void OnKeyPress(wxKeyEvent &e);
    void OnItemActivated(wxTreeEvent& e);
    void OnRightClick(wxTreeEvent& e);

    bool GetSelectionData(wxTreeItemData**) const;

    void SetHere(wxTreeItemId pos);

    void DeleteAllItems();

private:
    DECLARE_EVENT_TABLE()
};

#endif
