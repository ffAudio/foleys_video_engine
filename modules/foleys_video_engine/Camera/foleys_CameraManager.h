/*
 ==============================================================================

 Copyright (c) 2020, Foleys Finest Audio - Daniel Walz
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

#if FOLEYS_CAMERA_SUPPORT

namespace foleys
{

class VideoEngine;
class CameraClip;

/**
 The CameraReceiver is the platform specific backend to get the frames from
 the camera so it can be used in the engine.
 */
class CameraReceiver
{
public:
    CameraReceiver (int index, void* session);
    CameraReceiver (const juce::String& uid, void* session);
    ~CameraReceiver();

    struct Resolution
    {
        int    width           = 0;
        int    height          = 0;
        double framesPerSecond = 0.0;

        juce::String toString()
        {
            return juce::String (width) + "x" + juce::String (height) + " @" + juce::String (framesPerSecond) + " FPS";
        }
    };

    /**
     Returns the name as reported by the camera
     */
    juce::String getCameraName() const;

    juce::String getCameraUid() const;

    /**
     This can be queried if the driver is already initialised.
     Methods like getAvailableResolutions() or setResolution(int)
     won't work before that happened.
     */
    bool isReady() const;

    /**
     Start streaming (windows only)
     */
    bool start();

    /**
     Start streaming (windows only)
     */
    bool stop();

    juce::Array<Resolution> getAvailableResolutions() const;
    bool                    setResolution (int index);

    VideoFrame& getCurrentFrame();

    struct Pimpl;

    std::function<void()> onFrameCaptured = nullptr;

    std::function<void()> onCaptureEngineInitialized;
    std::function<void()> onCaptureEngineError;
    std::function<void()> onPreviewStarted;
    std::function<void()> onPreviewStopped;
    std::function<void()> onRecordingStarted;
    std::function<void()> onRecordingStopped;


private:
    VideoFifo               videoFifo { 4 };
    std::unique_ptr<Pimpl>  pimpl;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CameraReceiver)
};

/**
 This class manages access to the connected cameras.
 The VideoEngine will contain this manager by default.
 Don't create a second manager, the platform specific
 backend is not designed to support that.
 */
class CameraManager
{
public:
    struct CameraInfo
    {
        juce::String name;
        juce::String uid;
    };

    CameraManager (VideoEngine& engine);
    ~CameraManager();

    /**
     Returns the names of the connected cameras.
     */
    juce::StringArray getCameraNames() const;

    /**
     Returns the names and uids of the connected cameras
     */
    std::vector<CameraInfo> getCameraInfos() const;

    /**
     Open a camera and return a receiver that can be used in a CameraClip
     */
    std::unique_ptr<CameraReceiver> openCamera (int index);
    std::unique_ptr<CameraReceiver> openCamera (const juce::String& uid);

    /**
     Create a CameraClip connected to a camera
     */
    std::shared_ptr<CameraClip> createCameraClip (std::unique_ptr<CameraReceiver>);

private:
    struct Pimpl;

    std::unique_ptr<Pimpl> pimpl;

    friend CameraReceiver::Pimpl;

    VideoEngine& engine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CameraManager)
};

} // namespace foleys

#endif // FOLEYS_CAMERA_SUPPORT
