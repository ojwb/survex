//
//  fnt.cc
//
//  Draw text using texture mapped fonts.
//
//  Copyright (C) 2003,2004,2006 Olly Betts
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

#include "fnt.h"

#include <stdio.h>

static bool isSwapped = false;

inline unsigned char fnt_readByte(FILE *fd) {
    return (unsigned char)getc(fd);
}

inline unsigned short fnt_readShort(FILE *fd) {
    unsigned short x;
    if (isSwapped) {
	x = getc(fd) | (getc(fd) << 8);
    } else {
	x = (getc(fd) << 8) | getc(fd);
    }
    return x;
}

inline unsigned int fnt_readInt(FILE *fd) {
    unsigned int x;
    if (isSwapped) {
	x = getc(fd) | (getc(fd) << 8) | (getc(fd) << 16) | (getc(fd) << 24);
    } else {
	x = (getc(fd) << 24) | (getc(fd) << 16) | (getc(fd) << 8) | getc(fd);
    }
    return x;
}

#define FNT_BYTE_FORMAT		0
#define FNT_BITMAP_FORMAT	1

bool
fntTexFont::load(const char *fname)
{
    FILE *fd;

    if ((fd = fopen(fname, "rb")) == NULL) {
	fprintf(stderr, "Failed to open '%s' for reading.\n", fname);
	return false;
    }

    unsigned char magic[4];

    if (fread(&magic, 4, 1, fd) != 1) {
	fprintf(stderr, "'%s' an empty file!\n", fname);
	return false;
    }

    if (memcmp(magic, "\xfftxf", 4) != 0) {
	fprintf(stderr, "'%s' is not a 'txf' font file.\n", fname);
	return false;
    }

    isSwapped = false;
    int endianness = fnt_readInt(fd);

    isSwapped = (endianness != 0x12345678);

    int format      = fnt_readInt(fd);
    int tex_width   = fnt_readInt(fd);
    int tex_height  = fnt_readInt(fd);
    /* int max_height = */ fnt_readInt(fd);
    /* int unknown = */ fnt_readInt(fd);
    int num_glyphs  = fnt_readInt(fd);
    list_base = glGenLists(256 - 32) - 32;

    int i, j;

    // Skip the glyph info first so we can set up the texture, then create
    // display lists from it using the glyph info
    int fpos = ftell(fd);
    fseek(fd, num_glyphs * 12, SEEK_CUR);
    // Load the image part of the file
    int ntexels = tex_width * tex_height;

    unsigned char *teximage = new unsigned char[ntexels];

    switch (format) {
	case FNT_BYTE_FORMAT: {
	    if ((int)fread(teximage, 1, ntexels, fd) != ntexels) {
		delete [] teximage;
		fprintf(stderr, "Premature EOF in '%s'.\n", fname);
		return false;
	    }
	    break;
	}

	case FNT_BITMAP_FORMAT: {
	    int stride = (tex_width + 7) >> 3;

	    unsigned char *texbitmap = new unsigned char[stride * tex_height];

	    if ((int)fread(texbitmap, 1, stride * tex_height, fd)
		    != stride * tex_height) {
		delete [] texbitmap;
		delete [] teximage;
		fprintf(stderr, "Premature EOF in '%s'.\n", fname);
		return false;
	    }

	    memset((void*)teximage, 0, ntexels);

	    for (i = 0; i < tex_height; ++i) {
		for (j = 0; j < tex_width; ++j) {
		    if (texbitmap[i * stride + (j >> 3)] & (1 << (j & 7))) {
			teximage[(i * tex_width + j)] = 1;
		    }
		}
	    }

	    delete [] texbitmap;
	    break;
	}

	default:
	    delete [] teximage;
	    fprintf(stderr, "Unrecognised format type in '%s'.\n", fname);
	    return false;
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(1, & texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    static const GLuint map[2] = { 0, 0xffffffff };
    glPixelMapuiv(GL_PIXEL_MAP_I_TO_A, 2, map);
    glPixelMapuiv(GL_PIXEL_MAP_I_TO_R, 2, map);
    glPixelMapuiv(GL_PIXEL_MAP_I_TO_G, 2, map);
    glPixelMapuiv(GL_PIXEL_MAP_I_TO_B, 2, map);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, tex_width, tex_height, 0 /* Border */,
		 GL_COLOR_INDEX, GL_UNSIGNED_BYTE, (GLvoid *)teximage);
    delete [] teximage;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    fseek(fd, fpos, SEEK_SET);

    for (i = 0; i < FNT_MAXCHAR; ++i) widths[i] = -1;

    // Load the glyph array

    float W = 1.0f / (float)tex_width;
    float H = 1.0f / (float)tex_height;
    unsigned char max_w = 0;
    int u = 0, d = 0;
    for (i = 0; i < num_glyphs; ++i) {
	unsigned short ch = fnt_readShort(fd);
	unsigned char w = fnt_readByte(fd);
	if (w > max_w) max_w = w;
	unsigned char h = fnt_readByte(fd);
	int vtx_left = (signed char)fnt_readByte(fd);
	// We can't handle lbearing and rbearing correctly so ignore them.
	vtx_left = 0;
	int vtx_bot = (signed char)fnt_readByte(fd);
	/* signed char step =*/ fnt_readByte(fd);
	/* signed char unknown =*/ fnt_readByte(fd);
	short x = fnt_readShort(fd);
	short y = fnt_readShort(fd);

	if (ch < 32 || ch >= FNT_MAXCHAR) continue;

	float tex_left = x * W;
	float tex_right = (x + w) * W;
	float tex_bot = y * H;
	float tex_top = (y + h) * H;
	int vtx_right = vtx_left + w;
	int vtx_top = vtx_bot + h;
	glNewList(list_base + ch, GL_COMPILE);
	if (w != 0 && h != 0) {
	    glBegin(GL_QUADS);
	    glTexCoord2f(tex_left, tex_bot);
	    glVertex2i(vtx_left, vtx_bot);
	    glTexCoord2f(tex_right, tex_bot);
	    glVertex2i(vtx_right, vtx_bot);
	    glTexCoord2f(tex_right, tex_top);
	    glVertex2i(vtx_right, vtx_top);
	    glTexCoord2f(tex_left, tex_top);
	    glVertex2i(vtx_left, vtx_top);
	    glEnd();
	    widths[ch] = w + 1;
	} else {
	    widths[ch] = fnt_size / 2;
	}
	glTranslated(widths[ch], 0, 0);
	glEndList();
	if (vtx_bot < d) d = vtx_bot;
	if (vtx_top > u) u = vtx_top;
    }
    fnt_size = u - d;

    if (widths[(int)' '] == -1) {
	glNewList(list_base + ' ', GL_COMPILE);
	widths[(int)' '] = widths[(int)'n'];
	glTranslated(widths[(int)' '], 0, 0);
	glEndList();
    }

    fclose(fd);
    return true;
}
