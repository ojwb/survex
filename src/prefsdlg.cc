//
//  prefsdlg.cc
//
//  Preferences dialog box.
//
//  Copyright (C) 2002 Mark R. Shinwell
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

#include "ctlprefs.h"
#include "gridprefs.h"
#include "legprefs.h"
#include "prefsdlg.h"
#include "stnprefs.h"
#include "tubeprefs.h"
#include "unitsprefs.h"

PrefsDlg::PrefsDlg(wxWindow* parent) : PanelDlg(parent, 1000, "Preferences")
{
    CtlPrefs* ctlprefs = new CtlPrefs(this);
    GridPrefs* gridprefs = new GridPrefs(this);
    LegPrefs* legprefs = new LegPrefs(this);
    StnPrefs* stnprefs = new StnPrefs(this);
    TubePrefs* tubeprefs = new TubePrefs(this);
    UnitsPrefs* unitsprefs = new UnitsPrefs(this);

    list<PanelDlgPage*> pages;
    pages.push_back(stnprefs);
    pages.push_back(legprefs);
    pages.push_back(tubeprefs);
    pages.push_back(gridprefs);
    pages.push_back(ctlprefs);
    pages.push_back(unitsprefs);

    SetPages(pages);
}

PrefsDlg::~PrefsDlg()
{

}

