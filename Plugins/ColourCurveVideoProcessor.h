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
    static juce::String alphaContrast   { "alphaContrast" };
    static juce::String alphaBrightness { "alphaBrightness" };
    static juce::String alphaGamma      { "alphaGamma" };
}

class ColourCurveVideoProcessor : public VideoProcessor
{
    struct ColourCurve
    {
        double brightness = -1.0;
        double contrast   = -1.0;
        double gamma      = -1.0;

        uint8_t map[256];

        void calculateColourMap (double newBrightness, double newContrast, double newGamma)
        {
            if (newBrightness == brightness && newContrast == contrast && newGamma == gamma)
                return;

            brightness = newBrightness;
            contrast = newContrast;
            gamma = newGamma;

            for (size_t i = 0; i < 256; ++i)
                map [i] = juce::jlimit (0.0, 255.0, brightness * 127.0 + i * (contrast + 1.0));  // FIXME: exp and gamma
        }
    };

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
        params.emplace_back (std::make_unique<ProcessorParameterFloat> (IDs::alphaBrightness, "Alpha Brightness", juce::NormalisableRange<double> (-1.0, 1.0), 0.0));
        params.emplace_back (std::make_unique<ProcessorParameterFloat> (IDs::alphaContrast, "Alpha Contrast", juce::NormalisableRange<double> (-1.0, 1.0), 0.0));
        params.emplace_back (std::make_unique<ProcessorParameterFloat> (IDs::alphaGamma, "Alpha Gamma", juce::NormalisableRange<double> (0.1, 4.0), 1.0));

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
        alphaBrightness = state.getRawParameterValue (IDs::alphaBrightness);
        alphaContrast   = state.getRawParameterValue (IDs::alphaContrast);
        alphaGamma      = state.getRawParameterValue (IDs::alphaGamma);
    }

    void processFrameReplacing (juce::Image& frame, int64_t count, const VideoStreamSettings& settings, double clipDuration) override
    {
        red.calculateColourMap   (*redBrightness,   *redContrast,   *redGamma);
        green.calculateColourMap (*greenBrightness, *greenContrast, *greenGamma);
        blue.calculateColourMap  (*blueBrightness,  *blueContrast,  *blueGamma);
        alpha.calculateColourMap (*alphaBrightness, *alphaContrast, *alphaGamma);

        juce::Image::BitmapData data (frame, 0, 0,
                                      frame.getWidth(),
                                      frame.getHeight());
        if (frame.isARGB())
        {
            for (int y=0; y < frame.getHeight(); ++y)
            {
                auto* p = data.getLinePointer (y);
                for (int x=0; x < frame.getWidth(); ++x)
                {
                    *p = blue.map [*p]; ++p;
                    *p = green.map [*p]; ++p;
                    *p = red.map [*p]; ++p;
                    *p = alpha.map [*p]; ++p;
                }
            }
        }
    }

    void processFrame (juce::Image& output, const juce::Image& input, int64_t count, const VideoStreamSettings& settings, double clipDuration) override
    {
        if (output.getBounds() != input.getBounds() || input.getFormat() != output.getFormat())
            output = juce::Image (input.getFormat(), input.getWidth(), input.getHeight(), false);

        red.calculateColourMap   (*redBrightness,   *redContrast,   *redGamma);
        green.calculateColourMap (*greenBrightness, *greenContrast, *greenGamma);
        blue.calculateColourMap  (*blueBrightness,  *blueContrast,  *blueGamma);
        alpha.calculateColourMap (*alphaBrightness, *alphaContrast, *alphaGamma);

        juce::Image::BitmapData pIn (input, 0, 0,
                                     input.getWidth(),
                                     input.getHeight());
        juce::Image::BitmapData pOut (output, 0, 0,
                                      output.getWidth(),
                                      output.getHeight());
        if (input.isARGB())
        {
            for (int y=0; y < input.getHeight(); ++y)
            {
                auto* p = pIn.getLinePointer (y);
                auto* q = pOut.getLinePointer (y);

                for (int x=0; x < input.getWidth(); ++x)
                {
                    *q++ = blue.map [*p++];
                    *q++ = green.map [*p++];
                    *q++ = red.map [*p++];
                    *q++ = alpha.map [*p++];
                }
            }
        }

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
    double *alphaContrast, *alphaBrightness, *alphaGamma;

    ColourCurve red, green, blue, alpha;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ColourCurveVideoProcessor)
};

}
