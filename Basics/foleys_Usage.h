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

#pragma once

namespace foleys
{

/**
 This class sends a single, anonymous ping to our analytics collection.
 To use foleys_video_engine using the free "Personal License", you must keep this
 in your code and avoid anything, that might render this data collection ineffective.

 If you want to opt-out, you will have to buy one of the paid licenses.
 */
class UsageReporter : public juce::ThreadPoolJob
{
public:
    UsageReporter (const juce::String& event = "appStarted");

    juce::ThreadPoolJob::JobStatus runJob() override;

    static juce::StringPairArray createUsageData (const juce::String& event);

private:
    juce::String userAgent;
    juce::URL url;
    juce::String headers;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UsageReporter)
};

class FoleysSplashScreen  : public juce::Component
{
public:
    FoleysSplashScreen();
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    std::unique_ptr<juce::Drawable> foleys;
    juce::Image logo;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FoleysSplashScreen)
};

}
