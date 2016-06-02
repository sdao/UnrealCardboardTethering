#pragma once

typedef void* tjhandle;
struct libusb_context;

struct LibraryInitParams {
  tjhandle TurboJpegCompressor;
  libusb_context* UsbContext;

  LibraryInitParams();
  ~LibraryInitParams();
};