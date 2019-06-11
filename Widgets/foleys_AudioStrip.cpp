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

void AudioStrip::setClip (std::shared_ptr<AVClip> clipToUse)
{
    clip = clipToUse;

    if (clip && clip->getMediaFile().existsAsFile())
    {
        if (auto* reader = formatManager.createReaderFor (clip->getMediaFile()))
            thumbnail.setReader (reader, 0);
    }
}

void AudioStrip::paint (juce::Graphics& g)
{
    thumbnail.drawChannels (g, getLocalBounds(), startTime, startTime + timeLength, 1.0);
}

void AudioStrip::changeListenerCallback (juce::ChangeBroadcaster*)
{
    repaint();
}

void AudioStrip::setStartAndLength (double start, double length)
{
    startTime = start;
    timeLength = length;

    update();
}

void AudioStrip::update()
{
    if (clip == nullptr || timeLength == 0)
        return;

    auto* threadPool = getThreadPool();
    if (threadPool == nullptr)
        return;

    if (thumbnailJob != nullptr)
        threadPool->removeJob (thumbnailJob.get(), true, 200);
    else
        thumbnailJob = std::make_unique<ThumbnailJob>(*this);

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
    juce::ignoreUnused (owner);
}

juce::ThreadPoolJob::JobStatus AudioStrip::ThumbnailJob::runJob()
{
    return juce::ThreadPoolJob::jobHasFinished;
}

} // foleys
