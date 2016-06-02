#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "LibraryInitParams.h"

#include "AllowWindowsPlatformTypes.h"
#define NOMINMAX
#include <d3d11.h>
#include "HideWindowsPlatformTypes.h"

struct libusb_device_handle;

class InterruptibleThread {
public:
  using SharedAtomicBool = std::shared_ptr<std::atomic_bool>;

  InterruptibleThread(std::function<void(const SharedAtomicBool)> func) {
    _cancel = std::make_shared<std::atomic_bool>(false);
    std::thread thread([=]() {
      func(_cancel);
    });
    thread.detach();
  }

  ~InterruptibleThread() {
    cancel();
  }

  void cancel() { _cancel->store(true); }
  bool isCancelled() { return _cancel->load(); }

private:
  SharedAtomicBool _cancel;
};

struct MayaUsbDeviceId {
  uint16_t vid;
  uint16_t pid;
  MayaUsbDeviceId() : vid(0), pid(0) {}
  MayaUsbDeviceId(uint16_t v, uint16_t p) : vid(v), pid(p) {}
  static std::vector<MayaUsbDeviceId> getAoapIds() {
    return {
      MayaUsbDeviceId(0x18D1, 0x2D00), // accessory
      MayaUsbDeviceId(0x18D1, 0x2D01), // accessory + ADB
    };
  }
};

class MayaUsbDevice {
  static constexpr size_t RGB_IMAGE_SIZE = 2048 * 2048 * 16; // about 64 MB
  static constexpr size_t BUFFER_LEN     = 16384;

  TSharedPtr<LibraryInitParams> _initParams;

  libusb_device_handle* _hnd;

  MayaUsbDeviceId _id;
  std::string _manufacturer;
  std::string _product;
  uint8_t _inEndpoint;
  uint8_t _outEndpoint;

  std::atomic_bool _handshake;

  std::shared_ptr<InterruptibleThread> _receiveWorker;

  std::shared_ptr<InterruptibleThread> _sendWorker;
  bool _sendReady; /* Note: doesn't have to be atomic because we lock. */
  std::mutex _sendMutex;
  std::condition_variable _sendCv;

  unsigned char* _rgbImageBuffer;
  unsigned char* _jpegBuffer;
  size_t _jpegBufferSize;
  size_t _jpegBufferWidth;
  size_t _jpegBufferWidthPitch;
  size_t _jpegBufferHeight;

  int getControlInt16(int16_t* out, uint8_t request);
  int sendControl(uint8_t request);
  int sendControlString(uint8_t request, uint16_t index, std::string str);

  void flushInputBuffer(unsigned char* buf);

  MayaUsbDevice(TSharedPtr<LibraryInitParams>& initParams,
    MayaUsbDeviceId id,
    libusb_device_handle* handle,
    std::string manufacturer,
    std::string product,
    uint8_t inEndpoint,
    uint8_t outEndpoint);

public:
  static constexpr int STATUS_OK = 0;
  static constexpr int STATUS_NOT_FOUND_ERROR = -1;
  static constexpr int STATUS_DEVICE_DESCRIPTOR_ERROR = -2;
  static constexpr int STATUS_CONFIG_DESCRIPTOR_ERROR = -3;
  static constexpr int STATUS_DESCRIPTOR_READ_ERROR = -4;
  static constexpr int STATUS_INTERFACE_CLAIM_ERROR = -5;
  static constexpr int STATUS_RECEIVE_ERROR = -6;
  static constexpr int STATUS_SEND_ERROR = -7;
  static constexpr int STATUS_BAD_PROTOCOL_VERSION = -8;
  static constexpr int STATUS_LIBUSB_ERROR = -1000;
  static constexpr int STATUS_JPEG_ERROR = -2000;

  static int create(TSharedPtr<MayaUsbDevice>* out,
    TSharedPtr<LibraryInitParams>& initParams,
    uint16_t vid, uint16_t pid);
  static int create(TSharedPtr<MayaUsbDevice>* out,
    TSharedPtr<LibraryInitParams>& initParams,
    std::vector<MayaUsbDeviceId> ids = MayaUsbDeviceId::getAoapIds());
  ~MayaUsbDevice();
  std::string getDescription();
  int convertToAccessory();
  bool waitHandshakeAsync(std::function<void(bool)> callback);
  bool isHandshakeComplete();
  bool beginReadLoop(std::function<void(const unsigned char*, int)> callback,
      size_t readFrame);
  bool beginSendLoop(std::function<void(int)> failureCallback);
  bool isSending();
  bool sendImage(ID3D11Texture2D* source);
  static bool supportsRasterFormat(DXGI_FORMAT format);
};
