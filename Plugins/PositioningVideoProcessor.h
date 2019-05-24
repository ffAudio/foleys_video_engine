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
    static juce::String zoom     { "zoom" };
    static juce::String aspect   { "aspect" };
    static juce::String rotation { "rotation" };
    static juce::String transX   { "transX" };
    static juce::String transY   { "transY" };
}

class PositioningVideoProcessor : public VideoProcessor
{
public:
    static juce::String getPluginName() { return "Positioning"; }

    std::vector<std::unique_ptr<ProcessorParameter>> createParameters()
    {
        std::vector<std::unique_ptr<ProcessorParameter>> params;
        params.emplace_back (std::make_unique<ProcessorParameterFloat> (IDs::zoom, "Zoom", juce::NormalisableRange<double> (0.0, 100.0), 1.0));
        params.emplace_back (std::make_unique<ProcessorParameterFloat> (IDs::aspect, "Aspect Ratio", juce::NormalisableRange<double> (0.0, 2.0), 1.0));
        params.emplace_back (std::make_unique<ProcessorParameterFloat> (IDs::rotation, "Rotation", juce::NormalisableRange<double> (-360.0, 360.0), 0.0));
        params.emplace_back (std::make_unique<ProcessorParameterFloat> (IDs::transX, "Horiz. Translation", juce::NormalisableRange<double> (-1.0, 1.0), 0.0));
        params.emplace_back (std::make_unique<ProcessorParameterFloat> (IDs::transY, "Vert. Translation", juce::NormalisableRange<double> (-1.0, 1.0), 0.0));
        return params;
    }

    const juce::String getName() const override { return PositioningVideoProcessor::getPluginName(); }

    PositioningVideoProcessor()
    {
        zoom     = state.getRawParameterValue (IDs::zoom);
        aspect   = state.getRawParameterValue (IDs::aspect);
        rotation = state.getRawParameterValue (IDs::rotation);
        transX   = state.getRawParameterValue (IDs::transX);
        transY   = state.getRawParameterValue (IDs::transY);
    }

    void processFrameReplacing (juce::Image& frame, int64_t count, const VideoStreamSettings& settings, double clipDuration) override
    {
        processFrame (frame, frame.createCopy(), count, settings, clipDuration);
    }

    void processFrame (juce::Image& output, const juce::Image& input, int64_t count, const VideoStreamSettings& settings, double clipDuration) override
    {
        output = juce::Image (juce::Image::ARGB, settings.frameSize.width, settings.frameSize.height, true);
        juce::Graphics g (output);
        auto scaleX = *zoom;
        auto scaleY = *zoom;
        if (*aspect < 1.0f)
            scaleX *= *aspect;
        else if (*aspect > 1.0f)
            scaleY *= 2.0f - *aspect;

        g.drawImageTransformed (input, juce::AffineTransform::rotation (*rotation * juce::MathConstants<float>::pi / 180.0f, output.getWidth() * 0.5f, output.getHeight() * 0.5f)
                                .translated (*transX * output.getWidth(), *transY * output.getHeight())
                                .scaled (scaleX, scaleY));
    }

    std::vector<ProcessorParameter*> getParameters() override
    {
        return state.getParameters();
    }

private:
    juce::UndoManager undo;
    ProcessorState state { this, &undo, "PARAMETERS", createParameters() };

    double *zoom, *aspect, *rotation, *transX, *transY;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PositioningVideoProcessor)
};

}
