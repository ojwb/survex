/* cavernlog.cc
 * Run cavern inside an Aven window
 *
 * Copyright (C) 2005,2006,2010,2011,2012 Olly Betts
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

enum { LOG_REPROCESS = 1234 };

BEGIN_EVENT_TABLE(CavernLogWindow, wxHtmlWindow)
    EVT_BUTTON(LOG_REPROCESS, CavernLogWindow::OnReprocess)
    EVT_BUTTON(wxID_OK, CavernLogWindow::OnOK)
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

CavernLogWindow::CavernLogWindow(MainFrm * mainfrm_, wxWindow * parent)
    : wxHtmlWindow(parent), mainfrm(mainfrm_)
{
    int fsize = parent->GetFont().GetPointSize();
    int sizes[7] = { fsize, fsize, fsize, fsize, fsize, fsize, fsize };
    SetFonts(wxString(), wxString(), sizes);
}

void
CavernLogWindow::OnLinkClicked(const wxHtmlLinkInfo &link)
{
    wxString href = link.GetHref();
    wxString title = link.GetTarget();
    size_t colon = href.rfind(wxT(':'));
    if (colon != wxString::npos) {
#ifdef __WXMSW__
	wxString cmd = wxT("notepad $f");
#elif defined __WXMAC__
	wxString cmd = wxT("open -t $f");
#else
	wxString cmd = wxT("x-terminal-emulator -title $t -e vim -c $l $f");
	// wxString cmd = "x-terminal-emulator -title $t -e emacs +$l $f";
	// wxString cmd = "x-terminal-emulator -title $t -e nano +$l $f";
	// wxString cmd = "x-terminal-emulator -title $t -e jed -g $l $f";
#endif
	const char * p = getenv("SURVEXEDITOR");
	if (p) {
	    cmd = wxString(p, wxConvUTF8);
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
		    wxString l = escape_for_shell(href.substr(colon + 1));
		    cmd.replace(i - 1, 2, l);
		    i += l.size() - 1;
		    break;
		}
		default:
		    ++i;
	    }
	}
#ifdef __WXMSW__
	_wsystem(cmd.c_str());
#else
	system(cmd.mb_str());
#endif
    }
}

int
CavernLogWindow::process(const wxString &file)
{
    SetFocus();
    filename = file;

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
    FILE * cavern_out = _wpopen(cmd.c_str(), L"r");
#else
    FILE * cavern_out = popen(cmd.mb_str(), "r");
#endif
    if (!cavern_out) {
	wxString m;
	m.Printf(wmsg(/*Couldn’t open pipe: “%s”*/17), cmd.c_str());
	m += wxT(" (");
	m += wxString(strerror(errno), wxConvUTF8);
	m += wxT(')');
	wxGetApp().ReportError(m);
	return -2;
    }

    int cavern_fd;
#ifdef __WXMSW__
    cavern_fd = _fileno(cavern_out);
#else
    cavern_fd = fileno(cavern_out);
#endif
    assert(cavern_fd < FD_SETSIZE); // FIXME we shouldn't just assert, but what else to do?
    wxString cur;
    int link_count = 0;
    // We're only guaranteed one character of pushback by ungetc() but we
    // need two so for portability we implement the second ourselves.
    int left = EOF;
    while (!feof(cavern_out)) {
	fd_set rfds, efds;
	FD_ZERO(&rfds);
	FD_SET(cavern_fd, &rfds);
	FD_ZERO(&efds);
	FD_SET(cavern_fd, &efds);
	// Timeout instantly.
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	if (select(cavern_fd + 1, &rfds, NULL, &efds, &timeout) == 0) {
	    Update();
	    FD_SET(cavern_fd, &rfds);
	    FD_SET(cavern_fd, &efds);
	    // Set timeout to 0.1 seconds.
	    timeout.tv_sec = 0;
	    timeout.tv_usec = 100000;
	    if (select(cavern_fd + 1, &rfds, NULL, &efds, &timeout) == 0) {
		wxYield();
		continue;
	    }
	}
	if (!FD_ISSET(cavern_fd, &rfds)) {
	    // Error, which pclose() should report.
	    break;
	}
	int ch;
	if (left == EOF) {
	    ch = GETC(cavern_out);
	    if (ch == EOF) break;
	} else {
	    ch = left;
	    left = EOF;
	}
	// Decode UTF-8 first to avoid security issues with <, >, &, etc
	// encoded using multibyte encodings.
	if (ch >= 0xc0 && ch < 0xf0) {
	    int ch1 = GETC(cavern_out);
	    if ((ch1 & 0xc0) != 0x80) {
		left = ch1;
	    } else if (ch < 0xe0) {
		/* 2 byte sequence */
		ch = ((ch & 0x1f) << 6) | (ch1 & 0x3f);
	    } else {
		/* 3 byte sequence */
		int ch2 = GETC(cavern_out);
		if ((ch2 & 0xc0) != 0x80) {
		    ungetc(ch2, cavern_out);
		    left = ch1;
		} else {
		    ch = ((ch & 0x1f) << 12) | ((ch1 & 0x3f) << 6) | (ch2 & 0x3f);
		}
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
			wxString tag = wxT("<a href=\"");
			tag.append(cur, 0, colon);
			while (cur[++i] == wxT(' ')) { }
			tag += wxT("\" target=\"");
			tag.append(cur, i, wxString::npos);
			tag += wxT("\">");
			cur.insert(0, tag);
			cur.insert(colon + tag.size(), wxT("</a>"));
			++link_count;
		    }
		}

		// Save the scrollbar positions.
		int scroll_x = 0, scroll_y = 0;
		GetViewStart(&scroll_x, &scroll_y);

		cur += wxT("<br>\n");
		AppendToPage(cur);

		if (!link_count) {
		    // Auto-scroll the window until we've reported a warning or
		    // error.
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

    int retval = pclose(cavern_out);
    if (retval) {
	AppendToPage(wxString::Format(wxT("<avenbutton default id=1234 name=\"%s\">"),
				      wmsg(/*Reprocess*/184).c_str()));
	if (retval == -1) {
	    wxString m = wxT("Problem running cavern: ");
	    m += wxString(strerror(errno), wxConvUTF8);
	    wxGetApp().ReportError(m);
	    return -2;
	}
	return -1;
    }
    if (link_count) {
	AppendToPage(wxString::Format(wxT("<avenbutton id=1234 name=\"%s\">"),
				      wmsg(/*Reprocess*/184).c_str()));
	AppendToPage(wxString::Format(wxT("<avenbutton default id=%d>"), (int)wxID_OK));
	Update();
    }
    return link_count;
}

void
CavernLogWindow::OnReprocess(wxCommandEvent & e)
{
    SetPage(wxString());
    if (process(filename) == 0) {
	OnOK(e);
    }
}

void
CavernLogWindow::OnOK(wxCommandEvent &)
{
    mainfrm->InitialiseAfterLoad(filename);
}
