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

namespace foleys
{

namespace IDs
{
    static juce::Identifier value           { "Value" };
    static juce::Identifier keyframe        { "Keyframe" };
    static juce::Identifier time            { "Time" };
}

ParameterAutomation::ParameterAutomation (ProcessorController&                   controllerToUse)
  : controller (controllerToUse)
{
}

void ParameterAutomation::setValue (double valueToUse)
{
    value = valueToUse;
}

void ParameterAutomation::setValue (double pts, double newValue)
{
    if (!gestureInProgress)
        return;

    if (keyframes.empty())
    {
        value = newValue;
    }
    else
    {
        keyframes [pts] = juce::jlimit (0.0, 1.0, newValue);
    }

    if (! manualUpdate)
        controller.synchroniseState (*this);
}

void ParameterAutomation::addKeyframe (double pts, double newValue)
{
    keyframes [pts] = juce::jlimit (0.0, 1.0, newValue);

    if (! manualUpdate)
        controller.synchroniseState (*this);
}

void ParameterAutomation::setKeyframe (size_t index, double pts, double newValue)
{
    if (juce::isPositiveAndBelow (index, keyframes.size()))
    {
        auto it = std::next (keyframes.begin(), index);

        keyframes.erase (it);
        keyframes [pts] = newValue;
    }

    if (! manualUpdate)
        controller.synchroniseState (*this);
}

double ParameterAutomation::getValueForTime (double pts) const
{
    if (keyframes.empty())
        return value;

    const auto& next = keyframes.upper_bound (pts);
    if (next == keyframes.begin())
        return next->second;

    const auto& prev = std::next (next, -1);
    if (next == keyframes.cend())
        return prev->second;

    auto dy = next->first - prev->first;
    if (dy == 0.0)
        return juce::jlimit (0.0, 1.0, 0.5 * (prev->second + next->second));

    auto dx = next->second - prev->second;
    auto interpolated = prev->second + (pts - prev->first) * dx / dy;
    return juce::jlimit (0.0, 1.0, interpolated);
}

double ParameterAutomation::getValue() const
{
    return value;
}

void ParameterAutomation::startAutomationGesture()
{
    gestureInProgress = true;
}

void ParameterAutomation::finishAutomationGesture()
{
    gestureInProgress = false;
}

const std::map<double, double>& ParameterAutomation::getKeyframes() const
{
    return keyframes;
}

void ParameterAutomation::loadFromValueTree (const juce::ValueTree& state)
{
    juce::ScopedValueSetter<bool>(manualUpdate, true);

    if (state.hasProperty (IDs::value))
        value = double (state.getProperty (IDs::value));

    std::map<double, double> newKeyframes;
    for (const auto& child : state)
    {
        if (!child.hasProperty (IDs::time) || !child.hasProperty (IDs::value))
            continue;

        auto t = double (child.getProperty (IDs::time));
        auto v = double (child.getProperty (IDs::value));
        newKeyframes [t] = v;
    }

    keyframes = newKeyframes;
}

void ParameterAutomation::saveToValueTree (juce::ValueTree& state, juce::UndoManager* undo)
{
    if (manualUpdate)
        return;

    juce::ScopedValueSetter<bool>(manualUpdate, true);

    state.setProperty (IDs::value, value, undo);

    state.removeAllChildren (undo);
    for (const auto& k : keyframes)
    {
        juce::ValueTree node { IDs::keyframe };
        node.setProperty (IDs::time, k.first, undo);
        node.setProperty (IDs::value, k.second, undo);
        state.appendChild (node, undo);
    }
}

//==============================================================================

AudioParameterAutomation::AudioParameterAutomation (ProcessorController& controllerToUse,
                                                    juce::AudioProcessorParameter& parameterToUse)
  : ParameterAutomation (controllerToUse),
    parameter (parameterToUse)
{
    setValue (parameter.getDefaultValue());
    parameter.addListener (this);
}
AudioParameterAutomation::~AudioParameterAutomation()
{
    parameter.removeListener (this);
}

juce::String AudioParameterAutomation::getName() const
{
    return parameter.getName (128);
}

int AudioParameterAutomation::getNumSteps() const
{
    return parameter.getNumSteps();
}

juce::String AudioParameterAutomation::getText (float normalisedValue, int numDigits) const
{
    return parameter.getText (normalisedValue, numDigits);
}

double AudioParameterAutomation::getValueForText (const juce::String& text) const
{
    return parameter.getValueForText (text);
}

void AudioParameterAutomation::updateProcessor (double pts)
{
    if (!gestureInProgress)
        parameter.setValueNotifyingHost (getValueForTime (pts));
}

void AudioParameterAutomation::parameterValueChanged (int parameterIndex, float newValue)
{
    auto pts = controller.getOwningClipDescriptor().getCurrentPTS();
    setValue (pts, newValue);
}

void AudioParameterAutomation::parameterGestureChanged (int parameterIndex, bool gestureIsStarting)
{
    if (gestureIsStarting)
        startAutomationGesture();
    else
        finishAutomationGesture();
}

//==============================================================================

VideoParameterAutomation::VideoParameterAutomation (ProcessorController& controllerToUse,
                                                    ProcessorParameter& parameterToUse)
  : ParameterAutomation (controllerToUse),
    parameter (parameterToUse)
{
    setValue (parameter.normaliseValue (parameter.getDefaultValue()));
    parameter.addListener (this);
}
VideoParameterAutomation::~VideoParameterAutomation()
{
    parameter.removeListener (this);
}

juce::String VideoParameterAutomation::getName() const
{
    return parameter.getName();
}

int VideoParameterAutomation::getNumSteps() const
{
    return parameter.getNumSteps();
}

juce::String VideoParameterAutomation::getText (float normalisedValue, int numDigits) const
{
    return parameter.getText (normalisedValue, numDigits);
}

double VideoParameterAutomation::getValueForText (const juce::String& text) const
{
    return parameter.getValueForText (text);
}

void VideoParameterAutomation::updateProcessor (double pts)
{
    if (!gestureInProgress)
        parameter.setNormalisedValue (getValueForTime (pts));
}

void VideoParameterAutomation::valueChanged (ProcessorParameter&, double newValue)
{
    auto pts = controller.getOwningClipDescriptor().getCurrentPTS();
    setValue (pts, newValue);
}

void VideoParameterAutomation::gestureStarted (ProcessorParameter&)
{
    startAutomationGesture();
}

void VideoParameterAutomation::gestureFinished (ProcessorParameter&)
{
    finishAutomationGesture();
}


} // foleys
