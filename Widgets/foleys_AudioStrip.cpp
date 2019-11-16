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

AudioStrip::AudioStrip()
{
    formatManager.registerBasicFormats();
    thumbnail.addChangeListener (this);
    setOpaque (false);
    setInterceptsMouseClicks (false, true);
}

AudioStrip::~AudioStrip()
{
    if (thumbnailJob != nullptr)
        if (auto* threadPool = getThreadPool())
            threadPool->removeJob (thumbnailJob.get(), true, 1000);
}

void AudioStrip::setClip (std::shared_ptr<AVClip> clipToUse)
{
    clip = clipToUse;

    if (clip.get() == nullptr)
        return;

    const auto url = clip->getMediaFile();
    if (url.isLocalFile() && url.getLocalFile().existsAsFile())
    {
        if (auto* reader = formatManager.createReaderFor (url.getLocalFile()))
            thumbnail.setReader (reader, 0);
    }
    else
    {
        update();
    }
}

void AudioStrip::paint (juce::Graphics& g)
{
    thumbnail.drawChannels (g, getLocalBounds(), startTime, endTime, 2.0);
}

void AudioStrip::changeListenerCallback (juce::ChangeBroadcaster*)
{
    repaint();
}

void AudioStrip::setStartAndEnd (double start, double end)
{
    startTime = start;
    endTime = end;

    update();
}

void AudioStrip::update()
{
    if (clip == nullptr ||
        endTime <= startTime ||
        endTime <= rendered ||
        (clip->getMediaFile().isLocalFile() &&
         clip->getMediaFile().getLocalFile().existsAsFile()))
        return;

    auto* threadPool = getThreadPool();
    if (threadPool == nullptr)
        return;

    if (thumbnailJob != nullptr)
        threadPool->removeJob (thumbnailJob.get(), true, 1000);

    thumbnailJob = std::make_unique<ThumbnailJob>(*this);
    rendered = endTime;

    threadPool->addJob (thumbnailJob.get(), false);
}

juce::ThreadPool* AudioStrip::getThreadPool()
{
    if (clip == nullptr)
        return nullptr;

    if (auto* engine = clip->getVideoEngine())
        return &engine->getThreadPool();

    return nullptr;
}

//==============================================================================

AudioStrip::ThumbnailJob::ThumbnailJob (AudioStrip& ownerToUse)
  : juce::ThreadPoolJob ("Audio Thumbnail Job"),
    owner (ownerToUse)
{
    clipToRender = owner.clip->createCopy (StreamTypes::audio());
}

juce::ThreadPoolJob::JobStatus AudioStrip::ThumbnailJob::runJob()
{
    const auto        sampleRate = 48000.0;
    const auto        blockSize  = 1024;
    const juce::int64 length = (owner.startTime + owner.endTime) * sampleRate;
    juce::int64       pos = 0;

    if (shouldExit())
        return juce::ThreadPoolJob::jobHasFinished;

    clipToRender->prepareToPlay (blockSize, sampleRate);
    owner.thumbnail.reset (2, sampleRate, 0);
    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::AudioSourceChannelInfo info (&buffer, 0, buffer.getNumSamples());

    while (!shouldExit() && pos < length)
    {
        buffer.clear();
        clipToRender->waitForSamplesReady (blockSize);
        clipToRender->getNextAudioBlock (info);
        owner.thumbnail.addBlock (pos, buffer, 0, buffer.getNumSamples());
        pos += buffer.getNumSamples();
    }

    return juce::ThreadPoolJob::jobHasFinished;
}

} // foleys
