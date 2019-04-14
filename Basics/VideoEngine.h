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

#pragma once


namespace foleys
{

class VideoEngine  : public juce::DeletedAtShutdown,
                     private juce::Timer
{
public:
    VideoEngine();
    ~VideoEngine();

    std::shared_ptr<AVClip> createClipFromFile (juce::File file);
    std::shared_ptr<AVCompoundClip> createCompoundClip();

    void addJob (std::function<void()> job);
    void addJob (juce::ThreadPoolJob* job, bool deleteJobWhenFinished);
    void cancelJob (juce::ThreadPoolJob* job);

    juce::ThreadPool& getThreadPool();

    /**
     This method will add the clip to the background threads and hold an auto
     release pool to make sure, it won't be deleted in any realtime critical thread */
    void manageLifeTime (std::shared_ptr<AVClip> clip);

    /**
     You can set an external undomanager. In this case you are responsible for detetion.
     If you use the undomanager provided by the engine, you don't have to do anything
     for it's lifetime. */
    void setUndoManager (juce::UndoManager* undoManager);

    juce::UndoManager* getUndoManager();

private:

    void removeFromBackgroundThreads (juce::TimeSliceClient* client);

    void timerCallback() override;

    juce::OptionalScopedPointer<juce::UndoManager> undoManager { new juce::UndoManager(), true };

    juce::ThreadPool jobThreads { std::max (4, juce::SystemStats::getNumCpus()) };
    std::vector<std::unique_ptr<juce::TimeSliceThread>> readingThreads;

    std::vector<std::shared_ptr<AVClip>> releasePool;

    JUCE_DECLARE_WEAK_REFERENCEABLE (VideoEngine)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VideoEngine)
};

} // foleys
