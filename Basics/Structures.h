
#pragma once


namespace foleys
{

struct AVSize
{
    int width = 0;
    int height = 0;
};

struct AVTimecode
{
    /** The time code count. This is not necessarily incrementing in single steps, since
     for finer resolution the time base can be set different from the frame rate. */
    juce::int64 count = 0;

    /** The time base for audio is usually set to the sample rate and for video e.g. 1000
     to count in milli seconds. */
    int timebase = 0;

    /** Check if a timecode is set. A timecode is invalid, if no time base was set. */
    bool isValid() const { return timebase > 0; }
};

static inline juce::String timecodeToString (AVTimecode tc)
{
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
    + juce::String (hours).paddedLeft (2, '0') + ":"
    + juce::String (minutes).paddedLeft (2, '0') + ":"
    + juce::String (intSeconds).paddedLeft (2, '0') + "."
    + juce::String (milliSeconds).paddedLeft (3, '0');
}

}
