// AudioDecoderFfmpeg.cpp: Audio decoding using the FFmpeg library.
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
//

#include "AudioDecoderFfmpeg.h"

#include <cmath> // for std::ceil
#include <algorithm> // for std::copy, std::max

#include "MediaParserFfmpeg.h" // for ExtraAudioInfoFfmpeg
#include "FLVParser.h"
#include "SoundInfo.h"
#include "MediaParser.h" // for AudioInfo

//#define GNASH_DEBUG_AUDIO_DECODING

namespace gnash {
namespace media {
namespace ffmpeg {
    
AudioDecoderFfmpeg::AudioDecoderFfmpeg(const AudioInfo& info)
    :
    _audioCodec(nullptr),
    _audioCodecCtx(nullptr),
    _parser(nullptr),
    _needsParsing(false)
{
    setup(info);

    if (info.type == CODEC_TYPE_CUSTOM) {
        log_debug(_("AudioDecoderFfmpeg: initialized FFmpeg codec %d (%s)"),
            _audioCodec->id, _audioCodec->name);
    } else {
        log_debug(_("AudioDecoderFfmpeg: initialized FFmpeg codec %d (%s) "
                    "for flash codec %d (%s)"),
            _audioCodec->id, _audioCodec->name,
            info.codec, (audioCodecType)info.codec);
    }
}

AudioDecoderFfmpeg::AudioDecoderFfmpeg(SoundInfo& info)
    :
    _audioCodec(nullptr),
    _audioCodecCtx(nullptr),
    _parser(nullptr)
{
    setup(info);

      log_debug(_("AudioDecoderFfmpeg: initialized FFmpeg codec %s (%d)"),
        _audioCodec->name, _audioCodec->id);
}

AudioDecoderFfmpeg::~AudioDecoderFfmpeg()
{
    if (_audioCodecCtx)
    {
        avcodec_close(_audioCodecCtx);
        av_free(_audioCodecCtx);
    }
    if (_parser) av_parser_close(_parser);
}

void AudioDecoderFfmpeg::setup(SoundInfo& info)
{
    avcodec_register_all();// change this to only register need codec?

    enum CODECID codec_id;

    switch(info.getFormat()) {
        case AUDIO_CODEC_RAW:
            codec_id = AV_CODEC_ID_PCM_U16LE;
            break;
        case AUDIO_CODEC_ADPCM:
            codec_id = AV_CODEC_ID_ADPCM_SWF;
            break;
        case AUDIO_CODEC_MP3:
            codec_id = AV_CODEC_ID_MP3;
            _needsParsing=true;
            break;
        case AUDIO_CODEC_AAC:
            codec_id = AV_CODEC_ID_AAC;
            _needsParsing=true;
            break;
        default:
            boost::format err = boost::format(
                _("Unsupported audio codec %d")) %
                static_cast<int>(info.getFormat());
            throw MediaException(err.str());
    }

    _audioCodec = avcodec_find_decoder(codec_id);
    if (!_audioCodec) {
        audioCodecType codec = info.getFormat();
        boost::format err = boost::format(
            _("libavcodec could not find a decoder for codec %d (%s)")) %
            static_cast<int>(codec) % codec;
        throw MediaException(err.str());
    }

    if ( _needsParsing )
    {
        // Init the parser
        _parser = av_parser_init(codec_id);
        if (!_parser) {    
        throw MediaException(_("AudioDecoderFfmpeg can't initialize "
                    "MP3 parser"));
        }
    }

    _audioCodecCtx = avcodec_alloc_context3(_audioCodec);
    if (!_audioCodecCtx) {
        throw MediaException(_("libavcodec couldn't allocate context"));
    }

    int ret = avcodec_open2(_audioCodecCtx, _audioCodec, nullptr);
    if (ret < 0) {
        av_free(_audioCodecCtx);
        _audioCodecCtx=nullptr;
        boost::format err = boost::format(
            _("AudioDecoderFfmpeg: avcodec_open failed to initialize "
            "FFmpeg codec %s (%d)")) % _audioCodec->name % (int)codec_id;
        throw MediaException(err.str());
    }

    log_debug(_("AudioDecoder: initialized FFMPEG codec %s (%d)"), 
                _audioCodec->name, (int)codec_id);

    /// @todo do this only if !_needsParsing ?
    switch (_audioCodecCtx->codec->id)
    {
            case AV_CODEC_ID_MP3:
                break;

            case AV_CODEC_ID_PCM_U16LE:
                _audioCodecCtx->channels = (info.isStereo() ? 2 : 1);
                _audioCodecCtx->sample_rate = info.getSampleRate();
                _audioCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16; // ?! arbitrary ?
                _audioCodecCtx->frame_size = 1; 
                break;

            default:
                _audioCodecCtx->channels = (info.isStereo() ? 2 : 1);
                _audioCodecCtx->sample_rate = info.getSampleRate();
                _audioCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16; // ?! arbitrary ?
                break;
    }
}

void AudioDecoderFfmpeg::setup(const AudioInfo& info)
{
    // Init the avdecoder-decoder
    avcodec_register_all();// change this to only register need codec?

    enum CODECID codec_id = AV_CODEC_ID_NONE;

    if (info.type == CODEC_TYPE_CUSTOM)
    {
        codec_id = static_cast<CODECID>(info.codec);
    }
    else if (info.type == CODEC_TYPE_FLASH)
    {

        switch(info.codec)
        {
            case AUDIO_CODEC_UNCOMPRESSED:
            case AUDIO_CODEC_RAW:
                if (info.sampleSize == 2) {
                    // Flash's 16-bit UNCOMPRESSED and RAW audio comes as
                    // a signed PCM format. UNCOMPRESSED audio always use
                    // little-endian byte order. RAW audio uses encoder's
                    // native byte order; as majority of encoders run
                    // on little-endian machine, we use little-endian for it.
                    codec_id = AV_CODEC_ID_PCM_S16LE;
                } else {
                    // Flash's 8-bit UNCOMPRESSED and RAW audio comes as an
                    // unsigned PCM format. No difference between
                    // UNCOMPRESSED and RAW.
                    codec_id = AV_CODEC_ID_PCM_U8;
                }
                break;

            case AUDIO_CODEC_ADPCM:
                codec_id = AV_CODEC_ID_ADPCM_SWF;
                break;

            case AUDIO_CODEC_MP3:
                codec_id = AV_CODEC_ID_MP3;
                break;

            case AUDIO_CODEC_AAC:
                codec_id = AV_CODEC_ID_AAC;
                break;

#ifdef FFMPEG_NELLYMOSER
            // NOTE: bjacques found this failing in decodeFrame
            //       (but probably not Ffmpeg's fault, he said)
            //       I'd like to take a look at the testcase --strk
            case AUDIO_CODEC_NELLYMOSER:
                codec_id = AV_CODEC_ID_NELLYMOSER;
                break;
#endif

            default:
                boost::format err = boost::format(
                    _("AudioDecoderFfmpeg: unsupported flash audio "
                        "codec %d (%s)")) %
                        info.codec % (audioCodecType)info.codec;
                throw MediaException(err.str());
        }
    }
    else
    {
        boost::format err = boost::format(
            _("AudioDecoderFfmpeg: unknown codec type %d "
            "(should never happen)")) % info.type;
        throw MediaException(err.str());
    }

    _audioCodec = avcodec_find_decoder(codec_id);
    if (!_audioCodec)
    {
        if (info.type == CODEC_TYPE_FLASH) {
            boost::format err = boost::format(
                _("AudioDecoderFfmpeg: libavcodec could not find a decoder "
                    "for codec %d (%s)")) %
                    info.codec % static_cast<audioCodecType>(info.codec);
            throw MediaException(err.str());
        } else {
            boost::format err = boost::format(
                _("AudioDecoderFfmpeg: libavcodec could not find a decoder "
                    "for ffmpeg codec id %s")) % codec_id;
            throw MediaException(err.str());
        }
    }

    _parser = av_parser_init(codec_id);
    _needsParsing = (_parser != nullptr);

    // Create an audioCodecCtx from the ffmpeg parser if exists/possible
    _audioCodecCtx = avcodec_alloc_context3(_audioCodec);
    if (!_audioCodecCtx) {
        throw MediaException(_("AudioDecoderFfmpeg: libavcodec couldn't "
                    "allocate context"));
    }

    if ( info.extra.get() )
    {
        if (dynamic_cast<ExtraAudioInfoFfmpeg*>(info.extra.get())) {
            const ExtraAudioInfoFfmpeg& ei = 
                static_cast<ExtraAudioInfoFfmpeg&>(*info.extra);
            _audioCodecCtx->extradata = ei.data;
            _audioCodecCtx->extradata_size = ei.dataSize;
        } else if (dynamic_cast<ExtraAudioInfoFlv*>(info.extra.get())) {
            ExtraAudioInfoFlv* extra = 
                static_cast<ExtraAudioInfoFlv*>(info.extra.get());
            _audioCodecCtx->extradata = extra->data.get();
            _audioCodecCtx->extradata_size = extra->size;
        }
    }

    // Setup known configurations for the audio codec context
    // NOTE: this is done before calling avcodec_open, as that might update
    //       some of the variables
    switch (codec_id)
    {
            case AV_CODEC_ID_MP3:
                break;

            case AV_CODEC_ID_PCM_U8:
                _audioCodecCtx->sample_rate = info.sampleRate;
                _audioCodecCtx->channels = (info.stereo ? 2 : 1);
                break;
            case AV_CODEC_ID_PCM_S16LE:
                _audioCodecCtx->channels = (info.stereo ? 2 : 1);
                _audioCodecCtx->sample_rate = info.sampleRate;
                break;

            default:
                _audioCodecCtx->channels = (info.stereo ? 2 : 1);
                _audioCodecCtx->sample_rate = info.sampleRate;
                // was commented out (why?):
                _audioCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;
                break;
    }


#ifdef GNASH_DEBUG_AUDIO_DECODING
    log_debug("  Opening codec");
#endif // GNASH_DEBUG_AUDIO_DECODING
    int ret = avcodec_open2(_audioCodecCtx, _audioCodec, nullptr);
    if (ret < 0) {
        //avcodec_close(_audioCodecCtx);
        av_free(_audioCodecCtx);
        _audioCodecCtx = nullptr;

        boost::format err = boost::format(
            _("AudioDecoderFfmpeg: avcodec_open failed to initialize "
            "FFmpeg codec %s (%d)")) % _audioCodec->name % (int)codec_id;
        throw MediaException(err.str());
    }

}

std::uint8_t*
AudioDecoderFfmpeg::decode(const std::uint8_t* input,
        std::uint32_t inputSize, std::uint32_t&
        outputSize, std::uint32_t& decodedBytes)
{
    //GNASH_REPORT_FUNCTION;

    size_t retCapacity = MAX_AUDIO_FRAME_SIZE;
    std::uint8_t* retBuf = new std::uint8_t[retCapacity];
    int retBufSize = 0;

#ifdef GNASH_DEBUG_AUDIO_DECODING
    log_debug("  Parsing loop starts, input is %d bytes, retCapacity is %d "
            "bytes", inputSize, retCapacity);
#endif
    decodedBytes = 0; // nothing decoded yet
    while (decodedBytes < inputSize)
    {
        const std::uint8_t* frame=nullptr; // parsed frame (pointer into input)
        int framesize; // parsed frame size

        int consumed = parseInput(input+decodedBytes,
                             inputSize-decodedBytes,
                             &frame, &framesize);
        if (consumed < 0)
        {
            log_error(_("av_parser_parse returned %d. "
                "Upgrading ffmpeg/libavcodec might fix this issue."),
                consumed);
            // Setting data position to data size will get the sound removed
            // from the active sound list later on.
            decodedBytes = inputSize;
            break;
        }

#ifdef GNASH_DEBUG_AUDIO_DECODING
        log_debug("   parsed frame is %d bytes (consumed +%d = %d/%d)",
                framesize, consumed, decodedBytes+consumed, inputSize);
#endif

#if GNASH_PARANOIA_LEVEL > 1
        if ( frame )
        {
            // the returned frame pointer is inside the input buffer
            assert(frame == input+decodedBytes);
            // the returned frame size is within the input size
            assert(framesize <= inputSize);
        }
#endif

        // all good so far, keep going..
        // (we might do this immediately, as we'll override decodedBytes
        // on error anyway)
        decodedBytes += consumed;

        if ( ! framesize )
        {
            // See https://savannah.gnu.org/bugs/?25456
            // for a live testcase
            log_debug("AudioDecoderFfmpeg: "
                      "could not find a complete frame in "
                      "the last %d bytes of a %d bytes block"
                        " (nothing should be lost)",
                      consumed, inputSize);
            break; 
        }


        // Now, decode the frame. We use the ::decodeFrame specialized function
        // here so resampling is done appropriately
        std::uint32_t outSize = 0;
        std::unique_ptr<std::uint8_t[]> outBuf(
                decodeFrame(frame, framesize, outSize));

        if (!outBuf)
        {
            // Setting data position to data size will get the sound removed
            // from the active sound list later on.
            decodedBytes = inputSize;
            break;
        }

#ifdef GNASH_DEBUG_AUDIO_DECODING
        log_debug("   decoded frame is %d bytes, would grow return "
                "buffer size to %d bytes", outSize,
                retBufSize+static_cast<unsigned int>(outSize));
#endif 

        //
        // Now append this data to the buffer we're going to return
        //

        // if the new data doesn't fit, reallocate the output
        // TODO: can use the Buffer class here.. if we return it too...
        if ( retBufSize+(unsigned)outSize > retCapacity )
        {
#ifdef GNASH_DEBUG_AUDIO_DECODING
            log_debug("    output buffer won't be able to hold %d bytes, "
                    "capacity is only %d bytes",
                    retBufSize+(unsigned)outSize, retCapacity);
#endif 

            std::uint8_t* tmp = retBuf;
            retCapacity = std::max(retBufSize+static_cast<size_t>(outSize),
                    retCapacity * 2);

#ifdef GNASH_DEBUG_AUDIO_DECODING
            log_debug("    reallocating it to hold up to %d bytes",
                    retCapacity);
#endif // GNASH_DEBUG_AUDIO_DECODING

            retBuf = new std::uint8_t[retCapacity];
            if ( retBufSize ) std::copy(tmp, tmp+retBufSize, retBuf);
            delete [] tmp;
        }
        std::copy(outBuf.get(), outBuf.get()+outSize, retBuf+retBufSize);
        retBufSize += static_cast<unsigned int>(outSize);
    }

    
    outputSize = retBufSize;
    return retBuf;

}

std::uint8_t*
AudioDecoderFfmpeg::decode(const EncodedAudioFrame& ef,
        std::uint32_t& outputSize)
{
    return decodeFrame(ef.data.get(), ef.dataSize, outputSize);
}

/* basically a polyfill */
int decode_audio4(AVCodecContext *avctx, AVFrame *frame, int *got_frame, AVPacket *pkt)
{
    int ret;

    *got_frame = 0;
    if(avctx) {
        avctx->thread_safe_callbacks=1;
    }
    if (pkt) {
        ret = avcodec_send_packet(avctx, pkt);
        // In particular, we don't expect AVERROR(EAGAIN), because we read all
        // decoded frames with avcodec_receive_frame() until done.
        if (ret < 0)
            return ret == AVERROR_EOF ? 0 : ret;
    }

    ret = avcodec_receive_frame(avctx, frame);
    if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
        return ret;
    if (ret >= 0)
        *got_frame = 1;

    return 0;
}

std::uint8_t*
AudioDecoderFfmpeg::decodeFrame(const std::uint8_t* input,
        std::uint32_t inputSize, std::uint32_t& outputSize)
{
    //GNASH_REPORT_FUNCTION;

    assert(inputSize);

    size_t outSize = MAX_AUDIO_FRAME_SIZE;

    // TODO: make this a private member, to reuse (see NetStreamFfmpeg in 0.8.3)
    std::unique_ptr<std::int16_t, decltype(av_free)*> output(
        reinterpret_cast<std::int16_t*>(av_malloc(outSize)), av_free );
    if (!output.get()) {
        log_error(_("failed to allocate audio buffer."));
        outputSize = 0;
        return nullptr;
    }

    std::int16_t* outPtr = output.get();


#ifdef GNASH_DEBUG_AUDIO_DECODING
    log_debug("AudioDecoderFfmpeg: about to decode %d bytes; "
        "ctx->channels:%d, ctx->frame_size:%d",
        inputSize, _audioCodecCtx->channels, _audioCodecCtx->frame_size);
#endif

    // older ffmpeg versions didn't accept a const input..
    AVPacket pkt;
    int got_frm = 0;
    av_init_packet(&pkt);
    pkt.data = const_cast<uint8_t*>(input);
    pkt.size = inputSize;
    std::unique_ptr<AVFrame, FrameDeleter> frm(FRAMEALLOC(), FrameDeleter());
    if (!frm.get()) {
        log_error(_("failed to allocate frame."));
        return nullptr;
    }
    int tmp = decode_audio4(_audioCodecCtx, frm.get(), &got_frm, &pkt);

#ifdef GNASH_DEBUG_AUDIO_DECODING
    const char* fmtname = av_get_sample_fmt_name(_audioCodecCtx->sample_fmt);
    log_debug(" decodeFrame | frm->nb_samples: %d | &got_frm: %d | "
        "returned %d | inputSize: %d",
        frm->nb_samples, got_frm, tmp, inputSize);
#endif

    int plane_size;
    if (tmp >= 0 && got_frm) {
        int data_size = av_samples_get_buffer_size( &plane_size,
            _audioCodecCtx->channels, frm->nb_samples,
            _audioCodecCtx->sample_fmt, 1);
        if (static_cast<int>(outSize) < data_size) {
            log_error(_("output buffer size is too small for the current frame "
                "(%d < %d)"), outSize, data_size);
            return nullptr;
        }

        memcpy(outPtr, frm->extended_data[0], plane_size);

#if !(defined(HAVE_SWRESAMPLE_H) || defined(HAVE_AVRESAMPLE_H))
        int planar = av_sample_fmt_is_planar(_audioCodecCtx->sample_fmt);
        if (planar && _audioCodecCtx->channels > 1) {
            uint8_t *out = ((uint8_t *)outPtr) + plane_size;
            for (int ch = 1; ch < _audioCodecCtx->channels; ch++) {
                memcpy(out, frm->extended_data[ch], plane_size);
                out += plane_size;
            }
        }
#endif

        outSize = data_size;
#ifdef GNASH_DEBUG_AUDIO_DECODING
        log_debug(" decodeFrame | fmt: %d | fmt_name: %s | planar: %d | "
            "plane_size: %d | outSize: %d",
            _audioCodecCtx->sample_fmt, fmtname, planar, plane_size, outSize);
#endif
    } else {
        if (tmp < 0)
            log_error(_("avcodec_decode_audio returned %d."), tmp);
        if (outSize < 2)
            log_error(_("outputSize:%d after decoding %d bytes of input audio "
                "data."), outputSize, inputSize);
        log_error(_("Upgrading ffmpeg/libavcodec might fix this issue."));
        outputSize = 0;
        return nullptr;
    }

    // Resampling is needed.
    if (_resampler.init(_audioCodecCtx)) {
        // Resampling is needed.

        // Compute new size based on frame_size and
        // resampling configuration

        // Find out the needed sample rate scaling
        double resampleFactor = 44100.0/_audioCodecCtx->sample_rate;

        // Compute total number of input samples
        int inSamples = outSize;
        bool stereo = _audioCodecCtx->channels > 1 ? true : false;

        if (stereo) inSamples = inSamples >> 1;
        if (_audioCodecCtx->sample_fmt == AV_SAMPLE_FMT_S16 ||
            _audioCodecCtx->sample_fmt == AV_SAMPLE_FMT_S16P) {
            inSamples = inSamples >> 1;
        }

        // Compute total number of output samples
        int expectedMaxOutSamples = std::ceil(inSamples*resampleFactor);

        // Compute output buffer size (in bytes); by multiplying
        // output samples count with output sample format's frame size,
        // which is number of bytes per sample (2) times channels (2).
        int resampledFrameSize = expectedMaxOutSamples*2*2;

        // Allocate just the required amount of bytes
        std::uint8_t* resampledOutput = new std::uint8_t[resampledFrameSize];

#ifdef GNASH_DEBUG_AUDIO_DECODING
        log_debug(" decodeFrame | Calling the resampler, resampleFactor: %d | "
            "in %d hz %d ch %d bytes %d samples, %s fmt", resampleFactor,
            _audioCodecCtx->sample_rate, _audioCodecCtx->channels, outSize,
            inSamples, fmtname);
        log_debug(" decodeFrame | out 44100 hz 2 ch %d bytes",
            resampledFrameSize);
#endif

        int outSamples = _resampler.resample(frm->extended_data, // input
            plane_size, // input
            frm->nb_samples, // input
            &resampledOutput); // output

        // make sure to set outPtr *after* we use it as input to the resampler
        outPtr = reinterpret_cast<std::int16_t*>(resampledOutput);

#ifdef GNASH_DEBUG_AUDIO_DECODING
        log_debug("resampler returned %d samples ", outSamples);
#endif

        if (expectedMaxOutSamples < outSamples) {
            log_error(_(" --- Computation of resampled samples (%d) < then the actual returned samples (%d)"),
                expectedMaxOutSamples, outSamples);

            log_debug(" input frame size: %d", outSize);
            log_debug(" input sample rate: %d", _audioCodecCtx->sample_rate);
            log_debug(" input channels: %d", _audioCodecCtx->channels);
            log_debug(" input samples: %d", inSamples);

            log_debug(" output sample rate (assuming): %d", 44100);
            log_debug(" output channels (assuming): %d", 2);
            log_debug(" output samples: %d", outSamples);

            /// Memory errors...
            abort();
        }

        // Use the actual number of samples returned, multiplied
        // to get size in bytes (not two-byte samples) and for 
        // stereo?
        outSize = outSamples * 2 * 2;

    }
    else {
        std::uint8_t* newOutput = new std::uint8_t[outSize];
        std::memcpy(newOutput, outPtr, outSize);
        outPtr = reinterpret_cast<std::int16_t*>(newOutput);
    }

    outputSize = outSize;
    return reinterpret_cast<uint8_t*>(outPtr);
}

int
AudioDecoderFfmpeg::parseInput(const std::uint8_t* input,
        std::uint32_t inputSize,
        std::uint8_t const ** outFrame, int* outFrameSize)
{
    if ( _needsParsing )
    {
        return av_parser_parse2(_parser, _audioCodecCtx,
                    // as of 2008-10-28 SVN, ffmpeg doesn't
                    // accept a pointer to pointer to const..
                    const_cast<std::uint8_t**>(outFrame),
                    outFrameSize,
                    input, inputSize,
                    0, 0, AV_NOPTS_VALUE); // pts, dts, pos
    }
    else
    {
        // democratic value for a chunk to decode...
        // @todo this might be constrained by codec id, check that !

        // NOTE: AVCODEC_MAX_AUDIO_FRAME_SIZE (192000, deprecated replaced with
        //       MAX_AUDIO_FRAME_SIZE) resulted bigger
        //       than avcodec_decode_audio could handle, resulting
        //       in eventSoundTest1.swf regression.
        //static const unsigned int maxFrameSize = AVCODEC_MAX_AUDIO_FRAME_SIZE;

        // NOTE: 1024 resulted too few
        //       to properly decode (or resample?) raw audio
        //       thus resulting noisy (bugs #21177 and #22284)
        //static const unsigned int maxFrameSize = 1024;

        // NOTE: 96000 was found to be the max returned
        //       by avcodec_decode_audio when passed anything
        //       bigger than that. Works fine with all of
        //       eventSoundTest1.swf, bug #21177 and bug #22284
        //
        static const unsigned int maxFrameSize = 96000;

        int frameSize = inputSize < maxFrameSize ? inputSize : maxFrameSize;

        // we assume the input is just a set of frames
        // and we'll consume all
        *outFrame = input; // frame always start on input
        *outFrameSize = frameSize;
        int parsed = frameSize;
        return parsed;
    }
}


} // gnash.media.ffmpeg namespace 
} // gnash.media namespace 
} // gnash namespace
