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

void SoftwareVideoMixer::compose (juce::Image&        target,
                                  VideoStreamSettings settings,
                                  int64_t             count,
                                  double              timeInSeconds,
                                  const std::vector<std::shared_ptr<ClipDescriptor>>& clips)
{
    juce::ignoreUnused (count);

    juce::Graphics g (target);
    g.fillAll (juce::Colours::black);

    for (const auto& clip : clips)
    {
        const auto clipTime = timeInSeconds + clip->getOffset() - clip->getStart();
        auto frame = clip->clip->getFrame (clipTime).second;

        if (frame.isNull())
            continue;

        auto factor = std::min (double (target.getWidth()) / frame.getWidth(),
                                double (target.getHeight()) / frame.getHeight());
        frame = frame.rescaled (frame.getWidth() * factor, frame.getHeight() * factor);

        for (const auto& controller : clip->getVideoProcessors())
        {
            controller->updateAutomation ((timeInSeconds - clip->getStart()) + clip->getOffset());
            if (auto* videoProcessor = controller->getVideoProcessor())
                videoProcessor->processFrameReplacing (frame, count, settings, clip->getLength());
        }

        g.drawImageAt (frame, 0, 0);
    }
}

} // foleys
