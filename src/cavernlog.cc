/* cavernlog.cc
 * Run cavern inside an Aven window
 *
 * Copyright (C) 2005,2006 Olly Betts
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
#include "message.h"

#include <errno.h>
#include <stdio.h>

// For select():
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

static wxString escape_for_shell(wxString s, bool protect_dash = false)
{
    size_t p = 0;
#ifdef __WXMSW__
    bool needs_quotes = false;
    while (p < s.size()) {
	if (p == '"') {
	    s.insert(p, '\\');
	    ++p;
	    needs_quotes = true;
	}
	++p;
    }
    if (needs_quotes) {
	s.insert(0, '"');
	s += '"';
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
	    s.insert(p, wxT('\\'));
	    ++p;
	}
	++p;
    }
#endif
    return s;
}

static wxString
html_escape(const wxString &str)
{
    wxString res;
    size_t p = 0;
    while (p < str.size()) {
	char ch = str[p++];
	switch (ch) {
	    case '<':
		res += wxT("&lt;");
		continue;
	    case '>':
		res += wxT("&gt;");
		continue;
	    case '&':
		res += wxT("&amp;");
		continue;
	    case '"':
		res += wxT("&quot;");
		continue;
	    default:
		res += ch;
	}
    }
    return res;
}

CavernLogWindow::CavernLogWindow(wxWindow * parent) : wxHtmlWindow(parent) {
    //GetParser()->SetInputEncoding(wxFONTENCODING_UTF8);
    int fsize = parent->GetFont().GetPointSize();
    int sizes[7] = { fsize, fsize, fsize, fsize, fsize, fsize, fsize };
    SetFonts(wxString(), wxString(), sizes);
    //GetParser()->SetInputEncoding(wxFONTENCODING_UTF8);
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
		switch (cmd[i]) {
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
	system(cmd.fn_str());
    }
}

int
CavernLogWindow::process(const wxString &file)
{
    char *cavern = use_path(msg_exepth(), "cavern");
#ifdef __WXMSW__
    SetEnvironmentVariable("SURVEX_UTF8", "1");
#else
    setenv("SURVEX_UTF8", "1", 1);
#endif

    wxString escaped_file = escape_for_shell(file, true);
    wxString cmd = escape_for_shell(wxString(cavern,wxConvUTF8), false);
    cmd += wxT(" -o ");
    cmd += escaped_file;
    cmd += wxT(' ');
    cmd += escaped_file;

    FILE * cavern_out = popen(cmd.fn_str(), "r");
    if (!cavern_out) {
	wxString m;
	m.Printf(wmsg(/*Couldn't open pipe: `%s'*/17), cmd.c_str());
	m += wxT(" (");
	m += wxString(strerror(errno),wxConvUTF8);
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
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 10;
	if (select(cavern_fd + 1, &rfds, NULL, &efds, &timeout) == 0) {
	    wxYield();
	    continue;
	}
	if (!FD_ISSET(cavern_fd, &rfds)) {
	    // Error, which pclose() should report.
	    break;
	}
	int ch;
	if (left == EOF) {
	    ch = getc(cavern_out);
	    if (ch == EOF) break;
	} else {
	    ch = left;
	    left = EOF;
	}
	// Decode UTF-8 first to avoid security issues with <, >, &, etc
	// encoded using multibyte encodings.
	if (ch >= 0xc0 && ch < 0xf0) {
	    int ch1 = getc(cavern_out);
	    if ((ch1 & 0xc0) != 0x80) {
		left = ch1;
	    } else if (ch < 0xe0) {
		/* 2 byte sequence */
		ch = ((ch & 0x1f) << 6) | (ch1 & 0x3f);
	    } else {
		/* 3 byte sequence */
		int ch2 = getc(cavern_out);
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
		size_t colon = cur.find(':');
#ifdef __WXMSW__
		// If the path is "C:\path\to\file.svx" then don't split at the
		// : after the drive letter!  FIXME: better to look for ": "?
		if (colon == 1) colon = cur.find(':', 2);
#endif
		if (colon != wxString::npos) {
		    size_t colon2 = cur.find(':', colon + 1);
		    if (colon2 != wxString::npos && colon2 != cur.size() - 1) {
			wxString href = cur.substr(0, colon2);
			while (++colon2 < cur.size()) {
			    if (cur[colon2] != ' ') break;
			}
			wxString title = cur.substr(colon2);
			cur.insert(colon2, wxT("</a>"));
			wxString tag = wxT("<a href=\"");
			tag += html_escape(href);
			tag += wxT("\" target=\"");
			tag += html_escape(title);
			tag += wxT("\">");
			cur.insert(0, tag);
			++link_count;
		    }
		}

		// Save the scrollbar positions.
		int scroll_x = 0, scroll_y = 0;
		GetViewStart(&scroll_x, &scroll_y);

		cur += wxT("<br>\n");
		AppendToPage(cur);

		// Restore the scrollbar positions.
		Scroll(scroll_x, scroll_y);

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
		    Scroll(-1, y / yu);
		}
		cur.clear();
		wxYield();
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
	    default:
		if (ch >= 128) {
		    cur += wxString::Format(wxT("&#%u;"), ch);
		} else {
		    cur += (char)ch;
		}
	}
	ch = left;
    }
    int retval = pclose(cavern_out);
    if (retval) {
	if (retval == -1) {
	    wxString m = wxT("Problem running cavern: ");
	    m += wxString(strerror(errno), wxConvUTF8);
	    wxGetApp().ReportError(m);
	    return -2;
	}
	wxGetApp().ReportError(wxT("Failed to process survey data - see log window for details"));
	return -1;
    }
    return link_count;
}
