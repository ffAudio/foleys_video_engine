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
 This class allows storing and playing back automation values. It is used in
 the ClipDescriptor's ProcessorController.
 Using an Adapter it can handle VideoProcessors as well as AudioProcessors.
 */
class ParameterAutomation
{
public:
    ParameterAutomation (ProcessorController&);

    virtual ~ParameterAutomation() = default;

    virtual juce::String getName() const = 0;

    virtual int getNumSteps() const = 0;

    virtual void updateProcessor (double pts) = 0;

    void setValue (double value);

    void setValue (double pts, double value);

    void addKeyframe (double pts, double value);

    void setKeyframe (size_t index, double pts, double value);

    double getValueForTime (double pts) const;

    double getValue() const;

    void startAutomationGesture();
    void finishAutomationGesture();

    const std::map<double, double>& getKeyframes() const;

    void loadFromValueTree (const juce::ValueTree& state);

    void saveToValueTree (juce::ValueTree& state, juce::UndoManager* undo);

    virtual juce::String getText (float normalisedValue, int numDigits = 0) const = 0;
    virtual double getValueForText (const juce::String& text) const = 0;

protected:

    ProcessorController& controller;
    bool gestureInProgress = false;

private:
    double value = 0.0;
    std::map<double, double> keyframes;
    bool manualUpdate = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterAutomation)
};

//==============================================================================

class AudioParameterAutomation  : public ParameterAutomation,
                                  private juce::AudioProcessorParameter::Listener
{
public:
    AudioParameterAutomation (ProcessorController& controller, juce::AudioProcessorParameter& parameter);
    ~AudioParameterAutomation() override;

    juce::String getName() const override;

    int getNumSteps() const override;

    void updateProcessor (double pts) override;

    void parameterValueChanged (int parameterIndex, float newValue) override;

    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override;

    juce::String getText (float normalisedValue, int numDigits = 0) const override;
    double getValueForText (const juce::String& text) const override;

private:

    juce::AudioProcessorParameter& parameter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioParameterAutomation)
};

//==============================================================================

class VideoParameterAutomation  : public ParameterAutomation,
                                  private ProcessorParameter::Listener
{
public:
    VideoParameterAutomation (ProcessorController& controller, ProcessorParameter& parameter);
    ~VideoParameterAutomation() override;

    juce::String getName() const override;

    int getNumSteps() const override;

    void updateProcessor (double pts) override;

    void valueChanged (ProcessorParameter& parameter, double newValue) override;
    void gestureStarted (ProcessorParameter& parameter) override;
    void gestureFinished (ProcessorParameter& parameter) override;

    juce::String getText (float normalisedValue, int numDigits = 0) const override;
    double getValueForText (const juce::String& text) const override;

private:

    ProcessorParameter& parameter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VideoParameterAutomation)

};

}
