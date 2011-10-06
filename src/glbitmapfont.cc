//
//  glbitmapfont.cc
//
//  Draw text using glBitmap.
//
//  Copyright (C) 2011 Olly Betts
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

#include "glbitmapfont.h"

#include "aventypes.h"
#include "useful.h"
#include "wx.h"

#define CHECK_GL_ERROR(M, F) do { \
    GLenum error_code_ = glGetError(); \
    if (error_code_ != GL_NO_ERROR) \
	wxLogError(wxT(__FILE__":"STRING(__LINE__)": OpenGL error: %s " \
		   "(call "F" in method "M")"), \
		   wxString((const char *)gluErrorString(error_code_), \
			    wxConvUTF8).c_str()); \
} while (0)

bool
BitmapFont::load(const wxString & font_file)
{
#ifdef __WXMSW__
    FILE * fh = _wfopen(font_file.fn_str(), L"rb");
#else
    FILE * fh = fopen(font_file.mb_str(), "rb");
#endif

    if (!fh) {
	return false;
    }

    if (!gllist_base) {
	gllist_base = glGenLists(BITMAPFONT_MAX_CHAR);
    }

    unsigned char buf[32];

    for (int ch = 0; ch < BITMAPFONT_MAX_CHAR; ++ch) {
	glNewList(gllist_base + ch, GL_COMPILE);
	CHECK_GL_ERROR("BitmapFont::load", "glNewList");

	unsigned int byte_width = GETC(fh);

	int start = 0;
	int n = 0;
	if (byte_width) {
	    unsigned int start_and_n = GETC(fh);
	    start = start_and_n >> 4;
	    n = (start_and_n & 15) + 1;
	    fread(buf, n * byte_width, 1, fh);
	    char_width[ch] = byte_width * 8;
	} else {
	    char_width[ch] = 8;
	}

	// Even if there's nothing to display, we want to advance the
	// raster position.
	glBitmap(8 * byte_width, n, 0, -start, char_width[ch], 0, buf);
	CHECK_GL_ERROR("BitmapFont::load", "glBitmap");
	glEndList();
	CHECK_GL_ERROR("BitmapFont::load", "glEndList");
    }
    fclose(fh);

    return true;
}

inline void call_lists(GLsizei n, const GLvoid * lists)
{
    // Each test is a compile-time constant, so this should get collapsed by
    // the compiler.
    if (sizeof(wxChar) == 1) {
	glCallLists(n, GL_UNSIGNED_BYTE, lists);
    } else if (sizeof(wxChar) == 2) {
	glCallLists(n, GL_UNSIGNED_SHORT, lists);
    } else if (sizeof(wxChar) == 4) {
	glCallLists(n, GL_UNSIGNED_INT, lists);
    }
}

void
BitmapFont::write_string(const wxChar *s, size_t len) const
{
    glListBase(gllist_base);
    // Each test is a compile-time constant, so this should get collapsed by
    // the compiler.
    if (sizeof(wxChar) == 1) {
	call_lists(len, s);
    } else if (sizeof(wxChar) == 2 || sizeof(wxChar) == 4) {
	while (len) {
	    size_t n;
	    for (n = 0; n < len; ++n)
		if (s[n] >= BITMAPFONT_MAX_CHAR)
		    break;
	    call_lists(n, s);
	    s += n;
	    len -= n;
	    while (len && s[n] >= BITMAPFONT_MAX_CHAR) {
		// FIXME: plot character s[n]
		++s;
		--len;
	    }
	}
    }
}
