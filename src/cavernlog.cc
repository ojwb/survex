/* cavernlog.cc
 * Run cavern inside an Aven window
 *
 * Copyright (C) 2005,2006,2010,2011,2012,2014,2015 Olly Betts
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "aven.h"
#include "cavernlog.h"
#include "filename.h"
#include "mainfrm.h"
#include "message.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

// For select():
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

enum { LOG_REPROCESS = 1234, LOG_SAVE = 1235 };

#ifdef wxUSE_THREADS
// New event type for passing a chunk of cavern output from the worker thread
// to the main thread.
class CavernOutputEvent;

wxDEFINE_EVENT(wxEVT_CAVERN_OUTPUT, CavernOutputEvent);

class CavernOutputEvent : public wxEvent {
  public:
    char buf[1000];
    int len;
    CavernOutputEvent() : wxEvent(0, wxEVT_CAVERN_OUTPUT), len(0) { }

    wxEvent * Clone() const {
	CavernOutputEvent * e = new CavernOutputEvent();
	e->len = len;
	if (len > 0) memcpy(e->buf, buf, len);
	return e;
    }
};

class CavernThread : public wxThread {
  protected:
    virtual ExitCode Entry();

    CavernLogWindow *handler;

    int cavern_fd;

  public:
    CavernThread(CavernLogWindow *handler_, int fd)
	: wxThread(wxTHREAD_DETACHED), handler(handler_), cavern_fd(fd) { }

    ~CavernThread() {
	wxCriticalSectionLocker enter(handler->thread_lock);
	handler->thread = NULL;
    }
};

wxThread::ExitCode
CavernThread::Entry()
{
    ssize_t n;
    do {
	CavernOutputEvent * e = new CavernOutputEvent();
	e->len = n = read(cavern_fd, e->buf, sizeof(e->buf));
	if (TestDestroy()) {
	    delete e;
	    break;
	}
	wxQueueEvent(handler, e);
    } while (n > 0);
    return (wxThread::ExitCode)0;
}
#endif

BEGIN_EVENT_TABLE(CavernLogWindow, wxHtmlWindow)
    EVT_BUTTON(LOG_REPROCESS, CavernLogWindow::OnReprocess)
    EVT_BUTTON(LOG_SAVE, CavernLogWindow::OnSave)
    EVT_BUTTON(wxID_OK, CavernLogWindow::OnOK)
#ifdef wxUSE_THREADS
    EVT_CLOSE(CavernLogWindow::OnClose)
    EVT_COMMAND(wxID_ANY, wxEVT_CAVERN_OUTPUT, CavernLogWindow::OnCavernOutput)
#else
    EVT_IDLE(CavernLogWindow::OnIdle)
#endif
END_EVENT_TABLE()

static wxString escape_for_shell(wxString s, bool protect_dash = false)
{
    size_t p = 0;
#ifdef __WXMSW__
    bool needs_quotes = false;
    while (p < s.size()) {
	wxChar ch = s[p];
	if (ch < 127) {
	    if (ch == wxT('"')) {
		s.insert(p, 1, wxT('\\'));
		++p;
		needs_quotes = true;
	    } else if (strchr(" <>&|^", ch)) {
		needs_quotes = true;
	    }
	}
	++p;
    }
    if (needs_quotes) {
	s.insert(0u, 1, wxT('"'));
	s += wxT('"');
    }
#else
    if (protect_dash && !s.empty() && s[0u] == '-') {
	// If the filename starts with a '-', protect it from being
	// treated as an option by prepending "./".
	s.insert(0, wxT("./"));
	p = 2;
    }
    while (p < s.size()) {
	// Exclude a few safe characters which are common in filenames
	if (!isalnum(s[p]) && strchr("/._-", s[p]) == NULL) {
	    s.insert(p, 1, wxT('\\'));
	    ++p;
	}
	++p;
    }
#endif
    return s;
}

CavernLogWindow::CavernLogWindow(MainFrm * mainfrm_, const wxString & survey_, wxWindow * parent)
    : wxHtmlWindow(parent), mainfrm(mainfrm_), cavern_out(NULL),
      link_count(0), end(buf), init_done(false), survey(survey_)
#ifdef wxUSE_THREADS
      , thread(NULL)
#endif
{
    int fsize = parent->GetFont().GetPointSize();
    int sizes[7] = { fsize, fsize, fsize, fsize, fsize, fsize, fsize };
    SetFonts(wxString(), wxString(), sizes);
}

CavernLogWindow::~CavernLogWindow()
{
    if (cavern_out) {
	wxEndBusyCursor();
	pclose(cavern_out);
    }
}

#ifdef wxUSE_THREADS
void
CavernLogWindow::stop_thread()
{
    {
	wxCriticalSectionLocker enter(thread_lock);
	if (thread) {
	    if (thread->Delete() != wxTHREAD_NO_ERROR) {
		// FIXME
	    }
	}
    }

    // Wait for thread to complete.
    while (true) {
	{
	    wxCriticalSectionLocker enter(thread_lock);
	    if (!thread) break;
	}
	wxMilliSleep(1);
    }
}

void
CavernLogWindow::OnClose(wxCloseEvent &)
{
    if (thread) stop_thread();
    Destroy();
}
#endif

void
CavernLogWindow::OnLinkClicked(const wxHtmlLinkInfo &link)
{
    wxString href = link.GetHref();
    wxString title = link.GetTarget();
    size_t colon = href.rfind(wxT(':'));
    if (colon == wxString::npos)
	return;
    size_t colon2 = href.rfind(wxT(':'), colon - 1);
    if (colon2 != wxString::npos) swap(colon, colon2);
#ifdef __WXMSW__
    wxString cmd = wxT("notepad $f");
#elif defined __WXMAC__
    wxString cmd = wxT("open -t $f");
#else
    wxString cmd = wxT("x-terminal-emulator -title $t -e vim +'call cursor($l,$c)' $f");
    // wxString cmd = wxT("gedit -b $f +$l:$c $f");
    // wxString cmd = wxT("x-terminal-emulator -title $t -e emacs +$l $f");
    // wxString cmd = wxT("x-terminal-emulator -title $t -e nano +$l $f");
    // wxString cmd = wxT("x-terminal-emulator -title $t -e jed -g $l $f");
#endif
    wxChar * p = wxGetenv(wxT("SURVEXEDITOR"));
    if (p) {
	cmd = p;
	if (!cmd.find(wxT("$f"))) {
	    cmd += wxT(" $f");
	}
    }
    size_t i = 0;
    while ((i = cmd.find(wxT('$'), i)) != wxString::npos) {
	if (++i >= cmd.size()) break;
	switch ((int)cmd[i]) {
	    case wxT('$'):
		cmd.erase(i, 1);
		break;
	    case wxT('f'): {
		wxString f = escape_for_shell(href.substr(0, colon), true);
		cmd.replace(i - 1, 2, f);
		i += f.size() - 1;
		break;
	    }
	    case wxT('t'): {
		wxString t = escape_for_shell(title);
		cmd.replace(i - 1, 2, t);
		i += t.size() - 1;
		break;
	    }
	    case wxT('l'): {
		wxString l = escape_for_shell(href.substr(colon + 1, colon2 - colon - 1));
		cmd.replace(i - 1, 2, l);
		i += l.size() - 1;
		break;
	    }
	    case wxT('c'): {
		wxString l;
		if (colon2 == wxString::npos)
		    l = wxT("0");
		else
		    l = escape_for_shell(href.substr(colon2 + 1));
		cmd.replace(i - 1, 2, l);
		i += l.size() - 1;
		break;
	    }
	    default:
		++i;
	}
    }
    if (wxSystem(cmd) >= 0)
	return;
    wxString m;
    // TRANSLATORS: %s is replaced by the command we attempted to run.
    m.Printf(wmsg(/*Couldn’t run external command: “%s”*/17), cmd.c_str());
    m += wxT(" (");
    m += wxString(strerror(errno), wxConvUTF8);
    m += wxT(')');
    wxGetApp().ReportError(m);
}

void
CavernLogWindow::process(const wxString &file)
{
#ifdef wxUSE_THREADS
    if (thread) stop_thread();
#endif
    if (cavern_out) {
	pclose(cavern_out);
    } else {
	wxBeginBusyCursor();
    }

    SetFocus();
    filename = file;

    link_count = 0;
    cur.resize(0);
    log_txt.resize(0);

#ifdef __WXMSW__
    SetEnvironmentVariable(wxT("SURVEX_UTF8"), wxT("1"));
#else
    setenv("SURVEX_UTF8", "1", 1);
#endif

    wxString escaped_file = escape_for_shell(file, true);
#ifdef __WXMSW__
    wxString cmd;
    {
	DWORD len = 256;
	wchar_t *buf = NULL;
	while (1) {
	    DWORD got;
	    buf = (wchar_t*)osrealloc(buf, len * 2);
	    got = GetModuleFileNameW(NULL, buf, len);
	    if (got < len) break;
	    len += len;
	}
	/* Strange Win32 nastiness - strip prefix "\\?\" if present */
	if (wcsncmp(buf, L"\\\\?\\", 4) == 0) buf += 4;
	wchar_t * slash = wcsrchr(buf, L'\\');
	if (slash) {
	    cmd.assign(buf, slash - buf + 1);
	}
    }
    cmd += L"cavern";
    cmd = escape_for_shell(cmd, false);
#else
    char *cavern = use_path(msg_exepth(), "cavern");
    wxString cmd = escape_for_shell(wxString(cavern, wxConvUTF8), false);
    osfree(cavern);
#endif
    cmd += wxT(" -o ");
    cmd += escaped_file;
    cmd += wxT(' ');
    cmd += escaped_file;

#ifdef __WXMSW__
    cavern_out = _wpopen(cmd.c_str(), L"r");
#else
    cavern_out = popen(cmd.mb_str(), "r");
#endif
    if (!cavern_out) {
	wxString m;
	m.Printf(wmsg(/*Couldn’t run external command: “%s”*/17), cmd.c_str());
	m += wxT(" (");
	m += wxString(strerror(errno), wxConvUTF8);
	m += wxT(')');
	wxGetApp().ReportError(m);
	return;
    }
#ifndef wxUSE_THREADS
}

void
CavernLogWindow::OnIdle(wxIdleEvent& event)
{
    if (cavern_out == NULL) return;
#endif

    int cavern_fd;
#ifdef __WXMSW__
    cavern_fd = _fileno(cavern_out);
#else
    cavern_fd = fileno(cavern_out);
#endif
#ifdef wxUSE_THREADS
    thread = new CavernThread(this, cavern_fd);
    if (thread->Run() != wxTHREAD_NO_ERROR) {
	wxGetApp().ReportError(wxT("Thread failed to start"));
	delete thread;
	thread = NULL;
    }
}

void
CavernLogWindow::OnCavernOutput(wxCommandEvent & e_)
{
    CavernOutputEvent & e = (CavernOutputEvent&)e_;

    if (e.len > 0) {
	ssize_t n = e.len;
	if (n > sizeof(buf) - (end - buf)) abort();
	memcpy(end, e.buf, n);
#else
    ssize_t n;
#ifndef __WXMSW__
    assert(cavern_fd < FD_SETSIZE); // FIXME we shouldn't just assert, but what else to do?

    fd_set rfds, efds;
    FD_ZERO(&rfds);
    FD_ZERO(&efds);
    FD_SET(cavern_fd, &rfds);
    FD_SET(cavern_fd, &efds);
    // Wait up to 0.01 seconds for data to avoid a tight idle loop.
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;
    int r = select(cavern_fd + 1, &rfds, NULL, &efds, &timeout);
    if (r == 0) {
	// No new output to process.
	event.RequestMore();
	return;
    }
    if (r <= 0 || !FD_ISSET(cavern_fd, &rfds))
	goto abort;
#endif
    n = read(cavern_fd, end, sizeof(buf) - (end - buf));
    if (n > 0) {
#endif
	log_txt.append((const char *)end, n);
	end += n;

	const unsigned char * p = buf;

	while (p != end) {
	    int ch = *p++;
	    if (ch >= 0x80) {
		// Decode multi-byte UTF-8 sequence.
		if (ch < 0xc0) {
		    // FIXME: Invalid UTF-8 sequence.
		    goto abort;
		} else if (ch < 0xe0) {
		    /* 2 byte sequence */
		    if (p == end) {
			// Incomplete UTF-8 sequence - try to read more.
			break;
		    }
		    int ch1 = *p++;
		    if ((ch1 & 0xc0) != 0x80) {
			// FIXME: Invalid UTF-8 sequence.
			goto abort;
		    }
		    ch = ((ch & 0x1f) << 6) | (ch1 & 0x3f);
		} else if (ch < 0xf0) {
		    /* 3 byte sequence */
		    if (end - p <= 1) {
			// Incomplete UTF-8 sequence - try to read more.
			break;
		    }
		    int ch1 = *p++;
		    ch = ((ch & 0x1f) << 12) | ((ch1 & 0x3f) << 6);
		    if ((ch1 & 0xc0) != 0x80) {
			// FIXME: Invalid UTF-8 sequence.
			goto abort;
		    }
		    int ch2 = *p++;
		    if ((ch2 & 0xc0) != 0x80) {
			// FIXME: Invalid UTF-8 sequence.
			goto abort;
		    }
		    ch |= (ch2 & 0x3f);
		} else {
		    // FIXME: Overlong UTF-8 sequence.
		    goto abort;
		}
	    }

	    switch (ch) {
		case '\r':
		    // Ignore.
		    break;
		case '\n': {
		    if (cur.empty()) continue;
#ifndef __WXMSW__
		    size_t colon = cur.find(':');
#else
		    // If the path is "C:\path\to\file.svx" then don't split at the
		    // : after the drive letter!  FIXME: better to look for ": "?
		    size_t colon = cur.find(':', 2);
#endif
		    if (colon != wxString::npos && colon < cur.size() - 2) {
			++colon;
			size_t i = colon;
			while (i < cur.size() - 2 &&
			       cur[i] >= wxT('0') && cur[i] <= wxT('9')) {
			    ++i;
			}
			if (i > colon && cur[i] == wxT(':') ) {
			    colon = i;
			    // Check for column number.
			    while (++i < cur.size() - 2 &&
			       cur[i] >= wxT('0') && cur[i] <= wxT('9')) { }
			    if (i > colon + 1 && cur[i] == wxT(':') ) {
				colon = i;
			    }
			    wxString tag = wxT("<a href=\"");
			    tag.append(cur, 0, colon);
			    while (cur[++i] == wxT(' ')) { }
			    tag += wxT("\" target=\"");
			    tag.append(cur, i, wxString::npos);
			    tag += wxT("\">");
			    cur.insert(0, tag);
			    size_t offset = colon + tag.size();
			    cur.insert(offset, wxT("</a>"));
			    offset += 4 + 2;

			    static const wxString & error_marker = wmsg(/*error*/93) + ":";
			    static const wxString & warning_marker = wmsg(/*warning*/4) + ":";

			    if (cur.substr(offset, error_marker.size()) == error_marker) {
				// Show "error" marker in red.
				cur.insert(offset, wxT("<span style=\"color:red\">"));
				offset += 24 + error_marker.size() - 1;
				cur.insert(offset, wxT("</span>"));
			    } else if (cur.substr(offset, warning_marker.size()) == warning_marker) {
				// Show "warning" marker in orange.
				cur.insert(offset, wxT("<span style=\"color:orange\">"));
				offset += 27 + warning_marker.size() - 1;
				cur.insert(offset, wxT("</span>"));
			    }

			    ++link_count;
			}
		    }

		    // Save the scrollbar positions.
		    int scroll_x = 0, scroll_y = 0;
		    GetViewStart(&scroll_x, &scroll_y);

		    cur += wxT("<br>\n");
		    AppendToPage(cur);

		    if (!link_count) {
			// Auto-scroll the window until we've reported a
			// warning or error.
			int x, y;
			GetVirtualSize(&x, &y);
			int xs, ys;
			GetClientSize(&xs, &ys);
			y -= ys;
			int xu, yu;
			GetScrollPixelsPerUnit(&xu, &yu);
			Scroll(scroll_x, y / yu);
		    } else {
			// Restore the scrollbar positions.
			Scroll(scroll_x, scroll_y);
		    }

		    cur.clear();
		    break;
		}
		case '<':
		    cur += wxT("&lt;");
		    break;
		case '>':
		    cur += wxT("&gt;");
		    break;
		case '&':
		    cur += wxT("&amp;");
		    break;
		case '"':
		    cur += wxT("&#22;");
		    continue;
		default:
		    if (ch >= 128) {
			cur += wxString::Format(wxT("&#%u;"), ch);
		    } else {
			cur += (char)ch;
		    }
	    }
	}

	size_t left = end - p;
	end = buf + left;
	if (left) memmove(buf, p, left);
	Update();
#ifndef wxUSE_THREADS
	event.RequestMore();
#endif
	return;
    }

#ifdef wxUSE_THREADS
    if (e.len == 0 && buf != end) {
	// FIXME: Truncated UTF-8 sequence.
    }
#else
    if (n == 0 && buf != end) {
	// FIXME: Truncated UTF-8 sequence.
    }
#endif
abort:

    /* TRANSLATORS: Label for button in aven’s cavern log window which
     * allows the user to save the log to a file. */
    AppendToPage(wxString::Format(wxT("<avenbutton id=%d name=\"%s\">"),
				  (int)LOG_SAVE,
				  wmsg(/*Save Log*/446).c_str()));
    wxEndBusyCursor();
    int retval = pclose(cavern_out);
    cavern_out = NULL;
    if (retval) {
	/* TRANSLATORS: Label for button in aven’s cavern log window which
	 * causes the survey data to be reprocessed. */
	AppendToPage(wxString::Format(wxT("<avenbutton default id=%d name=\"%s\">"),
				      (int)LOG_REPROCESS,
				      wmsg(/*Reprocess*/184).c_str()));
	if (retval == -1) {
	    wxString m = wxT("Problem running cavern: ");
	    m += wxString(strerror(errno), wxConvUTF8);
	    wxGetApp().ReportError(m);
	    return;
	}
	return;
    }
    AppendToPage(wxString::Format(wxT("<avenbutton id=%d name=\"%s\">"),
				  (int)LOG_REPROCESS,
				  wmsg(/*Reprocess*/184).c_str()));
    AppendToPage(wxString::Format(wxT("<avenbutton default id=%d>"), (int)wxID_OK));
    Update();
    init_done = false;

    wxString file3d(filename, 0, filename.length() - 3);
    file3d.append(wxT("3d"));
    if (!mainfrm->LoadData(file3d, survey)) {
	return;
    }
    if (link_count == 0) {
	wxCommandEvent dummy;
	OnOK(dummy);
    }
}

void
CavernLogWindow::OnReprocess(wxCommandEvent &)
{
    SetPage(wxString());
    process(filename);
}

void
CavernLogWindow::OnSave(wxCommandEvent &)
{
    wxString filelog(filename, 0, filename.length() - 3);
    filelog += wxT("log");
    AvenAllowOnTop ontop(mainfrm);
#ifdef __WXMOTIF__
    wxString ext(wxT("*.log"));
#else
    /* TRANSLATORS: Log files from running cavern (extension .log) */
    wxString ext = wmsg(/*Log files*/447);
    ext += wxT("|*.log");
#endif
    wxFileDialog dlg(this, wmsg(/*Select an output filename*/319),
		     wxString(), filelog, ext,
		     wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;
    filelog = dlg.GetPath();
    FILE * fh_log = wxFopen(filelog, wxT("w"));
    if (!fh_log) {
	wxGetApp().ReportError(wxString::Format(wmsg(/*Error writing to file “%s”*/110), filelog.c_str()));
	return;
    }
    fwrite(log_txt.data(), log_txt.size(), 1, fh_log);
    fclose(fh_log);
}

void
CavernLogWindow::OnOK(wxCommandEvent &)
{
    if (init_done) {
	mainfrm->HideLog(this);
    } else {
	mainfrm->InitialiseAfterLoad(filename);
	init_done = true;
    }
}
