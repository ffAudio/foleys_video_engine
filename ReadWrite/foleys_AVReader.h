/*
 ==============================================================================

 Copyright (c) 2019-2021, Foleys Finest Audio - Daniel Walz
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

/**
 @class AVReader

 The AVReader allows reading of AVClip classes. This class is pure virtual to allow
 different implementations for various backends.
 */
class AVReader
{
public:
    AVReader (VideoEngine& engine) : videoEngine (engine) {}
    virtual ~AVReader() = default;

    virtual juce::File getMediaFile() const = 0;

    bool isOpenedOk() const { return opened; }

    /** Returns the length of the clip in seconds */
    virtual double getLengthInSeconds() const = 0;

    /**
     Returns the length in samples. Note before prepareToPlay this returns the number of samples in the audio stream.
     Since the Reader will resample to suit the output, this number can change.
     */
    virtual juce::int64 getTotalLength() const = 0;

    /**
     Seek the reader to a certain position. This position is given in audio samples
     */
    virtual void setPosition (const int64_t position) = 0;

    /** This method allows direct access to a specific time to render thumbnails.
        Don't use this to stream the video. Ideally use a separate reader for the
        thumbnails. */
    virtual juce::Image getStillImage (double seconds, Size size) = 0;

    virtual void readNewData (VideoFifo&, AudioFifo&) = 0;

    virtual bool hasVideo() const = 0;
    virtual bool hasAudio() const = 0;
    virtual bool hasSubtitle() const = 0;

    virtual void setOutputSampleRate (double sampleRate) = 0;

    virtual int                 getNumVideoStreams() const = 0;
    virtual VideoStreamSettings getVideoSettings (int streamIndex) const = 0;
    virtual int                 getNumAudioStreams() const = 0;
    virtual AudioStreamSettings getAudioSettings (int streamIndex) const = 0;

    Size   originalSize;
    int    pixelFormat;
    double timebase = {};

    double      sampleRate  = {};
    int         numChannels = 0;
    juce::int64 numSamples  = 0;

protected:
    bool   opened    = false;
    VideoEngine& videoEngine;
private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AVReader)
};

} // foleys
