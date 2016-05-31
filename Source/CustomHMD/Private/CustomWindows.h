#pragma once

#include <stdint.h>
#include "LibraryInitParams.h"

#include "AllowWindowsPlatformTypes.h"
#include <d3d11.h>
#include "HideWindowsPlatformTypes.h"

class CustomWindows {
  static size_t BitsPerPixel(DXGI_FORMAT fmt);
  static void ComputePitch(DXGI_FORMAT fmt, size_t width, size_t height,
    size_t& rowPitch, size_t& slicePitch);
  static size_t ComputeScanlines(DXGI_FORMAT fmt, size_t height);

public:
  static void DoImageStuff(TSharedPtr<LibraryInitParams>& libraryParams, ID3D11Texture2D* source, uint8_t** data, size_t* size);
};
