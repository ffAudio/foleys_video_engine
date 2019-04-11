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


#if FOLEYS_USE_FFMPEG
namespace foleys
{

class FFmpegReader : public AVReader
{
public:
    FFmpegReader (const juce::File& file, StreamTypes type);
    virtual ~FFmpegReader();

    juce::File getMediaFile() const override;

    juce::int64 getTotalLength() const override;

    void setPosition (const juce::int64 position) override;

    juce::Image getStillImage (double seconds, Size size) override;

    void readNewData (VideoFifo&, AudioFifo&) override;

    bool hasVideo() const override;
    bool hasAudio() const override;
    bool hasSubtitle() const override;

private:
    juce::File mediaFile;

    class Pimpl;
    friend Pimpl;

    std::unique_ptr<Pimpl> pimpl;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FFmpegReader)
};

} // foleys
#endif
