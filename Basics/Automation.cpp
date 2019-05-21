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

AutomationParameter::AutomationParameter (ClipDescriptor::ProcessorHolder&  holderToUse,
                                          juce::ControllableProcessorBase&  processorToUse,
                                          juce::AudioProcessorParameter&    parameterToUse)
  : holder     (holderToUse),
    processor  (processorToUse),
    parameter  (parameterToUse)
{
    value = parameter.getDefaultValue();
    parameter.addListener (this);
}

AutomationParameter::~AutomationParameter()
{
    parameter.removeListener (this);
}

void AutomationParameter::updateProcessor (double pts)
{
    if (!gestureInProgress)
        parameter.setValueNotifyingHost (getValueForTime (pts));
}

void AutomationParameter::setValue (double pts, double newValue)
{
    if (!gestureInProgress)
        return;

    if (keyframes.empty())
    {
        value = newValue;
    }
    else
    {
        auto diff = newValue - getValueForTime (pts);
        for (auto& k : keyframes)
            k.second = juce::jlimit (0.0, 1.0, k.second + diff);
    }

    if (! manualUpdate)
        holder.synchroniseState (*this);
}

void AutomationParameter::addKeyframe (double pts, double newValue)
{
    keyframes [pts] = juce::jlimit (0.0, 1.0, newValue);

    if (! manualUpdate)
        holder.synchroniseState (*this);
}

void AutomationParameter::setKeyframe (size_t index, double pts, double newValue)
{
    if (juce::isPositiveAndBelow (index, keyframes.size()))
    {
        auto it = std::next (keyframes.begin(), index);

        keyframes.erase (it);
        keyframes [pts] = newValue;
    }

    if (! manualUpdate)
        holder.synchroniseState (*this);
}

double AutomationParameter::getValueForTime (double pts) const
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

juce::String AutomationParameter::getName() const
{
    return parameter.getName (128);
}

double AutomationParameter::getValue() const
{
    return value;
}

const std::map<double, double>& AutomationParameter::getKeyframes() const
{
    return keyframes;
}

void AutomationParameter::loadFromValueTree (const juce::ValueTree& state)
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

void AutomationParameter::saveToValueTree (juce::ValueTree& state, juce::UndoManager* undo)
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

void AutomationParameter::parameterValueChanged (int parameterIndex, float newValue)
{
    auto pts = holder.getOwningClipDescriptor().getCurrentPTS();
    setValue (pts, newValue);
}

void AutomationParameter::parameterGestureChanged (int parameterIndex, bool gestureIsStarting)
{
    gestureInProgress = gestureIsStarting;
}

} // foleys
