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

    Library.h
    Created: 30 Mar 2019 4:45:01pm
    Author:  Daniel Walz

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class Player;

//==============================================================================
/*
*/
class Library    : public Component
{
public:
    Library (Player& player, foleys::VideoEngine& engine);

    void paint (Graphics&) override;
    void resized() override;

    class MediaList  : public Component,
                       private FileBrowserListener
    {
    public:
        MediaList (Player& player, TimeSliceThread& readThread, const File& root, std::unique_ptr<FileFilter> filter);
        ~MediaList() override;

        void resized() override;

        void selectionChanged() override {}
        void fileClicked (const File &file, const MouseEvent &e) override;
        void fileDoubleClicked (const File &file) override;
        void browserRootChanged (const File &) override {}

    private:
        Player&                     player;
        std::unique_ptr<FileFilter> filter;
        DirectoryContentsList       contents;
        FileTreeComponent           fileTree  { contents };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MediaList)
    };
private:
    TimeSliceThread directoryThread { "Directory read thread" };

    TabbedComponent tabs { TabbedButtonBar::TabsAtTop };

    foleys::VideoEngine& videoEngine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Library)
};
