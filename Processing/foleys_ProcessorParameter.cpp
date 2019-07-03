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

ProcessorParameter::ProcessorParameter (const juce::String& id,
                                        const juce::String& nameToUse)
  : paramID (id),
    name (nameToUse)
{
}

const juce::String& ProcessorParameter::getParameterID() const
{
    return paramID;
}

const juce::String& ProcessorParameter::getName() const
{
    return name;
}

void ProcessorParameter::sendUpdateNotification()
{
    listeners.call ([this, v = getRealValue()](Listener& l) { l.valueChanged (*this, v); });
}

void ProcessorParameter::beginGesture()
{
    jassert (gestureInProgress == 0);

    ++gestureInProgress;
    listeners.call ([this](Listener& l) { l.gestureStarted (*this); });
}

void ProcessorParameter::endGesture()
{
    --gestureInProgress;
    listeners.call ([this](Listener& l) { l.gestureFinished (*this); });
}

bool ProcessorParameter::isGestureInProgress() const
{
    return gestureInProgress <= 0;
}

void ProcessorParameter::addListener (Listener* listener)
{
    listeners.add (listener);
}
void ProcessorParameter::removeListener (Listener* listener)
{
    listeners.remove (listener);
}

//==============================================================================

ProcessorParameterFloat::ProcessorParameterFloat (const juce::String& id,
                                                  const juce::String& nameToUse,
                                                  juce::NormalisableRange<double> rangeToUse,
                                                  double defaultToUse,
                                                  std::function<juce::String (double, int)> valueToTextToUse,
                                                  std::function<double(const juce::String&)> textToValueToUse)
  : ProcessorParameter (id, nameToUse),
    range (rangeToUse),
    value (defaultToUse),
    defaultValue (defaultToUse),
    valueToText (valueToTextToUse),
    textToValue (textToValueToUse)
{
}

const int ProcessorParameterFloat::getNumSteps() const
{
    return range.interval == 0 ? 0 : (range.end - range.start) / range.interval;
}

double ProcessorParameterFloat::getNormalisedValue() const
{
    return range.convertTo0to1 (value);
}

double ProcessorParameterFloat::getRealValue() const
{
    return value;
}

double ProcessorParameterFloat::getDefaultValue() const
{
    return range.convertTo0to1 (defaultValue);
}

void ProcessorParameterFloat::setNormalisedValue (double newValue)
{
    auto candidate = range.convertFrom0to1 (newValue);
    if (candidate != value)
    {
        value = candidate;
        sendUpdateNotification();
    }
}

void ProcessorParameterFloat::setRealValue (double newValue)
{
    auto candidate = range.snapToLegalValue (newValue);
    if (candidate != value)
    {
        value = candidate;
        sendUpdateNotification();
    }
}

double* ProcessorParameterFloat::getRawParameterValue()
{
    return &value;
}

juce::String ProcessorParameterFloat::getText (float normalisedValue, int numDigits) const
{
    auto normal = range.convertFrom0to1 (normalisedValue);
    if (valueToText != nullptr)
        return valueToText (normal, numDigits);

    return juce::String (normal, numDigits);
}

double ProcessorParameterFloat::getValueForText (const juce::String& text) const
{
    auto normal = textToValue != nullptr ? textToValue (text) : text.getDoubleValue();
    return range.convertTo0to1 (normal);
}

//==============================================================================

ProcessorState::ProcessorState (void*, juce::UndoManager* undo, const juce::String& rootType, std::vector<std::unique_ptr<ProcessorParameter>> parametersToUse)
  : state (rootType),
    undoManager (undo)
{
    juce::ignoreUnused (undoManager);

    for (auto& p : parametersToUse)
    {
        // you have a duplicate parameter with paramID!
        jassert (parameters.find (p->getParameterID()) == parameters.end());

        parameters [p->getParameterID()] = std::move (p);
    }
}

ProcessorParameter* ProcessorState::getParameter (const juce::String& paramID)
{
    auto p = parameters.find (paramID);
    if (p != parameters.end())
        return p->second.get();

    return nullptr;
}

std::vector<ProcessorParameter*> ProcessorState::getParameters()
{
    std::vector<ProcessorParameter*> references;
    for (auto& p : parameters)
        references.push_back (p.second.get());
    return references;
}

double* ProcessorState::getRawParameterValue (const juce::String& paramID)
{
    if (auto* parameter = getParameter (paramID))
        return parameter->getRawParameterValue();

    jassertfalse;
    return nullptr;
}

} // foleys
