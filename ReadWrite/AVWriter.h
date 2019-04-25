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

class AVWriter
{
public:



    AVWriter() = default;
    virtual ~AVWriter() = default;

    virtual juce::File getMediaFile() const = 0;

    virtual bool isOpenedOk() const = 0;

    virtual void pushSamples (const juce::AudioBuffer<float>& buffer, int stream = 0) = 0;

    virtual void pushImage (juce::int64 pos, juce::Image image, int stream = 0) = 0;

    virtual int addVideoStream (const VideoStreamSettings& settings) = 0;

    virtual int addAudioStream (const AudioStreamSettings& settings) = 0;

    virtual bool startWriting() = 0;

    virtual void finishWriting() = 0;

    static juce::StringArray getMuxers() { return {}; }

    static juce::StringArray getPixelFormats() { return {}; }

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AVWriter)
};

} // foleys
