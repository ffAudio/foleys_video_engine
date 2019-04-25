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
}

juce::String AudioClip::getDescription() const
{
    return mediaFile.getFileNameWithoutExtension();
}

juce::File AudioClip::getMediaFile() const
{
    return mediaFile;
}

void AudioClip::setMediaFile (const juce::File& media)
{
    mediaFile = media;
}

void AudioClip::setAudioFormatReader (juce::AudioFormatReader* reader)
{
    readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
}

void AudioClip::prepareToPlay (int samplesPerBlockExpected, double sampleRateToUse)
{
    sampleRate = sampleRateToUse;
    if (readerSource)
        readerSource->prepareToPlay (samplesPerBlockExpected, sampleRate);
}

void AudioClip::releaseResources()
{
    if (readerSource)
        readerSource->releaseResources();
}

void AudioClip::getNextAudioBlock (const juce::AudioSourceChannelInfo& info)
{
    if (readerSource.get() == nullptr)
    {
        info.clearActiveBufferRegion();
        return;
    }

    readerSource->getNextAudioBlock (info);
}

void AudioClip::setNextReadPosition (juce::int64 samples)
{
    if (readerSource)
        readerSource->setNextReadPosition (samples);
}

juce::int64 AudioClip::getNextReadPosition() const
{
    if (readerSource)
        return readerSource->getNextReadPosition();

    return 0;
}

juce::int64 AudioClip::getTotalLength() const
{
    if (readerSource)
        return readerSource->getTotalLength();

    return 0;
}

double AudioClip::getCurrentTimeInSeconds() const
{
    if (readerSource && sampleRate > 0.0)
        return readerSource->getNextReadPosition() / sampleRate;

    return 0.0;
}

double AudioClip::getLengthInSeconds() const
{
    if (readerSource && sampleRate > 0.0)
        return readerSource->getTotalLength() / sampleRate;

    return 0.0;
}

Timecode AudioClip::getFrameTimecodeForTime (double time) const
{
    return {};
}

Timecode AudioClip::getCurrentTimecode() const
{
    return {};
}

std::shared_ptr<AVClip> AudioClip::createCopy()
{
    if (videoEngine == nullptr)
        return {};

    return videoEngine->createClipFromFile (getMediaFile());
}

}
