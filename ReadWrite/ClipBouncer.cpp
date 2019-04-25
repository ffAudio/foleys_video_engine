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

ClipBouncer::ClipBouncer (VideoEngine& engine)
  : videoEngine (engine),
    renderJob (*this)
{}

void ClipBouncer::setOutputFile (juce::File file)
{
    mediaFile = file;
}

juce::File ClipBouncer::getOutputFile() const
{
    return mediaFile;
}

void ClipBouncer::setClipToRender (std::shared_ptr<AVClip> clipToRender)
{
    clip = clipToRender;
}

void ClipBouncer::setVideoSettings (const VideoStreamSettings& settings)
{
    videoSettings = settings;
}

void ClipBouncer::setAudioSettings (const AudioStreamSettings& settings)
{
    audioSettings = settings;
}

void ClipBouncer::startRendering (bool cancelRunningJob)
{
    if (clip == nullptr || mediaFile.getFileName().isEmpty())
        return;

    if (renderJob.isRunning())
    {
        if (cancelRunningJob)
            videoEngine.getThreadPool().removeJob (&renderJob, true, 1000);
        else
            return;
    }

    progress.store (0.0);

    clip->prepareToPlay (audioSettings.defaultNumSamples, audioSettings.timebase);

    writer = videoEngine.getFormatManager().createClipWriter (mediaFile);
    if (clip->hasVideo())
        writer->addVideoStream (videoSettings);

    if (clip->hasAudio())
        writer->addAudioStream (audioSettings);

    if (writer->startWriting())
        videoEngine.getThreadPool().addJob (&renderJob, false);
}

void ClipBouncer::cancelRendering()
{
    videoEngine.getThreadPool().removeJob (&renderJob, true, 1000);

    if (onRenderingFinished)
        onRenderingFinished (false);
}

bool ClipBouncer::isRendering() const
{
    return renderJob.isRunning();
}

//==============================================================================

ClipBouncer::RenderJob::RenderJob (ClipBouncer& owner)
  : juce::ThreadPoolJob ("Bounce Job"),
    bouncer (owner)
{}

juce::ThreadPoolJob::JobStatus ClipBouncer::RenderJob::runJob()
{
    if (bouncer.writer == nullptr || bouncer.clip == nullptr)
        return juce::ThreadPoolJob::jobHasFinished;

    const auto videoSettings = bouncer.videoSettings;
    const auto audioSettings = bouncer.audioSettings;

    const auto totalDuration = bouncer.clip->getTotalLength();
    int64_t    audioPosition = 0;
    int64_t    videoPosition = - videoSettings.defaultDuration;

    auto  clip = bouncer.clip;

    buffer.setSize (audioSettings.numChannels, audioSettings.defaultNumSamples);
    clip->prepareToPlay (audioSettings.defaultNumSamples, audioSettings.timebase);
    clip->setNextReadPosition (0);

    while (! shouldExit() && audioPosition < totalDuration)
    {
        juce::AudioSourceChannelInfo info (&buffer, 0, std::min (int (totalDuration - audioPosition),
                                                                 audioSettings.defaultNumSamples));

        clip->getNextAudioBlock (info);
        juce::AudioBuffer<float> writeBuffer (buffer.getArrayOfWritePointers(),
                                              buffer.getNumChannels(),
                                              info.startSample,
                                              info.numSamples);
        bouncer.writer->pushSamples (writeBuffer);

        audioPosition += audioSettings.defaultNumSamples;

        const auto secs = audioPosition / double (audioSettings.timebase);
        const auto videoCount = secs * videoSettings.timebase;
        if (videoCount >= videoPosition)
        {
            videoPosition += videoSettings.defaultDuration;
            auto timestamp = videoCount / double (videoSettings.timebase);
            while (! clip->isFrameAvailable (timestamp))
            {
                if (shouldExit())
                {
                    if (bouncer.onRenderingFinished)
                        bouncer.onRenderingFinished (false);

                    return juce::ThreadPoolJob::jobHasFinished;
                }

                juce::Thread::yield();
            }

            auto frame = clip->getFrame (timestamp);

            bouncer.writer->pushImage (videoPosition, frame);
        }

        bouncer.progress.store (double (audioPosition) / totalDuration);
    }

    bouncer.writer->finishWriting();
    bouncer.writer.reset();

    bouncer.progress.store (1.0);

    if (bouncer.onRenderingFinished)
        bouncer.onRenderingFinished (shouldExit() == false);

    return juce::ThreadPoolJob::jobHasFinished;
}


}
