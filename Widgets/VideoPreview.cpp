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

VideoPreview::VideoPreview()
{
    setOpaque (true);
    setInterceptsMouseClicks (false, true);
}

VideoPreview::~VideoPreview()
{
    if (clip)
    {
        clip->removeTimecodeListener (this);
        clip->removeSubtitleListener (this);
    }
}

void VideoPreview::setClip (std::shared_ptr<AVClip> clipToUse)
{
    if (clip)
    {
        clip->removeTimecodeListener (this);
        clip->removeSubtitleListener (this);
    }

    clip = clipToUse;

    if (clip)
    {
        clip->addTimecodeListener (this);
        clip->addSubtitleListener (this);
    }
    repaint();
}

std::shared_ptr<AVClip> VideoPreview::getClip() const
{
    return clip;
}

void VideoPreview::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    if (clip)
    {
        auto image = clip->getCurrentFrame();
        if (image.isNull())
            juce::Timer::callAfterDelay (200, [&]{ repaint(); });
        else
            g.drawImage (image, getLocalBounds().toFloat(), placement);
    }

    if (subtitle.isNotEmpty())
    {
        g.drawFittedText (subtitle,
                          getLocalBounds().withTop (getHeight() * 0.9),
                          juce::Justification::centred, 3);
    }
}

void VideoPreview::timecodeChanged (Timecode tc)
{
    if (tc.count > subtitleClear.count)
    {
        subtitle.clear();
        subtitleClear.count = 0;
    }

    if (currentFrameCount == -1 || currentFrameCount != tc.count)
    {
        currentFrameCount = tc.count;
        repaint();
    }
}

void VideoPreview::setSubtitle (const juce::String& text, Timecode until)
{
    subtitle = text;
    subtitleClear = until;
}

} // foleys
