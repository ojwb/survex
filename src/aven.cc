//
//  aven.cc
//
//  Main class for Aven.
//
//  Copyright (C) 2001 Mark R. Shinwell.
//  Copyright (C) 2002,2003,2004 Olly Betts
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

#include "aven.h"
#include "mainfrm.h"

#include "cmdline.h"
#include "message.h"

#include <assert.h>
#include <locale.h>
#include <signal.h>

#include <wx/image.h>
#ifdef wxUSE_DISPLAY // wxDisplay was added in wx 2.5
#include <wx/display.h>
#endif

IMPLEMENT_APP(Aven)

Aven::Aven() :
    m_Frame(NULL)
{
}

bool Aven::OnInit()
{
#ifdef __WXMAC__
    // Tell wxMac which the About menu item is so it can be put where MacOS
    // users expect it to be
    wxApp::s_macAboutMenuItemId = menu_HELP_ABOUT;
    // wxMac is supposed to remove this magic command line option (which
    // Finder passes), but the code in 2.4.2 is bogus.  It just decrements
    // argc rather than copying argv down.  But luckily it also fails to
    // set argv[argc] to NULL so we can just recalculate argc, then remove
    // the -psn_* switch ourselves.
    if (argv[argc]) {
	argc = 1;
	while (argv[argc]) ++argc;
    }
    if (argc > 1 && strncmp(argv[1], "-psn_", 5) == 0) {
	--argc;
	memmove(argv + 1, argv + 2, argc * sizeof(char *));
    }
#endif
    msg_init(argv);

    const char *lang = msg_lang2 ? msg_lang2 : msg_lang;
    if (lang) {
	// suppress message box warnings about messages not found
	wxLogNull logNo;
	wxLocale *loc = new wxLocale();
	loc->AddCatalogLookupPathPrefix(msg_cfgpth());
	if (!loc->Init(lang, lang, lang, TRUE, TRUE)) {
	    if (strcmp(lang, "sk") == 0) {
	       // As of 2.2.9, wxWindows has cs but not sk - the two languages
	       // are close, so this makes sense...
	       loc->Init("cs", "cs", "cs", TRUE, TRUE);
	    }
	}
	// The existence of the wxLocale object is enough - no need to keep a
	// pointer to it!
    }
    // Set LC_NUMERIC back to the default - otherwise we break img.c
    // FIXME: eventually stop doing this so that floating point values are
    // displayed correctly for the locale...
    setlocale(LC_NUMERIC, "C");

    wxString survey;

    /* Want --version and a decent --help output, which cmdline does for us.
     * wxCmdLine is much less good.
     */
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
	{HLP_ENCODELONG(0),          "only load the sub-survey with this prefix"},
	{0, 0}
    };

    cmdline_set_syntax_message("[3d file]", NULL);
    cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 0, 1);
    while (true) {
	int opt = cmdline_getopt();
	if (opt == EOF) break;
	if (opt == 's') {
	    survey = optarg;
	}
    }

    wxImage::AddHandler(new wxPNGHandler);

    // Obtain the screen size.
    int x, y;
    int width, height;
#ifdef wxUSE_DISPLAY // wxDisplay was added in wx 2.5
    wxRect geom = wxDisplay().GetGeometry();
    x = geom.x;
    y = geom.y;
    width = geom.width;
    height = geom.height;
#else
    wxClientDisplayRect(&x, &y, &width, &height);
#endif

    // Calculate a reasonable size for our window.
    x += width / 8;
    y += height / 8;
    width = width * 3 / 4;
    height = height * 3 / 4;

    // Create the main window.
    m_Frame = new MainFrm(APP_NAME, wxPoint(x, y), wxSize(width, height));

    if (argv[optind]) {
	m_Frame->OpenFile(wxString(argv[optind]), survey);
    }

    m_Frame->Show(true);
#ifdef _WIN32
    m_Frame->SetFocus();
#endif
    return true;
}

void Aven::ReportError(const wxString& msg)
{
    AvenAllowOnTop ontop(m_Frame);
    wxMessageBox(msg, APP_NAME, wxOK | wxCENTRE | wxICON_EXCLAMATION);
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
