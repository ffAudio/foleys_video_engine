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

#pragma once

namespace foleys
{


class AudioProcessorWindow  : public juce::DocumentWindow
{
public:
    AudioProcessorWindow (juce::AudioProcessorEditor* editor, const juce::String& title);
    void closeButtonPressed() override;
private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioProcessorWindow)
};

//==============================================================================

class AudioPluginEditorManager  : public ClipDescriptor::Listener,
                                  public juce::FocusChangeListener
{
public:
    AudioPluginEditorManager();
    ~AudioPluginEditorManager();

    void processorControllerAdded() override;
    void processorControllerToBeDeleted (const ProcessorController*) override;

    void globalFocusChanged (juce::Component *focusedComponent) override;

private:
    std::vector<juce::Component::SafePointer<AudioProcessorWindow>> activeEditors;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginEditorManager)
};

}
