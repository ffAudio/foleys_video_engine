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

    Library.cpp
    Created: 30 Mar 2019 4:45:01pm
    Author:  Daniel Walz

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "Library.h"
#include "Player.h"

//==============================================================================
Library::Library (Player& player, foleys::VideoEngine& engine)
  : videoEngine (engine)
{
    ignoreUnused (videoEngine);

    directoryThread.startThread (3);

    tabs.addTab (NEEDS_TRANS ("Movies"), Colours::darkgrey,
                 new MediaList (player,
                                directoryThread,
                                File::getSpecialLocation (File::userMoviesDirectory),
                                std::make_unique<WildcardFileFilter> ("*", "*", "All")), true);

#if defined (JUCE_MODULE_AVAILABLE_filmstro_av_clip) && JUCE_MODULE_AVAILABLE_filmstro_av_clip==1
    // FILMSTRO:
    videoEngine.getFormatManager().registerFactory ("filmstro", filmstro::FilmstroClip::getFactory());
    auto* filmstroComponent = new FilmstroComponent();
    filmstroComponent->setOnStartAudition ([&](auto source) { player.setAuditionSource (std::move (source), 48000.0); });
    filmstroComponent->setOnStopAudition  ([&]() { player.stopAudition(); });
    tabs.addTab ("Filmstro", Colours::darkgrey, filmstroComponent, true);
#endif

    tabs.addTab (NEEDS_TRANS ("Music"), Colours::darkgrey,
                 new MediaList (player,
                                directoryThread,
                                File::getSpecialLocation (File::userMusicDirectory),
                                std::make_unique<WildcardFileFilter> ("*", "*", "All")), true);

    tabs.addTab (NEEDS_TRANS ("Stills"), Colours::darkgrey,
                 new MediaList (player,
                                directoryThread,
                                File::getSpecialLocation (File::userPicturesDirectory),
                                std::make_unique<WildcardFileFilter> ("*", "*", "All")), true);

    addAndMakeVisible (tabs);
}

void Library::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));   // clear the background
}

void Library::resized()
{
    tabs.setBounds (getLocalBounds().reduced (3));
}

//==============================================================================
Library::MediaList::MediaList (Player& playerToUse, TimeSliceThread& readThread, const File& root, std::unique_ptr<FileFilter> filterToUse)
  : player (playerToUse),
    filter (std::move (filterToUse)),
    contents (nullptr, readThread)
{
    contents.setDirectory (root, true, true);
    if (filter)
        contents.setFileFilter (filter.get());

    fileTree.setDragAndDropDescription ("media");
    fileTree.addListener (this);

    addAndMakeVisible (fileTree);
}

Library::MediaList::~MediaList()
{
    fileTree.removeListener (this);
}

void Library::MediaList::resized()
{
    fileTree.setBounds (getLocalBounds());
}

void Library::MediaList::fileDoubleClicked (const File& file)
{
    player.setAuditionFile (file);
}

void Library::MediaList::fileClicked (const File&, const MouseEvent&)
{
    player.stopAudition();
}
