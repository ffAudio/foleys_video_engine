// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.


// Miscellaneous helper functions.

#include <wincodec.h>

HRESULT CopyAttribute(IMFAttributes *pSrc, IMFAttributes *pDest, const GUID& key)
{
    PROPVARIANT var;
    PropVariantInit( &var );
    HRESULT hr = pSrc->GetItem(key, &var);
    if (SUCCEEDED(hr))
    {
        hr = pDest->SetItem(key, var);
        PropVariantClear(&var);
    }
    return hr;
}


// Creates a compatible video format with a different subtype.

HRESULT CloneVideoMediaType(IMFMediaType *pSrcMediaType, REFGUID guidSubType, IMFMediaType **ppNewMediaType)
{
    foleys::MFScopedPointer<IMFMediaType> pNewMediaType;

    HRESULT hr = MFCreateMediaType(pNewMediaType.getPointer());
    if (FAILED (hr))
        return hr;

    hr = pNewMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (FAILED (hr))
        return hr;

    hr = pNewMediaType->SetGUID(MF_MT_SUBTYPE, guidSubType);
    if (FAILED (hr))
        return hr;

    hr = CopyAttribute (pSrcMediaType, pNewMediaType.get(), MF_MT_FRAME_SIZE);
    if (FAILED (hr))
        return hr;

    hr = CopyAttribute (pSrcMediaType, pNewMediaType.get(), MF_MT_FRAME_RATE);
    if (FAILED (hr))
        return hr;

    hr = CopyAttribute (pSrcMediaType, pNewMediaType.get(), MF_MT_PIXEL_ASPECT_RATIO);
    if (FAILED (hr))
        return hr;

    hr = CopyAttribute (pSrcMediaType, pNewMediaType.get(), MF_MT_INTERLACE_MODE);
    if (FAILED (hr))
        return hr;

    *ppNewMediaType = pNewMediaType.get();
    (*ppNewMediaType)->AddRef();

    return hr;
}

// Creates a JPEG image type that is compatible with a specified video media type.

HRESULT CreatePhotoMediaType(IMFMediaType *pSrcMediaType, IMFMediaType **ppPhotoMediaType)
{
    *ppPhotoMediaType = NULL;

    const UINT32 uiFrameRateNumerator = 30;
    const UINT32 uiFrameRateDenominator = 1;

    
    foleys::MFScopedPointer<IMFMediaType> pPhotoMediaType;

    HRESULT hr = MFCreateMediaType(pPhotoMediaType.getPointer());
    if (FAILED (hr))
        return hr;

    hr = pPhotoMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Image);
    if (FAILED (hr))
        return hr;

    hr = pPhotoMediaType->SetGUID(MF_MT_SUBTYPE, GUID_ContainerFormatJpeg);
    if (FAILED (hr)) 
        return hr;

    hr = CopyAttribute(pSrcMediaType, pPhotoMediaType.get(), MF_MT_FRAME_SIZE);
    if (FAILED (hr)) 
        return hr;

    *ppPhotoMediaType = pPhotoMediaType.get();
    (*ppPhotoMediaType)->AddRef();

    return hr;
}

