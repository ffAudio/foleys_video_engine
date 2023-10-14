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

    ClipProperties.h
    Created: 30 Mar 2019 4:45:22pm
    Author:  Daniel Walz

 ==============================================================================
 */

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class AutomationComponent;
class Player;

class ClipProcessorProperties  : public Component,
                                 public ChangeListener,
                                 private foleys::ClipDescriptor::Listener
{
public:
    ClipProcessorProperties (foleys::VideoEngine& engine, std::shared_ptr<foleys::ClipDescriptor> clip, Player& player, bool video);
    ~ClipProcessorProperties() override;

    void paint (Graphics& g) override;

    void resized() override;

    void processorControllerAdded() override;
    void processorControllerToBeDeleted (const foleys::ProcessorController* toBeDeleted) override;

    void changeListenerCallback (ChangeBroadcaster*) override;

private:

    void updateEditors();

    foleys::VideoEngine& engine;
    Player& player;
    std::weak_ptr<foleys::ClipDescriptor> clip;
    std::vector<std::unique_ptr<AutomationComponent>> editors;

    TextButton processorSelect { "Add Effect" };
    Viewport   scroller;
    Component  container;

    bool video = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipProcessorProperties)
};
