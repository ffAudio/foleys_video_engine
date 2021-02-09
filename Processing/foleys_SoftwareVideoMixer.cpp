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

void SoftwareVideoMixer::compose (juce::Image&        target,
                                  VideoStreamSettings settings,
                                  int64_t             count,
                                  double              timeInSeconds,
                                  const std::vector<std::shared_ptr<ClipDescriptor>>& clips)
{
    juce::ignoreUnused (count);

    juce::Graphics g (target);
    g.fillAll (juce::Colours::black);

    const auto renderStart = juce::Time::getMillisecondCounter();
    const auto timeout = 1000;

    for (const auto& clip : clips)
    {
        const auto clipTime = timeInSeconds + clip->getOffset() - clip->getStart();

        clip->updateAudioAutomations (timeInSeconds - clip->getStart());

        if (clip->getVideoVisible() == false)
            continue;

        const auto alpha    = float (clip->getVideoParameterController().getValueAtTime (IDs::alpha,  clipTime, 1.0));
        const auto zoom     = clip->getVideoParameterController().getValueAtTime (IDs::zoom,   clipTime, 1.0);
        const auto transX   = clip->getVideoParameterController().getValueAtTime (IDs::translateX, clipTime, 0.0);
        const auto transY   = clip->getVideoParameterController().getValueAtTime (IDs::translateY, clipTime, 0.0);
        const auto rotation = clip->getVideoParameterController().getValueAtTime (IDs::rotation, clipTime, 0.0);

        if (clip->clip->waitForFrameReady (clipTime, std::min (timeout, int (juce::Time::getMillisecondCounter() + timeout - renderStart))) == false)
            continue;

        auto frame = clip->clip->getFrame (clipTime).image;

        auto factor = std::min (double (target.getWidth()) / frame.getWidth(),
                                      double (target.getHeight()) / frame.getHeight());
        if (zoom > 100.0)
            factor *= std::pow (2.0, (zoom - 100.0) / 50.0);
        else if (zoom > 0.0)
            factor *= zoom / 100.0;
        else
            factor = 0.0;

        const auto w = frame.getWidth() * factor;
        const auto h = frame.getHeight() * factor;

        if (frame.isNull() || w < 1 || h < 1)
            continue;

        for (const auto& controller : clip->getVideoProcessors())
        {
            if (controller->isActive() == false)
                continue;

            controller->updateAutomation ((timeInSeconds - clip->getStart()) + clip->getOffset());
            if (auto* videoProcessor = controller->getVideoProcessor())
                videoProcessor->processFrame (frame, count, settings, clip->getLength());
        }

        juce::Graphics::ScopedSaveState save (g);
        auto posX = (settings.frameSize.width - w) * 0.5 + transX * w;
        auto posY = (settings.frameSize.height - h)  * 0.5 - transY * h;
        g.setOpacity (alpha);
        if (rotation != 0)
            g.addTransform (juce::AffineTransform::rotation (float (rotation * juce::MathConstants<double>::pi / 180.0),
                                                             float (posX + w * 0.5),
                                                             float (posY + h * 0.5)));

        auto area = juce::Rectangle<double>(posX, posY, w, h).toNearestInt();
        g.drawImageWithin (frame,
                           area.getX(), area.getY(), area.getWidth(), area.getHeight(),
                           juce::RectanglePlacement (juce::RectanglePlacement::centred));
    }
}

} // foleys
