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

AutomationParameter::AutomationParameter (juce::AudioProcessor&          processorToUse,
                                          juce::AudioProcessorParameter& parameterToUse)
  : processor (processorToUse),
    parameter (parameterToUse)
{
    value = parameter.getDefaultValue();
}

void AutomationParameter::updateProcessor (double pts)
{
    parameter.setValue (getValueForTime (pts));
}

void AutomationParameter::setValue (double pts, double newValue)
{
    if (keyframes.empty())
    {
        value = newValue;
    }

    auto diff = newValue - getValueForTime (pts);
    for (auto& k : keyframes)
        k.second = juce::jlimit (0.0, 1.0, k.second + diff);
}

void AutomationParameter::addKeyframe (double pts, double newValue)
{
    keyframes [pts] = juce::jlimit (0.0, 1.0, newValue);
}

void AutomationParameter::setKeyframe (size_t index, double pts, double newValue)
{
    if (juce::isPositiveAndBelow (index, keyframes.size()))
    {
        auto it = keyframes.begin();
        std::advance (it, index);
        keyframes.erase (it);
        keyframes [pts] = newValue;
    }
}

double AutomationParameter::getValueForTime (double pts) const
{
    if (keyframes.empty())
        return value;

    const auto& prev = keyframes.lower_bound (pts);
    if (prev == keyframes.cend())
        return keyframes.begin()->first;

    const auto range = keyframes.equal_range (pts);
    if (range.first == keyframes.cend())
        return keyframes.cbegin()->second;

    if (range.second == keyframes.cend())
        return (--keyframes.cend())->second;

    auto interpolated = range.first->second + (pts - range.first->first) * (range.second->second - range.first->second) / (range.second->first - range.first->first);
    return juce::jlimit (0.0, 1.0, interpolated);
}

} // foleys
