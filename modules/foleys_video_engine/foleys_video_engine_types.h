//
// Created by Daniel Walz on 25.11.23.
//

#pragma once

/**
 * This is adding forward declarations that might be useful
 */

namespace foleys
{

class VideoEngine;
class AVClip;
class AVReader;
class AVWriter;
class VideoProcessor;
struct VideoFrame;

#if FOLEYS_CAMERA_SUPPORT

class CameraManager;
class CameraReceiver;

#endif


} // namespace foleys
