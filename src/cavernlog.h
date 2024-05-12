/* cavernlog.h
 * Run cavern inside an Aven window
 *
 * Copyright (C) 2005-2022 Olly Betts
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

// We probably want to use a thread if we can - that way we can use a blocking
// read from cavern rather than busy-waiting via idle events.
#ifdef wxUSE_THREADS
//# define CAVERNLOG_USE_THREADS
#endif

#ifdef CAVERNLOG_USE_THREADS
class CavernThread;
#endif
class MainFrm;

class CavernLogWindow : public wxScrolledWindow {
#ifdef CAVERNLOG_USE_THREADS
    friend class CavernThread;
#endif

    wxString filename;

    MainFrm * mainfrm;

    wxProcess * cavern_out = nullptr;
    size_t ptr = 0;
    bool expecting_caret_line = false;
    int info_count = 0;
    int link_count = 0;
    unsigned char buf[1024];
    unsigned char * end;

    bool init_done = false;

    wxString survey;

    std::string log_txt;

#ifdef CAVERNLOG_USE_THREADS
    void stop_thread();

    CavernThread * thread = nullptr;

    wxCriticalSection thread_lock;
#endif

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

#ifdef CAVERNLOG_USE_THREADS
    void OnClose(wxCloseEvent &);
#else
    void OnIdle(wxIdleEvent &);
#endif

    void OnPaint(wxPaintEvent &);

    void OnEndProcess(wxProcessEvent & e);

    DECLARE_EVENT_TABLE()
};

wxString escape_for_shell(wxString s, bool protect_dash = false);
wxString get_command_path(const wxChar * command_name);

#endif
