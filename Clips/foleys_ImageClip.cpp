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

namespace foleys
{

ImageClip::ImageClip (VideoEngine& engine)
  : AVClip (engine)
{
    addDefaultVideoParameters (*this);
}

juce::String ImageClip::getDescription() const
{
    if (mediaFile.isLocalFile())
        return mediaFile.getLocalFile().getFileNameWithoutExtension();

    return mediaFile.getFileName();
}

juce::URL ImageClip::getMediaFile() const
{
    return mediaFile;
}

void ImageClip::setMediaFile (const juce::URL& media)
{
    mediaFile = media;
}

void ImageClip::setImage (const juce::Image& imageToUse)
{
    frame.image = imageToUse;
    frame.timecode = 0;

    videoSettings.frameSize.width = frame.image.getWidth();
    videoSettings.frameSize.height = frame.image.getHeight();
}

VideoFrame& ImageClip::getFrame (double pts)
{
    juce::ignoreUnused (pts);
    return frame;
}

juce::Image ImageClip::getStillImage (double, Size size)
{
    return frame.image.rescaled (size.width, size.height);
}

#if FOLEYS_USE_OPENGL
void ImageClip::render (OpenGLView& view, double pts, float rotation, float zoom, juce::Point<float> translation, float alpha)
{
    juce::ignoreUnused (pts);
    renderFrame (view, frame, rotation, zoom, translation, alpha);
}
#endif

double ImageClip::getLengthInSeconds() const
{
    return std::numeric_limits<double>::max();
}

Size ImageClip::getVideoSize() const
{
    return { frame.image.getWidth(), frame.image.getHeight() };
}

double ImageClip::getCurrentTimeInSeconds() const
{
    return 0;
}

void ImageClip::prepareToPlay (int, double sampleRateToUse)
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

std::shared_ptr<AVClip> ImageClip::createCopy (StreamTypes types)
{
    if (types.test (foleys::StreamTypes::Video) == false)
        return {};

    auto* engine = getVideoEngine();
    if (engine == nullptr)
        return {};

    return engine->createClipFromFile (getMediaFile(), types);
}

double ImageClip::getSampleRate() const
{
    return sampleRate;
}


} // foleys
