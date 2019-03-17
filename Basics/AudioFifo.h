
#pragma once

namespace foleys
{

class AudioFifo
{
public:
    AudioFifo (int size = 16384);

    ~AudioFifo() = default;

    void pushSamples (const juce::AudioBuffer<float>& samples);

    void pullSamples (const juce::AudioSourceChannelInfo& info);

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
