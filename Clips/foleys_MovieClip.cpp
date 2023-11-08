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

MovieClip::MovieClip (VideoEngine& engine)
  : AVClip (engine)
{
    addDefaultAudioParameters (*this);
    addDefaultVideoParameters (*this);
}

juce::String MovieClip::getDescription() const
{
    if (movieReader)
        return movieReader->getMediaFile().getFileNameWithoutExtension();

    return "MovieClip";
}

bool MovieClip::openFromFile (const juce::File file)
{
    auto* engine = getVideoEngine();
    if (engine == nullptr)
        return false;

    const auto wasSuspended = backgroundJob.isSuspended();
    backgroundJob.setSuspended (true);

    auto reader = engine->createReaderFor (file);
    if (reader->isOpenedOk())
    {
        if (reader->hasVideo())
            setThumbnailReader (engine->createReaderFor (file, StreamTypes::video()));
        else
            setThumbnailReader ({});

        setReader (std::move (reader));
        backgroundJob.setSuspended (wasSuspended);
        return true;
    }

    return false;
}

juce::URL MovieClip::getMediaFile() const
{
    if (movieReader)
        return juce::URL (movieReader->getMediaFile());

    return {};
}

void MovieClip::setReader (std::unique_ptr<AVReader> readerToUse)
{
    backgroundJob.setSuspended (true);

    movieReader = std::move (readerToUse);
    audioFifo.setNumChannels (movieReader->numChannels);
    audioFifo.setSampleRate (sampleRate);
    audioFifo.setPosition (0);

    if (sampleRate > 0)
        movieReader->setOutputSampleRate (sampleRate);

    if (hasVideo())
        videoFifo.setVideoSettings (movieReader->getVideoSettings (0));

    videoFifo.clear();

    backgroundJob.setSuspended (false);
}

void MovieClip::setThumbnailReader (std::unique_ptr<AVReader> reader)
{
    thumbnailReader = std::move (reader);
}

Size MovieClip::getVideoSize() const
{
    return (movieReader != nullptr) ? movieReader->originalSize : Size();
}

double MovieClip::getLengthInSeconds() const
{
    if (movieReader && movieReader->isOpenedOk())
        return movieReader->getTotalLength() / movieReader->sampleRate;

    return {};
}

double MovieClip::getCurrentTimeInSeconds() const
{
    return sampleRate == 0 ? 0 : nextReadPosition / sampleRate;
}

VideoFrame& MovieClip::getFrame (double pts)
{
    return videoFifo.getFrameSeconds (pts);
}

void MovieClip::render (juce::Graphics& view, juce::Rectangle<float> area, double pts, float rotation, float zoom, juce::Point<float> translation, float alpha)
{
    renderFrame (view, area, getFrame (pts), rotation, zoom, translation, alpha);
}

#if FOLEYS_USE_OPENGL
void MovieClip::render (OpenGLView& view, double pts, float rotation, float zoom, juce::Point<float> translation, float alpha)
{
    renderFrame (view, getFrame (pts), rotation, zoom, translation, alpha);
}
#endif

bool MovieClip::isFrameAvailable (double pts) const
{
    if (juce::isPositiveAndBelow (pts * movieReader->sampleRate, movieReader->getTotalLength()))
        return videoFifo.isFrameAvailable (pts);

    return true;
}

juce::Image MovieClip::getStillImage (double seconds, Size size)
{
    if (thumbnailReader && thumbnailReader->isOpenedOk())
        return thumbnailReader->getStillImage (seconds, size);

    return {};
}

void MovieClip::prepareToPlay (int, double sampleRateToUse)
{
    sampleRate = sampleRateToUse;
    audioFifo.setNumSamples (juce::roundToInt (sampleRate));
    audioFifo.setSampleRate (sampleRate);

    if (movieReader)
        movieReader->setOutputSampleRate (sampleRate);

    backgroundJob.setSuspended (false);
}

void MovieClip::releaseResources()
{
    sampleRate = 0;
}

bool MovieClip::waitForSamplesReady (int samples, int timeout)
{
    jassert (samples > 0 && samples <= 4800);

    if (movieReader && movieReader->isOpenedOk() && movieReader->hasAudio())
    {
        const auto start = juce::Time::getMillisecondCounter();

        while (audioFifo.getAvailableSamples() < samples && int (juce::Time::getMillisecondCounter() - start) < timeout)
            juce::Thread::sleep (5);

        return audioFifo.getAvailableSamples() >= samples;
    }
    else
    {
        return true;
    }
}

bool MovieClip::waitForFrameReady (double pts, int timeout)
{
    const auto start = juce::Time::getMillisecondCounter();

    while (videoFifo.isFrameAvailable (pts) == false && int (juce::Time::getMillisecondCounter() - start) < timeout)
        juce::Thread::sleep (1);

    return videoFifo.isFrameAvailable (pts);
}

void MovieClip::getNextAudioBlock (const juce::AudioSourceChannelInfo& info)
{
    const auto gain = float (juce::Decibels::decibelsToGain (getAudioParameters().at(IDs::gain)->getRealValue()));

    if (movieReader && movieReader->isOpenedOk() && movieReader->hasAudio())
    {
        audioFifo.pullSamples (info);
        info.buffer->applyGainRamp (info.startSample, info.numSamples, lastGain, gain);
    }
    else
    {
        info.clearActiveBufferRegion();
    }
    nextReadPosition += info.numSamples;
    lastGain = gain;

    triggerAsyncUpdate();
}

bool MovieClip::hasVideo() const
{
    return movieReader ? movieReader->hasVideo() : false;
}

bool MovieClip::hasAudio() const
{
    return movieReader ? movieReader->hasAudio() : false;
}

double MovieClip::getFrameDurationInSeconds() const
{
    if (movieReader.get() != nullptr)
    {
        const auto& settings = movieReader->getVideoSettings (0);
        return double (settings.defaultDuration) / double (settings.timebase);
    }

    return {};
}

std::shared_ptr<AVClip> MovieClip::createCopy (StreamTypes types)
{
    auto* engine = getVideoEngine();
    if (engine == nullptr)
        return {};

    return engine->createClipFromFile (getMediaFile(), types);
}

double MovieClip::getSampleRate() const
{
    return sampleRate;
}

int MovieClip::getNumChannels() const 
{
    return movieReader ? movieReader->numChannels : 0;
}

void MovieClip::handleAsyncUpdate()
{
    if (sampleRate > 0 && hasVideo())
    {
        auto seconds = getCurrentTimeInSeconds();
        const auto& frame = videoFifo.getFrameSeconds (seconds);
        if (frame.timecode != lastShownFrame)
        {
            sendTimecode (frame.timecode, seconds, juce::sendNotificationAsync);
            lastShownFrame = frame.timecode;
        }
    }
}

void MovieClip::setNextReadPosition (juce::int64 samples)
{
    backgroundJob.setSuspended (true);

    nextReadPosition = samples;
    audioFifo.setPosition (samples);
    if (movieReader && sampleRate > 0)
    {
        auto time = samples / sampleRate;
        if (sampleRate == movieReader->sampleRate)
        {
            movieReader->setPosition (samples);
        }
        else
        {
            movieReader->setPosition (juce::int64 (time * movieReader->sampleRate));
        }
    }

    videoFifo.clear();

    backgroundJob.setSuspended (false);
    triggerAsyncUpdate();
}

juce::int64 MovieClip::getNextReadPosition() const
{
    return nextReadPosition;
}

juce::int64 MovieClip::getTotalLength() const
{
    if (movieReader && movieReader->isOpenedOk())
        return movieReader->getTotalLength();

    return 0;
}

bool MovieClip::isLooping() const
{
    return loop;
}

void MovieClip::setLooping (bool shouldLoop)
{
    loop = shouldLoop;
}

MovieClip::BackgroundReaderJob::BackgroundReaderJob (MovieClip& ownerToUse)
    : owner (ownerToUse)
{
}

int MovieClip::BackgroundReaderJob::useTimeSlice()
{
    if (suspended == false &&
        owner.sampleRate > 0 &&
        owner.movieReader.get() != nullptr &&
        (owner.hasAudio() == false || owner.audioFifo.getFreeSpace() > 2048) &&
        (owner.hasVideo() == false || owner.videoFifo.getFreeSpace() > 3))
    {
        juce::ScopedValueSetter<bool> guard (inDecodeBlock, true);
        owner.movieReader->readNewData (owner.videoFifo, owner.audioFifo);
    }

    if (owner.movieReader.get() != nullptr)
    {
        if (owner.getNextReadPosition() >= owner.movieReader->getTotalLength())
            return 100;
    }

    double secs = 1;
    if (owner.hasAudio())
        secs = std::min (secs, owner.audioFifo.getAvailableSamples() / owner.sampleRate);

    if (owner.hasVideo())
        secs = std::min (secs, owner.videoFifo.getNumAvailableFrames() * owner.videoFifo.getFrameDurationInSeconds());

    return int (50.0 * secs);
}

void MovieClip::BackgroundReaderJob::setSuspended (bool s)
{
    suspended = s;

    while (suspended && inDecodeBlock)
        juce::Thread::sleep (5);
}

bool MovieClip::BackgroundReaderJob::isSuspended() const
{
    return suspended;
}

juce::TimeSliceClient* MovieClip::getBackgroundJob()
{
    return &backgroundJob;
}

} // foleys
