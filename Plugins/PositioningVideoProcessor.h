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

namespace IDs
{
    static juce::String scaleX { "scaleX" };
    static juce::String scaleY { "scaleY" };
    static juce::String transX { "transX" };
    static juce::String transY { "transY" };
}

class PositioningVideoProcessor : public VideoProcessor
{
public:
    static juce::String getPluginName() { return "Positioning"; }

    const juce::String getName() const override { return PositioningVideoProcessor::getPluginName(); }

    PositioningVideoProcessor()
    {
        scaleX = state.getRawParameterValue (IDs::scaleX);
        scaleY = state.getRawParameterValue (IDs::scaleY);
        transX = state.getRawParameterValue (IDs::transX);
        transY = state.getRawParameterValue (IDs::transY);
    }

    void processFrameReplacing (juce::Image& frame, int64_t count, const VideoStreamSettings& settings, double clipDuration) override
    {
        processFrame (frame, frame.createCopy(), count, settings, clipDuration);
    }

    void processFrame (juce::Image& output, const juce::Image& input, int64_t count, const VideoStreamSettings& settings, double clipDuration) override
    {
        juce::Graphics g (output);
        g.fillAll (juce::Colours::transparentBlack);

        g.drawImageTransformed (input, juce::AffineTransform::scale (*scaleX, *scaleY).translated (*transX * input.getWidth(), *transY * input.getHeight()));

    }

    juce::AudioProcessorEditor* createEditor() override     { return nullptr; }
    bool hasEditor() const override                         { return false; }

private:
    juce::UndoManager undo;
    juce::AudioProcessorValueTreeState state { *this, &undo, "PARAMETERS",
        { std::make_unique<juce::AudioParameterFloat> (IDs::scaleX, "Horiz. Scale", juce::NormalisableRange<float> (0.0f, 100.0f), 1.0f),
            std::make_unique<juce::AudioParameterFloat> (IDs::scaleY, "Vert. Scale", juce::NormalisableRange<float> (0.0f, 100.0f), 1.0f),
            std::make_unique<juce::AudioParameterFloat> (IDs::transX, "Horiz. Translation", juce::NormalisableRange<float> (-1.0f, 1.0f), 0.0f),
            std::make_unique<juce::AudioParameterFloat> (IDs::transY, "Vert. Translation", juce::NormalisableRange<float> (-1.0f, 1.0f), 0.0f)} };

    float *scaleX, *scaleY, *transX, *transY;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PositioningVideoProcessor)
};

}
