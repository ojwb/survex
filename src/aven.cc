//
//  aven.cxx
//
//  Main class for Aven.
//
//  Copyright (C) 2001, Mark R. Shinwell.
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

#include "aven.h"
#include "mainfrm.h"
#include "avendoc.h"
#include "avenview.h"
#include "aboutdlg.h"

#include "message.h"

#include <assert.h>
#include <locale.h>
#include <signal.h>

#include <wx/image.h>
#include <wx/cmdline.h>
#include <wx/confbase.h>

#ifdef _WIN32
#include <windows.h>
#endif

IMPLEMENT_APP(Aven)

Aven::Aven() :
    m_Frame(NULL)
{
}

bool Aven::OnInit()
{
    msg_init(argv[0]);

    static wxLocale wx_locale;
    if (strcmp(msg_lang, "en") != 0 && strcmp(msg_lang, "en_US") != 0 &&
	!wx_locale.Init(msg_lang, msg_lang, msg_lang, TRUE, TRUE)) {
        if (msg_lang2) {
	    wx_locale.Init(msg_lang2, msg_lang2, msg_lang2, TRUE, TRUE);
        }
    }

    // wxLocale::Init() gives an error box if it doesn't find the catalog,
    // but using FALSE for the penultimate argument and then AddCatalog()
    // if IsOk() doesn't work...

    static wxCmdLineEntryDesc cmdline[] = {
        { wxCMD_LINE_OPTION, "h", "help", msgPerm(/*Display command line options*/201) },
	{ wxCMD_LINE_PARAM,  NULL, NULL, msgPerm(/*3d file*/119), wxCMD_LINE_VAL_STRING,
	  wxCMD_LINE_PARAM_OPTIONAL },
	{ wxCMD_LINE_NONE }
    };

    wxCmdLineParser cli(cmdline, argc, argv);
    int c = cli.Parse();
    if (c != 0 || cli.Found("h")) {
	// write in two goes to avoid msg() overwriting its buffer on 2nd call
        fprintf(stderr, "%s: %s ", msg(/*Syntax*/49), argv[0]);
        fprintf(stderr, "[%s]\n", msg(/*3d file*/119));
	exit(c > 0 ? 1 /* syntax error */ : 0 /* --help */);
    }

    m_Pens = new wxPen[NUM_DEPTH_COLOURS+1];
    m_Brushes = new wxBrush[NUM_DEPTH_COLOURS+1];
    for (int pen = 0; pen < NUM_DEPTH_COLOURS+1; pen++) {
	m_Pens[pen].SetColour(REDS[pen], GREENS[pen], BLUES[pen]);
	m_Brushes[pen].SetColour(REDS[pen], GREENS[pen], BLUES[pen]);
    }

    wxImage::AddHandler(new wxPNGHandler);
    //--need to sort this!
#ifdef _WIN32
    m_AboutBitmap.LoadFile("c:\\aven-about.png", wxBITMAP_TYPE_PNG);
#else
    m_AboutBitmap.LoadFile(wxString(msg_cfgpth()) + wxCONFIG_PATH_SEPARATOR +
			   wxString("aven-about.png"), wxBITMAP_TYPE_PNG);
#endif

    m_DocManager = new wxDocManager;
    (void) new wxDocTemplate(m_DocManager, "Survex 3d files", "*.3d", "", "3d",
			     "Survey Doc", "Survey View",
        CLASSINFO(AvenDoc), CLASSINFO(AvenView));

    m_Frame = new MainFrm(m_DocManager, NULL, -1, "Aven");

    if (cli.GetParamCount() == 1) {
        wxString file = cli.GetParam(0);
	//	m_Frame->OpenFile(file); //--tbs
    }

    m_Frame->Show(true);

    return true;
}

void Aven::ReportError(const wxString& msg)
{
    wxMessageBox(msg, "Aven", wxOK | wxCENTRE | wxICON_EXCLAMATION);
}

void Aven::OnAbout(wxCommandEvent&)
{
    // Display the About box.

    wxDialog* dlg = new AboutDlg(GetMainFrame());
    dlg->Centre();
    dlg->ShowModal();
}
