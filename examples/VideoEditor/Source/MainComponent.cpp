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

    Main GUI for the components for the VideoEditor

  ==============================================================================
*/

#include "MainComponent.h"
#include "RenderDialog.h"

namespace CommandIDs
{
    enum
    {
        fileOpen = 100,
        fileSave,
        fileSaveAs,
        fileNew,
        fileRender,
        fileQuit,

        editPreferences = 200,
        editSplice,
        editVisibility,

        playStart = 300,
        playStop,
        playReturn,
        playRecord,

        trackAdd = 400,
        trackRemove,

        viewFullScreen = 500,
        viewExtraWindow,
        viewExitFullScreen,
        viewOpenGL,

        helpAbout = 600,
        helpHelp
    };
}


//==============================================================================
MainComponent::MainComponent()
{
//    levelMeter.setLookAndFeel (&lookAndFeel);
//    lookAndFeel.setColour (FFAU::LevelMeter::lmBackgroundColour, getLookAndFeel().findColour (ResizableWindow::backgroundColourId).darker());
//    lookAndFeel.setColour (FFAU::LevelMeter::lmTicksColour, Colours::silver);

    addAndMakeVisible (library);
    addAndMakeVisible (properties);
    addAndMakeVisible (viewport);
    addAndMakeVisible (transport);
    addAndMakeVisible (levelMeter);

    viewport.setViewedComponent (&timeline);
    timeline.setSize (2000, 510);

    setUseOpenGL (usesOpenGL);

    if (const auto* primaryDisplay = Desktop::getInstance().getDisplays().getPrimaryDisplay())
        setBounds (primaryDisplay->totalArea);

    player.initialise();
    lmLookAndFeel.setColour (foleys::LevelMeter::lmBackgroundColour, getLookAndFeel().findColour (ResizableWindow::backgroundColourId).darker());
    lmLookAndFeel.setColour (foleys::LevelMeter::lmOutlineColour, Colours::transparentBlack);
    lmLookAndFeel.setColour (foleys::LevelMeter::lmMeterOutlineColour, Colours::transparentBlack);
    lmLookAndFeel.setColour (foleys::LevelMeter::lmTicksColour, Colours::silver);
    levelMeter.setLookAndFeel (&lmLookAndFeel);
    levelMeter.setMeterSource (&player.getMeterSource());

    resetEdit();

    commandManager.registerAllCommandsForTarget (this);
    commandManager.setFirstCommandTarget (this);
#if JUCE_MAC
    setMacMainMenu (this);
#endif

    commandManager.getKeyMappings()->resetToDefaultMappings();

    auto settingsFolder = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile (ProjectInfo::companyName).getChildFile (ProjectInfo::projectName);
    settingsFolder.createDirectory();
    videoEngine.getAudioPluginManager().setPluginDataFile (settingsFolder.getChildFile ("PluginList.xml"));

    startTimerHz (10);
}

MainComponent::~MainComponent()
{
    if (auto edit = timeline.getEditClip())
        edit->removeTimecodeListener (preview.get());

#if JUCE_MAC
    setMacMainMenu (nullptr);
#endif

    levelMeter.setLookAndFeel (nullptr);
}

void MainComponent::setUseOpenGL (bool shouldUseOpenGL)
{
    if (preview.get() != nullptr)
        if (auto edit = timeline.getEditClip())
            edit->removeTimecodeListener (preview.get());

#if FOLEYS_USE_OPENGL
    if (shouldUseOpenGL)
        preview = std::make_unique<foleys::OpenGLView>();
    else
        preview = std::make_unique<foleys::SoftwareView>();

#else
    juce::ignoreUnused (shouldUseOpenGL);
    preview = std::make_unique<foleys::SoftwareView>();
#endif

    preview->setClip (timeline.getEditClip());

    if (auto edit = timeline.getEditClip())
        edit->addTimecodeListener (preview.get());

    if (auto* p = dynamic_cast<juce::Component*>(preview.get()))
        addAndMakeVisible (p);

    menuItemsChanged();

    resized();
}

KeyPressMappingSet* MainComponent::getKeyMappings() const
{
    return commandManager.getKeyMappings();
}

//==============================================================================

void MainComponent::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
    g.setColour (getLookAndFeel().findColour (ResizableWindow::backgroundColourId).darker());
    g.fillRect (getLocalBounds().withTop (lowerPart));
    g.setColour (Colours::grey);
    g.drawRect (getLocalBounds());
}

void MainComponent::resized()
{
    if (viewerFullscreenMode == Mode::MaximiseView)
    {
        if (auto* p = dynamic_cast<juce::Component*>(preview.get()))
        {
            p->setBounds (getLocalBounds());
            p->toFront (false);
        }
    }
    else
    {
        auto bounds = getLocalBounds().reduced (1);
        lowerPart = juce::roundToInt (bounds.getHeight() * 0.4);
        auto lower  = bounds.removeFromBottom (lowerPart);
        levelMeter.setBounds (lower.removeFromRight (120).reduced (2));
        lower.removeFromTop (14); // TODO: ruler
        viewport.setBounds (lower);
        auto sides = juce::roundToInt (bounds.getWidth() / 4.0);
        library.setBounds (bounds.removeFromLeft (sides));
        properties.setBounds (bounds.removeFromRight (sides));
        transport.setBounds (bounds.removeFromBottom (24));
        if (auto* p = dynamic_cast<juce::Component*>(preview.get()))
            p->setBounds (bounds);
    }
}

void MainComponent::resetEdit()
{
    auto edit = std::make_shared<foleys::ComposedClip> (videoEngine);
    videoEngine.manageLifeTime (edit);

    timeline.setEditClip (edit);
    edit->addTimecodeListener (preview.get());
    preview->setClip (edit);

    editFileName = File();
    updateTitleBar();

    videoEngine.getUndoManager()->clearUndoHistory();
}

void MainComponent::loadEdit()
{
    FileChooser myChooser ("Please select the project you want to save...",
                           File::getSpecialLocation (File::userMoviesDirectory),
                           "*.videdit");
    if (myChooser.browseForFileToOpen())
    {
        loadEditFile (myChooser.getResult());
    }
}

void MainComponent::loadEditFile (const File& file)
{
    auto xml = XmlDocument::parse (file);
    if (xml.get() == nullptr)
    {
        AlertWindow::showMessageBox (AlertWindow::WarningIcon,
                                     NEEDS_TRANS ("Loading failed"),
                                     "Loading of the file \"" + file.getFullPathName() + "\" failed.");
        return;
    }

    editFileName = file;
    auto tree = ValueTree::fromXml (*xml);
    auto edit = std::make_shared<foleys::ComposedClip>(videoEngine);
    videoEngine.manageLifeTime (edit);

    for (auto clip : tree)
        edit->getStatusTree().appendChild (clip.createCopy(), nullptr);

    timeline.setEditClip (edit);
    edit->addTimecodeListener (preview.get());

    player.setPosition (0);
    updateTitleBar();

    videoEngine.getUndoManager()->clearUndoHistory();

    timeline.resized();
}

void MainComponent::saveEdit (bool saveAs)
{
    auto edit = timeline.getEditClip();

    if (saveAs || editFileName.getFullPathName().isEmpty())
    {
        FileChooser myChooser ("Please select the project you want to save...",
                               File::getSpecialLocation (File::userMoviesDirectory),
                               "*.videdit");
        if (myChooser.browseForFileToSave (true))
        {
            editFileName = myChooser.getResult();
        }
        else
        {
            return;
        }
    }

    if (edit && editFileName.getFullPathName().isNotEmpty())
    {
        FileOutputStream output (editFileName);
        if (output.openedOk())
        {
            edit->readPluginStatesIntoValueTree();

            output.setPosition (0);
            output.truncate();
            output.writeString (edit->getStatusTree().toXmlString());
        }
        else
        {
            AlertWindow::showMessageBox (AlertWindow::WarningIcon, NEEDS_TRANS("Saving failed"), "Saving of file \"" + editFileName.getFullPathName() + "\" failed.");
        }
        updateTitleBar();
    }
}

bool MainComponent::handleQuitRequest()
{
    if (renderer.isRendering())
    {
        if (AlertWindow::showOkCancelBox (AlertWindow::WarningIcon, NEEDS_TRANS("Quit application"), "Cancel rendering to quit?"))
            renderer.cancelRendering();
        else
            return false;
    }

    return true;
}

void MainComponent::showRenderDialog()
{
    if (! renderer.isRendering())
        renderer.setClipToRender (timeline.getEditClip()->createCopy (foleys::StreamTypes::all()));

    properties.showProperties (std::make_unique<RenderDialog>(renderer));
}

void MainComponent::deleteSelectedClip()
{
    if (auto selected = timeline.getSelectedClip())
        if (auto edit = timeline.getEditClip())
            edit->removeClip (selected);
}

void MainComponent::showPreferences()
{
    auto selector = std::make_unique<AudioDeviceSelectorComponent>(deviceManager, 0, 2, 2, 2, false, false, true, false);
    properties.showProperties (std::move (selector));
}

void MainComponent::updateTitleBar()
{
    if (auto* window = dynamic_cast<DocumentWindow*>(TopLevelWindow::getActiveTopLevelWindow()))
    {
        if (editFileName.getFullPathName().isNotEmpty())
            window->setName (ProjectInfo::projectName + String (": ") + editFileName.getFileNameWithoutExtension());
        else
            window->setName (ProjectInfo::projectName);
    }
}

void MainComponent::setViewerFullScreen (Mode fullscreenMode)
{
    viewerFullscreenMode = fullscreenMode;
    if (viewerFullscreenMode == Mode::ExtraWindow)
    {
        playerWindow = std::make_unique<PlayerWindow>(usesOpenGL);
        playerWindow->video->setClip (timeline.getEditClip());
        preview->setClip ({});
        auto displays = juce::Desktop::getInstance().getDisplays();
        if (displays.displays.size() > 1)
        {
            if (auto* currentDisplay = displays.getDisplayForRect (getScreenBounds()))
            {
                for (auto d : displays.displays)
                {
                    if (d.totalArea != currentDisplay->totalArea)
                    {
                        playerWindow->setBounds (d.totalArea);
                        break;
                    }
                }
            }
        }

        playerWindow->setVisible (true);
    }
    else
    {
        playerWindow.reset();
        preview->setClip (timeline.getEditClip());
    }

    resized();
}

//==============================================================================

void MainComponent::getAllCommands (Array<CommandID>& commands)
{
    commands.add (CommandIDs::fileNew, CommandIDs::fileOpen, CommandIDs::fileSave, CommandIDs::fileSaveAs, CommandIDs::fileRender, StandardApplicationCommandIDs::quit);
    commands.add (StandardApplicationCommandIDs::undo, StandardApplicationCommandIDs::redo,
                  StandardApplicationCommandIDs::del, StandardApplicationCommandIDs::copy, StandardApplicationCommandIDs::paste,
                  CommandIDs::editSplice, CommandIDs::editVisibility, CommandIDs::editPreferences);
    commands.add (CommandIDs::playStart, CommandIDs::playStop, CommandIDs::playReturn);
    commands.add (CommandIDs::trackAdd, CommandIDs::trackRemove);
    commands.add (CommandIDs::viewFullScreen, CommandIDs::viewExtraWindow, CommandIDs::viewExitFullScreen, CommandIDs::viewOpenGL);
    commands.add (CommandIDs::helpAbout, CommandIDs::helpHelp);
}

void MainComponent::getCommandInfo (CommandID commandID, ApplicationCommandInfo& result)
{
    auto categoryFile  = "file";
    auto categoryEdit  = "edit";
    auto categoryPlay  = "play";
    auto categoryTrack = "track";
    auto categoryView  = "view";
    auto categoryHelp  = "help";

    switch (commandID)
    {
        case CommandIDs::fileNew:
            result.setInfo ("New Project...", "Clear all and start fresh", categoryFile, 0);
            result.defaultKeypresses.add (KeyPress ('n', ModifierKeys::commandModifier, 0));
            break;
        case CommandIDs::fileOpen:
            result.setInfo ("Open Project...", "Select a project to open", categoryFile, 0);
            result.defaultKeypresses.add (KeyPress ('o', ModifierKeys::commandModifier, 0));
            break;
        case CommandIDs::fileSave:
            result.setInfo ("Save Project", "Save the current project", categoryFile, 0);
            result.defaultKeypresses.add (KeyPress ('s', ModifierKeys::commandModifier, 0));
            break;
        case CommandIDs::fileSaveAs:
            result.setInfo ("Save Project...", "Save the current project as new file", categoryFile, 0);
            break;
        case CommandIDs::fileRender:
            result.setInfo ("Render Project...", "Export the piece into an audio file", categoryFile, 0);
            result.defaultKeypresses.add (KeyPress ('r', ModifierKeys::commandModifier, 0));
            break;
        case StandardApplicationCommandIDs::quit:
            result.setInfo ("Quit...", "Quit Application", categoryFile, 0);
            result.defaultKeypresses.add (KeyPress ('q', ModifierKeys::commandModifier, 0));
            break;
        case StandardApplicationCommandIDs::undo:
            result.setInfo ("Undo", "Undo the last step", categoryEdit, 0);
            result.defaultKeypresses.add (KeyPress ('z', ModifierKeys::commandModifier, 0));
            break;
        case StandardApplicationCommandIDs::redo:
            result.setInfo ("Redo", "Redo the last undo step", categoryEdit, 0);
            result.defaultKeypresses.add (KeyPress ('z', ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 0));
            break;
        case StandardApplicationCommandIDs::del:
            result.setInfo ("Delete", "Delete the selected gesture", categoryEdit, 0);
            result.defaultKeypresses.add (KeyPress (KeyPress::backspaceKey, ModifierKeys::noModifiers, 0));
            break;
        case StandardApplicationCommandIDs::copy:
            result.setInfo ("Copy", "Copy the selected gesture", categoryEdit, 0);
            result.defaultKeypresses.add (KeyPress ('c', ModifierKeys::commandModifier, 0));
            break;
        case StandardApplicationCommandIDs::paste:
            result.setInfo ("Paste", "Paste the gesture in the clipboard", categoryEdit, 0);
            result.defaultKeypresses.add (KeyPress ('v', ModifierKeys::commandModifier, 0));
            break;
        case CommandIDs::editSplice:
            result.setInfo ("Splice", "Split selected clip at play position", categoryEdit, 0);
            result.defaultKeypresses.add (KeyPress ('b', ModifierKeys::commandModifier, 0));
            break;
        case CommandIDs::editVisibility:
            result.setInfo ("Visible", "Toggle visibility or mute of the selected clip", categoryEdit, 0);
            result.defaultKeypresses.add (KeyPress ('v', ModifierKeys::noModifiers, 0));
            break;
        case CommandIDs::editPreferences:
            result.setInfo ("Preferences", "Open the audio preferences", categoryEdit, 0);
            result.defaultKeypresses.add (KeyPress (',', ModifierKeys::commandModifier, 0));
            break;
        case CommandIDs::playStart:
            result.setInfo ("Play", "Start/Pause playback", categoryPlay, 0);
            result.defaultKeypresses.add (KeyPress (KeyPress::spaceKey, ModifierKeys::noModifiers, 0));
            break;
        case CommandIDs::playStop:
            result.setInfo ("Stop", "Stop playback", categoryPlay, 0);
            break;
        case CommandIDs::playReturn:
            result.setInfo ("Return", "Set playhead to begin", categoryPlay, 0);
            result.defaultKeypresses.add (KeyPress (KeyPress::returnKey, ModifierKeys::noModifiers, 0));
            break;
        case CommandIDs::trackAdd:
            result.setInfo ("Add Track", "Add a new AUX track", categoryTrack, 0);
            result.defaultKeypresses.add (KeyPress ('t', ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 0));
            break;
        case CommandIDs::trackRemove:
            result.setInfo ("Remove Track", "Remove an AUX track", categoryTrack, 0);
            break;
        case CommandIDs::viewFullScreen:
            result.setInfo ("Fullscreen", "Maximise Viewer", categoryView, 0);
            result.defaultKeypresses.add (KeyPress (KeyPress::returnKey, ModifierKeys::commandModifier, 0));
            break;
        case CommandIDs::viewExtraWindow:
            result.setInfo ("Full extra Monitor", "Maximise Viewer on extra monitor", categoryView, 0);
            result.defaultKeypresses.add (KeyPress (KeyPress::returnKey, ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 0));
            break;
        case CommandIDs::viewExitFullScreen:
            result.setInfo ("Exit Fullscreen", "Normal viewer size", categoryView, 0);
            result.defaultKeypresses.add (KeyPress (KeyPress::escapeKey, ModifierKeys::noModifiers, 0));
            break;
        case CommandIDs::viewOpenGL:
            result.setInfo ("Enable OpenGL", "Switch OpenGL on or off", categoryView, 0);
            result.setTicked (usesOpenGL);
            break;
        case CommandIDs::helpAbout:
            result.setInfo ("About", "Show information about the program", categoryHelp, 0);
            break;
        case CommandIDs::helpHelp:
            result.setInfo ("Help", "Show help how to use the program", categoryHelp, 0);
            break;
        default:
            JUCEApplication::getInstance()->getCommandInfo (commandID, result);
            break;
    }
}

bool MainComponent::perform (const InvocationInfo& info)
{
    switch (info.commandID) {
        case CommandIDs::fileNew: resetEdit(); break;
        case CommandIDs::fileOpen: loadEdit(); break;
        case CommandIDs::fileSave: saveEdit(false); break;
        case CommandIDs::fileSaveAs: saveEdit(true); break;
        case CommandIDs::fileRender: showRenderDialog(); break;
        case StandardApplicationCommandIDs::quit: JUCEApplication::getInstance()->systemRequestedQuit(); break;

        case StandardApplicationCommandIDs::undo: videoEngine.getUndoManager()->undo(); break;
        case StandardApplicationCommandIDs::redo: videoEngine.getUndoManager()->redo(); break;
        case StandardApplicationCommandIDs::del: deleteSelectedClip(); break;
        case CommandIDs::editSplice: timeline.spliceSelectedClipAtPlayPosition(); break;
        case CommandIDs::editVisibility: timeline.toggleVisibility(); break;

        case CommandIDs::editPreferences: showPreferences(); break;

        case CommandIDs::playStart: if (player.isPlaying()) player.stop(); else player.start(); break;
        case CommandIDs::playStop: player.stop(); break;
        case CommandIDs::playReturn: player.setPosition (0.0) ; break;

        case CommandIDs::trackAdd: break;
        case CommandIDs::trackRemove: break;

        case CommandIDs::viewFullScreen: setViewerFullScreen (viewerFullscreenMode == Mode::NormalView ? Mode::MaximiseView : Mode::NormalView); break;
        case CommandIDs::viewExtraWindow: setViewerFullScreen (Mode::ExtraWindow); break;
        case CommandIDs::viewExitFullScreen: setViewerFullScreen (Mode::NormalView); break;
        case CommandIDs::viewOpenGL: usesOpenGL = !usesOpenGL; setUseOpenGL (usesOpenGL); break;
        default:
            jassertfalse;
            break;
    }
    return true;
}

StringArray MainComponent::getMenuBarNames()
{
    return {NEEDS_TRANS ("File"), NEEDS_TRANS ("Edit"), NEEDS_TRANS ("Play"), NEEDS_TRANS ("Track"), NEEDS_TRANS ("View"), NEEDS_TRANS ("Help")};
}

PopupMenu MainComponent::getMenuForIndex (int topLevelMenuIndex,
                                          const String&)
{
    PopupMenu menu;
    if (topLevelMenuIndex == 0)
    {
        menu.addCommandItem (&commandManager, CommandIDs::fileNew);
        menu.addSeparator();
        menu.addCommandItem (&commandManager, CommandIDs::fileOpen);
        menu.addCommandItem (&commandManager, CommandIDs::fileSave);
        menu.addCommandItem (&commandManager, CommandIDs::fileSaveAs);
        menu.addSeparator();
        menu.addCommandItem (&commandManager, CommandIDs::fileRender);
#if ! JUCE_MAC
        menu.addSeparator();
        menu.addCommandItem (&commandManager, StandardApplicationCommandIDs::quit);
#endif
    }
    else if (topLevelMenuIndex == 1)
    {
        menu.addCommandItem (&commandManager, StandardApplicationCommandIDs::undo);
        menu.addCommandItem (&commandManager, StandardApplicationCommandIDs::redo);
        menu.addSeparator();
        menu.addCommandItem (&commandManager, StandardApplicationCommandIDs::del);
        menu.addCommandItem (&commandManager, StandardApplicationCommandIDs::copy);
        menu.addCommandItem (&commandManager, StandardApplicationCommandIDs::paste);
        menu.addSeparator();
        menu.addCommandItem (&commandManager, CommandIDs::editSplice);
        menu.addCommandItem (&commandManager, CommandIDs::editVisibility);
        menu.addSeparator();
        menu.addCommandItem (&commandManager, CommandIDs::editPreferences);
    }
    else if (topLevelMenuIndex == 2)
    {
        menu.addCommandItem (&commandManager, CommandIDs::playStart);
        menu.addCommandItem (&commandManager, CommandIDs::playStop);
        menu.addCommandItem (&commandManager, CommandIDs::playReturn);
    }
    else if (topLevelMenuIndex == 3)
    {
        menu.addCommandItem (&commandManager, CommandIDs::trackAdd);
        menu.addCommandItem (&commandManager, CommandIDs::trackRemove);
    }
    else if (topLevelMenuIndex == 4)
    {
        menu.addCommandItem (&commandManager, CommandIDs::viewFullScreen);
        menu.addCommandItem (&commandManager, CommandIDs::viewExtraWindow);
        menu.addCommandItem (&commandManager, CommandIDs::viewExitFullScreen);
        menu.addSeparator();
        menu.addCommandItem (&commandManager, CommandIDs::viewOpenGL);
    }
    else if (topLevelMenuIndex == 5)
    {
        menu.addCommandItem (&commandManager, CommandIDs::helpAbout);
        menu.addCommandItem (&commandManager, CommandIDs::helpHelp);
    }
    return menu;
}

void MainComponent::timerCallback()
{
    for (auto& source : Desktop::getInstance().getMouseSources())
        if (source.isDragging())
            return;

    videoEngine.getUndoManager()->beginNewTransaction();
}
