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

class AVReader
{
public:

    AVReader() = default;
    virtual ~AVReader() = default;

    virtual juce::File getMediaFile() const = 0;

    bool isOpenedOk() const { return opened; }
    virtual juce::int64 getTotalLength() const = 0;

    virtual void setPosition (const juce::int64 position) = 0;

    virtual juce::Image getStillImage (double seconds, Size size) = 0;

    virtual void readNewData (VideoFifo&, AudioFifo&) = 0;

    virtual bool hasVideo() const = 0;
    virtual bool hasAudio() const = 0;
    virtual bool hasSubtitle() const = 0;

    virtual void setOutputSampleRate (double sampleRate) = 0;

    Size   originalSize;
    int    pixelFormat;
    double timebase = {};

    double      sampleRate  = {};
    int         numChannels = 0;
    juce::int64 numSamples  = 0;

protected:
    bool   opened    = false;

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AVReader)
};

} // foleys
