//
//  aven.cc
//
//  Main class for Aven.
//
//  Copyright (C) 2001 Mark R. Shinwell.
//  Copyright (C) 2002,2003,2004,2005,2006 Olly Betts
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
#include "log.h"
#include "mainfrm.h"

#include "cmdline.h"
#include "message.h"

#include <assert.h>
#include <signal.h>

#include <wx/confbase.h>
#include <wx/image.h>
#if wxUSE_DISPLAY
// wxDisplay was added in wx 2.5; but it may not be built for mingw (because
// the header seems to be missing).
#include <wx/display.h>
#endif

static const struct option long_opts[] = {
    /* const char *name; int has_arg (0 no_argument, 1 required_*, 2 optional_*); int *flag; int val; */
    {"survey", required_argument, 0, 's'},
    {"print", no_argument, 0, 'p'},
    {"help", no_argument, 0, HLP_HELP},
    {"version", no_argument, 0, HLP_VERSION},
    {0, 0, 0, 0}
};

#define short_opts "s:p"

static struct help_msg help[] = {
    /*			     <-- */
    {HLP_ENCODELONG(0),          "only load the sub-survey with this prefix"},
    {HLP_ENCODELONG(1),          "print and exit (requires a 3d file)"},
    {0, 0}
};

IMPLEMENT_APP(Aven)

Aven::Aven() :
    m_Frame(NULL), m_pageSetupData(NULL)
{
}

Aven::~Aven()
{
    if (m_pageSetupData) delete m_pageSetupData;
}

#if wxCHECK_VERSION(2,5,1)
static int getopt_first_response = 0;

bool Aven::Initialize(int& my_argc, wxChar **my_argv)
{
#ifdef __WXMAC__
    // Tell wxMac which the About menu item is so it can be put where MacOS
    // users expect it to be.
    wxApp::s_macAboutMenuItemId = menu_HELP_ABOUT;

    // MacOS passes a magic -psn_XXXX command line argument in argv[1] which
    // wx ignores for us, but in wxApp::Initialize() which hasn't been
    // called yet.  So we need to remove it ourselves.
    if (my_argc > 1 && strncmp(my_argv[1], "-psn_", 5) == 0) {
	--my_argc;
	memmove(my_argv + 1, my_argv + 2, my_argc * sizeof(char *));
    }
#endif
    // Call msg_init() and start processing the command line first so that
    // we can respond to --help and --version even without an X display.
    msg_init(my_argv);
    cmdline_set_syntax_message("[3d file]", NULL);
    cmdline_init(my_argc, my_argv, short_opts, long_opts, NULL, help, 0, 1);
    getopt_first_response = cmdline_getopt();
    return wxApp::Initialize(my_argc, my_argv);
}
#endif

bool Aven::OnInit()
{
    wxLog::SetActiveTarget(new MyLogWindow());
#if !wxCHECK_VERSION(2,5,1) && defined __WXMAC__
    // Tell wxMac which the About menu item is so it can be put where MacOS
    // users expect it to be.
    wxApp::s_macAboutMenuItemId = menu_HELP_ABOUT;

    // wxMac is supposed to remove this magic command line option (which
    // Finder passes), but the code in 2.4.2 is bogus.  It just decrements
    // argc rather than copying argv down.  But we get to the command line
    // before wx does, so we can just remove it and then wx does nothing.
    // But luckily it also fails to set argv[argc] to NULL so we can just
    // recalculate argc, then remove the -psn_* switch ourselves.
    // The code was broken differently in 2.5.0, and fixed in 2.5.1.
    if (my_argv[my_argc]) {
	my_argc = 1;
	while (my_argv[my_argc]) ++my_argc;
    }
    if (my_argc > 1 && strncmp(my_argv[1], "-psn_", 5) == 0) {
	--my_argc;
	memmove(my_argv + 1, my_argv + 2, my_argc * sizeof(char *));
    }
#endif

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

    wxString survey;
    bool print_and_exit = false;

    /* Want --version and a decent --help output, which cmdline does for us.
     * wxCmdLine is much less good.
     */
#if !wxCHECK_VERSION(2,5,1)
    msg_init(argv);
    cmdline_set_syntax_message("[3d file]", NULL);
    cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 0, 1);
#endif
    while (true) {
	int opt;
#if wxCHECK_VERSION(2,5,1)
	if (getopt_first_response) {
	    opt = getopt_first_response;
	    getopt_first_response = 0;
	} else {
	    opt = cmdline_getopt();
	}
#else
	opt = cmdline_getopt();
#endif
	if (opt == EOF) break;
	if (opt == 's') {
	    survey = optarg;
	}
	if (opt == 'p') {
	    print_and_exit = true;
	}
    }

    if (print_and_exit && !argv[optind]) {
	cmdline_syntax(); // FIXME : not a helpful error...
	exit(1);
    }

    if (!InitGLVisual(NULL)) {
	wxString m;
	m.Printf(msg(/*This version of %s requires OpenGL to work, but it isn't available*/405), APP_NAME);
	wxMessageBox(m, APP_NAME, wxOK | wxCENTRE | wxICON_EXCLAMATION);
	exit(1);
    }

    wxImage::AddHandler(new wxPNGHandler);

    // Obtain the screen size.
    int x, y;
    int width, height;
#if wxUSE_DISPLAY // wxDisplay was added in wx 2.5
    wxRect geom = wxDisplay().GetGeometry();
    x = geom.x;
    y = geom.y;
    width = geom.width;
    height = geom.height;
#else
    wxClientDisplayRect(&x, &y, &width, &height);
    // Crude fix to help behaviour on multi-monitor displays.
    // Fudge factors are a bit specific to my setup...
    if (width > height * 3 / 2) {
	x += width;
	width = height * 3 / 2;
	x -= width;
    }
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

    if (print_and_exit) {
	wxCommandEvent dummy;
	m_Frame->OnPrint(dummy);
	m_Frame->OnQuit(dummy);
	return true;
    }

    m_Frame->Show(true);
#ifdef _WIN32
    m_Frame->SetFocus();
#endif
    return true;
}

wxPageSetupDialogData *
Aven::GetPageSetupDialogData()
{
    if (!m_pageSetupData) m_pageSetupData = new wxPageSetupDialogData;
#ifdef __WXGTK__
    // Fetch paper margins stored on disk.
    int left, right, top, bottom;
    wxConfigBase * cfg = wxConfigBase::Get();
    // These default margins were chosen by looking at all the .ppd files
    // on my machine.
    cfg->Read("paper_margin_left", &left, 7);
    cfg->Read("paper_margin_right", &right, 7);
    cfg->Read("paper_margin_top", &top, 14);
    cfg->Read("paper_margin_bottom", &bottom, 14);
    m_pageSetupData->SetMarginTopLeft(wxPoint(left, top));
    m_pageSetupData->SetMarginBottomRight(wxPoint(right, bottom));
#endif
    return m_pageSetupData;
}

void
Aven::SetPageSetupDialogData(const wxPageSetupDialogData & psdd)
{
    if (!m_pageSetupData) m_pageSetupData = new wxPageSetupDialogData;
    *m_pageSetupData = psdd;
#ifdef __WXGTK__
    wxPoint topleft = psdd.GetMarginTopLeft();
    wxPoint bottomright = psdd.GetMarginBottomRight();

    // Store user specified paper margins on disk/in registry.
    wxConfigBase * cfg = wxConfigBase::Get();
    cfg->Write("paper_margin_left", topleft.x);
    cfg->Write("paper_margin_right", bottomright.x);
    cfg->Write("paper_margin_top", topleft.y);
    cfg->Write("paper_margin_bottom", bottomright.y);
    cfg->Flush();
#endif
}

void Aven::ReportError(const wxString& msg)
{
    AvenAllowOnTop ontop(m_Frame);
    wxMessageDialog dlg(m_Frame, msg, APP_NAME, wxOK | wxICON_ERROR);
    dlg.ShowModal();
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
