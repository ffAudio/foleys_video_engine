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

namespace foleys
{

ImageClip::ImageClip (VideoEngine& engine)
  : AVClip (engine)
{
}

juce::String ImageClip::getDescription() const
{
    return mediaFile.getFileNameWithoutExtension();
}

juce::File ImageClip::getMediaFile() const
{
    return mediaFile;
}

void ImageClip::setMediaFile (const juce::File& media)
{
    mediaFile = media;
}

void ImageClip::setImage (const juce::Image& imageToUse)
{
    image = imageToUse;
    videoSettings.frameSize.width = image.getWidth();
    videoSettings.frameSize.height = image.getHeight();
}

std::pair<int64_t, juce::Image> ImageClip::getFrame (double pts) const
{
    return { convertTimecode (pts, videoSettings), image };
}

juce::Image ImageClip::getCurrentFrame() const
{
    return image;
}

juce::Image ImageClip::getStillImage (double seconds, Size size)
{
    return image.rescaled (size.width, size.height);
}

double ImageClip::getLengthInSeconds() const
{
    return std::numeric_limits<double>::max();
}

Size ImageClip::getVideoSize() const
{
    return { image.getWidth(), image.getHeight() };
}

double ImageClip::getCurrentTimeInSeconds() const
{
    return 0;
}

void ImageClip::prepareToPlay (int samplesPerBlockExpected, double sampleRateToUse)
{
    sampleRate = sampleRateToUse;
}

void ImageClip::releaseResources()
{
}

void ImageClip::getNextAudioBlock (const juce::AudioSourceChannelInfo& info)
{
    info.clearActiveBufferRegion();
}

void ImageClip::setNextReadPosition (juce::int64)
{
}

juce::int64 ImageClip::getNextReadPosition() const
{
    return 0;
}

juce::int64 ImageClip::getTotalLength() const
{
    return std::numeric_limits<juce::int64>::max();
}

bool ImageClip::isLooping() const
{
    return true;
}

void ImageClip::setLooping (bool)
{
}

std::shared_ptr<AVClip> ImageClip::createCopy()
{
    if (videoEngine == nullptr)
        return {};

    return videoEngine->createClipFromFile (getMediaFile());
}

double ImageClip::getSampleRate() const
{
    return sampleRate;
}


} // foleys
