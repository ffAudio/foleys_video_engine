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

ClipRenderer::ClipRenderer (VideoEngine& engine)
  : videoEngine (engine),
    renderJob (*this)
{}

void ClipRenderer::setOutputFile (juce::File file)
{
    mediaFile = file;
}

juce::File ClipRenderer::getOutputFile() const
{
    return mediaFile;
}

void ClipRenderer::setClipToRender (std::shared_ptr<AVClip> clipToRender)
{
    clip = clipToRender;
}

void ClipRenderer::setVideoSettings (const VideoStreamSettings& settings)
{
    videoSettings = settings;
}

void ClipRenderer::setAudioSettings (const AudioStreamSettings& settings)
{
    audioSettings = settings;
}

void ClipRenderer::startRendering (bool cancelRunningJob)
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

void ClipRenderer::cancelRendering()
{
    videoEngine.getThreadPool().removeJob (&renderJob, true, 1000);
    writer.reset();

    if (onRenderingFinished)
        onRenderingFinished (false);
}

bool ClipRenderer::isRendering() const
{
    return renderJob.isRunning();
}

//==============================================================================

ClipRenderer::RenderJob::RenderJob (ClipRenderer& owner)
  : juce::ThreadPoolJob ("Bounce Job"),
    bouncer (owner)
{}

juce::ThreadPoolJob::JobStatus ClipRenderer::RenderJob::runJob()
{
    if (bouncer.writer == nullptr || bouncer.clip == nullptr)
        return juce::ThreadPoolJob::jobHasFinished;

    const auto targetVideoSettings = bouncer.videoSettings;
    const auto targetAudioSettings = bouncer.audioSettings;

    const auto totalDuration = bouncer.clip->getTotalLength();
    int64_t    audioPosition = 0;
    int64_t    videoPosition = - targetVideoSettings.defaultDuration;

    auto  targetClip = bouncer.clip;

    buffer.setSize (targetAudioSettings.numChannels, targetAudioSettings.defaultNumSamples);
    targetClip->prepareToPlay (targetAudioSettings.defaultNumSamples, targetAudioSettings.timebase);
    targetClip->setNextReadPosition (0);

    while (! shouldExit() && audioPosition < totalDuration)
    {
        juce::AudioSourceChannelInfo info (&buffer, 0, std::min (int (totalDuration - audioPosition),
                                                                 targetAudioSettings.defaultNumSamples));

        targetClip->waitForSamplesReady (info.numSamples);
        targetClip->getNextAudioBlock (info);
        juce::AudioBuffer<float> writeBuffer (buffer.getArrayOfWritePointers(),
                                              buffer.getNumChannels(),
                                              info.startSample,
                                              info.numSamples);
        bouncer.writer->pushSamples (writeBuffer);

        audioPosition += writeBuffer.getNumSamples();

        const auto secs = audioPosition / double (targetAudioSettings.timebase);
        const auto videoCount = secs * targetVideoSettings.timebase;
        if (videoCount >= videoPosition)
        {
            videoPosition += targetVideoSettings.defaultDuration;
            auto timestamp = videoCount / double (targetVideoSettings.timebase);
            while (! targetClip->isFrameAvailable (timestamp))
            {
                if (shouldExit())
                {
                    if (bouncer.onRenderingFinished)
                        bouncer.onRenderingFinished (false);

                    return juce::ThreadPoolJob::jobHasFinished;
                }

                juce::Thread::sleep (10);
            }

            auto& frame = targetClip->getFrame (timestamp);

            bouncer.writer->pushImage (videoPosition, frame.image);
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
