//
//  paneldlg.cc
//
//  Dialog boxes with multiple pages and a selector panel.
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
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include "paneldlg.h"

#include <assert.h>

static const wxWindowID ID_PAGE_BASE = 100;
static const wxWindowID ID_PAGE_BASE_IMG = 150;
static const wxWindowID ID_PAGE_BASE_PANEL = 150;

PanelDlg::PanelDlg(wxWindow* parent, wxWindowID id, const wxString& title) :
    wxDialog(parent, id, title)//, wxDefaultPosition, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
}

PanelDlg::~PanelDlg()
{
}

void PanelDlg::SetPages(list<PanelDlgPage*> pages)
{
    assert(pages.size() >= 1);

    // Create sizers.
    wxBoxSizer* horiz_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_VertSizer = new wxBoxSizer(wxVERTICAL);

    // Create the left-hand page button panel.
    wxPanel* page_panel = new wxPanel(this, 2000, wxDefaultPosition, wxDefaultSize,
                                      wxTAB_TRAVERSAL | wxSUNKEN_BORDER);

    // Get the background colour and calculate a "darker" version of it.
    wxColour col = page_panel->GetBackgroundColour();
    
    unsigned char r = (unsigned char)(double(col.Red()) * 0.4);
    unsigned char g = (unsigned char)(double(col.Green()) * 0.4);
    unsigned char b = (unsigned char)(double(col.Blue()) * 0.4);

    col.Set(r, g, b);

    // Fill the page button panel.
    wxBoxSizer* page_panel_sizer = new wxBoxSizer(wxVERTICAL);
    const int PAGE_ICON_HEAD_FOOT = 12;
    const int PAGE_ICON_HEIGHT = 75;
    const int PAGE_PANEL_WIDTH = 48 + (2*PAGE_ICON_HEAD_FOOT);
    page_panel_sizer->Add(10, PAGE_ICON_HEAD_FOOT); // add a spacer
    list<PanelDlgPage*>::iterator iter = pages.begin();
    int page_num = 0;
    while (iter != pages.end()) {
        PanelDlgPage* page = *iter++;

        // Get the text and icon for this button.
        const wxString& text = page->GetName();
        const wxBitmap& icon = page->GetIcon();

        // Create the button and label together inside a panel.
        wxPanel* panel = new wxPanel(page_panel, ID_PAGE_BASE_PANEL + page_num);
        wxBoxSizer* panel_sizer = new wxBoxSizer(wxVERTICAL);
        wxBitmapButton* img_button = new wxBitmapButton(panel, ID_PAGE_BASE_IMG + page_num, icon,
                                                        wxPoint(0, 0), wxSize(48, 48));
        wxStaticText* label = new wxStaticText(panel, ID_PAGE_BASE + page_num, text);
        panel_sizer->Add(img_button, 0, wxALIGN_CENTER, 4);
        panel_sizer->Add(label, 0, wxALIGN_CENTER, 4);
        panel->SetAutoLayout(true);
        panel->SetSizer(panel_sizer);
        panel->SetSize(PAGE_PANEL_WIDTH - 8, PAGE_ICON_HEIGHT);

        page_panel_sizer->Add(panel, 0 /* not vertically stretchable */, wxALIGN_CENTER);
        panel->SetBackgroundColour(col);

        page_num++;
    }
    page_panel->SetAutoLayout(true);
    page_panel->SetSizer(page_panel_sizer);
    int height = PAGE_ICON_HEAD_FOOT*2 + ((page_num - 1) * PAGE_ICON_HEIGHT);
    page_panel->SetSizeHints(PAGE_PANEL_WIDTH, height);
    page_panel->SetSize(PAGE_PANEL_WIDTH, height);
 
    // Darken the background colour.
    page_panel->SetBackgroundColour(col);

    // Fill the horizontal sizer.
    horiz_sizer->Add(page_panel, 0 /* not horizontally stretchable */, wxEXPAND | wxALL, 2);
    horiz_sizer->Add(m_VertSizer, 1 /* fill available space */, wxEXPAND | wxALL, 2);

    // Retrieve the panel for the first page.
    PanelDlgPage* first_page = *(pages.begin());
    assert(first_page);

    // Create the OK/Cancel button panel.
    wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* ok_button = new wxButton(this, wxID_OK, "OK");
    wxButton* cancel_button = new wxButton(this, wxID_CANCEL, "Cancel");
    button_sizer->Add(ok_button, 0 /* not horizontally stretchable */, wxALIGN_RIGHT | wxRIGHT, 4);
    button_sizer->Add(cancel_button, 0 /* not horizontally stretchable */, wxALIGN_RIGHT);

    // Fill the sizer holding the page and the button panel.
    m_VertSizer->Add(first_page, 1 /* fill available space */, wxALIGN_TOP | wxALL, 4);
    m_VertSizer->Add(button_sizer, 0 /* not vertically stretchable */, wxALIGN_BOTTOM | wxALL, 4);

    // Cause the dialog to use the horizontal sizer.
    SetAutoLayout(true);
    SetSizer(horiz_sizer);
    horiz_sizer->Fit(this);

    // Set a reasonable size.
    SetSize(500, 350);
}

