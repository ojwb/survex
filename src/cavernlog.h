/* cavernlog.h
 * Run cavern inside an Aven window
 *
 * Copyright (C) 2005,2006,2010,2015,2016 Olly Betts
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
#include <wx/html/htmlwin.h>
#include <wx/process.h>

#include <string>

// We probably want to use a thread if we can - that way we can use a blocking
// read from cavern rather than busy-waiting via idle events.
#ifdef wxUSE_THREADS
# define CAVERNLOG_USE_THREADS
#endif

#ifdef CAVERNLOG_USE_THREADS
class CavernThread;
#endif
class MainFrm;

class CavernLogWindow : public wxHtmlWindow {
#ifdef CAVERNLOG_USE_THREADS
    friend class CavernThread;
#endif

    wxString filename;

    MainFrm * mainfrm;

    wxProcess * cavern_out;
    wxString cur;
    wxString source_line;
    const wxChar * highlight;
    int link_count;
    unsigned char buf[1024];
    unsigned char * end;

    bool init_done;

    wxString survey;

    std::string log_txt;

#ifdef CAVERNLOG_USE_THREADS
    void stop_thread();

    CavernThread * thread;

    wxCriticalSection thread_lock;
#endif

  public:
    CavernLogWindow(MainFrm * mainfrm_, const wxString & survey_, wxWindow * parent);

    ~CavernLogWindow();

    /** Start to process survey data in file. */
    void process(const wxString &file);

    virtual void OnLinkClicked(const wxHtmlLinkInfo &link);

    void OnReprocess(wxCommandEvent &);

    void OnSave(wxCommandEvent &);

    void OnOK(wxCommandEvent &);

    void OnCavernOutput(wxCommandEvent & e);

#ifdef CAVERNLOG_USE_THREADS
    void OnClose(wxCloseEvent &);
#else
    void OnIdle(wxIdleEvent &);
#endif

    void OnEndProcess(wxProcessEvent & e);

    DECLARE_EVENT_TABLE()
};

wxString escape_for_shell(wxString s, bool protect_dash = false);
wxString get_command_path(const wxChar * command_name);

#endif
