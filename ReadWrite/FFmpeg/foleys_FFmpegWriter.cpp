/*
 ==============================================================================

 Copyright (c) 2019, Foleys Finest Audio - Daniel Walz
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

#pragma once

#include "foleys_FFmpegHelpers.h"

namespace foleys
{

struct FFmpegWriter::Pimpl
{
    Pimpl(FFmpegWriter& owner) : writer (owner)
    {
        packet = av_packet_alloc();
        openContainer (writer.mediaFile, writer.formatName);
    }

    ~Pimpl()
    {
        closeContainer();
        av_packet_free (&packet);
    }

    int addVideoStream (const VideoStreamSettings& settings, AVCodecID codec)
    {
        if (formatContext == nullptr)
            return -1;

        if (codec == AV_CODEC_ID_PROBE)
            codec = av_guess_codec (formatContext->oformat,
                                    nullptr, writer.mediaFile.getFullPathName().toRawUTF8(),
                                    nullptr,
                                    AVMEDIA_TYPE_VIDEO);

        if (codec <= AV_CODEC_ID_NONE)
            return -1;

        auto* encoder = avcodec_find_encoder (codec);
        if (encoder == nullptr)
        {
            FOLEYS_LOG ("No encoder found for codec: " << codec);
            return -1;
        }

        auto* stream = avformat_new_stream (formatContext, nullptr);
        if (stream == nullptr)
        {
            FOLEYS_LOG ("Failed allocating video output stream");
            return -1;
        }

        stream->time_base = av_make_q (1, settings.timebase);

        auto* context = avcodec_alloc_context3 (encoder);
        context->width = settings.frameSize.width;
        context->height = settings.frameSize.height;
        context->pix_fmt = AV_PIX_FMT_YUV420P;
        context->sample_aspect_ratio = av_make_q (1, 1);
        context->color_range = AVCOL_RANGE_MPEG;
        context->bit_rate  = 480000;
        context->gop_size  = 16;
        context->time_base = av_make_q (1, settings.timebase);

        if (encoder->id == AV_CODEC_ID_H264)
            context->ticks_per_frame = 2;

        avcodec_parameters_from_context (stream->codecpar, context);

        AVDictionary* options = nullptr;

        if (encoder->id == AV_CODEC_ID_H264) {
            av_dict_set (&options, "preset", "slow", 0);
            av_dict_set (&options, "tune", "film", 0);
            av_opt_set (context->priv_data, "profile", "baseline", AV_OPT_SEARCH_CHILDREN);
        }

        int ret = avcodec_open2 (context, encoder, &options);
        if (ret < 0) {
            DBG ("Cannot open video encoder: " << codec);
            return -1;
        }
        av_dict_free (&options);

        auto descriptor = std::make_unique<VideoStreamDescriptor>();
        descriptor->streamIndex = int (formatContext->nb_streams - 1);
        descriptor->context = context;
        descriptor->settings = settings;

        videoStreams.push_back (std::move (descriptor));
        return int (videoStreams.size() - 1);
    }

    int addAudioStream (const AudioStreamSettings& settings, AVCodecID codec)
    {
        if (formatContext == nullptr)
            return -1;

        if (codec == AV_CODEC_ID_PROBE)
            codec = av_guess_codec (formatContext->oformat,
                                    nullptr, writer.mediaFile.getFullPathName().toRawUTF8(),
                                    nullptr,
                                    AVMEDIA_TYPE_AUDIO);

        auto* encoder = avcodec_find_encoder (codec);
        if (encoder == nullptr)
        {
            FOLEYS_LOG ("No encoder found for codec: " << codec);
            return -1;
        }

        auto* stream = avformat_new_stream (formatContext, nullptr);
        if (stream == nullptr)
        {
            FOLEYS_LOG ("Failed allocating video output stream");
            return -1;
        }

        auto channelLayout = uint64_t (AV_CH_LAYOUT_STEREO);

        stream->time_base = av_make_q (1, settings.timebase);
        auto* context = avcodec_alloc_context3 (encoder);
        context->sample_rate = settings.timebase;
        context->sample_fmt = AV_SAMPLE_FMT_FLTP;
        context->channel_layout = channelLayout;
        context->channels = av_get_channel_layout_nb_channels (channelLayout);
        context->bit_rate = 64000;
        context->frame_size = settings.defaultNumSamples;
        context->bits_per_raw_sample = 32;
        context->time_base = av_make_q (1, settings.timebase);
        avcodec_parameters_from_context (stream->codecpar, context);

        int ret = avcodec_open2 (context, encoder, nullptr);
        if (ret < 0) {
            FOLEYS_LOG ("Cannot open audio encoder: " << codec);
        }

        auto descriptor = std::make_unique<AudioStreamDescriptor>();
        descriptor->streamIndex = int (formatContext->nb_streams - 1);
        descriptor->context = context;
        descriptor->settings = settings;

        audioStreams.push_back (std::move (descriptor));
        return int (audioStreams.size() - 1);
    }

    void pushSamples (const juce::AudioBuffer<float>& input, int stream)
    {
        if (juce::isPositiveAndBelow (stream, audioStreams.size()) == false)
            return;

        auto& descriptor = audioStreams [size_t (stream)];
        descriptor->sampleBuffer.pushSamples (input);

        if (multiThreaded == false)
            while (processStreams());
    }

    void pushImage (int64_t pos, juce::Image image, int stream)
    {
        if (juce::isPositiveAndBelow (stream, videoStreams.size()) == false)
            return;

        auto& descriptor = videoStreams [size_t (stream)];
        descriptor->videoBuffer.pushVideoFrame (image, pos);

        if (multiThreaded == false)
            while (processStreams());
    }

    void encodeVideoFrame (VideoStreamDescriptor& descriptor, juce::Image& image, int64_t timestamp)
    {
        FOLEYS_LOG ("encodeVideoFrame: " << timestamp << " size: " << image.getWidth() << "x" << image.getHeight());

        jassert (formatContext != nullptr);
        jassert (descriptor.context != nullptr);

//        {
//            auto folder = juce::File::getSpecialLocation (juce::File::userDesktopDirectory).getChildFile ("debug");
//            juce::JPEGImageFormat format;
//            format.setQuality (0.8);
//            juce::FileOutputStream stream (folder.getChildFile ("frame" + juce::String (timestamp).paddedLeft ('0', 8) + ".jpg"));
//            format.writeImageToStream (image, stream);
//        }

        descriptor.scaler.setupScaler (image.getWidth(),
                                       image.getHeight(),
                                       AV_PIX_FMT_BGR0,
                                       descriptor.context->width,
                                       descriptor.context->height,
                                       descriptor.context->pix_fmt);

        auto* context = descriptor.context;

        FFmpegFrame frame;

        frame.frame->width = context->width;
        frame.frame->height = context->height;
        frame.frame->format = context->pix_fmt;
        frame.frame->pts = timestamp;

        FOLEYS_LOG ("Start writing video frame, pts: " << timestamp);

        auto ret = av_frame_get_buffer (frame.frame, 1);
        if (ret < 0)
        {
            FOLEYS_LOG ("Cannot allocate buffers for video frame: " << juce::String (ret));
        }

//        auto ret = av_frame_make_writable (frame.frame);
//        if (ret < 0)
//        {
//            FOLEYS_LOG ("Error making video frame writeable: " << juce::String (ret));
//            return;
//        }

        descriptor.scaler.convertImageToFrame (frame.frame, image);
        encodeWriteFrame (descriptor.context, frame.frame, descriptor.streamIndex);
    }

    void encodeAudioFrame (AudioStreamDescriptor& descriptor, juce::AudioBuffer<float>& buffer, int64_t timestamp)
    {
        FOLEYS_LOG ("encodeAudioFrame: " << timestamp << " num: " << buffer.getNumSamples());
        jassert (descriptor.settings.numChannels == buffer.getNumChannels());
        jassert (descriptor.settings.defaultNumSamples >= buffer.getNumSamples());

        FFmpegFrame frame;

        frame.frame->nb_samples   = buffer.getNumSamples();
        frame.frame->format       = AV_SAMPLE_FMT_FLTP;
        frame.frame->channel_layout = AV_CH_LAYOUT_STEREO;
        frame.frame->channels     = av_get_channel_layout_nb_channels (frame.frame->channel_layout);
        frame.frame->pts          = timestamp;
        FOLEYS_LOG ("Start writing audio frame, pts: " << timestamp);

        auto  bufferSize = av_samples_get_buffer_size (nullptr, frame.frame->channels, frame.frame->nb_samples, AVSampleFormat (frame.frame->format), 0);
        if (descriptor.converterBuffer.getData() == nullptr)
            descriptor.converterBuffer.malloc (bufferSize);

        auto* samples = reinterpret_cast<float*> (descriptor.converterBuffer.getData());

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            juce::FloatVectorOperations::copy (samples + channel * frame.frame->nb_samples,
                                               buffer.getReadPointer (channel),
                                               buffer.getNumSamples());

        if (auto ret = av_frame_get_buffer (frame.frame, 1))
        {
            FOLEYS_LOG ("Cannot allocate buffers for audio frame: " << juce::String (ret));
        }

        if (auto ret = av_frame_make_writable (frame.frame))
        {
            FOLEYS_LOG ("Error making audio frame writeable: " << juce::String (ret));
            return;
        }

        avcodec_fill_audio_frame (frame.frame,
                                  frame.frame->channels,
                                  AVSampleFormat (frame.frame->format),
                                  descriptor.converterBuffer.getData(),
                                  bufferSize,
                                  0);

        encodeWriteFrame (descriptor.context, frame.frame, descriptor.streamIndex);
    }

    bool startWriting()
    {
        if (!(formatContext->oformat->flags & AVFMT_NOFILE)) {
            if (avio_open (&formatContext->pb, writeFile.getFullPathName().toRawUTF8(), AVIO_FLAG_WRITE) < 0) {
                FOLEYS_LOG ("Could not open output file '" << writeFile.getFullPathName() << "'");
                closeContainer();
                return false;
            }
        }

        auto ret = avformat_write_header (formatContext, nullptr);
        if (ret <0)
        {
            FOLEYS_LOG ("Error writing header");
            closeContainer();
            return false;
        }

        av_dump_format (formatContext, 0, nullptr, 1);

        writer.started = true;
        return true;
    }

    void finishWriting()
    {
        if (formatContext == nullptr)
            return;

        for (int idx=0; idx < int (formatContext->nb_streams); ++idx)
        {
            auto it = std::find_if (videoStreams.begin(), videoStreams.end(), [idx](const auto& descriptor) { return descriptor->streamIndex == idx; });
            if (it != videoStreams.end())
            {
                auto* context = (*it)->context;
                if (context->codec != nullptr && context->codec->capabilities & AV_CODEC_CAP_DELAY)
                {
                    FOLEYS_LOG ("Flushing video encoder stream " << idx);
                    while (encodeWriteFrame (context, nullptr, (*it)->streamIndex));
                }
            }
            else
            {
                auto it2 = std::find_if (audioStreams.begin(), audioStreams.end(), [idx](const auto& descriptor) { return descriptor->streamIndex == idx; });
                if (it2 != audioStreams.end())
                {
                    auto* context = (*it2)->context;
                    if (context->codec != nullptr && context->codec->capabilities & AV_CODEC_CAP_DELAY)
                    {
                        FOLEYS_LOG ("Flushing audio encoder stream " << idx);
                        while (encodeWriteFrame (context, nullptr, (*it2)->streamIndex));
                    }
                }
            }
        }

        if (av_write_trailer (formatContext) == 0)
        {
            videoStreams.clear();
            audioStreams.clear();
        }
        else
        {
            closeContainer();
        }
    }

    class WriteThread : public juce::TimeSliceClient
    {
    public:
        WriteThread (Pimpl& o) : owner (o) {}
        int useTimeSlice() override
        {
            if (owner.multiThreaded == false)
                return 1000;

            if (owner.processStreams())
                return 0;

            return 50;
        }

    private:
        Pimpl& owner;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WriteThread)
    };

private:

    bool processStreams()
    {
        // find oldest stream
        bool found = false;
        size_t next = 0;
        bool   video = false;
        auto   pts = std::numeric_limits<double>::max();

        for (size_t s=0; s < audioStreams.size() && !found; ++s)
        {
            auto& descriptor = audioStreams [s];
            auto  streamPTS  = double (descriptor->sampleBuffer.getReadPosition()) / descriptor->settings.timebase;
            if (descriptor->sampleBuffer.getAvailableSamples() >= descriptor->settings.defaultNumSamples
                && streamPTS < pts)
            {
                next = s;
                pts = streamPTS;
                found = true;
            }
        }

        for (size_t s=0; s < videoStreams.size() && !found; ++s)
        {
            auto& descriptor = videoStreams [s];
            auto  streamPTS  = double (descriptor->videoBuffer.getLowestTimeCode()) / descriptor->settings.timebase;
            if (descriptor->videoBuffer.getNumAvailableFrames() > 0 && streamPTS < pts)
            {
                next = s;
                pts = streamPTS;
                video = true;
                found = true;
            }
        }

        if (found)
        {
            if (video)
            {
                auto& stream = videoStreams [next];
                auto frame = stream->videoBuffer.popVideoFrame();
                encodeVideoFrame (*stream, frame.second, frame.first);
            }
            else
            {
                auto& stream = audioStreams [next];
                juce::AudioBuffer<float> buffer (stream->settings.numChannels, stream->settings.defaultNumSamples);
                juce::AudioSourceChannelInfo info (&buffer, 0, buffer.getNumSamples());
                stream->sampleBuffer.pullSamples (info);
                encodeAudioFrame (*stream, buffer, stream->sampleBuffer.getReadPosition());
            }
            return true;
        }

        return false;
    }

    void openContainer (juce::File file, juce::String format)
    {
        if (format.isEmpty())
            avformat_alloc_output_context2 (&formatContext, nullptr, nullptr, file.getFullPathName().toRawUTF8());
        else
            avformat_alloc_output_context2 (&formatContext, nullptr, format.toRawUTF8(), file.getFullPathName().toRawUTF8());

        if (formatContext == nullptr)
        {
            FOLEYS_LOG ("Could not open output with format " + format);
            return;
        }

        writeFile = file;
        writer.opened = true;
    }

    void closeContainer()
    {
        writer.opened = false;
        writer.started = false;

        if (formatContext != nullptr)
        {
            avformat_free_context (formatContext);
            formatContext = nullptr;
        }
    }

    bool encodeWriteFrame (AVCodecContext* codecContext, AVFrame* frame, int streamIndex)
    {
        jassert (formatContext != nullptr);

        avcodec_send_frame (codecContext, frame);

        auto ret = avcodec_receive_packet (codecContext, packet);
        packet->stream_index = streamIndex;
        av_packet_rescale_ts (packet, codecContext->time_base, formatContext->streams [streamIndex]->time_base);
        if (ret == AVERROR (EAGAIN) || ret == AVERROR_EOF)
        {
            av_packet_unref (packet);
            return false;
        }

        if (av_interleaved_write_frame (formatContext, packet) < 0)
            FOLEYS_LOG ("Error muxing packet");

        av_packet_unref (packet);

        return true;
    }


    //==============================================================================

    FFmpegWriter& writer;
    juce::File    writeFile;

    AVFormatContext* formatContext = nullptr;
    AVPacket*        packet        = nullptr;

    std::vector<std::unique_ptr<VideoStreamDescriptor>> videoStreams;
    std::vector<std::unique_ptr<AudioStreamDescriptor>> audioStreams;

    WriteThread writeThread { *this };
    bool multiThreaded = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pimpl)
};

//==============================================================================

FFmpegWriter::FFmpegWriter (juce::File file, juce::String format)
  : mediaFile (file),
    formatName (format)
{
    pimpl = std::make_unique<Pimpl> (*this);
}

juce::File FFmpegWriter::getMediaFile() const
{
    return mediaFile;
}

bool FFmpegWriter::isOpenedOk() const
{
    return opened;
}

void FFmpegWriter::pushSamples (const juce::AudioBuffer<float>& input, int stream)
{
    if (!opened)
        return;

    pimpl->pushSamples (input, stream);
}

void FFmpegWriter::pushImage (int64_t pos, juce::Image image, int stream)
{
    if (!opened)
        return;

    pimpl->pushImage (pos, image, stream);
}

bool FFmpegWriter::startWriting()
{
    return pimpl->startWriting();
}

void FFmpegWriter::finishWriting()
{
    pimpl->finishWriting();
}

int FFmpegWriter::addVideoStream (const VideoStreamSettings& settings)
{
    // You should have set up the streams before you started sending frames or samples
    jassert (started == false);

    if (opened == false || started == true)
        return -1;

    return pimpl->addVideoStream (settings, AV_CODEC_ID_PROBE);
}

int FFmpegWriter::addAudioStream (const AudioStreamSettings& settings)
{
    // You should have set up the streams before you started sending frames or samples
    jassert (started == false);

    if (opened == false || started == true)
        return -1;

    return pimpl->addAudioStream (settings, AV_CODEC_ID_PROBE);
}

juce::StringArray FFmpegWriter::getMuxers()
{
    return {
        NEEDS_TRANS ("MP4"),
        NEEDS_TRANS ("AVI"),
        NEEDS_TRANS ("MOV"),
        NEEDS_TRANS ("WMV")
    };
}

juce::StringArray FFmpegWriter::getPixelFormats()
{
    return {
        NEEDS_TRANS ("RGB"),
        NEEDS_TRANS ("RGBA"),
        NEEDS_TRANS ("YUV420P")
    };
}


} // foleys
