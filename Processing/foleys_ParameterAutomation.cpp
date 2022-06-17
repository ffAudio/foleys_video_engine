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

namespace foleys
{

namespace IDs
{
    static juce::Identifier keyframe        { "Keyframe" };
    static juce::Identifier time            { "Time" };
}

ParameterAutomation::ParameterAutomation (ControllableBase& controllerToUse,
                                          double defaultValue,
                                          const juce::ValueTree& stateToUse,
                                          juce::UndoManager* undo)
  : controllable (controllerToUse),
    state (stateToUse),
    undoManager (undo)
{
    value.referTo (state, IDs::value, undoManager, defaultValue);
    loadFromValueTree();
    state.addListener (this);
}

void ParameterAutomation::setValue (double valueToUse)
{
    value = juce::jlimit (0.0, 1.0, valueToUse);
    controllable.notifyParameterAutomationChange (this);
}

void ParameterAutomation::setValue (double pts, double newValue)
{
    if (!gestureInProgress)
        return;

    if (keyframes.empty())
        setValue (newValue);
    else
        addKeyframe (pts, newValue);
}

void ParameterAutomation::addKeyframe (double pts, double newValue)
{
    if (pts < 0.0)
        return;

    auto child = state.getChildWithProperty (IDs::time, pts);
    if (child.isValid())
    {
        child.setProperty (IDs::value, juce::jlimit (0.0, 1.0, newValue), nullptr);
    }
    else
    {
        juce::ValueTree keyframe (IDs::keyframe);
        keyframe.setProperty (IDs::time, pts, nullptr);
        keyframe.setProperty (IDs::value, juce::jlimit (0.0, 1.0, newValue), nullptr);
        state.addChild (keyframe, -1, undoManager);
    }

    sortKeyframesInValueTree();
    loadFromValueTree();

    controllable.notifyParameterAutomationChange (this);
}

void ParameterAutomation::setKeyframe (int index, double pts, double newValue)
{
    if (pts < 0.0)
        return;

    auto key = state.getChild (index);
    if (key.isValid())
    {
        key.setProperty (IDs::time, pts, undoManager);
        key.setProperty (IDs::value, juce::jlimit (0.0, 1.0, newValue), undoManager);
        sortKeyframesInValueTree();
        loadFromValueTree();
    }

    controllable.notifyParameterAutomationChange (this);
}

void ParameterAutomation::deleteKeyframe (int index)
{
    state.removeChild (index, undoManager);
    loadFromValueTree();

    controllable.notifyParameterAutomationChange (this);
}

double ParameterAutomation::getValueForTime (double pts) const
{
    if (keyframes.empty())
        return getValue();

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
    return value.get();
}

double ParameterAutomation::getPreviousKeyframeTime (double time) const
{
    if (keyframes.empty())
        return time;

    if (time <= keyframes.begin()->first)
        return time;

    const auto& next = keyframes.upper_bound (time);
    if (next == keyframes.begin())
        return time;

    const auto& prev = std::next (next, -1);
    if (prev->first == time && prev != keyframes.begin())
        return std::next (prev, -1)->first;

    return time;
}

double ParameterAutomation::getNextKeyframeTime (double time) const
{
    auto next = keyframes.upper_bound (time);
    if (next == keyframes.end())
        return time;

    if (next->first == time)
		next = std::next (next);

    if (next == keyframes.end())
        return time;

    return next->first;
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

void ParameterAutomation::setKeyframes (std::map<double, double> keys)
{
    if (undoManager)
        undoManager->beginNewTransaction();

    int index = 0;
    for (auto& k : keys)
    {
        if (state.getNumChildren() > index)
        {
            auto tree = state.getChild (index);
            tree.setProperty (IDs::time, k.first, undoManager);
            tree.setProperty (IDs::value, k.second, undoManager);
        }
        else
        {
            state.appendChild ({IDs::keyframe, {
                { IDs::time, k.first },
                { IDs::value, k.second }
            }}, undoManager);
        }
        ++index;
    }

    while (state.getNumChildren() > index)
        state.removeChild (index, undoManager);

    loadFromValueTree();
}

void ParameterAutomation::loadFromValueTree()
{
    juce::ScopedValueSetter<bool>(manualUpdate, true);

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
    controllable.notifyParameterAutomationChange (this);
}

struct KeyframeComparator
{
    int compareElements (const juce::ValueTree& first, const juce::ValueTree& second)
    {
        if (first.hasProperty (IDs::time) == false)
            return 0;

        if (second.hasProperty (IDs::time) == false)
            return 0;

        auto t1 = double (first.getProperty (IDs::time));
        auto t2 = double (second.getProperty (IDs::time));

        if (t1 < t2) return -1;
        if (t1 > t2) return 1;
        return 0;
    }
};

void ParameterAutomation::sortKeyframesInValueTree()
{
    KeyframeComparator comp;
    state.sort (comp, undoManager, true);
}

void ParameterAutomation::valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&)
{
    loadFromValueTree();
}

void ParameterAutomation::valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&)
{
    loadFromValueTree();
}

void ParameterAutomation::valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int)
{
    loadFromValueTree();
}

ControllableBase& ParameterAutomation::getControllable()
{
    return controllable;
}

//==============================================================================

AudioParameterAutomation::AudioParameterAutomation (ProcessorController& controllerToUse,
                                                    juce::AudioProcessorParameter& parameterToUse,
                                                    const juce::ValueTree& stateToUse,
                                                    juce::UndoManager* undo)
  : ParameterAutomation (controllerToUse,
                         parameterToUse.getDefaultValue(),
                         stateToUse,
                         undo),
    parameter (parameterToUse)
{
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

int AudioParameterAutomation::getParameterIndex() const
{
    return parameter.getParameterIndex();
}

juce::NamedValueSet& AudioParameterAutomation::getParameterProperties()
{
    return properties;
}

int AudioParameterAutomation::getNumSteps() const
{
    return parameter.getNumSteps();
}

juce::StringArray AudioParameterAutomation::getAllValueStrings() const
{
    return parameter.getAllValueStrings();
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
        parameter.setValueNotifyingHost (float (getValueForTime (pts)));
}

double AudioParameterAutomation::getRealValueForTime (double pts) const
{
    if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(&parameter))
        return ranged->getNormalisableRange().convertFrom0to1 (float (getValueForTime (pts)));

    return getValueForTime (pts);
}

void AudioParameterAutomation::setRealValue (double newValue)
{
    if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(&parameter))
        setValue (ranged->getNormalisableRange().convertTo0to1 (float (newValue)));
    else
        setValue (newValue);
}

void AudioParameterAutomation::setRealValue (double pts, double newValue)
{
    if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(&parameter))
        setValue (pts, ranged->getNormalisableRange().convertTo0to1 (float (newValue)));
    else
        setValue (pts, newValue);
}

void AudioParameterAutomation::addRealKeyframe (double pts, double newValue)
{
    if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(&parameter))
        addKeyframe (pts, ranged->getNormalisableRange().convertTo0to1 (float (newValue)));
    else
        addKeyframe (pts, newValue);
}

void AudioParameterAutomation::setKeyframesWithRealValues (std::map<double, double> keys)
{
    if (auto* param = dynamic_cast<juce::RangedAudioParameter*>(&parameter))
    {
        auto range = param->getNormalisableRange();
        std::map<double, double> realKeys;
        for (auto& k : keys)
            realKeys [k.first] = range.convertTo0to1 (float (k.second));

        setKeyframes (realKeys);
    }
    else
    {
        setKeyframes (keys);
    }
}

void AudioParameterAutomation::parameterValueChanged (int, float newValue)
{
//    DBG ("Got Value: " << getName() << " - " << juce::String (newValue) << " " << juce::String (parameterIndex) << "/" << juce::String (parameter.getParameterIndex()));
    auto pts = controllable.getCurrentPTS();
    setValue (pts, newValue);
}

void AudioParameterAutomation::parameterGestureChanged (int, bool gestureIsStarting)
{
    if (gestureIsStarting)
    {
//        DBG ("Start gesture: " << getName() << " " << juce::String (parameterIndex) << "/" << juce::String (parameter.getParameterIndex()));
        startAutomationGesture();
    }
    else
    {
//        DBG ("End gesture: " << getName() << " " << juce::String (parameterIndex) << "/" << juce::String (parameter.getParameterIndex()));
        finishAutomationGesture();
    }
}

//==============================================================================

VideoParameterAutomation::VideoParameterAutomation (ControllableBase& controllerToUse,
                                                    ProcessorParameter& parameterToUse,
                                                    const juce::ValueTree& stateToUse,
                                                    juce::UndoManager* undo)
  : ParameterAutomation (controllerToUse,
                         parameterToUse.getDefaultValue(),
                         stateToUse,
                         undo),
    parameter (parameterToUse)
{
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

int VideoParameterAutomation::getParameterIndex() const
{
    return parameter.getParameterIndex();
}

juce::NamedValueSet& VideoParameterAutomation::getParameterProperties()
{
    return parameter.getProperties();
}

int VideoParameterAutomation::getNumSteps() const
{
    return parameter.getNumSteps();
}

juce::StringArray VideoParameterAutomation::getAllValueStrings() const
{
    return {};
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

double VideoParameterAutomation::getRealValueForTime (double pts) const
{
    return parameter.unNormaliseValue (getValueForTime (pts));
}

void VideoParameterAutomation::setRealValue (double newValue)
{
    setValue (parameter.normaliseValue (newValue));
}

void VideoParameterAutomation::setRealValue (double pts, double newValue)
{
    setValue (pts, parameter.normaliseValue (newValue));
}

void VideoParameterAutomation::addRealKeyframe (double pts, double newValue)
{
    addKeyframe (pts, parameter.normaliseValue (newValue));
}

void VideoParameterAutomation::setKeyframesWithRealValues (std::map<double, double> keys)
{
    std::map<double, double> realKeys;
    for (auto& k : keys)
        realKeys [k.first] = parameter.normaliseValue (k.second);

    setKeyframes (realKeys);
}

void VideoParameterAutomation::valueChanged (ProcessorParameter&, double newValue)
{
    auto pts = controllable.getCurrentPTS();
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
