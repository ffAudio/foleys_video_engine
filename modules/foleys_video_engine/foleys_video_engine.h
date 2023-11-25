/*
  ==============================================================================

  Copyright (c) 2019-2023, Foleys Finest Audio - Daniel Walz
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

  BEGIN_JUCE_MODULE_DECLARATION

  ID:                foleys_video_engine
  vendor:            Foleys Finest Audio Ltd.
  version:           0.3.0
  name:              Video engine to read, process, display and write video in JUCE
  description:       Provides classes to read audio streams from video files or to
                     mux audio into an existing video
  dependencies:      juce_audio_basics juce_audio_formats juce_gui_basics
                     juce_graphics juce_core juce_audio_utils
  OSXFrameworks:     AVFoundation CoreMedia
  iOSFrameworks:     AVFoundation CoreMedia
  minimumCppStandard: 17

  website:       https://foleysfinest.com/

  END_JUCE_MODULE_DECLARATION

  ==============================================================================
 */

/** Config: FOLEYS_USE_OPENGL
    *Experimental* This flag will switch from the juce paint() to the
    juce::OpenGLRenderer. That way every AVClip has the chance to issue
    raw OpenGL calls.
 */
#ifndef FOLEYS_USE_OPENGL
    #define FOLEYS_USE_OPENGL 0
#endif

/** Config: FOLEYS_CAMERA_SUPPORT
    Set this flag to access the cameras attached to the system
 */
#ifndef FOLEYS_CAMERA_SUPPORT
    #define FOLEYS_CAMERA_SUPPORT 1
#endif

/** Config: FOLEYS_USE_FFMPEG
    Set this flag to use FFmpeg as reading/writing library
 */
#ifndef FOLEYS_USE_FFMPEG
    #define FOLEYS_USE_FFMPEG 1
#endif

/** Config: FOLEYS_DEBUG_LOGGING
    Set this flag to enable logging
 */
#ifndef FOLEYS_DEBUG_LOGGING
    #define FOLEYS_DEBUG_LOGGING 0
#endif

#define FOLEYS_ENGINE_VERSION "0.3.0"


#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#if FOLEYS_DEBUG_LOGGING
    #define FOLEYS_LOG(textToWrite) \
        JUCE_BLOCK_WITH_FORCED_SEMICOLON (juce::String tempDbgBuf; tempDbgBuf << "foleys: " << textToWrite; juce::Logger::outputDebugString (tempDbgBuf);)
#else
    #define FOLEYS_LOG(textToWrite)
#endif

#ifdef JUCE_MODULE_AVAILABLE_juce_opengl
    #include <juce_opengl/juce_opengl.h>
#endif

#include "foleys_video_engine_types.h"

#include "Basics/foleys_AudioFifo.h"
#include "Basics/foleys_Structures.h"
#include "Basics/foleys_TimeCodeAware.h"
#include "Camera/foleys_CameraManager.h"
#include "Basics/foleys_VideoFifo.h"
#include "Basics/foleys_VideoFrame.h"
#include "Camera/foleys_CameraClip.h"
#include "Basics/foleys_VideoEngine.h"
#include "Clips/foleys_AVClip.h"
#include "Clips/foleys_AudioClip.h"
#include "Clips/foleys_ClipDescriptor.h"
#include "Clips/foleys_ComposedClip.h"
#include "Clips/foleys_ImageClip.h"
#include "Clips/foleys_MovieClip.h"
#include "Plugins/foleys_AudioPluginManager.h"
#include "Plugins/foleys_VideoPluginManager.h"
#include "Plugins/foleys_VideoProcessor.h"
#include "Processing/foleys_AudioMixer.h"
#include "Processing/foleys_ColourLookuptables.h"
#include "Processing/foleys_ControllableBase.h"
#include "Processing/foleys_DefaultAudioMixer.h"
#include "Processing/foleys_ParameterAutomation.h"
#include "Processing/foleys_ProcessorController.h"
#include "Processing/foleys_ProcessorParameter.h"
#include "Processing/foleys_VideoMixer.h"
#include "ReadWrite/foleys_AVFormatManager.h"
#include "ReadWrite/foleys_AVReader.h"
#include "ReadWrite/foleys_AVWriter.h"
#include "ReadWrite/foleys_ClipRenderer.h"
#include "Widgets/foleys_AudioStrip.h"
#include "Widgets/foleys_FilmStrip.h"
#include "Widgets/foleys_OpenGLDraw.h"
#include "Widgets/foleys_OpenGLView.h"
#include "Widgets/foleys_SoftwareView.h"
#include "Widgets/foleys_VideoView.h"

#if FOLEYS_USE_FFMPEG
    #include "ReadWrite/FFmpeg/foleys_FFmpegFormat.h"
    #include "ReadWrite/FFmpeg/foleys_FFmpegReader.h"
    #include "ReadWrite/FFmpeg/foleys_FFmpegWriter.h"
#endif

#if JUCE_WINDOWS
    #include "ReadWrite/foleys_MediaFoundation_Win.h"
#endif

#include "Plugins/foleys_ColourCurveVideoProcessor.h"
