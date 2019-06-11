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

/**
 The ColourCurve provides an 8-bit lookup table to manipulate brightness, contrast
 and gamma correction per channel.
 */
class ColourCurve
{
public:
    ColourCurve() = default;

    /**
     calculateColourMap will set up the lookup table.
     @param newBrightness will add or subtract a normalised value (-1..1)
     @param newContrast multiplies the slope of the curve (-1..1)
     @param newGamma the gamma value for the curve (0.1..4.0)
     */
    void calculateColourMap (double newBrightness, double newContrast, double newGamma)
    {
        if (newBrightness == brightness && newContrast == contrast && newGamma == gamma)
            return;

        brightness = newBrightness;
        contrast = newContrast;
        gamma = newGamma;

        if (isLinear())
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

    /**
     The method isLinear returns true, if the curve is the identity function.
     */
    bool isLinear() const
    {
        return (brightness == 0.0 && contrast == 0.0 && gamma == 1.0);
    }

    /**
     Applies a ColourCurve to a channel of an image.

     @param image the image to apply the ColourCurve
     @param component is the index of the channel in the packed pixel
     */
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

    /**
     Applies a set of ColourCurves to an image.
     This method assumes BGR or BGRA format.

     @param image the image to apply the ColourCurve
     @param red is the ColourCurve for the red channel
     @param green is the ColourCurve for the green channel
     @param blue is the ColourCurve for the blue channel
     */
    static void applyLUTs (juce::Image& image, const ColourCurve& red, const ColourCurve& green, const ColourCurve& blue)
    {
        juce::Image::BitmapData data (image, 0, 0,
                                      image.getWidth(),
                                      image.getHeight());

        const auto* redMap = red.getLookupTable();
        const auto* greenMap = green.getLookupTable();
        const auto* blueMap = blue.getLookupTable();

        if (data.pixelStride == 4)
        {
            for (int y=0; y < data.height; ++y)
            {
                auto* p = data.getLinePointer (y);
                for (int x=0; x < data.width; ++x)
                {
                    *p = blueMap [*p]; ++p;
                    *p = greenMap [*p]; ++p;
                    *p = redMap [*p]; ++p;
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
                    *p = blueMap [*p]; ++p;
                    *p = greenMap [*p]; ++p;
                    *p = redMap [*p]; ++p;
                }
            }
        }
    }

    /**
     Applies a set of ColourCurves to an image.
     This method assumes BGRA format.

     @param image the image to apply the ColourCurve
     @param red is the ColourCurve for the red channel
     @param green is the ColourCurve for the green channel
     @param blue is the ColourCurve for the blue channel
     @param alpha is the ColourCurve for the alpha channel
     */
    static void applyLUTs (juce::Image& image,
                           const ColourCurve& red,
                           const ColourCurve& green,
                           const ColourCurve& blue,
                           const ColourCurve& alpha)
    {
        juce::Image::BitmapData data (image, 0, 0,
                                      image.getWidth(),
                                      image.getHeight());

        // You need pixels with 4 components to apply 4 LUTs
        jassert (data.pixelStride == 4);

        if (data.pixelStride != 4)
            return;

        const auto* redMap = red.getLookupTable();
        const auto* greenMap = green.getLookupTable();
        const auto* blueMap = blue.getLookupTable();
        const auto* alphaMap = alpha.getLookupTable();

        for (int y=0; y < data.height; ++y)
        {
            auto* p = data.getLinePointer (y);
            for (int x=0; x < data.width; ++x)
            {
                *p = blueMap [*p]; ++p;
                *p = greenMap [*p]; ++p;
                *p = redMap [*p]; ++p;
                *p = alphaMap [*p]; ++p;
            }
        }
    }

    /**
     This methods gives you reading access to the lookup table.
     */
    const uint8_t* getLookupTable() const
    {
        return map;
    }

private:
    double brightness = -1.0;
    double contrast   = -1.0;
    double gamma      = -1.0;

    uint8_t map[256];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ColourCurve)
};

} // foleys
