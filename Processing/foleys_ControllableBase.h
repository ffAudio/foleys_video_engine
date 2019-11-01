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

class ClipDescriptor;
class ParameterAutomation;


/**
 The ControllableBase acts as counterpart to ParameterAutomation. By inheriting this
 interface, it is possible to automate things like AudioProcessors or VideoProcessors.
 But also Clips could have automated parameters, e.g. for geometric information, alpha
 or an audio gain.
 */
class ControllableBase
{
public:
    ControllableBase() = default;
    virtual ~ControllableBase() = default;

    /**
     Since the automation values are time dependent, every instance, that inherits
     ControllableBase needs a way to tell the local time (presentation time stamp).
     */
    virtual double getCurrentPTS() const = 0;

    /**
     Grant access to the individual parameters.
     */
    virtual std::vector<std::unique_ptr<ParameterAutomation>>& getParameters() = 0;
    virtual int getNumParameters() const = 0;

    /**
     This notifies all ProcessorController::Listeners about an automation change,
     so they can adapt accordingly by redrawing the curves or invalidating pre-rendered
     video frames.
     */
    virtual void notifyParameterAutomationChange (const ParameterAutomation*) = 0;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControllableBase)
};


} // foleys
