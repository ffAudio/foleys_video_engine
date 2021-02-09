/*
 ==============================================================================

 Copyright (c) 2019 - 2021, Foleys Finest Audio - Daniel Walz
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
    clipAudioParameters = juce::ValueTree (IDs::audioParameters);

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

void AudioStrip::setClip (std::shared_ptr<ClipDescriptor> descriptor)
{
    clipAudioParameters = descriptor->getStatusTree().getChildWithName (IDs::audioParameters);
    setClip (descriptor->clip);
}

void AudioStrip::setClip (std::shared_ptr<AVClip> clipToUse)
{
    clip = clipToUse;

    if (clip.get() == nullptr)
        return;

    update();
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
        endTime <= rendered)
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

    parameterController = std::make_unique<ClipDescriptor::ClipParameterController>(*this);
    parameterController->setClip (clipToRender->getAudioParameters(), owner.clipAudioParameters, nullptr);
}

double AudioStrip::ThumbnailJob::getCurrentTimeInSeconds() const
{
    return position / sampleRate;
}

juce::ThreadPoolJob::JobStatus AudioStrip::ThumbnailJob::runJob()
{
    const auto blockSize  = 1024;
    const auto length = juce::int64 ((owner.startTime + owner.endTime) * sampleRate);

    if (shouldExit())
        return juce::ThreadPoolJob::jobHasFinished;

    clipToRender->prepareToPlay (blockSize, sampleRate);
    if (owner.thumbnail.getNumChannels() != 2)
        owner.thumbnail.reset (2, sampleRate, 0);

    position = juce::int64 (std::max (0.0, owner.thumbnail.getTotalLength() * sampleRate - blockSize));

    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::AudioSourceChannelInfo info (&buffer, 0, buffer.getNumSamples());

    if (position >= blockSize)
    {
        // one to discard -> update automation state from potential smoothing
        parameterController->updateAutomations (getCurrentTimeInSeconds());
        clipToRender->getNextAudioBlock (info);
        position += buffer.getNumSamples();
    }

    int retryCount = 0;
    while (!shouldExit() && position < length)
    {
        parameterController->updateAutomations (getCurrentTimeInSeconds());
        if (clipToRender->waitForSamplesReady (blockSize))
        {
            clipToRender->getNextAudioBlock (info);
            owner.thumbnail.addBlock (position, buffer, 0, buffer.getNumSamples());
            position += buffer.getNumSamples();
            retryCount = 0;
        }
        else
        {
            if (++retryCount > 10)
                return juce::ThreadPoolJob::jobHasFinished;
        }
    }

    return juce::ThreadPoolJob::jobHasFinished;
}

} // foleys
