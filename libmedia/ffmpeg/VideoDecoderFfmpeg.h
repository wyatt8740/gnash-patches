// VideoDecoderFfmpeg.h: Video decoding using the FFMPEG library.
// 
//     Copyright (C) 2007, 2008, 2009, 2010, 2011, 2012
//   Free Software Foundation, Inc.
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA    02110-1301    USA

#ifndef GNASH_VIDEODECODERFFMPEG_H
#define GNASH_VIDEODECODERFFMPEG_H

#ifdef HAVE_CONFIG_H
#include "gnashconfig.h"
#endif

#include <vector>
#include <memory>
#include "dsodefs.h" //For DSOEXPORT
#include "VideoDecoder.h"
#include "MediaParser.h" // for videoCodecType enum
#include "ffmpegHeaders.h"

/* 
  typedef struct AVPicture {
    uint8_t *data[AV_NUM_DATA_POINTERS];
    int linesize[AV_NUM_DATA_POINTERS];
  } AVPicture;
*/

namespace gnash {
namespace media {
namespace ffmpeg {

// Forward declarations
class CodecContextWrapper;
#ifdef HAVE_SWSCALE_H
class SwsContextWrapper;
#endif


/// FFMPEG based VideoDecoder
class VideoDecoderFfmpeg : public VideoDecoder {
    
public:

    DSOEXPORT VideoDecoderFfmpeg(videoCodecType format, int width, int height);

    DSOEXPORT VideoDecoderFfmpeg(const VideoInfo& info);

    DSOEXPORT ~VideoDecoderFfmpeg();
    
    void push(const EncodedVideoFrame& buffer);

    std::unique_ptr<image::GnashImage> pop();
    
    bool peek();

    int width() const;

    int height() const;
    
private:
    
    /// Convert CODEC_TYPE_FLASH codec id to FFMPEG codec id
    //
    /// @return CODEC_ID_NONE for unsupported flash codecs
    ///
    static CODECID flashToFfmpegCodec(videoCodecType format);

    /// \brief converts an video frame from (almost) any type to RGB24.
    ///
    /// @param srcCtx The source context that was used to decode srcFrame.
    /// @param srcFrame the source frame to be converted.
    /// @return an AVPicture containing the converted image. Please be advised
    ///         that the RGB data pointer is stored in AVPicture::data[0]. The
    ///         caller owns that pointer, which must be freed with delete [].
    ///         It is advised to wrap the pointer in a boost::scoped_array.
    ///         If conversion fails, AVPicture::data[0] will be NULL.
    std::unique_ptr<image::GnashImage> frameToImage(AVCodecContext* srcCtx,
            const AVFrame& srcFrame);

    void init(enum CODECID format, int width, int height,
            std::uint8_t* extradata=nullptr, int extradataSize=0);

    std::unique_ptr<image::GnashImage> decode(const std::uint8_t* input,
            std::uint32_t input_size);

    std::unique_ptr<image::GnashImage> decode(const EncodedVideoFrame* vf)
    {
    	return decode(vf->data(), vf->dataSize());
    }

    AVCodec* _videoCodec;
    std::unique_ptr<CodecContextWrapper> _videoCodecCtx;

#if HAVE_SWSCALE_H
    /// A pointer to a wrapper round an SwsContext
    //
    /// This is constructed with a SwsContext*, which
    /// can be NULL, so it is important to check
    /// not only that the wrapper exists, but also
    /// the context inside it.    
    std::unique_ptr<SwsContextWrapper> _swsContext;
#endif

    std::vector<const EncodedVideoFrame*> _video_frames;
};
    
} // gnash.media.ffmpeg namespace 
} // gnash.media namespace 
} // gnash namespace

#endif // __VIDEODECODERFFMPEG_H__
