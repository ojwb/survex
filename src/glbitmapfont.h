//
//  glbitmapfont.h
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

#ifndef GLBITMAPFONT_H_INCLUDED
#define GLBITMAPFONT_H_INCLUDED

#include "wx.h"

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

class BitmapFont {
    enum {
	// The largest character to handle.
	//
	// FIXME: We can't generate a GL list for every Unicode character, but
	// we could special case the first 256 (and perhaps even store strings
	// with only those as ISO8859-1), and use glBitmap directly to draw
	// other characters.
	BITMAPFONT_MAX_CHAR = 256
    };

    int gllist_base;

    int char_width[BITMAPFONT_MAX_CHAR];

  public:

    BitmapFont() : gllist_base(0) { }

    ~BitmapFont() {
	if (gllist_base)
	    glDeleteLists(gllist_base, BITMAPFONT_MAX_CHAR);
    }

    bool load(const wxString & font_file);

    // Hard-code for now.
    int get_font_size() const { return 16; }

    void get_text_extent(const wxChar *s, size_t len, int *width, int *height) const {
	if (width) {
	    int total_width = 0;
	    while (len--) {
		wxChar ch = *s++;
		if (ch < BITMAPFONT_MAX_CHAR)
		    total_width += char_width[ch];
	    }
	    *width = total_width;
	}
	if (height) {
	    *height = get_font_size() + 1;
	}
    }

    void write_string(const wxChar *s, size_t len) const;
};

#endif
