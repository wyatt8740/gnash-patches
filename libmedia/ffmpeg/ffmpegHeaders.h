// ffmpegHeaders.h - hide braindamage required to support ffmpeg includes in a single file
// 
//   Copyright (C) 2007, 2008, 2009, 2010, 2011, 2012
//   Free Software Foundation, Inc.
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef GNASH_MEDIA_FFMPEG_HEADERS_H
#define GNASH_MEDIA_FFMPEG_HEADERS_H

#ifdef HAVE_CONFIG_H
#include "gnashconfig.h"
#endif

/// Allows complilation of 64-bit constants on a 32-bit machine.
#ifndef __STDC_CONSTANT_MACROS
# define __STDC_CONSTANT_MACROS
#endif

// This is for compatibility with braindamaged versions of ffmpeg
#if !defined INT64_C
#if defined __WORDSIZE && __WORDSIZE == 64
#define INT64_C(c) c ## L
#else
#define INT64_C(c) c ## LL
#endif
#endif

#define MAX_AUDIO_FRAME_SIZE 192000

#ifdef HAVE_FFMPEG_AVCODEC_H
extern "C" {
# include <ffmpeg/avcodec.h>
}
#endif

#ifdef HAVE_LIBAVCODEC_AVCODEC_H
extern "C" {
# include <libavcodec/avcodec.h>
}
#endif

#ifdef HAVE_FFMPEG_AVFORMAT_H
extern "C" {
#include <ffmpeg/avformat.h>
}
#endif

#ifdef HAVE_LIBAVFORMAT_AVFORMAT_H
extern "C" {
#include <libavformat/avformat.h>
}
#endif


#ifdef HAVE_SWSCALE_H
extern "C" {
#include <swscale.h>
}
#endif

#ifdef HAVE_FFMPEG_SWSCALE_H
extern "C" {
#include <ffmpeg/swscale.h>
}
#define HAVE_SWSCALE_H 1
#endif

#ifdef HAVE_LIBSWSCALE_SWSCALE_H
extern "C" {
#include <libswscale/swscale.h>
}
#define HAVE_SWSCALE_H 1
#endif

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(52,94,1)
#define AV_SAMPLE_FMT_NONE SAMPLE_FMT_NONE
#define AV_SAMPLE_FMT_U8   SAMPLE_FMT_U8
#define AV_SAMPLE_FMT_S16  SAMPLE_FMT_S16
#define AV_SAMPLE_FMT_S32  SAMPLE_FMT_S32
#define AV_SAMPLE_FMT_FLT  SAMPLE_FMT_FLT
#define AV_SAMPLE_FMT_DBL  SAMPLE_FMT_DBL

#define AVSampleFormat SampleFormat
#endif

// till Debian libav 10~beta1
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,34,1)
#define AV_CODEC_ID_AAC CODEC_ID_AAC
#define AV_CODEC_ID_ADPCM_SWF CODEC_ID_ADPCM_SWF
#define AV_CODEC_ID_FLASHSV CODEC_ID_FLASHSV
#define AV_CODEC_ID_FLV1 CODEC_ID_FLV1
#define AV_CODEC_ID_H264 CODEC_ID_H264
#define AV_CODEC_ID_MP3 CODEC_ID_MP3
#define AV_CODEC_ID_NELLYMOSER CODEC_ID_NELLYMOSER
#define AV_CODEC_ID_NONE CODEC_ID_NONE
#define AV_CODEC_ID_PCM_S8 CODEC_ID_PCM_S8
#define AV_CODEC_ID_PCM_U8 CODEC_ID_PCM_U8
#define AV_CODEC_ID_PCM_S16LE CODEC_ID_PCM_S16LE
#define AV_CODEC_ID_PCM_U16LE CODEC_ID_PCM_U16LE
#define AV_CODEC_ID_VP6A CODEC_ID_VP6A
#define AV_CODEC_ID_VP6F CODEC_ID_VP6F

#define avg_frame_rate r_frame_rate
#define FRAMEALLOC avcodec_alloc_frame
struct FrameDeleter
{
    void operator()(AVFrame* frame)
    {
        av_free(frame);
    }
};
#else
#define FRAMEALLOC av_frame_alloc
struct FrameDeleter
{
    void operator()(AVFrame* frame)
    {
        av_frame_free(&frame);
    }
};
#endif

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,18,102)
#define CODECID AVCodecID
#else
#define CODECID CodecID
#endif

#ifdef HAVE_SWRESAMPLE_H
extern "C" {
#include <swresample.h>
}
#endif

#ifdef HAVE_FFMPEG_SWRESAMPLE_H
extern "C" {
#include <ffmpeg/swresample.h>
}
#define HAVE_SWRESAMPLE_H 1
#endif

#ifdef HAVE_LIBSWRESAMPLE_SWRESAMPLE_H
extern "C" {
#include <libswresample/swresample.h>
}
#define HAVE_SWRESAMPLE_H 1
#endif

#ifdef HAVE_AVRESAMPLE_H
extern "C" {
#include <avresample.h>
}
#endif

#ifdef HAVE_LIBAV_AVRESAMPLE_H
extern "C" {
#include <libav/avresample.h>
}
#define HAVE_AVRESAMPLE_H 1
#endif

#ifdef HAVE_LIBAVRESAMPLE_AVRESAMPLE_H
extern "C" {
#include <libavresample/avresample.h>
}
#define HAVE_AVRESAMPLE_H 1
#endif

#ifdef HAVE_LIBAVUTIL_OPT_H
extern "C" {
#include <libavutil/opt.h>
}
#endif

#ifdef HAVE_LIBAVUTIL_IMGUTILS_H
extern "C" {
#include <libavutil/imgutils.h>
typedef struct AVPicture {
  uint8_t *data[AV_NUM_DATA_POINTERS];
  int linesize[AV_NUM_DATA_POINTERS];
} AVPicture;
#ifndef AVPICTURE_JUNK
int avpicture_fill(AVPicture *picture, const uint8_t *ptr,
                   enum AVPixelFormat pix_fmt, int width, int height);
int avpicture_get_size(enum AVPixelFormat pix_fmt, int width, int height);
#endif
}
#endif



#if LIBAVUTIL_VERSION_MAJOR < 55
#define AVPixelFormat        PixelFormat
#define AV_PIX_FMT_YUV444P   PIX_FMT_YUV444P
#define AV_PIX_FMT_YUVJ444P  PIX_FMT_YUVJ444P
#define AV_PIX_FMT_YUV440P   PIX_FMT_YUV440P
#define AV_PIX_FMT_YUVJ440P  PIX_FMT_YUVJ440P
#define AV_PIX_FMT_YUV422P   PIX_FMT_YUV422P
#define AV_PIX_FMT_YUVJ422P  PIX_FMT_YUVJ422P
#define AV_PIX_FMT_YUV420P   PIX_FMT_YUV420P
#define AV_PIX_FMT_YUVJ420P  PIX_FMT_YUVJ420P
#define AV_PIX_FMT_YUV411P   PIX_FMT_YUV411P
#define AV_PIX_FMT_YUV410P   PIX_FMT_YUV410P
#define AV_PIX_FMT_NV12      PIX_FMT_NV12
#define AV_PIX_FMT_NV21      PIX_FMT_NV21
#define AV_PIX_FMT_YUYV422   PIX_FMT_YUYV422
#define AV_PIX_FMT_UYVY422   PIX_FMT_UYVY422
#define AV_PIX_FMT_UYYVYY411 PIX_FMT_UYYVYY411
#define AV_PIX_FMT_NONE      PIX_FMT_NONE
#define AV_PIX_FMT_RGB24     PIX_FMT_RGB24
#define AV_PIX_FMT_RGBA      PIX_FMT_RGBA
#define AV_PIX_FMT_VAAPI_VLD PIX_FMT_VAAPI_VLD
#endif

#endif // GNASH_MEDIA_FFMPEG_HEADERS_H
