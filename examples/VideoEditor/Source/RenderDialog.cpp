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

    RenderDialog.cpp
    Created: 23 Apr 2019 8:36:47pm
    Author:  Daniel Walz

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "RenderDialog.h"

//==============================================================================
RenderDialog::RenderDialog (foleys::ClipRenderer& rendererToUse) : renderer (rendererToUse)
{
    filename.setColour (Label::outlineColourId, Colours::silver);
    addAndMakeVisible (filename);
    browse.setConnectedEdges (TextButton::ConnectedOnLeft);
    addAndMakeVisible (browse);
    addAndMakeVisible (progressBar);
    addAndMakeVisible (cancel);
    addAndMakeVisible (start);

    updateGUI();

    start.onClick = [&]()
    {
        if (renderer.getOutputFile().existsAsFile())
        {
            auto answer = AlertWindow::showOkCancelBox (AlertWindow::WarningIcon,
                                                        NEEDS_TRANS ("Overwrite existing file?"),
                                                        NEEDS_TRANS ("The file \"" + renderer.getOutputFile().getFileName() + "\" exists. Do you want to overwrite it?"));
            if (answer)
                renderer.getOutputFile().deleteFile();
            else
                return;
        }

        foleys::VideoStreamSettings settings;
        settings.frameSize = { 800, 576 };
        renderer.setVideoSettings (settings);
        renderer.startRendering (true);
        startTimerHz (2);
    };
    cancel.onClick = [&]
    {
        renderer.cancelRendering();
    };
    browse.onClick = [&]
    {
        FileChooser myChooser ("Please select the project you want to save...",
                               File::getSpecialLocation (File::userMoviesDirectory),
                               "*.mp4");
        if (myChooser.browseForFileToSave (false))
        {
            auto newFileName = myChooser.getResult();
            if (newFileName.existsAsFile())
                newFileName.deleteFile();

            renderer.setOutputFile (newFileName);
            updateGUI();
        }
    };
    renderer.onRenderingFinished = [r = &renderer](bool success)
    {
        if (success)
            r->getOutputFile().startAsProcess();
    };
}

void RenderDialog::resized()
{
    auto line = 30;
    auto bounds = getLocalBounds().reduced (5);
    auto w = bounds.getWidth() / 3;

    auto f = bounds.removeFromTop (line).reduced (3);
    browse.setBounds (f.removeFromRight (w));
    filename.setBounds (f);

    progressBar.setBounds (bounds.removeFromTop (line).reduced (3));

    auto b = bounds.removeFromTop (line).reduced (3);
    cancel.setBounds (b.removeFromLeft (w));
    start.setBounds (b.removeFromRight (w));
}

void RenderDialog::updateGUI()
{
    auto rendering = renderer.isRendering();
    filename.setText (renderer.getOutputFile().getFullPathName(), dontSendNotification);
    filename.setEditable (! rendering);
    cancel.setEnabled (rendering);
    start.setEnabled (! rendering);

    progress = renderer.progress.load();
}

void RenderDialog::timerCallback()
{
    if (renderer.isRendering())
    {
        updateGUI();
    }
    else
    {
        updateGUI();
        stopTimer();
    }
}
