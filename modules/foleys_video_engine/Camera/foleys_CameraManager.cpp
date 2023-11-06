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

CameraManager::CameraManager (VideoEngine& engineToUse)
  : engine (engineToUse)
{
    pimpl = std::make_unique<Pimpl>();

    engine.setCameraManager (this);

    auto& manager = engine.getFormatManager();
    manager.registerFactory ("camera", [] (foleys::VideoEngine& theEngine, juce::URL url, StreamTypes)
    {
        auto camera = theEngine.getCameraManager()->openCamera (url.getDomain());
        return theEngine.getCameraManager()->createCameraClip (std::move (camera));
    });
}

CameraManager::~CameraManager() { }

juce::StringArray CameraManager::getCameraNames() const
{
    return pimpl->getCameraNames();
}

std::vector<CameraManager::CameraInfo> CameraManager::getCameraInfos() const
{
    return pimpl->getCameraInfos();
}

std::unique_ptr<CameraReceiver> CameraManager::openCamera (int index)
{
    jassert (getCameraNames().size() > index);
    return pimpl->openCamera (index);
}

std::unique_ptr<CameraReceiver> CameraManager::openCamera (const juce::String& uid)
{
    return pimpl->openCamera (uid);
}

std::shared_ptr<CameraClip> CameraManager::createCameraClip (std::unique_ptr<CameraReceiver> camera)
{
    auto clip = std::make_shared<CameraClip> (engine, std::move (camera));
    engine.manageLifeTime (clip);
    return clip;
}

//==============================================================================

CameraReceiver::CameraReceiver (int index, void* session)
{
    pimpl = std::make_unique<Pimpl> (*this, index, session);
}

CameraReceiver::CameraReceiver (const juce::String& uid, void* session)
{
    pimpl = std::make_unique<Pimpl> (*this, uid, session);
}

CameraReceiver::~CameraReceiver() { }

juce::String CameraReceiver::getCameraName() const
{
    return pimpl->getCameraName();
}

juce::String CameraReceiver::getCameraUid() const
{
    return pimpl->getCameraUid();
}

bool CameraReceiver::isReady() const
{
    return pimpl->isReady();
}

bool CameraReceiver::start()
{
    return pimpl->isReady() && pimpl->start();
}

bool CameraReceiver::stop()
{
    return pimpl->stop();
}

juce::Array<CameraReceiver::Resolution> CameraReceiver::getAvailableResolutions() const 
{
    return pimpl->getAvailableResolutions();
}

bool CameraReceiver::setResolution(int index) 
{
    return pimpl->setResolution (index);
}

VideoFrame& CameraReceiver::getCurrentFrame()
{
    return videoFifo.getLatestFrame();
}

}  // namespace foleys

#endif  // FOLEYS_CAMERA_SUPPORT
