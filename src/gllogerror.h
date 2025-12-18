//
//  gllogerror.h
//
//  Check for and report OpenGL errors
//
//  Copyright (C) 2002 Mark R. Shinwell.
//  Copyright (C) 2003,2004,2005,2006,2007,2011,2012,2014,2017,2018 Olly Betts
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

#include <wx/wx.h>

void log_gl_error(const wxChar * str, GLenum error_code);

// Important: CHECK_GL_ERROR must not be called within a glBegin()/glEnd() pair
//            (thus it must not be called from BeginLines(), etc., or within a
//             BeginLines()/EndLines() block etc.)
#define CHECK_GL_ERROR(M, F) do { \
    extern bool opengl_initialised; \
    if (!opengl_initialised) { \
	wxLogError(wxT(__FILE__ ":" STRING(__LINE__) ": OpenGL not initialised " \
		       "before (call " F " in method " M ")")); \
    } \
    GLenum error_code_ = glGetError(); \
    if (error_code_ != GL_NO_ERROR) { \
	log_gl_error(wxT(__FILE__ ":" STRING(__LINE__) ": OpenGL error: %s " \
			 "(call " F " in method " M ")"), error_code_); \
    } \
} while (0)

