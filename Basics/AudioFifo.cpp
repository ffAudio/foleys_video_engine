
namespace foleys
{

AudioFifo::AudioFifo (int size) : audioBuffer (2, size), audioFifo (size)
{
}

void AudioFifo::pushSamples (const juce::AudioBuffer<float>& samples)
{
    jassert (samples.getNumChannels() == audioBuffer.getNumChannels());
    jassert (samples.getNumSamples()  <  audioFifo.getFreeSpace());

    auto write = audioFifo.write(samples.getNumSamples());

    for (int c=0; c<audioBuffer.getNumChannels(); ++c)
    {
        audioBuffer.copyFrom (c, write.startIndex1, samples.getReadPointer (c), write.blockSize1);
        if (write.blockSize2 > 0)
            audioBuffer.copyFrom (c, write.startIndex2, samples.getReadPointer (c, write.blockSize1), write.blockSize2);
    }

    writePosition.fetch_add (write.blockSize1 + write.blockSize2);
}

void AudioFifo::pullSamples (const juce::AudioSourceChannelInfo& info)
{
    auto read = audioFifo.read (info.numSamples);

    for (int c=0; c<audioBuffer.getNumChannels(); ++c)
    {
        info.buffer->copyFrom (c, info.startSample, audioBuffer.getReadPointer (c, read.startIndex1), read.blockSize1);
        if (read.blockSize2 > 0)
            info.buffer->copyFrom (c, info.startSample + read.blockSize1, audioBuffer.getReadPointer (c, read.startIndex2), read.blockSize2);
    }

    readPosition.fetch_add (read.blockSize1 + read.blockSize2);
}

void AudioFifo::setPosition (const juce::int64 position)
{
    readPosition.store (position);
    writePosition.store (position);
    audioFifo.reset();
}

juce::int64 AudioFifo::getWritePosition() const
{
    return writePosition.load();
}

juce::int64 AudioFifo::getReadPosition() const
{
    return readPosition.load();
}

int AudioFifo::getFreeSpace() const
{
    return audioFifo.getFreeSpace();
}

void AudioFifo::setNumChannels (int numChannels)
{
    audioBuffer.setSize (numChannels, audioBuffer.getNumSamples());
}

void AudioFifo::setSampleRate (double sampleRateToUse)
{
    sampleRate = sampleRateToUse;
}

}
