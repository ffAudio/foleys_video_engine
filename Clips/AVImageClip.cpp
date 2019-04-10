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

juce::String AVImageClip::getDescription() const
{
    return mediaFile.getFileNameWithoutExtension();
}

void AVImageClip::setMediaFile (const juce::File& media)
{
    mediaFile = media;
}

void AVImageClip::setImage (const juce::Image& imageToUse)
{
    image = imageToUse;
}

juce::Image AVImageClip::getFrame (const Timecode) const
{
    return image;
}

juce::Image AVImageClip::getCurrentFrame() const
{
    return image;
}

juce::Image AVImageClip::getStillImage (double seconds, Size size)
{
    return image.rescaled (size.width, size.height);
}

double AVImageClip::getLengthInSeconds() const
{
    return std::numeric_limits<double>::max();
}

Timecode AVImageClip::getFrameTimecodeForTime (double time) const
{
    return { 0, 1.0 };
}

Timecode AVImageClip::getCurrentTimecode() const
{
    return { 0, 1.0 };
}

Size AVImageClip::getVideoSize() const
{
    return { image.getWidth(), image.getHeight() };
}

double AVImageClip::getCurrentTimeInSeconds() const
{
    return 0;
}

void AVImageClip::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
}

void AVImageClip::releaseResources()
{
}

void AVImageClip::getNextAudioBlock (const juce::AudioSourceChannelInfo& info)
{
    info.clearActiveBufferRegion();
}

void AVImageClip::setNextReadPosition (juce::int64)
{
}

juce::int64 AVImageClip::getNextReadPosition() const
{
    return 0;
}

juce::int64 AVImageClip::getTotalLength() const
{
    return std::numeric_limits<juce::int64>::max();
}

bool AVImageClip::isLooping() const
{
    return true;
}

void AVImageClip::setLooping (bool)
{
}


} // foleys
