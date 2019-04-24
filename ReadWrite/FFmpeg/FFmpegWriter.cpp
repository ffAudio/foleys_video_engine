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

#include "FFmpegHelpers.h"

namespace foleys
{

struct FFmpegWriter::Pimpl
{
    struct VideoStreamDescriptor
    {
        FFmpegVideoScaler    scaler;
        int                  streamIndex = -1;
        AVCodecContext*      context = nullptr;
        VideoStreamSettings  settings;
        VideoFifo            videoBuffer;
    };

    struct AudioStreamDescriptor
    {
        FFmpegAudioConverter converter;
        int                  streamIndex = -1;
        AVCodecContext*      context = nullptr;
        AudioStreamSettings  settings;
        AudioFifo            sampleBuffer;
    };

    Pimpl(FFmpegWriter& owner) : writer (owner)
    {
        openContainer (writer.mediaFile, writer.formatName);
    }

    ~Pimpl()
    {
        closeContainer();
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

        auto* stream = avformat_new_stream (formatContext, NULL);
        if (stream == nullptr)
        {
            FOLEYS_LOG ("Failed allocating video output stream");
            return -1;
        }

        stream->time_base = av_make_q (1, 24000);

        auto* context = avcodec_alloc_context3 (encoder);
        context->width = settings.frameSize.width;
        context->height = settings.frameSize.height;
        context->pix_fmt = AV_PIX_FMT_YUV420P;
        context->sample_aspect_ratio = av_make_q (1, 1);
        context->color_range = AVCOL_RANGE_JPEG;
        context->bit_rate  = 480000;
        context->gop_size  = 10;
        context->time_base = av_make_q (1, 24000);
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
        descriptor->streamIndex = formatContext->nb_streams - 1;
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

        auto* stream = avformat_new_stream (formatContext, NULL);
        if (stream == nullptr)
        {
            FOLEYS_LOG ("Failed allocating video output stream");
            return -1;
        }

        stream->time_base = av_make_q (1, juce::roundToInt (settings.timebase));
        auto* context = avcodec_alloc_context3 (encoder);
        context->sample_rate = settings.timebase;
        context->sample_fmt = AV_SAMPLE_FMT_FLTP;
        context->channel_layout = AV_CH_LAYOUT_STEREO;
        context->channels = av_get_channel_layout_nb_channels (AV_CH_LAYOUT_STEREO);
        context->bit_rate = 64000;
        context->frame_size = settings.defaultNumSamples;
        context->bits_per_raw_sample = 32;
        avcodec_parameters_from_context (stream->codecpar, context);

        int ret = avcodec_open2 (context, encoder, NULL);
        if (ret < 0) {
            FOLEYS_LOG ("Cannot open audio encoder: " << codec);
        }

        auto descriptor = std::make_unique<AudioStreamDescriptor>();
        descriptor->streamIndex = formatContext->nb_streams - 1;
        descriptor->context = context;
        descriptor->settings = settings;

        audioStreams.push_back (std::move (descriptor));
        return int (audioStreams.size() - 1);
    }

    void pushSamples (const juce::AudioBuffer<float>& input, int stream)
    {
        if (juce::isPositiveAndBelow (stream, audioStreams.size()) == false)
            return;

        auto& descriptor = audioStreams [stream];
        descriptor->sampleBuffer.pushSamples (input);

        if (multiThreaded == false)
            while (processStreams());
    }

    void pushImage (juce::int64 pos, juce::Image image, int stream)
    {
        if (juce::isPositiveAndBelow (stream, videoStreams.size()) == false)
            return;

        auto& descriptor = videoStreams [stream];
        descriptor->videoBuffer.pushVideoFrame (image, pos);

        if (multiThreaded == false)
            while (processStreams());
    }

    void encodeVideoFrame (VideoStreamDescriptor& descriptor, juce::Image& image, juce::int64 timestamp)
    {
        jassert (formatContext != nullptr);
        jassert (descriptor.context != nullptr);

        descriptor.scaler.setupScaler (image.getWidth(),
                                       image.getHeight(),
                                       AV_PIX_FMT_BGR0,
                                       descriptor.context->width,
                                       descriptor.context->height,
                                       descriptor.context->pix_fmt);

        auto* context = descriptor.context;
        auto* frame = av_frame_alloc ();
        frame->width = context->width;
        frame->height = context->height;
        frame->format = context->pix_fmt;
        frame->pts = timestamp;

        FOLEYS_LOG ("Start writing video frame, pts: " << timestamp);

        int ret = av_image_alloc(frame->data, frame->linesize,
                                 context->width,
                                 context->height,
                                 context->pix_fmt, 32);
        if (ret < 0)
        {
            FOLEYS_LOG ("Could not allocate raw picture buffer");
            av_free (&frame);
            return;
        }

        descriptor.scaler.convertImageToFrame (frame, image);
        encodeWriteFrame (descriptor.context, frame);
    }

    void encodeAudioFrame (AudioStreamDescriptor& descriptor, juce::AudioBuffer<float>& buffer, juce::int64 timestamp)
    {
        FOLEYS_LOG ("encodeAudioFrame: " << timestamp << " num: " << buffer.getNumSamples());
        jassert (descriptor.settings.numChannels == buffer.getNumChannels());
        jassert (descriptor.settings.defaultNumSamples >= buffer.getNumSamples());

        auto* frame = av_frame_alloc ();
        frame->nb_samples   = buffer.getNumSamples();
        frame->format       = AV_SAMPLE_FMT_FLTP;
        frame->channel_layout = AV_CH_LAYOUT_STEREO;
        frame->channels     = buffer.getNumChannels();
        frame->pts          = timestamp;
        FOLEYS_LOG ("Start writing audio frame, pts: " << timestamp);
        auto  bufferSize = av_samples_get_buffer_size (nullptr, frame->channels, frame->nb_samples, AVSampleFormat (frame->format), 0);
        auto* samples = static_cast<float*> (av_malloc (bufferSize));

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            juce::FloatVectorOperations::copy (samples + channel * frame->nb_samples,
                                               buffer.getReadPointer (channel),
                                               buffer.getNumSamples());

        avcodec_fill_audio_frame (frame,
                                  frame->channels,
                                  AVSampleFormat (frame->format),
                                  (const uint8_t*) (samples),
                                  bufferSize,
                                  0);

        encodeWriteFrame (descriptor.context, frame);
    }

    void finishWriting()
    {
        if (formatContext == nullptr)
            return;

        for (int idx=0; idx < formatContext->nb_streams; ++idx)
        {
            auto descriptor = std::find_if (videoStreams.begin(), videoStreams.end(), [idx](const auto& descriptor) { return descriptor->streamIndex == idx; });
            if (descriptor != videoStreams.end())
            {
                if ((*descriptor)->context->codec->capabilities & AV_CODEC_CAP_DELAY)
                {
                    FOLEYS_LOG ("Flushing encoder dor stream " << idx);
                    while (encodeWriteFrame ((*descriptor)->context, nullptr));
                }
            }
            else
            {
                auto descriptor = std::find_if (audioStreams.begin(), audioStreams.end(), [idx](const auto& descriptor) { return descriptor->streamIndex == idx; });
                if (descriptor != audioStreams.end())
                {
                    if ((*descriptor)->context->codec->capabilities & AV_CODEC_CAP_DELAY)
                    {
                        FOLEYS_LOG ("Flushing encoder dor stream " << idx);
                        while (encodeWriteFrame ((*descriptor)->context, nullptr));
                    }
                }
            }
        }

        av_write_trailer (formatContext);
        closeContainer();
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
        int  next = -1;
        bool video = false;
        auto pts = std::numeric_limits<double>::max();

        for (int s=0; s < audioStreams.size(); ++s)
        {
            auto& descriptor = audioStreams [s];
            auto  streamPTS  = double (descriptor->sampleBuffer.getReadPosition()) / descriptor->settings.timebase;
            if (descriptor->sampleBuffer.getAvailableSamples() >= descriptor->settings.defaultNumSamples
                && streamPTS < pts)
            {
                next = s;
                pts = streamPTS;
            }
        }

        for (int s=0; s < videoStreams.size(); ++s)
        {
            auto& descriptor = videoStreams [s];
            auto  streamPTS  = double (descriptor->videoBuffer.getLowestTimeCode()) / descriptor->settings.timebase;
            if (descriptor->videoBuffer.getNumAvailableFrames() > 0 && streamPTS < pts)
            {
                next = s;
                pts = streamPTS;
                video = true;
            }
        }

        if (next >= 0)
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
            avformat_alloc_output_context2 (&formatContext, NULL, NULL, file.getFullPathName().toRawUTF8());
        else
            avformat_alloc_output_context2 (&formatContext, NULL, format.toRawUTF8(), file.getFullPathName().toRawUTF8());

        if (formatContext == nullptr)
        {
            FOLEYS_LOG ("Could not open output with format " + format);
            return;
        }


        writer.opened = true;
    }

    void closeContainer()
    {
        writer.opened = false;
        writer.started = false;

        for (auto& descriptor : videoStreams)
            av_free (&descriptor->context);
        for (auto& descriptor : audioStreams)
            av_free (&descriptor->context);

        videoStreams.clear();
        audioStreams.clear();

        if (formatContext != nullptr)
        {
            avformat_free_context (formatContext);
            formatContext = nullptr;
        }
    }

    bool encodeWriteFrame (AVCodecContext* codecContext, AVFrame* frame)
    {
        jassert (formatContext != nullptr);

        AVPacket packet;
        packet.data = NULL;
        packet.size = 0;
        av_init_packet (&packet);

        avcodec_send_frame (codecContext, frame);

        av_frame_free (&frame);

        int ret = 0;
        while (ret >= 0)
        {
            ret = avcodec_receive_packet (codecContext, &packet);
            if (ret == AVERROR (EAGAIN) || ret == AVERROR_EOF)
            {
                FOLEYS_LOG ("avcodec_receive_packet: " << ret);
                av_packet_unref (&packet);
                return true;
            }
            else if (ret < 0)
            {
                FOLEYS_LOG ("Error during encoding: " << ret);
                av_packet_unref (&packet);
                return false;
            }
            ret = av_interleaved_write_frame (formatContext, &packet);
            if (ret < 0) {
                FOLEYS_LOG ("Error muxing packet");
                return false;
            }
        }
        return true;
    }


    //==============================================================================

    FFmpegWriter& writer;

    AVFormatContext* formatContext = nullptr;

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
    started = true;
    if (!opened)
        return;

    pimpl->pushSamples (input, stream);
}

void FFmpegWriter::pushImage (juce::int64 pos, juce::Image image, int stream)
{
    started = true;
    if (!opened)
        return;

    pimpl->pushImage (pos, image, stream);
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
