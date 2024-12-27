// MediaParserFfmpeg.cpp: FFMPEG media parsers, for Gnash
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

#include "ffmpegHeaders.h"
#include "MediaParserFfmpeg.h"
#include "GnashException.h"
#include "log.h"
#include "IOChannel.h"

//#define GNASH_ALLOW_VCODEC_ENV 1
// Set this to enable a special GNASH_DEFAULT_VCODEC environment variable, which
// is used as a default when the video codec can't be detected. This is a quick
// hack to make MJPEG HTTP videos work (which can't be detected as their MIME
// type is just "mixed/multipart"). Perhaps the codec will be configurable via
// ActionScript sometime. - Udo 

namespace gnash {
namespace media {
namespace ffmpeg {

namespace { 

// Used to calculate a decimal value from a ffmpeg fraction
inline double as_double(AVRational time) {
    return time.num / static_cast<double>(time.den);
}

} // anonymous namespace


int
MediaParserFfmpeg::readPacketWrapper(void* opaque, std::uint8_t* buf,
        int buf_size)
{
	MediaParserFfmpeg* p = static_cast<MediaParserFfmpeg*>(opaque);
	return p->readPacket(buf, buf_size);
}

std::int64_t
MediaParserFfmpeg::seekMediaWrapper(void *opaque, std::int64_t offset, int whence)
{
	MediaParserFfmpeg* p = static_cast<MediaParserFfmpeg*>(opaque);
	return p->seekMedia(offset, whence);
}

AVInputFormat*
MediaParserFfmpeg::probeStream()
{
    const size_t probeSize = 4096;
    const size_t bufSize = probeSize + AV_INPUT_BUFFER_PADDING_SIZE;

	std::unique_ptr<std::uint8_t[]> buffer(new std::uint8_t[bufSize]);

	assert(_stream->tell() == static_cast<std::streampos>(0));
	size_t actuallyRead = _stream->read(buffer.get(), probeSize);
    
    // Fill any padding with 0s.
    std::fill(buffer.get() + actuallyRead, buffer.get() + bufSize, 0);

	_stream->seek(0);

	if (actuallyRead < 1) {
 		throw IOException(_("MediaParserFfmpeg could not read probe data "
                    "from input"));
	}

	// Probe the file to detect the format
	AVProbeData probe_data;
	probe_data.filename = "";
	probe_data.buf = buffer.get();
    probe_data.buf_size = actuallyRead;
#if LIBAVFORMAT_VERSION_MAJOR > 55
    probe_data.mime_type = nullptr;
#endif

    AVInputFormat* ret = (AVInputFormat*)av_probe_input_format(&probe_data, 1);
	return ret;
}

bool
MediaParserFfmpeg::seek(std::uint32_t& pos)
{
    // lock the stream while reading from it, so actionscript
    // won't mess with the parser on seek  or on getBytesLoaded
    std::lock_guard<std::mutex> streamLock(_streamMutex);

    // NOTE: seeking when timestamps are unknown is a pain
    // See https://savannah.gnu.org/bugs/index.php?33085
    // TODO: newer ffmpeg versions seem to have an
    //  av_seek_frame_generic() function 
    // which may help us. May be worth taking a look
    //
    if ( pos == 0 ) {
        // Handle 0 by seeking to byte 0
        // Doing this saves lots of headakes in absence
        // of correct timestamps (which is the case for mp3)
        log_debug("Seeking MediaParserFfmpeg input to byte offset zero");
        if (av_seek_frame(_formatCtx, -1, pos, AVSEEK_FLAG_BYTE) < 0) {
            log_error(_("%s: seeking failed"), __FUNCTION__);
            return 0;
        }
    }
    else {
        // This is most likely wrong
	    log_debug("MediaParserFfmpeg::seek(%d) TESTING", pos);
        long newpos = static_cast<long>(pos / AV_TIME_BASE);
        if (av_seek_frame(_formatCtx, -1, newpos, 0) < 0) {
            log_error(_("%s: seeking failed"), __FUNCTION__);
            return 0;
        }
    }

    // We'll restart parsing
    _parsingComplete = false;

    // Finally, clear the buffers.
    // The call will also wake the parse up if it was sleeping.
    // WARNING: a race condition might be pending here:
    // If we handled to do all the seek work in the *small*
    // time that the parser runs w/out mutex locked (ie:
    // after it unlocked the stream mutex and before it locked
    // the queue mutex), it will still push an old encoded frame
    // to the queue; if the pushed frame alone makes it block
    // again (bufferFull) we'll have a problem.
    // Note though, that a single frame can't reach a bufferFull
    // condition, as it takes at least two for anything != 0.
    //
    clearBuffers();

    return true;
}

bool
MediaParserFfmpeg::parseVideoFrame(AVPacket& packet)
{
	assert(packet.stream_index == _videoStreamIndex);
	assert(_videoStream);

	// packet.dts is "decompression" timestamp
	// packet.pts is "presentation" timestamp
	// Dunno why we use dts, and don't understand the magic formula either...
	//
	// From ffmpeg dox:
	//    pkt->pts can be AV_NOPTS_VALUE if the video format has B frames,
	//    so it is better to rely on pkt->dts if you do not decompress the payload.
	//
	std::uint64_t timestamp = static_cast<std::uint64_t>(packet.dts * as_double(_videoStream->time_base) * 1000.0);

#if 0
	LOG_ONCE( log_unimpl("%s", __PRETTY_FUNCTION__) );
	return false;
#else

	// flags, for keyframe
	//bool isKeyFrame = packet.flags&PKT_FLAG_KEY;

	// TODO: We might avoid the copy as a whole by making EncodedVideoFrame
	//       virtual.
	size_t allocSize = packet.size + AV_INPUT_BUFFER_PADDING_SIZE;
	std::uint8_t* data = new std::uint8_t[allocSize];
	std::copy(packet.data, packet.data+packet.size, data);
	std::unique_ptr<EncodedVideoFrame> frame(new EncodedVideoFrame(data, packet.size, 0, timestamp));

	pushEncodedVideoFrame(std::move(frame));

	return true;
#endif
}

bool
MediaParserFfmpeg::parseAudioFrame(AVPacket& packet)
{
	assert(packet.stream_index == _audioStreamIndex);
	assert(_audioStream);

	// packet.dts is "decompression" timestamp
	// packet.pts is "presentation" timestamp
	// Dunno why we use dts, and don't understand the magic formula either...
	//
	// From ffmpeg dox:
	//    pkt->pts can be AV_NOPTS_VALUE if the video format has B frames,
	//    so it is better to rely on pkt->dts if you do not decompress the payload.
	//

	std::uint64_t dts = packet.dts;
    if ( dts == static_cast<std::uint64_t>(AV_NOPTS_VALUE) ) {
        // We'll take 'nopts' value as zero.
        // Would likely be better to make it use timestamp
        // of previous frame, if any.
        //
        // For now, this handling fixes warnings like:
        //   mdb:93, lastbuf:0 skiping granule 0
        //   mdb:93, lastbuf:0 skiping granule 0
        // When playing: http://downloads.bbc.co.uk/news/nol/shared/spl/hi/audio_slideshow/kenadamptw/slideshow_629.swf
        //
        LOG_ONCE(log_error(_("FIXME: FFmpeg packet decompression "
                             "timestamp has no value, taking as zero")));
        dts = 0;
    }
	std::uint64_t timestamp = static_cast<std::uint64_t>(dts * as_double(_audioStream->time_base) * 1000.0);
    //log_debug("On getting audio frame with timestamp %d, duration is %d", timestamp, _audioStream->duration);

	std::unique_ptr<EncodedAudioFrame> frame ( new EncodedAudioFrame );

	// TODO: We might avoid the copy as a whole by making EncodedAudioFrame
	//       virtual.
	size_t allocSize = packet.size + AV_INPUT_BUFFER_PADDING_SIZE;
	std::uint8_t* data = new std::uint8_t[allocSize];
	std::copy(packet.data, packet.data+packet.size, data);

	frame->data.reset(data); 
	frame->dataSize = packet.size;
	frame->timestamp = timestamp;

	pushEncodedAudioFrame(std::move(frame)); 

	return true;
}

bool
MediaParserFfmpeg::parseNextFrame()
{
	// lock the stream while reading from it, so actionscript
	// won't mess with the parser on seek  or on getBytesLoaded
	std::lock_guard<std::mutex> streamLock(_streamMutex);

	if ( _parsingComplete )
	{
		//log_debug("MediaParserFfmpeg::parseNextFrame: parsing "
        //"complete, nothing to do");
		return false;
	}

	// position the stream where we left parsing as
	// it could be somewhere else for reading a specific
	// or seeking.
	//_stream->seek(_lastParsedPosition);

	assert(_formatCtx);

  	AVPacket packet;

	//log_debug("av_read_frame call");
  	int rc = av_read_frame(_formatCtx, &packet);

	// Update _lastParsedPosition, even in case of error..
	std::uint64_t curPos = _stream->tell();
	if ( curPos > _lastParsedPosition )
	{
		_lastParsedPosition = curPos;
	}

	//log_debug("av_read_frame returned %d", rc);
	if ( rc < 0 )
	{
        log_error(_("MediaParserFfmpeg::parseNextFrame: "
            "Problems parsing next frame "
            "(av_read_frame returned %d)."
            " We'll consider the stream fully parsed."), rc);
        _parsingComplete=true; // No point in parsing over
        return false;
	}

	bool ret = false;

	if ( packet.stream_index == _videoStreamIndex )
	{
		ret = parseVideoFrame(packet);
	}
	else if ( packet.stream_index == _audioStreamIndex )
	{
		ret = parseAudioFrame(packet);
	}
	else
	{
		ret = false; // redundant..
		log_debug("MediaParserFfmpeg::parseNextFrame: unknown stream index %d",
                packet.stream_index);
	}

	av_packet_unref(&packet);

	// Check if EOF was reached
	if ( _stream->eof() )
	{
		log_debug("MediaParserFfmpeg::parseNextFrame: at eof after "
                "av_read_frame");
		_parsingComplete=true;
	}

	return ret;
}

bool
MediaParserFfmpeg::parseNextChunk()
{
	if (!parseNextFrame()) return false;
	return true;
}

std::uint64_t
MediaParserFfmpeg::getBytesLoaded() const
{
	return _lastParsedPosition;
}

MediaParserFfmpeg::MediaParserFfmpeg(std::unique_ptr<IOChannel> stream)
	:
	MediaParser(std::move(stream)),
	_inputFmt(nullptr),
	_formatCtx(nullptr),
	_videoStreamIndex(-1),
	_videoStream(nullptr),
	_audioStreamIndex(-1),
	_audioStream(nullptr),
        _avIOCxt(nullptr),
	_lastParsedPosition(0)
{
	initializeParser();

	startParserThread();
}

/*private*/
void
MediaParserFfmpeg::initializeParser()
{
  /* wyatt */
/*    av_register_all();*/ // TODO: needs to be invoked only once ?

    _inputFmt = probeStream();

#ifdef GNASH_ALLOW_VCODEC_ENV	
    if (!_inputFmt) {
        char* defcodec = getenv("GNASH_DEFAULT_VCODEC");
        if (defcodec && strlen(defcodec)) {
            _inputFmt = av_find_input_format(defcodec);	
        }
    }
#endif	
    if (! _inputFmt) {
        throw MediaException("MediaParserFfmpeg couldn't figure out input "
                     "format");
    }
    
    // Setup the filereader/seeker mechanism.
    // 7th argument (NULL) is the writer function,
    // which isn't needed.
    _byteIOBuffer.reset(new unsigned char[byteIOBufferSize]);

    _avIOCxt = avio_alloc_context(
		  _byteIOBuffer.get(), // buffer
		  byteIOBufferSize, // buffer size
		  0, // write flags
		  this, // opaque pointer to pass to the callbacks
		  MediaParserFfmpeg::readPacketWrapper, // packet reader callback
		  nullptr, // packet writer callback
		  MediaParserFfmpeg::seekMediaWrapper // seeker callback
		  );
    
    _avIOCxt->seekable = 0;

    _formatCtx = avformat_alloc_context();
    assert(_formatCtx);

    _formatCtx->pb = _avIOCxt;

    if (avformat_open_input(&_formatCtx, "", _inputFmt, nullptr) < 0)
    {
        throw IOException("MediaParserFfmpeg couldn't open input stream");
    }

    // Note: in at least some versions of ffmpeg, av_open_input_stream does
    // not parse metadata; not sure why.
    AVDictionary* md = _formatCtx->metadata;
    if (md) {
        AVDictionaryEntry* tag = av_dict_get(md, "album", nullptr,
                AV_DICT_MATCH_CASE);
        if (tag && tag->value) {
            setId3Info(&Id3Info::album, std::string(tag->value),
                    _id3Object);
        }
    }

    log_debug("Parsing FFMPEG media file: format:%s; nstreams:%d",
        _inputFmt->name, _formatCtx->nb_streams);
    
    // Find first audio and video stream
    for (size_t i = 0; i < static_cast<size_t>(_formatCtx->nb_streams); ++i) {

	    AVStream* stream = _formatCtx->streams[i];
	    if (! stream) {
            log_debug("Stream %d of FFMPEG media file is null ?", i);
            continue;
	    }
	    
// wyatt
        
/*	    AVCodecContext* enc = stream->codec; */
	    AVCodecParameters* enc = stream->codecpar;
	    if (!enc) {
            log_debug("Stream %d of FFMPEG media file has no codec info", i);
            continue;
	    }
	    
	    switch (enc->codec_type) {
            case AVMEDIA_TYPE_AUDIO:
                if (_audioStreamIndex < 0) {
                    _audioStreamIndex = i;
                    _audioStream = _formatCtx->streams[i];
                    // codec_name will only be filled by avcodec_find_decoder
                    // (later);
                    log_debug(_("  Using stream %d for audio: codec id %d"),
                          i, _audioStream->codecpar->codec_id);
                }
                break;
		
            case AVMEDIA_TYPE_VIDEO:
                if (_videoStreamIndex < 0) {
                    _videoStreamIndex = i;
                    _videoStream = _formatCtx->streams[i];
                    log_debug(_("  Using stream %d for video: codec id %d"),
                          i, _videoStream->codecpar->codec_id);
                }
                break;
            default:
                break;
	    }
	}
    
    // Create VideoInfo
    if ( _videoStream) {
        const int codec = static_cast<int>(_videoStream->codecpar->codec_id); 
        std::uint16_t width = _videoStream->codecpar->width;
        std::uint16_t height = _videoStream->codecpar->height;
        std::uint16_t frameRate = static_cast<std::uint16_t>(
                as_double(_videoStream->avg_frame_rate));
#if !defined(HAVE_LIBAVFORMAT_AVFORMAT_H) && !defined(HAVE_FFMPEG_AVCODEC_H)
        std::uint64_t duration = _videoStream->codec_info_duration;
#else
        std::uint64_t duration = _videoStream->duration;
#endif
        if (duration == static_cast<std::uint64_t>(AV_NOPTS_VALUE)) {
            log_error(_("Duration of video stream unknown"));
            duration=0; // TODO: guess!
        } else {
            duration = duration / as_double(_videoStream->time_base); // TODO: check this
        }
        /* wyatt */
        _videoInfo.reset(new VideoInfo(codec, width, height, frameRate,
                    duration, CODEC_TYPE_CUSTOM /*codec type*/));
	
        // NOTE: AVCodecContext.extradata : void* for 51.11.0, uint8_t* for 51.38.0
        _videoInfo->extra.reset(new ExtraVideoInfoFfmpeg(
                     (uint8_t*)_videoStream->codecpar->extradata,
                     _videoStream->codecpar->extradata_size));
	
    }
    
    // Create AudioInfo
    if (_audioStream) {

        const int codec = static_cast<int>(_audioStream->codecpar->codec_id); 
        std::uint16_t sampleRate = _audioStream->codecpar->sample_rate;
        std::uint16_t sampleSize = SampleFormatToSampleSize(static_cast<AVSampleFormat>(_audioStream->codecpar->format));
        bool stereo = (_audioStream->codecpar->channels == 2);
#if !defined(HAVE_LIBAVFORMAT_AVFORMAT_H) && !defined(HAVE_FFMPEG_AVCODEC_H)
        std::uint64_t duration = _audioStream->codec_info_duration;
#else
        std::uint64_t duration = _audioStream->duration;
#endif
        if (duration == static_cast<std::uint64_t>(AV_NOPTS_VALUE)) {
            log_error(_("Duration of audio stream unknown to ffmpeg"));
            duration=0; // TODO: guess!
        } 
        else {
            duration = duration / as_double(_audioStream->time_base); // TODO: check this
        }
	
        _audioInfo.reset(new AudioInfo(codec, sampleRate, sampleSize, stereo,
                    duration, CODEC_TYPE_CUSTOM /*codec type*/));
        
        // NOTE: AVCodecContext.extradata : void* for 51.11.0, uint8_t* for 51.38.0
        _audioInfo->extra.reset(new ExtraAudioInfoFfmpeg(
                     (uint8_t*)_audioStream->codecpar->extradata,
                     _audioStream->codecpar->extradata_size));
	
    }
    
    
}
    
MediaParserFfmpeg::~MediaParserFfmpeg()
{
	stopParserThread();


        avformat_close_input(&_formatCtx);

	if ( _inputFmt )
	{
		// TODO: check if this is correct (should we create RIIA classes for ffmpeg stuff?)
		//av_free(_inputFmt); // it seems this one blows up, could be due to av_free(_formatCtx) above
	}

        av_free(_avIOCxt);
}

// NOTE: as this function is used as a callback from FFMPEG, it should not
// throw any exceptions, because:
// a) The behaviour of C++ exceptions passed into C code is undefined.
// b) Even if we don't crash and burn, the FFMPEG parser is left in an
//    undefined state.
int 
MediaParserFfmpeg::readPacket(std::uint8_t* buf, int buf_size)
{
	//GNASH_REPORT_FUNCTION;
	//log_debug("readPacket(%d)", buf_size);

	size_t ret = _stream->read(static_cast<void*>(buf), buf_size);

	return ret;

}

boost::optional<Id3Info>
MediaParserFfmpeg::getId3Info() const
{
    return _id3Object;
}

// NOTE: as this function is used as a callback from FFMPEG, it should not
// throw any exceptions, because:
// a) The behaviour of C++ exceptions passed into C code is undefined.
// b) Even if we don't crash and burn, the FFMPEG parser is left in an
//    undefined state.
std::int64_t
MediaParserFfmpeg::seekMedia(std::int64_t offset, int whence)
{
	//GNASH_REPORT_FUNCTION;
	//log_debug("::seekMedia(%1%, %2%)", offset, whence);

	assert(_stream.get());

	if (whence == SEEK_SET)
	{	
		// Offset is absolute new position in the file
		if ( offset < 0 ) {
			boost::format fmt = boost::format(
				_("MediaParserFfmpeg couldn't parse input format: "
				"tried to seek at negative offset %1%."))
				% offset;
	   		log_error(fmt);
			return -1;
		} else {
			_stream->seek(offset);
		}
	}
	else if (whence == SEEK_CUR)
	{
		// New position is offset + old position
		_stream->seek(_stream->tell() + static_cast<std::streamoff>(offset));
	}
	else if (whence == SEEK_END)
	{
		// New position is offset + end of file
		LOG_ONCE(log_unimpl("MediaParserFfmpeg seek from end of file"));
		// This is (most likely) a streamed file, so we can't seek to the end!
		// Instead we seek to byteIOBufferSize bytes... seems to work fine...
		_stream->seek(byteIOBufferSize);

	}
	else
	{
		// ffmpeg uses whence=AVSEEK_SIZE and offset=0 to request
		// stream size !
		LOG_ONCE(log_unimpl("MediaParserFfmpeg: unsupported whence value %d", whence));
		return -1;
	}


	return _stream->tell(); 
}

std::uint16_t
MediaParserFfmpeg::SampleFormatToSampleSize(AVSampleFormat fmt)
{
        return av_get_bytes_per_sample(fmt);
}


} // gnash.media.ffmpeg namespace 
} // end of gnash::media namespace
} // end of gnash namespace

#undef PADDING_BYTES
#undef READ_CHUNKS 
