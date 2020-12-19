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

#if FOLEYS_CAMERA_SUPPORT

namespace foleys
{

#if JUCE_WINDOWS
struct CameraManager::Pimpl
{
    Pimpl();
    ~Pimpl();
    juce::StringArray getCameraNames() { return {}; }

    std::unique_ptr<CameraReceiver> openCamera (int index) { return {}; }

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pimpl)
};
#endif


CameraManager::CameraManager (VideoEngine& engineToUse)
  : engine (engineToUse)
{
    pimpl = std::make_unique<Pimpl>();
}

juce::StringArray CameraManager::getCameraNames() const
{
    return pimpl->getCameraNames();
}

std::unique_ptr<CameraReceiver> CameraManager::openCamera (int index)
{
    return pimpl->openCamera (index);
}

std::shared_ptr<CameraClip> CameraManager::createCameraClip (int index)
{
    auto clip = std::make_shared<CameraClip>(engine, openCamera (index));
    engine.manageLifeTime (clip);
    return clip;
}

//==============================================================================

CameraReceiver::CameraReceiver(int index, void* session)
{
    pimpl = std::make_unique<Pimpl>(*this, index, session);
}

VideoFrame& CameraReceiver::getCurrentFrame()
{
    return videoFifo.getLatestFrame();
}

} // namespace foleys

#endif // FOLEYS_CAMERA_SUPPORT


