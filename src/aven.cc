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

#include "cmdline.h"
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
    msg_init(argv);

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

    wxString survey;
   
    /* Want --version and a decent --help output, which cmdline does for us */
#ifndef USE_WXCMDLINE
    static const struct option long_opts[] = {
	/* const char *name; int has_arg (0 no_argument, 1 required_*, 2 optional_*); int *flag; int val; */
	{"survey", required_argument, 0, 's'},
	{"help", no_argument, 0, HLP_HELP},
	{"version", no_argument, 0, HLP_VERSION},
	{0, 0, 0, 0}
    };

#define short_opts "s:"

    static struct help_msg help[] = {
	/*			     <-- */
	{HLP_ENCODELONG(0),          "Only load the sub-survey with this prefix"},
	{0, 0}
    };

    cmdline_set_syntax_message("[3d file]", NULL);
    cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 0, 1);
    while (1) {
	int opt = cmdline_getopt();
	if (opt == EOF) break;
	if (opt == 's') {
	    survey = optarg;
	}
    }
#else
    static wxCmdLineEntryDesc cmdline[] = {
	// FIXME - if this code is every reenabled, check this is correct, and also handle it below...
        { wxCMD_LINE_OPTION, "s", "survey", "Only load the sub-survey with this prefix", wxCMD_LINE_VAL_STRING},
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
#endif

    wxImage::AddHandler(new wxPNGHandler);

    m_AboutBitmap.LoadFile(wxString(msg_cfgpth()) + wxCONFIG_PATH_SEPARATOR +
			   wxString("aven-about.png"), wxBITMAP_TYPE_PNG);

    m_Frame = new MainFrm("Aven", wxPoint(50, 50), wxSize(640, 480));

    // FIXME: handle survey

#ifndef USE_WXCMDLINE
    if (argv[optind]) {
	m_Frame->OpenFile(wxString(argv[optind]), survey);
    }
#else
    if (cli.GetParamCount() == 1) {
        wxString file = cli.GetParam(0);
	m_Frame->OpenFile(file, survey);
    }
#endif

    m_Frame->Show(true);

    return true;
}

void Aven::ReportError(const wxString& msg)
{
    wxMessageBox(msg, "Aven", wxOK | wxCENTRE | wxICON_EXCLAMATION);
}

// called to report errors by message.c
extern "C" void
aven_v_report(int severity, const char *fnm, int line, int en, va_list ap)
{
   wxString m;
   if (fnm) {
      m = fnm;
      if (line) m += wxString::Format(":%d", line);
      m += ": ";
   }

   if (severity == 0) {
      m += msg(/*warning*/4);
      m += ": ";
   }

   wxString s;
   s.PrintfV(msg(en), ap);
   m += s;
   wxGetApp().ReportError(m);
}
