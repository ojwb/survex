/* cavernlog.cc
 * Run cavern inside an Aven window
 *
 * Copyright (C) 2005 Olly Betts
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
	s.insert(0, "./");
	p = 2;
    }
    while (p < s.size()) {
	// Exclude a few safe characters which are common in filenames
	if (!isalnum(s[p]) && strchr("/._-", s[p]) == NULL) {
	    s.insert(p, '\\');
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
	        res += "&lt;";
	        continue;
	    case '>':
	        res += "&gt;";
	        continue;
	    case '&':
	        res += "&amp;";
	        continue;
	    case '"':
	        res += "&quot;";
	        continue;
	    default:
	        res += ch;
	}
    }
    return res;
}

void
CavernLogWindow::OnLinkClicked(const wxHtmlLinkInfo &link)
{
    wxString href = link.GetHref();
    wxString title = link.GetTarget();
    size_t colon = href.rfind(':');
    if (colon != wxString::npos) {
#ifdef __WXMSW__
	wxString cmd = "notepad $f";
#else
	wxString cmd = "x-terminal-emulator -title $t -e vim -c $l $f";
	// wxString cmd = "x-terminal-emulator -title $t -e emacs +$l $f";
	// wxString cmd = "x-terminal-emulator -title $t -e nano +$l $f";
	// wxString cmd = "x-terminal-emulator -title $t -e jed -g $l $f";
#endif
	const char *p = getenv("SURVEXEDITOR");
	if (p) {
	    cmd = p;
	    if (!cmd.find("$f")) {
		cmd += " $f";
	    }
	}
	size_t i = 0;
	while ((i = cmd.find('$', i)) != wxString::npos) {
	    if (++i >= cmd.size()) break;
		switch (cmd[i]) {
		    case '$':
			cmd.erase(i, 1);
			break;
		    case 'f': {
			wxString f = escape_for_shell(href.substr(0, colon), true);
			cmd.replace(i - 1, 2, f);
			i += f.size() - 1;
			break;
		    }
		    case 't': {
			wxString t = escape_for_shell(title);
			cmd.replace(i - 1, 2, t);
			i += t.size() - 1;
			break;
		    }
		    case 'l': {
			wxString l = escape_for_shell(href.substr(colon + 1));
			cmd.replace(i - 1, 2, l);
			i += l.size() - 1;
			break;
		    }
		    default:
			++i;
		}
	}
	system(cmd);
    }
}

bool
CavernLogWindow::process(const wxString &file)
{
    char *cavern = use_path(msg_exepth(), "cavern");
#ifdef __WXMSW__
    SetEnvironmentVariable("SURVEX_UTF8", "1");
#else
    setenv("SURVEX_CHARSET", "utf8", 1);
#endif
    wxString escaped_file = escape_for_shell(file, true);
    wxString cmd = escape_for_shell(cavern, false);
    cmd += " -o ";
    cmd += escaped_file;
    cmd += ' ';
    cmd += escaped_file;

    FILE * cavern_out = popen(cmd.c_str(), "r");
    if (!cavern_out) {
	wxString m = wxString::Format(msg(/*Couldn't open pipe: `%s'*/17), cmd.c_str());
	m += " (";
	m += strerror(errno);
	m += ')';
	wxGetApp().ReportError(m);
	return false;
    }

    int cavern_fd;
#ifdef __WXMSW__
    cavern_fd = _fileno(cavern_out);
#else
    cavern_fd = fileno(cavern_out);
#endif
    assert(cavern_fd < FD_SETSIZE); // FIXME we shouldn't just assert, but what else to do?
    wxString cur;
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
	if (!FD_SET(cavern_fd, &rfds)) {
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
			cur.insert(colon2, "</a>");
			wxString tag = "<a href=\"";
			tag += html_escape(href);
			tag += "\" target=\"";
			tag += html_escape(title);
			tag += "\">";
			cur.insert(0, tag);
		    }
		}
		AppendToPage(cur);
		AppendToPage("<br>\n");
#if 0
		// FIXME: do we want to scroll to the bottom?  Doing so seems
		// unhelpful if there are warnings or errors...
		int x, y;
		GetVirtualSize(&x, &y);
		//printf("vs %dx%d\n", x, y);
		int xs, ys;
		GetClientSize(&xs, &ys);
		//printf("cs %dx%d\n", xs, ys);
		y -= ys;
		int xu, yu;
		GetScrollPixelsPerUnit(&xu, &yu);
		//printf("ppu %dx%d\n", xu, yu);
		Scroll(-1, y / yu);
		//cout << "[" << cur << "]" << endl;
#endif
		cur.clear();
		wxYield();
		break;
	    }
	    case '<':
		cur += "&lt;";
		break;
	    case '>':
		cur += "&gt;";
		break;
	    case '&':
		cur += "&amp;";
		break;
	    default:
		if (ch >= 128) {
		    cur += wxString::Format("&#%u;", ch);
		} else {
		    cur += (char)ch;
		}
	}
	ch = left;
    }
    if (pclose(cavern_out) == -1) {
	// FIXME: improve error message.
	wxGetApp().ReportError("Failed to process survey data");
	return false;
    }
    return true;
}
