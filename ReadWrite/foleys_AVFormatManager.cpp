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

AVFormatManager::AVFormatManager()
{
    audioFormatManager.registerBasicFormats();
}

std::shared_ptr<AVClip> AVFormatManager::createClipFromFile (VideoEngine& engine, juce::URL url, StreamTypes type)
{
    const auto& factory = factories.find (url.getScheme());
    if (factory != factories.end())
    {
        auto clip = std::shared_ptr<foleys::AVClip> (factory->second (engine, url, type));
        return clip;
    }

    if (url.isLocalFile())
    {
        const auto file = url.getLocalFile();
        auto image = juce::ImageFileFormat::loadFrom (file);
        if (image.isValid())
        {
            auto clip = std::make_shared<ImageClip> (engine);
            clip->setImage (image);
            clip->setMediaFile (url);
            return clip;
        }

        // findFormatForFileExtension would consume some video formats as well
        // if (audioFormatManager.findFormatForFileExtension (file.getFileExtension()) != nullptr)
        if (file.hasFileExtension ("wav;aif;aiff;mp3;wma;m4a"))
        {
            if (auto* audio = audioFormatManager.createReaderFor (file))
            {
                auto clip = std::make_shared<AudioClip> (engine);
                clip->setAudioFormatReader (audio);
                clip->setMediaFile (url);
                return clip;
            }
        }

        auto reader = AVFormatManager::createReaderFor (file, type);
        if (reader->isOpenedOk())
        {
            auto clip = std::make_shared<MovieClip> (engine);
            if (reader->hasVideo())
                clip->setThumbnailReader (AVFormatManager::createReaderFor (file, StreamTypes::video()));

            clip->setReader (std::move (reader));
            return clip;
        }
    }

    return {};
}


std::unique_ptr<AVReader> AVFormatManager::createReaderFor (juce::File file, StreamTypes type)
{

#if FOLEYS_USE_FFMPEG
    return std::make_unique<FFmpegReader> (file, type);
#else
    juce::ignoreUnused (file, type);
    return {};
#endif
}

std::unique_ptr<AVWriter> AVFormatManager::createClipWriter (juce::File file)
{

#if FOLEYS_USE_FFMPEG
    auto writer = std::make_unique<FFmpegWriter>(file, "");
    return writer;
#else
    juce::ignoreUnused (file);
    return {};
#endif
}

void AVFormatManager::registerFactory (const juce::String& schema, std::function<AVClip*(foleys::VideoEngine& videoEngine, juce::URL url, StreamTypes type)> factory)
{
    factories [schema] = factory;
}


} // foleys
