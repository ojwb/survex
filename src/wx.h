#include "wx/wxprec.h"

#ifdef __BORLANDC__
# pragma hdrstop
#endif

#ifndef WX_PRECOMP
# include "wx/wx.h"
#endif

#include "wx/splitter.h"
#include "wx/treectrl.h"
#include "wx/dnd.h"

#ifdef AVENGL
#if !wxUSE_GLCANVAS
#error wxWindows must be built with wxUSE_GLCANVAS set to 1
#endif
#include <wx/glcanvas.h>
#endif

