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


#define NOMINMAX

// clang-format off
#include <propvarutil.h>
#include <mfapi.h>
#include <mfidl.h>
#include <shlwapi.h>
#include <mfreadwrite.h>
#include <combaseapi.h>
#include <Mferror.h>
// clang-format on

namespace foleys
{

class VideoBufferLock
{
 public:
    VideoBufferLock (IMFMediaBuffer* buffer)
      : mediaBuffer2D (NULL)
      , isBlocked (FALSE)
    {
        mediaBuffer = buffer;                                                    // TODO: - mh: Don't need to addRef?
        mediaBuffer->QueryInterface (IID_IMF2DBuffer, (void**) &mediaBuffer2D);  // Try to get a 2D buffer. Not a problem if that fails.
    }

    ~VideoBufferLock()
    {
        unlock();
    }

    HRESULT lock (LONG   defaultStride,  // Minimum stride (with no padding).
                  DWORD  height,         // Height of the image, in pixels.
                  BYTE** dataPointer,    // Receives a pointer to the start of scan line 0.
                  LONG*  actualStride    // Receives the actual stride.
    )
    {
        HRESULT hr = S_OK;
        numBytes   = 0;

        // Use the 2-D version if available...
        if (mediaBuffer2D)
        {
            hr = mediaBuffer2D->Lock2D (dataPointer, actualStride);
            if (SUCCEEDED (hr))
            {
                hr = mediaBuffer2D->GetContiguousLength (&numBytes);
                if (FAILED (hr)) return hr;
            }
        }
        else
        {
            // Use non-2D version.
            BYTE* rawData = NULL;

            hr = mediaBuffer->Lock (&rawData, NULL, &numBytes);
            if (SUCCEEDED (hr))
            {
                *actualStride = defaultStride;
                if (defaultStride < 0)
                {
                    // Bottom-up orientation. Return a pointer to the start of the
                    // last row *in memory* which is the top row of the image.
                    *dataPointer = rawData + abs (defaultStride) * (height - 1);
                }
                else
                {
                    // Top-down orientation. Return a pointer to the start of the
                    // buffer.
                    *dataPointer = rawData;
                }
            }
        }

        isBlocked = (SUCCEEDED (hr));

        return hr;
    }

    void unlock()
    {
        if (isBlocked)
        {
            if (mediaBuffer2D) { (void) mediaBuffer2D->Unlock2D(); }
            else
            {
                (void) mediaBuffer->Unlock();
            }
            isBlocked = FALSE;
        }
    }

    DWORD getNumBytes() const noexcept
    {
        return numBytes;
    }

 private:
    IMFMediaBuffer* mediaBuffer;
    IMF2DBuffer*    mediaBuffer2D;
    BOOL            isBlocked = FALSE;
    DWORD           numBytes  = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VideoBufferLock)
};


struct MediaFoundationReader::Pimpl
{
    // clang-format off
                                        Pimpl                       (MediaFoundationReader& myOwner, const juce::File& file, StreamTypes type);
                                        ~Pimpl                      () { }

    void                                setPosition                 (juce::int64 pos);   
    [[nodiscard]] double                getLengthInSeconds          ();
    [[nodiscard]] juce::int64           getTotalLength              ();

    [[nodiscard]] int                   getNumVideoStreams          () const noexcept { return (videoStreamIndex >= 0 ? 1 : 0 ); }
    [[nodiscard]] int                   getNumAudioStreams          () const noexcept { return (audioStreamIndex >= 0 ? 1 : 0 ); }
    [[nodiscard]] VideoStreamSettings   getVideoSettings            (int streamIndex) const noexcept { juce::ignoreUnused (streamIndex); return videoSettings; }
    [[nodiscard]] AudioStreamSettings   getAudioSettings            (int streamIndex) const noexcept { juce::ignoreUnused (streamIndex); return audioSettings; }
    
    [[nodiscard]] juce::Image           getStillImage               (double seconds, Size size);
    void                                readNewData                 (VideoFifo&, AudioFifo&);

    void                                setOutputSampleRate         (double sr);

private: 
    
    MediaFoundationReader&              owner;
    MFScopedPointer<IMFSourceReader>    sourceReader;
    juce::AudioBuffer<float>            audioOutBuffer;

    // Stream index of the first audio and video stream (-1 means not set/found)...
    int                                 videoStreamIndex            { -1 };
    int                                 audioStreamIndex            { -1 };

    juce::int64                         stride                      { 0 }; // Total size (in bytes) of 1 line of video pixels.
    foleys::VideoStreamSettings         videoSettings               {};
    foleys::AudioStreamSettings         audioSettings               {};
   
    // Audio and video decoding ...
    HRESULT                             findTypeForAllStreams       ();
    HRESULT                             findTypeForStream           (DWORD streamIndex);   
    HRESULT                             fillVideoSettings           (IMFMediaType* mediaType);     
    HRESULT                             fillAudioSettings           (IMFMediaType* mediaType);
    HRESULT                             configureDecoder            (DWORD dwStreamIndex, foleys::Size outputSize, int outputSampleRate);   
    
    // Actual 'sample' reading ...
    HRESULT                             readSample                  (DWORD streamIdx, IMFSample** sample, LONGLONG& timeStamp, DWORD& actualStreamIdx);
    HRESULT                             fillFrameWithSample         (IMFSample* sample, VideoFrame& frame, foleys::Size frameSize);
    HRESULT                             fillBufferWithSample        (IMFSample* sample, juce::AudioBuffer<float>& buffer);
    
    // Helper methods (maybe extract) ...
    [[nodiscard]] juce::Image           resizeImageKeepingAspect    (const juce::Image& source, foleys::Size destinationSize, foleys::Aspect aspectOptions);
    HRESULT                             getDefaultStride            (IMFMediaType* pType, LONG* plStride);    
    HRESULT                             getSourceFlags              (ULONG* pulFlags);
    [[nodiscard]] bool                  sourceCanSeek               ();
    [[nodiscard]] HRESULT               setPosition_hns             (juce::int64 pos_hns);

    // clang-format on
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pimpl)
};


MediaFoundationReader::MediaFoundationReader (const juce::File& file, foleys::StreamTypes type)
{
    mediaFile = file;
    pimpl     = std::make_unique<Pimpl> (*this, file, type);
}

juce::File MediaFoundationReader::getMediaFile() const
{
    return mediaFile;
}

juce::int64 MediaFoundationReader::getTotalLength() const
{
    return pimpl->getTotalLength();
}

double MediaFoundationReader::getLengthInSeconds() const
{
    return pimpl->getLengthInSeconds();
}

void MediaFoundationReader::setPosition (const int64_t position)
{
    pimpl->setPosition (position);
}

void MediaFoundationReader::readNewData (VideoFifo& videoFifo, AudioFifo& audioFifo)
{
    pimpl->readNewData (videoFifo, audioFifo);
}

void MediaFoundationReader::setOutputSampleRate (double sr)
{
    jassert (sr > 0.0);
    sampleRate = sr;

    pimpl->setOutputSampleRate (sampleRate);
}

juce::Image MediaFoundationReader::getStillImage (double seconds, Size size)
{
    return pimpl->getStillImage (seconds, size);
}

int MediaFoundationReader::getNumVideoStreams() const
{
    return pimpl->getNumVideoStreams();
}

VideoStreamSettings MediaFoundationReader::getVideoSettings (int streamIndex) const
{
    return pimpl->getVideoSettings (streamIndex);
}

int MediaFoundationReader::getNumAudioStreams() const
{
    return pimpl->getNumAudioStreams();
}

AudioStreamSettings MediaFoundationReader::getAudioSettings (int streamIndex) const
{
    return pimpl->getAudioSettings (streamIndex);
}

void MediaFoundationFormat::initialise()
{
    // Weirdly it works without this, but according to the docs it should be necessary:
    // https://docs.microsoft.com/en-us/windows/win32/api/mfreadwrite/nf-mfreadwrite-mfcreatesourcereaderfromurl#remarks
    wasInitialised = false;
    auto hr        = CoInitializeEx (nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED (hr))
    {
        hr             = MFStartup (MF_VERSION);
        wasInitialised = SUCCEEDED (hr);
    }
    jassert (wasInitialised);
}

void MediaFoundationFormat::shutdown()
{
    MFShutdown();

    if (wasInitialised) CoUninitialize();
}


// ==============================================================================
#pragma region Pimpl

MediaFoundationReader::Pimpl::Pimpl (MediaFoundationReader& myOwner, const juce::File& file, [[maybe_unused]] StreamTypes types)
  : owner (myOwner)
{
    // Configure the source reader to perform video processing.
    //
    // This includes:
    //   - YUV to RGB-32
    //   - Software de-interlace
    // https://docs.microsoft.com/en-us/windows/win32/api/mfreadwrite/nf-mfreadwrite-imfsourcereader-setcurrentmediatype#remarks
    MFScopedPointer<IMFAttributes> attributes;
    auto                           hr       = MFCreateAttributes (attributes.getPointer(), 1);  // Get attribute object ...
    bool                           openedOk = SUCCEEDED (hr);
    hr                                      = attributes->SetUINT32 (MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);  // Set attribute object ...
    openedOk &= SUCCEEDED (hr);

    // Create source reader ...
    hr = MFCreateSourceReaderFromURL (file.getFullPathName().toWideCharPointer(), attributes.get(), sourceReader.getPointer());
    openedOk &= SUCCEEDED (hr);
    if (openedOk)
    {
        openedOk &= (sourceReader);   // Check there is actually a valid source pointer.
        openedOk &= sourceCanSeek();  // Check that the source is able to seek (not a stream, etc... ) TODO: - or check in setPosition?

        // Enumerate all the media streams in the file...
        hr = findTypeForAllStreams();
        openedOk &= SUCCEEDED (hr);
        openedOk &= (videoStreamIndex >= 0 || audioStreamIndex >= 0);  // There should be at least one audio or video stream present.

        // Configure audio and video decoders ...
        if (videoStreamIndex != -1) hr = configureDecoder (videoStreamIndex, {}, 0);
        openedOk &= SUCCEEDED (hr);
    }

    owner.opened = openedOk;
    jassert (openedOk);  // Problem opening file!
}


void MediaFoundationReader::Pimpl::setPosition (juce::int64 position)
{
    jassert (owner.sampleRate > 0.0);
    if (sourceReader && owner.sampleRate > 0.0)
    {
        LONGLONG hnsPosition = 0;
        hnsPosition          = static_cast<LONGLONG> ((position / owner.sampleRate) * 10000000.0);

        [[maybe_unused]] auto hr = setPosition_hns (hnsPosition);
        jassert (SUCCEEDED (hr));
    }
}

HRESULT MediaFoundationReader::Pimpl::setPosition_hns (juce::int64 pos_hns)
{
    if (!sourceReader) return E_FAIL;

    PROPVARIANT var;
    auto        hr = InitPropVariantFromInt64 (pos_hns, &var);
    if (FAILED (hr)) return hr;

    hr = sourceReader->SetCurrentPosition (GUID_NULL, var);
    jassert (SUCCEEDED (hr));

    hr = PropVariantClear (&var);
    if (FAILED (hr)) return hr;

    return S_OK;
}

double MediaFoundationReader::Pimpl::getLengthInSeconds()
{
    if (!sourceReader) return 0.0;

    PROPVARIANT var;
    PropVariantInit (&var);

    // Get 'presentation' duration ...
    // https://docs.microsoft.com/en-us/windows/win32/medfound/presentation-descriptor-attributes
    auto hr = sourceReader->GetPresentationAttribute ((DWORD) MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var);
    if (SUCCEEDED (hr))
    {
        // Specifies the duration in 100 nanosecond units.
        LONGLONG hnsDuration;
        hr = PropVariantToInt64 (var, &hnsDuration);
        if (SUCCEEDED (hr))
        {
            PropVariantClear (&var);

            return static_cast<double> (hnsDuration) / 10000000.0;
        }
        else
            jassertfalse;  // not SUCCEEDED
    }
    else
        jassertfalse;  // not SUCCEEDED

    return 0.0;
}

juce::int64 MediaFoundationReader::Pimpl::getTotalLength()
{
    if (owner.sampleRate > 0.0) return static_cast<juce::int64> (getLengthInSeconds() * owner.sampleRate);
    return 0;
}


HRESULT MediaFoundationReader::Pimpl::getSourceFlags (ULONG* pulFlags)
{
    if (!sourceReader) return E_FAIL;

    HRESULT     hr = E_FAIL;
    PROPVARIANT var;
    PropVariantInit (&var);
    hr = sourceReader->GetPresentationAttribute ((DWORD) MF_SOURCE_READER_MEDIASOURCE, MF_SOURCE_READER_MEDIASOURCE_CHARACTERISTICS, &var);

    ULONG flags = 0;
    if (SUCCEEDED (hr))
    {
        hr = PropVariantToUInt32 (var, &flags);

        if (SUCCEEDED (hr))
            *pulFlags = flags;
        else
            jassertfalse;  // not SUCCEEDED
    }
    else
        jassertfalse;  // not SUCCEEDED

    PropVariantClear (&var);

    return hr;
}

bool MediaFoundationReader::Pimpl::sourceCanSeek()
{
    bool  canSeek = false;
    ULONG flags;
    if (SUCCEEDED (getSourceFlags (&flags))) canSeek = ((flags & MFMEDIASOURCE_CAN_SEEK) == MFMEDIASOURCE_CAN_SEEK);
    return canSeek;
}


HRESULT MediaFoundationReader::Pimpl::findTypeForAllStreams()
{
    if (!sourceReader) return E_FAIL;

    HRESULT hr        = S_OK;
    int     streamIdx = 0;
    videoStreamIndex  = -1;  // Video and audio stream index has not been set. So it's negative, because 0 is a valid stream index.
    audioStreamIndex  = -1;  // So long as we only deal with the first audio and video stream.
    while (SUCCEEDED (hr) && (videoStreamIndex == -1 || audioStreamIndex == -1))  // Also bail if we have one video and audio stream...
    {
        hr = findTypeForStream (streamIdx++);
        if (hr == MF_E_INVALIDSTREAMNUMBER) return S_OK;  // This is a valid return and only way to get all streams...
    }
    return hr;
}

HRESULT MediaFoundationReader::Pimpl::findTypeForStream (DWORD streamIndex)
{
    if (!sourceReader) return E_FAIL;

    HRESULT hr             = S_OK;
    DWORD   mediaTypeIndex = 0;


    // Loop through all possible media types ...
    while (SUCCEEDED (hr))
    {
        // Check if the type at mediaTypeIndex matches ...
        MFScopedPointer<IMFMediaType> mediaType;

        hr = sourceReader->GetNativeMediaType (streamIndex, mediaTypeIndex, mediaType.getPointer());
        if (hr == MF_E_NO_MORE_TYPES)  // No more types to try. Give up...
        {
            return S_OK;
        }
        else if (SUCCEEDED (hr))  // Type is recognized!
        {
            // Get the major media type (video, audio, etc...)
            GUID majorType {};
            hr = mediaType->GetMajorType (&majorType);

            // Only get video settings from the first video stream ...
            if (majorType == MFMediaType_Video && videoStreamIndex == -1)
            {
                videoStreamIndex = streamIndex;
                hr               = fillVideoSettings (mediaType.get());
                if (FAILED (hr)) return hr;

                DBG ("File video stream: " << videoSettings.toString() << " at stream index: " << juce::String (streamIndex) << ".");
            }
            // ... and audio settings from the first audio streams ...
            else if (majorType == MFMediaType_Audio && audioStreamIndex == -1)
            {
                audioStreamIndex = streamIndex;
                hr               = fillAudioSettings (mediaType.get());
                if (FAILED (hr)) return hr;

                DBG ("File audio stream: " << audioSettings.toString() << " at stream index: " << juce::String (streamIndex) << ".");
            }

            return hr;
        }
        ++mediaTypeIndex;
    }
    return hr;
}


HRESULT MediaFoundationReader::Pimpl::fillVideoSettings (IMFMediaType* mediaType)
{
    if (!mediaType) return E_FAIL;

    HRESULT hr = S_OK;

    // Make sure it is a video format ...
    GUID majorType {};
    hr = mediaType->GetMajorType (&majorType);
    if (FAILED (hr)) return hr;
    if (majorType != MFMediaType_Video) return E_UNEXPECTED;

    // Get the frame size ...
    UINT32 width, height = 0;
    hr = MFGetAttributeSize (mediaType, MF_MT_FRAME_SIZE, &width, &height);
    if (FAILED (hr)) return hr;
    if (width > 0 && height > 0)
        videoSettings.frameSize = { static_cast<int> (width), static_cast<int> (height) };
    else
        jassertfalse;  // Something wrong with the frame size!

    // Get the pixel aspect ratio. (This value might not be set.)
    MFRatio pixelAspectRatio = { 0, 0 };
    hr = MFGetAttributeRatio (mediaType, MF_MT_PIXEL_ASPECT_RATIO, (UINT32*) &pixelAspectRatio.Numerator, (UINT32*) &pixelAspectRatio.Denominator);
    // TODO: - mh: Handle invalid or non-square pixels!
    // if (FAILED (hr) || pixelAspectRatio.Denominator != pixelAspectRatio.Numerator) jassertfalse;

    // Get frame rate ...
    UINT32 frameRateNumerator, frameRateDenominator;
    hr = MFGetAttributeRatio (mediaType, MF_MT_FRAME_RATE, &frameRateNumerator, &frameRateDenominator);
    if (SUCCEEDED (hr) && frameRateDenominator != 0)
    {
        videoSettings.timebase        = 10000000;
        videoSettings.defaultDuration = static_cast<int> (std::round (10000000.0 * (frameRateDenominator / static_cast<double> (frameRateNumerator))));
    }
    else
        jassertfalse;

    // Get the stride to find out if the bitmap is top-down or bottom-up ...
    LONG defaultStride = 0;
    hr                 = getDefaultStride (mediaType, &defaultStride);
    if (SUCCEEDED (hr)) stride = defaultStride;

    return S_OK;
}

HRESULT MediaFoundationReader::Pimpl::fillAudioSettings (IMFMediaType* mediaType)
{
    if (!mediaType) return E_FAIL;

    HRESULT hr = S_OK;

    // Make sure it is a audio format ...
    GUID majorType {};
    hr = mediaType->GetMajorType (&majorType);
    if (FAILED (hr)) return hr;
    if (majorType != MFMediaType_Audio) return E_UNEXPECTED;

    audioSettings.numChannels   = static_cast<int> (MFGetAttributeUINT32 (mediaType, MF_MT_AUDIO_NUM_CHANNELS, 0));
    audioSettings.timebase      = static_cast<int> (MFGetAttributeUINT32 (mediaType, MF_MT_AUDIO_SAMPLES_PER_SECOND, 0));
    audioSettings.bitsPerSample = static_cast<int> (MFGetAttributeUINT32 (mediaType, MF_MT_AUDIO_BITS_PER_SAMPLE, 0));

    return hr;
}


HRESULT MediaFoundationReader::Pimpl::getDefaultStride (IMFMediaType* mediaType, LONG* defaultStride)
{
    LONG tempStride = 0;

    // Try to get the default stride from the media type...
    HRESULT hr = mediaType->GetUINT32 (MF_MT_DEFAULT_STRIDE, (UINT32*) &tempStride);

    if (FAILED (hr))
    {
        // Attribute not set. Try to calculate the default stride...

        // Get the subtype ...
        GUID subtype = GUID_NULL;
        hr           = mediaType->GetGUID (MF_MT_SUBTYPE, &subtype);
        if (FAILED (hr)) return hr;

        // Get the frame size ...
        UINT32 width  = 0;
        UINT32 height = 0;
        hr            = MFGetAttributeSize (mediaType, MF_MT_FRAME_SIZE, &width, &height);
        if (FAILED (hr)) return hr;

        hr = MFGetStrideForBitmapInfoHeader (subtype.Data1, width, &tempStride);
        if (FAILED (hr)) return hr;

        // Set the attribute for later reference ...
        (void) mediaType->SetUINT32 (MF_MT_DEFAULT_STRIDE, UINT32 (tempStride));
    }

    if (SUCCEEDED (hr)) *defaultStride = tempStride;


    return hr;
}

void MediaFoundationReader::Pimpl::setOutputSampleRate (double sr)
{
    if (audioStreamIndex < 0) 
        return;

    configureDecoder (audioStreamIndex, {}, juce::roundToInt (sr));
}


HRESULT MediaFoundationReader::Pimpl::configureDecoder (DWORD streamIndex, foleys::Size outputSize, int outputSampleRate)
{
    if (!sourceReader) return E_FAIL;

    MFScopedPointer<IMFMediaType> inputType;
    MFScopedPointer<IMFMediaType> outputType;

    // Find the native format of the stream ...
    HRESULT hr = sourceReader->GetNativeMediaType (streamIndex, 0, inputType.getPointer());
    if (FAILED (hr)) return hr;

    // Find the major type ...
    GUID majorType {};
    hr = inputType->GetGUID (MF_MT_MAJOR_TYPE, &majorType);
    if (FAILED (hr)) return hr;

    // Define the output type ...
    hr = MFCreateMediaType (outputType.getPointer());
    if (FAILED (hr)) return hr;

    // Set the destination major type (same as native one) ...
    hr = outputType->SetGUID (MF_MT_MAJOR_TYPE, majorType);
    if (FAILED (hr)) return hr;

    // Select a subtype for the destination type ...
    if (majorType == MFMediaType_Video)
    {
        // RGB32 for video ...
        hr = outputType->SetGUID (MF_MT_SUBTYPE, MFVideoFormat_RGB32);
        if (FAILED (hr)) return hr;

        if (!outputSize.isEmpty()) 
        { 
            hr = MFSetAttributeSize (outputType.get(), MF_MT_FRAME_SIZE, outputSize.width, outputSize.height);
            if (FAILED (hr)) return hr;
        }
    }
    else if (majorType == MFMediaType_Audio)
    {
        // PCM for audio ...
        hr = outputType->SetGUID (MF_MT_SUBTYPE, MFAudioFormat_PCM);
        if (FAILED (hr)) return hr;

        // Select output sample rate to re-sample ...
        if (outputSampleRate > 0)
        {
            hr = outputType->SetUINT32 (MF_MT_AUDIO_SAMPLES_PER_SECOND, outputSampleRate);
            if (FAILED (hr)) return hr;
        }
    }
    else
    {
        // Unrecognized type. Skip.
        return hr;
    }

    // Set the uncompressed format.
    hr = sourceReader->SetCurrentMediaType (streamIndex, NULL, outputType.get());
    if (FAILED (hr)) return hr;

    // Can only get a correct format after it has been set with 'SetCurrentMediaType'
    MFScopedPointer<IMFMediaType> actualType;
    if (majorType == MFMediaType_Video)
    {
        hr = sourceReader->GetCurrentMediaType ((DWORD) MF_SOURCE_READER_FIRST_VIDEO_STREAM, actualType.getPointer());
        if (FAILED (hr)) return hr;

        hr = fillVideoSettings (actualType.get());
        if (FAILED (hr)) return hr;

        DBG ("Decoder video output settings: " << videoSettings.toString() << ".");
    }
    else if (majorType == MFMediaType_Audio)
    {
        hr = sourceReader->GetCurrentMediaType ((DWORD) MF_SOURCE_READER_FIRST_AUDIO_STREAM, actualType.getPointer());
        if (FAILED (hr)) return hr;

        hr = fillAudioSettings (actualType.get());
        if (FAILED (hr)) return hr;

        DBG ("Decoder audio output settings: " << audioSettings.toString() << ".");
    }

    // Select the first video stream ...
    hr = sourceReader->SetStreamSelection ((DWORD) MF_SOURCE_READER_ALL_STREAMS, FALSE);  // De-select all stream ...
    jassert (SUCCEEDED (hr));
    hr = sourceReader->SetStreamSelection ((DWORD) MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);  // Ensure the video stream is selected ...
    hr = sourceReader->SetStreamSelection ((DWORD) MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);

    return S_OK;
}


[[nodiscard]] juce::Image MediaFoundationReader::Pimpl::resizeImageKeepingAspect (const juce::Image& source, foleys::Size destinationSize, foleys::Aspect aspectOptions)
{
    juce::Image output (juce::Image::PixelFormat::ARGB, destinationSize.width, destinationSize.height, true);
    // TODO: - Extract letterbox/resizing from AVClip::renderFrame.
    if (aspectOptions == foleys::Aspect::LetterBox)
    {
        // Find scaling factor ...
        const auto widthRatio    = destinationSize.width / static_cast<double> (source.getWidth());
        auto       desiredHeight = static_cast<int> (widthRatio * static_cast<double> (source.getHeight()));

        // Scale the image while maintaining it's aspect ratio ...
        auto scaledImage = source.rescaled (destinationSize.width, desiredHeight, juce::Graphics::ResamplingQuality::lowResamplingQuality);

        // Caluclate 'gap' (the letterboxed area).
        const auto gap = static_cast<int> (static_cast<double> (destinationSize.height - desiredHeight) / 2.0);
        if (gap < 0.0) return {};  // No portrait ratios allowed so far ...

        // The bitmap data objects needed for scaling ...
        juce::Image::BitmapData scaledData (scaledImage, juce::Image::BitmapData::readOnly);
        juce::Image::BitmapData outputData (output, juce::Image::BitmapData::writeOnly);

        // Check if the scaling and copying can be done safely ...
        bool conversionSafe = true;
        conversionSafe &= (scaledData.lineStride == outputData.lineStride);    // Line stride's should match.
        conversionSafe &= (scaledData.pixelStride == outputData.pixelStride);  // Pixel stride should match (32bpp).
        conversionSafe &= (gap < (output.getHeight() / 2) - 1);                // Gap (letterbox area) is valid.
        jassert (conversionSafe);
        if (conversionSafe)
        {
            // Copy the scaled image to the output ...
            for (int y = 0; y < scaledImage.getHeight(); ++y)
                memcpy (outputData.getLinePointer (y + gap), scaledData.getLinePointer (y), static_cast<size_t> (scaledData.lineStride));
        }
    }
    return output;
}


[[nodiscard]] juce::Image MediaFoundationReader::Pimpl::getStillImage (double seconds, foleys::Size size)
{
    if (size.isEmpty()) return {};

    HRESULT hr = S_OK;

    // Go to the correct time position ...
    auto pos_hns = static_cast<juce::int64> (seconds * 10000000.0);  // Convert the desired timecode to hecto-nano-seconds.
    hr           = setPosition_hns (pos_hns);                        // Seek to correct position (or just prior).
    if (FAILED (hr))
    {
        DBG ("Problem reading next sample!");
        jassertfalse;
        return {};
    }

    // Find the first frame after the requested time ...
    MFScopedPointer<IMFSample> sample;
    DWORD                      actualStreamIndex         = 0;
    juce::int64                timeStamp                 = 0;
    int                        framesToTryBeforeAborting = kMaxFramesToTryBeforeTcFound;
    while (SUCCEEDED (hr) && timeStamp < pos_hns && framesToTryBeforeAborting-- > 0)
    {
        sample.reset();
        hr = readSample ((DWORD) MF_SOURCE_READER_FIRST_VIDEO_STREAM, sample.getPointer(), timeStamp, actualStreamIndex);
        if (FAILED (hr))
        {
            DBG ("Problem reading next sample!");
            jassertfalse;
            return {};
        }
    }

    if (sample)
    {
        // Handle video stream ...
        if (static_cast<int> (actualStreamIndex) == videoStreamIndex)
        {
            VideoFrame frame {};
            hr = fillFrameWithSample (sample.get(), frame, videoSettings.frameSize);
            if (FAILED (hr))
            {
                DBG ("Problem filling next video frame!");
                jassertfalse;
            }
            return resizeImageKeepingAspect (frame.image, size, foleys::Aspect::LetterBox);
        }
    }

    return {};
}



void MediaFoundationReader::Pimpl::readNewData (VideoFifo& videoFifo, AudioFifo& audioFifo)
{
    // Read a audio buffer ...
    MFScopedPointer<IMFSample> sample;
    DWORD                      actualStreamIndex = 0;
    LONGLONG                   actualTimeStamp   = 0;
    auto                       hr                = readSample ((DWORD) MF_SOURCE_READER_ANY_STREAM, sample.getPointer(), actualTimeStamp, actualStreamIndex);
    if (FAILED (hr))
    {
        DBG ("Problem reading next sample!");
        jassertfalse;
        return;
    }

    if (sample)
    {
        // Synchronization ...
        auto factor          = owner.sampleRate / 10000000.0;
        auto actualSamplePos = juce::int64 (actualTimeStamp * factor);
        auto writePosition   = audioFifo.getWritePosition();
        int  outOfSync       = static_cast<int> (writePosition - actualSamplePos);

        const auto timeStamp_secs = actualTimeStamp / 10000000.0;

        // Handle audio stream ...
        if (actualStreamIndex == static_cast<DWORD> (audioStreamIndex))
        {
            hr = fillBufferWithSample (sample.get(), audioOutBuffer);

            if (outOfSync < 0)
            {
                DBG ("Audio stream ( timestamp: " << timeStamp_secs << " s. Out of sync: " << outOfSync << " samples).");
                audioFifo.pushSilence (-outOfSync);
                audioFifo.pushSamples (audioOutBuffer);
            }
            else if (outOfSync > 0)
            {
                DBG ("Audio stream ( timestamp: " << timeStamp_secs << " s. Out of sync: " << outOfSync << " samples).");
                auto samplesLeft = audioOutBuffer.getNumSamples() - outOfSync;

                if (samplesLeft > 0)
                {
                    juce::AudioBuffer<float> subBuffer (audioOutBuffer.getArrayOfWritePointers(), audioOutBuffer.getNumChannels(), outOfSync,
                                                        audioOutBuffer.getNumSamples() - outOfSync);
                    audioFifo.pushSamples (subBuffer);
                }
            }
            else
            {
                audioFifo.pushSamples (audioOutBuffer);
            }
        }
        // Handle video stream ...
        else if (static_cast<int> (actualStreamIndex) == videoStreamIndex)
        {
            auto& frame = videoFifo.getWritingFrame();
            hr          = fillFrameWithSample (sample.get(), frame, videoSettings.frameSize);
            if (FAILED (hr))
            {
                DBG ("Problem filling next video frame!");
                jassertfalse;
            }
            // timeStamp = frame.timecode;
            videoFifo.finishWriting();
        }
    }
}

HRESULT MediaFoundationReader::Pimpl::readSample (DWORD streamIdx, IMFSample** sample, LONGLONG& timeStamp, DWORD& actualStreamIdx)
{
    if (!sourceReader) return E_FAIL;

    HRESULT hr    = S_OK;
    DWORD   flags = 0;
    hr            = sourceReader->ReadSample (streamIdx,         // Stream index.
                                   0,                 // Flags.
                                   &actualStreamIdx,  // Receives the actual stream index.
                                   &flags,            // Receives status flags.
                                   &timeStamp,        // Receives the time stamp.
                                   sample             // Receives the sample or NULL.
    );
    if (FAILED (hr)) return hr;

    if (flags & MF_SOURCE_READERF_ENDOFSTREAM) return hr;  // Exit on end of stream ...

#if JUCE_DEBUG
    if (flags & MF_SOURCE_READERF_NEWSTREAM) DBG ("New stream.");
    if (flags & MF_SOURCE_READERF_NATIVEMEDIATYPECHANGED) DBG ("Decoder input type changed.");
    if (flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED) DBG ("Decoder output type changed.");
    if (flags & MF_SOURCE_READERF_STREAMTICK) DBG ("Stream tick.");
#endif

    // The format changed. Reconfigure the decoder ...
    if (flags & MF_SOURCE_READERF_NATIVEMEDIATYPECHANGED)
    {
        hr = configureDecoder (actualStreamIdx, {}, 0);
        if (FAILED (hr)) return hr;
    }

    return hr;
}


HRESULT MediaFoundationReader::Pimpl::fillFrameWithSample (IMFSample* sample, VideoFrame& frame, foleys::Size frameSize)
{
    if (!sample) return E_FAIL;
    // Get buffer ...
    MFScopedPointer<IMFMediaBuffer> mediaBuffer;
    auto                            hr = sample->ConvertToContiguousBuffer (mediaBuffer.getPointer());
    if (FAILED (hr)) return hr;

    // Get and check default duration ...
    LONGLONG defaultDuration_hns = 0;
    hr                           = sample->GetSampleDuration (&defaultDuration_hns);
    if (FAILED (hr)) return hr;
    // jassert (videoSettings.defaultDuration == defaultDuration_hns);

    // Get time-stamp ...
    juce::int64 time = 0;
    hr               = sample->GetSampleTime (&time);
    frame.timecode   = time;
    if (FAILED (hr)) return hr;

    // Lock the buffer and get a pointer to the data ...
    BYTE*           scanLine0;
    LONG            actualStride = 0;
    VideoBufferLock bufferLock (mediaBuffer.get());
    hr = bufferLock.lock (static_cast<LONG> (stride), static_cast<DWORD> (videoSettings.frameSize.height), &scanLine0, &actualStride);
    if (FAILED (hr)) return hr;
    if (scanLine0 == nullptr) return E_FAIL;

    // Get number of bytes ...
    DWORD        numBytesInBuffer = bufferLock.getNumBytes();
    juce::int64  pitch            = std::abs (stride);
    const double duration_frame   = numBytesInBuffer / static_cast<double> (pitch * videoSettings.frameSize.height);
    if (numBytesInBuffer < (pitch * videoSettings.frameSize.height))
    {
        jassertfalse;  // Not enough data available!
        return E_FAIL;
    }

    // Check if buffer contains a valid image ...
    if (frame.image.isNull() || frame.image.getWidth() != frameSize.width || frame.image.getHeight() != frameSize.height)
        frame.image = juce::Image (juce::Image::ARGB, frameSize.width, frameSize.height, false);

    // Create bitmap from image ...
    juce::Image::BitmapData data (frame.image, juce::Image::BitmapData::writeOnly);
    jassert (data.lineStride == static_cast<int> (pitch));  // Assert the stream stride equals the bitmap's stride.
    jassert (data.pixelStride == 4);                        // Assert a pixel has 4 bytes (32bpp).
    jassert (data.width == videoSettings.frameSize.width);  // Assert bitmap and frame width are the same.
    for (int y = 0; y < videoSettings.frameSize.height; ++y) memcpy (data.getLinePointer (y), scanLine0 + (y * stride), static_cast<size_t> (pitch));

    return S_OK;
}


HRESULT MediaFoundationReader::Pimpl::fillBufferWithSample (IMFSample* sample, juce::AudioBuffer<float>& buffer)
{
    if (!sample) return E_FAIL;

    // Get buffer ...
    MFScopedPointer<IMFMediaBuffer> mediaBuffer;
    auto                            hr = sample->ConvertToContiguousBuffer (mediaBuffer.getPointer());
    if (FAILED (hr)) return hr;

    // Get total length in bytes ...
    DWORD expectedBufferSize = 0;
    hr                       = sample->GetTotalLength (&expectedBufferSize);
    if (FAILED (hr)) return hr;

    // Get time-stamp ...
    juce::int64 timeStamp = 0;
    hr                    = sample->GetSampleTime (&timeStamp);
    if (FAILED (hr)) return hr;

    // Lock the buffer and get a pointer to the data ...
    BYTE* audioData { nullptr };
    DWORD bufferSize = 0;
    hr               = mediaBuffer->Lock (&audioData, NULL, &bufferSize);

    const auto frameSize = audioSettings.getFrameSize();
    jassert (SUCCEEDED (hr));
    jassert (audioData);                            // Assert audioData is not null.
    jassert (bufferSize == expectedBufferSize);     // Assert buffer size is correct.
    jassert (expectedBufferSize % frameSize == 0);  // Assert buffer size is dividable by the frame size

    // Make sure the audio buffer is the right size.
    int        totalNumSamples   = 0;
    int        samplesPerChannel = 0;
    const bool sourceIsMono      = (audioSettings.numChannels < 2);
    const int  numAudioChannels  = (sourceIsMono ? 2 : audioSettings.numChannels);  // Upmix mono audio ...

    if (audioSettings.isValid())
    {
        totalNumSamples   = bufferSize / audioSettings.getSampleSize();
        samplesPerChannel = totalNumSamples / audioSettings.numChannels;
        if (samplesPerChannel > 0) buffer.setSize (numAudioChannels, samplesPerChannel);
    }

    // Convert audioData ...
    juce::HeapBlock<float> floatAudioData (totalNumSamples);
    if (audioSettings.bitsPerSample == 16)  // Source is 16 bit Little Endian integer.
        juce::AudioDataConverters::convertInt16LEToFloat (audioData, floatAudioData, totalNumSamples);
    else if (audioSettings.bitsPerSample == 24)  // Source is 24 bit Little Endian integer.
        juce::AudioDataConverters::convertInt24LEToFloat (audioData, floatAudioData, totalNumSamples);

    // De-interleave data and copy it to the audio buffer ...
    if (sourceIsMono)
    {
        for (int channelIdx = 0; channelIdx < buffer.getNumChannels(); ++channelIdx)  // Copy audio data to all channels ...
            buffer.copyFrom (channelIdx, 0, floatAudioData, samplesPerChannel);
    }
    else
    {
        juce::AudioDataConverters::deinterleaveSamples (floatAudioData, buffer.getArrayOfWritePointers(), samplesPerChannel, numAudioChannels);
    }

    hr = mediaBuffer->Unlock();
    if (FAILED (hr)) return hr;

    return S_OK;
}

#pragma endregion

// ==============================================================================


}  // namespace foleys