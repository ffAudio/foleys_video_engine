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

/**
 Add this interface if your class is aware of a time continuum.
 It can notify observers when the time advances, usually when the visual frame changes.
 */
class TimeCodeAware
{
public:
    TimeCodeAware() = default;
    virtual ~TimeCodeAware() = default;

    /** Return the clip's read position in seconds */
    virtual double getCurrentTimeInSeconds() const = 0;

    /** Use a TimecodeListener to be notified, when the visual frame changes */
    struct Listener
    {
        /**
         Destructor
         */
        virtual ~Listener() = default;

        /**
         Listen to this callback to get notified, when the time code changes.
         This is most useful to redraw the display or encode the next frame
         */
        virtual void timecodeChanged (int64_t count, double seconds) = 0;
    };

    /** Register a TimecodeListener to be notified, when the visual frame changes */
    void addTimecodeListener (Listener* listener);
    /** Unregister a TimecodeListener */
    void removeTimecodeListener (Listener* listener);

protected:

    /**
     Subclasses can call this to notify displays, that the time code has changed, e.g. to display a new frame
     */
    void sendTimecode (int64_t count, double seconds, juce::NotificationType nt);

private:
    juce::ListenerList<Listener> timecodeListeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimeCodeAware)
};


} // namespace foleys
