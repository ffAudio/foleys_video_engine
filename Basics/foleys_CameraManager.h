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

    VideoFrame& getCurrentFrame();

    struct Pimpl;

    std::function<void()> onFrameCaptured = nullptr;

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

    CameraManager (VideoEngine& engine);

    /**
     Returns the names of the connected cameras.
     */
    juce::StringArray getCameraNames() const;

    /**
     Open a camera and return a receiver that can be used in a CameraClip
     */
    std::unique_ptr<CameraReceiver> openCamera (int index);

    /**
     Create a CameraClip connected to a camera
     */
    std::shared_ptr<CameraClip> createCameraClip (int index);


private:
    struct Pimpl;

    std::unique_ptr<Pimpl> pimpl;

    VideoEngine& engine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CameraManager)
};

} // namespace foleys

#endif // FOLEYS_CAMERA_SUPPORT

