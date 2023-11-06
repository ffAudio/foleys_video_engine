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

#include "foleys_video_engine.h"

#include "Basics/foleys_VideoFifo.cpp"
#include "Basics/foleys_AudioFifo.cpp"
#include "Basics/foleys_VideoEngine.cpp"
#include "Basics/foleys_TimeCodeAware.cpp"

#include "Clips/foleys_AVClip.cpp"
#include "Clips/foleys_AudioClip.cpp"
#include "Clips/foleys_ImageClip.cpp"
#include "Clips/foleys_MovieClip.cpp"
#include "Clips/foleys_ComposedClip.cpp"
#include "Clips/foleys_ClipDescriptor.cpp"

#include "Plugins/foleys_AudioPluginManager.cpp"
#include "Plugins/foleys_VideoPluginManager.cpp"

#include "Processing/foleys_ControllableBase.cpp"
#include "Processing/foleys_ParameterAutomation.cpp"
#include "Processing/foleys_ProcessorParameter.cpp"
#include "Processing/foleys_ProcessorController.cpp"
#include "Processing/foleys_DefaultAudioMixer.cpp"

#include "ReadWrite/foleys_AVFormatManager.cpp"
#include "ReadWrite/foleys_ClipRenderer.cpp"

#if JUCE_WINDOWS
    #include "Native/foleys_Helpers_Win.h"
    #include "Camera/foleys_CameraManager_Win.cpp"
    #include "ReadWrite/Windows/foleys_MediaFoundation_Win.cpp"
#endif

#include "Camera/foleys_CameraManager.cpp"
#include "Camera/foleys_CameraClip.cpp"

#if FOLEYS_USE_FFMPEG
#include "ReadWrite/FFmpeg/foleys_FFmpegReader.cpp"
#include "ReadWrite/FFmpeg/foleys_FFmpegWriter.cpp"
#include "ReadWrite/FFmpeg/foleys_FFmpegFormat.cpp"
#endif

#include "Widgets/foleys_SoftwareView.cpp"
#include "Widgets/foleys_FilmStrip.cpp"
#include "Widgets/foleys_AudioStrip.cpp"
#include "Widgets/foleys_OpenGLView.cpp"
