/*
 ==============================================================================

 Copyright (c) 2020, Foleys Finest Audio - Daniel Walz
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
#include <objc/message.h>
#include <juce_core/native/juce_osx_ObjCHelpers.h>
#undef AVMediaType

namespace foleys
{

//==============================================================================

struct CameraManager::Pimpl
{
    Pimpl();
    ~Pimpl();
    juce::StringArray getCameraNames();

    std::unique_ptr<CameraReceiver> openCamera (int index);

private:
    void* session = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pimpl)
};

//==============================================================================

struct CameraReceiver::Pimpl : public juce::ObjCClass<NSObject>
{
    Pimpl (CameraReceiver& ownerToUse, int index, void* sessionToUse)
      : juce::ObjCClass<NSObject> ("JUCEVideoDelegate_"),
        owner (ownerToUse),
        session (sessionToUse)
    {
        addProtocol (@protocol (AVCaptureVideoDataOutputSampleBufferDelegate));
        addMethod (@selector (captureOutput:didOutputSampleBuffer:fromConnection:), captureOutput, "v@:@@@");
        registerClass();

        videoDataOutputQueue = dispatch_queue_create ("VideoDataOutputQueue", DISPATCH_QUEUE_SERIAL);

        //Add device
        NSArray *devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];

        AVCaptureDevice *inputDevice = devices [(NSUInteger)index];

        //Add input
        NSError *inputError=nil;
        input = [[[AVCaptureDeviceInput alloc] initWithDevice:inputDevice error:&inputError] autorelease];
        [(AVCaptureSession*)session addInput:input];

        //Add output
        output = [[[AVCaptureVideoDataOutput alloc] init] autorelease];
        [output setSampleBufferDelegate: (id <AVCaptureVideoDataOutputSampleBufferDelegate>)this queue:(NSObject<OS_dispatch_queue> *)videoDataOutputQueue];
        [(AVCaptureSession*)session addOutput: output];
    }

    ~Pimpl()
    {
        [(AVCaptureSession*)session removeOutput: output];
        [(AVCaptureSession*)session removeInput: input];
    }

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
        juce::ignoreUnused (captureOutput);
        juce::ignoreUnused (connection);

        CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
        CIImage *sourceImage = [CIImage imageWithCVPixelBuffer:(CVPixelBufferRef)imageBuffer options:nil];
        CIContext* myContext = [[[CIContext alloc] init] autorelease];
        CGImageRef loadedImage = [myContext createCGImage:sourceImage fromRect:[sourceImage extent]];
        [(id)loadedImage autorelease];
        
        if (loadedImage != nil)
        {
            int width = (int) CGImageGetWidth (loadedImage);
            int height = (int) CGImageGetHeight (loadedImage);

            auto& frame = ((Pimpl*)self)->getNewFrameToFill();

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

            ((Pimpl*)self)->finishWriting();
        }
    }

    CameraReceiver& owner;

    void* session = nullptr;

    NSObject<OS_dispatch_queue>* videoDataOutputQueue = nullptr;
    AVCaptureDeviceInput*  input = nullptr;
    AVCaptureVideoDataOutput* output = nullptr;
    juce::int64 timecode = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pimpl)
};

//==============================================================================

CameraManager::Pimpl::Pimpl()
{
    //Capture Session
    AVCaptureSession* newSession = [[[AVCaptureSession alloc]init] autorelease];
    newSession.sessionPreset = AVCaptureSessionPresetPhoto;

    //Start capture session
    [newSession startRunning];

    session = newSession;
}

CameraManager::Pimpl::~Pimpl()
{
    if (session)
        [(AVCaptureSession*)session stopRunning];

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

std::unique_ptr<CameraReceiver> CameraManager::Pimpl::openCamera (int index)
{
    return std::make_unique<CameraReceiver>(index, session);
}

//==============================================================================


} // namespace foleys

#endif // FOLEYS_CAMERA_SUPPORT



