README - Foleys Video Engine
============================

Daniel Walz - developer@foleysfinest.com - 2019 - 2021

Foleys Finest Audio Ltd.

With the module foleys_video_engine we offer a simple way to implement reading,
writing, displaying and editing of videos using the audio framework JUCE (https://juce.com)

Setup
--------

To use the video engine, add this module via Projucer or CMake to your JUCE project.

In CMake, this module can be added like this:
```cmake
add_subdirectory (foleys_video_engine)
```
or like this:
```cmake
find_package (foleys_video_engine)
```
This repo's CMake scripts add JUCE by calling `find_package()`. Foleys video engine requires JUCE version 6, and may
be incompatible with JUCE version 7.

You can override the path to JUCE on the command line like so:
```shell
cmake -B Builds -D FETCHCONTENT_SOURCE_DIR_JUCE=<your_juce_path>
```

If you are using CMake and the CPM.cmake package manager to add this repository, be aware that:
- JUCE's module system expects the root folder of a module to have the same name as the module
- By default, CPM.cmake will download the source code into a nested folder named with the version, for example `foleys_video_engine/<version>/<sourcesHere>`
This can cause the JUCE module system to get confused. If you are using CPM.cmake, we recommend you set the `CPM_USE_NAMED_CACHE_DIRECTORIES` CMake or environment variable to `ON` to prevent this issue.

foleys_video_engine requires [ffmpeg](https://www.ffmpeg.org/), which is built in CMake using [this repository](https://github.com/ffAudio/FFmpegBuild). The CMake configuration should "just work" out of the box, you don't need
to download or install anything.

Features
--------

- Reading of video files, video and audio synchronous
- Normalise different sample rates and frame rates
- Compositing of multiple videos or still images in layers (paint on top)
- Writing of video clips
- Audio plugins for clips
- Automatable parameters for video composition
- Video plugins for image processing / colour adjustments etc.
- Hardware rendering backend
- _Multiple audio stems per ComposedClip (available soon)_
- _Video generator plugins for titles, backgrounds etc. (coming later)_
- _Alternative video file backend (coming much later)_

To read and write video files it uses FFmpeg (https://ffmpeg.org). Alternative backends
like AVFoundation (OSX) or DirectShow (Windows) might follow in the future, but no
promises.

License
-------

This module is offered under a dual license: GPL and a paid license. For details please
refer to the included document LICENSE.md

The paid licenses are currently developed and will be announced soon. You are welcome to
get in touch to register your interest and tell us about your use case.

API-Documentation
-----------------

Please find the API documentation (doxygen) here: https://foleysfinest.com/foleys_video_engine/

There is also the support forum: https://forum.foleysfinest.com

Examples
--------

On https://github.com/ffAudio/VideoExamples we have a simple video player using this engine,
as well as a Video Editor (NLE) to test the functionality and to give an idea about it's
intended use.

To use the engine, just start the Projucer and add foleys_video_engine to your project.

We hope this is useful and are looking forward to your feedback:

Email:       developer@foleysfinest.com \
github:      ffAudio / https://github.com/ffAudio \
JUCE forum:  daniel \
discord:     daniel (JUCE and theaudioprogrammer) \
facebook:    https://fb.com/FoleysFinest/

Disclaimer
----------

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


Foleys Finest Audio - 2019 - Brighton, UK - 2021 Esslingen, Germany
