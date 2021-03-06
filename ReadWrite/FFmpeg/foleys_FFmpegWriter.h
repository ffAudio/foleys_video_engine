/*
 ==============================================================================

 Copyright (c) 2019 - 2021, Foleys Finest Audio - Daniel Walz
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

class FFmpegWriter : public AVWriter
{
public:

    FFmpegWriter (juce::File file, juce::String format = {});

    juce::File getMediaFile() const override;

    bool isOpenedOk() const override;

    void pushSamples (const juce::AudioBuffer<float>& input, int stream = 0) override;

    void pushImage (int64_t pos, juce::Image image, int stream = 0) override;

    int addVideoStream (const VideoStreamSettings& settings) override;

    int addAudioStream (const AudioStreamSettings& settings) override;

    bool startWriting() override;

    void finishWriting() override;

    static juce::StringArray getMuxers();

    static juce::StringArray getPixelFormats();

private:

    struct Pimpl;
    friend Pimpl;

    juce::File   mediaFile;
    juce::String formatName;
    bool         opened  = false;
    bool         started = false;

    std::unique_ptr<Pimpl> pimpl;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FFmpegWriter)
};

} // foleys
