//
//  glbitmapfont.cc
//
//  Draw text using glBitmap.
//
//  Copyright (C) 2011-2022 Olly Betts
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

#include <config.h>

#include "glbitmapfont.h"

#ifdef HAVE_GL_GL_H
# include <GL/gl.h>
#elif defined HAVE_OPENGL_GL_H
# include <OpenGL/gl.h>
#endif

#ifdef HAVE_GL_GLU_H
# include <GL/glu.h>
#elif defined HAVE_OPENGL_GLU_H
# include <OpenGL/glu.h>
#endif

#include "gllogerror.h"
#include "useful.h"
#include "wx.h"

#include <cerrno>
#include <cstdint>
#ifdef HAVE_MMAP
#include <sys/mman.h>
#else
#include <unistd.h>
#endif

using namespace std;

#include "../lib/preload_font.h"

BitmapFont::~BitmapFont() {
    if (!gllist_base) return;
    glDeleteLists(gllist_base, BITMAPFONT_MAX_CHAR);
    CHECK_GL_ERROR("BitmapFont::~BitmapFont", "glDeleteLists");
}

static uint16_t* tab_2x = NULL;
static uint16_t* fontdata_2x = NULL;
static uint16_t* fontdata_2x_extra = NULL;

bool
BitmapFont::load(const wxString & font_file_, bool double_size)
{
    font_file = font_file_;

    font_size = double_size ? 32 : 16;

    if (!gllist_base) {
	gllist_base = glGenLists(BITMAPFONT_MAX_CHAR);
	CHECK_GL_ERROR("BitmapFont::load", "glGenLists");
    }

    if (double_size && !tab_2x) {
	tab_2x = new uint16_t[256];
	for (unsigned i = 0; i < 256; ++i) {
	    unsigned v = (i & 1) << 8 |
			 (i & 2) << 9 |
			 (i & 4) << 10 |
			 (i & 8) << 11 |
			 (i & 16) >> 4 |
			 (i & 32) >> 3 |
			 (i & 64) >> 2 |
			 (i & 128) >> 1;
	    tab_2x[i] = 3 * v;
	}
	fontdata_2x = new uint16_t[sizeof(fontdata_preloaded) * 2];
    }
    const unsigned char * p = fontdata_preloaded;
    const unsigned char * end = p + sizeof(fontdata_preloaded);
    uint16_t* out = fontdata_2x;
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
	unsigned int n = 0;
	if (byte_width) {
	    if (p == end) {
		return false;
	    }
	    unsigned int start_and_n = *p++;
	    start = start_and_n >> 4;
	    n = (start_and_n & 15) + 1;

	    if (unsigned(end - p) < n * byte_width) {
		return false;
	    }
	}

	// Note that even if there's nothing to display, we still call
	// glBitmap() to advance the raster position.
	if (double_size) {
	    const GLubyte* q = reinterpret_cast<const GLubyte*>(out);
	    for (unsigned i = 0; i != n; ++i) {
		for (unsigned j = 0; j != byte_width; ++j) {
		    *out++ = tab_2x[*p++];
		}
		memcpy(out, out - byte_width, byte_width * sizeof(uint16_t));
		out += byte_width;
	    }

	    char_width[ch] *= 2;
	    glBitmap(2 * 8 * byte_width, 2 * n, 0, -2 * start, char_width[ch], 0, q);
	} else {
	    glBitmap(8 * byte_width, n, 0, -start, char_width[ch], 0, p);
	    p += n * byte_width;
	}
	CHECK_GL_ERROR("BitmapFont::load", "glBitmap");
	glEndList();
	CHECK_GL_ERROR("BitmapFont::load", "glEndList");
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
    bool double_size = (tab_2x != NULL);
    int fd = wxOpen(font_file,
#ifdef __WXMSW__
	    _O_RDONLY|_O_SEQUENTIAL|_O_BINARY,
#else
	    O_RDONLY,
#endif
	    0);

    int data_len = 0;
    unsigned char * data = NULL;
    if (fd >= 0) {
	struct stat sb;
	if (fstat(fd, &sb) >= 0) {
	    data_len = sb.st_size;
	}
    }

#if HAVE_MMAP
    if (data_len) {
	void * p = mmap(NULL, data_len, PROT_READ, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED) {
	    data_len = 0;
	} else {
	    extra_data = data = static_cast<unsigned char*>(p);
	}
    }
#else
    data = new unsigned char[data_len];
    extra_data = data;
    if (data_len) {
	size_t c = data_len;
	unsigned char * p = data;
	while (c) {
	    ssize_t n = read(fd, p, c);
	    if (n <= 0) {
		if (errno == EINTR) continue;
		data_len = 0;
		// FIXME: do something better.  wxGetApp().ReportError(m);
		// We have this message available: Error in format of font file “%s”
		// fprintf(stderr, "Couldn't load extended font.\n");
		break;
	    }
	    p += n;
	    c -= n;
	}
    }
#endif

    if (fd >= 0)
	close(fd);

    extra_chars = new int[0x10000 - BITMAPFONT_MAX_CHAR];

    int ch = 0;
    if (double_size) {
	// The bitmap data will increase 4*, while the metadata will increase
	// 2*, so we can use 4* the total size as an overestimate.
	fontdata_2x_extra = new uint16_t[data_len * 2];
	uint16_t* out = fontdata_2x_extra;
	int data_ch = 0;
	while (data_ch < data_len) {
	    extra_chars[ch++] = out - fontdata_2x_extra;
	    unsigned int b = data[data_ch++];
	    *out++ = b;
	    unsigned int byte_width = b >> 6;
	    if (byte_width) {
		unsigned int start_and_n = data[data_ch++];
		*out++ = start_and_n;
		unsigned int n = (start_and_n & 15) + 1;
		for (unsigned i = 0; i != n; ++i) {
		    for (unsigned j = 0; j != byte_width; ++j) {
			*out++ = tab_2x[data[data_ch++]];
			if (out - fontdata_2x_extra >= data_len * 2) abort();
		    }
		    memcpy(out, out - byte_width, byte_width * sizeof(uint16_t));
		    out += byte_width;
		    if (out - fontdata_2x_extra >= data_len * 2) abort();
		}
	    }
	}
    } else {
	int data_ch = 0;
	while (data_ch < data_len) {
	    extra_chars[ch++] = data_ch;
	    unsigned int byte_width = data[data_ch++];
	    byte_width >>= 6;

	    if (byte_width) {
		unsigned int start_and_n = data[data_ch];
		int n = (start_and_n & 15) + 1;
		data_ch += n * byte_width + 1;
	    }
	}
    }

    while (ch < 0x10000 - BITMAPFONT_MAX_CHAR) {
	extra_chars[ch++] = -1;
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

    int char_idx = extra_chars[ch - BITMAPFONT_MAX_CHAR];
    if (char_idx < 0) {
	return tab_2x ? 16 : 8;
    }
    if (tab_2x) {
	unsigned int byte_width = fontdata_2x_extra[char_idx];
	return ((byte_width & 0x0f) + 2) << 1;
    } else {
	unsigned int byte_width = extra_data[char_idx];
	return (byte_width & 0x0f) + 2;
    }
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

    int char_idx = extra_chars[ch - BITMAPFONT_MAX_CHAR];
    const unsigned char* data = NULL;
    if (char_idx >= 0) {
	if (tab_2x) {
	    const uint16_t* p = fontdata_2x_extra + char_idx;
	    byte_width = *p++;
	    width = (byte_width & 0x0f) + 2;
	    byte_width >>= 6;

	    if (byte_width) {
		byte_width <<= 1;
		unsigned int start_and_n = *p++;
		start = (start_and_n >> 4) << 1;
		n = ((start_and_n & 15) + 1) << 1;
	    }
	    width <<= 1;

	    data = reinterpret_cast<const unsigned char*>(p);
	} else {
	    const unsigned char * p = extra_data + char_idx;
	    byte_width = *p++;
	    width = (byte_width & 0x0f) + 2;
	    byte_width >>= 6;

	    if (byte_width) {
		unsigned int start_and_n = *p++;
		start = start_and_n >> 4;
		n = (start_and_n & 15) + 1;
	    }
	    data = p;
	}
    }

    // Even if there's nothing to display, we want to advance the
    // raster position.
    glBitmap(8 * byte_width, n, 0, -start, width, 0, data);
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
