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

VideoEngine::VideoEngine()
{
    const int numReaders = std::max (4, juce::SystemStats::getNumCpus());
    for (int i = 0; i < numReaders; ++i)
        readingThreads.emplace_back (std::make_unique<juce::TimeSliceThread>("Reading Thread #" + juce::String (i)));

    for (auto& reader : readingThreads)
        reader->startThread();

    startTimer (1000);
}

VideoEngine::~VideoEngine()
{
    for (auto& reader : readingThreads)
        reader->stopThread (500);

    masterReference.clear();
}

std::shared_ptr<AVClip> VideoEngine::createClipFromFile (juce::URL url, StreamTypes type)
{
    auto clip = formatManager.createClipFromFile (*this, url, type);
    if (clip)
        manageLifeTime (clip);

    return clip;
}

std::unique_ptr<AVReader> VideoEngine::createReaderFor (juce::File file, StreamTypes type)
{
    return formatManager.createReaderFor (file, type);
}

juce::TimeSliceThread& VideoEngine::getNextTimeSliceThread()
{
    jassert (!readingThreads.empty());

    int minClients = std::numeric_limits<int>::max();
    size_t nextThread = 0;
    for (size_t i=0; i < readingThreads.size(); ++i)
    {
        auto& thread = readingThreads.at (i);
        if (thread->getNumClients() <= minClients)
        {
            minClients = thread->getNumClients();
            nextThread = i;
        }
    }

    return *readingThreads [nextThread];
}

void VideoEngine::manageLifeTime (std::shared_ptr<AVClip> clip)
{
    releasePool.push_back (clip);

    auto* client = clip->getBackgroundJob();
    if (client == nullptr)
        return;

    getNextTimeSliceThread().addTimeSliceClient (client);
}

void VideoEngine::removeFromBackgroundThreads (juce::TimeSliceClient* client)
{
    for (auto& reader : readingThreads)
        reader->removeTimeSliceClient (client);
}

void VideoEngine::addJob (std::function<void()> job)
{
    jobThreads.addJob (std::move (job));
}

void VideoEngine::addJob (juce::ThreadPoolJob* job, bool deleteJobWhenFinished)
{
    jobThreads.addJob (job, deleteJobWhenFinished);
}

void VideoEngine::cancelJob (juce::ThreadPoolJob* job)
{
    jobThreads.removeJob (job, true, 100);
}

juce::ThreadPool& VideoEngine::getThreadPool()
{
    return jobThreads;
}

void VideoEngine::timerCallback()
{
    for (auto p = releasePool.begin(); p != releasePool.end();)
    {
        if (p->use_count() == 1)
        {
            if (auto* client = (*p)->getBackgroundJob())
                removeFromBackgroundThreads (client);

            p = releasePool.erase (p);
        }
        else
        {
            ++p;
        }
    }
}

juce::UndoManager* VideoEngine::getUndoManager()
{
    return undoManager;
}

void VideoEngine::setUndoManager (juce::UndoManager* undoManagerToUse)
{
    undoManager.setNonOwned (undoManagerToUse);
}

AVFormatManager& VideoEngine::getFormatManager()
{
    return formatManager;
}

AudioPluginManager& VideoEngine::getAudioPluginManager()
{
    return audioPluginManager;
}

VideoPluginManager& VideoEngine::getVideoPluginManager()
{
    return videoPluginManager;
}

juce::AudioFormatManager& VideoEngine::getAudioFormatManager()
{
    return formatManager.audioFormatManager;
}

std::unique_ptr<VideoProcessor> VideoEngine::createVideoPluginInstance (const juce::String& identifierString,
                                                                        juce::String& error) const
{
    return videoPluginManager.createVideoPluginInstance (identifierString, error);
}

std::unique_ptr<juce::AudioProcessor> VideoEngine::createAudioPluginInstance (const juce::String& identifierString,
                                                                              double sampleRate,
                                                                              int blockSize,
                                                                              juce::String& error) const
{
    return audioPluginManager.createAudioPluginInstance (identifierString, sampleRate, blockSize, error);
}


} // foleys
