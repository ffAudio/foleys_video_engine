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

AVClip::~AVClip()
{
    masterReference.clear();
}

void AVClip::sendTimecode (Timecode newTimecode, juce::NotificationType nt)
{
    if (nt == juce::sendNotification || nt == juce::sendNotificationAsync)
    {
        timecodeListeners.call ([newTimecode](TimecodeListener& l)
                                {
                                    juce::MessageManager::callAsync ([&l, newTimecode]
                                    {
                                        l.timecodeChanged (newTimecode);
                                    });
                                });
    }
    else if (nt == juce::sendNotificationSync)
    {
        timecodeListeners.call ([newTimecode](TimecodeListener& l)
                                {
                                    l.timecodeChanged (newTimecode);
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

void AVClip::sendSubtitle (const juce::String& text, Timecode until, juce::NotificationType nt)
{
    subtitleListeners.call ([=](SubtitleListener& l)
                            {
                                l.setSubtitle (text, until);
                            });
}

void AVClip::addSubtitleListener (SubtitleListener* listener)
{
    subtitleListeners.add (listener);
}

void AVClip::removeSubtitleListener (SubtitleListener* listener)
{
    subtitleListeners.remove (listener);
}

juce::TimeSliceClient* AVClip::getBackgroundJob()
{
    return nullptr;
}

}
