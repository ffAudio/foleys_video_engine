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

namespace foleys
{

class VideoEngine;

#if FOLEYS_USE_OPENGL
class OpenGLView;
#endif

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
class AVClip  : public juce::PositionableAudioSource,
                public TimeCodeAware
{
public:
    AVClip (VideoEngine& videoEngine);

    virtual ~AVClip() = default;

    /** Used to identify the clip type to the user */
    virtual juce::String getClipType() const = 0;

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
    virtual VideoFrame& getFrame (double pts) = 0;

    /** This is the virtual render() method for OpenGL rendering */
    virtual void render (juce::Graphics& g, juce::Rectangle<float> area, double pts, float rotation = 0.0f, float zoom = 100.0f, juce::Point<float> translation = juce::Point<float>(), float alpha = 1.0f) = 0;

    /** Renders a frame on the OpenGLView. You can call this from the AVClip subclasses */
    void renderFrame (juce::Graphics& g, juce::Rectangle<float> area, VideoFrame& frame, float rotation, float zoom, juce::Point<float> translation, float alpha);

#if FOLEYS_USE_OPENGL
    /** This is the virtual render() method for OpenGL rendering */
    virtual void render (OpenGLView& view, double pts, float rotation = 0.0f, float zoom = 100.0f, juce::Point<float> translation = juce::Point<float>(), float alpha = 1.0f) = 0;

    /** Renders a frame on the OpenGLView. You can call this from the AVClip subclasses */
    void renderFrame (OpenGLView& view, VideoFrame& frame, float rotation, float zoom, juce::Point<float> translation, float alpha);
#endif

    /** Checks, if a frame is available */
    virtual bool isFrameAvailable (double pts) const = 0;

    /** This returns a still frame on the selected position. Don't use
        this method for streaming a video, because it will be slow */
    virtual juce::Image getStillImage (double seconds, Size size) = 0;

    /** Returns true, if this clip will produce visual frames */
    virtual bool hasVideo() const = 0;
    /** Returns true, if this clip will produce audio */
    virtual bool hasAudio() const = 0;

    virtual int getNumChannels() const = 0;

    /**
     This is the samplerate supplied from prepareToPlay and the sample rate
     this clip will produce audio and use as clock source. */
    virtual double getSampleRate() const = 0;

    void setAspectType (Aspect type);

    /**
     Returns the duration of a frame in seconds. This is the inverse of frame rate.
     */
    virtual double getFrameDurationInSeconds() const { return 0.0; }

    /** This returns a copy of the clip. Note that this will not work properly
        if the clip is not properly registered in the engine, because the
        copy will automatically be registered with the engine as well. */
    virtual std::shared_ptr<AVClip> createCopy (StreamTypes types) = 0;

    /** When rendering non realtime (bounce), use this to wait for background
        threads to read ahead */
    virtual bool waitForSamplesReady (int samples, int timeout=1000)
    {
        juce::ignoreUnused (samples);
        juce::ignoreUnused (timeout);
        return true;
    }

    /** When rendering non realtime (bounce), use this to wait for background
     threads to read ahead */
    virtual bool waitForFrameReady (double pts, int timeout=1000)
    {
        juce::ignoreUnused (pts);
        juce::ignoreUnused (timeout);
        return true;
    }

    static void addDefaultAudioParameters (AVClip& clip);
    static void addDefaultVideoParameters (AVClip& clip);

    const ParameterMap& getVideoParameters();
    const ParameterMap& getAudioParameters();

    /** @internal */
    virtual juce::TimeSliceClient* getBackgroundJob();

    /** @internal */
    VideoEngine* getVideoEngine() const;

    //==============================================================================

protected:

    void addAudioParameter (std::unique_ptr<ProcessorParameter> parameter);
    void addVideoParameter (std::unique_ptr<ProcessorParameter> parameter);

    //==============================================================================

private:
    juce::WeakReference<VideoEngine> videoEngine;

    Aspect zoomType = Aspect::LetterBox;

    ParameterMap videoParameters;
    ParameterMap audioParameters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AVClip)
};


} // foleys
