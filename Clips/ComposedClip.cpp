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
    static juce::Identifier compoundClip { "CompoundClip" };

    static juce::Identifier clip         { "Clip" };
    static juce::Identifier description  { "description" };
    static juce::Identifier source       { "source" };
    static juce::Identifier start        { "start" };
    static juce::Identifier length       { "length" };
    static juce::Identifier offset       { "offset" };
    static juce::Identifier audioProcessors { "AudioProcessors" };
    static juce::Identifier videoProcessors { "VideoProcessors" };
}

ComposedClip::ComposedClip (VideoEngine& engine)
  : AVClip (engine),
    videoRenderJob (*this)
{
    state = juce::ValueTree (IDs::compoundClip);

    videoMixer = std::make_unique<SoftwareVideoMixer>();
    videoSize = {800, 500};
    auto& settings = videoFifo.getVideoSettings();
    settings.frameSize = videoSize;
    settings.timebase = 24000;
    settings.defaultDuration = 1001;

    audioMixer = std::make_unique<DefaultAudioMixer>();

    state.addListener (this);
}

juce::String ComposedClip::getDescription() const
{
    return "Edit";
}

void ComposedClip::invalidateVideo()
{
    const auto wasSuspended = videoRenderJob.isSuspended();
    videoRenderJob.setSuspended (true);

    videoFifo.clear();
    lastShownFrame = 0;
    triggerAsyncUpdate();
    handleUpdateNowIfNeeded();

    videoRenderJob.setSuspended (wasSuspended);
}

std::shared_ptr<ClipDescriptor> ComposedClip::addClip (std::shared_ptr<AVClip> clip, double start, double length, double offset)
{
    auto clipDescriptor = std::make_shared<ClipDescriptor> (*this, clip);
    clip->prepareToPlay (audioSettings.defaultNumSamples, audioSettings.timebase);

    clipDescriptor->setDescription (clip->getDescription());
    clipDescriptor->setStart (start);
    clipDescriptor->setOffset (offset);
    clipDescriptor->setLength (length > 0 ? length : clip->getLengthInSeconds());

    clipDescriptor->updateSampleCounts();

    juce::ScopedLock sl (clipDescriptorLock);

    clips.push_back (clipDescriptor);

    juce::ScopedValueSetter<bool> manual (manualStateChange, true);
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
    return videoFifo.getVideoFrame (getCurrentTimeInSeconds()).second;
}

Size ComposedClip::getVideoSize() const
{
    return videoSize;
}

double ComposedClip::getCurrentTimeInSeconds() const
{
    return convertToSamples (position.load());
}

juce::Image ComposedClip::getStillImage (double seconds, Size size)
{
    // TODO
    return {};
}

double ComposedClip::getLengthInSeconds() const
{
    return convertToSamples (getTotalLength());
}

void ComposedClip::prepareToPlay (int samplesPerBlockExpected, double sampleRateToUse)
{
    audioSettings.timebase = sampleRateToUse;
    audioSettings.defaultNumSamples = samplesPerBlockExpected;

    audioMixer->setup (audioSettings.numChannels, audioSettings.timebase, audioSettings.defaultNumSamples);

    for (auto& descriptor : getClips())
    {
        descriptor->clip->prepareToPlay (audioSettings.defaultNumSamples, audioSettings.timebase);
        descriptor->updateSampleCounts();
    }

    videoRenderJob.setSuspended (false);
}

void ComposedClip::releaseResources()
{
    for (auto& descriptor : getClips())
        descriptor->clip->releaseResources();
}

void ComposedClip::getNextAudioBlock (const juce::AudioSourceChannelInfo& info)
{
    info.clearActiveBufferRegion();
    auto pos = position.load();

    audioMixer->mixAudio (info,
                          position.load(),
                          getCurrentTimeInSeconds(),
                          getActiveClips ([pos](ClipDescriptor& clip) { return clip.clip->hasAudio() && pos >= clip.start && pos < clip.start + clip.length; }));

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

    lastShownFrame = 0;
    videoRenderJob.setSuspended (wasSuspended);

    triggerAsyncUpdate();
}

juce::int64 ComposedClip::getNextReadPosition() const
{
    return position;
}

juce::int64 ComposedClip::getTotalLength() const
{
    int64_t length = 0;
    for (auto& descriptor : getClips())
        length = std::max (length, descriptor->start.load() + descriptor->length.load());

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

std::shared_ptr<AVClip> ComposedClip::createCopy()
{
    auto* engine = getVideoEngine();
    if (engine == nullptr)
        return {};

    auto clipCopy = engine->createComposedClip();
    for (auto clip : getStatusTree())
        clipCopy->getStatusTree().appendChild (clip.createCopy(), nullptr);

    return clipCopy;
}

double ComposedClip::getSampleRate() const
{
    return audioSettings.timebase;
}

int ComposedClip::getDefaultBufferSize() const
{
    return audioSettings.defaultNumSamples;
}

void ComposedClip::handleAsyncUpdate()
{
    if (audioSettings.timebase > 0 && hasVideo())
    {
        auto seconds = getCurrentTimeInSeconds();
        auto count = videoFifo.getFrameCountForTime (seconds);
        if (count != lastShownFrame)
        {
            sendTimecode (count, seconds, juce::sendNotificationAsync);
            lastShownFrame = count;
        }

        videoFifo.clearFramesOlderThan (count);
    }
}

double ComposedClip::convertToSamples (int64_t pos) const
{
    if (audioSettings.timebase > 0)
        return pos / double (audioSettings.timebase);

    return {};
}

void ComposedClip::valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
                                               const juce::Identifier& property)
{
}

void ComposedClip::valueTreeChildAdded (juce::ValueTree& parentTree,
                                          juce::ValueTree& childWhichHasBeenAdded)
{
    if (manualStateChange)
        return;

    auto descriptor = std::make_shared<ClipDescriptor>(*this, childWhichHasBeenAdded);
    if (descriptor->clip != nullptr)
    {
        descriptor->updateSampleCounts();

        juce::ScopedLock sl (clipDescriptorLock);
        clips.push_back (std::move (descriptor));
    }
}

void ComposedClip::valueTreeChildRemoved (juce::ValueTree& parentTree,
                                            juce::ValueTree& childWhichHasBeenRemoved,
                                            int indexFromWhichChildWasRemoved)
{
    if (manualStateChange)
        return;

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
    auto* engine = getVideoEngine();
    if (engine)
        return engine->getUndoManager();

    return nullptr;
}

juce::ValueTree& ComposedClip::getStatusTree()
{
    return state;
}

std::vector<std::shared_ptr<ClipDescriptor>> ComposedClip::getClips() const
{
    juce::ScopedLock sl (clipDescriptorLock);
    return clips;
}

std::vector<std::shared_ptr<ClipDescriptor>> ComposedClip::getActiveClips (std::function<bool(ClipDescriptor&)> selector) const
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
    auto pos = timeInSeconds * owner.getSampleRate();

    owner.videoMixer->compose (image, settings, nextTimeCode, timeInSeconds,
                               owner.getActiveClips ([pos](ClipDescriptor& clip) { return clip.clip->hasVideo() && pos >= clip.start && pos < clip.start + clip.length; }));

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
