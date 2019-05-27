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

class ClipRenderer
{
public:
    ClipRenderer (VideoEngine& engine);

    void setOutputFile (juce::File file);
    juce::File getOutputFile() const;

    void setClipToRender (std::shared_ptr<AVClip> clip);

    void setVideoSettings (const VideoStreamSettings& settings);
    void setAudioSettings (const AudioStreamSettings& settings);

    void startRendering (bool cancelRunningJob);
    void cancelRendering();
    bool isRendering() const;

    std::function<void(bool success)> onRenderingFinished;
    std::atomic<double> progress {};

private:

    class RenderJob : public juce::ThreadPoolJob
    {
    public:
        RenderJob (ClipRenderer& owner);
        juce::ThreadPoolJob::JobStatus runJob() override;

    private:
        ClipRenderer& bouncer;
        juce::AudioBuffer<float> buffer;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RenderJob)
    };

    VideoEngine&              videoEngine;

    VideoStreamSettings videoSettings;
    AudioStreamSettings audioSettings;

    juce::File                mediaFile;
    std::unique_ptr<AVWriter> writer;
    std::shared_ptr<AVClip>   clip;

    RenderJob renderJob;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipRenderer)
};

}
