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

private:

    void addToThreadPool (std::shared_ptr<AVClip> clip);
    void removeFromThreadPool (std::shared_ptr<AVClip> clip);

    void timerCallback() override;

    juce::ThreadPool jobThreads { std::max (4, juce::SystemStats::getNumCpus()) };
    std::vector<std::unique_ptr<juce::TimeSliceThread>> readingThreads;

    std::vector<std::shared_ptr<AVClip>> releasePool;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VideoEngine)
};

} // foleys
