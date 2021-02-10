/*
 ==============================================================================

 Copyright (c) 2019 - 2021, Foleys Finest Audio - Daniel Walz
 All rights reserved.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 OF THE POSSIBILITY OF SUCH DAMAGE.

 ==============================================================================
 */

#if FOLEYS_USE_FFMPEG

#include "foleys_FFmpegHelpers.h"


namespace foleys
{


class FFmpegReader::Pimpl
{
public:
    Pimpl (FFmpegReader& readerToUse, juce::File file, StreamTypes type)  : reader (readerToUse)
    {
        frame = av_frame_alloc();

        auto ret = avformat_open_input (&formatContext, file.getFullPathName().toRawUTF8(), nullptr, nullptr);
        if (ret < 0)
        {
            FOLEYS_LOG ("Opening file failed: " << getErrorString (ret));
            return;
        }

        // retrieve stream information
        if (avformat_find_stream_info (formatContext, nullptr) < 0)
        {
            closeVideoFile();
            return;
        }

        // open the streams
        numVideoStreams    = 0;
        numAudioStreams    = 0;
        numSubtitleStreams = 0;
        for (unsigned int i = 0; i < formatContext->nb_streams; ++i)
        {
            auto* stream = formatContext->streams [i];
            switch (stream->codecpar->codec_type)
            {
                case AVMEDIA_TYPE_VIDEO: ++numVideoStreams; break;
                case AVMEDIA_TYPE_AUDIO: ++numAudioStreams; break;
                case AVMEDIA_TYPE_SUBTITLE: ++numSubtitleStreams; break;
                case AVMEDIA_TYPE_NB:
                case AVMEDIA_TYPE_DATA:
                case AVMEDIA_TYPE_ATTACHMENT:
                case AVMEDIA_TYPE_UNKNOWN:
                default: break;
            }
        }

        if (type.test (StreamTypes::Audio))
            audioStreamIdx = openCodecContext (&audioContext, AVMEDIA_TYPE_AUDIO, true);

        if (juce::isPositiveAndBelow (audioStreamIdx, static_cast<int> (formatContext->nb_streams)))
        {
            auto* stream = formatContext->streams [audioStreamIdx];
            channelLayout = stream->codecpar->channel_layout;

            reader.sampleRate  = audioContext->sample_rate;
            reader.numChannels = audioContext->channels;
            reader.numSamples  = stream->duration > 0 ? stream->duration : std::numeric_limits<int64_t>::max();

            if (! setOutputSampleRate (audioContext->sample_rate))
            {
                FOLEYS_LOG ("Error initialising audio converter: " << getErrorString (ret));
                closeVideoFile();
                return;
            }

            FOLEYS_LOG ("Audio stream [" << audioStreamIdx << "]: timebase " << stream->time_base.den << "/" << stream->time_base.num);
        }

        if (type.test (StreamTypes::Video))
            videoStreamIdx = openCodecContext (&videoContext, AVMEDIA_TYPE_VIDEO, true);

        if (juce::isPositiveAndBelow (videoStreamIdx, static_cast<int> (formatContext->nb_streams)))
        {
            auto* stream = formatContext->streams [videoStreamIdx];
            reader.originalSize = { videoContext->width, videoContext->height };
            reader.pixelFormat  = videoContext->pix_fmt;
            reader.timebase     = stream->time_base.num > 0 ? double (stream->time_base.den) / stream->time_base.num : AV_TIME_BASE;

            scaler.setupScaler (videoContext->width,
                                videoContext->height,
                                videoContext->pix_fmt,
                                videoContext->width,
                                videoContext->height,
                                AV_PIX_FMT_BGR0);

            FOLEYS_LOG ("Video stream [" << videoStreamIdx << "]: timebase " << stream->time_base.den << "/" << stream->time_base.num);
        }

        // TODO subtitle and data stream

#if FOLEYS_DEBUG_LOGGING
        av_dump_format (formatContext, 0, file.getFullPathName().toRawUTF8(), 0);
#endif

        reader.opened = true;
    }

    ~Pimpl()
    {
        closeVideoFile();
        av_frame_free (&frame);
    }

    void closeVideoFile()
    {
        reader.opened = false;

        if (videoStreamIdx >= 0)
        {
            avcodec_free_context (&videoContext);
            videoStreamIdx = -1;
        }

        if (audioStreamIdx >= 0)
        {
            avcodec_free_context (&audioContext);
            audioStreamIdx = -1;
        }

        if (subtitleStreamIdx >= 0)
        {
            avcodec_free_context (&subtitleContext);
            subtitleStreamIdx = -1;
        }

        if (audioConverterContext)
        {
            swr_free(&audioConverterContext);
        }

        if (formatContext != nullptr)
            avformat_close_input (&formatContext);
    }

    void processPacket (VideoFifo& videoFifo, AudioFifo& audioFifo)
    {
        AVPacket packet;
        // initialize packet, set data to nullptr, let the demuxer fill it
        packet.data = nullptr;
        packet.size = 0;
        av_init_packet (&packet);

        auto error = av_read_frame (formatContext, &packet);

        if (error >= 0) {
            if (packet.stream_index == videoStreamIdx) {
                decodePacket (packet, videoFifo);
            }
            else if (packet.stream_index == audioStreamIdx) {
                decodePacket (packet, audioFifo);
            }
            else if (packet.stream_index == subtitleStreamIdx) {
                decodeSubtitlePacket (packet);
            }
            else {
                //DBG ("Packet is neither audio nor video... stream: " + String (packet.stream_index));
            }
        }
        av_packet_unref (&packet);
    }

    void setPosition (int64_t position)
    {
        FOLEYS_LOG ("Seek for sample position: " << position);
//        auto videoPts = av_rescale_q (position, audioContext->time_base, videoContext->time_base);
//        auto response = av_seek_frame (formatContext, videoStreamIdx, videoPts, AVSEEK_FLAG_BACKWARD);
        auto response = av_seek_frame (formatContext, audioStreamIdx, position, AVSEEK_FLAG_BACKWARD);
        if (response < 0)
        {
            FOLEYS_LOG ("Error seeking in audio stream: " << getErrorString (response));
        }
    }

    juce::Image getStillImage (double seconds, Size size)
    {
        scaler.setupScaler (videoContext->width,
                            videoContext->height,
                            videoContext->pix_fmt,
                            size.width,
                            size.height,
                            AV_PIX_FMT_BGR0);

        auto targetPts = int64_t (seconds * reader.timebase);
        auto response = av_seek_frame (formatContext, videoStreamIdx, targetPts, AVSEEK_FLAG_BACKWARD);
        if (response < 0)
        {
            FOLEYS_LOG ("Error seeking in video stream: " << getErrorString (response));
        }

        AVPacket* packet = av_packet_alloc();
        av_init_packet (packet);

        while (true)
        {
            av_read_frame (formatContext, packet);
            if (packet->stream_index == videoStreamIdx)
            {
                response = avcodec_send_packet (videoContext, packet);
                if (response < 0)
                {
                    FOLEYS_LOG ("Error reading packet for still image: " << getErrorString (response));
                    break;
                }
                response = avcodec_receive_frame(videoContext, frame);
                if (response < 0)
                {
                    FOLEYS_LOG ("Error reading frame for still image: " << getErrorString (response));
                }

                if (frame->best_effort_timestamp + frame->pkt_duration > targetPts)
                    break;
            }
        }
        FOLEYS_LOG ("Still PTS: " << frame->best_effort_timestamp << " vs. " << targetPts);
        juce::Image image (juce::Image::ARGB, size.width, size.height, false);
        scaler.convertFrameToImage (image, frame);
        av_packet_unref (packet);

        return image;
    }

    bool setOutputSampleRate (double sr)
    {
        outputSampleRate = sr;

        if (audioContext == nullptr)
        {
            if (juce::isPositiveAndBelow (videoStreamIdx, formatContext->nb_streams))
                reader.numSamples  = int (formatContext->streams [videoStreamIdx]->duration * sr
                * av_q2d (formatContext->streams [videoStreamIdx]->time_base));

            return false;
        }

        audioConverterContext = swr_alloc_set_opts (audioConverterContext,
                                                    int64_t (channelLayout),    // out_ch_layout
                                                    AV_SAMPLE_FMT_FLTP,         // out_sample_fmt
                                                    juce::roundToInt (sr),      // out_sample_rate
                                                    int64_t (channelLayout),    // in_ch_layout
                                                    audioContext->sample_fmt,   // in_sample_fmt
                                                    audioContext->sample_rate,  // in_sample_rate
                                                    0,                          // log_offset
                                                    nullptr);                   // log_ctx

        return swr_init (audioConverterContext) >= 0;
    }

    double getLengthInSeconds() const
    {
        AVStream* stream = nullptr;

        if (audioStreamIdx >= 0)
            stream = formatContext->streams [audioStreamIdx];
        else if (videoStreamIdx >= 0)
            stream = formatContext->streams [videoStreamIdx];

        if (stream)
            return stream->duration * av_q2d (stream->time_base);

        return 0.0;
    }

    juce::int64 getTotalLength() const
    {
        if (outputSampleRate > 0.0)
            return juce::int64 (getLengthInSeconds() * outputSampleRate);

        if (audioStreamIdx >= 0)
            return formatContext->streams [audioStreamIdx]->duration;

        return 0;
    }

    bool hasVideo() const
    {
        return videoStreamIdx >= 0;
    }

    bool hasAudio() const
    {
        return audioStreamIdx >= 0;
    }

    bool hasSubtitle() const
    {
        return subtitleStreamIdx >= 0;
    }

    VideoStreamSettings getVideoSettings (int streamIndex) const
    {
        // multiple video streams not yet implemented
        juce::ignoreUnused (streamIndex);

        foleys::VideoStreamSettings settings;
        settings.frameSize = { videoContext->width, videoContext->height };

        if (juce::isPositiveAndBelow (videoStreamIdx, static_cast<int> (formatContext->nb_streams)))
        {
            auto* stream = formatContext->streams [videoStreamIdx];
            settings.timebase = int (stream->time_base.num > 0 ? double (stream->time_base.den) / stream->time_base.num : AV_TIME_BASE);
            settings.defaultDuration = stream->time_base.den == stream->avg_frame_rate.num ? stream->avg_frame_rate.den : int (double (stream->avg_frame_rate.den * stream->time_base.den) / stream->avg_frame_rate.num);
        }

        return settings;
    }

    AudioStreamSettings getAudioSettings (int streamIndex) const
    {
        // multiple audio streams not yet implemented
        juce::ignoreUnused (streamIndex);

        foleys::AudioStreamSettings settings;
        settings.numChannels = audioContext->channels;
        settings.timebase = audioContext->sample_rate;
        settings.defaultNumSamples = int (audioContext->max_samples);
        return settings;
    }

    int numVideoStreams    =  0;
    int videoStreamIdx     = -1;
    int numAudioStreams    =  0;
    int audioStreamIdx     = -1;
    int numSubtitleStreams =  0;
    int subtitleStreamIdx  = -1;

private:

    int openCodecContext (AVCodecContext** decoderContext,
                          enum AVMediaType type,
                          bool refCounted)
    {
        AVCodec *decoder = nullptr;
        AVDictionary *opts = nullptr;

        int id = av_find_best_stream (formatContext, type, -1, -1, nullptr, 0);

        if (juce::isPositiveAndBelow(id, static_cast<int> (formatContext->nb_streams))) {
            AVStream* stream = formatContext->streams [id];
            // find decoder for the stream
            decoder = avcodec_find_decoder(stream->codecpar->codec_id);
            if (!decoder) {
                FOLEYS_LOG ("Failed to find " << juce::String (av_get_media_type_string(type)) << " codec");
                return -1;
            }
            // Allocate a codec context for the decoder
            *decoderContext = avcodec_alloc_context3 (decoder);
            if (!*decoderContext) {
                FOLEYS_LOG ("Failed to allocate the " << juce::String (av_get_media_type_string(type)) << " codec context");
                return -1;
            }
            // Copy codec parameters from input stream to output codec context
            if (avcodec_parameters_to_context (*decoderContext, stream->codecpar) < 0) {
                FOLEYS_LOG ("Failed to copy " + juce::String (av_get_media_type_string(type)) + " codec parameters to decoder context");
                return -1;
            }
            // Init the decoders, with or without reference counting
            av_dict_set (&opts, "refcounted_frames", refCounted ? "1" : "0", 0);
            if (avcodec_open2 (*decoderContext, decoder, &opts) < 0)
            {
                FOLEYS_LOG ("Failed to open " + juce::String (av_get_media_type_string(type)) + " codec");
                avcodec_free_context (decoderContext);
                return -1;
            }

            return id;
        }
        else
        {
            FOLEYS_LOG ("Could not find " << juce::String (av_get_media_type_string(type)) << " stream in input file");
            return -1;
        }
    }


    void decodePacket (AVPacket& packet, VideoFifo& videoFifo)
    {
        int response = avcodec_send_packet (videoContext, &packet);

        if (response < 0)
        {
            FOLEYS_LOG ("Error while sending video packet to the decoder: " << getErrorString (response));
            return;
        }

        while (response >= 0) {
            response = avcodec_receive_frame(videoContext, frame);
            if (response >= 0)
            {
                AVRational timeBase = av_make_q (1, AV_TIME_BASE);
                if (juce::isPositiveAndBelow (videoStreamIdx, static_cast<int> (formatContext->nb_streams)))
                {
                    timeBase = formatContext->streams [videoStreamIdx]->time_base;
                }

                auto& target = videoFifo.getWritingFrame();
                if (target.image.getWidth() != frame->width || target.image.getHeight() != frame->height)
                    target.image = juce::Image (juce::Image::ARGB, frame->width, frame->height, false);

                scaler.convertFrameToImage (target.image, frame);
                target.timecode = frame->best_effort_timestamp;
                videoFifo.finishWriting();

                FOLEYS_LOG ("Stream " << juce::String (packet.stream_index) <<
                     " (Video) " <<
                     " DTS: " << juce::String (packet.dts) <<
                     " PTS: " << juce::String (packet.pts) <<
                     " best effort PTS: " << juce::String (frame->best_effort_timestamp) <<
                     " in ms: " << juce::String (frame->best_effort_timestamp * av_q2d (timeBase) * 1000.0) <<
                     " timebase: " << juce::String (timeBase.num != 0 ? double (timeBase.den) / double (timeBase.num) : 0));
            }
        }
    }

    void decodePacket (AVPacket& packet, AudioFifo& audioFifo)
    {
        int response = avcodec_send_packet (audioContext, &packet);

        // decode audio frame
        while (response >= 0)
        {
            response = avcodec_receive_frame (audioContext, frame);
            if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
            {
                break;
            }
            else if (response < 0)
            {
                FOLEYS_LOG ("Error while sending audio packet to the decoder: " << getErrorString (response));
                break;
            }

            FOLEYS_LOG ("Stream " << juce::String (packet.stream_index) <<
                        " (Audio) " <<
                        " DTS: " << juce::String (packet.dts) <<
                        " PTS: " << juce::String (packet.pts) <<
                        " Frame PTS: " << juce::String (frame->best_effort_timestamp) <<
                        " in ms: " << juce::String (frame->best_effort_timestamp * 1000.0 / reader.sampleRate) <<
                        " timebase: " << reader.sampleRate);

            if (frame->extended_data != nullptr  && reader.sampleRate > 0)
            {
                const int  channels     = av_get_channel_layout_nb_channels (frame->channel_layout);
                const auto numSamples   = frame->nb_samples;
                const auto outTimestamp = int64_t (frame->best_effort_timestamp * outputSampleRate / reader.sampleRate);
                const auto numProduced  = int (numSamples * outputSampleRate / reader.sampleRate);

                jassert (std::abs (audioFifo.getWritePosition() - outTimestamp) < std::numeric_limits<int>::max());
                auto offset = int (audioFifo.getWritePosition() - outTimestamp);

                if (audioConvertBuffer.getNumChannels() != channels || audioConvertBuffer.getNumSamples() < numProduced)
                    audioConvertBuffer.setSize (channels, numProduced, false, false, true);

                if (outTimestamp < 0)
                    return;

                // FIXME: add a strategy to smooth back to zero
                offset = 0;

                swr_convert (audioConverterContext,
                             (uint8_t**)audioConvertBuffer.getArrayOfWritePointers(), numProduced,
                             (const uint8_t**)frame->extended_data, numSamples);
                juce::AudioBuffer<float> buffer (audioConvertBuffer.getArrayOfWritePointers(), channels, int (offset), int (numProduced - offset));
                audioFifo.pushSamples (buffer);
            }
        }
    }

    void decodeSubtitlePacket (AVPacket& packet)
    {
        int response = avcodec_send_packet (subtitleContext, &packet);
        if (response < 0)
        {
            FOLEYS_LOG ("Error while sending subtitle packet to the decoder: " << getErrorString (response));
        }
    }

    juce::String getErrorString (int code)
    {
        if(code < 0)
        {
            juce::MemoryBlock error (128);
            av_strerror (code, (char*)error.getData(), 128);
            return error.toString();
        }
        return {};
    }

    FFmpegReader& reader;

    AVFormatContext*  formatContext   = nullptr;
    AVCodecContext*   videoContext    = nullptr;
    AVCodecContext*   audioContext    = nullptr;
    AVCodecContext*   subtitleContext = nullptr;
    SwrContext*       audioConverterContext = nullptr;
    FFmpegVideoScaler scaler;

    AVFrame  *frame             = nullptr;

    uint64_t  channelLayout = AV_CH_LAYOUT_STEREO;
    double    outputSampleRate = {};

    juce::AudioBuffer<float>  audioConvertBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pimpl)
};

// ==============================================================================

FFmpegReader::FFmpegReader (const juce::File& file, StreamTypes type)
{
    mediaFile = file;
    pimpl = std::make_unique<Pimpl> (*this, file, type);
}

juce::File FFmpegReader::getMediaFile() const
{
    return mediaFile;
}

juce::int64 FFmpegReader::getTotalLength() const
{
    return pimpl->getTotalLength();
}

double FFmpegReader::getLengthInSeconds() const
{
    return pimpl->getLengthInSeconds();
}

void FFmpegReader::setPosition (const int64_t position)
{
    pimpl->setPosition (position);
}

juce::Image FFmpegReader::getStillImage (double seconds, Size size)
{
    return pimpl->getStillImage (seconds, size);
}

void FFmpegReader::readNewData (VideoFifo& videoFifo, AudioFifo& audioFifo)
{
    pimpl->processPacket (videoFifo, audioFifo);
}

void FFmpegReader::setOutputSampleRate (double sr)
{
    pimpl->setOutputSampleRate (sr);
}

bool FFmpegReader::hasVideo() const
{
    return pimpl->hasVideo();
}

bool FFmpegReader::hasAudio() const
{
    return pimpl->hasAudio();
}

bool FFmpegReader::hasSubtitle() const
{
    return pimpl->hasSubtitle();
}

int FFmpegReader::getNumVideoStreams() const
{
    return pimpl->numVideoStreams;
}

VideoStreamSettings FFmpegReader::getVideoSettings (int streamIndex) const
{
    juce::ignoreUnused (streamIndex);
    // Multiple streams opening not yet implemented
    jassert (streamIndex == 0);

    return pimpl->getVideoSettings (pimpl->videoStreamIdx);
}

int FFmpegReader::getNumAudioStreams() const
{
    return pimpl->numAudioStreams;
}

AudioStreamSettings FFmpegReader::getAudioSettings (int streamIndex) const
{
    juce::ignoreUnused (streamIndex);
    // Multiple streams opening not yet implemented
    jassert (streamIndex == 0);

    return pimpl->getAudioSettings (pimpl->audioStreamIdx);
}


} // foleys
#endif
