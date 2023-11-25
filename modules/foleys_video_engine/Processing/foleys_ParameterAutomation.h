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

#include "foleys_ProcessorParameter.h"

namespace foleys
{

class ControllableBase;
class ProcessorController;

/**
 This class allows storing and playing back automation values. It is used in
 the ClipDescriptor's ProcessorController.
 Using an Adapter it can handle VideoProcessors as well as AudioProcessors.
 */
class ParameterAutomation  : private juce::ValueTree::Listener
{
public:
    /**
     The ParameterAutomation holds the information about keyframes / automation
     points and takes care of updating the processor values.
     */
    ParameterAutomation (ControllableBase&,
                         double defaultValue,
                         const juce::ValueTree& state,
                         juce::UndoManager*);

    virtual juce::String getName() const = 0;
    virtual int getParameterIndex() const = 0;

    virtual int getNumSteps() const = 0;
    virtual juce::StringArray getAllValueStrings() const = 0;

    /**
     This updates the parameter to match the state at a certain timepoint.
     Call this before you process a block.
     */
    virtual void updateProcessor (double pts) = 0;

    /**
     This sets the value of the parameter.
     Note as soon as there is a keyframe on the automation, this has no more effect.
     */
    void setValue (double value);

    /**
     Set the value at a certain timepoint.
     If there was no keyframe recorded yet, this will just call setValue.
     If there was a keyframe anywhere on the automation, this will add a keyframe at this position.
     */
    void setValue (double pts, double value);

    /**
     This will add a keyframe at the given position.
     */
    void addKeyframe (double pts, double value);

    /**
     Set the value from an unnormalised value.
     */
    virtual void setRealValue (double value) = 0;

    /**
     Set the value from an unnormalised value at a certain timepoint.
     If there was no keyframe recorded yet, this will just call setValue.
     If there was a keyframe anywhere on the automation, this will add a keyframe at this position.
     */
    virtual void setRealValue (double pts, double value) = 0;

    /**
     This will add a keyframe with an unnormalised value at the given position.
     */
    virtual void addRealKeyframe (double pts, double value) = 0;

    void setKeyframe (int index, double pts, double value);
    void deleteKeyframe (int index);

    /**
     Returns the normalised value at a certain time.
     */
    double         getValueForTime (double pts) const;

    /**
     Returns the unnormalised value at a certain time.
     */
    virtual double getRealValueForTime (double pts) const = 0;

    double getValue() const;

    double getPreviousKeyframeTime (double time) const;
    double getNextKeyframeTime (double time) const;

    /**
     Call this before calling setValue commands (e.g. from the processor editor)
     to avoid conflicting information from the currently playing automation
     */
    void startAutomationGesture();

    /**
     Call this to finish user interaction to give back controll to the
     playing automation.
     */
    void finishAutomationGesture();

    /**
     Give reading access to the keyframes
     */
    const std::map<double, double>& getKeyframes() const;

    /**
     Replace all keyframes
     */
    void  setKeyframes (std::map<double, double> keys);

    /**
     Replace all keyframes specifying the value in unnormalised values
     */
    virtual void  setKeyframesWithRealValues (std::map<double, double> keys) = 0;

    virtual juce::String getText (float normalisedValue, int numDigits = 2) const = 0;
    virtual double getValueForText (const juce::String& text) const = 0;

    virtual juce::NamedValueSet& getParameterProperties() = 0;

    virtual bool isVideoParameter() { return false; }
    virtual bool isAudioParameter() { return false; }

    ControllableBase& getControllable();

protected:

    ControllableBase& controllable;
    bool gestureInProgress = false;

private:

    void loadFromValueTree();
    void sortKeyframesInValueTree();

    /** @internal */
    void valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
                                   const juce::Identifier& property) override;

    /** @internal */
    void valueTreeChildAdded (juce::ValueTree& parentTree,
                              juce::ValueTree& childWhichHasBeenAdded) override;

    /** @internal */
    void valueTreeChildRemoved (juce::ValueTree& parentTree,
                                juce::ValueTree& childWhichHasBeenRemoved,
                                int indexFromWhichChildWasRemoved) override;

    /** @internal */
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override {}

    /** @internal */
    void valueTreeParentChanged (juce::ValueTree&) override {}

    juce::ValueTree state;
    juce::UndoManager* undoManager = nullptr;

    juce::CachedValue<double> value;
    std::map<double, double> keyframes;
    bool manualUpdate = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterAutomation)
};

//==============================================================================

/** Controls the AudioProcessorParameters of an AudioProcessor */
class AudioParameterAutomation  : public ParameterAutomation,
                                  private juce::AudioProcessorParameter::Listener
{
public:
    AudioParameterAutomation (ProcessorController& controller,
                              juce::AudioProcessorParameter& parameter,
                              const juce::ValueTree& state,
                              juce::UndoManager*);

    ~AudioParameterAutomation() override;

    juce::String getName() const override;
    int getParameterIndex() const override;

    juce::NamedValueSet& getParameterProperties() override;

    int getNumSteps() const override;
    juce::StringArray getAllValueStrings() const override;

    void updateProcessor (double pts) override;
    double getRealValueForTime (double pts) const override;

    void setRealValue (double value) override;
    void setRealValue (double pts, double value) override;
    void addRealKeyframe (double pts, double value) override;

    /**
     Replace all keyframes specifying the value in unnormalised values
     */
    void setKeyframesWithRealValues (std::map<double, double> keys) override;

    void parameterValueChanged (int parameterIndex, float newValue) override;

    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override;

    juce::String getText (float normalisedValue, int numDigits = 0) const override;
    double getValueForText (const juce::String& text) const override;

    bool isAudioParameter() override { return true; }

private:

    juce::AudioProcessorParameter& parameter;
    juce::NamedValueSet            properties;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioParameterAutomation)
};

//==============================================================================

/** Controls the ProcessorParameters of a VideoProcessor */
class VideoParameterAutomation  : public ParameterAutomation,
                                  private ProcessorParameter::Listener
{
public:
    VideoParameterAutomation (ControllableBase& controller,
                              ProcessorParameter& parameter,
                              const juce::ValueTree& state,
                              juce::UndoManager*);

    ~VideoParameterAutomation() override;

    juce::String getName() const override;
    int getParameterIndex() const override;

    juce::NamedValueSet& getParameterProperties() override;

    int getNumSteps() const override;
    juce::StringArray getAllValueStrings() const override;

    void updateProcessor (double pts) override;
    double getRealValueForTime (double pts) const override;

    void setRealValue (double value) override;
    void setRealValue (double pts, double value) override;
    void addRealKeyframe (double pts, double value) override;

    /**
     Replace all keyframes specifying the value in unnormalised values
     */
    void setKeyframesWithRealValues (std::map<double, double> keys) override;

    void valueChanged (ProcessorParameter& parameter, double newValue) override;
    void gestureStarted (ProcessorParameter& parameter) override;
    void gestureFinished (ProcessorParameter& parameter) override;

    juce::String getText (float normalisedValue, int numDigits = 0) const override;
    double getValueForText (const juce::String& text) const override;

    bool isVideoParameter() override { return true; }

private:

    ProcessorParameter& parameter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VideoParameterAutomation)

};

} // foleys
