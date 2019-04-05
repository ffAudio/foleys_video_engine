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

class VideoPreview  : public juce::Component,
                      public AVClip::TimecodeListener,
                      public AVClip::SubtitleListener
{
public:
    VideoPreview();

    virtual ~VideoPreview();

    void setClip (AVClip* clip);

    AVClip* getClip() const;

    void paint (juce::Graphics& g) override;

    void timecodeChanged (Timecode tc) override;

    void setSubtitle (const juce::String& text, Timecode until) override;

private:
    juce::WeakReference<AVClip> clip;
    juce::RectanglePlacement placement { juce::RectanglePlacement::centred };

    juce::int64  currentFrameCount = -1;
    juce::String subtitle;
    Timecode subtitleClear;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VideoPreview)
};

} // foleys
