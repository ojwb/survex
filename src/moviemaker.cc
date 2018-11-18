//
//  moviemaker.cc
//
//  Class for writing movies from Aven.
//
//  Copyright (C) 2004,2011,2012,2013,2014,2015,2016,2018 Olly Betts
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

#ifdef WITH_LIBAV
extern "C" {
# include <libavutil/imgutils.h>
# include <libavutil/mathematics.h>
# include <libavformat/avformat.h>
# include <libswscale/swscale.h>
}
#endif

// Handle the "no libav/FFmpeg" case in this file.
#if !defined WITH_LIBAV || LIBAVCODEC_VERSION_MAJOR >= 57

#ifdef WITH_LIBAV
enum {
    MOVIE_NO_SUITABLE_FORMAT = 1,
    MOVIE_AUDIO_ONLY,
    MOVIE_FILENAME_TOO_LONG
};
#endif

MovieMaker::MovieMaker()
#ifdef WITH_LIBAV
    : oc(0), video_st(0), context(0), frame(0), pixels(0), sws_ctx(0), averrno(0)
#endif
{
#ifdef WITH_LIBAV
    static bool initialised_ffmpeg = false;
    if (initialised_ffmpeg) return;

#if LIBAVCODEC_VERSION_MAJOR < 58
    avcodec_register_all();
    av_register_all();
#endif

    initialised_ffmpeg = true;
#endif
}

#ifdef WITH_LIBAV
static int
write_packet(void *opaque, uint8_t *buf, int buf_size) {
    FILE * fh = (FILE*)opaque;
    size_t res = fwrite(buf, 1, buf_size, fh);
    return res > 0 ? res : -1;
}

static int64_t
seek_stream(void *opaque, int64_t offset, int whence) {
    FILE * fh = (FILE*)opaque;
    return fseek(fh, offset, whence);
}
#endif

#define MAX_EXTENSION_LEN 8

bool MovieMaker::Open(FILE* fh, const char * ext, int width, int height)
{
#ifdef WITH_LIBAV
    fh_to_close = fh;

    /* Allocate the output media context. */
    char dummy_filename[MAX_EXTENSION_LEN + 3] = "x.";
    oc = NULL;
    if (strlen(ext) <= MAX_EXTENSION_LEN) {
	// Use "x." + extension for format detection to avoid having to deal
	// with wide character filenames.
	strcpy(dummy_filename + 2, ext);
	avformat_alloc_output_context2(&oc, NULL, NULL, dummy_filename);
    }
    if (!oc) {
	averrno = MOVIE_NO_SUITABLE_FORMAT;
	return false;
    }

    AVOutputFormat * fmt = oc->oformat;
    if (fmt->video_codec == AV_CODEC_ID_NONE) {
	averrno = MOVIE_AUDIO_ONLY;
	return false;
    }

    /* find the video encoder */
    AVCodec *codec = avcodec_find_encoder(fmt->video_codec);
    if (!codec) {
	// FIXME : Erm - internal ffmpeg library problem?
	averrno = AVERROR(ENOMEM);
	return false;
    }

    // Add the video stream.
    video_st = avformat_new_stream(oc, NULL);
    if (!video_st) {
	averrno = AVERROR(ENOMEM);
	return false;
    }

    context = avcodec_alloc_context3(codec);
    context->codec_id = fmt->video_codec;
    context->width = width;
    context->height = height;
    video_st->time_base.den = 25; // Frames per second.
    video_st->time_base.num = 1;
    context->time_base = video_st->time_base;
    context->bit_rate = width * height * (4 * 0.07) * context->time_base.den / context->time_base.num;
    context->bit_rate_tolerance = context->bit_rate;
    context->global_quality = 4;
    context->rc_buffer_size = 2 * 1024 * 1024;
    context->rc_max_rate = context->bit_rate * 8;
    context->gop_size = 50; /* Twice the framerate */
    context->pix_fmt = AV_PIX_FMT_YUV420P;
    if (context->has_b_frames) {
	// B frames are backwards predicted - they can improve compression,
	// but may slow encoding and decoding.
	context->max_b_frames = 4;
    }

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
	context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    int retval;
    retval = avcodec_open2(context, codec, NULL);
    if (retval < 0) {
	averrno = retval;
	return false;
    }

    /* Allocate the encoded raw picture. */
    frame = av_frame_alloc();
    if (!frame) {
	averrno = AVERROR(ENOMEM);
	return false;
    }

    frame->format = context->pix_fmt;
    frame->width = width;
    frame->height = height;
    frame->pts = 0;

    retval = av_frame_get_buffer(frame, 32);
    if (retval < 0) {
	averrno = retval;
	return false;
    }

    if (frame->format != AV_PIX_FMT_YUV420P) {
	// FIXME need to allocate another frame for this case if we stop
	// hardcoding AV_PIX_FMT_YUV420P.
	abort();
    }

    /* copy the stream parameters to the muxer */
    retval = avcodec_parameters_from_context(video_st->codecpar, context);
    if (retval < 0) {
	averrno = retval;
	return false;
    }

    pixels = (unsigned char *)av_malloc(width * height * 6);
    if (!pixels) {
	averrno = AVERROR(ENOMEM);
	return false;
    }

    // Show the format we've ended up with (for debug purposes).
    // av_dump_format(oc, 0, dummy_filename, 1);

    av_free(sws_ctx);
    sws_ctx = sws_getContext(width, height, AV_PIX_FMT_RGB24,
			     width, height, context->pix_fmt, SWS_BICUBIC,
			     NULL, NULL, NULL);
    if (sws_ctx == NULL) {
	fprintf(stderr, "Cannot initialize the conversion context!\n");
	averrno = AVERROR(ENOMEM);
	return false;
    }

    if (!(fmt->flags & AVFMT_NOFILE)) {
	const int buf_size = 8192;
	void * buf = av_malloc(buf_size);
	oc->pb = avio_alloc_context(static_cast<uint8_t*>(buf), buf_size, 1,
				    fh, NULL, write_packet, seek_stream);
	if (!oc->pb) {
	    averrno = AVERROR(ENOMEM);
	    return false;
	}
    }

    // Write the stream header, if any.
    retval = avformat_write_header(oc, NULL);
    if (retval < 0) {
	averrno = retval;
	return false;
    }

    averrno = 0;
    return true;
#else
    (void)fh;
    (void)ext;
    (void)width;
    (void)height;
    return false;
#endif
}

unsigned char * MovieMaker::GetBuffer() const {
#ifdef WITH_LIBAV
    return pixels + GetWidth() * GetHeight() * 3;
#else
    return NULL;
#endif
}

int MovieMaker::GetWidth() const {
#ifdef WITH_LIBAV
    assert(video_st);
    return video_st->codecpar->width;
#else
    return 0;
#endif
}

int MovieMaker::GetHeight() const {
#ifdef WITH_LIBAV
    assert(video_st);
    return video_st->codecpar->height;
#else
    return 0;
#endif
}

#ifdef WITH_LIBAV
// Call with frame=NULL when done.
int
MovieMaker::encode_frame(AVFrame* frame_or_null)
{
    int ret = avcodec_send_frame(context, frame_or_null);
    if (ret < 0) return ret;

    AVPacket *pkt = av_packet_alloc();
    pkt->size = 0;
    while ((ret = avcodec_receive_packet(context, pkt)) == 0) {
	// Rescale output packet timestamp values from codec to stream timebase.
	av_packet_rescale_ts(pkt, context->time_base, video_st->time_base);
	pkt->stream_index = video_st->index;

	// Write the compressed frame to the media file.
	ret = av_interleaved_write_frame(oc, pkt);
	if (ret < 0) {
	    av_packet_free(&pkt);
	    release();
	    return ret;
	}
    }
    av_packet_free(&pkt);
    return 0;
}
#endif

bool MovieMaker::AddFrame()
{
#ifdef WITH_LIBAV
    int ret = av_frame_make_writable(frame);
    if (ret < 0) {
	averrno = ret;
	return false;
    }

    enum AVPixelFormat pix_fmt = context->pix_fmt;

    if (pix_fmt != AV_PIX_FMT_YUV420P) {
	// FIXME convert...
	abort();
    }

    int len = 3 * GetWidth();
    {
	// Flip image vertically
	int h = GetHeight();
	unsigned char * src = pixels + h * len;
	unsigned char * dest = src - len;
	while (h--) {
	    memcpy(dest, src, len);
	    src += len;
	    dest -= len;
	}
    }
    sws_scale(sws_ctx, &pixels, &len, 0, GetHeight(),
	      frame->data, frame->linesize);

    ++frame->pts;

    // Encode this frame.
    ret = encode_frame(frame);
    if (ret < 0) {
	averrno = ret;
	return false;
    }
#endif
    return true;
}

bool
MovieMaker::Close()
{
#ifdef WITH_LIBAV
    if (video_st && averrno == 0) {
	// Flush out any remaining data.
	int ret = encode_frame(NULL);
	if (ret < 0) {
	    averrno = ret;
	    return false;
	}
	av_write_trailer(oc);
    }

    release();
#endif
    return true;
}

#ifdef WITH_LIBAV
void
MovieMaker::release()
{
    // Close codec.
    avcodec_free_context(&context);
    av_frame_free(&frame);
    av_free(pixels);
    pixels = NULL;
    sws_freeContext(sws_ctx);
    sws_ctx = NULL;

    // Free the stream.
    avformat_free_context(oc);
    oc = NULL;

    if (fh_to_close) {
	fclose(fh_to_close);
	fh_to_close = NULL;
    }
}
#endif

MovieMaker::~MovieMaker()
{
#ifdef WITH_LIBAV
    release();
#endif
}

const char *
MovieMaker::get_error_string() const
{
#ifdef WITH_LIBAV
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
    }
#endif
    return "Unknown error";
}

#else

#include "moviemaker-legacy.cc"

#endif
