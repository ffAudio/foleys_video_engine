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

#pragma once

#if FOLEYS_USE_FFMPEG

#if JUCE_MAC
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wfloat-conversion"
#endif

#if JUCE_MSVC
#pragma comment (lib, "avformat.lib")
#pragma comment (lib, "avutil.lib")
#pragma comment (lib, "avcodec.lib")
#pragma comment (lib, "swscale.lib")
#pragma comment (lib, "swresample.lib")
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/pixdesc.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

#ifdef __cplusplus
}
#endif

#if JUCE_MAC
#pragma clang diagnostic pop
#endif

namespace foleys
{

class FFmpegVideoScaler
{
public:
    /** Creates a scaler object. It does nothing before you call setupScaler */
    FFmpegVideoScaler() = default;

    ~FFmpegVideoScaler ()
    {
        if (scalerContext)
            sws_freeContext (scalerContext);
    }

    /** Setup a scaler to scale video frames and to convert pixel formats */
    void setupScaler (const int in_width,  const int in_height,  const AVPixelFormat in_format,
                      const int out_width, const int out_height, const AVPixelFormat out_format)
    {
        if (scalerContext)
        {
            if (in_width == iWidth &&
                in_height == iHeight &&
                in_format == iFormat &&
                out_width == oWidth &&
                out_height == oHeight &&
                out_format == oFormat)
                return;

            sws_freeContext (scalerContext);
            scalerContext = nullptr;
        }

        iWidth = in_width;
        iHeight = in_height;
        iFormat = in_format;
        oWidth = out_width;
        oHeight = out_height;
        oFormat = out_format;

        const AVPixFmtDescriptor* in_descriptor = av_pix_fmt_desc_get (in_format);
        if (!in_descriptor)
        {
            FOLEYS_LOG ("No description for input pixel format");
            return;
        }

        const int in_bitsPerPixel = av_get_padded_bits_per_pixel (in_descriptor);
        for (int i=0; i < 4; ++i)
            inLinesizes [i] = i < in_descriptor->nb_components ? in_width * in_bitsPerPixel >> 3 : 0;

        const AVPixFmtDescriptor* out_descriptor = av_pix_fmt_desc_get (out_format);
        if (!out_descriptor)
        {
            FOLEYS_LOG ("No description for output pixel format");
            return;
        }

        const int out_bitsPerPixel = av_get_padded_bits_per_pixel (out_descriptor);
        for (int i=0; i < 4; ++i)
            outLinesizes [i] = i < out_descriptor->nb_components ? out_width * out_bitsPerPixel >> 3 : 0;

        /* create scaling context */
        scalerContext = sws_getContext (in_width,  in_height, in_format,
                                        out_width, out_height, out_format,
                                        SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!scalerContext)
        {
            FOLEYS_LOG ("Impossible to create scale context for the conversion");
        }
    }


    /** takes an AVFrame from ffmpeg and converts it to a JUCE Image. Image is in a format
     matching to the platform */
    void convertFrameToImage (juce::Image& image, const AVFrame* frame)
    {
        if (scalerContext)
        {
            juce::Image::BitmapData data (image, 0, 0,
                                          image.getWidth(),
                                          image.getHeight(),
                                          juce::Image::BitmapData::writeOnly);

            uint8_t* destination[4] = {data.data, nullptr, nullptr, nullptr};

            sws_scale (scalerContext,
                       frame->data,
                       frame->linesize,
                       0,
                       frame->height,
                       destination,
                       outLinesizes);
        }
    }


    /** Converts a JUCE Image into a ffmpeg AVFrame to be written into a video stream */
    void convertImageToFrame (AVFrame* frame, const juce::Image& image)
    {
        if (scalerContext) {
            juce::Image::BitmapData data (image, 0, 0,
                                          image.getWidth(),
                                          image.getHeight());

            uint8_t* source[4] = {data.data, nullptr, nullptr, nullptr};

            sws_scale (scalerContext,
                       source,
                       inLinesizes,
                       0,
                       image.getHeight(),
                       frame->data,
                       frame->linesize);
        }
    }

#if JUCE_ANDROID
  #if JUCE_BIG_ENDIAN
    static const AVPixelFormat juceInternalFormat = AV_PIX_FMT_0BGR;
  #else
    static const AVPixelFormat juceInternalFormat = AV_PIX_FMT_RGB0;
  #endif
#else
  #if JUCE_BIG_ENDIAN
    static const AVPixelFormat juceInternalFormat = AV_PIX_FMT_0RGB;
  #else
    static const AVPixelFormat juceInternalFormat = AV_PIX_FMT_BGR0;
  #endif
#endif


private:
    SwsContext*     scalerContext = nullptr;

    int             iWidth  = 0;
    int             iHeight = 0;
    AVPixelFormat   iFormat = AV_PIX_FMT_NONE;
    int             oWidth  = 0;
    int             oHeight = 0;
    AVPixelFormat   oFormat = AV_PIX_FMT_NONE;

    int             inLinesizes[4];
    int             outLinesizes[4];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FFmpegVideoScaler)
};


class FFmpegAudioConverter
{
public:
    FFmpegAudioConverter() = default;

    ~FFmpegAudioConverter()
    {
        if (context != nullptr)
            swr_free (&context);
    }

    void setupConverter (int64_t inChannelLayout,  AVSampleFormat inFormat,  int inSampleRate,
                         int64_t outChannelLayout, AVSampleFormat outFormat, int outSampleRate)
    {
        context = swr_alloc_set_opts (context,
                                      outChannelLayout,
                                      outFormat,
                                      outSampleRate,
                                      inChannelLayout,
                                      inFormat,
                                      inSampleRate,
                                      0,         // log_offset
                                      nullptr);  // log_ctx

    }
private:
    SwrContext* context = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FFmpegAudioConverter)
};

struct FFmpegFrame
{
    FFmpegFrame()
    {
        frame = av_frame_alloc();
    }

    ~FFmpegFrame()
    {
        av_frame_unref (frame);
        av_frame_free (&frame);
    }

    AVFrame* frame;
};

struct VideoStreamDescriptor
{
    FFmpegVideoScaler    scaler;
    int                  streamIndex = -1;
    AVCodecContext*      context = nullptr;
    VideoStreamSettings  settings;
    VideoFifo            videoBuffer { 30 };
};

struct AudioStreamDescriptor
{
    FFmpegAudioConverter converter;
    int                  streamIndex = -1;
    AVCodecContext*      context = nullptr;
    AudioStreamSettings  settings;
    AudioFifo            sampleBuffer;
    juce::HeapBlock<uint8_t> converterBuffer;
};


} // foleys

#endif // FOLEYS_USE_FFMPEG
