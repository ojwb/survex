//
//  prefsdlg.cc
//
//  Preferences dialog box.
//
//  Copyright (C) 2002 Mark R. Shinwell
//  Copyright (C) 2004 Olly Betts
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
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <wx/confbase.h>

#include "ctlprefs.h"
#include "gridprefs.h"
#include "indicatorprefs.h"
#include "legprefs.h"
#include "prefsdlg.h"
#include "stnprefs.h"
#include "tubeprefs.h"
#include "unitsprefs.h"
#include "winprefs.h"
#include "mainfrm.h"

PrefsDlg::PrefsDlg(GfxCore* parent, MainFrm* parent_win) : PanelDlg(parent_win, 1000, "Preferences")
{
    CtlPrefs* ctlprefs = new CtlPrefs(this);
    GridPrefs* gridprefs = new GridPrefs(this);
    LegPrefs* legprefs = new LegPrefs(this);
    StnPrefs* stnprefs = new StnPrefs(parent, this);
    TubePrefs* tubeprefs = new TubePrefs(this);
    UnitsPrefs* unitsprefs = new UnitsPrefs(this);
    IndicatorPrefs* indicatorprefs = new IndicatorPrefs(this);
    WinPrefs* winprefs = new WinPrefs(parent_win, this);

    list<PanelDlgPage*> pages;
    pages.push_back(stnprefs);
    pages.push_back(legprefs);
    pages.push_back(tubeprefs);
    pages.push_back(indicatorprefs);
    pages.push_back(gridprefs);
    pages.push_back(ctlprefs);
    pages.push_back(unitsprefs);
    pages.push_back(winprefs);

    list<PanelDlgPage*>::iterator iter = pages.begin();
    while (iter != pages.end()) {
        (*iter++)->Hide();
    }

    SetPages(pages);
}

wxBitmap PrefsDlg::LoadPreferencesIcon(const wxString& icon) const
{
    // Load an icon for use in the preferences dialog.
    //FIXME share code with LoadIcon...

    const wxString path = wxString(msg_cfgpth()) +
                          wxCONFIG_PATH_SEPARATOR + wxString("icons") +
                          wxCONFIG_PATH_SEPARATOR +
                          wxString(icon) + wxString("prefs.png");
    return wxBitmap(path, wxBITMAP_TYPE_PNG);
}
