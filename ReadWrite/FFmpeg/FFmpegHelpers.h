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


#if FOLEYS_DEBUG_LOGGING
#  define FOLEYS_LOG(textToWrite)          JUCE_BLOCK_WITH_FORCED_SEMICOLON(juce::String tempDbgBuf; tempDbgBuf << "FFmpeg: " << textToWrite; juce::Logger::outputDebugString (tempDbgBuf);)
#else
#  define FOLEYS_LOG(textToWrite)
#endif

namespace foleys
{

class FFmpegVideoScaler
{
public:
    /** Creates a scaler object. It does nothing before you call setupScaler */
    FFmpegVideoScaler () : scalerContext (nullptr) {}

    ~FFmpegVideoScaler ()
    {
        if (scalerContext)
            sws_freeContext (scalerContext);
    }

    /** Setup a scaler to scale video frames and to convert pixel formats */
    void setupScaler (const int in_width,  const int in_height,  const AVPixelFormat in_format,
                      const int out_width, const int out_height, const AVPixelFormat out_format)
    {
        if (scalerContext) {
            sws_freeContext (scalerContext);
            scalerContext = nullptr;
        }

        const AVPixFmtDescriptor* in_descriptor = av_pix_fmt_desc_get (in_format);
        if (!in_descriptor) {
            FOLEYS_LOG ("No description for input pixel format");
            return;
        }
        const int in_bitsPerPixel = av_get_padded_bits_per_pixel (in_descriptor);
        for (int i=0; i < 4; ++i)
            inLinesizes [i] = i < in_descriptor->nb_components ? in_width * in_bitsPerPixel >> 3 : 0;

        const AVPixFmtDescriptor* out_descriptor = av_pix_fmt_desc_get (out_format);
        if (!out_descriptor) {
            FOLEYS_LOG ("No description for output pixel format");
            return;
        }
        const int out_bitsPerPixel = av_get_padded_bits_per_pixel (out_descriptor);
        for (int i=0; i < 4; ++i)
            outLinesizes [i] = i < out_descriptor->nb_components ? out_width * out_bitsPerPixel >> 3 : 0;

        /* create scaling context */
        scalerContext = sws_getContext (in_width,  in_height, in_format,
                                        out_width, out_height, out_format,
                                        SWS_BILINEAR, NULL, NULL, NULL);
        if (!scalerContext) {
            FOLEYS_LOG ("Impossible to create scale context for the conversion");
        }
    }


    /** takes an AVFrame from ffmpeg and converts it to a JUCE Image. Image is in a format
     matching to the platform */
    void convertFrameToImage (juce::Image& image, const AVFrame* frame)
    {
        if (scalerContext) {
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


private:
    SwsContext* scalerContext;

    int         inLinesizes[4];
    int         outLinesizes[4];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FFmpegVideoScaler)
};

} // foleys
