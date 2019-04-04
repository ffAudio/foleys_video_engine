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

JUCE_IMPLEMENT_SINGLETON (VideoEngine)

VideoEngine::VideoEngine()
{
    for (int i = 0; i < juce::SystemStats::getNumCpus(); ++i)
        readingThreads.emplace_back (std::make_unique<juce::TimeSliceThread>("Reading Thread #" + juce::String (i)));

    for (auto& reader : readingThreads)
        reader->startThread();

}

VideoEngine::~VideoEngine()
{
    for (auto& reader : readingThreads)
        reader->stopThread (500);

    clearSingletonInstance();
}

void VideoEngine::addAVClip (AVClip& clip)
{
    auto* client = clip.getBackgroundJob();
    if (client == nullptr)
        return;

    int minClients = std::numeric_limits<int>::max();
    int nextThread = 0;
    for (int i=0; i < readingThreads.size(); ++i)
    {
        auto& thread = readingThreads.at (i);
        if (thread->getNumClients() <= minClients)
        {
            minClients = thread->getNumClients();
            nextThread = i;
        }
    }
    readingThreads [nextThread]->addTimeSliceClient (client);
}

void VideoEngine::removeAVClip (AVClip& clip)
{
    if (auto* client = clip.getBackgroundJob())
        for (auto& reader : readingThreads)
            reader->removeTimeSliceClient (client);
}

}
