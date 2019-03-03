/*
  ==============================================================================

  Copyright (c) 2017, Daniel Walz
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

  3. Neither the name of the copyright holder nor the names of its contributors
     may be used to endorse or promote products derived from this software without
     specific prior written permission.

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
  dependencies:  juce_audio_basics, juce_audio_formats, juce_core
  website:       https://foleysfinest.com/
  license:       BSD V2 3-clause

  END_JUCE_MODULE_DECLARATION

  ==============================================================================
 */

/** Config: FOLEYS_USE_FFMPEG
    Set this flag to use FFmpeg as reading/writing library */
#ifndef FOLEYS_USE_FFMPEG
#define FOLEYS_USE_FFMPEG 1
#endif

#pragma once


#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Maths/Structures.h"
#include "Clips/AVClip.h"
#include "ReadWrite/AVReader.h"

#include "Clips/AVImageClip.h"
#include "Clips/AVMovieClip.h"
#include "Clips/AVCompoundClip.h"

#include "Widgets/AVComponent.h"

#include "ReadWrite/FFmpegReader.h"
