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

AVCompoundClip::AVCompoundClip()
  : videoRenderJob (*this)
{
    composer = std::make_unique<SoftwareCompositingContext>();
    videoSize = {800, 500};
    videoFifo.setSize (videoSize);
    videoFifo.setTimebase (0.000041666666666666665);

}

juce::String AVCompoundClip::getDescription() const
{
    return "Edit";
}

void AVCompoundClip::addClip (std::shared_ptr<AVClip> clip, double start, double length, double offset)
{
    auto clipDescriptor = std::make_shared<ClipDescriptor> (clip);
    clipDescriptor->name   = clip->getDescription();
    clipDescriptor->start  = start * sampleRate;
    clipDescriptor->offset = offset * sampleRate;
    clipDescriptor->length = length > 0 ? length * sampleRate : clip->getTotalLength();

    clip->prepareToPlay (buffer.getNumSamples(), sampleRate);

    juce::ScopedLock sl (clipDescriptorLock);

    clips.push_back (clipDescriptor);
}

juce::Image AVCompoundClip::getFrame (double pts) const
{
    return videoFifo.getVideoFrame (pts);
}

juce::Image AVCompoundClip::getCurrentFrame() const
{
    const auto pts = sampleRate > 0 ? position.load() / sampleRate : 0.0;
    return videoFifo.getVideoFrame (pts);
}

Size AVCompoundClip::getVideoSize() const
{
    return videoSize;
}

double AVCompoundClip::getCurrentTimeInSeconds() const
{
    return sampleRate > 0 ? position.load() / sampleRate : 0;
}

juce::Image AVCompoundClip::getStillImage (double seconds, Size size)
{
    return {};
}

double AVCompoundClip::getLengthInSeconds() const
{
    return sampleRate > 0 ? getTotalLength() / sampleRate : 0;
}

Timecode AVCompoundClip::getFrameTimecodeForTime (double time) const
{
    return videoFifo.getFrameTimecodeForTime (time);
}

Timecode AVCompoundClip::getCurrentTimecode() const
{
    const auto pts = sampleRate > 0 ? position.load() / sampleRate : 0.0;
    return getFrameTimecodeForTime (pts);
}

void AVCompoundClip::prepareToPlay (int samplesPerBlockExpected, double sampleRateToUse)
{
    sampleRate = sampleRateToUse;
    buffer.setSize (2, samplesPerBlockExpected);

    for (auto& descriptor : getClips())
        descriptor->clip->prepareToPlay (samplesPerBlockExpected, sampleRate);
}

void AVCompoundClip::releaseResources()
{
    for (auto& descriptor : getClips())
        descriptor->clip->releaseResources();

    sampleRate = 0;
}

void AVCompoundClip::getNextAudioBlock (const juce::AudioSourceChannelInfo& info)
{
    info.clearActiveBufferRegion();
    auto pos = position.load();

    for (auto& clip : getActiveClips ([pos](AVCompoundClip::ClipDescriptor& clip) { return pos >= clip.start && pos < clip.start + clip.length; }))
    {
        if (! clip->clip->hasAudio())
            continue;

        auto start = clip->start.load();
        if (pos + info.numSamples >= start && pos < start + clip->length.load())
        {
            auto offset = std::max (int (start - pos), 0);
            juce::AudioSourceChannelInfo reader (&buffer, 0, info.numSamples - offset);
            clip->clip->getNextAudioBlock (reader);
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                info.buffer->addFrom (channel, info.startSample, buffer.getReadPointer (channel), info.numSamples - offset);
        }
    }

    position.fetch_add (info.numSamples);
    triggerAsyncUpdate();
}

void AVCompoundClip::setNextReadPosition (juce::int64 samples)
{
    videoRenderJob.setSuspended (true);

    position.store (samples);
    for (auto& descriptor : getClips())
        descriptor->clip->setNextReadPosition (std::max (juce::int64 (samples + descriptor->offset - descriptor->start), juce::int64 (descriptor->offset)));

    videoFifo.clear();

    videoRenderJob.setSuspended (false);
    triggerAsyncUpdate();
}

juce::int64 AVCompoundClip::getNextReadPosition() const
{
    return position;
}

juce::int64 AVCompoundClip::getTotalLength() const
{
    juce::int64 length = 0;
    for (auto& descriptor : getClips())
        length = std::max (length, descriptor->start + descriptor->length);

    return length;
}

bool AVCompoundClip::isLooping() const
{
    return false;
}

void AVCompoundClip::setLooping (bool shouldLoop)
{
    juce::ignoreUnused (shouldLoop);
}

bool AVCompoundClip::hasVideo() const
{
    bool hasVideo = false;
    for (auto& descriptor : getClips())
        hasVideo |= descriptor->clip->hasVideo();

    return hasVideo;
}

bool AVCompoundClip::hasAudio() const
{
    bool hasAudio = false;
    for (auto& descriptor : clips)
        hasAudio |= descriptor->clip->hasAudio();

    return hasAudio;
}

bool AVCompoundClip::hasSubtitle() const
{
    bool hasSubtitle = false;
    for (auto& descriptor : clips)
        hasSubtitle |= descriptor->clip->hasSubtitle();

    return hasSubtitle;
}

void AVCompoundClip::handleAsyncUpdate()
{
    if (sampleRate > 0 && hasVideo())
    {
        auto currentTimecode = videoFifo.getFrameTimecodeForTime (position.load() / sampleRate);
        if (currentTimecode != lastShownFrame)
        {
            sendTimecode (currentTimecode, juce::sendNotificationAsync);
            lastShownFrame = currentTimecode;
        }

        videoFifo.clearFramesOlderThan (lastShownFrame);
    }
}

std::vector<std::shared_ptr<AVCompoundClip::ClipDescriptor>> AVCompoundClip::getClips() const
{
    juce::ScopedLock sl (clipDescriptorLock);
    return clips;
}

std::vector<std::shared_ptr<AVCompoundClip::ClipDescriptor>> AVCompoundClip::getActiveClips (std::function<bool(AVCompoundClip::ClipDescriptor&)> selector) const
{
    std::vector<std::shared_ptr<ClipDescriptor>> active;
    {
        juce::ScopedLock sl (clipDescriptorLock);
        
        for (auto clip : clips)
            if (selector (*clip))
                active.push_back (clip);
    }

    std::sort (active.begin(), active.end(), [](auto& a, auto& b){ return a->start.load() < b->start.load(); });
    return active;
}

//==============================================================================

AVCompoundClip::ComposingThread::ComposingThread (AVCompoundClip& ownerToUse)
  : owner (ownerToUse)
{
}

juce::TimeSliceClient* AVCompoundClip::getBackgroundJob()
{
    return &videoRenderJob;
}

int AVCompoundClip::ComposingThread::useTimeSlice()
{
    juce::ScopedValueSetter<bool> guard (inRenderBlock, true);

    if (owner.videoFifo.getNumAvailableFrames() >= 10)
        return 30;

    const int duration = 1001;
    const double timebase = 0.000041666666666666665;

    auto image = owner.videoFifo.getOldestFrameForRecycling();
    auto nextTimeCode = owner.videoFifo.getHighestTimeCode() + duration;

    auto timeInSeconds = nextTimeCode * timebase;
    auto pos = timeInSeconds * owner.sampleRate;

    juce::Graphics g (image);
    g.fillAll (juce::Colours::black);

    for (auto& clip : owner.getActiveClips ([pos](AVCompoundClip::ClipDescriptor& clip) { return pos >= clip.start && pos < clip.start + clip.length; }))
    {
        const auto tc = (pos + clip->offset.load() - clip->start.load()) / owner.sampleRate;
        const auto frame = clip->clip->getFrame (tc);

        g.drawImageWithin (frame, 0, 0, image.getWidth(), image.getHeight(), juce::RectanglePlacement::fillDestination);
    }

    owner.videoFifo.pushVideoFrame (image, nextTimeCode);

    return 5;
}

void AVCompoundClip::ComposingThread::setSuspended (bool s)
{
    suspended = s;

    while (suspended && inRenderBlock)
        juce::Thread::sleep (5);
}

//==============================================================================

AVCompoundClip::ClipDescriptor::ClipDescriptor (std::shared_ptr<AVClip> clipToUse)
{
    clip = clipToUse;
}

} // foleys
