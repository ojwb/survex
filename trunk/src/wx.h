//
//  wx.h
//
//  Include wxWidgets headers.
//
//  Copyright (C) 2000,2001,2002 Mark R. Shinwell
//  Copyright (C) 2001,2003,2005,2006,2007,2008 Olly Betts
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
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
//

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
