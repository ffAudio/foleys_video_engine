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

AudioClip::AudioClip (VideoEngine& engine) : foleys::AVClip (engine)
{
    addDefaultAudioParameters (*this);
}

juce::String AudioClip::getDescription() const
{
    if (mediaFile.isLocalFile())
        return mediaFile.getLocalFile().getFileNameWithoutExtension();

    return mediaFile.getFileName();
}

juce::URL AudioClip::getMediaFile() const
{
    return mediaFile;
}

void AudioClip::setMediaFile (const juce::URL& media)
{
    mediaFile = media;
}

void AudioClip::setAudioFormatReader (juce::AudioFormatReader* readerToUse, int samplesToBuffer)
{
    if (readerToUse == nullptr)
    {
        resampler.reset();
        readerSource.reset();
        reader.reset();
        return;
    }

    reader.reset (readerToUse);

    if (samplesToBuffer > 0 && getVideoEngine() != nullptr)
    {
        readerSource = std::make_unique<juce::BufferingAudioSource>(new juce::AudioFormatReaderSource (reader.get(), false),
                                                                    getVideoEngine()->getNextTimeSliceThread(),
                                                                    true,
                                                                    samplesToBuffer,
                                                                    reader->numChannels);
    }
    else
    {
        readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader.get(), false);
    }

    setupResampler();
}

void AudioClip::setupResampler()
{
    if (reader.get() == nullptr)
    {
        resampler.reset();
        readerSource.reset();
        return;
    }

    originalSampleRate = reader->sampleRate;
    if (originalSampleRate != sampleRate)
        resampler = std::make_unique<juce::ResamplingAudioSource> (readerSource.get(), false, reader->numChannels);
    else
        resampler.reset();

    if (resampler.get() != nullptr && sampleRate > 0)
    {
        resampler->setResamplingRatio (originalSampleRate / sampleRate);
        resampler->prepareToPlay (samplesPerBlock, sampleRate);
    }
}

void AudioClip::prepareToPlay (int samplesPerBlockExpected, double sampleRateToUse)
{
    sampleRate = sampleRateToUse;
    samplesPerBlock = samplesPerBlockExpected;

    setupResampler();

    readerSource->prepareToPlay (samplesPerBlockExpected, sampleRate);
}

void AudioClip::releaseResources()
{
    if (readerSource)
        readerSource->releaseResources();
}

void AudioClip::getNextAudioBlock (const juce::AudioSourceChannelInfo& info)
{
    const auto gain = juce::Decibels::decibelsToGain (getAudioParameters().at(IDs::gain)->getRealValue());

    if (resampler.get() != nullptr)
        resampler->getNextAudioBlock (info);
    else if (readerSource.get() != nullptr)
        readerSource->getNextAudioBlock (info);
    else
        info.clearActiveBufferRegion();

    info.buffer->applyGainRamp (info.startSample, info.numSamples, lastGain, gain);
    lastGain = gain;
}

void AudioClip::setNextReadPosition (juce::int64 samples)
{
    if (readerSource)
    {
        if (sampleRate > 0 && originalSampleRate != sampleRate)
            readerSource->setNextReadPosition (samples * originalSampleRate / sampleRate);
        else
            readerSource->setNextReadPosition (samples);
    }
}

juce::int64 AudioClip::getNextReadPosition() const
{
    if (readerSource)
    {
        if (originalSampleRate > 0 && sampleRate != originalSampleRate)
            return readerSource->getNextReadPosition() * sampleRate / originalSampleRate;

        return readerSource->getNextReadPosition();
    }

    return 0;
}

juce::int64 AudioClip::getTotalLength() const
{
    if (readerSource)
    {
        if (originalSampleRate > 0 && sampleRate != originalSampleRate)
            return readerSource->getTotalLength() * sampleRate / originalSampleRate;

        return readerSource->getTotalLength();
    }

    return 0;
}

double AudioClip::getCurrentTimeInSeconds() const
{
    if (readerSource && originalSampleRate > 0.0)
        return readerSource->getNextReadPosition() / originalSampleRate;

    return 0.0;
}

double AudioClip::getLengthInSeconds() const
{
    if (readerSource && originalSampleRate > 0.0)
        return readerSource->getTotalLength() / originalSampleRate;

    return 0.0;
}

std::shared_ptr<AVClip> AudioClip::createCopy (StreamTypes types)
{
    auto* engine = getVideoEngine();
    if (engine == nullptr)
        return {};

    return engine->createClipFromFile (getMediaFile(), types);
}

} // foleys
