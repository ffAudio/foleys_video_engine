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

namespace IDs
{
    const juce::Identifier colour     { "Colour" };
    const juce::Identifier gain       { "gain" };
    const juce::Identifier pan        { "pan" };
    const juce::Identifier width      { "width" };
    const juce::Identifier alpha      { "alpha" };
    const juce::Identifier zoom       { "zoom" };
    const juce::Identifier translateX { "translateX" };
    const juce::Identifier translateY { "translateY" };
    const juce::Identifier rotation   { "rotation" };
}


void AVClip::addDefaultAudioParameters (AVClip& clip)
{
    auto gain  = std::make_unique<ProcessorParameterFloat>(IDs::gain, "Gain",   juce::NormalisableRange<double>(-100.0, 6.0, 0.1), -6.0);
    gain->getProperties().set (IDs::colour, "ffa0a0a0");
    clip.addAudioParameter (std::move (gain));

//    TODO:
//    auto pan   = std::make_unique<ProcessorParameterFloat>(IDs::pan,   "Panning", juce::NormalisableRange<double>(-1.0, 1.0, 0.01), 0.0);
//    auto width = std::make_unique<ProcessorParameterFloat>(IDs::width, "Width",   juce::NormalisableRange<double>(0.0, 1.0, 0.01), 1.0);
//    pan->getProperties().set (IDs::colour, "ff0000a0");
//    width->getProperties().set (IDs::colour, "ff00a000");
//    clip.addAudioParameter (std::move (pan));
//    clip.addAudioParameter (std::move (width));
}

void AVClip::addDefaultVideoParameters (AVClip& clip)
{
    auto alpha    = std::make_unique<ProcessorParameterFloat>(IDs::alpha,      "Alpha", juce::NormalisableRange<double>(0.0, 1.0, 0.1), 1.0);
    auto zoom     = std::make_unique<ProcessorParameterFloat>(IDs::zoom,       "Zoom", juce::NormalisableRange<double>(0.0, 200.0, 1.0), 100.0);
    auto transX   = std::make_unique<ProcessorParameterFloat>(IDs::translateX, "Translate X", juce::NormalisableRange<double>(-1.0, 1.0, 0.01), 0.0);
    auto transY   = std::make_unique<ProcessorParameterFloat>(IDs::translateY, "Translate Y", juce::NormalisableRange<double>(-1.0, 1.0, 0.01), 0.0);
    auto rotation = std::make_unique<ProcessorParameterFloat>(IDs::rotation,   "Rotation", juce::NormalisableRange<double>(-360.0, 360.0, 1.0), 0.0);
    alpha->getProperties().set (IDs::colour, "ffa0a0a0");
    zoom->getProperties().set (IDs::colour, "ff0000a0");
    transX->getProperties().set (IDs::colour, "ff00a0a0");
    transY->getProperties().set (IDs::colour, "ff00a000");
    rotation->getProperties().set (IDs::colour, "ffa00000");
    clip.addVideoParameter (std::move (alpha));
    clip.addVideoParameter (std::move (zoom));
    clip.addVideoParameter (std::move (transX));
    clip.addVideoParameter (std::move (transY));
    clip.addVideoParameter (std::move (rotation));
}

AVClip::AVClip (VideoEngine& videoEngineToUse) : videoEngine (&videoEngineToUse)
{
}


const ParameterMap& AVClip::getVideoParameters()
{
    return videoParameters;
}

const ParameterMap& AVClip::getAudioParameters()
{
    return audioParameters;
}

void AVClip::addAudioParameter (std::unique_ptr<ProcessorParameter> parameter)
{
    parameter->setParameterIndex (int (audioParameters.size() + 1));
    audioParameters [parameter->getParameterID()] = std::move (parameter);
}

void AVClip::addVideoParameter (std::unique_ptr<ProcessorParameter> parameter)
{
    parameter->setParameterIndex (int (videoParameters.size() + 1));
    videoParameters [parameter->getParameterID()] = std::move (parameter);
}

juce::TimeSliceClient* AVClip::getBackgroundJob()
{
    return nullptr;
}

VideoEngine* AVClip::getVideoEngine() const
{
    return videoEngine;
}

#if FOLEYS_USE_OPENGL
void AVClip::renderFrame (OpenGLView& view, VideoFrame& frame, Zoom zoomType)
{
    if (frame.image.isNull())
        return;

    auto& texture = view.getTexture (*this, frame);
    texture.bind();

    auto w      = float (texture.getWidth())  / frame.image.getWidth();
    auto h      = float (texture.getHeight()) / frame.image.getHeight();
    auto aspect = float (frame.image.getWidth())    / frame.image.getHeight();
    auto target = view.getLocalBounds();

    if (zoomType == Zoom::LetterBox)
    {
        auto targetWidth  = view.getHeight() * aspect;
        auto targetHeight = view.getWidth()  / aspect;

        if (targetWidth > view.getWidth())
            target.reduce (0, juce::roundToInt ((view.getHeight() - targetHeight) / 2.0f));

        if (targetHeight > view.getHeight())
            target.reduce (juce::roundToInt ((view.getWidth() - targetWidth) / 2.0f), 0);
    }
    // FIXME: Do other zoom types

    // FIXME: apply geometry from clips

    view.getContext().copyTexture (target,
                                   juce::Rectangle<int>(0, 0, juce::roundToInt (w * view.getWidth()), juce::roundToInt (h * view.getHeight())),
                                   view.getWidth(), view.getHeight(), false);

    texture.unbind();
}
#endif

} // foleys
