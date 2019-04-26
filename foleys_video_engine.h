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

  BEGIN_JUCE_MODULE_DECLARATION

  ID:            foleys_video_engine
  vendor:        Foleys Finest Audio Ltd.
  version:       0.0.1
  name:          Video engine to read, process, display and write video in JUCE
  description:   Provides classes to read audio streams from video files or to
                 mux audio into an existing video
  dependencies:  juce_audio_basics, juce_audio_formats, juce_gui_basics,
                 juce_graphics, juce_core, juce_audio_utils

  website:       https://foleysfinest.com/

  END_JUCE_MODULE_DECLARATION

  ==============================================================================
 */

/** Config: FOLEYS_USE_FFMPEG
    Set this flag to use FFmpeg as reading/writing library */
#ifndef FOLEYS_USE_FFMPEG
#define FOLEYS_USE_FFMPEG 1
#endif

/** Config: FOLEYS_DEBUG_LOGGING
    Set this flag to enable logging */
#ifndef FOLEYS_DEBUG_LOGGING
#define FOLEYS_DEBUG_LOGGING 0
#endif


#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include "Basics/Structures.h"
#include "Basics/AudioFifo.h"
#include "Basics/VideoFifo.h"
#include "Clips/AVClip.h"
#include "ReadWrite/AVReader.h"
#include "ReadWrite/AVWriter.h"
#include "ReadWrite/ClipBouncer.h"
#include "Processing/CompositingContext.h"
#include "Processing/SoftwareCompositingContext.h"

#include "Clips/AudioClip.h"
#include "Clips/ImageClip.h"
#include "Clips/MovieClip.h"
#include "Clips/ComposedClip.h"

#include "Basics/VideoEngine.h"
#include "Widgets/VideoPreview.h"
#include "Widgets/FilmStrip.h"
#include "Widgets/AudioStrip.h"

#if FOLEYS_USE_FFMPEG
#include "ReadWrite/FFmpeg/FFmpegReader.h"
#include "ReadWrite/FFmpeg/FFmpegWriter.h"
#endif

#include "ReadWrite/AVFormatManager.h"
