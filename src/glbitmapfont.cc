//
//  glbitmapfont.cc
//
//  Draw text using glBitmap.
//
//  Copyright (C) 2011,2012,2013,2014 Olly Betts
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

#include "../lib/preload_font.h"

#define CHECK_GL_ERROR(M, F) do { \
    GLenum error_code_ = glGetError(); \
    if (error_code_ != GL_NO_ERROR) { \
	wxLogError(wxT(__FILE__":"STRING(__LINE__)": OpenGL error: %s " \
		   "(call "F" in method "M")"), \
		   wxString((const char *)gluErrorString(error_code_), \
			    wxConvUTF8).c_str()); \
    } \
} while (0)

bool
BitmapFont::load(const wxString & font_file_)
{
    font_file = font_file_;

    if (!gllist_base) {
	gllist_base = glGenLists(BITMAPFONT_MAX_CHAR);
    }

    const unsigned char * p = fontdata_preloaded;
    const unsigned char * end = p + sizeof(fontdata_preloaded);
    for (int ch = 0; ch < BITMAPFONT_MAX_CHAR; ++ch) {
	glNewList(gllist_base + ch, GL_COMPILE);
	CHECK_GL_ERROR("BitmapFont::load", "glNewList");
	if (p == end) {
	    return false;
	}
	unsigned int byte_width = *p++;

	char_width[ch] = (byte_width & 0x0f) + 2;
	byte_width >>= 6;

	int start = 0;
	int n = 0;
	if (byte_width) {
	    if (p == end) {
		return false;
	    }
	    unsigned int start_and_n = *p++;
	    start = start_and_n >> 4;
	    n = (start_and_n & 15) + 1;

	    if (end - p < n * byte_width) {
		return false;
	    }
	}

	// Even if there's nothing to display, we want to advance the
	// raster position.
	glBitmap(8 * byte_width, n, 0, -start, char_width[ch], 0, p);
	CHECK_GL_ERROR("BitmapFont::load", "glBitmap");
	glEndList();
	CHECK_GL_ERROR("BitmapFont::load", "glEndList");

	p += n * byte_width;
    }

    return true;
}

inline void call_lists(GLsizei n, const GLvoid * lists)
{
#if SIZEOF_WXCHAR == 1
    glCallLists(n, GL_UNSIGNED_BYTE, lists);
#elif SIZEOF_WXCHAR == 2
    glCallLists(n, GL_UNSIGNED_SHORT, lists);
#elif SIZEOF_WXCHAR == 4
    glCallLists(n, GL_UNSIGNED_INT, lists);
#else
# error "sizeof(wxChar) not 1, 2 or 4"
#endif
}

void
BitmapFont::init_extra_chars() const
{
#ifdef __WXMSW__
    FILE * fh = _wfopen(font_file.fn_str(), L"rb");
#else
    FILE * fh = fopen(font_file.mb_str(), "rb");
#endif

    long data_len = 0;
    if (fh && fseek(fh, 0, SEEK_END) >= 0) {
	data_len = ftell(fh);
	if (data_len == -1)
	    data_len = 0;
    }
    unsigned char * data = new unsigned char [data_len];
    if (data_len && (fseek(fh, 0, SEEK_SET) < 0 ||
	       fread(data, data_len, 1, fh) != 1)) {
	data_len = 0;
	// FIXME: do something better.  wxGetApp().ReportError(m);
	// We have this message available: Error in format of font file “%s”
	// fprintf(stderr, "Couldn't load extended font.\n");
    }
    if (fh)
	fclose(fh);

    extra_chars = new unsigned char * [0x10000 - BITMAPFONT_MAX_CHAR];
    for (int i = 0; i < 0x10000 - BITMAPFONT_MAX_CHAR; ++i) {
	if (data_len <= 0) {
	    extra_chars[i] = NULL;
	    continue;
	}
	extra_chars[i] = data;
	unsigned int byte_width = *data++;
	byte_width >>= 6;

	if (byte_width) {
	    unsigned int start_and_n = *data;
	    int n = (start_and_n & 15) + 1;
	    data += n * byte_width + 1;
	    data_len -= n * byte_width + 1;
	}
    }
}

int
BitmapFont::glyph_width(wxChar ch) const
{
#if SIZEOF_WXCHAR > 2
    if (ch >= 0x10000) return 0;
#endif
    if (!extra_chars)
	init_extra_chars();

    int width = 8;

    const unsigned char * p = extra_chars[ch - BITMAPFONT_MAX_CHAR];
    if (p) {
	unsigned int byte_width = *p;
	width = (byte_width & 0x0f) + 2;
    }

    return width;
}

void
BitmapFont::write_glyph(wxChar ch) const
{
#if SIZEOF_WXCHAR > 2
    if (ch >= 0x10000) return;
#endif
    if (!extra_chars)
	init_extra_chars();

    unsigned int byte_width = 0;
    int start = 0;
    int n = 0;
    int width = 8;

    const unsigned char * p = extra_chars[ch - BITMAPFONT_MAX_CHAR];
    if (p) {
	byte_width = *p++;
	width = (byte_width & 0x0f) + 2;
	byte_width >>= 6;

	if (byte_width) {
	    unsigned int start_and_n = *p++;
	    start = start_and_n >> 4;
	    n = (start_and_n & 15) + 1;
	}
    }

    // Even if there's nothing to display, we want to advance the
    // raster position.
    glBitmap(8 * byte_width, n, 0, -start, width, 0, p);
    CHECK_GL_ERROR("BitmapFont::write_glyph", "glBitmap");
}

void
BitmapFont::write_string(const wxChar *s, size_t len) const
{
    if (!gllist_base) return;
    glListBase(gllist_base);
#if SIZEOF_WXCHAR == 1
    call_lists(len, s);
#elif SIZEOF_WXCHAR == 2 || SIZEOF_WXCHAR == 4
    while (len) {
	size_t n;
	for (n = 0; n < len; ++n)
	    if (int(s[n]) >= BITMAPFONT_MAX_CHAR)
		break;
	call_lists(n, s);
	s += n;
	len -= n;
	while (len && int(*s) >= BITMAPFONT_MAX_CHAR) {
	    write_glyph(*s);
	    ++s;
	    --len;
	}
    }
#else
# error "sizeof(wxChar) not 1, 2 or 4"
#endif
}
