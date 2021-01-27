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

#include <bitset>

namespace foleys
{

/** A 2-dimensional size for images and video streams */
struct Size final
{
    int width = 0;
    int height = 0;

    double getAspectRatio()
    {
        return (height > 0) ? width / double (height) : 1.33;
    }
};

enum class Zoom
{
    NoZoom = 0, /**< Don't zoom, keep pixel size */
    ZoomScale,  /**< Zoom to fill target, ignore aspect ratio */
    LetterBox,  /**< Zoom to show all pixels, may leave bars unpainted */
    Crop        /**< Zoom to fill all pixels, may crop some pixels from the original */
};

/** Defines the size and time settings for a VideoStream */
struct VideoStreamSettings final
{
    Size frameSize;
    int  defaultDuration = 1001;
    int  timebase        = 24000;
};

/** Defines the number of channels and time settings for an AudioStream */
struct AudioStreamSettings final
{
    int numChannels       = 2;
    int defaultNumSamples = 1024;
    int timebase          = 48000;
};

/** Convert a time in seconds in frame counts, using the time base and duration in VideoStreamSettings */
static inline int64_t convertTimecode (double pts, const VideoStreamSettings& settings)
{
    if (settings.defaultDuration == 0)
        return 0;

    return settings.defaultDuration * int64_t (pts * settings.timebase / settings.defaultDuration);
}

/** Convert a time in seconds into samples, using the sample rate in AudioStreamSettings */
static inline int64_t convertTimecode (double pts, const AudioStreamSettings& settings)
{
    return int64_t (pts * settings.timebase);
}

/** Convert a time in seconds into a formatted string */
static inline juce::String timecodeToString (double pts)
{
    int64_t intSeconds = int (pts);

    auto milliSeconds = int (1000.0 * (pts - intSeconds));
    auto days = int (intSeconds / (3600 * 24));
    intSeconds -= days * 3600 * 24;
    auto hours = int (intSeconds / 3600);
    intSeconds -= hours * 3600;
    auto minutes = int (intSeconds / 60);
    intSeconds -= minutes * 60;

    return days != 0 ? juce::String (days) + "d " : ""
    + juce::String (hours).paddedLeft ('0', 2) + ":"
    + juce::String (minutes).paddedLeft ('0', 2) + ":"
    + juce::String (intSeconds).paddedLeft ('0', 2) + "."
    + juce::String (milliSeconds).paddedLeft ('0', 3);
}

/** Used to select, which streams to read and which to ignore in AVReader instances */
class StreamTypes final
{
public:
    enum StreamType
    {
        Video,
        Audio,
        Subtitles,
        Data
    };

    StreamTypes (int arg)
    {
        types = StreamType(arg);
    }

    static StreamTypes video()
    {
        return { 1 << StreamType::Video };
    }

    static StreamTypes audio()
    {
        return { 1 << StreamType::Audio };
    }

    static StreamTypes subtitles()
    {
        return { 1 << StreamType::Subtitles };
    }

    static StreamTypes data()
    {
        return { 1 << StreamType::Data };
    }

    static StreamTypes all()
    {
        return
        {
            (1 << StreamType::Video)
            + (1 << StreamType::Audio)
            + (1 << StreamType::Subtitles)
            + (1 << StreamType::Data)
        };
    }

    bool test (StreamType t)
    {
        return types.test (size_t (t));
    }

private:
    std::bitset<4> types;
};

} // foleys
