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

FilmStrip::FilmStrip()
{
    setOpaque (true);
    setInterceptsMouseClicks (false, true);
}

FilmStrip::~FilmStrip()
{
    if (thumbnailJob != nullptr)
        if (auto* threadPool = getThreadPool())
            threadPool->removeJob (thumbnailJob.get(), true, 1000);
}

void FilmStrip::setClip (std::shared_ptr<AVClip> clipToUse)
{
    clip = clipToUse;
    if (clip != nullptr)
        aspectRatio = clip->getVideoSize().getAspectRatio();

    update();
}

void FilmStrip::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    auto target = getLocalBounds().withWidth (juce::roundToInt (getHeight() * aspectRatio));
    for (auto& image : thumbnails)
    {
        g.drawImage (image, target.toFloat());
        target.setX (target.getRight() + 1);
    }
}

void FilmStrip::resized()
{
    update();
}

void FilmStrip::setStartAndEnd (double startToUse, double endTimeToUse)
{
    startTime = startToUse;
    endTime = endTimeToUse;
    update();
}

void FilmStrip::update()
{
    if (clip == nullptr || endTime <= startTime)
        return;

    auto* threadPool = getThreadPool();
    if (threadPool == nullptr)
        return;

    if (thumbnailJob != nullptr)
        threadPool->removeJob (thumbnailJob.get(), true, 2000);

    thumbnailJob = std::make_unique<ThumbnailJob>(*this);

    threadPool->addJob (thumbnailJob.get(), false);
}

void FilmStrip::setThumbnail (int index, juce::Image image)
{
    if (index >= int (thumbnails.size()))
        thumbnails.resize (size_t (index + 1));

    thumbnails [size_t (index)] = image;

    repaint();
}

juce::ThreadPool* FilmStrip::getThreadPool()
{
    if (clip == nullptr)
        return nullptr;

    if (auto* engine = clip->getVideoEngine())
        return &engine->getThreadPool();

    return nullptr;
}

//==============================================================================

FilmStrip::ThumbnailJob::ThumbnailJob (FilmStrip& ownerToUse)
  : juce::ThreadPoolJob ("Thumbnail Reader"),
    owner (ownerToUse)
{
}

juce::ThreadPoolJob::JobStatus FilmStrip::ThumbnailJob::runJob()
{
    auto width  = owner.getWidth();
    auto height = owner.getHeight();

    if (owner.clip == nullptr || width <= 0 || height <= 0)
        return juce::ThreadPoolJob::jobHasFinished;

    Size thumbSize { int (height * owner.aspectRatio), height };
    double time = owner.startTime;
    double end  = owner.endTime;
    double step = thumbSize.width * (end - time) / width;

    int index = 0;
    while (! shouldExit() && time < end)
    {
        if (owner.clip == nullptr)
            return juce::ThreadPoolJob::jobHasFinished;

        juce::Component::SafePointer<FilmStrip> strip (&owner);
        auto image = owner.clip->getStillImage (time, thumbSize);
        juce::MessageManager::callAsync ([strip, index, image]() mutable
                                         {
                                             if (strip)
                                                 strip->setThumbnail (index, image);
                                         });

        time += step;
        ++index;
    }

    return juce::ThreadPoolJob::jobHasFinished;
}


} // foleys
