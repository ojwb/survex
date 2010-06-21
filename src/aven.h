/*
//  aven.h
//
//  Main class for Aven.
//
//  Copyright (C) 2001, Mark R. Shinwell.
//  Copyright (C) 2002,2003,2004,2005 Olly Betts
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

#define APP_NAME "Aven"

extern
#ifdef __cplusplus
 "C"
#endif
void aven_v_report(int severity, const char *fnm, int line, int en,
		   va_list ap);

#ifdef __cplusplus

#include "wx.h"

class MainFrm;

class Aven : public wxApp {
    MainFrm * m_Frame;
    // This must be a pointer, otherwise it gets initialised too early and
    // we get a segfault on MS Windows when it tries to look up paper
    // sizes in wxThePrintPaperDatabase which is still NULL at the point
    // when the Aven class is constructed.
    wxPageSetupDialogData * m_pageSetupData;

public:
    Aven();
    ~Aven();

    virtual bool OnInit();

    wxPageSetupDialogData * GetPageSetupDialogData();
    void SetPageSetupDialogData(const wxPageSetupDialogData & psdd);

    void ReportError(const wxString&);
};

DECLARE_APP(Aven)

#endif

#endif
