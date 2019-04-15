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

void AudioFifo::setNumSamples (int samples)
{
    audioFifo.setTotalSize (samples);
    audioBuffer.setSize (2, samples);
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

void AudioFifo::pushSilence (int numSamples)
{
    jassert (numSamples  <  audioFifo.getFreeSpace());

    auto clear = audioFifo.write (numSamples);
    audioBuffer.clear (clear.startIndex1, clear.blockSize1);
    if (clear.blockSize2 > 0)
        audioBuffer.clear (clear.startIndex2, clear.blockSize2);

    writePosition.fetch_add (clear.blockSize1 + clear.blockSize2);
}

void AudioFifo::skipSamples (int numSamples)
{
    audioFifo.read (numSamples);
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

} // foleys
