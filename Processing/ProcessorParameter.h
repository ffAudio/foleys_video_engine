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

class ProcessorParameter
{
public:
    class Listener
    {
    public:
        Listener() = default;
        virtual ~Listener() = default;

        virtual void valueChanged (double newValue) = 0;
        virtual void gestureStarted() = 0;
        virtual void gestureFinished() = 0;
    };

    /**
     This defines a parameter to be used in Processors like the VideoProcessor.
     @param paramID an unique identifier for host and your processor to identify the parameter
     @param name the name the host will use to display the automation
     */
    ProcessorParameter (const juce::String& paramID,
                        const juce::String& name);

    virtual ~ProcessorParameter() = default;

    const juce::String& getParameterID() const;
    const juce::String& getName() const;

    virtual double* getRawParameterValue() = 0;

    virtual double getNormalisedValue() const = 0;
    virtual double getRealValue() const = 0;
    virtual double getDefaultValue() const = 0;

    virtual void setNormalisedValue (double value) = 0;
    virtual void setRealValue (double value) = 0;

    virtual double normaliseValue (double realValue) = 0;
    virtual double unnormalisedValue (double normalValue) = 0;

    void beginGesture();
    void endGesture();
    bool isGestureInProgress() const;

    void addListener (Listener* listener);
    void removeListener (Listener* listener);

    void sendUpdateNotification();

private:
    const juce::String paramID;
    juce::String name;
    int gestureInProgress = 0;

    juce::ListenerList<Listener> listeners;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorParameter)
};

//==============================================================================

class ProcessorParameterFloat : public ProcessorParameter
{
public:
    ProcessorParameterFloat (const juce::String& paramID,
                             const juce::String& name,
                             juce::NormalisableRange<double> range,
                             double defaultValue,
                             std::function<double(const juce::String&)> textToValue = nullptr,
                             std::function<juce::String (double)> valueToText = nullptr);

    double* getRawParameterValue() override;

    double getNormalisedValue() const override;
    double getRealValue() const override;
    double getDefaultValue() const override;

    void setNormalisedValue (double value) override;
    void setRealValue (double value) override;

    double normaliseValue (double realValue) override;
    double unnormalisedValue (double normalValue) override;

private:
    juce::NormalisableRange<double> range;
    double value = {};
    double defaultValue = {};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorParameterFloat)
};

//==============================================================================

class ProcessorState
{
public:
    ProcessorState (void*, juce::UndoManager*, const juce::String& rootType, std::vector<std::unique_ptr<ProcessorParameter>> layout);

    ProcessorParameter* getParameter (const juce::String& paramID);

    double* getRawParameterValue (const juce::String& paramID);

    std::vector<ProcessorParameter*> getParameters();

    juce::ValueTree state;
private:
    std::map<juce::String, std::unique_ptr<ProcessorParameter>> parameters;
    juce::UndoManager* undoManager = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorState)
};

}
