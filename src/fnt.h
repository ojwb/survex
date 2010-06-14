//
//  fnt.h
//
//  Draw text using texture mapped fonts.
//
//  Copyright (C) 2003,2004,2006,2010 Olly Betts
//
//     Based on code from PLIB - http://plib.sourceforge.net
//     Copyright (C) 1998,2002  Steve Baker
//     Relicensed under the GNU GPL as permitted by the GNU LGPL
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
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#ifndef _FNT_H_
#define _FNT_H_  1

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#else
#  include <GL/gl.h>
#endif

#include "wx.h" // For wxChar

#include <string.h>

#define FNT_MAXCHAR 256

class fntTexFont {
  private:
    GLuint texture;

    int fnt_size;

    int list_base;

    /* Nominal baseline widths */
    int widths[FNT_MAXCHAR];

  public:

    fntTexFont() : texture(0), fnt_size(0), list_base(0)
    {
    }

    ~fntTexFont()
    {
	if (list_base != 0) glDeleteLists(list_base + 32, 256 - 32);
	if (texture != 0) glDeleteTextures(1, &texture);
    }

    bool load(const char *fname);

    bool hasGlyph(char c) const { return widths[(unsigned char)c] >= 0; }

    int getFontSize() const { return fnt_size; }

    void getTextExtent(const wxChar *s, int *width, int *height) const {
	if (width) {
	    int w = -1;
	    while (*s) w += widths[(unsigned char)*s++];
	    if (w < 0) w = 0;
	    *width = w;
	}
	if (height) *height = fnt_size + 1;
    }

    void puts(int x, int y, const wxChar *s, size_t len) const {
	glBindTexture(GL_TEXTURE_2D, texture);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glTranslated(x, y, 0);
	glListBase(list_base);
	if (sizeof(wxChar) == 1) {
	    glCallLists(len, GL_UNSIGNED_BYTE, s);
	} else if (sizeof(wxChar) == 2) {
	    glCallLists(len, GL_UNSIGNED_SHORT, s);
	} else if (sizeof(wxChar) == 4) {
	    glCallLists(len, GL_UNSIGNED_INT, s);
	}
	glPopMatrix();
    }
#if 0
    void putch(int x, int y, char c) {
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glTranslated(x, y, 0);
	glCallList(list_base + (unsigned char)c);
	glPopMatrix();
    }
#endif

};

#endif
