//
//  moviemaker.cc
//
//  Class for writing movies from Aven.
//
//  Copyright (C) 2004,2011,2012 Olly Betts
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

/* Based on output-example.c:
 *
 * Libavformat API example: Output a media file in any supported
 * libavformat format. The default codecs are used.
 *
 * Copyright (c) 2003 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define __STDC_CONSTANT_MACROS

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "moviemaker.h"

#ifdef HAVE_LIBAVFORMAT_AVFORMAT_H
extern "C" {
# include <libavformat/avformat.h>
# include <libavutil/avutil.h>
# include <libswscale/swscale.h>
}
# ifndef AV_PKT_FLAG_KEY
#  define AV_PKT_FLAG_KEY PKT_FLAG_KEY
# endif
# ifndef HAVE_AV_GUESS_FORMAT
#  define av_guess_format guess_format
# endif
# ifndef HAVE_AVIO_OPEN
#  define avio_open url_fopen
# endif
# ifndef HAVE_AVIO_CLOSE
#  define avio_close url_fclose
# endif
# ifndef HAVE_AVCODEC_OPEN2
// We always pass NULL for OPTS below.
#  define avcodec_open2(CTX, CODEC, OPTS) avcodec_open(CTX, CODEC)
# endif
# ifndef HAVE_AVFORMAT_NEW_STREAM
// We always pass NULL for CODEC below.
#  define avformat_new_stream(S, CODEC) av_new_stream(S, 0)
# endif
# if !HAVE_DECL_AVMEDIA_TYPE_VIDEO
#  define AVMEDIA_TYPE_VIDEO CODEC_TYPE_VIDEO
# endif
#endif

enum {
    MOVIE_NO_SUITABLE_FORMAT = 1,
    MOVIE_AUDIO_ONLY,
    MOVIE_FILENAME_TOO_LONG,
    MOVIE_NOT_ENABLED
};

const int OUTBUF_SIZE = 200000;

MovieMaker::MovieMaker()
    : oc(0), st(0), frame(0), outbuf(0), pixels(0), sws_ctx(0), averrno(0)
{
#ifdef HAVE_LIBAVFORMAT_AVFORMAT_H
    static bool initialised_ffmpeg = false;
    if (initialised_ffmpeg) return;

    // FIXME: register only the codec(s) we want to use...
    av_register_all();

    initialised_ffmpeg = true;
#endif
}

bool MovieMaker::Open(const char *fnm, int width, int height)
{
#ifdef HAVE_LIBAVFORMAT_AVFORMAT_H
    AVOutputFormat * fmt = av_guess_format(NULL, fnm, NULL);
    if (!fmt) {
	// We couldn't deduce the output format from file extension so default
	// to MPEG.
	fmt = av_guess_format("mpeg", NULL, NULL);
	if (!fmt) {
	    averrno = MOVIE_NO_SUITABLE_FORMAT;
	    return false;
	}
    }
    if (fmt->video_codec == CODEC_ID_NONE) {
	averrno = MOVIE_AUDIO_ONLY;
	return false;
    }

    // Allocate the output media context.
    oc = avformat_alloc_context();
    if (!oc) {
	averrno = AVERROR(ENOMEM);
	return false;
    }
    oc->oformat = fmt;
    if (strlen(fnm) >= sizeof(oc->filename)) {
	averrno = MOVIE_FILENAME_TOO_LONG;
	return false;
    }
    strcpy(oc->filename, fnm);

    // Add the video stream using the default format codec.
    st = avformat_new_stream(oc, NULL);
    if (!st) {
	averrno = AVERROR(ENOMEM);
	return false;
    }

    // Initialise the code.
    AVCodecContext *c = st->codec;
    c->codec_id = fmt->video_codec;
    c->codec_type = AVMEDIA_TYPE_VIDEO;

    // Set sample parameters.
    c->bit_rate = 400000;
    c->width = width;
    c->height = height;
    c->time_base.num = 1;
    c->time_base.den = 25; // Frames per second.
    c->gop_size = 12; // One intra frame every twelve frames.
    c->pix_fmt = PIX_FMT_YUV420P;
    // B frames are backwards predicted - they can improve compression,
    // but may slow encoding and decoding.
    // c->max_b_frames = 2;

    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
	c->flags |= CODEC_FLAG_GLOBAL_HEADER;

    int retval;
#ifndef HAVE_AVFORMAT_WRITE_HEADER
    // Set the output parameters (must be done even if no parameters).
    retval = av_set_parameters(oc, NULL);
    if (retval < 0) {
	averrno = retval;
	return false;
    }
#endif

    // Show the format we've ended up with (for debug purposes).
    // dump_format(oc, 0, fnm, 1);

    // Open the video codec and allocate the necessary encode buffers.
    AVCodec * codec = avcodec_find_encoder(c->codec_id);
    if (!codec) {
	// FIXME : Erm - internal ffmpeg library problem?
	return false;
    }

    retval = avcodec_open2(c, codec, NULL);
    if (retval < 0) {
	averrno = retval;
	return false;
    }

    if ((oc->oformat->flags & AVFMT_RAWPICTURE)) {
	outbuf = NULL;
    } else {
	outbuf = (unsigned char *)malloc(OUTBUF_SIZE);
	if (!outbuf) {
	    averrno = AVERROR(ENOMEM);
	    return false;
	}
    }

    frame = avcodec_alloc_frame();
    if (!frame) {
	averrno = AVERROR(ENOMEM);
	return false;
    }
    int size = avpicture_get_size(c->pix_fmt, width, height);
    uint8_t * picture_buf = (uint8_t*)av_malloc(size);
    if (!picture_buf) {
	av_free(frame);
	averrno = AVERROR(ENOMEM);
	return false;
    }
    avpicture_fill((AVPicture *)frame, picture_buf, c->pix_fmt, width, height);

    if (c->pix_fmt != PIX_FMT_YUV420P) {
	// FIXME need to allocate another frame for this case if we stop
	// hardcoding PIX_FMT_YUV420P.
	abort();
    }

    pixels = (unsigned char *)malloc(width * height * 6);
    if (!pixels) {
	averrno = AVERROR(ENOMEM);
	return false;
    }

    retval = avio_open(&oc->pb, fnm, URL_WRONLY);
    if (retval < 0) {
	averrno = retval;
	return false;
    }

    // Write the stream header, if any.
#ifdef HAVE_AVFORMAT_WRITE_HEADER
    retval = avformat_write_header(oc, NULL);
#else
    retval = av_write_header(oc);
#endif
    if (retval < 0) {
	averrno = retval;
	return false;
    }

    av_free(sws_ctx);
    sws_ctx = sws_getContext(width, height, PIX_FMT_RGB24,
			     width, height, c->pix_fmt, SWS_BICUBIC,
			     NULL, NULL, NULL);
    if (sws_ctx == NULL) {
	fprintf(stderr, "Cannot initialize the conversion context!\n");
	return false;
    }

    averrno = 0;
    return true;
#else
    averrno = MOVIE_NOT_ENABLED;
    return false;
#endif
}

unsigned char * MovieMaker::GetBuffer() const {
#ifdef HAVE_LIBAVFORMAT_AVFORMAT_H
    AVCodecContext * c = st->codec;
    return pixels + c->height * c->width * 3;
#else
    return NULL;
#endif
}

int MovieMaker::GetWidth() const {
    assert(st);
#ifdef HAVE_LIBAVFORMAT_AVFORMAT_H
    AVCodecContext *c = st->codec;
    return c->width;
#else
    return 0;
#endif
}

int MovieMaker::GetHeight() const {
    assert(st);
#ifdef HAVE_LIBAVFORMAT_AVFORMAT_H
    AVCodecContext *c = st->codec;
    return c->height;
#else
    return 0;
#endif
}

void MovieMaker::AddFrame()
{
#ifdef HAVE_LIBAVFORMAT_AVFORMAT_H
    AVCodecContext * c = st->codec;

    if (c->pix_fmt != PIX_FMT_YUV420P) {
	// FIXME convert...
	abort();
    }

    int len = 3 * c->width;
    {
	// Flip image vertically
	int h = c->height;
	unsigned char * src = pixels + h * len;
	unsigned char * dest = src - len;
	while (h--) {
	    memcpy(dest, src, len);
	    src += len;
	    dest -= len;
	}
    }
    sws_scale(sws_ctx, &pixels, &len, 0, c->height, frame->data, frame->linesize);

    if (oc->oformat->flags & AVFMT_RAWPICTURE) {
	abort();
    }

    // Encode this frame.
    out_size = avcodec_encode_video(c, outbuf, OUTBUF_SIZE, frame);
    // outsize == 0 means that this frame has been buffered, so there's nothing
    // to write yet.
    if (out_size) {
	// Write the compressed frame to the media file.
	AVPacket pkt;
	av_init_packet(&pkt);

	if (c->coded_frame->pts != (int64_t)AV_NOPTS_VALUE)
	    pkt.pts = av_rescale_q(c->coded_frame->pts, c->time_base, st->time_base);
	if (c->coded_frame->key_frame)
	    pkt.flags |= AV_PKT_FLAG_KEY;
	pkt.stream_index = st->index;
	pkt.data = outbuf;
	pkt.size = out_size;

	/* write the compressed frame in the media file */
	if (av_interleaved_write_frame(oc, &pkt) != 0) {
	    abort();
	}
    }
#endif
}

MovieMaker::~MovieMaker()
{
#ifdef HAVE_LIBAVFORMAT_AVFORMAT_H
    if (st && averrno == 0) {
	// No more frames to compress.  The codec may have a few frames
	// buffered if we're using B frames, so write those too.
	AVCodecContext * c = st->codec;

	while (out_size) {
	    out_size = avcodec_encode_video(c, outbuf, OUTBUF_SIZE, NULL);
	    if (out_size) {
		// Write the compressed frame to the media file.
		AVPacket pkt;
		av_init_packet(&pkt);

		if (c->coded_frame->pts != (int64_t)AV_NOPTS_VALUE)
		    pkt.pts = av_rescale_q(c->coded_frame->pts, c->time_base, st->time_base);
		if (c->coded_frame->key_frame)
		    pkt.flags |= AV_PKT_FLAG_KEY;
		pkt.stream_index = st->index;
		pkt.data = outbuf;
		pkt.size = out_size;

		/* write the compressed frame in the media file */
		if (av_interleaved_write_frame(oc, &pkt) != 0) {
		    abort();
		}
	    }
	}

	av_write_trailer(oc);
    }

    if (st) {
	// Close codec.
	avcodec_close(st->codec);
    }

    if (frame) {
	free(frame->data[0]);
	free(frame);
    }
    free(outbuf);
    free(pixels);
    av_free(sws_ctx);

    if (oc) {
	// Free the streams.
	for (size_t i = 0; i < oc->nb_streams; ++i) {
	    av_freep(&oc->streams[i]->codec);
	    av_freep(&oc->streams[i]);
	}

	if (!(oc->oformat->flags & AVFMT_NOFILE)) {
	    // Close the output file.
	    avio_close(oc->pb);
	}

	// Free the stream.
	av_free(oc);
    }
#endif
}

const char *
MovieMaker::get_error_string() const
{
#ifdef HAVE_LIBAVFORMAT_AVFORMAT_H
    switch (averrno) {
	case AVERROR(EIO):
	    return "I/O error";
	case AVERROR(EDOM):
	    return "Number syntax expected in filename";
	case AVERROR_INVALIDDATA:
	    /* same as AVERROR_UNKNOWN: return "unknown error"; */
	    return "invalid data found";
	case AVERROR(ENOMEM):
	    return "not enough memory";
	case AVERROR(EILSEQ):
	    return "unknown format";
	case AVERROR(ENOSYS):
	    return "Operation not supported";
	case AVERROR(ENOENT):
	    return "No such file or directory";
	case AVERROR_EOF:
	    return "End of file";
	case AVERROR_PATCHWELCOME:
	    return "Not implemented in FFmpeg";
	case 0:
	    return "No error";
	case MOVIE_NO_SUITABLE_FORMAT:
	    return "Couldn't find a suitable output format";
	case MOVIE_AUDIO_ONLY:
	    return "Audio-only format specified";
	case MOVIE_FILENAME_TOO_LONG:
	    return "Filename too long";
	case MOVIE_NOT_ENABLED:
	    return "Movie export support not included";
    }
    return "Unknown error";
#else
    return "Movie generation support code not present";
#endif
}
