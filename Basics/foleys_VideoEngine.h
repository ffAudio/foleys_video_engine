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

#pragma once


namespace foleys
{

#if FOLEYS_HAS_ADDONS && FOLEYS_CAMERA_SUPPORT
class CameraManager;
#endif

/**
 @class VideoEngine

 The VideoEngine is handling the background tasks of the video. You should have only one
 instance.

 You can use the VideoEngine to create instances of AVClip, using the built in AVFormatManager.
 It also manages the lifetime of the clips, managing the auto release pool and the thread
 pools for reading ahead and for creating the thumbnails.
 */
class VideoEngine  : private juce::Timer
{
public:
    VideoEngine();
    ~VideoEngine() override;

    /**
     Use the source file to figure the most appropriate AVClip descendant.
     */
    std::shared_ptr<AVClip> createClipFromFile (juce::URL url, StreamTypes type = StreamTypes::all());

    /**
     Find an appropriate AVReader to be used to read a video file.
     */
    std::unique_ptr<AVReader> createReaderFor (juce::File file, StreamTypes type = StreamTypes::all());

    void addJob (std::function<void()> job);
    void addJob (juce::ThreadPoolJob* job, bool deleteJobWhenFinished);
    void cancelJob (juce::ThreadPoolJob* job);

    juce::ThreadPool& getThreadPool();

    /**
     This method will add the clip to the background threads and hold an auto
     release pool to make sure, it won't be deleted in any realtime critical thread.

     When using the engine's factory methods, this is already done for you, if you
     create a clip manually, calling make_shared, you will have to call this function
     for the clip to function.
     */
    void manageLifeTime (std::shared_ptr<AVClip> clip);

    /**
     You can set an external undomanager. In this case you are responsible for detetion.
     If you use the undomanager provided by the engine, you don't have to do anything
     for it's lifetime.
     */
    void setUndoManager (juce::UndoManager* undoManager);

    juce::UndoManager* getUndoManager();

    /**
     Grant access to the AVFormatManager. The AVFormatManager is responsible to load
     the different kinds of AVClips from files.
     */
    AVFormatManager& getFormatManager();

    /**
     Grants access to the AudioPluginManager, e.g. to register AudioProcessor factories
     */
    AudioPluginManager& getAudioPluginManager();

    /**
     Grants access to the VideoPluginManager, e.g. to register VideoProcessor factories
     */
    VideoPluginManager& getVideoPluginManager();

    /**
     Grants access to the VideoPluginManager, e.g. to register VideoProcessor factories
     */
    juce::AudioFormatManager& getAudioFormatManager();

    /**
     Use the VideoPluginManager to create a VideoProcessor instance.
     */
    std::unique_ptr<VideoProcessor> createVideoPluginInstance (const juce::String& identifierString,
                                                               juce::String& error) const;

    /**
     Use the AudioPluginManager to create a AudioProcessor instance.
     */
    std::unique_ptr<juce::AudioProcessor> createAudioPluginInstance (const juce::String& identifierString,
                                                                     double sampleRate,
                                                                     int blockSize,
                                                                     juce::String& error) const;

    /**
     This method will find the TimeSliceThread with the least number of clients to balance the load.
     */
    juce::TimeSliceThread& getNextTimeSliceThread();

private:

    void removeFromBackgroundThreads (juce::TimeSliceClient* client);

    void timerCallback() override;

    AVFormatManager formatManager;

    AudioPluginManager audioPluginManager { *this };

    VideoPluginManager videoPluginManager { *this };

    juce::OptionalScopedPointer<juce::UndoManager> undoManager { new juce::UndoManager(), true };

    juce::ThreadPool jobThreads{ (std::max)(4, juce::SystemStats::getNumCpus()) };
    std::vector<std::unique_ptr<juce::TimeSliceThread>> readingThreads;

    std::vector<std::shared_ptr<AVClip>> releasePool;

    JUCE_DECLARE_WEAK_REFERENCEABLE (VideoEngine)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VideoEngine)
    
#if FOLEYS_HAS_ADDONS && FOLEYS_CAMERA_SUPPORT
public:
    void setCameraManager (CameraManager* manager)
    {
        cameraManager = manager;
    }
    CameraManager* getCameraManager()
    {
        return cameraManager;
    }

private:
    CameraManager* cameraManager = nullptr;
#endif

};

} // foleys
