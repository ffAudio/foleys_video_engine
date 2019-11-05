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
 @class AVFormatManager

 This class will create the appropriate type of AVClip depending on the supplied file type.
 You don't need to create your own, the VideoEngine has an instance ready, that should be used.
 */
class AVFormatManager
{
public:
    enum class Formats
    {
        PROBE = 0,
        FFmpeg
    };

    AVFormatManager();

    std::shared_ptr<AVClip> createClipFromFile (VideoEngine& engine, juce::URL url, StreamTypes type = StreamTypes::all());

    std::unique_ptr<AVReader> createReaderFor (juce::File file, StreamTypes type = StreamTypes::all());

    std::unique_ptr<AVWriter> createClipWriter (juce::File file);

    void registerFactory (const juce::String& schema, std::function<AVClip*(foleys::VideoEngine& videoEngine, juce::URL url, StreamTypes type)> factory);

    juce::AudioFormatManager audioFormatManager;

private:

    std::map<juce::String, std::function<AVClip*(foleys::VideoEngine& videoEngine, juce::URL url, StreamTypes type)>> factories;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AVFormatManager)
};

} // foleys
