//
//  moviemaker.h
//
//  Class for writing movies from Aven.
//
//  Copyright (C) 2004 Olly Betts
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

struct AVCodec;
struct AVCodecContext;
struct AVFrame;
struct AVPicture;

class MovieMaker {
    AVCodec *codec;
    AVCodecContext *c;
    int out_size;
    FILE *fh;
    AVFrame *frame;
    unsigned char *outbuf;
    AVPicture *in;
    AVPicture *out;
    unsigned char *pixels;
 
public:
    MovieMaker(int width, int height);
    bool Open(const char *fnm);
    unsigned char * GetBuffer() const;
    int GetWidth() const;
    int GetHeight() const;
    void AddFrame();
    ~MovieMaker();
};
