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
#if FOLEYS_USE_OPENGL
    openGLcontext.setImageCacheSize (64 * 1024 * 1024);
    openGLcontext.attachTo (*this);
#endif

    setOpaque (true);
    setInterceptsMouseClicks (false, true);

#if FOLEYS_SHOW_SPLASHSCREEN
    addAndMakeVisible (splashscreen);
#endif
}

VideoPreview::~VideoPreview()
{
    if (clip)
        clip->removeTimecodeListener (this);

#if FOLEYS_USE_OPENGL
    openGLcontext.detach();
#endif

}

void VideoPreview::setClip (std::shared_ptr<AVClip> clipToUse)
{
    if (clip)
        clip->removeTimecodeListener (this);

    clip = clipToUse;

    if (clip)
        clip->addTimecodeListener (this);

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
        const auto pts = clip->getCurrentTimeInSeconds();

        if (clip->isFrameAvailable (pts) == false)
            juce::Timer::callAfterDelay (20, [&]{ repaint(); });
        else
            g.drawImage (clip->getFrame (pts).image, getLocalBounds().toFloat(), placement);
    }
}

void VideoPreview::timecodeChanged (int64_t, double)
{
    repaint();
}

} // foleys
