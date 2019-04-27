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


namespace foleys
{

struct Size
{
    int width = 0;
    int height = 0;

    double getAspectRatio()
    {
        return (height > 0) ? width / double (height) : 1.33;
    }
};

struct VideoStreamSettings
{
    Size frameSize;
    int  defaultDuration = 1001;
    int  timebase        = 24000;
};

struct AudioStreamSettings
{
    int numChannels       = 2;
    int defaultNumSamples = 1024;
    int timebase          = 48000;
};

struct Timecode
{
    /** The time code count. This is not necessarily incrementing in single steps, since
     for finer resolution the time base can be set different from the frame rate. */
    juce::int64 count = -1;

    /** The time base for audio is usually set to the sample rate and for video e.g. 1000
     to count in milli seconds. */
    double timebase = 1;

    /** Check if a timecode is set. A timecode is invalid, if no time base was set. */
    bool isValid() const { return timebase > 0 && count >= 0; }

    bool operator==(const Timecode& other)
    {
        return count == other.count && timebase == other.timebase;
    }

    bool operator!=(const Timecode& other)
    {
        return count != other.count || timebase != other.timebase;
    }
};

static inline int64_t convertTimecode (double pts, const VideoStreamSettings& settings)
{
    if (settings.defaultDuration == 0)
        return 0;

    return settings.defaultDuration * int64_t (pts * settings.timebase / settings.defaultDuration);
}

static inline int64_t convertTimecode (double pts, const AudioStreamSettings& settings)
{
    return int64_t (pts * settings.timebase);
}

static inline juce::String timecodeToString (Timecode tc)
{
    if (tc.timebase == 0)
        return "--:--:--.---";

    auto seconds = tc.count / double (tc.timebase);
    juce::int64 intSeconds = seconds;

    auto milliSeconds = int (1000.0 * (seconds - intSeconds));
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


class StreamTypes
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
        types = arg;
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
