

namespace foleys
{

AVClip::~AVClip()
{
    VideoEngine::getInstance()->removeAVClip (*this);
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
