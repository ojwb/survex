/* cavernlog.h
 * Run cavern inside an Aven window
 *
 * Copyright (C) 2005-2024 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef SURVEX_CAVERNLOG_H
#define SURVEX_CAVERNLOG_H

#include "wx.h"
#include <wx/process.h>

#include <string>
#include <vector>

class MainFrm;

class CavernLogWindow : public wxScrolledWindow {
    wxString filename;

    MainFrm * mainfrm;

    wxProcess * cavern_out = nullptr;
    size_t ptr = 0;
    bool expecting_caret_line = false;
    int info_count = 0;
    int link_count = 0;

    bool init_done = false;

    wxString survey;

    wxTimer timer;

    std::string log_txt;

    enum { LOG_NONE, LOG_ERROR, LOG_WARNING, LOG_INFO };

    class LineInfo {
      public:
	unsigned start_offset = 0;
	unsigned len = 0;
	unsigned link_len = 0;
	unsigned link_pixel_width = 0;
	unsigned colour = LOG_NONE;
	unsigned colour_start = 0;
	unsigned colour_len = 0;

	LineInfo() { }

	explicit LineInfo(unsigned start_offset_)
	    : start_offset(start_offset_) { }
    };

    std::vector<LineInfo> line_info;

  public:
    CavernLogWindow(MainFrm * mainfrm_, const wxString & survey_, wxWindow * parent);

    ~CavernLogWindow();

    /** Start to process survey data in file. */
    void process(const wxString &file);

    void OnMouseMove(wxMouseEvent& e);

    void OnLinkClicked(wxMouseEvent& e);

    void OnReprocess(wxCommandEvent &);

    void OnSave(wxCommandEvent &);

    void OnOK(wxCommandEvent &);

    void OnCavernOutput(wxCommandEvent & e);

    void CheckForOutput();

    void OnIdle(wxIdleEvent&) { CheckForOutput(); }

    void OnTimer(wxTimerEvent&) { CheckForOutput(); }

    void OnPaint(wxPaintEvent&);

    void OnEndProcess(wxProcessEvent & e);

    DECLARE_EVENT_TABLE()
};

wxString escape_for_shell(wxString s, bool protect_dash = false);
wxString get_command_path(const wxChar * command_name);

#endif
