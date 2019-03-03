

namespace foleys
{

AVClip::~AVClip()
{
    masterReference.clear();
}


void AVClip::sendTimecode (AVTimecode newTimecode)
{
    timecodeListeners.call ([newTimecode](TimecodeListener& l)
                            {
                                l.timecodeChanged (newTimecode);
                            });
}

void AVClip::addTimecodeListener (TimecodeListener* listener)
{
    timecodeListeners.add (listener);
}

void AVClip::removeTimecodeListener (TimecodeListener* listener)
{
    timecodeListeners.remove (listener);
}

void AVClip::sendSubtitle (const juce::String& text, AVTimecode until)
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

}
