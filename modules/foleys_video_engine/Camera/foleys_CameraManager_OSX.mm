/*
 ==============================================================================

 Copyright (c) 2020-2023, Foleys Finest Audio - Daniel Walz
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

#if FOLEYS_CAMERA_SUPPORT

#define AVMediaType MyAVMediaType
#include <AVFoundation/AVFoundation.h>
#include <CoreImage/CoreImage.h>
#include <objc/message.h>
#include <juce_core/native/juce_ObjCHelpers_mac.h>
#undef AVMediaType

namespace foleys
{

//==============================================================================

struct CameraManager::Pimpl
{
    Pimpl();
    ~Pimpl();
    juce::StringArray getCameraNames();
    std::vector<CameraManager::CameraInfo> getCameraInfos() const;

    std::unique_ptr<CameraReceiver> openCamera (int index);
    std::unique_ptr<CameraReceiver> openCamera (const juce::String& uid);

private:
    AVCaptureSession* session = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pimpl)
};

//==============================================================================

struct CameraReceiver::Pimpl : public juce::ObjCClass<NSObject>
{
    Pimpl (CameraReceiver& ownerToUse, const juce::String& uid, void* sessionToUse)
      : juce::ObjCClass<NSObject> ("FOLEYSVideoDelegate_"),
        owner (ownerToUse),
        session ((AVCaptureSession*)sessionToUse)
    {
        device = [AVCaptureDevice deviceWithUniqueID: juce::juceStringToNS(uid)];

        openCamera();
    }

    Pimpl (CameraReceiver& ownerToUse, int index, void* sessionToUse)
      : juce::ObjCClass<NSObject> ("FOLEYSVideoDelegate_"),
        owner (ownerToUse),
        session ((AVCaptureSession*)sessionToUse)
    {
        //Add device
        NSArray *devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];

        device = devices [(NSUInteger)index];
        jassert (device != nullptr);

        openCamera();
    }

    ~Pimpl()
    {
        if (output)
            [session removeOutput: output];

        if (input)
            [session removeInput: input];
    }

    void openCamera()
    {
        jassert (session != nullptr);
        jassert (device != nullptr);

        if (device == nullptr)
            return;

        addProtocol (@protocol (AVCaptureVideoDataOutputSampleBufferDelegate));
        addMethod (@selector (captureOutput:didOutputSampleBuffer:fromConnection:), captureOutput);
        registerClass();

        videoDataOutputQueue = dispatch_queue_create ("VideoDataOutputQueue", DISPATCH_QUEUE_SERIAL);

        //Add input
        NSError *inputError=nil;
        input = [[[AVCaptureDeviceInput alloc] initWithDevice:device error:&inputError] autorelease];
        jassert (input != nullptr);
        [(AVCaptureSession*)session addInputWithNoConnections:input];

        //Add output
        output = [[[AVCaptureVideoDataOutput alloc] init] autorelease];
        jassert (output != nullptr);
        [output setSampleBufferDelegate: (id <AVCaptureVideoDataOutputSampleBufferDelegate>)this queue:(NSObject<OS_dispatch_queue> *)videoDataOutputQueue];
        [(AVCaptureSession*)session addOutputWithNoConnections: output];

        for (AVCaptureInputPort *port in [input ports]) {
            if ([[port mediaType] isEqualToString:AVMediaTypeVideo]) {
                AVCaptureConnection* cxn = [AVCaptureConnection
                    connectionWithInputPorts:[NSArray arrayWithObject:port]
                    output:output
                ];
                if ([session canAddConnection:cxn]) {
                    [session addConnection:cxn];
                }
                break;
            }
        }
    }

    juce::String getCameraName() const
    {
        if (device)
            return juce::String ([device.localizedName cStringUsingEncoding:NSUTF8StringEncoding]);

        return {};
    }

    juce::String getCameraUid() const
    {
        if (device)
            return juce::String ([device.uniqueID cStringUsingEncoding:NSUTF8StringEncoding]);

        return {};
    }

    bool                    isReady() const                 { return true; }
    bool                    start() const                   { return true; }
    bool                    stop() const                    { return true; }
    juce::Array<Resolution> getAvailableResolutions() const { return {}; }
    bool                    setResolution (int /*index*/)   { return true; }

    VideoFrame& getNewFrameToFill()
    {
        auto& frame = owner.videoFifo.getWritingFrame();
        frame.timecode = ++timecode;
        return frame;
    }

    void finishWriting()
    {
        owner.videoFifo.finishWriting();

        if (owner.onFrameCaptured)
            owner.onFrameCaptured();
    }

private:

    static void captureOutput (id self, SEL, AVCaptureOutput* captureOutput, CMSampleBufferRef sampleBuffer, AVCaptureConnection* connection)
    {
        auto* pimpl = (Pimpl*)self;

        juce::ignoreUnused (captureOutput);
        juce::ignoreUnused (connection);

        CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
        CIImage *sourceImage = [CIImage imageWithCVPixelBuffer:(CVPixelBufferRef)imageBuffer options:nil];
        CIContext* imageContext = [[[CIContext alloc] init] autorelease];
        CGImageRef loadedImage = [imageContext createCGImage:sourceImage fromRect:[sourceImage extent]];
        [(id)loadedImage autorelease];

        if (loadedImage != nil)
        {
            int width = (int) CGImageGetWidth (loadedImage);
            int height = (int) CGImageGetHeight (loadedImage);

            auto& frame = pimpl->getNewFrameToFill();

            if (frame.image.getWidth() != width || frame.image.getHeight() != height)
                frame.image = juce::Image (juce::Image::ARGB, width, height, false);

            CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();

            juce::Image::BitmapData bitmap (frame.image, juce::Image::BitmapData::readWrite);

            int pixelStride = 4;
            int lineStride = (pixelStride * std::max (1, width) + 3) & ~3;

            CGContextRef context = CGBitmapContextCreate(bitmap.data, (size_t) width, (size_t) height, 8, (size_t) lineStride, colorSpace, kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);

            CGColorSpaceRelease(colorSpace);

            CGContextDrawImage(context, CGRectMake(0, 0, width, height), loadedImage);

            CGContextFlush(context);

            pimpl->finishWriting();

            CGContextRelease(context);
        }
    }

    CameraReceiver& owner;

    AVCaptureSession* session = nullptr;

    NSObject<OS_dispatch_queue>* videoDataOutputQueue = nullptr;
    AVCaptureDevice*            device = nullptr;
    AVCaptureDeviceInput*       input = nullptr;
    AVCaptureVideoDataOutput*   output = nullptr;
    juce::int64                 timecode = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pimpl)
};

//==============================================================================

CameraManager::Pimpl::Pimpl()
{
#if JUCE_IOS
    if (@available(iOS 13.0, *)) {
        session = [[[AVCaptureMultiCamSession alloc] init] retain];
    } else {
        session = [[[AVCaptureSession alloc] init] retain];
        session.sessionPreset = AVCaptureSessionPresetPhoto;
    }
#else
    session = [[[AVCaptureSession alloc] init] retain];
    session.sessionPreset = AVCaptureSessionPresetPhoto;
#endif

    [session startRunning];
}

CameraManager::Pimpl::~Pimpl()
{
    if (session)
    {
        [session stopRunning];
        [session release];
    }

    session = nullptr;
}

juce::StringArray CameraManager::Pimpl::getCameraNames()
{
    if (session == nullptr)
        return {};

    juce::StringArray names;
    NSArray *devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    for (AVCaptureDevice* d in devices)
        names.add ([d.localizedName cStringUsingEncoding:NSUTF8StringEncoding]);

    return names;
}

std::vector<CameraManager::CameraInfo> CameraManager::Pimpl::getCameraInfos() const
{
    if (session == nullptr)
        return {};

    std::vector<CameraManager::CameraInfo> infos;

    NSArray *devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    for (AVCaptureDevice* d in devices)
    {
        CameraManager::CameraInfo info;
        info.name = [d.localizedName cStringUsingEncoding:NSUTF8StringEncoding];
        info.uid = [d.uniqueID cStringUsingEncoding:NSUTF8StringEncoding];
        infos.push_back (info);
    }

    return infos;
}

std::unique_ptr<CameraReceiver> CameraManager::Pimpl::openCamera (int index)
{
    return std::make_unique<CameraReceiver>(index, session);
}

std::unique_ptr<CameraReceiver> CameraManager::Pimpl::openCamera (const juce::String& uid)
{
    return std::make_unique<CameraReceiver>(uid, session);
}

//==============================================================================


} // namespace foleys

#endif // FOLEYS_CAMERA_SUPPORT
