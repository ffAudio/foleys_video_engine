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

class AudioFifo
{
public:
    AudioFifo (int size = 192000);

    ~AudioFifo() = default;

    void pushSamples (const juce::AudioBuffer<float>& samples);

    void pullSamples (const juce::AudioSourceChannelInfo& info);

    void pushSilence (int numSamples);

    void skipSamples (int numSamples);

    /**
     This method will set the read and write pointer to position, render the fifo empty
     */
    void setPosition (const juce::int64 position);

    juce::int64 getWritePosition() const;
    juce::int64 getReadPosition() const;

    int getFreeSpace() const;

    void setNumChannels (int numChannels);
    void setSampleRate (double sampleRate);

private:
    double sampleRate = 0;

    std::atomic<juce::int64> readPosition {};
    std::atomic<juce::int64> writePosition {};

    juce::AudioBuffer<float> audioBuffer;
    juce::AbstractFifo       audioFifo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioFifo)
};


}
