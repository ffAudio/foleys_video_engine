/*
 ==============================================================================

 Copyright (c) 2020 - 2021, Foleys Finest Audio - Daniel Walz
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

#if FOLEYS_USE_OPENGL

namespace foleys
{

OpenGLView::OpenGLView()
{
#if FOLEYS_SHOW_SPLASHSCREEN
    addSplashscreen (*this);
#endif
}

OpenGLView::~OpenGLView()
{
    shutdownOpenGL();
}

void OpenGLView::paint (juce::Graphics& g)
{
    juce::ignoreUnused (g);
}

void OpenGLView::render()
{
    juce::ScopedLock l (clipLock);
    if (clip.get() == nullptr)
    {
        juce::OpenGLHelpers::clear (juce::Colours::black);
        return;
    }

    jassert (juce::OpenGLHelpers::isContextActive());

    auto desktopScale = (float) openGLContext.getRenderingScale();
    juce::OpenGLHelpers::clear (juce::Colours::black);

    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glViewport (0, 0, juce::roundToInt (desktopScale * (float) getWidth()), juce::roundToInt (desktopScale * (float) getHeight()));

    clip->render (*this, clip->getCurrentTimeInSeconds());
}

void OpenGLView::setClip (std::shared_ptr<AVClip> clipToUse)
{
    juce::ScopedLock l (clipLock);
    clip = clipToUse;
}

std::shared_ptr<AVClip> OpenGLView::getClip() const
{
    return clip;
}

void OpenGLView::initialise()
{
    openGLContext.setImageCacheSize (64 * 1024 * 1024);
}

void OpenGLView::shutdown()
{
    textures.clear();
}

juce::OpenGLContext& OpenGLView::getContext()
{
    return openGLContext;
}

juce::OpenGLTexture& OpenGLView::getTexture (AVClip& source, VideoFrame& frame)
{
    auto& texture = textures [&source];

    if (texture.get() == nullptr)
        texture.reset (new Texture);

    if (texture->timestamp != frame.timecode)
    {
        texture->texture.loadImage (frame.image);
        texture->timestamp = frame.timecode;
    }

    return texture->texture;
}

void OpenGLView::timecodeChanged (int64_t count, double seconds)
{
    juce::ignoreUnused (count);
    juce::ignoreUnused (seconds);

}

} // namespace foleys

#endif // FOLEYS_USE_OPENGL
