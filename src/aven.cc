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
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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

bool double_buffered = false;

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
    {HLP_ENCODELONG(0),       /*only load the sub-survey with this prefix*/199, 0},
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

#ifdef __WXMSW__
bool Aven::Initialize(int& my_argc, wxChar **my_argv)
{
    // wxWidgets passes us wxChars, which may be wide characters but cmdline
    // wants UTF-8 so we need to convert.
    char *utf8_argv[my_argc + 1];
    for (int i = 0; i < my_argc; ++i){
	utf8_argv[i] = strdup(wxString(my_argv[i]).mb_str());
    }
    utf8_argv[my_argc] = NULL;

    msg_init(utf8_argv);
    select_charset(CHARSET_UTF8);
    /* Want --version and decent --help output, which cmdline does for us.
     * wxCmdLine is much less good.
     */
    cmdline_set_syntax_message(/*[SURVEY_FILE]*/269, 0, NULL);
    cmdline_init(my_argc, utf8_argv, short_opts, long_opts, NULL, help, 0, 1);
    getopt_first_response = cmdline_getopt();
    return wxApp::Initialize(my_argc, my_argv);
}
#define real_argv argv
#else
static char ** real_argv;

int main(int argc, char **argv) 
{
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

    real_argv = argv;

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
	loc->Init(msg_lang_str, lang_str, msg_lang_str, TRUE, TRUE);
	// The existence of the wxLocale object is enough - no need to keep a
	// pointer to it!
    }

    wxString survey;
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
	    survey = wxString(optarg, wxConvUTF8);
	}
	if (opt == 'p') {
	    print_and_exit = true;
	}
    }

    if (print_and_exit && !real_argv[optind]) {
	cmdline_syntax(); // FIXME : not a helpful error...
	exit(1);
    }

    wxString fnm;
    if (real_argv[optind]) {
	fnm = wxString(real_argv[optind], wxConvUTF8);
	if (fnm.empty() && *(real_argv[optind])) {
	    wxGetApp().ReportError(wxT("File argument's filename has bad encoding"));
	    return false;
	}
    }

    // Use a double-buffered visual if available, as it will give much smoother
    // animation.
    double_buffered = true;
    int wx_gl_attribs[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, 0 };
    if (!InitGLVisual(wx_gl_attribs)) {
	int wx_gl_attribs_no_db[] = { WX_GL_RGBA, 0 };
	if (!InitGLVisual(wx_gl_attribs_no_db)) {
	    wxString m;
	    m.Printf(wmsg(/*This version of %s requires OpenGL to work, but it isn't available*/405), APP_NAME);
	    wxMessageBox(m, APP_NAME, wxOK | wxCENTRE | wxICON_EXCLAMATION);
	    exit(1);
	}
	double_buffered = false;
    }

    wxImage::AddHandler(new wxPNGHandler);

    // Obtain the screen size.
    wxPoint pos(wxDefaultPosition);
    int width, height;
    wxConfigBase::Get()->Read(wxT("width"), &width, 0);
    if (width > 0) wxConfigBase::Get()->Read(wxT("height"), &height, 0);
    bool maximized = (width == -1);
    bool full_screen = (width <= -2);
    if (width <= 0 || height <= 0) {
#if wxUSE_DISPLAY // wxDisplay was added in wx 2.5
	wxRect geom = wxDisplay().GetGeometry();
	pos.x = geom.x;
	pos.y = geom.y;
	width = geom.width;
	height = geom.height;
#else
	wxClientDisplayRect(&pos.x, &pos.y, &width, &height);
	// Crude fix to help behaviour on multi-monitor displays.
	// Fudge factors are a bit specific to my setup...
	if (width > height * 3 / 2) {
	    pos.x += width;
	    width = height * 3 / 2;
	    pos.x -= width;
	}
#endif

	// Calculate a reasonable size for our window.
	pos.x += width / 8;
	pos.y += height / 8;
	width = width * 3 / 4;
	height = height * 3 / 4;
    }

    // Create the main window.
    m_Frame = new MainFrm(APP_NAME, pos, wxSize(width, height));

    // Select full_screen or maximised if that's the saved state.
    if (full_screen) {
	m_Frame->ShowFullScreen(true);
    } else if (maximized) {
	m_Frame->Maximize();
    }

    if (real_argv[optind]) {
	m_Frame->OpenFile(fnm, survey);
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

void Aven::ReportError(const wxString& msg)
{
    AvenAllowOnTop ontop(m_Frame);
    wxMessageDialog dlg(m_Frame, msg, APP_NAME, wxOK | wxICON_ERROR);
    dlg.ShowModal();
}

wxString
wmsg(int msg_no) 
{
#if !wxCHECK_VERSION(2,8,0)
    return wxString(msg(msg_no), wxConvUTF8);
#else
    return wxString::FromUTF8(msg(msg_no));
#endif
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

   m += wxString::FormatV(wmsg(en), ap);
   wxGetApp().ReportError(m);
}
