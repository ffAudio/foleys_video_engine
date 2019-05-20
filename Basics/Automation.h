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
 @class AutomationParameter

 This class references a RangedAudioParameter in an AudioProcessor, allowing
 to store and playback automation values. It is used in the ClipDescriptor's
 AudioProcessorHolder.
 */
class AutomationParameter  : private juce::AudioProcessorParameter::Listener
{
public:
    AutomationParameter (ClipDescriptor::ProcessorHolder&,
                         juce::ControllableProcessorBase&,
                         juce::AudioProcessorParameter&);

    ~AutomationParameter();

    void updateProcessor (double pts);

    void setValue (double pts, double value);

    void addKeyframe (double pts, double value);

    void setKeyframe (size_t index, double pts, double value);

    juce::String getName() const;

    double getValue() const;

    const std::map<double, double>& getKeyframes() const;

    void loadFromValueTree (const juce::ValueTree& state);

    void saveToValueTree (juce::ValueTree& state, juce::UndoManager* undo) const;

    void parameterValueChanged (int parameterIndex, float newValue) override;

    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override;

private:

    double getValueForTime (double pts) const;

    ClipDescriptor::ProcessorHolder&   holder;
    juce::ControllableProcessorBase&   processor;
    juce::AudioProcessorParameter&     parameter;

    double value = 0.0;
    std::map<double, double> keyframes;
    bool gestureInProgress = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutomationParameter)
};

}
