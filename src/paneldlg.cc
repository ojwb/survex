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
static const wxWindowID ID_PAGE_BASE_PANEL = 200;
static const wxWindowID ID_PAGE_BASE_IMG_END = ID_PAGE_BASE_PANEL - 1;

BEGIN_EVENT_TABLE(PanelDlg, wxDialog)
    EVT_COMMAND_RANGE(ID_PAGE_BASE_IMG, ID_PAGE_BASE_IMG_END, wxEVT_COMMAND_BUTTON_CLICKED, PanelDlg::OnPageChange)
END_EVENT_TABLE()

PanelDlg::PanelDlg(wxWindow* parent, wxWindowID id, const wxString& title) :
    wxDialog(parent, id, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
}

PanelDlg::~PanelDlg()
{
}

void PanelDlg::OnPageChange(wxCommandEvent& event)
{
    // The user has requested a change of page.
    
    // Identify the new page.
    int page = event.GetId() - ID_PAGE_BASE_IMG;
    assert(page >= 0 && page < m_Pages.size());
    list<PanelDlgPage*>::iterator iter = m_Pages.begin();
    for (int x = 0; x < page; x++) iter++;
    PanelDlgPage* page_ptr = *iter;
    assert(page_ptr);

    if (page_ptr == m_CurrentPage) return;
    
    // Remove the current page from display.
    m_CurrentPage->Hide();
    m_VertSizer->Remove(m_CurrentPage);
    m_VertSizer->Layout();

    // Display the new page.
    m_VertSizer->Prepend(page_ptr, 1 /* fill available space */, wxALL, 4);
    m_CurrentPage = page_ptr;

    // Force a re-layout of the sizer.
    m_VertSizer->Layout();
    page_ptr->Show();
}

void PanelDlg::SetPages(list<PanelDlgPage*> pages)
{
    assert(pages.size() >= 1);

    m_Pages = pages;

    // Create sizers.
    wxBoxSizer* horiz_sizer = new wxBoxSizer(wxHORIZONTAL);
    m_VertSizer = new wxBoxSizer(wxVERTICAL);

    // Create the left-hand page button panel.
    wxScrolledWindow* scrolled_win = new wxScrolledWindow(this, 3000, wxDefaultPosition, wxDefaultSize,
                                                          wxVSCROLL | wxSUNKEN_BORDER);
    wxPanel* page_panel = new wxPanel(scrolled_win, 2000);

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
    const int PAGE_PANEL_WIDTH = int((48 + (2*PAGE_ICON_HEAD_FOOT)) * 4.0/3.0);
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
        page_panel_sizer->Add(8, PAGE_ICON_HEAD_FOOT); // add a spacer
        panel_sizer->Add(img_button, 0, wxALIGN_CENTER, 4);
        panel_sizer->Add(label, 0, wxALIGN_CENTER, 4);
        panel->SetAutoLayout(true);
        panel->SetSizer(panel_sizer);
        panel->SetSize(PAGE_PANEL_WIDTH - 8, PAGE_ICON_HEIGHT - PAGE_ICON_HEAD_FOOT);

        page_panel_sizer->Add(panel, 0 /* not vertically stretchable */, wxALIGN_CENTER);
        panel->SetBackgroundColour(col);

        page_num++;
    }
    page_panel_sizer->Add(8, PAGE_ICON_HEAD_FOOT); // add a spacer
    page_panel->SetAutoLayout(true);
    page_panel->SetSizer(page_panel_sizer);
    int height = PAGE_ICON_HEAD_FOOT + (page_num * PAGE_ICON_HEIGHT);
    page_panel->SetSizeHints(PAGE_PANEL_WIDTH, height);
    page_panel->SetSize(PAGE_PANEL_WIDTH, height);
    scrolled_win->SetScrollbars(0, height / page_num, 0, page_num);
    scrolled_win->SetSize(/* FIXME */ 16 + PAGE_PANEL_WIDTH, 500);
 
    // Darken the background colour.
    page_panel->SetBackgroundColour(col);

    // Fill the horizontal sizer.
    horiz_sizer->Add(scrolled_win, 0 /* not horizontally stretchable */, wxEXPAND | wxALL, 2);
    horiz_sizer->Add(m_VertSizer, 1 /* fill available space */, wxEXPAND | wxALL, 2);

    // Retrieve the panel for the first page.
    PanelDlgPage* first_page = *(pages.begin());
    assert(first_page);
    m_CurrentPage = first_page;

    // Create the OK/Cancel button panel.
    m_ButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* ok_button = new wxButton(this, wxID_OK, "OK");
    ok_button->SetDefault();
    wxButton* cancel_button = new wxButton(this, wxID_CANCEL, "Cancel");
    m_ButtonSizer->Add(ok_button, 0 /* not horizontally stretchable */, wxALIGN_RIGHT | wxRIGHT, 4);
    m_ButtonSizer->Add(cancel_button, 0 /* not horizontally stretchable */, wxALIGN_RIGHT);

    // Fill the sizer holding the page and the button panel.
    m_VertSizer->Add(first_page, 1 /* fill available space */, wxALIGN_TOP | wxALL, 4);
    m_VertSizer->Add(m_ButtonSizer, 0 /* not vertically stretchable */, wxALIGN_RIGHT | wxALL, 4);

    // Cause the dialog to use the horizontal sizer.
    SetAutoLayout(true);
    SetSizer(horiz_sizer);
    horiz_sizer->Fit(this);

    // Set a reasonable size and centre the dialog with respect to the parent window.
    SetSize(500, PAGE_ICON_HEIGHT*4 + PAGE_ICON_HEAD_FOOT);
    CentreOnParent();
}

