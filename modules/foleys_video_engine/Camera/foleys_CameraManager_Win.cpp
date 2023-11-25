/*
==============================================================================

Copyright (c) 2019-2021, Foleys Finest Audio - Daniel Walz
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
#include <mfapi.h>
#include <mfidl.h>
#include <shlwapi.h>
#include <mfcaptureengine.h>
#include <d3d11.h>
#include "microsoft_utils.cpp"
#include <combaseapi.h>
// clang-format on

#include "../Native/foleys_D3DManager_Win.h"

namespace foleys
{

struct CameraManager::Pimpl
{
    Pimpl();
    ~Pimpl();
    juce::StringArray                      getCameraNames() const;
    std::vector<CameraManager::CameraInfo> getCameraInfos() const;

    std::unique_ptr<CameraReceiver> openCamera (int index);
    std::unique_ptr<CameraReceiver> openCamera (const juce::String& uid);

    IMFActivate*          getCameraDevice (int index);
    IMFActivate*          getCameraDevice (const juce::String& uid);
    IMFDXGIDeviceManager* getDeviceManager();

 private:
    IMFActivate** ppDevices  = nullptr;
    UINT32        numDevices = 0;

    //==============================================================================

    bool enumerateDevices();
    void clearDevices();

    D3DManager d3dManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pimpl)
};

//==============================================================================

struct CameraReceiver::Pimpl
{
    Pimpl (CameraReceiver& ownerToUse, int index, void* session);

    Pimpl (CameraReceiver& ownerToUse, const juce::String& uid, void* session);

    ~Pimpl();

    struct SampleCallback : public IMFCaptureEngineOnSampleCallback
    {
        SampleCallback (CameraReceiver::Pimpl& pimpl)
          : owner (pimpl)
        {
        }

        // IUnknown
        STDMETHODIMP QueryInterface (REFIID riid, void** ppv);
        STDMETHODIMP_ (ULONG) AddRef();
        STDMETHODIMP_ (ULONG) Release();

        HRESULT OnSample (IMFSample* pSample) override;

     private:
        CameraReceiver::Pimpl& owner;
        long                   m_cRef = 1;
    };

    class CaptureEngineCB : public IMFCaptureEngineOnEventCallback
    {

     public:
        CaptureEngineCB (CameraReceiver::Pimpl& pimpl)
          : owner (pimpl)
        {
        }

        // IUnknown
        STDMETHODIMP QueryInterface (REFIID riid, void** ppv);
        STDMETHODIMP_ (ULONG) AddRef();
        STDMETHODIMP_ (ULONG) Release();

        // IMFCaptureEngineOnEventCallback
        STDMETHODIMP OnEvent (_In_ IMFMediaEvent* pEvent);

     private:
        CameraReceiver::Pimpl& owner;
        long                   m_cRef = 1;
    };



    juce::String getCameraName() const
    {
        return cameraDriverName;
    }

    juce::String getCameraUid() const
    {
        return cameraUID;
    }

    bool isReady() const
    {
        return engineInitialised;
    }

    bool start()
    {
        auto hr = engine->StartPreview();
        return SUCCEEDED (hr);
    }

    bool stop()
    {
        auto hr = engine->StopPreview();
        return SUCCEEDED (hr);
    }

    juce::Array<CameraReceiver::Resolution> getAvailableResolutions();

    bool setResolution (int)
    {
        // FIXME
        return true;
    }

    void setResolutionIndex (int index);

    VideoFrame& getNewFrameToFill();
    void        finishWriting();

 private:
    bool setupReceiver (IMFActivate* camera);
    void openCamera();

    CameraReceiver&       owner;
    CameraManager::Pimpl& cameraManager;

    static const auto streamType = MF_CAPTURE_ENGINE_PREFERRED_SOURCE_STREAM_FOR_VIDEO_RECORD;

    // CaptureManager manager;
    HANDLE                            engineHandle = nullptr;
    MFScopedPointer<IMFCaptureEngine> engine;
    MFScopedPointer<CaptureEngineCB>  engineCallback;
    bool                              engineInitialised = false;

    MFScopedPointer<IMFCapturePreviewSink> captureSink;

    juce::int64 timecode    = 0;
    int         frameWidth  = 0;
    int         frameHeight = 0;

    juce::String   cameraDriverName;
    juce::String   cameraUID;

    SampleCallback sampleCallback { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pimpl)
};

//==============================================================================

CameraManager::Pimpl::Pimpl()
{
    enumerateDevices();
}

CameraManager::Pimpl::~Pimpl()
{
    void clearDevices();
}

bool CameraManager::Pimpl::enumerateDevices()
{
    clearDevices();

    MFScopedPointer<IMFAttributes> pAttributes;
    HRESULT                        hr = MFCreateAttributes (pAttributes.getPointer(), 1);
    if (FAILED (hr)) { return false; }

    // Ask for source type = video capture devices
    hr = pAttributes->SetGUID (MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    if (FAILED (hr)) { return false; }

    // Enumerate devices.
    hr = MFEnumDeviceSources (pAttributes.get(), &ppDevices, &numDevices);
    if (FAILED (hr)) { return false; }

    return true;
}

void CameraManager::Pimpl::clearDevices()
{
    for (DWORD i = 0; i < numDevices; ++i) SafeRelease (&ppDevices[i]);

    if (ppDevices) CoTaskMemFree (ppDevices);

    numDevices = 0;
}

IMFActivate* CameraManager::Pimpl::getCameraDevice (int index)
{
    if (index < int (numDevices)) return ppDevices[index];

    return nullptr;
}

IMFActivate* CameraManager::Pimpl::getCameraDevice (const juce::String& uid)
{
    for (DWORD index = 0; index < numDevices; ++index) 
    {
        WCHAR* szSymbolicLink = NULL;
        UINT32 numChars       = 0;

        auto hr = ppDevices [index]->GetAllocatedString (MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &szSymbolicLink, &numChars);
        if (FAILED (hr)) continue;

        auto match = (uid == szSymbolicLink);

        CoTaskMemFree (szSymbolicLink);

        if (match) 
            return ppDevices[index];
    }

    return nullptr;
}

IMFDXGIDeviceManager* CameraManager::Pimpl::getDeviceManager()
{
    return d3dManager.getDeviceManager();
}

juce::StringArray CameraManager::Pimpl::getCameraNames() const
{
    juce::StringArray names;
    HRESULT           hr = S_OK;

    for (DWORD i = 0; i < numDevices; ++i)
    {
        WCHAR* szFriendlyName = NULL;
        UINT32 cchName;

        hr = ppDevices[i]->GetAllocatedString (MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &szFriendlyName, &cchName);
        if (FAILED (hr)) { break; }

        names.add (szFriendlyName);  // TODO: - Implicit conversion from a wchar* to juce::String

        CoTaskMemFree (szFriendlyName);
    }

    return names;
}

std::vector<CameraManager::CameraInfo> CameraManager::Pimpl::getCameraInfos() const
{
    std::vector<CameraManager::CameraInfo> infos;
    HRESULT                                hr = S_OK;

    for (DWORD i = 0; i < numDevices; i++)
    {
        WCHAR* szFriendlyName = NULL;
        WCHAR* szSymbolicLink = NULL;
        UINT32 cchName        = 0;

        hr = ppDevices[i]->GetAllocatedString (MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &szFriendlyName, &cchName);
        if (FAILED (hr)) continue;

        hr = ppDevices[i]->GetAllocatedString (MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &szSymbolicLink, &cchName);
        if (FAILED (hr)) continue;

        infos.push_back ({ szFriendlyName, szSymbolicLink });

        CoTaskMemFree (szSymbolicLink);
        CoTaskMemFree (szFriendlyName);
    }
    return infos;
}

std::unique_ptr<CameraReceiver> CameraManager::Pimpl::openCamera (int index)
{
    return std::make_unique<CameraReceiver> (index, this);
}

std::unique_ptr<CameraReceiver> CameraManager::Pimpl::openCamera (const juce::String& uid)
{
    return std::make_unique<CameraReceiver> (uid, this);
}

//==============================================================================

CameraReceiver::Pimpl::Pimpl (CameraReceiver& ownerToUse, int index, void* session)
  : owner (ownerToUse)
  , cameraManager (*static_cast<CameraManager::Pimpl*> (session))
{
    auto device = cameraManager.getCameraDevice (index);

    setupReceiver (device);
}

CameraReceiver::Pimpl::Pimpl (CameraReceiver& ownerToUse, const juce::String& uid, void* session)
  : owner (ownerToUse)
  , cameraManager (*static_cast<CameraManager::Pimpl*> (session))
{
    auto device = cameraManager.getCameraDevice (uid);

    setupReceiver (device);
}

CameraReceiver::Pimpl::~Pimpl()
{
    jassert (engine);
    engine->StopPreview();

    captureSink.reset();
}

bool CameraReceiver::Pimpl::setupReceiver (IMFActivate* device) 
{
    WCHAR* szFriendlyName = NULL;
    WCHAR* szSymbolicLink = NULL;
    UINT32 cchName        = 0;

    auto hr = device->GetAllocatedString (MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &szFriendlyName, &cchName);
    if (FAILED (hr)) return false;

    cameraDriverName = szFriendlyName;
    CoTaskMemFree (szFriendlyName);

    hr = device->GetAllocatedString (MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &szSymbolicLink, &cchName);
    if (FAILED (hr)) return false;

    cameraUID = szSymbolicLink;
    CoTaskMemFree (szSymbolicLink);

    engineHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (engineHandle == nullptr) return false;

    engineCallback.reset (new (std::nothrow) CaptureEngineCB (*this));
    if (!engineCallback) return false;

    MFScopedPointer<IMFAttributes>                pAttributes;
    MFScopedPointer<IMFCaptureEngineClassFactory> pFactory;

    hr = MFCreateAttributes (pAttributes.getPointer(), 1);
    if (FAILED (hr)) return false;

    hr = pAttributes->SetUnknown (MF_CAPTURE_ENGINE_D3D_MANAGER, cameraManager.getDeviceManager());
    if (FAILED (hr)) return false;

    // Create the factory object for the capture engine.
    hr = CoCreateInstance (CLSID_MFCaptureEngineClassFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS (pFactory.getPointer()));
    if (FAILED (hr)) return false;

    // Create and initialize the capture engine.
    hr = pFactory->CreateInstance (CLSID_MFCaptureEngine, IID_PPV_ARGS (engine.getPointer()));
    if (FAILED (hr)) return false;

    hr = engine->Initialize (engineCallback.get(), pAttributes.get(), NULL, device);
    if (FAILED (hr)) return false;

    return true;
}

void CameraReceiver::Pimpl::openCamera()
{
    jassert (isReady());
    if (!isReady()) return;

    jassert (engine);

    MFScopedPointer<IMFCaptureSink>   pSink;
    MFScopedPointer<IMFMediaType>     pMediaType;
    MFScopedPointer<IMFMediaType>     pMediaType2;
    MFScopedPointer<IMFCaptureSource> pSource;

    auto hr = engine->GetSink (MF_CAPTURE_ENGINE_SINK_TYPE_PREVIEW, pSink.getPointer());
    if (FAILED (hr))
    {
        jassertfalse;
        return;
    }

    hr = pSink->QueryInterface (IID_PPV_ARGS (captureSink.getPointer()));
    if (FAILED (hr))
    {
        jassertfalse;
        return;
    }

    hr = engine->GetSource (pSource.getPointer());
    if (FAILED (hr))
    {
        jassertfalse;
        return;
    }

    // Configure the video format for the preview sink.
    hr = pSource->GetCurrentDeviceMediaType ((DWORD) streamType, pMediaType.getPointer());
    if (FAILED (hr))
    {
        jassertfalse;
        return;
    }

    hr = CloneVideoMediaType (pMediaType.get(), MFVideoFormat_RGB32, pMediaType2.getPointer());
    if (FAILED (hr))
    {
        jassertfalse;
        return;
    }

    hr = pMediaType2->SetUINT32 (MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
    if (FAILED (hr))
    {
        jassertfalse;
        return;
    }

    // Connect the video stream to the preview sink.
    DWORD dwSinkStreamIndex;
    hr = captureSink->AddStream ((DWORD) streamType, pMediaType2.get(), NULL, &dwSinkStreamIndex);
    if (FAILED (hr))
    {
        jassertfalse;
        return;
    }

    hr = captureSink->SetSampleCallback (dwSinkStreamIndex, &sampleCallback);
    if (FAILED (hr))
    {
        jassertfalse;
        return;
    }

    UINT32 width, height;
    hr = MFGetAttributeSize (pMediaType.get(), MF_MT_FRAME_SIZE, &width, &height);
    jassert (!FAILED (hr));

    frameWidth  = int (width);
    frameHeight = int (height);
}

juce::Array<CameraReceiver::Resolution> CameraReceiver::Pimpl::getAvailableResolutions()
{
    jassert (isReady());
    if (!isReady()) return {};

    juce::Array<CameraReceiver::Resolution> resolutions;

    jassert (engine);

    MFScopedPointer<IMFCaptureSource> pSource;

    auto hr = engine->GetSource (pSource.getPointer());

    DWORD count = 0;
    while (hr == S_OK)
    {
        MFScopedPointer<IMFMediaType> mediaType;
        hr = pSource->GetAvailableDeviceMediaType ((DWORD) streamType, count, mediaType.getPointer());
        if (hr == S_OK)
        {
            ++count;
            UINT32 width, height;
            auto   hr2 = MFGetAttributeSize (mediaType.get(), MF_MT_FRAME_SIZE, &width, &height);
            jassert (hr2 == S_OK);

            UINT32 enumerator, denominator;
            hr2 = MFGetAttributeRatio (mediaType.get(), MF_MT_FRAME_RATE, &enumerator, &denominator);
            jassert (hr2 == S_OK);

            resolutions.add ({ int (width), int (height), double (enumerator) / double (denominator) });
        }
    }

    return resolutions;
}


void CameraReceiver::Pimpl::setResolutionIndex (int index)
{
    jassert (isReady());
    if (!isReady()) return;

    jassert (engine);

    engine->StopPreview();

    MFScopedPointer<IMFCaptureSource> pSource;
    MFScopedPointer<IMFMediaType>     pMediaType;

    auto hr = engine->GetSource (pSource.getPointer());
    if (FAILED (hr))
    {
        jassertfalse;
        return;
    }

    hr = pSource->GetAvailableDeviceMediaType ((DWORD) streamType, DWORD (index), pMediaType.getPointer());
    if (FAILED (hr))
    {
        jassertfalse;
        return;
    }

    hr = pSource->SetCurrentDeviceMediaType ((DWORD) streamType, pMediaType.get());
    if (FAILED (hr))
    {
        jassertfalse;
        return;
    }

    UINT32 width, height;
    hr = MFGetAttributeSize (pMediaType.get(), MF_MT_FRAME_SIZE, &width, &height);
    jassert (!FAILED (hr));

    frameWidth  = int (width);
    frameHeight = int (height);

    engine->StartPreview();
}


VideoFrame& CameraReceiver::Pimpl::getNewFrameToFill()
{
    auto& frame    = owner.videoFifo.getWritingFrame();
    frame.timecode = ++timecode;
    return frame;
}

void CameraReceiver::Pimpl::finishWriting()
{
    owner.videoFifo.finishWriting();

    if (owner.onFrameCaptured) owner.onFrameCaptured();
}


//==============================================================================

STDMETHODIMP CameraReceiver::Pimpl::SampleCallback::QueryInterface (REFIID riid, void** ppv)
{
    static const QITAB qit[] = { QITABENT (SampleCallback, IMFCaptureEngineOnSampleCallback), { 0 } };
    return QISearch (this, qit, riid, ppv);
}

STDMETHODIMP_ (ULONG) CameraReceiver::Pimpl::SampleCallback::AddRef()
{
    return InterlockedIncrement (&m_cRef);
}

STDMETHODIMP_ (ULONG) CameraReceiver::Pimpl::SampleCallback::Release()
{
    LONG cRef = InterlockedDecrement (&m_cRef);
    if (cRef == 0) { delete this; }
    return cRef;
}

HRESULT CameraReceiver::Pimpl::SampleCallback::OnSample (IMFSample* pSample)
{
    MFScopedPointer<IMFMediaBuffer> mediaBuffer;

    pSample->ConvertToContiguousBuffer (mediaBuffer.getPointer());

    auto& frame = owner.getNewFrameToFill();

    juce::int64 time = 0;
    auto        hr   = pSample->GetSampleTime (&time);
    frame.timecode   = time;

    MFScopedPointer<IMF2DBuffer> image;
    hr = mediaBuffer->QueryInterface (IID_PPV_ARGS (image.getPointer()));
    jassert (!FAILED (hr));

    BYTE* ppbScanline0;
    LONG  plPitch;
    hr = image->Lock2D (&ppbScanline0, &plPitch);
    jassert (!FAILED (hr));

    DWORD numBytes;
    hr = image->GetContiguousLength (&numBytes);
    jassert (!FAILED (hr));

    if (frame.image.isNull() || frame.image.getWidth() != owner.frameWidth || frame.image.getHeight() != owner.frameHeight)
        frame.image = juce::Image (juce::Image::ARGB, owner.frameWidth, owner.frameHeight, false);

    juce::Image::BitmapData data (frame.image, juce::Image::BitmapData::writeOnly);

    for (int y = 0; y < owner.frameHeight; ++y) memcpy (data.getLinePointer (y), ppbScanline0 + y * plPitch, owner.frameWidth * 4);

    hr = image->Unlock2D();
    jassert (!FAILED (hr));

    owner.finishWriting();

    return S_OK;
}


//==============================================================================

STDMETHODIMP CameraReceiver::Pimpl::CaptureEngineCB::QueryInterface (REFIID riid, void** ppv)
{
    static const QITAB qit[] = { QITABENT (CaptureEngineCB, IMFCaptureEngineOnEventCallback), { 0 } };
    return QISearch (this, qit, riid, ppv);
}

STDMETHODIMP_ (ULONG) CameraReceiver::Pimpl::CaptureEngineCB::AddRef()
{
    return InterlockedIncrement (&m_cRef);
}

STDMETHODIMP_ (ULONG) CameraReceiver::Pimpl::CaptureEngineCB::Release()
{
    LONG cRef = InterlockedDecrement (&m_cRef);
    if (cRef == 0) { delete this; }
    return cRef;
}

// Callback method to receive events from the capture engine.
STDMETHODIMP CameraReceiver::Pimpl::CaptureEngineCB::OnEvent (_In_ IMFMediaEvent* pEvent)
{
    HRESULT hrStatus;
    HRESULT hr = pEvent->GetStatus (&hrStatus);
    if (FAILED (hr)) hrStatus = hr;

    GUID guidType;
    hr = pEvent->GetExtendedType (&guidType);

    if (SUCCEEDED (hr))
    {
        if (guidType == MF_CAPTURE_ENGINE_INITIALIZED)
        {
            owner.engineInitialised = true;
            owner.openCamera();

            if (owner.owner.onCaptureEngineInitialized) owner.owner.onCaptureEngineInitialized();
        }
        else if (guidType == MF_CAPTURE_ENGINE_PREVIEW_STARTED)
        {
            if (owner.owner.onPreviewStarted) owner.owner.onPreviewStarted();
        }
        else if (guidType == MF_CAPTURE_ENGINE_PREVIEW_STOPPED)
        {
            if (owner.owner.onPreviewStopped) owner.owner.onPreviewStopped();
        }
        else if (guidType == MF_CAPTURE_ENGINE_RECORD_STARTED)
        {
            if (owner.owner.onRecordingStarted) owner.owner.onRecordingStarted();
        }
        else if (guidType == MF_CAPTURE_ENGINE_RECORD_STOPPED)
        {
            if (owner.owner.onRecordingStopped) owner.owner.onRecordingStopped();
        }
        else if (guidType == MF_CAPTURE_ENGINE_PHOTO_TAKEN)
        {
            // Not implemented
        }
        else if (guidType == MF_CAPTURE_ENGINE_ERROR)
        {
            owner.engineInitialised = false;

            if (owner.owner.onCaptureEngineError) owner.owner.onCaptureEngineError();
        }
    }

    return S_OK;
}


}  // namespace foleys
