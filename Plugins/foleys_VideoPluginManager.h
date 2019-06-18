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

/**
 The VideoPluginManager is used to create VideoProcessor instances to be placed into
 the image processing pipeline.
 */
class VideoPluginManager
{
public:
    VideoPluginManager (VideoEngine& videoEngine);

    /**
     Register a VideoProcessor factory to create a VideoProcessor instance from an identifying string.
     */
    void registerVideoProcessor (const juce::String& identifierString, std::function<std::unique_ptr<VideoProcessor>()>);

    /**
     Create an instance of a VideoProcessor from an identifying string.
     */
    std::unique_ptr<VideoProcessor> createVideoPluginInstance (const juce::String& identifierString,
                                                               juce::String& error) const;

    void populatePluginSelection (juce::PopupMenu& menu);
    juce::String getPluginDescriptionFromMenuID (int index);

private:

    VideoEngine& videoEngine;

    std::map<juce::String, std::function<std::unique_ptr<VideoProcessor>()>> factories;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VideoPluginManager)
};

} // foleys
