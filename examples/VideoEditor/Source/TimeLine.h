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

    TimeLine.h
    Created: 30 Mar 2019 4:46:00pm
    Author:  Daniel Walz

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
*/
class TimeLine    : public Component,
                    public DragAndDropTarget,
                    public FileDragAndDropTarget,
                    public TextDragAndDropTarget,
                    public foleys::TimeCodeAware::Listener,
                    public ValueTree::Listener
{
public:
    TimeLine (foleys::VideoEngine& videoEngine, Player& player, Properties& properies);
    ~TimeLine() override;

    bool isInterestedInFileDrag (const StringArray& files) override;
    void filesDropped (const StringArray& files, int x, int y) override;
    bool isInterestedInDragSource (const SourceDetails &dragSourceDetails) override;
    void itemDropped (const SourceDetails &dragSourceDetails) override;
    bool isInterestedInTextDrag (const String& text) override;
    void textDropped (const String& text, int x, int y) override;

    void mouseDown (const MouseEvent& event) override;
    bool keyPressed (const KeyPress& key) override;

    void paint (Graphics&) override;
    void resized() override;
    void timecodeChanged (int64_t count, double seconds) override;

    void setEditClip (std::shared_ptr<foleys::ComposedClip> clip);
    std::shared_ptr<foleys::ComposedClip> getEditClip() const;

    void setSelectedClip (std::shared_ptr<foleys::ClipDescriptor> clip, bool video);
    std::shared_ptr<foleys::ClipDescriptor> getSelectedClip() const;
    bool selectedClipIsVideo() const;

    void toggleVisibility();

    void spliceSelectedClipAtPlayPosition();
    void spliceSelectedClipAtPosition (double pts);

    void restoreClipComponents();

    class ClipComponent   : public Component,
                            public DragAndDropTarget,
                            private foleys::ClipDescriptor::Listener
    {
    public:
        ClipComponent (TimeLine& tl, std::shared_ptr<foleys::ClipDescriptor> clip, ThreadPool& threadPool, bool video);
        ~ClipComponent() override;

        void paint (Graphics& g) override;
        void resized() override;

        double getLeftTime() const;
        double getRightTime() const;

        void mouseMove (const MouseEvent& event) override;
        void mouseDown (const MouseEvent& event) override;
        void mouseDrag (const MouseEvent& event) override;
        void mouseUp (const MouseEvent& event) override;
        bool keyPressed (const KeyPress& key) override;

        bool isInterestedInDragSource (const SourceDetails &dragSourceDetails) override;
        void itemDragEnter (const SourceDetails &dragSourceDetails) override;
        void itemDragExit (const SourceDetails &dragSourceDetails) override;
        void itemDropped (const SourceDetails &dragSourceDetails) override;

        bool isVideoClip() const;

        std::shared_ptr<foleys::ClipDescriptor> clip;

        class ParameterGraph  : public Component,
                                public foleys::ControllableBase::Listener,
                                public foleys::ClipDescriptor::Listener
        {
        public:
            ParameterGraph (ClipComponent& owner, foleys::ParameterAutomation& automation);
            ~ParameterGraph() override;

            void setColour (juce::Colour colour);

            void paint (Graphics& g) override;

            bool hitTest (int x, int y) override;

            void mouseDown (const MouseEvent&) override;
            void mouseDrag (const MouseEvent&) override;
            void mouseUp (const MouseEvent&) override;

            void processorControllerAdded() override {}
            void processorControllerToBeDeleted (const foleys::ProcessorController*) override {}
            void parameterAutomationChanged (const foleys::ParameterAutomation*) override;

        private:
            int mapFromTime (double time) const;
            int mapFromValue (double value) const;

            double mapToTime (int x) const;
            double mapToValue (int y) const;

            int findClosestKeyFrame (int x, int y) const;

            ClipComponent& owner;
            foleys::ParameterAutomation& automation;
            int draggingIndex = -1;

            juce::Colour colour { juce::Colours::silver };

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterGraph)
        };

    private:
        void updateProcessorList();
        void updateParameterGraphs (foleys::ControllableBase&);

        void processorControllerAdded() override;
        void processorControllerToBeDeleted (const foleys::ProcessorController*) override;

        enum DragMode
        {
            notDragging,
            dragPosition,
            dragLength,
            dragOffset
        };

        TimeLine& timeline;
        std::unique_ptr<foleys::FilmStrip>  filmstrip;
        std::unique_ptr<foleys::AudioStrip> audiostrip;
        ComboBox processorSelect;

        Point<int> localDragStart;
        DragMode dragmode = notDragging;
        bool highlight    = false;
        bool wasSelected  = false;

        std::vector<std::unique_ptr<ParameterGraph>> automations;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipComponent)
    };

    int getXFromTime (double seconds) const;
    double getTimeFromX (int pixels) const;

    int getVideoLine (const std::shared_ptr<foleys::ClipDescriptor> clip) const;
    int getAudioLine (const std::shared_ptr<foleys::ClipDescriptor> clip) const;
    void setVideoLine (std::shared_ptr<foleys::ClipDescriptor> clip, int lane) const;
    void setAudioLine (std::shared_ptr<foleys::ClipDescriptor> clip, int lane) const;

    double getSampleRate() const;

    class TimeMarker : public Component
    {
    public:
        TimeMarker() = default;
        void paint (Graphics& g)
        {
            g.setColour (Colours::red);
            g.drawVerticalLine (1, 0, getHeight());
        }
    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimeMarker)
    };

    void valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
                                   const juce::Identifier& property) override;

    void valueTreeChildAdded (juce::ValueTree& parentTree,
                              juce::ValueTree& childWhichHasBeenAdded) override;

    void valueTreeChildRemoved (juce::ValueTree& parentTree,
                                juce::ValueTree& childWhichHasBeenRemoved,
                                int indexFromWhichChildWasRemoved) override;

    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override {}

    void valueTreeParentChanged (juce::ValueTree&) override {}

private:

    void addClipToEdit (std::shared_ptr<foleys::AVClip> clip, double start, int y);
    void addClipComponent (std::shared_ptr<foleys::ClipDescriptor> clip, bool video);

    foleys::VideoEngine& videoEngine;
    Player&     player;
    Properties& properties;
    TimeMarker  timemarker;

    const int numVideoLines = 2;
    const int numAudioLines = 3;
    const int videoHeight = 90;
    const int audioHeight = 90;
    const int margin = 10;

    double timelineLength = 60.0;

    std::shared_ptr<foleys::ComposedClip> edit;

    std::vector<std::unique_ptr<ClipComponent>> clipComponents;

    std::weak_ptr<foleys::ClipDescriptor> selectedClip;
    bool selectedIsVideo = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimeLine)
};
