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

#pragma once

#if FOLEYS_USE_OPENGL

namespace foleys
{

class OpenGLView  : public juce::OpenGLAppComponent,
                    public VideoView
{
public:

    OpenGLView();
    ~OpenGLView() override;

    /**
     Set the clip to display. This is a shared ptr, so it will stay alive until you set a new one or a nullptr.
     */
    void setClip (std::shared_ptr<AVClip> clip) override;

    /**
     Access the currently played clip
     */
    std::shared_ptr<AVClip> getClip() const override;

    juce::OpenGLContext& getContext();
    juce::OpenGLTexture& getTexture (AVClip& clip, VideoFrame& frame);

    void paint (juce::Graphics& g) override;
    void render() override;
    void initialise() override;
    void shutdown() override;

    void setContinuousRepaint (int hz) override;

    void timecodeChanged (int64_t count, double seconds) override;

private:
    struct Texture
    {
        juce::OpenGLTexture texture;
        juce::int64         timestamp = -1;
    };

    std::map<AVClip*, std::unique_ptr<Texture>> textures;

    juce::CriticalSection   clipLock;
    std::shared_ptr<AVClip> clip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenGLView)
};

} // namespace foleys

#endif // FOLEYS_USE_OPENGL
