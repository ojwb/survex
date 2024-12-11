/*
//  aven.h
//
//  Main class for Aven.
//
//  Copyright (C) 2001, Mark R. Shinwell.
//  Copyright (C) 2002,2003,2004,2005,2006,2015 Olly Betts
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
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/* Note: this header needs to be safe to include from C code */

#ifndef aven_h
#define aven_h

#include <stdarg.h>

#define APP_NAME wxT("Aven")
#define APP_ABOUT_IMAGE wxT("aven-about.png")

extern
#ifdef __cplusplus
 "C"
#endif
void aven_v_report(int severity, const char *fnm, int line, int en,
		   va_list ap);

#ifdef __cplusplus

#include "message.h"
#include "wx.h"

#include <string>

inline std::string
string_formatv(const char * fmt, va_list args)
{
    static size_t len = 4096;
    static char * buf = NULL;
    while (true) {
	if (!buf) buf = new char[len];
	int r = vsnprintf(buf, len, fmt, args);
	if (r < int(len) && r != -1) {
	    return std::string(buf, r);
	}
	delete [] buf;
	buf = NULL;
	len += len;
    }
}

inline std::string
string_format(const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::string s = string_formatv(fmt, args);
    va_end(args);
    return s;
}

// wmsg is the unicode version of msg.
inline wxString wmsg(int msg_no) {
    return wxString::FromUTF8(msg(msg_no));
}

const wxString & wmsg_cfgpth();

class MainFrm;

class wxPageSetupDialogData;

class Aven : public wxApp {
    MainFrm * m_Frame = nullptr;
    // This must be a pointer, otherwise it gets initialised too early and
    // we get a segfault on MS Windows when it tries to look up paper
    // sizes in wxThePrintPaperDatabase which is still NULL at the point
    // when the Aven class is constructed.
    wxPageSetupDialogData * m_pageSetupData = nullptr;

public:
    Aven();
    ~Aven();

#ifdef __WXMSW__
    virtual bool Initialize(int& argc, wxChar **argv);
#endif
    virtual bool OnInit();

    wxPageSetupDialogData * GetPageSetupDialogData();
    void SetPageSetupDialogData(const wxPageSetupDialogData & psdd);

#ifdef __WXMAC__
    void MacOpenFiles(const wxArrayString & filenames);
    void MacPrintFiles(const wxArrayString & filenames);
#endif

    void ReportError(const wxString&);
};

DECLARE_APP(Aven)

#endif

#endif
