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

class ComposedClip;

/**
 @class ClipDescriptor

 The ClipDescriptor configures the placement of each clip to be used
 in compositing the ComposedClip.
 */
struct ClipDescriptor : private juce::ValueTree::Listener
{
    ClipDescriptor (ComposedClip& owner, std::shared_ptr<AVClip> clip);

    ClipDescriptor (ComposedClip& owner, juce::ValueTree state);

    juce::String getDescription() const;
    void setDescription (const juce::String& name);

    /** start of the clip in seconds */
    double getStart() const;
    int64_t getStartInSamples() const;
    void setStart (double start);

    /** length of the clip in seconds */
    double getLength() const;
    int64_t getLengthInSamples() const;
    void setLength (double length);

    /** offset in seconds into the media */
    double getOffset() const;
    int64_t getOffsetInSamples() const;
    void setOffset (double offset);

    int getVideoLine() const;
    void setVideoLine (int line);

    int getAudioLine() const;
    void setAudioLine (int line);

    std::shared_ptr<AVClip> clip;

    juce::ValueTree& getStatusTree();

    void updateSampleCounts();

    struct AudioProcessorHolder
    {
        std::unique_ptr<juce::AudioProcessor> processor;

        std::vector<std::unique_ptr<AutomationParameter>> parameters;
    };

    void addAudioProcessor (std::unique_ptr<juce::AudioProcessor> processor, int index=-1);
    void removeAudioProcessor (int index);

    std::vector<std::unique_ptr<AudioProcessorHolder>> audioProcessors;

private:
    std::atomic<int64_t> start {0};
    std::atomic<int64_t> length {0};
    std::atomic<int64_t> offset {0};

    void valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
                                   const juce::Identifier& property) override;

    void valueTreeChildAdded (juce::ValueTree& parentTree,
                              juce::ValueTree& childWhichHasBeenAdded) override {}

    void valueTreeChildRemoved (juce::ValueTree& parentTree,
                                juce::ValueTree& childWhichHasBeenRemoved,
                                int indexFromWhichChildWasRemoved) override {}

    void valueTreeChildOrderChanged (juce::ValueTree& parentTreeWhoseChildrenHaveMoved,
                                     int oldIndex, int newIndex) override {}

    void valueTreeParentChanged (juce::ValueTree& treeWhoseParentHasChanged) override {}

    juce::ValueTree state;
    ComposedClip& owner;

    friend ComposedClip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipDescriptor)
};

} // foleys
