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

namespace foleys
{

class ClipDescriptor;
class ParameterAutomation;

using AutomationMap=std::map<juce::Identifier, std::unique_ptr<ParameterAutomation>>;

/**
 The ControllableBase acts as counterpart to ParameterAutomation. By inheriting this
 interface, it is possible to automate things like AudioProcessors or VideoProcessors.
 But also Clips could have automated parameters, e.g. for geometric information, alpha
 or an audio gain.
 */
class ControllableBase
{
public:
    ControllableBase (TimeCodeAware& reference);

    virtual ~ControllableBase() = default;

    /**
     Since the automation values are time dependent, every instance, that inherits
     ControllableBase needs a way to tell the local time (presentation time stamp).
     The PTS refers to the audio clock master, video is rendered asynchronously and
     may have a different PTS
     */
    virtual double getCurrentPTS() const = 0;

    /**
     Grant access to the individual parameters.
     */
    virtual AutomationMap& getParameters() = 0;
    virtual int getNumParameters() const = 0;

    /**
     Return the value of the parameter at a certain time point.
     Since the parameter can be used from outside the parameter (i.e. for scale, position etc.)
     There needs to be a default value, in case the ControllableBase doesn't provide that parameter.

     @param paramID      the identifier of the parameter
     @param pts          the timestamp in seconds in clip time
     @param defaultValue the value that is returned, if the parameter is not set up
     */
    virtual double getValueAtTime (juce::Identifier paramID, double pts, double defaultValue) = 0;

    /**
     The Listener can subscribe to automation changes, e.g. to invalidate existing render, to update UI elements etc.
     */
    class Listener
    {
    public:
        Listener() = default;
        virtual ~Listener() = default;

        virtual void parameterAutomationChanged (const ParameterAutomation*) = 0;
    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Listener)
    };

    void addListener (Listener*);
    void removeListener (Listener*);

    /**
     This notifies all ProcessorController::Listeners about an automation change,
     so they can adapt accordingly by redrawing the curves or invalidating pre-rendered
     video frames.
     */
    void notifyParameterAutomationChange (const ParameterAutomation* p);

    /**
     Grant access to the time reference of this ControllableBase
     */
    TimeCodeAware& getTimeReference();
    const TimeCodeAware& getTimeReference() const;

private:
    TimeCodeAware&               timeReference;
    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControllableBase)
};


} // foleys
