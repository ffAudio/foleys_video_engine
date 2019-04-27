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
    juce::Identifier videoLine    { "videoLine" };
    juce::Identifier audioLine    { "audioLine" };
}

ComposedClip::ComposedClip (VideoEngine& engine)
  : AVClip (engine),
    videoRenderJob (*this)
{
    state = juce::ValueTree (IDs::compoundClip);

    composer = std::make_unique<SoftwareCompositingContext>();
    videoSize = {800, 500};
    auto& settings = videoFifo.getVideoSettings();
    settings.frameSize = videoSize;
    settings.timebase = 24000;
    settings.defaultDuration = 1001;

    state.addListener (this);
}

juce::String ComposedClip::getDescription() const
{
    return "Edit";
}

std::shared_ptr<ComposedClip::ClipDescriptor> ComposedClip::addClip (std::shared_ptr<AVClip> clip, double start, double length, double offset)
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

void ComposedClip::removeClip (std::shared_ptr<ClipDescriptor> descriptor)
{
    auto it = std::find (clips.begin(), clips.end(), descriptor);
    if (it != clips.end())
        clips.erase (it);

    state.removeChild (descriptor->getStatusTree(), getUndoManager());
}

std::pair<int64_t, juce::Image> ComposedClip::getFrame (double pts) const
{
    return videoFifo.getVideoFrame (pts);
}

bool ComposedClip::isFrameAvailable (double pts) const
{
    return videoFifo.isFrameAvailable (pts);
}

juce::Image ComposedClip::getCurrentFrame() const
{
    const auto pts = sampleRate > 0 ? position.load() / sampleRate : 0.0;
    return videoFifo.getVideoFrame (pts).second;
}

Size ComposedClip::getVideoSize() const
{
    return videoSize;
}

double ComposedClip::getCurrentTimeInSeconds() const
{
    return sampleRate > 0 ? position.load() / sampleRate : 0;
}

juce::Image ComposedClip::getStillImage (double seconds, Size size)
{
    return {};
}

double ComposedClip::getLengthInSeconds() const
{
    return sampleRate > 0 ? getTotalLength() / sampleRate : 0;
}

Timecode ComposedClip::getFrameTimecodeForTime (double time) const
{
    return videoFifo.getFrameTimecodeForTime (time);
}

Timecode ComposedClip::getCurrentTimecode() const
{
    const auto pts = sampleRate > 0 ? position.load() / sampleRate : 0.0;
    return getFrameTimecodeForTime (pts);
}

void ComposedClip::prepareToPlay (int samplesPerBlockExpected, double sampleRateToUse)
{
    sampleRate = sampleRateToUse;
    buffer.setSize (2, samplesPerBlockExpected);

    for (auto& descriptor : getClips())
    {
        descriptor->clip->prepareToPlay (samplesPerBlockExpected, sampleRate);
        descriptor->updateSampleCounts();
    }

    videoRenderJob.setSuspended (false);
}

void ComposedClip::releaseResources()
{
    for (auto& descriptor : getClips())
        descriptor->clip->releaseResources();

    sampleRate = 0;
}

void ComposedClip::getNextAudioBlock (const juce::AudioSourceChannelInfo& info)
{
    info.clearActiveBufferRegion();
    auto pos = position.load();

    for (auto& clip : getActiveClips ([pos](ComposedClip::ClipDescriptor& clip) { return pos >= clip.start && pos < clip.start + clip.length; }))
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
                info.buffer->addFrom (channel, info.startSample + offset, buffer.getReadPointer (channel), info.numSamples - offset);
        }
    }

    position.fetch_add (info.numSamples);
    triggerAsyncUpdate();
}

void ComposedClip::setNextReadPosition (juce::int64 samples)
{
    const auto wasSuspended = videoRenderJob.isSuspended();
    videoRenderJob.setSuspended (true);

    position.store (samples);
    for (auto& descriptor : getClips())
        descriptor->clip->setNextReadPosition (std::max (juce::int64 (samples + descriptor->offset - descriptor->start), juce::int64 (descriptor->offset)));

    videoFifo.clear();

    lastShownFrame = { -1, double (videoFifo.getVideoSettings().timebase) };
    videoRenderJob.setSuspended (wasSuspended);

    triggerAsyncUpdate();
}

juce::int64 ComposedClip::getNextReadPosition() const
{
    return position;
}

juce::int64 ComposedClip::getTotalLength() const
{
    juce::int64 length = 0;
    for (auto& descriptor : getClips())
        length = std::max (length, descriptor->start + descriptor->length);

    return length;
}

bool ComposedClip::isLooping() const
{
    return false;
}

void ComposedClip::setLooping (bool shouldLoop)
{
    juce::ignoreUnused (shouldLoop);
}

bool ComposedClip::hasVideo() const
{
    bool hasVideo = false;
    for (auto& descriptor : getClips())
        hasVideo |= descriptor->clip->hasVideo();

    return hasVideo;
}

bool ComposedClip::hasAudio() const
{
    bool hasAudio = false;
    for (auto& descriptor : clips)
        hasAudio |= descriptor->clip->hasAudio();

    return hasAudio;
}

bool ComposedClip::hasSubtitle() const
{
    bool hasSubtitle = false;
    for (auto& descriptor : clips)
        hasSubtitle |= descriptor->clip->hasSubtitle();

    return hasSubtitle;
}

std::shared_ptr<AVClip> ComposedClip::createCopy()
{
    if (videoEngine == nullptr)
        return {};

    auto clipCopy = videoEngine->createComposedClip();
    for (auto clip : getStatusTree())
        clipCopy->getStatusTree().appendChild (clip.createCopy(), nullptr);

    return clipCopy;
}

double ComposedClip::getSampleRate() const
{
    return sampleRate;
}

void ComposedClip::handleAsyncUpdate()
{
    if (sampleRate > 0 && hasVideo())
    {
        auto currentTimecode = videoFifo.getFrameTimecodeForTime (position.load() / sampleRate);
        if ( currentTimecode != lastShownFrame)
        {
            sendTimecode (currentTimecode, juce::sendNotificationAsync);
            lastShownFrame = currentTimecode;
        }

        videoFifo.clearFramesOlderThan (lastShownFrame);
    }
}

void ComposedClip::valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
                                               const juce::Identifier& property)
{
}

void ComposedClip::valueTreeChildAdded (juce::ValueTree& parentTree,
                                          juce::ValueTree& childWhichHasBeenAdded)
{
    auto descriptor = std::make_shared<ClipDescriptor>(*this, childWhichHasBeenAdded);
    if (descriptor->clip != nullptr)
    {
        descriptor->updateSampleCounts();

        juce::ScopedLock sl (clipDescriptorLock);
        clips.push_back (descriptor);
    }
}

void ComposedClip::valueTreeChildRemoved (juce::ValueTree& parentTree,
                                            juce::ValueTree& childWhichHasBeenRemoved,
                                            int indexFromWhichChildWasRemoved)
{
    for (auto it = clips.begin(); it != clips.end(); ++it)
    {
        if ((*it)->getStatusTree() == childWhichHasBeenRemoved)
        {
            clips.erase (it);
            return;
        }
    }
}

juce::UndoManager* ComposedClip::getUndoManager()
{
    return videoEngine ? videoEngine->getUndoManager() : nullptr;
}

juce::ValueTree& ComposedClip::getStatusTree()
{
    return state;
}

std::vector<std::shared_ptr<ComposedClip::ClipDescriptor>> ComposedClip::getClips() const
{
    juce::ScopedLock sl (clipDescriptorLock);
    return clips;
}

std::vector<std::shared_ptr<ComposedClip::ClipDescriptor>> ComposedClip::getActiveClips (std::function<bool(ComposedClip::ClipDescriptor&)> selector) const
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

ComposedClip::ClipDescriptor::ClipDescriptor (ComposedClip& ownerToUse, std::shared_ptr<AVClip> clipToUse)
  : owner (ownerToUse)
{
    clip = clipToUse;
    state = juce::ValueTree (IDs::clip);
    auto mediaFile = clip->getMediaFile();
    if (mediaFile.getFullPathName().isNotEmpty())
        state.setProperty (IDs::source, mediaFile.getFullPathName(), nullptr);

    state.addListener (this);
}

ComposedClip::ClipDescriptor::ClipDescriptor (ComposedClip& ownerToUse, juce::ValueTree stateToUse)
  : owner (ownerToUse)
{
    state = stateToUse;
    if (state.hasProperty (IDs::source) && owner.videoEngine)
    {
        auto source = state.getProperty (IDs::source);
        clip = owner.videoEngine->createClipFromFile ({source});
    }
    state.addListener (this);
}

juce::String ComposedClip::ClipDescriptor::getDescription() const
{
    return state.getProperty (IDs::description, "unnamed");
}

void ComposedClip::ClipDescriptor::setDescription (const juce::String& name)
{
    state.setProperty (IDs::description, name, owner.getUndoManager());
}

double ComposedClip::ClipDescriptor::getStart() const
{
    return state.getProperty (IDs::start, 0.0);
}

void ComposedClip::ClipDescriptor::setStart (double s)
{
    state.setProperty (IDs::start, s, owner.getUndoManager());
}

double ComposedClip::ClipDescriptor::getLength() const
{
    return state.getProperty (IDs::length, 0.0);
}

void ComposedClip::ClipDescriptor::setLength (double l)
{
    state.setProperty (IDs::length, l, owner.getUndoManager());
}

double ComposedClip::ClipDescriptor::getOffset() const
{
    return state.getProperty (IDs::offset, 0.0);
}

void ComposedClip::ClipDescriptor::setOffset (double o)
{
    state.setProperty (IDs::offset, o, owner.getUndoManager());
}

int ComposedClip::ClipDescriptor::getVideoLine() const
{
    return state.getProperty (IDs::videoLine, 0.0);
}

void ComposedClip::ClipDescriptor::setVideoLine (int line)
{
    state.setProperty (IDs::videoLine, line, owner.getUndoManager());
}

int ComposedClip::ClipDescriptor::getAudioLine() const
{
    return state.getProperty (IDs::audioLine, 0.0);
}

void ComposedClip::ClipDescriptor::setAudioLine (int line)
{
    state.setProperty (IDs::audioLine, line, owner.getUndoManager());
}

void ComposedClip::ClipDescriptor::valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
                                                               const juce::Identifier& property)
{
    if (treeWhosePropertyHasChanged != state)
        return;

    updateSampleCounts();
}

void ComposedClip::ClipDescriptor::updateSampleCounts()
{
    auto sampleRate = clip->getSampleRate();

    start = sampleRate * double (state.getProperty (IDs::start));
    length = sampleRate * double (state.getProperty (IDs::length));
    offset = sampleRate * double (state.getProperty (IDs::offset));

}

juce::ValueTree& ComposedClip::ClipDescriptor::getStatusTree()
{
    return state;
}

//==============================================================================

ComposedClip::ComposingThread::ComposingThread (ComposedClip& ownerToUse)
  : owner (ownerToUse)
{
}

juce::TimeSliceClient* ComposedClip::getBackgroundJob()
{
    return &videoRenderJob;
}

int ComposedClip::ComposingThread::useTimeSlice()
{
    juce::ScopedValueSetter<bool> guard (inRenderBlock, true);

    if (suspended || owner.videoFifo.getNumAvailableFrames() >= 10)
        return 10;

    auto& settings = owner.videoFifo.getVideoSettings();

    auto image = owner.videoFifo.getOldestFrameForRecycling();
    auto current = convertTimecode (owner.getCurrentTimeInSeconds(), settings);
    auto highest = owner.videoFifo.getHighestTimeCode() + settings.defaultDuration;

    auto nextTimeCode = std::max (current, highest);

    auto timeInSeconds = nextTimeCode / double (settings.timebase);
    auto pos = timeInSeconds * owner.sampleRate;

    juce::Graphics g (image);
    g.fillAll (juce::Colours::black);

    for (auto& clip : owner.getActiveClips ([pos](ComposedClip::ClipDescriptor& clip) { return pos >= clip.start && pos < clip.start + clip.length; }))
    {
        const auto tc = (pos + clip->offset.load() - clip->start.load()) / owner.sampleRate;
        const auto frame = clip->clip->getFrame (tc);

        g.drawImageWithin (frame.second, 0, 0, image.getWidth(), image.getHeight(), juce::RectanglePlacement::centred);
    }

    owner.videoFifo.pushVideoFrame (image, nextTimeCode);

    return 0;
}

void ComposedClip::ComposingThread::setSuspended (bool s)
{
    suspended = s;

    while (suspended && inRenderBlock)
        juce::Thread::sleep (5);
}

bool ComposedClip::ComposingThread::isSuspended() const
{
    return suspended;
}


} // foleys
