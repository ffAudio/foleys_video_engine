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
 @class AudioStrip

 This class displays the audio curves of the clip.
 */
class AudioStrip  : public juce::Component,
                    private juce::ChangeListener
{
public:
    AudioStrip();

    /** Set the clip to be shown as thimbnail */
    void setClip (std::shared_ptr<AVClip> clip);

    /** @internal */
    void paint (juce::Graphics&) override;

    /** Set the start time and the end time of the clip in seconds. This
        is used to allow only a subset of thumbnails to be shown. */
    void setStartAndLength (double start, double length);

    /** @internal */
    class ThumbnailJob : public juce::ThreadPoolJob
    {
    public:
        ThumbnailJob (AudioStrip& owner);

        juce::ThreadPoolJob::JobStatus runJob() override;
    private:
        AudioStrip& owner;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThumbnailJob)
    };

private:

    /** @internal */
    void update();
    /** @internal */
    void changeListenerCallback (juce::ChangeBroadcaster* sender) override;

    /** @internal */
    juce::ThreadPool* getThreadPool();

    std::shared_ptr<AVClip> clip;
    double startTime = {};
    double timeLength = {};

    std::unique_ptr<ThumbnailJob> thumbnailJob;
    juce::AudioThumbnailCache cache { 1 };
    juce::AudioFormatManager  formatManager;
    juce::AudioThumbnail      thumbnail { 64, formatManager, cache };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioStrip)
};

}
