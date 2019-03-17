
namespace foleys
{

bool AVMovieClip::openFromFile (const juce::File file)
{
    backgroundJob.setSuspended (true);

    std::unique_ptr<AVReader> reader = AVFormatManager::createReaderFor (file);
    if (reader->isOpenedOk())
    {
        movieReader = std::move (reader);
        audioFifo.setNumChannels (movieReader->numChannels);
        audioFifo.setSampleRate (movieReader->sampleRate);

        videoFifo.setTimebase (movieReader->timebase);
        videoFifo.setSize (movieReader->originalSize);

        backgroundJob.setSuspended (false);

        return true;
    }

    return false;
}

AVSize AVMovieClip::getOriginalSize() const
{
    return (movieReader != nullptr) ? movieReader->originalSize : AVSize();
}

double AVMovieClip::getLengthInSeconds() const
{
    if (movieReader && movieReader->isOpenedOk())
        return movieReader->getTotalLength() / sampleRate;

    return {};
}

double AVMovieClip::getCurrentTimeInSeconds() const
{
    return sampleRate == 0 ? 0 : nextReadPosition / sampleRate;
}

AVTimecode AVMovieClip::getCurrentTimecode() const
{
    return {}; // FIXME
}

juce::Image AVMovieClip::getFrame (const AVTimecode) const
{
    return {};
}

juce::Image AVMovieClip::getCurrentFrame() const
{
    return {};
}

void AVMovieClip::prepareToPlay (int samplesPerBlockExpected, double sampleRateToUse)
{
    sampleRate = sampleRateToUse;
}

void AVMovieClip::releaseResources()
{
    sampleRate = 0;
}

void AVMovieClip::getNextAudioBlock (const juce::AudioSourceChannelInfo& info)
{
    if (movieReader && movieReader->isOpenedOk())
    {
        audioFifo.pullSamples (info);
    }
    else
    {
        info.clearActiveBufferRegion();
    }
    nextReadPosition += info.numSamples;
}

void AVMovieClip::setNextReadPosition (juce::int64 samples)
{
    backgroundJob.setSuspended (true);

    nextReadPosition = samples;
    audioFifo.setPosition (samples);
    if (movieReader)
        movieReader->setPosition (samples);

    backgroundJob.setSuspended (false);
}

juce::int64 AVMovieClip::getNextReadPosition() const
{
    return nextReadPosition;
}

juce::int64 AVMovieClip::getTotalLength() const
{
    if (movieReader && movieReader->isOpenedOk())
        return movieReader->getTotalLength();

    return 0;
}

bool AVMovieClip::isLooping() const
{
    return loop;
}

void AVMovieClip::setLooping (bool shouldLoop)
{
    loop = shouldLoop;
}

AVMovieClip::BackgroundReaderJob::BackgroundReaderJob (AVMovieClip& ownerToUse)
    : owner (ownerToUse)
{
}

int AVMovieClip::BackgroundReaderJob::useTimeSlice()
{
    if (!suspended && owner.movieReader && owner.audioFifo.getFreeSpace() > 2048)
    {
        owner.movieReader->readNewData (owner.videoFifo, owner.audioFifo);
        return 0;
    }

    return 50;
}

void AVMovieClip::BackgroundReaderJob::setSuspended (bool s)
{
    suspended = s;
}

juce::TimeSliceClient* AVMovieClip::getBackgroundJob()
{
    return &backgroundJob;
}

}
