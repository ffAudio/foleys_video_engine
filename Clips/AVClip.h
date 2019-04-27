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

class VideoEngine;

class AVClip  : public juce::PositionableAudioSource
{
public:
    AVClip (VideoEngine& videoEngine);

    virtual ~AVClip() = default;

    /** returns the original media file to restore */
    virtual juce::File getMediaFile() const { return {}; }

    /** returns a string describing the clip. This could be the
        filename of the original media file */
    virtual juce::String getDescription() const = 0;

    /** returns the pixel size of the media as a tuple.
        In some video files this can change at any frame. */
    virtual Size getVideoSize() const = 0;

    /** Returns the length of the clip in seconds */
    virtual double getLengthInSeconds() const = 0;

    /** Return the frame count for a timestamp */
    virtual Timecode getFrameTimecodeForTime (double time) const = 0;

    /** Returns the frame for a certain timecode */
    virtual std::pair<int64_t, juce::Image> getFrame (double pts) const = 0;

    /** Checks, if a frame is available */
    virtual bool isFrameAvailable (double pts) const = 0;

    /** Returns the frame for the current timecode */
    virtual juce::Image getCurrentFrame() const = 0;

    virtual Timecode getCurrentTimecode() const = 0;

    virtual double getCurrentTimeInSeconds() const = 0;

    virtual juce::Image getStillImage (double seconds, Size size) = 0;

    virtual bool hasVideo() const = 0;
    virtual bool hasAudio() const = 0;
    virtual bool hasSubtitle() const = 0;

    /**
     This is the samplerate supplied from prepareToPlay and the sample rate
     this clip will produce audio and use as clock source. */
    virtual double getSampleRate() const = 0;

    virtual std::shared_ptr<AVClip> createCopy() = 0;

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

    VideoEngine* getVideoEngine() const;

protected:
    /** Subclasses can call this to notify displays, that the time code has changed, e.g. to display a new frame */
    void sendTimecode (Timecode newTimecode, juce::NotificationType nt);

    /** Subclasses can use this to send sub titles for display or text to speech */
    void sendSubtitle (const juce::String& text, Timecode until, juce::NotificationType nt);

    juce::WeakReference<VideoEngine> videoEngine;

private:

    juce::ListenerList<TimecodeListener> timecodeListeners;
    juce::ListenerList<SubtitleListener> subtitleListeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AVClip)
};


} // foleys
