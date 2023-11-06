/*
 ==============================================================================

 Copyright (c) 2021, Foleys Finest Audio - Daniel Walz
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


class MediaFoundationReader : public AVReader
{
 public:
    // clang-format off

    static constexpr auto   kMaxFramesToTryBeforeTcFound    = 40;

                            MediaFoundationReader           (const juce::File& file, StreamTypes type);

    juce::File              getMediaFile                    () const override;
    juce::int64             getTotalLength                  () const override;
    double                  getLengthInSeconds              () const override;
    void                    setPosition                     (const int64_t position) override;

    juce::Image             getStillImage                   (double seconds, Size size) override;

    void                    readNewData                     (VideoFifo&, AudioFifo&) override;

    bool                    hasVideo                        () const override { return getNumVideoStreams() > 0; }
    bool                    hasAudio                        () const override { return getNumAudioStreams() > 0; }
    bool                    hasSubtitle                     () const override { return false; }

    void                    setOutputSampleRate             (double sampleRate) override;

    int                     getNumVideoStreams              () const override;
    VideoStreamSettings     getVideoSettings                (int streamIndex) const override;
    int                     getNumAudioStreams              () const override;
    AudioStreamSettings     getAudioSettings                (int streamIndex) const override;

 private:

    juce::File              mediaFile;

    struct                  Pimpl;
    std::unique_ptr<Pimpl>  pimpl;

    // clang-format on
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MediaFoundationReader)
};


struct MediaFoundationFormat : public AVFormat
{
    MediaFoundationFormat()
    {
        initialise();
    }
    ~MediaFoundationFormat() override
    {
        shutdown();
    }

    bool canRead (juce::File file) override
    {
        juce::ignoreUnused (file);
        return true;
    }

    std::unique_ptr<AVReader> createReaderFor (juce::File file, StreamTypes type = StreamTypes::all())
    {
        return std::make_unique<MediaFoundationReader> (file, type);
    }

    bool canWrite (juce::File file) override
    {
        juce::ignoreUnused (file);
        return false;
    }

    std::unique_ptr<AVWriter> createWriterFor (juce::File file, StreamTypes type = StreamTypes::all()) override
    {
        juce::ignoreUnused (type);
        juce::ignoreUnused (file);
        return {};
    }

 private:
    void initialise();
    void shutdown();

    bool wasInitialised = false;
};

}  // namespace foleys