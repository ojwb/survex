#include <wx/wxprec.h>

#ifdef __BORLANDC__
# pragma hdrstop
#endif

#ifndef WX_PRECOMP
# include <wx/wx.h>
#endif

#ifndef __WXMAC__
# if !wxCHECK_VERSION(2,4,0)
#  error We support building with wxWidgets 2.4.0 or newer
# endif
#else
# if !wxCHECK_VERSION(2,5,1)
#  error We support building with wxWidgets 2.5.1 or newer on MacOS X
# endif
#endif

// These were renamed in wx 2.7.
#if !wxCHECK_VERSION(2,7,0)
# define wxFD_OPEN wxOPEN
# define wxFD_OVERWRITE_PROMPT wxOVERWRITE_PROMPT
# define wxFD_SAVE wxSAVE
# if wxCHECK_VERSION(2,6,0)
#  define wxFD_FILE_MUST_EXIST wxFILE_MUST_EXIST
# else
// This was a new feature in wx 2.6.
#  define wxFD_FILE_MUST_EXIST 0
# endif

# define wxBK_BOTTOM wxNB_BOTTOM
# define wxBK_LEFT wxNB_LEFT
#endif

#include <wx/splitter.h>
#include <wx/treectrl.h>
#include <wx/dnd.h>

#if !wxUSE_GLCANVAS
# error wxWindows must be built with wxUSE_GLCANVAS set to 1
#endif
#include <wx/glcanvas.h>
