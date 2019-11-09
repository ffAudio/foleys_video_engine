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

AVClip::AVClip (VideoEngine& videoEngineToUse) : videoEngine (&videoEngineToUse)
{
}

void AVClip::sendTimecode (int64_t count, double seconds, juce::NotificationType nt)
{
    if (nt == juce::sendNotification || nt == juce::sendNotificationAsync)
    {
        timecodeListeners.call ([count, seconds](TimecodeListener& l)
                                {
                                    juce::MessageManager::callAsync ([&l, count, seconds]
                                    {
                                        l.timecodeChanged (count, seconds);
                                    });
                                });
    }
    else if (nt == juce::sendNotificationSync)
    {
        timecodeListeners.call ([count, seconds](TimecodeListener& l)
                                {
                                    l.timecodeChanged (count, seconds);
                                });
    }
}

void AVClip::addTimecodeListener (TimecodeListener* listener)
{
    timecodeListeners.add (listener);
}

void AVClip::removeTimecodeListener (TimecodeListener* listener)
{
    timecodeListeners.remove (listener);
}

const std::vector<std::unique_ptr<ProcessorParameter>>& AVClip::getVideoParameters()
{
    return videoParameters;
}

const std::vector<std::unique_ptr<ProcessorParameter>>& AVClip::getAudioParameters()
{
    return audioParameters;
}

void AVClip::addAudioParameter (std::unique_ptr<ProcessorParameter> parameter)
{
    audioParameters.push_back (std::move (parameter));
}

void AVClip::addVideoParameter (std::unique_ptr<ProcessorParameter> parameter)
{
    videoParameters.push_back (std::move (parameter));
}

juce::TimeSliceClient* AVClip::getBackgroundJob()
{
    return nullptr;
}

VideoEngine* AVClip::getVideoEngine() const
{
    return videoEngine;
}

} // foleys
