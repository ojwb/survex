//
//  moviemaker.cc
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

#include <stdlib.h>
#include <string.h>

#include "moviemaker.h"

#ifdef HAVE_AVCODEC_H
#include "avcodec.h"
#endif

const int OUTBUF_SIZE = 100000;

MovieMaker::MovieMaker(int width, int height)
{
#ifdef HAVE_AVCODEC_H
    static bool initialised_ffmpeg = false;
    if (!initialised_ffmpeg) {
	avcodec_init();

	// FIXME: register only the codec(s) we want to use...
	avcodec_register_all();

	initialised_ffmpeg = true;
    }

    codec = avcodec_find_encoder(CODEC_ID_MPEG1VIDEO);
    //codec = avcodec_find_encoder(CODEC_ID_MPEG2VIDEO);
    //codec = avcodec_find_encoder(CODEC_ID_MPEG2VIDEO_XVMC);
    //codec = avcodec_find_encoder(CODEC_ID_MPEG4);
    if (!codec) {
	fprintf(stderr, "codec not found\n");
	exit(1);
    }

    c = avcodec_alloc_context();
    if (!c) {
	fprintf(stderr, "failed to allocate context\n");
	exit(1);
    }

    c->width = width;
    c->height = height;

    // Set sample parameters.
    c->bit_rate = 400000;

    c->frame_rate = 25; // FPS
    c->frame_rate_base = 1;
    c->gop_size = 10; // Emit one intra frame every ten frames.
    c->max_b_frames = 1;
    c->pix_fmt = PIX_FMT_YUV420P;

    if (avcodec_open(c, codec) < 0) {
	fprintf(stderr, "could not open codec\n");
	exit(1);
    }
    
    outbuf = (unsigned char *)malloc(OUTBUF_SIZE);
    if (!outbuf) {
	fprintf(stderr, "could not alloc outbuf\n");
	exit(1);
    }
 
    frame = avcodec_alloc_frame();
    if (!frame) {
	fprintf(stderr, "failed to allocate frame\n");
	exit(1);
    }

    frame->linesize[0] = width;
    frame->linesize[1] = width / 2;
    frame->linesize[2] = width / 2;

    in = new AVPicture;
    out = new AVPicture;
    in->linesize[0] = width * 3;

    int size = width * height;
    frame->data[0] = out->data[0] = (unsigned char *)malloc(size * 3 / 2);
    if (!out->data[0]) {
	fprintf(stderr, "could not alloc out->data[0]\n");
	exit(1);
    }
    frame->data[1] = out->data[1] = out->data[0] + size;
    frame->data[2] = out->data[2] = out->data[1] + size / 4;
    out->linesize[0] = width;
    out->linesize[1] = width / 2;
    out->linesize[2] = width / 2;

    out_size = 0;

    pixels = (unsigned char *)malloc(width * height * 6);
    if (!pixels) {
	fprintf(stderr, "could not alloc pixels\n");
	exit(1);
    }
    in->data[0] = pixels + height * width * 3;
#endif
}

bool MovieMaker::Open(const char *fnm)
{
#ifdef HAVE_AVCODEC_H
    fh = fopen(fnm, "wb");
    return (fh != NULL);
#else
    return false;
#endif
}

unsigned char * MovieMaker::GetBuffer() const {
    return pixels;
}

int MovieMaker::GetWidth() const {
    return c->width;
}

int MovieMaker::GetHeight() const {
    return c->height;
}

void MovieMaker::AddFrame()
{
#ifdef HAVE_AVCODEC_H
    const int len = 3 * c->width;
    const int h = c->height;
    // Flip image vertically
    for (int y = 0; y < h; ++y) {
	memcpy(pixels + (2 * h - y - 1) * len, pixels + y * len, len);
    }

    img_convert(out, PIX_FMT_YUV420P, in, PIX_FMT_RGB24, c->width, c->height);

    /* encode the image */
    out_size = avcodec_encode_video(c, outbuf, OUTBUF_SIZE, frame);
    if (fwrite(outbuf, out_size, 1, fh) != 1) {
	// FIXME : write error
    }
#endif
}

MovieMaker::~MovieMaker()
{
#ifdef HAVE_AVCODEC_H
    /* get the delayed frames */
    while (out_size) {
	out_size = avcodec_encode_video(c, outbuf, OUTBUF_SIZE, NULL);
	if (fwrite(outbuf, out_size, 1, fh) != 1) {
	    // FIXME : write error
	}
    }

    // Write MPEG sequence end code.
    if (fwrite("\x00\x00\x01\xb7", 4, 1, fh) != 1) {
	// FIXME : write error
    }
    fclose(fh);
    free(outbuf);

    avcodec_close(c);
    free(c);
    free(frame);
    free(out->data[0]);
    free(pixels);
    delete in;
    delete out;
#endif
}
