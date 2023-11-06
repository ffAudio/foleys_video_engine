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

#pragma once

namespace foleys
{

class D3DManager
{
 public:
    D3DManager() 
    {
        // Create a D3D Manager
        [[maybe_unused]] auto hr = CreateD3DManager();
        jassert (!FAILED (hr));
    }

    ~D3DManager() 
    {
        if (pDXGIMan) { pDXGIMan->ResetDevice (pDX11Device.get(), ResetToken); }
    }

    IMFDXGIDeviceManager* getDeviceManager() 
    {
        return pDXGIMan.get();
    }

private:

    HRESULT CreateDX11Device (_Out_ ID3D11Device** ppDevice, _Out_ ID3D11DeviceContext** ppDeviceContext, _Out_ D3D_FEATURE_LEVEL* pFeatureLevel)
   {
       HRESULT                        hr       = S_OK;
       static const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
                                                   D3D_FEATURE_LEVEL_9_3,  D3D_FEATURE_LEVEL_9_2,  D3D_FEATURE_LEVEL_9_1 };


       hr = D3D11CreateDevice (nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_VIDEO_SUPPORT, levels, ARRAYSIZE (levels), D3D11_SDK_VERSION,
                               ppDevice, pFeatureLevel, ppDeviceContext);

       if (SUCCEEDED (hr))
       {
           MFScopedPointer<ID3D10Multithread> pMultithread;
           hr = ((*ppDevice)->QueryInterface (IID_PPV_ARGS (pMultithread.getPointer())));

           if (SUCCEEDED (hr)) { pMultithread->SetMultithreadProtected (TRUE); }
       }

       return hr;
   }

   HRESULT CreateD3DManager()
   {
       HRESULT              hr = S_OK;
       D3D_FEATURE_LEVEL    FeatureLevel;
       MFScopedPointer<ID3D11DeviceContext> pDX11DeviceContext;

       hr = CreateDX11Device (pDX11Device.getPointer(), pDX11DeviceContext.getPointer(), &FeatureLevel);

       if (SUCCEEDED (hr)) { hr = MFCreateDXGIDeviceManager (&ResetToken, pDXGIMan.getPointer()); }

       if (SUCCEEDED (hr)) { hr = pDXGIMan->ResetDevice (pDX11Device.get(), ResetToken); }

       return hr;
   }

    MFScopedPointer<IMFDXGIDeviceManager> pDXGIMan;
    MFScopedPointer<ID3D11Device>         pDX11Device;
    UINT                                  ResetToken = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (D3DManager)
};

}  // namespace foleys