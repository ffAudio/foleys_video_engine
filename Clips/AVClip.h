
#pragma once

namespace foleys
{


class AVClip : public juce::PositionableAudioSource
{
public:
    AVClip() = default;
    virtual ~AVClip();

    /** returns the pixel size of the original media as a tuple.
        In some video files this can change at any frame. */
    virtual Size getOriginalSize() const = 0;

    /** Returns the length of the clip in seconds */
    virtual double getLengthInSeconds() const = 0;

    /** Return the frame count for a timestamp */
    virtual Timecode getFrameTimecodeForTime (double time) const = 0;

    /** Returns the frame for a certain timecode */
    virtual juce::Image getFrame (const Timecode) const = 0;

    /** Returns the frame for the current timecode */
    virtual juce::Image getCurrentFrame() const = 0;

    virtual Timecode getCurrentTimecode() const = 0;

    virtual double getCurrentTimeInSeconds() const = 0;

    virtual juce::Image getStillImage (double seconds, Size size) = 0;

    virtual bool hasVideo() const = 0;
    virtual bool hasAudio() const = 0;
    virtual bool hasSubtitle() const = 0;

    struct TimecodeListener
    {
        virtual ~TimecodeListener() = default;

        /** Listen to this callback to get notified, when the time code changes.
            This is most useful to redraw the display or encode the next frame */
        virtual void timecodeChanged (Timecode tc) = 0;
    };

    struct SubtitleListener
    {
        virtual ~SubtitleListener() = default;

        /** Listen to this callback to display a subtitle.
            A time until when this text shall be displayed can be supplied. */
        virtual void setSubtitle (const juce::String& text, Timecode until) = 0;
    };

    void addTimecodeListener (TimecodeListener* listener);
    void removeTimecodeListener (TimecodeListener* listener);

    void addSubtitleListener (SubtitleListener* listener);
    void removeSubtitleListener (SubtitleListener* listener);

    virtual juce::TimeSliceClient* getBackgroundJob();

protected:
    /** Subclasses can call this to notify displays, that the time code has changed, e.g. to display a new frame */
    void sendTimecode (Timecode newTimecode, juce::NotificationType nt);

    /** Subclasses can use this to send sub titles for display or text to speech */
    void sendSubtitle (const juce::String& text, Timecode until, juce::NotificationType nt);

private:
    juce::ListenerList<TimecodeListener> timecodeListeners;
    juce::ListenerList<SubtitleListener> subtitleListeners;

    JUCE_DECLARE_WEAK_REFERENCEABLE (AVClip)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AVClip)
};


}
