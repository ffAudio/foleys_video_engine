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
 */

#pragma once


namespace foleys
{

struct ColourCurve
{
    double brightness = -1.0;
    double contrast   = -1.0;
    double gamma      = -1.0;
    bool   isLinear   = false;
    
    uint8_t map[256];
    
    void calculateColourMap (double newBrightness, double newContrast, double newGamma)
    {
        if (newBrightness == brightness && newContrast == contrast && newGamma == gamma)
            return;
        
        brightness = newBrightness;
        contrast = newContrast;
        gamma = newGamma;
        
        isLinear = (brightness == 0.0 && contrast == 0.0 && gamma == 1.0);
        
        if (isLinear)
        {
            for (size_t i = 0; i < 256; ++i)
                map [i] = i;
        }
        else
        {
            for (size_t i = 0; i < 256; ++i)
                map [i] = juce::jlimit (0.0, 255.0,
                                        ((std::pow (i / 255.0, gamma) - 0.5) * (contrast + 1.0) + 0.5
                                         + brightness) * 255.0);
        }
    }

    void applyLUT (juce::Image& image, int component)
    {
        juce::Image::BitmapData data (image, 0, 0,
                                      image.getWidth(),
                                      image.getHeight());
        
        for (int y=0; y < data.height; ++y)
        {
            auto* p = data.getLinePointer (y);
            for (int x=0; x < data.width; ++x)
            {
                p += component;
                *p = map [*p];
                p += data.pixelStride - component;
            }
        }
    }

    static void applyLUTs (juce::Image& image, const ColourCurve& red, const ColourCurve& green, const ColourCurve& blue)
    {
        juce::Image::BitmapData data (image, 0, 0,
                                      image.getWidth(),
                                      image.getHeight());
        if (data.pixelStride == 4)
        {
            for (int y=0; y < data.height; ++y)
            {
                auto* p = data.getLinePointer (y);
                for (int x=0; x < data.width; ++x)
                {
                    *p = blue.map [*p]; ++p;
                    *p = green.map [*p]; ++p;
                    *p = red.map [*p]; ++p;
                    ++p;
                }
            }
        }
        else if (data.pixelStride == 3)
        {
            for (int y=0; y < data.height; ++y)
            {
                auto* p = data.getLinePointer (y);
                for (int x=0; x < data.width; ++x)
                {
                    *p = blue.map [*p]; ++p;
                    *p = green.map [*p]; ++p;
                    *p = red.map [*p]; ++p;
                }
            }
        }
    }
};

} // namespace foleys
