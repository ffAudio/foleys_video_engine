
#if FOLEYS_USE_FFMPEG

#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

#ifdef __cplusplus
}
#endif

namespace foleys
{

class FFmpegReader::Pimpl
{
public:
    Pimpl (FFmpegReader& readerToUse, juce::File file)  : reader (readerToUse)
    {
        openVideoFile (file);
    }

    ~Pimpl()
    {
        closeVideoFile();
    }

    bool openVideoFile (juce::File file)
    {
        auto ret = avformat_open_input (&formatContext, file.getFullPathName().toRawUTF8(), NULL, NULL);
        if (ret < 0)
        {
            DBG ("Opening file failed: " << ret);
            return false;
        }

        // retrieve stream information
        if (avformat_find_stream_info (formatContext, NULL) < 0)
        {
            closeVideoFile();
            return false;
        }

        // open the streams
        audioStreamIdx = openCodecContext (&audioContext, AVMEDIA_TYPE_AUDIO, true);

        if (juce::isPositiveAndBelow (audioStreamIdx, static_cast<int> (formatContext->nb_streams)))
        {
            auto channelLayout = formatContext->streams [audioStreamIdx]->codecpar->channel_layout;
            audioConverterContext = swr_alloc_set_opts(nullptr,
                                                       channelLayout,              // out_ch_layout
                                                       AV_SAMPLE_FMT_FLTP,         // out_sample_fmt
                                                       audioContext->sample_rate,  // out_sample_rate
                                                       channelLayout,              // in_ch_layout
                                                       audioContext->sample_fmt,   // in_sample_fmt
                                                       audioContext->sample_rate,  // in_sample_rate
                                                       0,                          // log_offset
                                                       nullptr);                   // log_ctx
            ret = swr_init (audioConverterContext);
            if(ret < 0)
            {
                DBG ("Error initialising audio converter: " << getErrorString (ret));
                closeVideoFile();
                return false;
            }
        }

        videoStreamIdx = openCodecContext (&videoContext, AVMEDIA_TYPE_VIDEO, true);
        if (juce::isPositiveAndBelow (videoStreamIdx, static_cast<int> (formatContext->nb_streams)))
        {
            reader.originalSize = { videoContext->width, videoContext->height };
            reader.pixelFormat  = videoContext->pix_fmt;
        }

        av_dump_format (formatContext, 0, file.getFullPathName().toRawUTF8(), 0);


        reader.opened = true;

        return true;
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


private:

    int openCodecContext (AVCodecContext** decoderContext,
                          enum AVMediaType type,
                          bool refCounted)
    {
        AVCodec *decoder = NULL;
        AVDictionary *opts = NULL;

        int id = av_find_best_stream (formatContext, type, -1, -1, NULL, 0);

        if (juce::isPositiveAndBelow(id, static_cast<int> (formatContext->nb_streams))) {
            AVStream* stream = formatContext->streams [id];
            // find decoder for the stream
            decoder = avcodec_find_decoder(stream->codecpar->codec_id);
            if (!decoder) {
                DBG ("Failed to find " + juce::String (av_get_media_type_string(type)) + " codec");
                return -1;
            }
            // Allocate a codec context for the decoder
            *decoderContext = avcodec_alloc_context3 (decoder);
            if (!*decoderContext) {
                DBG ("Failed to allocate the " + juce::String (av_get_media_type_string(type)) +
                     " codec context");
                return -1;
            }
            // Copy codec parameters from input stream to output codec context
            if (avcodec_parameters_to_context (*decoderContext, stream->codecpar) < 0) {
                DBG ("Failed to copy " + juce::String (av_get_media_type_string(type)) +
                     " codec parameters to decoder context");
                return -1;
            }
            // Init the decoders, with or without reference counting
            av_dict_set (&opts, "refcounted_frames", refCounted ? "1" : "0", 0);
            if (avcodec_open2 (*decoderContext, decoder, &opts) < 0) {
                DBG ("Failed to open " + juce::String (av_get_media_type_string(type)) + " codec");
                avcodec_free_context (decoderContext);
                return -1;
            }

            return id;
        }
        else {
            DBG ("Could not find " + juce::String (av_get_media_type_string(type)) +
                 " stream in input file");
            return -1;
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

    AVFormatContext* formatContext   = nullptr;
    AVCodecContext*  videoContext    = nullptr;
    AVCodecContext*  audioContext    = nullptr;
    AVCodecContext*  subtitleContext = nullptr;
    SwrContext*      audioConverterContext = nullptr;

    int width, height;
    enum AVPixelFormat pixelFormat  = AV_PIX_FMT_NONE;
    AVStream *videoStream       = nullptr;
    AVStream *audioStream       = nullptr;
    AVStream *subtitleStream    = nullptr;
    int       videoStreamIdx    = -1;
    int       audioStreamIdx    = -1;
    int       subtitleStreamIdx = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pimpl)
};

// ==============================================================================

FFmpegReader::FFmpegReader (juce::File file)
{
    pimpl = new Pimpl (*this, file);
}

FFmpegReader::~FFmpegReader()
{
    delete pimpl;
}







}
#endif
