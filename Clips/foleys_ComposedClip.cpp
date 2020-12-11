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
    static juce::Identifier visible      { "visible" };
    static juce::Identifier audio        { "audio" };
    static juce::Identifier state        { "state" };
    static juce::Identifier audioProcessors { "AudioProcessors" };
    static juce::Identifier videoProcessors { "VideoProcessors" };
}

ComposedClip::ComposedClip (VideoEngine& engine)
  : AVClip (engine)
{
    state = juce::ValueTree (IDs::compoundClip);

    videoMixer = std::make_unique<SoftwareVideoMixer>();

    videoSettings.frameSize = {1280, 720};
    videoSettings.timebase = 24000;
    videoSettings.defaultDuration = 1001;

    audioMixer = std::make_unique<DefaultAudioMixer>();

    state.addListener (this);
}

juce::String ComposedClip::getDescription() const
{
    return "Edit";
}

double ComposedClip::getFrameDurationInSeconds() const
{
    return double (videoSettings.defaultDuration) / double (videoSettings.timebase);
}

void ComposedClip::invalidateVideo()
{
    lastShownFrame = -1;
    triggerAsyncUpdate();
    handleUpdateNowIfNeeded();
}

std::shared_ptr<ClipDescriptor> ComposedClip::addClip (std::shared_ptr<AVClip> clip, double start, double length, double offset)
{
    auto clipDescriptor = std::make_shared<ClipDescriptor> (*this, clip, getUndoManager());
    clip->prepareToPlay (audioSettings.defaultNumSamples, audioSettings.timebase);

    clipDescriptor->setDescription (makeUniqueDescription (clip->getDescription()));
    clipDescriptor->setStart (start);
    clipDescriptor->setOffset (offset);
    clipDescriptor->setLength (length > 0 ? length : clip->getLengthInSeconds());

    clipDescriptor->updateSampleCounts();

    juce::ScopedLock sl (clipDescriptorLock);

    clips.push_back (clipDescriptor);

    juce::ScopedValueSetter<bool> manual (manualStateChange, true);
    state.appendChild (clipDescriptor->getStatusTree(), getUndoManager());

    clipDescriptor->getVideoParameterController().addListener (this);
    addTimecodeListener (clipDescriptor.get());

    return clipDescriptor;
}

void ComposedClip::removeClip (std::shared_ptr<ClipDescriptor> descriptor)
{
    removeTimecodeListener (descriptor.get());
    descriptor->getVideoParameterController().removeListener (this);

    auto it = std::find (clips.begin(), clips.end(), descriptor);
    if (it != clips.end())
        clips.erase (it);

    state.removeChild (descriptor->getStatusTree(), getUndoManager());
}

std::shared_ptr<ClipDescriptor> ComposedClip::getClip (int index)
{
    if (juce::isPositiveAndBelow (index, clips.size()))
        return clips [size_t (index)];

    return {};
}

bool ComposedClip::isFrameAvailable (double pts) const
{
    for (auto clip : getActiveClips ([pos = pts * getSampleRate()](ClipDescriptor& clip)
                    {
                        return clip.clip->hasVideo() && pos >= clip.start && pos < clip.start + clip.length;
                    }))
    {
        if (clip->clip->isFrameAvailable (pts - clip->getStart() + clip->getOffset()) == false)
            return false;
    }

    return true;
}

VideoFrame& ComposedClip::getFrame (double pts)
{
    auto nextTimeCode = convertTimecode (pts, videoSettings);

    if (frame.image.getWidth() != videoSettings.frameSize.width || frame.image.getHeight() != videoSettings.frameSize.height)
        frame.image = juce::Image (juce::Image::ARGB, videoSettings.frameSize.width, videoSettings.frameSize.height, true);
    else
        frame.image.clear (frame.image.getBounds());

    videoMixer->compose (frame.image, videoSettings, nextTimeCode, pts,
                         getActiveClips ([pos = pts * getSampleRate()](ClipDescriptor& clip)
    {
        return clip.clip->hasVideo() && pos >= clip.start && pos < clip.start + clip.length;
    }));

    frame.timecode = nextTimeCode;
#if FOLEYS_USE_OPENGL
    frame.upToDate = false;
#endif

    return frame;
}

Size ComposedClip::getVideoSize() const
{
    return videoSettings.frameSize;
}

double ComposedClip::getCurrentTimeInSeconds() const
{
    return convertToSamples (position.load());
}

juce::Image ComposedClip::getStillImage ([[maybe_unused]]double seconds, [[maybe_unused]]Size size)
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
    audioSettings.timebase = juce::roundToInt (sampleRateToUse);
    audioSettings.defaultNumSamples = samplesPerBlockExpected;

    audioMixer->setup (audioSettings.numChannels, audioSettings.timebase, audioSettings.defaultNumSamples);

    for (auto& descriptor : getClips())
    {
        descriptor->clip->prepareToPlay (audioSettings.defaultNumSamples, audioSettings.timebase);
        descriptor->updateSampleCounts();
    }
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

bool ComposedClip::waitForSamplesReady (int samples, int timeout)
{
    auto ready = true;
    const auto start = juce::Time::getMillisecondCounter();
    for (auto clip : getActiveClips ([pos = position.load()](ClipDescriptor& clip) { return clip.clip->hasAudio() && pos >= clip.start && pos < clip.start + clip.length; }))
    {
        ready &= clip->clip->waitForSamplesReady (samples, std::min (timeout, timeout + int (start - juce::Time::getMillisecondCounter())));
        if (ready == false)
            break;
    }

    return ready;
}

void ComposedClip::setNextReadPosition (juce::int64 samples)
{
    position.store (samples);
    for (auto& descriptor : getClips())
        descriptor->clip->setNextReadPosition (std::max (juce::int64 (samples + descriptor->offset - descriptor->start), juce::int64 (descriptor->offset)));

    lastShownFrame = 0;

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

void ComposedClip::parameterAutomationChanged (const ParameterAutomation*)
{
    invalidateVideo();
}

std::shared_ptr<AVClip> ComposedClip::createCopy (StreamTypes)
{
    auto* engine = getVideoEngine();
    if (engine == nullptr)
        return {};

    auto clipCopy = std::make_shared<ComposedClip>(*engine);
    engine->manageLifeTime (clipCopy);

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
    if (audioSettings.timebase > 0)
    {
        const auto v = hasVideo();
        auto seconds = getCurrentTimeInSeconds();
        auto count = v ? convertTimecode (seconds, videoSettings) : 0;
        if (count != lastShownFrame || lastShownFrame < 0 || v == false)
        {
            sendTimecode (count, seconds, juce::sendNotificationAsync);
            lastShownFrame = count;
        }
    }
}

double ComposedClip::convertToSamples (int64_t pos) const
{
    if (audioSettings.timebase > 0)
        return pos / double (audioSettings.timebase);

    return {};
}

void ComposedClip::valueTreePropertyChanged (juce::ValueTree&,
                                             const juce::Identifier&)
{
}

void ComposedClip::valueTreeChildAdded (juce::ValueTree&, juce::ValueTree& childWhichHasBeenAdded)
{
    if (manualStateChange)
        return;

    if (childWhichHasBeenAdded.getType() == IDs::clip)
    {
        auto descriptor = std::make_shared<ClipDescriptor>(*this, childWhichHasBeenAdded, getUndoManager());
        if (descriptor->clip != nullptr)
        {
            descriptor->clip->prepareToPlay (getDefaultBufferSize(), getSampleRate());
            descriptor->updateSampleCounts();
            descriptor->getVideoParameterController().addListener (this);
            addTimecodeListener (descriptor.get());

            juce::ScopedLock sl (clipDescriptorLock);
            clips.push_back (descriptor);
        }
    }
}

void ComposedClip::valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree& childWhichHasBeenRemoved, int)
{
    if (manualStateChange)
        return;

    if (childWhichHasBeenRemoved.getType() == IDs::clip)
    {
        for (auto it = clips.begin(); it != clips.end(); ++it)
        {
            if ((*it)->getStatusTree() == childWhichHasBeenRemoved)
            {
                (*it)->getVideoParameterController().removeListener (this);
                removeTimecodeListener (it->get());

                juce::ScopedLock sl (clipDescriptorLock);
                clips.erase (it);
                return;
            }
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

void ComposedClip::readPluginStatesIntoValueTree()
{
    for (auto clip : getClips())
        clip->readPluginStatesIntoValueTree();
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

juce::String ComposedClip::makeUniqueDescription (const juce::String& description) const
{
    int suffix = 0;
    bool needsSuffix = false;
    auto length = description.lastIndexOfChar ('#');
    auto withoutNumber = length > 0 ? description.substring (0, length - 1).trim() : description;

    for (auto clip : getClips())
    {
        auto o = clip->getDescription();
        if (o.startsWith (withoutNumber))
        {
            suffix = std::max (suffix, o.getTrailingIntValue());
            needsSuffix = true;
        }
    }

    if (needsSuffix)
        return withoutNumber + " #" + juce::String (suffix + 1);

    return description;
}


} // foleys
