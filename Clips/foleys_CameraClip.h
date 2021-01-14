/*
 ==============================================================================

 Copyright (c) 2020, Foleys Finest Audio - Daniel Walz
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

#if FOLEYS_CAMERA_SUPPORT

namespace foleys
{

class CameraClip : public AVClip
{
public:
    CameraClip (VideoEngine& videoEngine, std::unique_ptr<CameraReceiver> source);

    /** Used to identify the clip type to the user */
    juce::String getClipType() const override { return "Camera"; }

    /** returns the name of the connected camera */
    juce::String getDescription() const override;

    /** returns the pixel size of the camera. */
    Size getVideoSize() const override;

    /** Returns the length of the clip in seconds */
    double getLengthInSeconds() const override;

    /** Returns the frame for a certain timecode */
    VideoFrame& getFrame (double pts) override;

#if FOLEYS_USE_OPENGL
    void render (juce::OpenGLContext& context, double pts) override;
#endif

    /** Checks, if a frame is available */
    bool isFrameAvailable (double pts) const override;

    /** This returns a still frame on the selected position. Don't use
        this method for streaming a video, because it will be slow */
    juce::Image getStillImage (double seconds, Size size) override;

    /** Returns true, if this clip will produce visual frames */
    bool hasVideo() const override { return true; }
    /** Returns true, if this clip will produce audio */
    bool hasAudio() const override { return false; }

    /**
     This is the samplerate supplied from prepareToPlay and the sample rate
     this clip will produce audio and use as clock source. */
    double getSampleRate() const override;

    /** Return the clip's read position in seconds */
    double getCurrentTimeInSeconds() const override;

    /** Set the read position in samples */
    void setNextReadPosition (juce::int64 newPosition) override;

    /** Returns the position from which the next block will be returned.

     @see setNextReadPosition
     */
    juce::int64 getNextReadPosition() const override;

    /** Returns the total length of the stream (in samples). */
    juce::int64 getTotalLength() const override;

    /** Returns true if this source is actually playing in a loop. */
    bool isLooping() const override { return false; }

    void prepareToPlay (int samplesPerBlockExpected,
                        double sampleRate) override;

    void releaseResources() override;

    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;

    std::shared_ptr<AVClip> createCopy (StreamTypes types) override;

private:
    std::unique_ptr<CameraReceiver> source;
    double sampleRate = 0.0;
    juce::int64 position = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CameraClip)
};

} // namespace foleys

#endif // FOLEYS_CAMERA_SUPPORT
