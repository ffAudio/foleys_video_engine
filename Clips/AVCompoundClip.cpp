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

namespace IDs
{
    juce::Identifier compoundClip { "CompoundClip" };

    juce::Identifier clip         { "Clip" };
    juce::Identifier description  { "description" };
    juce::Identifier source       { "source" };
    juce::Identifier start        { "start" };
    juce::Identifier length       { "length" };
    juce::Identifier offset       { "offset" };
}

AVCompoundClip::AVCompoundClip (VideoEngine& engine)
  : AVClip (engine),
    videoRenderJob (*this)
{
    state = juce::ValueTree (IDs::compoundClip);

    composer = std::make_unique<SoftwareCompositingContext>();
    videoSize = {800, 500};
    videoFifo.setSize (videoSize);
    videoFifo.setTimebase (0.000041666666666666665);

}

juce::String AVCompoundClip::getDescription() const
{
    return "Edit";
}

std::shared_ptr<AVCompoundClip::ClipDescriptor> AVCompoundClip::addClip (std::shared_ptr<AVClip> clip, double start, double length, double offset)
{
    auto clipDescriptor = std::make_shared<ClipDescriptor> (*this, clip);
    clip->prepareToPlay (buffer.getNumSamples(), sampleRate);

    clipDescriptor->setDescription (clip->getDescription());
    clipDescriptor->setStart (start);
    clipDescriptor->setOffset (offset);
    clipDescriptor->setLength (length > 0 ? length : clip->getLengthInSeconds());

    clipDescriptor->updateSampleCounts();

    juce::ScopedLock sl (clipDescriptorLock);

    clips.push_back (clipDescriptor);

    state.appendChild (clipDescriptor->getStatusTree(), getUndoManager());

    return clipDescriptor;
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
    {
        descriptor->clip->prepareToPlay (samplesPerBlockExpected, sampleRate);
        descriptor->updateSampleCounts();
    }
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

double AVCompoundClip::getSampleRate() const
{
    return sampleRate;
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

juce::UndoManager* AVCompoundClip::getUndoManager()
{
    return videoEngine ? videoEngine->getUndoManager() : nullptr;
}

juce::ValueTree& AVCompoundClip::getStatusTree()
{
    return state;
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

AVCompoundClip::ClipDescriptor::ClipDescriptor (AVCompoundClip& ownerToUse, std::shared_ptr<AVClip> clipToUse)
  : owner (ownerToUse)
{
    clip = clipToUse;
    state = juce::ValueTree (IDs::clip);
    state.addListener (this);
}

juce::String AVCompoundClip::ClipDescriptor::getDescription() const
{
    return state.getProperty (IDs::description, "unnamed");
}

void AVCompoundClip::ClipDescriptor::setDescription (const juce::String& name)
{
    state.setProperty (IDs::description, name, owner.getUndoManager());
}

double AVCompoundClip::ClipDescriptor::getStart() const
{
    return state.getProperty (IDs::start, 0.0);
}

void AVCompoundClip::ClipDescriptor::setStart (double s)
{
    state.setProperty (IDs::start, s, owner.getUndoManager());
}

double AVCompoundClip::ClipDescriptor::getLength() const
{
    return state.getProperty (IDs::length, 0.0);
}

void AVCompoundClip::ClipDescriptor::setLength (double l)
{
    state.setProperty (IDs::length, l, owner.getUndoManager());
}

double AVCompoundClip::ClipDescriptor::getOffset() const
{
    return state.getProperty (IDs::offset, 0.0);
}

void AVCompoundClip::ClipDescriptor::setOffset (double o)
{
    state.setProperty (IDs::offset, o, owner.getUndoManager());
}

void AVCompoundClip::ClipDescriptor::valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
                                                               const juce::Identifier& property)
{
    if (treeWhosePropertyHasChanged != state)
        return;

    updateSampleCounts();
}

void AVCompoundClip::ClipDescriptor::updateSampleCounts()
{
    auto sampleRate = clip->getSampleRate();

    start = sampleRate * double (state.getProperty (IDs::start));
    length = sampleRate * double (state.getProperty (IDs::length));
    offset = sampleRate * double (state.getProperty (IDs::offset));

}

juce::ValueTree& AVCompoundClip::ClipDescriptor::getStatusTree()
{
    return state;
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
        return 10;

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

    return 0;
}

void AVCompoundClip::ComposingThread::setSuspended (bool s)
{
    suspended = s;

    while (suspended && inRenderBlock)
        juce::Thread::sleep (5);
}


} // foleys
