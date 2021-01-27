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

#pragma once

namespace foleys
{

class VideoView
{
public:
    VideoView() = default;
    virtual ~VideoView() = default;

    virtual void setClip (std::shared_ptr<AVClip> clip) = 0;

    virtual std::shared_ptr<AVClip> getClip() const = 0;

#if FOLEYS_SHOW_SPLASHSCREEN
protected:
    void addSplashscreen (juce::Component& view)
    {
        view.addAndMakeVisible (splashscreen);
    }

    void viewResized (juce::Component& view)
    {
        splashscreen.setBounds (view.getWidth() - 210, view.getHeight() - 90, 200, 80);
    }

private:
    FoleysSplashScreen splashscreen;
#endif
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VideoView)
};

} // namespace foleys
