/* cavernlog.cc
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

#include <config.h>

#include "aven.h"
#include "cavernlog.h"
#include "filename.h"
#include "mainfrm.h"
#include "message.h"
#include "osalloc.h"

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

#include <wx/process.h>

#define GVIM_COMMAND "gvim +'call cursor($l,$c)' $f"
#define VIM_COMMAND "x-terminal-emulator -e vim +'call cursor($l,$c)' $f"
#define NVIM_COMMAND "x-terminal-emulator -e nvim +'call cursor($l,$c)' $f"
#define GEDIT_COMMAND "gedit $f +$l:$c"
// Pluma currently ignores the column, but include it assuming some future
// version will add support.
#define PLUMA_COMMAND "pluma +$l:$c $f"
#define EMACS_COMMAND "x-terminal-emulator -e emacs +$l:$c $f"
#define NANO_COMMAND "x-terminal-emulator -e nano +$l,$c $f"
#define JED_COMMAND "x-terminal-emulator -e jed $f -g $l"
#define KATE_COMMAND "kate -l $l -c $c $f"

#ifdef __WXMSW__
# define DEFAULT_EDITOR_COMMAND "notepad $f"
#elif defined __WXMAC__
# define DEFAULT_EDITOR_COMMAND "open -t $f"
#else
# define DEFAULT_EDITOR_COMMAND VIM_COMMAND
#endif

enum { LOG_REPROCESS = 1234, LOG_SAVE = 1235 };

static const wxString badutf8_html(
    wxT("<span style=\"color:white;background-color:red;\">&#xfffd;</span>"));
static const wxString badutf8(wxUniChar(0xfffd));

// New event type for passing a chunk of cavern output from the worker thread
// to the main thread (or from the idle event handler if we're not using
// threads).
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

void
CavernLogWindow::OnIdle(wxIdleEvent& event)
{
    if (cavern_out == NULL) return;

    wxInputStream * in = cavern_out->GetInputStream();

    if (!in->CanRead()) {
	cout << "!CanRead()\n";
	return;
    }

    CavernOutputEvent * e = new CavernOutputEvent();
    in->Read(e->buf, sizeof(e->buf));
    size_t n = in->LastRead();
    cout << "Read gave " << n << " bytes\n";
    if (n == 0) {
	delete e;
	return;
    }
    e->len = n;
    QueueEvent(e);

    event.RequestMore();
}

void
CavernLogWindow::OnPaint(wxPaintEvent&)
{
    wxPaintDC dc(this);
    wxFont font = dc.GetFont();
    wxFont bold_font = font.Bold();
    wxFont underlined_font = font.Underlined();
    const wxRegion& region = GetUpdateRegion();
    const wxRect& rect = region.GetBox();
    int scroll_x = 0, scroll_y = 0;
    GetViewStart(&scroll_x, &scroll_y);
    int fsize = dc.GetFont().GetPixelSize().GetHeight();
    int limit = min((rect.y + rect.height + fsize - 1) / fsize + scroll_y, int(line_info.size()) - 1);
    for (int i = max(rect.y / fsize, scroll_y); i <= limit ; ++i) {
	LineInfo& info = line_info[i];
	// Leave a small margin to the left.
	int x = fsize / 2 - scroll_x * fsize;
	int y = (i - scroll_y) * fsize;
	unsigned offset = info.start_offset;
	unsigned len = info.len;
	if (info.link_len) {
	    dc.SetFont(underlined_font);
	    dc.SetTextForeground(wxColour(255, 0, 255));
	    wxString link(&log_txt[offset], info.link_len);
	    offset += info.link_len;
	    len -= info.link_len;
	    dc.DrawText(link, x, y);
	    if (info.link_pixel_width == 0) {
		info.link_pixel_width = dc.GetTextExtent(link).GetWidth();
	    }
	    x += info.link_pixel_width;
	    dc.SetFont(font);
	}
	if (info.colour_len) {
	    dc.SetTextForeground(*wxBLACK);
	    {
		size_t s_len = info.start_offset + info.colour_start - offset;
		wxString s(&log_txt[offset], s_len);
		offset += s_len;
		len -= s_len;
		dc.DrawText(s, x, y);
		x += dc.GetTextExtent(s).GetWidth();
	    }
	    switch (info.colour) {
		case LOG_ERROR:
		    dc.SetTextForeground(*wxRED);
		    break;
		case LOG_WARNING:
		    dc.SetTextForeground(wxColour(0xf2, 0x8C, 0x28));
		    break;
		case LOG_INFO:
		    dc.SetTextForeground(*wxBLUE);
		    break;
	    }
	    dc.SetFont(bold_font);
	    wxString d(&log_txt[offset], info.colour_len);
	    offset += info.colour_len;
	    len -= info.colour_len;
	    dc.DrawText(d, x, y);
	    x += dc.GetTextExtent(d).GetWidth();
	    dc.SetFont(font);
	}
	dc.SetTextForeground(*wxBLACK);
	dc.DrawText(wxString(&log_txt[offset], len), x, y);
    }
}

BEGIN_EVENT_TABLE(CavernLogWindow, wxScrolledWindow)
    EVT_BUTTON(LOG_REPROCESS, CavernLogWindow::OnReprocess)
    EVT_BUTTON(LOG_SAVE, CavernLogWindow::OnSave)
    EVT_BUTTON(wxID_OK, CavernLogWindow::OnOK)
    EVT_COMMAND(wxID_ANY, wxEVT_CAVERN_OUTPUT, CavernLogWindow::OnCavernOutput)
    EVT_IDLE(CavernLogWindow::OnIdle)
    EVT_PAINT(CavernLogWindow::OnPaint)
    EVT_MOTION(CavernLogWindow::OnMouseMove)
    EVT_LEFT_UP(CavernLogWindow::OnLinkClicked)
    EVT_END_PROCESS(wxID_ANY, CavernLogWindow::OnEndProcess)
END_EVENT_TABLE()

wxString escape_for_shell(wxString s, bool protect_dash)
{
#ifdef __WXMSW__
    // Correct quoting rules are insane:
    //
    // http://blogs.msdn.com/b/twistylittlepassagesallalike/archive/2011/04/23/everyone-quotes-arguments-the-wrong-way.aspx
    //
    // Thankfully wxExecute passes the command string to CreateProcess(), so
    // at least we don't need to quote for cmd.exe too.
    if (s.empty() || s.find_first_of(wxT(" \"\t\n\v")) != s.npos) {
	// Need to quote.
	s.insert(0, wxT('"'));
	for (size_t p = 1; p < s.size(); ++p) {
	    size_t backslashes = 0;
	    while (s[p] == wxT('\\')) {
		++backslashes;
		if (++p == s.size()) {
		    // Escape all the backslashes, since they're before
		    // the closing quote we add below.
		    s.append(backslashes, wxT('\\'));
		    goto done;
		}
	    }

	    if (s[p] == wxT('"')) {
		// Escape any preceding backslashes and this quote.
		s.insert(p, backslashes + 1, wxT('\\'));
		p += backslashes + 1;
	    }
	}
done:
	s.append(wxT('"'));
    }
#else
    size_t p = 0;
    if (protect_dash && !s.empty() && s[0u] == '-') {
	// If the filename starts with a '-', protect it from being
	// treated as an option by prepending "./".
	s.insert(0, wxT("./"));
	p = 2;
    }
    while (p < s.size()) {
	// Exclude a few safe characters which are common in filenames
	if (!isalnum((unsigned char)s[p]) && strchr("/._-", s[p]) == NULL) {
	    s.insert(p, 1, wxT('\\'));
	    ++p;
	}
	++p;
    }
#endif
    return s;
}

wxString get_command_path(const wxChar * command_name)
{
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
	wchar_t *start = buf;
	if (wcsncmp(start, L"\\\\?\\", 4) == 0) start += 4;
	wchar_t * slash = wcsrchr(start, L'\\');
	if (slash) {
	    cmd.assign(start, slash - start + 1);
	}
	osfree(buf);
    }
#else
    wxString cmd = wxString(msg_exepth(), wxConvUTF8);
#endif
    cmd += command_name;
    return cmd;
}

CavernLogWindow::CavernLogWindow(MainFrm * mainfrm_, const wxString & survey_, wxWindow * parent)
    : wxScrolledWindow(parent),
      mainfrm(mainfrm_),
      end(buf), survey(survey_)
{
    AlwaysShowScrollbars(true, true);
    Update();
}

CavernLogWindow::~CavernLogWindow()
{
    if (cavern_out) {
	wxEndBusyCursor();
	cavern_out->Detach();
    }
}

void
CavernLogWindow::OnMouseMove(wxMouseEvent& e)
{
    const auto& pos = e.GetPosition();
    int fsize = GetFont().GetPixelSize().GetHeight();
    int scroll_x = 0, scroll_y = 0;
    GetViewStart(&scroll_x, &scroll_y);
    unsigned line = pos.y / fsize + scroll_y;
    unsigned x = pos.x + scroll_x * fsize - fsize / 2;
    if (line < line_info.size() && x <= line_info[line].link_pixel_width) {
	SetCursor(wxCursor(wxCURSOR_HAND));
    } else {
	SetCursor(wxNullCursor);
    }
}

void
CavernLogWindow::OnLinkClicked(wxMouseEvent& e)
{
    const auto& pos = e.GetPosition();
    int fsize = GetFont().GetPixelSize().GetHeight();
    int scroll_x = 0, scroll_y = 0;
    GetViewStart(&scroll_x, &scroll_y);
    unsigned line = pos.y / fsize + scroll_y;
    unsigned x = pos.x + scroll_x * fsize - fsize / 2;
    if (!(line < line_info.size() && x <= line_info[line].link_pixel_width))
	return;

    const char* cur = &log_txt[line_info[line].start_offset];
    size_t link_len = line_info[line].link_len;
    size_t colon = link_len;
    while (colon > 1 && (unsigned)(cur[--colon] - '0') <= 9) { }
    size_t colon2 = colon;
    while (colon > 1 && (unsigned)(cur[--colon] - '0') <= 9) { }
    if (cur[colon] != ':') {
	colon = colon2;
	colon2 = link_len;
    }

    wxString cmd;
    wxChar * p = wxGetenv(wxT("SURVEXEDITOR"));
    if (p) {
	cmd = p;
	if (!cmd.find(wxT("$f"))) {
	    cmd += wxT(" $f");
	}
    } else {
	p = wxGetenv(wxT("VISUAL"));
	if (!p) p = wxGetenv(wxT("EDITOR"));
	if (!p) {
	    cmd = wxT(DEFAULT_EDITOR_COMMAND);
	} else {
	    cmd = p;
	    if (cmd == "gvim") {
		cmd = wxT(GVIM_COMMAND);
	    } else if (cmd == "vim") {
		cmd = wxT(VIM_COMMAND);
	    } else if (cmd == "nvim") {
		cmd = wxT(NVIM_COMMAND);
	    } else if (cmd == "gedit") {
		cmd = wxT(GEDIT_COMMAND);
	    } else if (cmd == "pluma") {
		cmd = wxT(PLUMA_COMMAND);
	    } else if (cmd == "emacs") {
		cmd = wxT(EMACS_COMMAND);
	    } else if (cmd == "nano") {
		cmd = wxT(NANO_COMMAND);
	    } else if (cmd == "jed") {
		cmd = wxT(JED_COMMAND);
	    } else if (cmd == "kate") {
		cmd = wxT(KATE_COMMAND);
	    } else {
		// Escape any $.
		cmd.Replace(wxT("$"), wxT("$$"));
		cmd += wxT(" $f");
	    }
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
		wxString f = escape_for_shell(wxString(cur, colon), true);
		cmd.replace(i - 1, 2, f);
		i += f.size() - 1;
		break;
	    }
	    case wxT('l'): {
		wxString l = escape_for_shell(wxString(cur + colon + 1, colon2 - colon - 1));
		cmd.replace(i - 1, 2, l);
		i += l.size() - 1;
		break;
	    }
	    case wxT('c'): {
		wxString l;
		if (colon2 == link_len)
		    l = wxT("0");
		else
		    l = escape_for_shell(wxString(cur + colon2 + 1, link_len - colon2 - 1));
		cmd.replace(i - 1, 2, l);
		i += l.size() - 1;
		break;
	    }
	    default:
		++i;
	}
    }

    if (wxExecute(cmd, wxEXEC_ASYNC|wxEXEC_MAKE_GROUP_LEADER) >= 0)
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
    if (cavern_out) {
	cavern_out->Detach();
	cavern_out = NULL;
    } else {
	wxBeginBusyCursor();
    }

    SetFocus();
    filename = file;

    info_count = 0;
    link_count = 0;
    log_txt.resize(0);
    line_info.resize(0);
    // Reserve enough that we won't need to grow the allocations in normal cases.
    log_txt.reserve(16384);
    line_info.reserve(256);
    ptr = 0;

#ifdef __WXMSW__
    SetEnvironmentVariable(wxT("SURVEX_UTF8"), wxT("1"));
#else
    setenv("SURVEX_UTF8", "1", 1);
#endif

    wxString escaped_file = escape_for_shell(file, true);
    wxString cmd = get_command_path(L"cavern");
    cmd = escape_for_shell(cmd, false);
    cmd += wxT(" -o ");
    cmd += escaped_file;
    cmd += wxT(' ');
    cmd += escaped_file;

    cavern_out = wxProcess::Open(cmd);
    if (!cavern_out) {
	wxString m;
	m.Printf(wmsg(/*Couldn’t run external command: “%s”*/17), cmd.c_str());
	m += wxT(" (");
	m += wxString(strerror(errno), wxConvUTF8);
	m += wxT(')');
	wxGetApp().ReportError(m);
	return;
    }

    // We want to receive the wxProcessEvent when cavern exits.
    cavern_out->SetNextHandler(this);
}

void
CavernLogWindow::OnCavernOutput(wxCommandEvent & e_)
{
    CavernOutputEvent & e = (CavernOutputEvent&)e_;

    if (e.len > 0) {
	ssize_t n = e.len;
	if (size_t(n) > sizeof(buf) - (end - buf)) abort();
	log_txt.append((const char *)e.buf, n);

	// ptr gives the start of the first line we've not yet processed.

	size_t nl;
	while ((nl = log_txt.find('\n', ptr)) != std::string::npos) {
	    if (nl == ptr || (nl - ptr == 1 && log_txt[ptr] == '\r')) {
		// Don't show empty lines in the window.
		ptr = nl + 1;
		continue;
	    }
	    size_t line_len = nl - ptr - (log_txt[nl - 1] == '\r');
	    // FIXME: Avoid copy, use string_view?
	    string cur(log_txt, ptr, line_len);
	    if (log_txt[ptr] == ' ') {
		if (expecting_caret_line) {
		    // FIXME: Check the line is only space, `^` and `~`?
		    // Otherwise an error without caret info followed
		    // by an error which contains a '^' gets
		    // mishandled...
		    size_t caret = cur.rfind('^');
		    if (caret != wxString::npos) {
			size_t tilde = cur.rfind('~');
			if (tilde == wxString::npos || tilde < caret) {
			    tilde = caret;
			}
			line_info.back().colour = line_info[line_info.size() - 2].colour;
			line_info.back().colour_start = caret;
			line_info.back().colour_len = tilde - caret + 1;
			expecting_caret_line = false;
			ptr = nl + 1;
			continue;
		    }
		}
		expecting_caret_line = true;
	    }
	    line_info.emplace_back(ptr);
	    line_info.back().len = line_len;
	    size_t colon = cur.find(": ");
	    if (colon != wxString::npos) {
		size_t link_len = colon;
		while (colon > 1 && (unsigned)(cur[--colon] - '0') <= 9) { }
		if (cur[colon] == ':') {
		    line_info.back().link_len = link_len;

		    static string info_marker = string(msg(/*info*/485)) + ':';
		    static string warning_marker = string(msg(/*warning*/4)) + ':';
		    static string error_marker = string(msg(/*error*/93)) + ':';

		    size_t offset = link_len + 2;
		    if (cur.compare(offset, info_marker.size(), info_marker) == 0) {
			// Show "info" marker in blue.
			++info_count;
			line_info.back().colour = LOG_INFO;
			line_info.back().colour_start = offset;
			line_info.back().colour_len = info_marker.size() - 1;
		    } else if (cur.compare(offset, warning_marker.size(), warning_marker) == 0) {
			// Show "warning" marker in orange.
			line_info.back().colour = LOG_WARNING;
			line_info.back().colour_start = offset;
			line_info.back().colour_len = warning_marker.size() - 1;
		    } else if (cur.compare(offset, error_marker.size(), error_marker) == 0) {
			// Show "error" marker in red.
			line_info.back().colour = LOG_ERROR;
			line_info.back().colour_start = offset;
			line_info.back().colour_len = error_marker.size() - 1;
		    }
		    ++link_count;
		}
	    }

	    int fsize = GetFont().GetPixelSize().GetHeight();
	    SetScrollRate(fsize, fsize);
	    int width = 144; // FIXME
	    int height = line_info.size();
	    SetVirtualSize(width, height * fsize);
	    if (!link_count) {
		// Auto-scroll until the first diagnostic.
		int scroll_x = 0, scroll_y = 0;
		GetViewStart(&scroll_x, &scroll_y);
		int xs, ys;
		GetClientSize(&xs, &ys);
		Scroll(scroll_x, line_info.size() - ys);
	    }
	    ptr = nl + 1;
	}

	Update();
	return;
    }

    /* TRANSLATORS: Label for button in aven’s cavern log window which
     * allows the user to save the log to a file. */
    auto buttons = new wxBoxSizer(wxHORIZONTAL);
    buttons->Add(new wxButton(this, LOG_SAVE, wmsg(/*&Save Log*/446)),
		 0, wxRIGHT | wxBOTTOM, 6);
    wxEndBusyCursor();
    delete cavern_out;
    cavern_out = NULL;
    if (e.len < 0) {
	/* Negative length indicates non-zero exit status from cavern. */
	/* TRANSLATORS: Label for button in aven’s cavern log window which
	 * causes the survey data to be reprocessed. */
	buttons->Add(new wxButton(this, LOG_REPROCESS, wmsg(/*&Reprocess*/184)),
		     0, wxRIGHT | wxBOTTOM, 15);
    } else {
	buttons->Add(new wxButton(this, LOG_REPROCESS, wmsg(/*&Reprocess*/184)),
		     0, wxRIGHT | wxBOTTOM, 6);
	auto ok_button = new wxButton(this, wxID_OK, wxString());
	ok_button->SetDefault();
	buttons->Add(ok_button, 0, wxRIGHT | wxBOTTOM, 15);
    }
    //SetSizer(buttons);
    if (e.len < 0) return;

    Update();
    init_done = false;

    {
	wxString file3d(filename, 0, filename.length() - 3);
	file3d.append(wxT("3d"));
	if (!mainfrm->LoadData(file3d, survey)) {
	    return;
	}
    }

    // Don't stay on log if there there are only "info" diagnostics.
    if (link_count == info_count) {
	wxCommandEvent dummy;
	OnOK(dummy);
    }
}

void
CavernLogWindow::OnEndProcess(wxProcessEvent & evt)
{
    CavernOutputEvent * e = new CavernOutputEvent();
    // Zero length indicates successful exit, negative length unsuccessful exit.
    e->len = (evt.GetExitCode() == 0 ? 0 : -1);
    QueueEvent(e);
}

void
CavernLogWindow::OnReprocess(wxCommandEvent &)
{
    process(filename);
}

void
CavernLogWindow::OnSave(wxCommandEvent &)
{
    wxString filelog(filename, 0, filename.length() - 3);
    filelog += wxT("log");
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
	mainfrm->InitialiseAfterLoad(filename, survey);
	init_done = true;
    }
}
