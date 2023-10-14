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

  BEGIN_JUCE_MODULE_DECLARATION

  ID:                foleys_video_engine
  vendor:            Foleys Finest Audio Ltd.
  version:           0.2.0
  name:              Video engine to read, process, display and write video in JUCE
  description:       Provides classes to read audio streams from video files or to
                     mux audio into an existing video
  dependencies:      juce_audio_basics juce_audio_formats juce_gui_basics
                     juce_graphics juce_core juce_audio_utils
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

#define FOLEYS_ENGINE_VERSION "0.2.0"

// foleys_video_addons is a proprietory module containing
// camera support and reading and writing using the
// platform SDKs instead of ffmpeg
#ifdef JUCE_MODULE_AVAILABLE_foleys_video_addons
#define FOLEYS_HAS_ADDONS JUCE_MODULE_AVAILABLE_foleys_video_addons
#else
#define FOLEYS_HAS_ADDONS 0
#endif


#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>

#if FOLEYS_DEBUG_LOGGING
#  define FOLEYS_LOG(textToWrite)          JUCE_BLOCK_WITH_FORCED_SEMICOLON(juce::String tempDbgBuf; tempDbgBuf << "foleys: " << textToWrite; juce::Logger::outputDebugString (tempDbgBuf);)
#else
#  define FOLEYS_LOG(textToWrite)
#endif

#ifdef JUCE_MODULE_AVAILABLE_juce_opengl
#include <juce_opengl/juce_opengl.h>
#endif

#include "Basics/foleys_Structures.h"
#include "Basics/foleys_VideoFrame.h"
#include "Basics/foleys_TimeCodeAware.h"
#include "Basics/foleys_AudioFifo.h"
#include "Basics/foleys_VideoFifo.h"
#include "Processing/foleys_ProcessorParameter.h"
#include "Plugins/foleys_AudioPluginManager.h"
#include "Plugins/foleys_VideoProcessor.h"
#include "Plugins/foleys_VideoPluginManager.h"
#include "Processing/foleys_ControllableBase.h"
#include "Processing/foleys_ProcessorController.h"
#include "Processing/foleys_ParameterAutomation.h"
#include "Clips/foleys_AVClip.h"
#include "Clips/foleys_ClipDescriptor.h"
#include "ReadWrite/foleys_AVReader.h"
#include "ReadWrite/foleys_AVWriter.h"
#include "ReadWrite/foleys_AVFormatManager.h"
#include "ReadWrite/foleys_ClipRenderer.h"
#include "Processing/foleys_AudioMixer.h"
#include "Processing/foleys_VideoMixer.h"
#include "Processing/foleys_DefaultAudioMixer.h"
#include "Processing/foleys_ColourLookuptables.h"

#include "Clips/foleys_AudioClip.h"
#include "Clips/foleys_ImageClip.h"
#include "Clips/foleys_MovieClip.h"
#include "Clips/foleys_ComposedClip.h"

#include "Basics/foleys_VideoEngine.h"
#include "Widgets/foleys_VideoView.h"
#include "Widgets/foleys_SoftwareView.h"
#include "Widgets/foleys_FilmStrip.h"
#include "Widgets/foleys_AudioStrip.h"
#include "Widgets/foleys_OpenGLDraw.h"
#include "Widgets/foleys_OpenGLView.h"

#if FOLEYS_USE_FFMPEG
#include "ReadWrite/FFmpeg/foleys_FFmpegReader.h"
#include "ReadWrite/FFmpeg/foleys_FFmpegWriter.h"
#include "ReadWrite/FFmpeg/foleys_FFmpegFormat.h"
#endif

#include "Plugins/foleys_ColourCurveVideoProcessor.h"
