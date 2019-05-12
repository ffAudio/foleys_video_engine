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
 @class PanningAudioProcessor

 This class provides a simple stereo panner. You can use it to add in as default
 for each AVClip when it is added to a ComposedClip. This allows to use this for
 simple stereo setups, or to use your own processor instead, if you want more
 versatile panning methods.
 */
class PanningAudioProcessor : public juce::AudioProcessor
{
    static inline juce::String paramGain { "Gain" };
    static inline juce::String paramPan  { "Pan" };

public:

    static juce::String getPluginName() { return "Panning"; }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
        parameters.emplace_back (std::make_unique<juce::AudioParameterFloat> (paramGain, NEEDS_TRANS ("Gain"),
                                                                              juce::NormalisableRange<float>(-80.0, 0.0, 0.1),
                                                                              0.0f,
                                                                              NEEDS_TRANS ("Gain"),
                                                                              juce::AudioProcessorParameter::genericParameter,
                                                                              [](float value, int) { return juce::String (value, 1) + " dB"; },
                                                                              [](const juce::String& text) { return text.dropLastCharacters (3).getFloatValue(); } ));

        parameters.emplace_back (std::make_unique<juce::AudioParameterFloat> (paramPan, NEEDS_TRANS ("Panning"),
                                                                              juce::NormalisableRange<float>(-1.0, 1.0, 0.01),
                                                                              0.0f,
                                                                              NEEDS_TRANS ("Panning"),
                                                                              juce::AudioProcessorParameter::genericParameter,
                                                                              [](float value, int) { return value == 0.0 ? "Center" :
                                                                                  value < 0.0 ? "L " + juce::String (juce::roundToInt (-value * 100.0f)) :
                                                                                  juce::String (juce::roundToInt (value * 100.0f)) + " R"; },
                                                                              [](const juce::String& text) { return text.startsWith ("L ") ?
                                                                                  text.substring (2).getFloatValue() * -100.0f :
                                                                                  text.endsWith (" R") ? text.dropLastCharacters (2).getFloatValue() * 100.0f : 0.0f; } ));

        return { parameters.begin(), parameters.end() };
    }

    PanningAudioProcessor()  : juce::AudioProcessor (BusesProperties()
                                                     .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    {
        gain = state.getRawParameterValue (paramGain);
        pan  = state.getRawParameterValue (paramPan);
    }

    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override
    {
        lastGains.resize (getMainBusNumOutputChannels(), 0.0f);
    }

    void processBlock (juce::AudioBuffer<float>& buffer,
                       juce::MidiBuffer& midiMessages) override
    {
        const int numIn  = getMainBusNumInputChannels();
        const int numOut = getMainBusNumOutputChannels();

        // Did you call prepareToPlay?
        jassert (lastGains.size() >= numOut);

        if (numOut == 2 && numIn == 1)
            buffer.copyFrom (1, 0, buffer.getReadPointer (0), buffer.getNumSamples());

        // TODO: add multi channel matrix here

        const auto cosine = std::cos (*pan * juce::MathConstants<float>::halfPi);
        const auto g = juce::Decibels::decibelsToGain (*gain);
        const auto left  = g * ((*pan > 0.0f) ? cosine : 1.0f);
        const auto right = g * ((*pan < 0.0f) ? cosine : 1.0f);

        buffer.applyGainRamp (0, 0, buffer.getNumSamples(), lastGains [0], left);
        buffer.applyGainRamp (1, 0, buffer.getNumSamples(), lastGains [1], right);

        lastGains [0] = left;
        lastGains [1] = right;
    }

    bool isBusesLayoutSupported (const juce::AudioProcessor::BusesLayout& layout) const override
    {
        const auto numIn  = layout.getMainInputChannels();
        const auto numOut = layout.getMainOutputChannels();

        if ((numIn > 0 && numIn <= 2) && (numOut == 2))
            return true;

        return false;
    }

    double getTailLengthSeconds() const override { return 0.0f; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }

    const juce::String getName() const override { return getPluginName(); }
    void releaseResources() override {}
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const juce::String getProgramName (int index) override { return "Default"; }
    void changeProgramName (int index, const juce::String& newName) override {}

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
    juce::UndoManager                  undo;
    juce::AudioProcessorValueTreeState state { *this, &undo, "Panning", createParameterLayout() };

    float* gain = nullptr;
    float* pan  = nullptr;
    std::vector<float> lastGains;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PanningAudioProcessor)
};

} // foleys
