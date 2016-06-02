#include "CustomHMDPrivatePCH.h"
#include "CustomWindows.h"

#include <stdint.h>
#include <algorithm>
#include <memory>
#include <fstream>

#include "AllowWindowsPlatformTypes.h"
#include <wrl/client.h>
#include "turbojpeg.h"
#include "HideWindowsPlatformTypes.h"

using Microsoft::WRL::ComPtr;

/** Begin code taken from DirectXTex */

size_t CustomWindows::BitsPerPixel(DXGI_FORMAT fmt) {
  switch (static_cast<int>(fmt)) {
  case DXGI_FORMAT_R32G32B32A32_TYPELESS:
  case DXGI_FORMAT_R32G32B32A32_FLOAT:
  case DXGI_FORMAT_R32G32B32A32_UINT:
  case DXGI_FORMAT_R32G32B32A32_SINT:
    return 128;

  case DXGI_FORMAT_R32G32B32_TYPELESS:
  case DXGI_FORMAT_R32G32B32_FLOAT:
  case DXGI_FORMAT_R32G32B32_UINT:
  case DXGI_FORMAT_R32G32B32_SINT:
    return 96;

  case DXGI_FORMAT_R16G16B16A16_TYPELESS:
  case DXGI_FORMAT_R16G16B16A16_FLOAT:
  case DXGI_FORMAT_R16G16B16A16_UNORM:
  case DXGI_FORMAT_R16G16B16A16_UINT:
  case DXGI_FORMAT_R16G16B16A16_SNORM:
  case DXGI_FORMAT_R16G16B16A16_SINT:
  case DXGI_FORMAT_R32G32_TYPELESS:
  case DXGI_FORMAT_R32G32_FLOAT:
  case DXGI_FORMAT_R32G32_UINT:
  case DXGI_FORMAT_R32G32_SINT:
  case DXGI_FORMAT_R32G8X24_TYPELESS:
  case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
  case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
  case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
  case DXGI_FORMAT_Y416:
  case DXGI_FORMAT_Y210:
  case DXGI_FORMAT_Y216:
    return 64;

  case DXGI_FORMAT_R10G10B10A2_TYPELESS:
  case DXGI_FORMAT_R10G10B10A2_UNORM:
  case DXGI_FORMAT_R10G10B10A2_UINT:
  case DXGI_FORMAT_R11G11B10_FLOAT:
  case DXGI_FORMAT_R8G8B8A8_TYPELESS:
  case DXGI_FORMAT_R8G8B8A8_UNORM:
  case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
  case DXGI_FORMAT_R8G8B8A8_UINT:
  case DXGI_FORMAT_R8G8B8A8_SNORM:
  case DXGI_FORMAT_R8G8B8A8_SINT:
  case DXGI_FORMAT_R16G16_TYPELESS:
  case DXGI_FORMAT_R16G16_FLOAT:
  case DXGI_FORMAT_R16G16_UNORM:
  case DXGI_FORMAT_R16G16_UINT:
  case DXGI_FORMAT_R16G16_SNORM:
  case DXGI_FORMAT_R16G16_SINT:
  case DXGI_FORMAT_R32_TYPELESS:
  case DXGI_FORMAT_D32_FLOAT:
  case DXGI_FORMAT_R32_FLOAT:
  case DXGI_FORMAT_R32_UINT:
  case DXGI_FORMAT_R32_SINT:
  case DXGI_FORMAT_R24G8_TYPELESS:
  case DXGI_FORMAT_D24_UNORM_S8_UINT:
  case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
  case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
  case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
  case DXGI_FORMAT_R8G8_B8G8_UNORM:
  case DXGI_FORMAT_G8R8_G8B8_UNORM:
  case DXGI_FORMAT_B8G8R8A8_UNORM:
  case DXGI_FORMAT_B8G8R8X8_UNORM:
  case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
  case DXGI_FORMAT_B8G8R8A8_TYPELESS:
  case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
  case DXGI_FORMAT_B8G8R8X8_TYPELESS:
  case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
  case DXGI_FORMAT_AYUV:
  case DXGI_FORMAT_Y410:
  case DXGI_FORMAT_YUY2:
    return 32;

  case DXGI_FORMAT_P010:
  case DXGI_FORMAT_P016:
    return 24;

  case DXGI_FORMAT_R8G8_TYPELESS:
  case DXGI_FORMAT_R8G8_UNORM:
  case DXGI_FORMAT_R8G8_UINT:
  case DXGI_FORMAT_R8G8_SNORM:
  case DXGI_FORMAT_R8G8_SINT:
  case DXGI_FORMAT_R16_TYPELESS:
  case DXGI_FORMAT_R16_FLOAT:
  case DXGI_FORMAT_D16_UNORM:
  case DXGI_FORMAT_R16_UNORM:
  case DXGI_FORMAT_R16_UINT:
  case DXGI_FORMAT_R16_SNORM:
  case DXGI_FORMAT_R16_SINT:
  case DXGI_FORMAT_B5G6R5_UNORM:
  case DXGI_FORMAT_B5G5R5A1_UNORM:
  case DXGI_FORMAT_A8P8:
  case DXGI_FORMAT_B4G4R4A4_UNORM:
    return 16;

  case DXGI_FORMAT_NV12:
  case DXGI_FORMAT_420_OPAQUE:
  case DXGI_FORMAT_NV11:
    return 12;

  case DXGI_FORMAT_R8_TYPELESS:
  case DXGI_FORMAT_R8_UNORM:
  case DXGI_FORMAT_R8_UINT:
  case DXGI_FORMAT_R8_SNORM:
  case DXGI_FORMAT_R8_SINT:
  case DXGI_FORMAT_A8_UNORM:
  case DXGI_FORMAT_AI44:
  case DXGI_FORMAT_IA44:
  case DXGI_FORMAT_P8:
    return 8;

  case DXGI_FORMAT_R1_UNORM:
    return 1;

  case DXGI_FORMAT_BC1_TYPELESS:
  case DXGI_FORMAT_BC1_UNORM:
  case DXGI_FORMAT_BC1_UNORM_SRGB:
  case DXGI_FORMAT_BC4_TYPELESS:
  case DXGI_FORMAT_BC4_UNORM:
  case DXGI_FORMAT_BC4_SNORM:
    return 4;

  case DXGI_FORMAT_BC2_TYPELESS:
  case DXGI_FORMAT_BC2_UNORM:
  case DXGI_FORMAT_BC2_UNORM_SRGB:
  case DXGI_FORMAT_BC3_TYPELESS:
  case DXGI_FORMAT_BC3_UNORM:
  case DXGI_FORMAT_BC3_UNORM_SRGB:
  case DXGI_FORMAT_BC5_TYPELESS:
  case DXGI_FORMAT_BC5_UNORM:
  case DXGI_FORMAT_BC5_SNORM:
  case DXGI_FORMAT_BC6H_TYPELESS:
  case DXGI_FORMAT_BC6H_UF16:
  case DXGI_FORMAT_BC6H_SF16:
  case DXGI_FORMAT_BC7_TYPELESS:
  case DXGI_FORMAT_BC7_UNORM:
  case DXGI_FORMAT_BC7_UNORM_SRGB:
    return 8;

  default:
    return 0;
  }
}

void CustomWindows::ComputePitch(DXGI_FORMAT fmt, size_t width, size_t height,
  size_t& rowPitch, size_t& slicePitch) {
  switch (static_cast<int>(fmt)) {
  case DXGI_FORMAT_BC1_TYPELESS:
  case DXGI_FORMAT_BC1_UNORM:
  case DXGI_FORMAT_BC1_UNORM_SRGB:
  case DXGI_FORMAT_BC4_TYPELESS:
  case DXGI_FORMAT_BC4_UNORM:
  case DXGI_FORMAT_BC4_SNORM:
  {
    size_t nbw = std::max<size_t>(1, (width + 3) / 4);
    size_t nbh = std::max<size_t>(1, (height + 3) / 4);
    rowPitch = nbw * 8;

    slicePitch = rowPitch * nbh;
    break;
  }

  case DXGI_FORMAT_BC2_TYPELESS:
  case DXGI_FORMAT_BC2_UNORM:
  case DXGI_FORMAT_BC2_UNORM_SRGB:
  case DXGI_FORMAT_BC3_TYPELESS:
  case DXGI_FORMAT_BC3_UNORM:
  case DXGI_FORMAT_BC3_UNORM_SRGB:
  case DXGI_FORMAT_BC5_TYPELESS:
  case DXGI_FORMAT_BC5_UNORM:
  case DXGI_FORMAT_BC5_SNORM:
  case DXGI_FORMAT_BC6H_TYPELESS:
  case DXGI_FORMAT_BC6H_UF16:
  case DXGI_FORMAT_BC6H_SF16:
  case DXGI_FORMAT_BC7_TYPELESS:
  case DXGI_FORMAT_BC7_UNORM:
  case DXGI_FORMAT_BC7_UNORM_SRGB:
  {
    size_t nbw = std::max<size_t>(1, (width + 3) / 4);
    size_t nbh = std::max<size_t>(1, (height + 3) / 4);
    rowPitch = nbw * 16;

    slicePitch = rowPitch * nbh;
    break;
  }

  case DXGI_FORMAT_R8G8_B8G8_UNORM:
  case DXGI_FORMAT_G8R8_G8B8_UNORM:
  case DXGI_FORMAT_YUY2:
    rowPitch = ((width + 1) >> 1) * 4;
    slicePitch = rowPitch * height;
    break;

  case DXGI_FORMAT_Y210:
  case DXGI_FORMAT_Y216:
    rowPitch = ((width + 1) >> 1) * 8;
    slicePitch = rowPitch * height;
    break;

  case DXGI_FORMAT_NV12:
  case DXGI_FORMAT_420_OPAQUE:
    rowPitch = ((width + 1) >> 1) * 2;
    slicePitch = rowPitch * (height + ((height + 1) >> 1));
    break;

  case DXGI_FORMAT_P010:
  case DXGI_FORMAT_P016:
    rowPitch = ((width + 1) >> 1) * 4;
    slicePitch = rowPitch * (height + ((height + 1) >> 1));
    break;

  case DXGI_FORMAT_NV11:
    rowPitch = ((width + 3) >> 2) * 4;
    slicePitch = rowPitch * height * 2;
    break;

  default:
  {
    size_t bpp = BitsPerPixel(fmt);
    rowPitch = (width * bpp + 7) / 8;
    slicePitch = rowPitch * height;
    break;
  }

  }
}

size_t CustomWindows::ComputeScanlines(DXGI_FORMAT fmt, size_t height) {
  switch (static_cast<int>(fmt)) {
  case DXGI_FORMAT_BC1_TYPELESS:
  case DXGI_FORMAT_BC1_UNORM:
  case DXGI_FORMAT_BC1_UNORM_SRGB:
  case DXGI_FORMAT_BC2_TYPELESS:
  case DXGI_FORMAT_BC2_UNORM:
  case DXGI_FORMAT_BC2_UNORM_SRGB:
  case DXGI_FORMAT_BC3_TYPELESS:
  case DXGI_FORMAT_BC3_UNORM:
  case DXGI_FORMAT_BC3_UNORM_SRGB:
  case DXGI_FORMAT_BC4_TYPELESS:
  case DXGI_FORMAT_BC4_UNORM:
  case DXGI_FORMAT_BC4_SNORM:
  case DXGI_FORMAT_BC5_TYPELESS:
  case DXGI_FORMAT_BC5_UNORM:
  case DXGI_FORMAT_BC5_SNORM:
  case DXGI_FORMAT_BC6H_TYPELESS:
  case DXGI_FORMAT_BC6H_UF16:
  case DXGI_FORMAT_BC6H_SF16:
  case DXGI_FORMAT_BC7_TYPELESS:
  case DXGI_FORMAT_BC7_UNORM:
  case DXGI_FORMAT_BC7_UNORM_SRGB:
    return std::max<size_t>(1, (height + 3) / 4);

  case DXGI_FORMAT_NV11:
    return height * 2;

  case DXGI_FORMAT_NV12:
  case DXGI_FORMAT_P010:
  case DXGI_FORMAT_P016:
  case DXGI_FORMAT_420_OPAQUE:
    return height + ((height + 1) >> 1);

  default:
    return height;
  }
}

/** End code taken from DirectXTex */

void CustomWindows::DoImageStuff(TSharedPtr<LibraryInitParams>& libraryParams, ID3D11Texture2D* source, uint8_t** data, size_t* size) {
  ComPtr<ID3D11Device> device;
  source->GetDevice(&device);

  ComPtr<ID3D11DeviceContext> context;
  device->GetImmediateContext(&context);

  D3D11_TEXTURE2D_DESC desc;
  source->GetDesc(&desc);

  desc.BindFlags = 0;
  desc.MiscFlags = 0;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  desc.Usage = D3D11_USAGE_STAGING;

  if (desc.Width == 0 || desc.Height == 0) {
    return;
  }

  ComPtr<ID3D11Texture2D> staging;
  device->CreateTexture2D(&desc, nullptr, &staging);

  context->CopyResource(staging.Get(), source);

  size_t rowPitch, slicePitch;
  ComputePitch(desc.Format, desc.Width, desc.Height, rowPitch, slicePitch);
  size_t lines = ComputeScanlines(desc.Format, desc.Height);

  //UE_LOG(LogTemp, Warning, TEXT("Width %d"), desc.Width);
  //UE_LOG(LogTemp, Warning, TEXT("Height %d"), desc.Height);
  //UE_LOG(LogTemp, Warning, TEXT("Row %d"), rowPitch);
  //UE_LOG(LogTemp, Warning, TEXT("Slice %d"), slicePitch);

  if (rowPitch == 0 || slicePitch == 0) {
    //UE_LOG(LogTemp, Warning, TEXT("BAD DIMS!"));
    return;
  }

  D3D11_MAPPED_SUBRESOURCE mapped;
  HRESULT hr = context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped);
  //UE_LOG(LogTemp, Warning, TEXT("ExistingRow %d"), mapped.RowPitch);
  //UE_LOG(LogTemp, Warning, TEXT("DepthPitch %d"), mapped.DepthPitch);
  //UE_LOG(LogTemp, Warning, TEXT("Scanlines %d"), lines);

  if (mapped.RowPitch == 0 || mapped.DepthPitch == 0) {
    //UE_LOG(LogTemp, Warning, TEXT("NO MAPPING!"));
    return;
  }

  auto sptr = reinterpret_cast<const uint8_t*>(mapped.pData);
  if (*size < slicePitch) {
    UE_LOG(LogTemp, Warning, TEXT("Require resize, old = %d, new = %d"), *size, slicePitch);
    if (*data != nullptr) {
      delete[] * data;
    }
    *data = new uint8_t[slicePitch];
    *size = slicePitch;
  }

  // TODO: never deleted!
  uint8_t* dptr = *data;

  // Source row pitch should be >= dest row pitch (source might have padding).
  for (size_t h = 0; h < lines; ++h) {
    size_t msize = std::min<size_t>(rowPitch, mapped.RowPitch);
    memcpy_s(dptr, rowPitch, sptr, msize);
    sptr += mapped.RowPitch;
    dptr += rowPitch;
  }

  // TODO: check actual format (90 = DXGI_FORMAT_B8G8R8A8_TYPELESS)!
  unsigned char *outBuf = nullptr;
  unsigned long outSize = 0;
  int status = tjCompress2(libraryParams->TurboJpegCompressor, *data, desc.Width, rowPitch, desc.Height, TJPF_BGRX, &outBuf, &outSize, TJSAMP_444, 100, 0);
  UE_LOG(LogTemp, Warning, TEXT("image status %d, format %d"), status, (int)desc.Format);

  {
    std::ofstream file("C:\\Users\\Steve\\Downloads\\example.jpeg", std::ios::binary);
    file.write(reinterpret_cast<char*>(outBuf), outSize);
  }

  tjFree(outBuf);

  context->Unmap(staging.Get(), 0);
}