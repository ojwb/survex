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

static const int PAGE_ICON_HEAD_FOOT = 12;
static const int PAGE_ICON_HEIGHT = 75;
static const int PAGE_PANEL_WIDTH = int((48 + (2*PAGE_ICON_HEAD_FOOT)) * 4.0/3.0);
static const int EDGE_MARGIN = 4;
static const int EDGE_MARGIN_V = 3;
static const int SCROLLED_WIN_WIDTH = 16 + PAGE_PANEL_WIDTH;
static const int DIALOG_WIDTH = 500;
static const int DIALOG_HEIGHT = PAGE_ICON_HEIGHT*4 + PAGE_ICON_HEAD_FOOT;

BEGIN_EVENT_TABLE(PanelDlg, wxDialog)
    EVT_COMMAND_RANGE(ID_PAGE_BASE_IMG, ID_PAGE_BASE_IMG_END, wxEVT_COMMAND_BUTTON_CLICKED, PanelDlg::OnPageChange)
END_EVENT_TABLE()

PanelDlg::PanelDlg(wxWindow* parent, wxWindowID id, const wxString& title) : wxDialog(parent, id, title)
{
}

PanelDlg::~PanelDlg()
{
}

void PanelDlg::PositionPage()
{
    // Position the current page on the dialog.
    
    wxSize button_size = wxButton::GetDefaultSize();

    m_CurrentPage->SetSizeHints(-1, -1,
                                DIALOG_WIDTH - EDGE_MARGIN*3 - SCROLLED_WIN_WIDTH,
                                DIALOG_HEIGHT - EDGE_MARGIN_V*4 - button_size.GetHeight());
    m_CurrentPage->SetSize(SCROLLED_WIN_WIDTH + EDGE_MARGIN*3, EDGE_MARGIN_V*2,
                           DIALOG_WIDTH - EDGE_MARGIN*3 - SCROLLED_WIN_WIDTH,
                           DIALOG_HEIGHT - EDGE_MARGIN_V*4 - button_size.GetHeight());
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

    // Display the new page.
    m_CurrentPage = page_ptr;
    m_CurrentPage->Show();
    PositionPage();
}

void PanelDlg::SetPages(list<PanelDlgPage*> pages)
{
    assert(pages.size() >= 1);

    m_Pages = pages;

    // Create the left-hand page button panel.
    wxScrolledWindow* scrolled_win = new wxScrolledWindow(this, 3000, wxDefaultPosition, wxDefaultSize,
                                                          wxVSCROLL | wxSUNKEN_BORDER);
    wxPanel* page_panel = new wxPanel(scrolled_win, 2000);

    // Get the background colour and calculate a "darker" version of it.
    wxColour col = page_panel->GetBackgroundColour();
    
    unsigned char r = (unsigned char)(double(col.Red()) * 0.5);
    unsigned char g = (unsigned char)(double(col.Green()) * 0.5);
    unsigned char b = (unsigned char)(double(col.Blue()) * 0.5);

    col.Set(r, g, b);

    // Fill the page button panel.
    wxBoxSizer* page_panel_sizer = new wxBoxSizer(wxVERTICAL);
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
    scrolled_win->SetSize(EDGE_MARGIN, EDGE_MARGIN, SCROLLED_WIN_WIDTH, DIALOG_HEIGHT - EDGE_MARGIN_V*2);
 
    // Darken the background colour for the page selector panel.
    page_panel->SetBackgroundColour(col);

    // Create the OK/Cancel button panel.
    wxSize button_size = wxButton::GetDefaultSize();
    wxButton* ok_button = new wxButton(this, wxID_OK, "OK",
                                       wxPoint(DIALOG_WIDTH - button_size.GetWidth()*2 - EDGE_MARGIN*4,
                                               DIALOG_HEIGHT - button_size.GetHeight() - EDGE_MARGIN_V));
    ok_button->SetDefault();
    wxButton* cancel_button = new wxButton(this, wxID_CANCEL, "Cancel",
                                           wxPoint(DIALOG_WIDTH - button_size.GetWidth() - EDGE_MARGIN*2,
                                                   DIALOG_HEIGHT - button_size.GetHeight() - EDGE_MARGIN_V));

    // Retrieve the panel for the first page and put it in the correct place.
    PanelDlgPage* first_page = *(pages.begin());
    assert(first_page);
    m_CurrentPage = first_page;
    PositionPage();
    m_CurrentPage->Show();

    // Set a reasonable size and centre the dialog with respect to the parent window.
    SetSize(DIALOG_WIDTH, DIALOG_HEIGHT);
    CentreOnParent();
}

