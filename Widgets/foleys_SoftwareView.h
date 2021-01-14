/*
 ==============================================================================

 Copyright (c) 2019-2020, Foleys Finest Audio - Daniel Walz
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

namespace foleys
{

/**
 @class VideoPreview

 The VideoPreview is a juce Component, that will display the connected AVClip in
 real time.
 */
class SoftwareView  : public juce::Component,
                      public VideoView,
                      public TimeCodeAware::Listener
{
public:
    SoftwareView();

    ~SoftwareView() override;

    void setClip (std::shared_ptr<AVClip> clip) override;

    std::shared_ptr<AVClip> getClip() const override;

    void paint (juce::Graphics& g) override;

    void timecodeChanged (int64_t count, double seconds) override;

#if FOLEYS_SHOW_SPLASHSCREEN
    void resized() override
    {
        viewResized (*this);
    }
#endif

private:
    std::shared_ptr<AVClip> clip;
    juce::RectanglePlacement placement { juce::RectanglePlacement::centred };

#if FOLEYS_USE_OPENGL
    juce::OpenGLContext openGLcontext;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoftwareView)
};

} // foleys
