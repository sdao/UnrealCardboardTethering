#include "CustomHMDPrivatePCH.h"
#include "MayaUsbDevice.h"
#include "EndianUtils.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <algorithm>

#include "AllowWindowsPlatformTypes.h"
#define NOMINMAX
#include <wrl/client.h>
#include "libusb.h"
#include "turbojpeg.h"
#include "HideWindowsPlatformTypes.h"

#define CHECKSTATUS(status) if (status) return status;

using Microsoft::WRL::ComPtr;

int MayaUsbDevice::create(TSharedPtr<MayaUsbDevice>* out,
  TSharedPtr<LibraryInitParams>& initParams,
  uint16_t vid, uint16_t pid) {
  return create(out, initParams, { MayaUsbDeviceId(vid, pid) });
}

int MayaUsbDevice::create(TSharedPtr<MayaUsbDevice>* out,
  TSharedPtr<LibraryInitParams>& initParams,
  std::vector<MayaUsbDeviceId> ids) {
  int status;

  MayaUsbDeviceId outId;
  libusb_device_handle* outHandle = nullptr;
  std::string outManufacturer, outProduct;
  uint8_t outInputEndpoint = 0, outOutputEndpoint = 0;

  for (const MayaUsbDeviceId& id : ids) {
    libusb_device_handle* tempHnd = libusb_open_device_with_vid_pid(
      initParams->UsbContext, id.vid, id.pid);
    if (tempHnd != nullptr) {
      outId = id;
      outHandle = tempHnd;
      break;
    }
  }
  if (outHandle == nullptr) {
    return STATUS_NOT_FOUND_ERROR;
  }

  libusb_device* dev = libusb_get_device(outHandle);

  libusb_device_descriptor desc;
  status = libusb_get_device_descriptor(dev, &desc);
  if (status < 0) {
    libusb_close(outHandle);
    return STATUS_DEVICE_DESCRIPTOR_ERROR;
  }

  char manufacturerString[256];
  status = libusb_get_string_descriptor_ascii(
    outHandle,
    desc.iManufacturer,
    reinterpret_cast<unsigned char*>(manufacturerString),
    sizeof(manufacturerString)
    );
  if (status < 0) {
    libusb_close(outHandle);
    return STATUS_DESCRIPTOR_READ_ERROR;
  }

  char productString[256];
  status = libusb_get_string_descriptor_ascii(
    outHandle,
    desc.iProduct,
    reinterpret_cast<unsigned char*>(productString),
    sizeof(productString)
    );
  if (status < 0) {
    libusb_close(outHandle);
    return STATUS_DESCRIPTOR_READ_ERROR;
  }

  outManufacturer = std::string(manufacturerString);
  outProduct = std::string(productString);

  libusb_config_descriptor* configDesc;
  status = libusb_get_active_config_descriptor(dev, &configDesc);
  if (status < 0) {
    libusb_close(outHandle);
    return STATUS_CONFIG_DESCRIPTOR_ERROR;
  }

  const libusb_interface& interface = configDesc->interface[0];
  const libusb_interface_descriptor& interfaceDesc = interface.altsetting[0];

  for (int i = 0; i < interfaceDesc.bNumEndpoints; ++i) {
    const libusb_endpoint_descriptor& endpoint = interfaceDesc.endpoint[i];
    bool ie = (endpoint.bEndpointAddress & 0b10000000) == LIBUSB_ENDPOINT_IN;
    bool oe = (endpoint.bEndpointAddress & 0b10000000) == LIBUSB_ENDPOINT_OUT;

    if (ie) {
      outInputEndpoint = endpoint.bEndpointAddress;
    } else if (oe) {
      outOutputEndpoint = endpoint.bEndpointAddress;
    }
  }

  libusb_free_config_descriptor(configDesc);

  status = libusb_claim_interface(outHandle, 0);
  if (status < 0) {
    libusb_close(outHandle);
    return STATUS_INTERFACE_CLAIM_ERROR;
  }

  // Populate device.
  *out = TSharedPtr<MayaUsbDevice>(new MayaUsbDevice(initParams,
    outId, outHandle,
    outManufacturer, outProduct,
    outInputEndpoint, outOutputEndpoint));
  return STATUS_OK;
}

MayaUsbDevice::MayaUsbDevice(TSharedPtr<LibraryInitParams>& initParams,
  MayaUsbDeviceId id,
  libusb_device_handle* handle,
  std::string manufacturer,
  std::string product,
  uint8_t inEndpoint,
  uint8_t outEndpoint)
  : _initParams(initParams),
    _id(id),
    _hnd(handle),
    _manufacturer(manufacturer),
    _product(product),
    _inEndpoint(inEndpoint),
    _outEndpoint(outEndpoint),
    _receiveWorker(nullptr),
    _sendWorker(nullptr),
    _handshake(false),
    _sendReady(false),
    _rgbImageBuffer(new unsigned char[RGB_IMAGE_SIZE]),
    _jpegBuffer(nullptr) {}

MayaUsbDevice::~MayaUsbDevice() {
  bool receiving = _receiveWorker && !_receiveWorker->isCancelled();
  bool sending = _sendWorker && !_sendWorker->isCancelled();
  bool needDelay = false;

  if (receiving) {
    _receiveWorker->cancel();
    needDelay = true;
  }

  if (sending) {
    _sendWorker->cancel();
    _sendCv.notify_one(); // Wake up the send loop so it can cancel.
    needDelay = true;
  }

  if (needDelay) {
    // Wait 2s since our send/receive loops check every 500ms for cancel flag.
    std::this_thread::sleep_for(std::chrono::seconds(2));
  }

  delete[] _rgbImageBuffer;
  if (_jpegBuffer != nullptr) {
    tjFree(_jpegBuffer);
  }

  libusb_release_interface(_hnd, 0);
  libusb_close(_hnd);
}

void MayaUsbDevice::flushInputBuffer(unsigned char* buf) {
  if (_inEndpoint != 0) {
    int status = 0;
    int read;
    while (status == 0) {
      status = libusb_bulk_transfer(_hnd,
        _inEndpoint,
        buf,
        BUFFER_LEN,
        &read,
        10);
    }
  }
}

std::string MayaUsbDevice::getDescription() {
  std::ostringstream os;
  os << std::setfill('0') << std::hex
     << std::setw(4) << _id.vid << ":"
     << std::setw(4) << _id.pid << " "
     << std::dec << std::setfill(' ');

  // Print device manufacturer and product name.
  os << _manufacturer << " " << _product;
  return os.str();
}

int MayaUsbDevice::getControlInt16(int16_t* out, uint8_t request) {
  int16_t data;
  if (libusb_control_transfer(
      _hnd,
      LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR,
      request,
      0,
      0,
      reinterpret_cast<unsigned char*>(&data),
      sizeof(data),
      0) < 0) {
    return STATUS_RECEIVE_ERROR;
  }

  *out = data;
  return STATUS_OK;
}

int MayaUsbDevice::sendControl(uint8_t request) {
  if (libusb_control_transfer(
      _hnd,
      LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
      request,
      0,
      0,
      nullptr,
      0,
      0) < 0) {
    return STATUS_SEND_ERROR;
  }

  return STATUS_OK;
}

int MayaUsbDevice::sendControlString(uint8_t request, uint16_t index,
    std::string str) {
  char temp[256];
  str.copy(&temp[0], sizeof(temp));
  if (libusb_control_transfer(
      _hnd,
      LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR,
      request,
      0,
      index,
      reinterpret_cast<unsigned char*>(temp),
      str.size(),
      0) < 0) {
    return STATUS_SEND_ERROR;
  }

  return STATUS_OK;
}

int MayaUsbDevice::convertToAccessory() {
  int status = 0;

  // Get protocol.
  int16_t protocolVersion;
  status = getControlInt16(&protocolVersion, 51);
  CHECKSTATUS(status);
  if (protocolVersion < 1) {
    return STATUS_BAD_PROTOCOL_VERSION;
  }

  // Send manufacturer string.
  status = sendControlString(52, 0, "SiriusCybernetics");
  CHECKSTATUS(status);

  // Send model string.
  status = sendControlString(52, 1, "MayaUsb");
  CHECKSTATUS(status);

  // Send description.
  status = sendControlString(52, 2, "Maya USB streaming");
  CHECKSTATUS(status);

  // Send version.
  status = sendControlString(52, 3, "0.42");
  CHECKSTATUS(status);

  // Send URI.
  status = sendControlString(52, 4, "https://sdao.me");
  CHECKSTATUS(status);

  // Send serial number.
  status = sendControlString(52, 5, "42");
  CHECKSTATUS(status);

  // Start accessory.
  status = sendControl(53);
  CHECKSTATUS(status);

  return STATUS_OK;
}

bool MayaUsbDevice::waitHandshakeAsync(std::function<void(bool)> callback) {
  if (_inEndpoint == 0) {
    return false;
  }

  if (_handshake.load()) {
    std::cout << "Handshake previously completed!" << std::endl;
    return false;
  }

  _receiveWorker = std::make_shared<InterruptibleThread>(
    [=](const InterruptibleThread::SharedAtomicBool cancel) {
      unsigned char* inputBuffer = new unsigned char[BUFFER_LEN];
      flushInputBuffer(inputBuffer);

      int i = 0;
      int read = 0;
      int status = LIBUSB_ERROR_TIMEOUT;
      bool cancelled;
      while (true) {
        cancelled = cancel->load();
        if (cancelled || status != LIBUSB_ERROR_TIMEOUT) {
          break;
        }

        std::cout << i++ << " Waiting..." << std::endl;
        status = libusb_bulk_transfer(_hnd,
            _inEndpoint,
            inputBuffer,
            BUFFER_LEN,
            &read,
            500);
      }

      if (cancelled) {
        std::cout << "Handshake cancelled!" << std::endl;
      } else {
        bool success = true;
        if (read == BUFFER_LEN) {
          for (int i = 0; i < BUFFER_LEN; ++i) {
            unsigned char expected = (unsigned char) i;
            if (inputBuffer[i] != expected) {
              std::cout << "Handshake expect=" << (int) expected
                        << ", receive=" << (int) inputBuffer[i] << std::endl;
              success = false;
              break;
            }
          }
        } else {
          std::cout << "Handshake read=" << read << std::endl;
          success = false;
        }
        std::cout << "Received handshake, status=" << status << std::endl;

        _handshake.store(success);
        callback(success);
      }

      delete[] inputBuffer;
      cancel->store(true);
    }
  );

  return true;
}

bool MayaUsbDevice::isHandshakeComplete() {
  return _handshake.load();
}

bool MayaUsbDevice::beginReadLoop(
    std::function<void(const unsigned char*)> callback,
    size_t readFrame) {
  if (_inEndpoint == 0) {
    return false;
  }

  if (!_handshake.load()) {
    return false;
  }

  _receiveWorker = std::make_shared<InterruptibleThread>(
    [=](const InterruptibleThread::SharedAtomicBool cancel) {
      unsigned char* inputBuffer = new unsigned char[readFrame];
      int read = 0;
      int status = LIBUSB_ERROR_TIMEOUT;
      bool cancelled;
      while (true) {
        cancelled = cancel->load();
        if (cancelled || (status != 0 && status != LIBUSB_ERROR_TIMEOUT)) {
          break;
        }

        status = libusb_bulk_transfer(_hnd,
            _inEndpoint,
            inputBuffer,
            readFrame,
            &read,
            500);
        if (status == 0) {
          callback(inputBuffer);
        }
      }
      delete[] inputBuffer;

      if (!cancelled) {
        // Error if loop ended but not cancelled.
        std::cout << "Status in beginReadLoop=" << status << std::endl;
        callback(nullptr);
      }

      std::cout << "Read loop ended" << std::endl;
      cancel->store(true);
    }
  );

  return true;
}

bool MayaUsbDevice::beginSendLoop(std::function<void()> failureCallback) {
  if (_outEndpoint == 0) {
    return false;
  }

  if (!_handshake.load()) {
    return false;
  }

  // Reset in case there was a previous send loop.
  _sendReady = false;

  _sendWorker = std::make_shared<InterruptibleThread>(
    [=](const InterruptibleThread::SharedAtomicBool cancel) {
      while (true) {
        bool error = false;

        {
          std::unique_lock<std::mutex> lock(_sendMutex);
          _sendCv.wait(lock, [&] {
            return _sendReady || cancel->load();
          });

          if (cancel->load()) {
            int written = 0;

            // Write 0 buffer size.
            uint32_t bytes = 0;
            libusb_bulk_transfer(_hnd,
              _outEndpoint,
              reinterpret_cast<unsigned char*>(&bytes),
              4,
              &written,
              500);

            // Ignore if written or not.
            break;
          } else {
            unsigned long jpegBufferSizeUlong;
            tjCompress2(_initParams->TurboJpegCompressor,
              _rgbImageBuffer,
              _jpegBufferWidth,
              _jpegBufferWidthPitch,
              _jpegBufferHeight,
              TJPF_BGRX,
              &_jpegBuffer,
              &jpegBufferSizeUlong,
              TJSAMP_420,
              100 /* quality 1 to 100 */,
              0);

            _jpegBufferSize = jpegBufferSizeUlong;
            int written = 0;

            // Write size of JPEG (32-bit int).
            uint32_t header = EndianUtils::nativeToBig(
              (uint32_t)_jpegBufferSize);

            libusb_bulk_transfer(_hnd,
              _outEndpoint,
              reinterpret_cast<unsigned char*>(&header),
              sizeof(header),
              &written,
              500);
            if (written < sizeof(header)) {
              error = true;
            } else {
              // Write JPEG in BUFFER_LEN chunks.
              for (int i = 0; i < _jpegBufferSize; i += BUFFER_LEN) {
                written = 0;

                int chunk = std::min(BUFFER_LEN, _jpegBufferSize - i);
                libusb_bulk_transfer(_hnd,
                  _outEndpoint,
                  _jpegBuffer + i,
                  chunk,
                  &written,
                  500);

                if (written < chunk) {
                  error = true;
                  break;
                }
              }
            }
          }
        }

        if (error) {
          // Only signal on a send error.
          failureCallback();
          break;
        } else {
          // Only reset send flag if successful.
          _sendReady = false;
        }
      }

      std::cout << "Send loop ended" << std::endl;
      cancel->store(true);
    }
  );

  return true;
}

bool MayaUsbDevice::isSending() {
  return _sendWorker && !_sendWorker->isCancelled();
}

bool MayaUsbDevice::supportsRasterFormat(DXGI_FORMAT format) {
  switch (format) {
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
      return true;
    default:
      return false;
  }
}

bool MayaUsbDevice::sendImage(ID3D11Texture2D* source) {
  // If the send loop is busy, then skip this frame.
  if (_sendMutex.try_lock()) {
    std::lock_guard<std::mutex> lock(_sendMutex, std::adopt_lock);
    if (!_sendReady) {
      ComPtr<ID3D11Device> device;
      source->GetDevice(&device);

      ComPtr<ID3D11DeviceContext> context;
      device->GetImmediateContext(&context);

      D3D11_TEXTURE2D_DESC desc;
      source->GetDesc(&desc);

      if (desc.Width == 0 || desc.Height == 0) {
        return false;
      }

      // We can only deal with BGRA/BGRX 32-bit formats.
      switch (desc.Format) {
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
          break;
        default:
          return false;
      }

      desc.BindFlags = 0;
      desc.MiscFlags = 0;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
      desc.Usage = D3D11_USAGE_STAGING;

      ComPtr<ID3D11Texture2D> staging;
      device->CreateTexture2D(&desc, nullptr, &staging);
      context->CopyResource(staging.Get(), source);

      D3D11_MAPPED_SUBRESOURCE mapped;
      HRESULT hr = context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped);

      if (SUCCEEDED(hr)) {
        int sizeNeeded = mapped.RowPitch * desc.Height;
        if (sizeNeeded > RGB_IMAGE_SIZE) {
          // Can't store image size, not enough pre-allocated memory.
          context->Unmap(staging.Get(), 0);
          return false;
        }

        memcpy_s(_rgbImageBuffer, RGB_IMAGE_SIZE, mapped.pData, sizeNeeded);
        context->Unmap(staging.Get(), 0);

        // Delay JPEG creation until send loop to improve Maya performance.
        _jpegBufferWidth = desc.Width;
        _jpegBufferWidthPitch = mapped.RowPitch;
        _jpegBufferHeight = desc.Height;

        // Dispatch send loop.
        _sendReady = true;
        _sendCv.notify_one();

        return true;
      }
    }
  }

  return false;
}
