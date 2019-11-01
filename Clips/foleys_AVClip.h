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


/**
 @class AVClip

 AVClip is the abstract base class of displayable/playable clips. They can provide
 video frames an/or audio streams.

 To be compatible with juce's playback engines, they inherit PositionableAudioSource.
 Each AVClip is responsible to resample the audio to the sampleRate that is set
 in prepareToPlay. The audio stream acts as clock master.

 When you created a shared_ptr of an AVClip, call manageLifeTime() on the VideoEngine,
 that will add it to the auto release pool and register possible background jobs
 with the TimeSliceThreads.
 */
class AVClip  : public juce::PositionableAudioSource
{
public:
    AVClip (VideoEngine& videoEngine);

    virtual ~AVClip() = default;

    /** returns the original media file to restore */
    virtual juce::URL getMediaFile() const { return {}; }

    /** returns a string describing the clip. This could be the
        filename of the original media file */
    virtual juce::String getDescription() const = 0;

    /** returns the pixel size of the media as a tuple.
        In some video files this can change at any frame. */
    virtual Size getVideoSize() const = 0;

    /** Returns the length of the clip in seconds */
    virtual double getLengthInSeconds() const = 0;

    /** Returns the frame for a certain timecode */
    virtual std::pair<int64_t, juce::Image> getFrame (double pts) const = 0;

    /** Checks, if a frame is available */
    virtual bool isFrameAvailable (double pts) const = 0;

    /** Returns the frame for the current time */
    virtual juce::Image getCurrentFrame() const = 0;

    /** Return the clip's read position in seconds */
    virtual double getCurrentTimeInSeconds() const = 0;

    /** This returns a still frame on the selected position. Don't use
        this method for streaming a video, because it will be slow */
    virtual juce::Image getStillImage (double seconds, Size size) = 0;

    /** Returns true, if this clip will produce visual frames */
    virtual bool hasVideo() const = 0;
    /** Returns true, if this clip will produce audio */
    virtual bool hasAudio() const = 0;

    /**
     This is the samplerate supplied from prepareToPlay and the sample rate
     this clip will produce audio and use as clock source. */
    virtual double getSampleRate() const = 0;

    /** This returns a copy of the clip. Note that this will not work properly
        if the clip is not properly registered in the engine, because the
        copy will automatically be registered with the engine as well. */
    virtual std::shared_ptr<AVClip> createCopy (StreamTypes types) = 0;

    /** Use a TimecodeListener to be notified, when the visual frame changes */
    struct TimecodeListener
    {
        /** Destructor:w
         */
        virtual ~TimecodeListener() = default;

        /** Listen to this callback to get notified, when the time code changes.
            This is most useful to redraw the display or encode the next frame */
        virtual void timecodeChanged (int64_t count, double seconds) = 0;
    };

    /** Register a TimecodeListener to be notified, when the visual frame changes */
    void addTimecodeListener (TimecodeListener* listener);
    /** Unregister a TimecodeListener */
    void removeTimecodeListener (TimecodeListener* listener);

    /** When rendering non realtime (bounce), use this to wait for background
        threads to read ahead */
    virtual bool waitForDataReady (int samples)
    {
        juce::ignoreUnused (samples);
        return true;
    }

    /** @internal */
    virtual juce::TimeSliceClient* getBackgroundJob();

    /** @internal */
    VideoEngine* getVideoEngine() const;

    //==============================================================================

protected:

    /** Subclasses can call this to notify displays, that the time code has changed, e.g. to display a new frame */
    void sendTimecode (int64_t count, double seconds, juce::NotificationType nt);

    //==============================================================================

private:
    juce::WeakReference<VideoEngine> videoEngine;

    juce::ListenerList<TimecodeListener> timecodeListeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AVClip)
};


} // foleys
