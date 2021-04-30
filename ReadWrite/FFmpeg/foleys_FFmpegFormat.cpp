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

#if FOLEYS_USE_FFMPEG


namespace foleys
{

FFmpegFormat::FFmpegFormat() {}
FFmpegFormat::~FFmpegFormat() {}

bool FFmpegFormat::canRead(juce::File file)
{
    juce::ignoreUnused(file);
    return true;
}

std::unique_ptr<AVReader> FFmpegFormat::createReaderFor(juce::File file, StreamTypes types)
{
    return std::make_unique<FFmpegReader>(file, types);
}

bool FFmpegFormat::canWrite(juce::File file)
{
    juce::ignoreUnused(file);
    return true;
}

std::unique_ptr<AVWriter> FFmpegFormat::createWriterFor(juce::File file, StreamTypes types)
{
    juce::ignoreUnused (file, types);
    return {};
}


} // foleys
#endif
