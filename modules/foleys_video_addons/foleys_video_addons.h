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

  ID:                foleys_video_addons
  vendor:            Foleys Finest Audio UG
  version:           0.1.0
  name:              Video engine to read, process, display and write video in JUCE
  description:       Provides classes to read audio streams from video files or to
                     mux audio into an existing video
  dependencies:      foleys_video_engine
  OSXFrameworks:     AVFoundation CoreMedia
  iOSFrameworks:     AVFoundation CoreMedia
  minimumCppStandard: 17

  website:       https://foleysfinest.com/

  END_JUCE_MODULE_DECLARATION

  ==============================================================================
 */

#pragma once


/** Config: FOLEYS_CAMERA_SUPPORT
    Set this flag to access the cameras attached to the system
 */
#ifndef FOLEYS_CAMERA_SUPPORT
#define FOLEYS_CAMERA_SUPPORT 1
#endif


#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <foleys_video_engine/foleys_video_engine.h>

#if JUCE_WINDOWS
    #pragma comment(lib, "d3d11.lib")
    #pragma comment(lib, "mfplat.lib")
    #pragma comment(lib, "mf.lib")
    #pragma comment(lib, "mfreadwrite.lib")
    #pragma comment(lib, "mfuuid.lib")
    #pragma comment(lib, "propsys.lib")
#endif

#include "Camera/foleys_CameraManager.h"
#include "Camera/foleys_CameraClip.h"

#if JUCE_WINDOWS
#include "ReadWrite/foleys_MediaFoundation_Win.h"
#endif
