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

namespace IDs
{
    static juce::Identifier redBrightness   { "01redBrightness" };
    static juce::Identifier redContrast     { "02redContrast" };
    static juce::Identifier redGamma        { "03redGamma" };
    static juce::Identifier greenBrightness { "04greenBrightness" };
    static juce::Identifier greenContrast   { "05greenContrast" };
    static juce::Identifier greenGamma      { "06greenGamma" };
    static juce::Identifier blueBrightness  { "07blueBrightness" };
    static juce::Identifier blueContrast    { "08blueContrast" };
    static juce::Identifier blueGamma       { "09blueGamma" };
}

class ColourCurveVideoProcessor : public VideoProcessor
{

public:
    static juce::String getPluginName() { return "Colour Curves"; }

    ParameterMap createParameters()
    {
        ParameterMap params;
        params [IDs::redBrightness] = std::make_unique<ProcessorParameterFloat> (IDs::redBrightness, "Red Brightness", juce::NormalisableRange<double> (-1.0, 1.0), 0.0);
        params [IDs::redContrast] = std::make_unique<ProcessorParameterFloat> (IDs::redContrast, "Red Contrast", juce::NormalisableRange<double> (-1.0, 1.0), 0.0);
        params [IDs::redGamma] = std::make_unique<ProcessorParameterFloat> (IDs::redGamma, "Red Gamma", juce::NormalisableRange<double> (0.1, 4.0), 1.0);
        params [IDs::greenBrightness] = std::make_unique<ProcessorParameterFloat> (IDs::greenBrightness, "Green Brightness", juce::NormalisableRange<double> (-1.0, 1.0), 0.0);
        params [IDs::greenContrast] = std::make_unique<ProcessorParameterFloat> (IDs::greenContrast, "Green Contrast", juce::NormalisableRange<double> (-1.0, 1.0), 0.0);
        params [IDs::greenGamma] = std::make_unique<ProcessorParameterFloat> (IDs::greenGamma, "Green Gamma", juce::NormalisableRange<double> (0.1, 4.0), 1.0);
        params [IDs::blueBrightness] = std::make_unique<ProcessorParameterFloat> (IDs::blueBrightness, "Blue Brightness", juce::NormalisableRange<double> (-1.0, 1.0), 0.0);
        params [IDs::blueContrast] = std::make_unique<ProcessorParameterFloat> (IDs::blueContrast, "Blue Contrast", juce::NormalisableRange<double> (-1.0, 1.0), 0.0);
        params [IDs::blueGamma] = std::make_unique<ProcessorParameterFloat> (IDs::blueGamma, "Blue Gamma", juce::NormalisableRange<double> (0.1, 4.0), 1.0);

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

    void processFrame (juce::Image& frame,
    [[maybe_unused]]int64_t count,
    [[maybe_unused]]const VideoStreamSettings& settings,
    [[maybe_unused]]double clipDuration) override
    {
        red.calculateColourMap   (*redBrightness,   *redContrast,   *redGamma);
        green.calculateColourMap (*greenBrightness, *greenContrast, *greenGamma);
        blue.calculateColourMap  (*blueBrightness,  *blueContrast,  *blueGamma);

        if (red.isLinear() && green.isLinear() && blue.isLinear())
            return;

        if (red.isLinear() == false && green.isLinear() && blue.isLinear())
            red.applyLUT (frame, 2);
        else if (red.isLinear() && green.isLinear() == false && blue.isLinear())
            green.applyLUT (frame, 1);
        else if (red.isLinear() && green.isLinear() && blue.isLinear() == false)
            blue.applyLUT (frame, 0);
        else
            ColourCurve::applyLUTs (frame, red, green, blue);
    }

    std::vector<ProcessorParameter*> getParameters() override
    {
        return state.getParameters();
    }

    void getStateInformation (juce::MemoryBlock& destData) override
    {
        juce::MemoryOutputStream stream(destData, false);
        state.state.writeToStream (stream);
    }

    void setStateInformation (const void* data, int sizeInBytes) override
    {
        auto tree = juce::ValueTree::readFromData (data, size_t (sizeInBytes));
        if (tree.isValid())
            state.state = tree;
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

} // foleys
