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

class ClipDescriptor;
class ParameterAutomation;

class ProcessorController  : private juce::ValueTree::Listener
{
public:
    ProcessorController (ClipDescriptor& owner, std::unique_ptr<juce::AudioProcessor> processor);
    ProcessorController (ClipDescriptor& owner, std::unique_ptr<VideoProcessor> processor);
    ProcessorController (ClipDescriptor& owner, const juce::ValueTree& state, int index=-1);

    ~ProcessorController();

    void updateAutomation (double pts);

    void synchroniseState (ParameterAutomation& parameter);
    void synchroniseParameter (const juce::ValueTree& tree);

    juce::ValueTree& getProcessorState();

    ClipDescriptor& getOwningClipDescriptor();

    struct ProcessorAdapter
    {
        ProcessorAdapter() = default;
        virtual ~ProcessorAdapter() = default;

        virtual const juce::String getName() const = 0;
        virtual const juce::String getIdentifierString() const = 0;

        virtual VideoProcessor* getVideoProcessor() { return nullptr; }
        virtual juce::AudioProcessor* getAudioProcessor() { return nullptr; }

        virtual void createAutomatedParameters (ProcessorController& controller,
                                                std::vector<std::unique_ptr<ParameterAutomation>>& parameters,
                                                juce::ValueTree& parameterNode) = 0;
    };

    juce::AudioProcessor* getAudioProcessor();
    VideoProcessor* getVideoProcessor();

private:

    void valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
                                   const juce::Identifier& property) override;

    void valueTreeChildAdded (juce::ValueTree& parentTree,
                              juce::ValueTree& childWhichHasBeenAdded) override;

    void valueTreeChildRemoved (juce::ValueTree& parentTree,
                                juce::ValueTree& childWhichHasBeenRemoved,
                                int indexFromWhichChildWasRemoved) override;

    void valueTreeChildOrderChanged (juce::ValueTree& parentTreeWhoseChildrenHaveMoved,
                                     int oldIndex, int newIndex) override {}

    void valueTreeParentChanged (juce::ValueTree& treeWhoseParentHasChanged) override {}

    ClipDescriptor& owner;
    juce::ValueTree state;

    bool isUpdating = false;

    std::unique_ptr<ProcessorAdapter> adapter;
    std::vector<std::unique_ptr<ParameterAutomation>> parameters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorController)
};

}
