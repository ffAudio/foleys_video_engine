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
{
    composer = std::make_unique<SoftwareCompositingContext>();
}

void AVCompoundClip::addClip (std::shared_ptr<AVClip> clip, double start, double length, double offset)
{
    if (length < 0)
        length = clip->getLengthInSeconds();

    auto clipDescriptor = std::make_unique<ClipDescriptor> (clip);
    clipDescriptor->offset = offset;
    clipDescriptor->length = length;

    clips.push_back (std::move (clipDescriptor));
}


juce::Image AVCompoundClip::getFrame (const Timecode time) const
{
    return videoFifo.getVideoFrame (time.count / time.timebase);
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

    for (auto& descriptor : clips)
        descriptor->clip->prepareToPlay (samplesPerBlockExpected, sampleRate);
}

void AVCompoundClip::releaseResources()
{
    for (auto& descriptor : clips)
        descriptor->clip->releaseResources();

    sampleRate = 0;
}

void AVCompoundClip::getNextAudioBlock (const juce::AudioSourceChannelInfo& info)
{
    info.clearActiveBufferRegion();

    // TODO: Mix sources

    position.fetch_add (info.numSamples);
}

void AVCompoundClip::setNextReadPosition (juce::int64 samples)
{
    position.store (samples);
    for (auto& descriptor : clips)
        descriptor->clip->setNextReadPosition (samples + descriptor->offset - descriptor->start);
}

juce::int64 AVCompoundClip::getNextReadPosition() const
{
    return position;
}

juce::int64 AVCompoundClip::getTotalLength() const
{
    juce::int64 length = 0;
    for (auto& descriptor : clips)
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
    for (auto& descriptor : clips)
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

//==============================================================================

juce::TimeSliceClient* AVCompoundClip::getBackgroundJob()
{
    return &videoRenderJob;
}

int AVCompoundClip::ComposingThread::useTimeSlice()
{
    return 10;
}

//==============================================================================

AVCompoundClip::ClipDescriptor::ClipDescriptor (std::shared_ptr<AVClip> clipToUse)
{
    clip = clipToUse;
}

AVCompoundClip::ClipDescriptor::~ClipDescriptor()
{
    masterReference.clear();
}

} // foleys
