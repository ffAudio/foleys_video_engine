README - Foleys Video Engine
============================

Daniel Walz - developer@foleysfinest.com - 2019

Foleys Finest Audio Ltd.

With the module foleys_video_engine we offer a simple way to implement reading,
writing, displaying and editing of videos using the audio framework JUCE (https://juce.com)

Features
--------

- Reading of video files, video and audio synchronous
- Normalise different sample rates and frame rates
- Compositing of multiple videos or still images in layers (paint on top)
- Writing of video clips
- Audio plugins for clips
- Automatable parameters for video composition
- Video plugins for image processing / colour adjustments etc.
- _Multiple audio stems per ComposedClip (avadilable soon)_
- _Video generator plugins for titles, backgrounds etc. (coming later)_
- _Alternative video file backend (coming much later)_
- _Hardware rendering backend (coming even later)_

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

Examples
--------

On https://github.com/ffAudio/VideoExamples we have a simple video player using this engine,
as well as a Video Editor (NLE) to test the functionality and to give an idea about it's
intended use.

To use the engine, just start the Projucer and add foleys_video_engine to your project.

We hope this is useful and are looking forward to your feedback:

Email:       developer@foleysfinest.com\\
github:      ffAudio / https://github.com/ffAudio\\
JUCE forum:  daniel\\
discord:     daniel (JUCE and theaudioprogrammer)\\
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


Foleys Finest Audio Ltd. - Brighton, UK - 2019
