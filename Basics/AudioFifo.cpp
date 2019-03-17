
namespace foleys
{

AudioFifo::AudioFifo (int size) : audioBuffer (2, size), audioFifo (size)
{
}

void AudioFifo::pushSamples (const juce::AudioBuffer<float>& samples)
{
    jassert (samples.getNumChannels() == audioBuffer.getNumChannels());
    jassert (samples.getNumSamples()  <  audioFifo.getFreeSpace());

    int pos1, size1, pos2, size2;
    audioFifo.prepareToWrite (samples.getNumSamples(), pos1, size1, pos2, size2);

    for (int c=0; c<audioBuffer.getNumChannels(); ++c)
    {
        audioBuffer.copyFrom (c, pos1, samples.getReadPointer (c), size1);
        if (size2 > 0)
            audioBuffer.copyFrom (c, pos2, samples.getReadPointer (c, size1), size2);
    }

    audioFifo.finishedWrite (size1 + size2);
    writePosition.fetch_add (size1 + size2);
}

void AudioFifo::pullSamples (const juce::AudioSourceChannelInfo& info)
{
    int pos1, size1, pos2, size2;
    audioFifo.prepareToRead (info.numSamples, pos1, size1, pos2, size2);

    for (int c=0; c<audioBuffer.getNumChannels(); ++c)
    {
        info.buffer->copyFrom (c, info.startSample, audioBuffer.getReadPointer (c, pos1), size1);
        if (size2 > 0)
            info.buffer->copyFrom (c, info.startSample + size1, audioBuffer.getReadPointer (c, pos2), size2);
    }
    audioFifo.finishedRead (size1 + size2);
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
