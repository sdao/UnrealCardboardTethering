#include "CustomHMDPrivatePCH.h"
#include "LibraryInitParams.h"

#include "AllowWindowsPlatformTypes.h"
#include "turbojpeg.h"
#include "libusb.h"
#include "HideWindowsPlatformTypes.h"

LibraryInitParams::LibraryInitParams() {
  TurboJpegCompressor = tjInitCompress();
  libusb_init(&UsbContext);
}

LibraryInitParams::~LibraryInitParams() {
  tjDestroy(TurboJpegCompressor);
  libusb_exit(UsbContext);
}