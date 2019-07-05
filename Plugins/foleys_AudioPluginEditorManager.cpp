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


AudioProcessorWindow::AudioProcessorWindow (juce::AudioProcessorEditor* editor, const juce::String& title)
  : juce::DocumentWindow (title, juce::Colours::darkgrey, juce::DocumentWindow::closeButton, true)
{
    setUsingNativeTitleBar (true);
    setResizable (editor->isResizable(), false);
    setContentOwned (editor, true);
    setVisible (true);
}

void AudioProcessorWindow::closeButtonPressed()
{
    setVisible (false);
    setContentOwned (nullptr, false);
}

//==============================================================================

AudioPluginEditorManager::AudioPluginEditorManager()
{
    juce::Desktop::getInstance().addFocusChangeListener (this);
}

AudioPluginEditorManager::~AudioPluginEditorManager()
{
    juce::Desktop::getInstance().removeFocusChangeListener (this);
}

void AudioPluginEditorManager::processorControllerAdded()
{
}

void AudioPluginEditorManager::processorControllerToBeDeleted (const ProcessorController*)
{
    
}

void AudioPluginEditorManager::globalFocusChanged (juce::Component *focusedComponent)
{
    if (focusedComponent == nullptr)
        return;
    
    if (focusedComponent->getTopLevelComponent() == mainWindow.get())
    {
        auto i = int (activeEditors.size() - 1);
        while (i >= 0)
        {
            if (activeEditors [i] == 0)
                activeEditors.erase (std::next (activeEditors.begin(), i));
            else
                activeEditors [i]->toFront (true);
            
            --i;
        }
    }
    else
    {
        auto* topLevel = focusedComponent->getTopLevelComponent();
        if (topLevel != nullptr && topLevel != mainWindow.get())
            std::remove_if (activeEditors.begin(), activeEditors.end(), [topLevel](auto& it) { return it == topLevel; });
        
        activeEditors.push_back (topLevel);
    }
}
    
}
