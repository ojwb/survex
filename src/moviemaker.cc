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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "moviemaker.h"

#ifdef HAVE_AVFORMAT_H
#include "avformat.h"
#endif

// ffmpeg CVS has added av_alloc_format_context() - we're ready for it!
#define av_alloc_format_context() \
    ((AVFormatContext*)av_mallocz(sizeof(AVFormatContext)))

const int OUTBUF_SIZE = 200000;

MovieMaker::MovieMaker()
    : oc(0), st(0), frame(0), outbuf(0), in(0), out(0), pixels(0)
{
#ifdef HAVE_AVFORMAT_H
    static bool initialised_ffmpeg = false;
    if (initialised_ffmpeg) return;

    // FIXME: register only the codec(s) we want to use...
    av_register_all();

    initialised_ffmpeg = true;
#endif
}

bool MovieMaker::Open(const char *fnm, int width, int height)
{
#ifdef HAVE_AVFORMAT_H
    AVOutputFormat * fmt = guess_format(NULL, fnm, NULL);
    if (!fmt) {
	// We couldn't deduce the output format from file extension so default
	// to MPEG.
	fmt = guess_format("mpeg", NULL, NULL);
	if (!fmt) {
	    // FIXME : error finding a format...
	    return false;
	}
    }
    if (fmt->video_codec == CODEC_ID_NONE) {
	// FIXME : The user asked for a format which is audio-only!
	return false;
    }

    // Allocate the output media context.
    oc = av_alloc_format_context();
    if (!oc) {
	// FIXME : out of memory
	return false;
    }
    oc->oformat = fmt;
    if (strlen(fnm) >= sizeof(oc->filename)) {
	// FIXME : filename too long
	return false;
    }
    strcpy(oc->filename, fnm);

    // Add the video stream using the default format codec.
    st = av_new_stream(oc, 0);
    if (!st) {
	// FIXME : possible errors are "too many streams" (can't be - we only
	// ask for one) and "out of memory"
	return false;
    }

    // Initialise the code.
    AVCodecContext *c = &st->codec;
    c->codec_id = fmt->video_codec;
    c->codec_type = CODEC_TYPE_VIDEO;

    // Set sample parameters.
    c->bit_rate = 400000;
    c->width = width;
    c->height = height;
    c->frame_rate = 25; // FPS
    c->frame_rate_base = 1;
    c->gop_size = 12; // One intra frame every twelve frames.
    c->pix_fmt = PIX_FMT_YUV420P;
    // B frames are backwards predicted - they can improve compression,
    // but may slow encoding and decoding.
    // c->max_b_frames = 2;

    // Set the output parameters (must be done even if no parameters).
    if (av_set_parameters(oc, NULL) < 0) {
	// FIXME : return value is an AVERROR_* value - probably
	// AVERROR_NOMEM or AVERROR_UNKNOWN.
	return false;
    }

    // Show the format we've ended up with (for debug purposes).
    // dump_format(oc, 0, fnm, 1);

    // Open the video codec and allocate the necessary encode buffers.
    AVCodec * codec = avcodec_find_encoder(c->codec_id);
    if (!codec) {
	// FIXME : Erm - internal ffmpeg library problem?
	return false;
    }
    if (avcodec_open(c, codec) < 0) {
	// FIXME : return value is an AVERROR_* value - probably
	// AVERROR_NOMEM or AVERROR_UNKNOWN.
	return false;
    }

    outbuf = (unsigned char *)malloc(OUTBUF_SIZE);
    if (!outbuf) {
	// FIXME : out of memory
	return false;
    }

    frame = avcodec_alloc_frame();
    in = (AVPicture *)malloc(sizeof(AVPicture));
    out = (AVPicture *)malloc(sizeof(AVPicture));
    if (!frame || !in || !out) {
	// FIXME : out of memory
	return false;
    }

    int size = width * height;
    out->data[0] = (unsigned char *)malloc(size * 3 / 2);
    if (!out->data[0]) {
	// FIXME : out of memory
	return false;
    }
    avpicture_fill((AVPicture *)frame, out->data[0], c->pix_fmt, width, height);
    out->data[1] = frame->data[1];
    out->data[2] = frame->data[2];
    out->linesize[0] = frame->linesize[0];
    out->linesize[1] = frame->linesize[1];
    out->linesize[2] = frame->linesize[2];

    pixels = (unsigned char *)malloc(width * height * 6);
    if (!pixels) {
	// FIXME : out of memory
	return false;
    }
    in->data[0] = pixels + height * width * 3;
    in->linesize[0] = width * 3;

    out_size = 0;

    if (url_fopen(&oc->pb, fnm, URL_WRONLY) < 0) {
	// FIXME : return value is -E* (e.g. -EIO).
	return false;
    }

    // Write the stream header, if any.
    if (av_write_header(oc) < 0) {
	// FIXME : return value is an AVERROR_* value.
	return false;
    }
    return true;
#else
    return false;
#endif
}

unsigned char * MovieMaker::GetBuffer() const {
    return pixels;
}

int MovieMaker::GetWidth() const {
    assert(st);
#ifdef HAVE_AVFORMAT_H
    AVCodecContext *c = &st->codec;
    return c->width;
#else
    return 0;
#endif
}

int MovieMaker::GetHeight() const {
    assert(st);
#ifdef HAVE_AVFORMAT_H
    AVCodecContext *c = &st->codec;
    return c->height;
#else
    return 0;
#endif
}

void MovieMaker::AddFrame()
{
#ifdef HAVE_AVFORMAT_H
    AVCodecContext * c = &st->codec;

    const int len = 3 * c->width;
    const int h = c->height;
    // Flip image vertically
    for (int y = 0; y < h; ++y) {
	memcpy(pixels + (2 * h - y - 1) * len, pixels + y * len, len);
    }

    img_convert(out, PIX_FMT_YUV420P, in, PIX_FMT_RGB24, c->width, c->height);

    // Encode this frame.
    out_size = avcodec_encode_video(c, outbuf, OUTBUF_SIZE, frame);
    // outsize == 0 means that this frame has been buffered, so there's nothing
    // to write yet.
    if (out_size != 0) {
	// Write the compressed frame to the media file.
	if (av_write_frame(oc, st->index, outbuf, out_size) != 0) {
	    fprintf(stderr, "Error while writing video frame\n");
	    exit(1);
	}
    }
#endif
}

MovieMaker::~MovieMaker()
{
#ifdef HAVE_AVFORMAT_H
    if (st) {
	// No more frames to compress.  The codec may have a few frames
	// buffered if we're using B frames, so write those too.
	AVCodecContext * c = &st->codec;

	while (out_size) {
	    out_size = avcodec_encode_video(c, outbuf, OUTBUF_SIZE, NULL);
	    if (out_size) {
		if (av_write_frame(oc, st->index, outbuf, out_size) != 0) {
		    fprintf(stderr, "Error while writing video frame\n");
		    exit(1);
		}
	    }
	}

	// Close codec.
	avcodec_close(c);
    }

    free(frame);
    if (out) free(out->data[0]);
    free(outbuf);
    free(pixels);
    free(in);
    free(out);

    if (oc) {
	// Write the trailer, if any.
	av_write_trailer(oc);

	// Free the streams.
	for(int i = 0; i < oc->nb_streams; ++i) {
	    av_freep(&oc->streams[i]);
	}

	// Close the output file.
	url_fclose(&oc->pb);

	// Free the stream.
	free(oc);
    }
#endif
}
