//
//  moviemaker.cc
//
//  Class for writing movies from Aven for old FFmpeg
//
//  Copyright (C) 2004,2011,2012,2013,2014,2015,2016 Olly Betts
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

#include <config.h>

#define __STDC_CONSTANT_MACROS

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "moviemaker.h"

#ifdef WITH_FFMPEG
extern "C" {
# include <libavutil/imgutils.h>
# include <libavutil/mathematics.h>
# include <libavformat/avformat.h>
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
# ifndef HAVE_AV_FRAME_ALLOC
static inline AVFrame * av_frame_alloc() {
    return avcodec_alloc_frame();
}
# endif
# ifndef HAVE_AV_FRAME_FREE
#  ifdef HAVE_AVCODEC_FREE_FRAME
static inline void av_frame_free(AVFrame ** frame) {
    avcodec_free_frame(frame);
}
#  else
static inline void av_frame_free(AVFrame ** frame) {
    free((*frame)->data[0]);
    free(*frame);
    *frame = NULL;
}
#  endif
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
# if !HAVE_DECL_AV_CODEC_ID_NONE
#  define AV_CODEC_ID_NONE CODEC_ID_NONE
# endif
# if !HAVE_DECL_AV_PIX_FMT_RGB24
#  define AV_PIX_FMT_RGB24 PIX_FMT_RGB24
# endif
# if !HAVE_DECL_AV_PIX_FMT_YUV420P
#  define AV_PIX_FMT_YUV420P PIX_FMT_YUV420P
# endif
# ifndef AVIO_FLAG_WRITE
#  define AVIO_FLAG_WRITE URL_WRONLY
# endif

enum {
    MOVIE_NO_SUITABLE_FORMAT = 1,
    MOVIE_AUDIO_ONLY,
    MOVIE_FILENAME_TOO_LONG
};

# ifndef HAVE_AVCODEC_ENCODE_VIDEO2
const int OUTBUF_SIZE = 200000;
# endif
#endif

MovieMaker::MovieMaker()
{
#ifdef WITH_FFMPEG
    static bool initialised_ffmpeg = false;
    if (initialised_ffmpeg) return;

    // FIXME: register only the codec(s) we want to use...
    avcodec_register_all();
    av_register_all();

    initialised_ffmpeg = true;
#endif
}

#ifdef WITH_FFMPEG
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
#ifdef WITH_FFMPEG
    fh_to_close = fh;

    AVOutputFormat * fmt = NULL;
    char dummy_filename[MAX_EXTENSION_LEN + 3] = "x.";
    if (strlen(ext) <= MAX_EXTENSION_LEN) {
	strcpy(dummy_filename + 2, ext);
	// Pass "x." + extension to av_guess_format() to avoid having to deal
	// with wide character filenames.
	fmt = av_guess_format(NULL, dummy_filename, NULL);
    }
    if (!fmt) {
	// We couldn't deduce the output format from file extension so default
	// to MPEG.
	fmt = av_guess_format("mpeg", NULL, NULL);
	if (!fmt) {
	    averrno = MOVIE_NO_SUITABLE_FORMAT;
	    return false;
	}
	strcpy(dummy_filename + 2, "mpg");
    }
    if (fmt->video_codec == AV_CODEC_ID_NONE) {
	averrno = MOVIE_AUDIO_ONLY;
	return false;
    }

    /* Allocate the output media context. */
    oc = avformat_alloc_context();
    if (!oc) {
	averrno = AVERROR(ENOMEM);
	return false;
    }
    oc->oformat = fmt;
    strcpy(oc->filename, dummy_filename);

    /* find the video encoder */
    AVCodec *codec = avcodec_find_encoder(fmt->video_codec);
    if (!codec) {
	// FIXME : Erm - internal ffmpeg library problem?
	averrno = AVERROR(ENOMEM);
	return false;
    }

    // Add the video stream.
    video_st = avformat_new_stream(oc, codec);
    if (!video_st) {
	averrno = AVERROR(ENOMEM);
	return false;
    }

    // Set sample parameters.
    AVCodecContext *c = video_st->codec;
    c->bit_rate = 400000;
    /* Resolution must be a multiple of two. */
    c->width = width;
    c->height = height;
    /* timebase: This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identical to 1. */
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(55, 44, 0)
    // Old way, which now causes deprecation warnings.
    c->time_base.den = 25; // Frames per second.
    c->time_base.num = 1;
#else
    video_st->time_base.den = 25; // Frames per second.
    video_st->time_base.num = 1;
    c->time_base = video_st->time_base;
#endif
    c->gop_size = 12; /* emit one intra frame every twelve frames at most */
    c->pix_fmt = AV_PIX_FMT_YUV420P;
    c->rc_buffer_size = c->bit_rate * 4; // Enough for 4 seconds
    c->rc_max_rate = c->bit_rate * 2;
    // B frames are backwards predicted - they can improve compression,
    // but may slow encoding and decoding.
    // if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
    //     c->max_b_frames = 2;
    // }

    /* Some formats want stream headers to be separate. */
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

    retval = avcodec_open2(c, NULL, NULL);
    if (retval < 0) {
	averrno = retval;
	return false;
    }

#ifndef HAVE_AVCODEC_ENCODE_VIDEO2
    outbuf = NULL;
    if (!(oc->oformat->flags & AVFMT_RAWPICTURE)) {
	outbuf = (unsigned char *)av_malloc(OUTBUF_SIZE);
	if (!outbuf) {
	    averrno = AVERROR(ENOMEM);
	    return false;
	}
    }
#endif

    /* Allocate the encoded raw picture. */
    frame = av_frame_alloc();
    if (!frame) {
	averrno = AVERROR(ENOMEM);
	return false;
    }
    retval = av_image_alloc(frame->data, frame->linesize,
			    c->width, c->height, c->pix_fmt, 1);
    if (retval < 0) {
	averrno = retval;
	return false;
    }

    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
	// FIXME need to allocate another frame for this case if we stop
	// hardcoding AV_PIX_FMT_YUV420P.
	abort();
    }

    frame->format = c->pix_fmt;
    frame->width = c->width;
    frame->height = c->height;

    pixels = (unsigned char *)av_malloc(width * height * 6);
    if (!pixels) {
	averrno = AVERROR(ENOMEM);
	return false;
    }

    // Show the format we've ended up with (for debug purposes).
    // av_dump_format(oc, 0, fnm, 1);

    av_free(sws_ctx);
    sws_ctx = sws_getContext(width, height, AV_PIX_FMT_RGB24,
			     width, height, c->pix_fmt, SWS_BICUBIC,
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
#ifdef HAVE_AVFORMAT_WRITE_HEADER
    retval = avformat_write_header(oc, NULL);
#else
    retval = av_write_header(oc);
#endif
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
#ifdef WITH_FFMPEG
    return pixels + GetWidth() * GetHeight() * 3;
#else
    return NULL;
#endif
}

int MovieMaker::GetWidth() const {
#ifdef WITH_FFMPEG
    assert(video_st);
    AVCodecContext *c = video_st->codec;
    return c->width;
#else
    return 0;
#endif
}

int MovieMaker::GetHeight() const {
#ifdef WITH_FFMPEG
    assert(video_st);
    AVCodecContext *c = video_st->codec;
    return c->height;
#else
    return 0;
#endif
}

bool MovieMaker::AddFrame()
{
#ifdef WITH_FFMPEG
    AVCodecContext * c = video_st->codec;

    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
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
#ifdef HAVE_AVCODEC_ENCODE_VIDEO2
    AVPacket pkt;
    int got_packet;
    av_init_packet(&pkt);
    pkt.data = NULL;

    int ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
	averrno = ret;
	return false;
    }
    if (got_packet && pkt.size) {
	// Write the compressed frame to the media file.
	if (pkt.pts != int64_t(AV_NOPTS_VALUE)) {
	    pkt.pts = av_rescale_q(pkt.pts,
				   c->time_base, video_st->time_base);
	}
	if (pkt.dts != int64_t(AV_NOPTS_VALUE)) {
	    pkt.dts = av_rescale_q(pkt.dts,
				   c->time_base, video_st->time_base);
	}
	pkt.stream_index = video_st->index;

	/* Write the compressed frame to the media file. */
	ret = av_interleaved_write_frame(oc, &pkt);
	if (ret < 0) {
	    averrno = ret;
	    return false;
	}
    }
#else
    out_size = avcodec_encode_video(c, outbuf, OUTBUF_SIZE, frame);
    // outsize == 0 means that this frame has been buffered, so there's nothing
    // to write yet.
    if (out_size) {
	// Write the compressed frame to the media file.
	AVPacket pkt;
	av_init_packet(&pkt);

	if (c->coded_frame->pts != (int64_t)AV_NOPTS_VALUE)
	    pkt.pts = av_rescale_q(c->coded_frame->pts, c->time_base, video_st->time_base);
	if (c->coded_frame->key_frame)
	    pkt.flags |= AV_PKT_FLAG_KEY;
	pkt.stream_index = video_st->index;
	pkt.data = outbuf;
	pkt.size = out_size;

	/* Write the compressed frame to the media file. */
	int ret = av_interleaved_write_frame(oc, &pkt);
	if (ret < 0) {
	    averrno = ret;
	    return false;
	}
    }
#endif
#endif
    return true;
}

bool
MovieMaker::Close()
{
#ifdef WITH_FFMPEG
    if (video_st && averrno == 0) {
	// No more frames to compress.  The codec may have a few frames
	// buffered if we're using B frames, so write those too.
	AVCodecContext * c = video_st->codec;

#ifdef HAVE_AVCODEC_ENCODE_VIDEO2
	while (1) {
	    AVPacket pkt;
	    int got_packet;
	    av_init_packet(&pkt);
	    pkt.data = NULL;
	    pkt.size = 0;

	    int ret = avcodec_encode_video2(c, &pkt, NULL, &got_packet);
	    if (ret < 0) {
		release();
		averrno = ret;
		return false;
	    }
	    if (!got_packet) break;
	    if (!pkt.size) continue;

	    // Write the compressed frame to the media file.
	    if (pkt.pts != int64_t(AV_NOPTS_VALUE)) {
		pkt.pts = av_rescale_q(pkt.pts,
				       c->time_base, video_st->time_base);
	    }
	    if (pkt.dts != int64_t(AV_NOPTS_VALUE)) {
		pkt.dts = av_rescale_q(pkt.dts,
				       c->time_base, video_st->time_base);
	    }
	    pkt.stream_index = video_st->index;

	    /* Write the compressed frame to the media file. */
	    ret = av_interleaved_write_frame(oc, &pkt);
	    if (ret < 0) {
		release();
		averrno = ret;
		return false;
	    }
	}
#else
	while (out_size) {
	    out_size = avcodec_encode_video(c, outbuf, OUTBUF_SIZE, NULL);
	    if (out_size) {
		// Write the compressed frame to the media file.
		AVPacket pkt;
		av_init_packet(&pkt);

		if (c->coded_frame->pts != (int64_t)AV_NOPTS_VALUE)
		    pkt.pts = av_rescale_q(c->coded_frame->pts, c->time_base, video_st->time_base);
		if (c->coded_frame->key_frame)
		    pkt.flags |= AV_PKT_FLAG_KEY;
		pkt.stream_index = video_st->index;
		pkt.data = outbuf;
		pkt.size = out_size;

		/* write the compressed frame in the media file */
		int ret = av_interleaved_write_frame(oc, &pkt);
		if (ret < 0) {
		    release();
		    averrno = ret;
		    return false;
		}
	    }
	}
#endif

	av_write_trailer(oc);
    }

    release();
#endif
    return true;
}

#ifdef WITH_FFMPEG
void
MovieMaker::release()
{
    if (video_st) {
	// Close codec.
	avcodec_close(video_st->codec);
	video_st = NULL;
    }

    if (frame) {
	av_frame_free(&frame);
    }
    av_free(pixels);
    pixels = NULL;
    av_free(outbuf);
    outbuf = NULL;
    av_free(sws_ctx);
    sws_ctx = NULL;

    if (oc) {
	// Free the streams.
	for (size_t i = 0; i < oc->nb_streams; ++i) {
	    av_freep(&oc->streams[i]->codec);
	    av_freep(&oc->streams[i]);
	}

	if (!(oc->oformat->flags & AVFMT_NOFILE)) {
	    // Release the AVIOContext.
	    av_free(oc->pb);
	}

	// Free the stream.
	av_free(oc);
	oc = NULL;
    }
    if (fh_to_close) {
	fclose(fh_to_close);
	fh_to_close = NULL;
    }
}
#endif

MovieMaker::~MovieMaker()
{
#ifdef WITH_FFMPEG
    release();
#endif
}

const char *
MovieMaker::get_error_string() const
{
#ifdef WITH_FFMPEG
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
