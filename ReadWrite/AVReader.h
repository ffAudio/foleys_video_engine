
#pragma once

namespace foleys
{

class AVReader
{
public:
    AVReader() = default;
    virtual ~AVReader() = default;

    bool isOpenedOk() const;

    AVSize originalSize;
    int    pixelFormat;

protected:
    bool   opened = false;


private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AVReader)
};

}
