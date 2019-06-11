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
    static juce::String redContrast     { "redContrast" };
    static juce::String redBrightness   { "redBrightness" };
    static juce::String redGamma        { "redGamma" };
    static juce::String greenContrast   { "greenContrast" };
    static juce::String greenBrightness { "greenBrightness" };
    static juce::String greenGamma      { "greenGamma" };
    static juce::String blueContrast    { "blueContrast" };
    static juce::String blueBrightness  { "blueBrightness" };
    static juce::String blueGamma       { "blueGamma" };
}

class ColourCurveVideoProcessor : public VideoProcessor
{

public:
    static juce::String getPluginName() { return "Colour Curves"; }

    std::vector<std::unique_ptr<ProcessorParameter>> createParameters()
    {
        std::vector<std::unique_ptr<ProcessorParameter>> params;
        params.emplace_back (std::make_unique<ProcessorParameterFloat> (IDs::redBrightness, "Red Brightness", juce::NormalisableRange<double> (-1.0, 1.0), 0.0));
        params.emplace_back (std::make_unique<ProcessorParameterFloat> (IDs::redContrast, "Red Contrast", juce::NormalisableRange<double> (-1.0, 1.0), 0.0));
        params.emplace_back (std::make_unique<ProcessorParameterFloat> (IDs::redGamma, "Red Gamma", juce::NormalisableRange<double> (0.1, 4.0), 1.0));
        params.emplace_back (std::make_unique<ProcessorParameterFloat> (IDs::greenBrightness, "Green Brightness", juce::NormalisableRange<double> (-1.0, 1.0), 0.0));
        params.emplace_back (std::make_unique<ProcessorParameterFloat> (IDs::greenContrast, "Green Contrast", juce::NormalisableRange<double> (-1.0, 1.0), 0.0));
        params.emplace_back (std::make_unique<ProcessorParameterFloat> (IDs::greenGamma, "Green Gamma", juce::NormalisableRange<double> (0.1, 4.0), 1.0));
        params.emplace_back (std::make_unique<ProcessorParameterFloat> (IDs::blueBrightness, "Blue Brightness", juce::NormalisableRange<double> (-1.0, 1.0), 0.0));
        params.emplace_back (std::make_unique<ProcessorParameterFloat> (IDs::blueContrast, "Blue Contrast", juce::NormalisableRange<double> (-1.0, 1.0), 0.0));
        params.emplace_back (std::make_unique<ProcessorParameterFloat> (IDs::blueGamma, "Blue Gamma", juce::NormalisableRange<double> (0.1, 4.0), 1.0));

        return params;
    }

    const juce::String getName() const override { return ColourCurveVideoProcessor::getPluginName(); }

    ColourCurveVideoProcessor()
    {
        redBrightness   = state.getRawParameterValue (IDs::redBrightness);
        redContrast     = state.getRawParameterValue (IDs::redContrast);
        redGamma        = state.getRawParameterValue (IDs::redGamma);
        greenBrightness = state.getRawParameterValue (IDs::greenBrightness);
        greenContrast   = state.getRawParameterValue (IDs::greenContrast);
        greenGamma      = state.getRawParameterValue (IDs::greenGamma);
        blueBrightness  = state.getRawParameterValue (IDs::blueBrightness);
        blueContrast    = state.getRawParameterValue (IDs::blueContrast);
        blueGamma       = state.getRawParameterValue (IDs::blueGamma);
    }

    void processFrame (juce::Image& frame, int64_t count, const VideoStreamSettings& settings, double clipDuration) override
    {
        red.calculateColourMap   (*redBrightness,   *redContrast,   *redGamma);
        green.calculateColourMap (*greenBrightness, *greenContrast, *greenGamma);
        blue.calculateColourMap  (*blueBrightness,  *blueContrast,  *blueGamma);

        ColourCurve::applyLUTs (frame, red, green, blue);
    }

    std::vector<ProcessorParameter*> getParameters() override
    {
        return state.getParameters();
    }

private:
    juce::UndoManager undo;
    ProcessorState state { this, &undo, "PARAMETERS", createParameters() };

    double *redContrast,   *redBrightness,   *redGamma;
    double *greenContrast, *greenBrightness, *greenGamma;
    double *blueContrast,  *blueBrightness,  *blueGamma;

    ColourCurve red, green, blue;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ColourCurveVideoProcessor)
};

}
