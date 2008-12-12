#include <wx/wxprec.h>

#ifdef __BORLANDC__
# pragma hdrstop
#endif

#ifndef WX_PRECOMP
# include <wx/wx.h>
#endif

#if !wxCHECK_VERSION(2,6,0)
# error We support building with wxWidgets 2.6.0 or newer
#endif

// These were renamed in wx 2.7.
#if !wxCHECK_VERSION(2,7,0)
# define wxFD_OPEN wxOPEN
# define wxFD_OVERWRITE_PROMPT wxOVERWRITE_PROMPT
# define wxFD_SAVE wxSAVE
# define wxFD_FILE_MUST_EXIST wxFILE_MUST_EXIST

# define wxBK_BOTTOM wxNB_BOTTOM
# define wxBK_LEFT wxNB_LEFT
#endif

#include <wx/splitter.h>
#include <wx/treectrl.h>
#include <wx/dnd.h>

#if !wxUSE_GLCANVAS
# error wxWidgets must be built with wxUSE_GLCANVAS set to 1
#endif
#include <wx/glcanvas.h>
