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

#include "foleys_video_engine.h"

#include "Basics/VideoFifo.cpp"
#include "Basics/AudioFifo.cpp"
#include "Basics/VideoEngine.cpp"

#include "Clips/AVClip.cpp"
#include "Clips/AVImageClip.cpp"
#include "Clips/AVMovieClip.cpp"
#include "Clips/AVCompoundClip.cpp"

#include "ReadWrite/AVReader.cpp"
#include "ReadWrite/AVFormatManager.cpp"

#if FOLEYS_USE_FFMPEG
#include "ReadWrite/FFmpeg/FFmpegReader.cpp"
#endif

#include "Widgets/VideoPreview.cpp"
