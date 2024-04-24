//  log.cc - Error log window for Aven.
//
//  Copyright (C) 2006,2011,2014 Olly Betts
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
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
//

#include <config.h>

#include "aven.h"
#include "log.h"
#include "gla.h"

MyLogWindow::MyLogWindow()
    /* TRANSLATORS: %s will be replaced by the application name ("Aven"
     * currently) */
    : wxLogWindow(NULL,
		  wxString::Format(wmsg(/*%s Error Log*/228), APP_NAME).c_str(),
		  false, false),
      first(true)
{
}

void MyLogWindow::DoLogText(const wxString & msg) {
    if (first) {
	wxLogWindow::DoLogText(wxString(GetGLSystemDescription().c_str(), wxConvUTF8));
	first = false;
    }
    wxLogWindow::DoLogText(msg);
    Show();
}
