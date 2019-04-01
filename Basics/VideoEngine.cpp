
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
