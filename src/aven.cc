//
//  aven.cc
//
//  Main class for Aven.
//
//  Copyright (C) 2001 Mark R. Shinwell.
//  Copyright (C) 2002,2003,2004,2005,2006,2011,2013,2014,2015,2016,2017,2018 Olly Betts
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
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MSG_SETUP_PROJ_SEARCH_PATH 1

#include "aven.h"
#include "log.h"
#include "gla.h"
#include "mainfrm.h"

#include "cmdline.h"
#include "message.h"
#include "useful.h"

#include <assert.h>
#include <stdio.h>

#include <wx/confbase.h>
#include <wx/image.h>
#if wxUSE_DISPLAY
// wxDisplay was added in wx 2.5; but it may not be built for mingw (because
// the header seems to be missing).
#include <wx/display.h>
#endif

#ifdef __WXMSW__
#include <windows.h>
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
    /*				<-- */
    /* TRANSLATORS: --help output for --survey option.
     *
     * "this" has been added to English translation */
    {HLP_ENCODELONG(0),       /*only load the sub-survey with this prefix*/199, 0},
    /* TRANSLATORS: --help output for aven --print option */
    {HLP_ENCODELONG(1),       /*print and exit (requires a 3d file)*/119, 0},
    {0, 0, 0}
};

#ifdef __WXMSW__
IMPLEMENT_APP(Aven)
#else
IMPLEMENT_APP_NO_MAIN(Aven)
IMPLEMENT_WX_THEME_SUPPORT
#endif

Aven::Aven() :
    m_Frame(NULL), m_pageSetupData(NULL)
{
    wxFont::SetDefaultEncoding(wxFONTENCODING_UTF8);
}

Aven::~Aven()
{
    delete m_pageSetupData;
}

static int getopt_first_response = 0;

static char ** utf8_argv;

#ifdef __WXMSW__
bool Aven::Initialize(int& my_argc, wxChar **my_argv)
{
    const wxChar * cmd_line = GetCommandLineW();

    // Horrible bodge to handle therion's assumptions about the "Process"
    // file association.
    if (cmd_line) {
	// None of these are valid aven command line options, so this is not
	// going to be triggered accidentally.
	const wxChar * p = wxStrstr(cmd_line,
				    wxT("aven.exe\" --quiet --log --output="));
	if (p) {
	    // Just change the command name in the command line string - that
	    // way the quoting should match what the C runtime expects.
	    wxString cmd(cmd_line, p - cmd_line);
	    cmd += "cavern";
	    cmd += p + 4;
	    exit(wxExecute(cmd, wxEXEC_SYNC));
	}
    }

    int utf8_argc;
    {
	// wxWidgets doesn't split up the command line in the standard way, so
	// redo it ourselves using the standard API function.
	//
	// Warning: The returned array from this has no terminating NULL
	// element.
	wxChar ** new_argv = NULL;
	if (cmd_line)
	    new_argv = CommandLineToArgvW(cmd_line, &utf8_argc);
	bool failed = (new_argv == NULL);
	if (failed) {
	    wxChar * p;
	    FormatMessage(
		    FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		    NULL,
		    GetLastError(),
		    0,
		    (LPWSTR)&p,
		    4096,
		    NULL);
	    wxString m = "CommandLineToArgvW failed: ";
	    m += p;
	    wxMessageBox(m, APP_NAME, wxOK | wxCENTRE | wxICON_EXCLAMATION);
	    LocalFree(p);
	    utf8_argc = my_argc;
	    new_argv = my_argv;
	}

	// Convert wide characters to UTF-8.
	utf8_argv = new char * [utf8_argc + 1];
	for (int i = 0; i < utf8_argc; ++i){
	    utf8_argv[i] = strdup(wxString(new_argv[i]).utf8_str());
	}
	utf8_argv[utf8_argc] = NULL;

	if (!failed) LocalFree(new_argv);
    }

    msg_init(utf8_argv);
    select_charset(CHARSET_UTF8);
    /* Want --version and decent --help output, which cmdline does for us.
     * wxCmdLine is much less good.
     */
    /* TRANSLATORS: Here "survey" is a "cave map" rather than list of questions
     * - it should be translated to the terminology that cavers using the
     * language would use.
     *
     * Part of aven --help */
    cmdline_set_syntax_message(/*[SURVEY_FILE]*/269, 0, NULL);
    cmdline_init(utf8_argc, utf8_argv, short_opts, long_opts, NULL, help, 0, 1);
    getopt_first_response = cmdline_getopt();

    // The argc and argv arguments don't actually get used here.
    int dummy_argc = 0;
    return wxApp::Initialize(dummy_argc, NULL);
}
#else
int main(int argc, char **argv)
{
#ifdef __WXGTK3__
    // Currently wxGLCanvas doesn't work under Wayland, and the code segfaults.
    // https://trac.wxwidgets.org/ticket/17702
    // Setting GDK_BACKEND=x11 is the recommended workaround, and it seems to
    // work to set it here.  GTK2 doesn't support Wayland, so doesn't need
    // this.
    setenv("GDK_BACKEND", "x11", 1);
    // FIXME: The OpenGL code needs work before scaling on hidpi displays will
    // work usefully, so for now disable such scaling (which simulates how
    // things are when using GTK2).
    setenv("GDK_SCALE", "1", 1);
#endif
#ifdef __WXMAC__
    // MacOS passes a magic -psn_XXXX command line argument in argv[1] which
    // wx ignores for us, but in wxApp::Initialize() which hasn't been
    // called yet.  So we need to remove it ourselves.
    if (argc > 1 && strncmp(argv[1], "-psn_", 5) == 0) {
	--argc;
	memmove(argv + 1, argv + 2, argc * sizeof(char *));
    }
#endif
    // Call msg_init() and start processing the command line first so that
    // we can respond to --help and --version even without an X display.
    msg_init(argv);
    select_charset(CHARSET_UTF8);
    /* Want --version and decent --help output, which cmdline does for us.
     * wxCmdLine is much less good.
     */
    cmdline_set_syntax_message(/*[SURVEY_FILE]*/269, 0, NULL);
    cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 0, 1);
    getopt_first_response = cmdline_getopt();

    utf8_argv = argv;

#if wxUSE_UNICODE
    wxWCharBuffer buf(wxConvFileName->cMB2WX(argv[0]));
    wxChar * wargv[2];
    if (buf) {
	wargv[0] = wxStrdup(buf);
    } else {
	// Eep - couldn't convert the executable's name to wide characters!
	wargv[0] = wxStrdup(APP_NAME);
    }
    wargv[1] = NULL;
    int wargc = 1;
    return wxEntry(wargc, wargv);
#else
    char *dummy_argv[2] = { argv[0], NULL };
    int dummy_argc = 1;
    return wxEntry(dummy_argc, dummy_argv);
#endif
}
#endif

bool Aven::OnInit()
{
    wxLog::SetActiveTarget(new MyLogWindow());

    {
	// Suppress message box warnings about messages not found.
	wxLogNull logNo;
	wxLocale *loc = new wxLocale();
	loc->AddCatalogLookupPathPrefix(wmsg_cfgpth());
	wxString msg_lang_str(msg_lang, wxConvUTF8);
	const char *lang = msg_lang2 ? msg_lang2 : msg_lang;
	wxString lang_str(lang, wxConvUTF8);
	loc->Init(msg_lang_str, lang_str, msg_lang_str);
	// The existence of the wxLocale object is enough - no need to keep a
	// pointer to it!
    }

    const char* opt_survey = NULL;
    bool print_and_exit = false;

    while (true) {
	int opt;
	if (getopt_first_response) {
	    opt = getopt_first_response;
	    getopt_first_response = 0;
	} else {
	    opt = cmdline_getopt();
	}
	if (opt == EOF) break;
	if (opt == 's') {
	    if (opt_survey != NULL) {
		// FIXME: Not a helpful error, but this is temporary until
		// we actually hook up support for specifying multiple
		// --survey options properly here.
		cmdline_syntax();
		exit(1);
	    }
	    opt_survey = optarg;
	}
	if (opt == 'p') {
	    print_and_exit = true;
	}
    }

    if (print_and_exit && !utf8_argv[optind]) {
	cmdline_syntax(); // FIXME : not a helpful error...
	exit(1);
    }

    wxString fnm;
    if (utf8_argv[optind]) {
	fnm = wxString(utf8_argv[optind], wxConvUTF8);
	if (fnm.empty() && *(utf8_argv[optind])) {
	    ReportError(wxT("File argument's filename has bad encoding"));
	    return false;
	}
    }

    if (!GLACanvas::check_visual()) {
	wxString m;
	/* TRANSLATORS: %s will be replaced with "Aven" currently (and
	 * perhaps by "Survex" or other things in future). */
	m.Printf(wmsg(/*This version of %s requires OpenGL to work, but it isn’t available.*/405), APP_NAME);
	wxMessageBox(m, APP_NAME, wxOK | wxCENTRE | wxICON_EXCLAMATION);
	exit(1);
    }

    wxImage::AddHandler(new wxPNGHandler);

    // Obtain the screen geometry.
#if wxUSE_DISPLAY
    wxRect geom = wxDisplay().GetGeometry();
#else
    wxRect geom;
    wxClientDisplayRect(&geom.x, &geom.y, &geom.width, &geom.height);
#endif

    wxPoint pos(wxDefaultPosition);
    int width, height;
    wxConfigBase::Get()->Read(wxT("width"), &width, 0);
    if (width > 0) wxConfigBase::Get()->Read(wxT("height"), &height, 0);
    // We used to persist full screen mode (-1 was maximized,
    // -2 full screen), but people would get stuck in full
    // screen mode, unsure how to exit.
    bool maximized = (width <= -1);
    if (width <= 0 || height <= 0) {
	pos.x = geom.x;
	pos.y = geom.y;
	width = geom.width;
	height = geom.height;

	// Calculate a reasonable size for our window.
	pos.x += width / 8;
	pos.y += height / 8;
	width = width * 3 / 4;
	height = height * 3 / 4;
    } else {
	// Impose a minimum size for sanity, and make sure the window fits on
	// the display (in case the current display is smaller than the one
	// in use when the window size was saved).  (480x320) is about the
	// smallest usable size for aven's window.
	const int min_width = min(geom.width, 480);
	const int min_height = min(geom.height, 320);
	if (width < min_width || height < min_height) {
	    if (width < min_width) {
		width = min_width;
	    }
	    if (height < min_height) {
		height = min_height;
	    }
	    pos.x = geom.x + (geom.width - width) / 4;
	    pos.y = geom.y + (geom.height - height) / 4;
	}
    }

    // Create the main window.
    m_Frame = new MainFrm(APP_NAME, pos, wxSize(width, height));

    // Select maximised if that's the saved state.
    if (maximized) {
	m_Frame->Maximize();
    }

    if (utf8_argv[optind]) {
	if (!opt_survey) opt_survey = "";
	m_Frame->OpenFile(fnm, wxString(opt_survey, wxConvUTF8));
    }

    if (print_and_exit) {
	m_Frame->PrintAndExit();
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
    cfg->Read(wxT("paper_margin_left"), &left, 7);
    cfg->Read(wxT("paper_margin_right"), &right, 7);
    cfg->Read(wxT("paper_margin_top"), &top, 14);
    cfg->Read(wxT("paper_margin_bottom"), &bottom, 14);
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
    cfg->Write(wxT("paper_margin_left"), topleft.x);
    cfg->Write(wxT("paper_margin_right"), bottomright.x);
    cfg->Write(wxT("paper_margin_top"), topleft.y);
    cfg->Write(wxT("paper_margin_bottom"), bottomright.y);
    cfg->Flush();
#endif
}

#ifdef __WXMAC__
void
Aven::MacOpenFiles(const wxArrayString & filenames)
{
    if (filenames.size() != 1) {
	ReportError(wxT("Aven can only load one file at a time"));
	return;
    }
    m_Frame->OpenFile(filenames[0], wxString());
}

void
Aven::MacPrintFiles(const wxArrayString & filenames)
{
    if (filenames.size() != 1) {
	ReportError(wxT("Aven can only print one file at a time"));
	return;
    }
    m_Frame->OpenFile(filenames[0], wxString());
    m_Frame->PrintAndExit();
}
#endif

void Aven::ReportError(const wxString& msg)
{
    if (!m_Frame) {
	wxMessageBox(msg, APP_NAME, wxOK | wxICON_ERROR);
	return;
    }
    wxMessageDialog dlg(m_Frame, msg, APP_NAME, wxOK | wxICON_ERROR);
    dlg.ShowModal();
}

const wxString &
wmsg_cfgpth()
{
    static wxString path;
    if (path.empty())
	path = wxString(msg_cfgpth(), wxConvUTF8);
    return path;
}

// called to report errors by message.c
extern "C" void
aven_v_report(int severity, const char *fnm, int line, int en, va_list ap)
{
    wxString m;
    if (fnm) {
	m = wxString(fnm, wxConvUTF8);
	if (line) m += wxString::Format(wxT(":%d"), line);
	m += wxT(": ");
    }

    if (severity == 0) {
	m += wmsg(/*warning*/4);
	m += wxT(": ");
    }

    char buf[1024];
    vsnprintf(buf, sizeof(buf), msg(en), ap);
    m += wxString(buf, wxConvUTF8);
    if (wxTheApp == NULL) {
	// We haven't initialised the Aven app object yet.
	if (!wxInitialize()) {
	    fputs(buf, stderr);
	    PUTC('\n', stderr);
	    exit(1);
	}
	wxMessageBox(m, APP_NAME, wxOK | wxICON_ERROR);
	wxUninitialize();
    } else {
	wxGetApp().ReportError(m);
    }
}
