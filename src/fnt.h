/*
     Based on:
     PLIB - A Suite of Portable Game Libraries
     Copyright (C) 1998,2002  Steve Baker
 
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
 
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
 
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 
     For further information visit http://plib.sourceforge.net

     $Id: fnt.h,v 1.1.2.5 2003-12-04 02:12:47 olly Exp $
*/


#ifndef _FNT_H_
#define _FNT_H_  1

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#else
#  include <GL/gl.h>
#endif

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

    void getTextExtent(const char *s, int *width, int *height) const {
	if (width) {
	    int w = -1;
	    while (*s) w += widths[(unsigned char)*s++] + 1;
	    if (w < 0) w = 0;
	    *width = w;
	}
	if (height) *height = fnt_size + 2;
    }

    void puts(int x, int y, const char *s) const {
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glTranslated(x, y, 0);
	glListBase(list_base);
	glCallLists(strlen(s), GL_UNSIGNED_BYTE, s);
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
