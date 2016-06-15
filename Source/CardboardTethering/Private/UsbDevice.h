#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sstream>
#include <iostream>
#include <iomanip>
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

struct UsbDeviceId {
  uint16_t vid;
  uint16_t pid;
  UsbDeviceId() : vid(0), pid(0) {}
  UsbDeviceId(uint16_t v, uint16_t p) : vid(v), pid(p) {}
  bool operator==(const UsbDeviceId& other) const {
    return vid == other.vid && pid == other.pid;
  }
  bool isAoapId() const {
    for (UsbDeviceId id : getAoapIds()) {
      if (id == *this) {
        return true;
      }
    }
    return false;
  }
  std::string toString() const {
    std::ostringstream os;
    os << std::setfill('0') << std::hex
      << std::setw(4) << vid << ":"
      << std::setw(4) << pid
      << std::dec << std::setfill(' ');
    return os.str();
  }
  static std::vector<UsbDeviceId> getAoapIds() {
    return {
      UsbDeviceId(0x18D1, 0x2D00), // accessory
      UsbDeviceId(0x18D1, 0x2D01), // accessory + ADB
    };
  }
};

struct UsbDeviceDesc {
  UsbDeviceId id;
  std::string manufacturer;
  std::string product;
  UsbDeviceDesc(UsbDeviceId i, std::string m, std::string p) : id(i), manufacturer(m), product(p) {}
  bool isAoapDesc() {
    return id.isAoapId();
  }
};

class UsbDevice {
  static constexpr size_t RGB_IMAGE_SIZE = 2048 * 2048 * 16; // about 64 MB
  static constexpr size_t BUFFER_LEN     = 16384;

  TSharedPtr<LibraryInitParams> _initParams;

  libusb_device_handle* _hnd;

  UsbDeviceDesc _desc;
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

  std::mutex _paramsMutex;
  int32_t _width;
  int32_t _height;
  float _interpupillary;

  int getControlInt16(int16_t* out, uint8_t request);
  int sendControl(uint8_t request);
  int sendControlString(uint8_t request, uint16_t index, std::string str);

  void flushInputBuffer(unsigned char* buf);

  UsbDevice(TSharedPtr<LibraryInitParams>& initParams,
    UsbDeviceDesc desc,
    libusb_device_handle* handle,
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

  static constexpr unsigned char TAG_HEADER = 0x27;
  static constexpr unsigned char TAG_WIDTH = 0x28;
  static constexpr unsigned char TAG_HEIGHT = 0x29;
  static constexpr unsigned char TAG_INTERPUPILLARY = 0x2A;
  static constexpr unsigned char TAG_FILL = 0x30;

  static int create(TSharedPtr<UsbDevice>* out,
    TSharedPtr<LibraryInitParams>& initParams,
    uint16_t vid, uint16_t pid);
  static int create(TSharedPtr<UsbDevice>* out,
    TSharedPtr<LibraryInitParams>& initParams,
    std::vector<UsbDeviceId> ids = UsbDeviceId::getAoapIds());
  static std::vector<UsbDeviceDesc> getConnectedDeviceDescriptions(
    TSharedPtr<LibraryInitParams>& initParams);
  ~UsbDevice();
  std::string getDescription();
  int convertToAccessory();
  bool waitHandshakeAsync(std::function<void(bool)> callback);
  bool isHandshakeComplete();
  bool beginReadLoop(std::function<void(const unsigned char*, int)> callback,
      size_t readFrame);
  bool beginSendLoop(std::function<void(int)> failureCallback);
  bool isSending();
  bool sendImage(ID3D11Texture2D* source);
  void getViewerParams(int32_t* width, int32_t* height, float* interpupillary);
  static bool supportsRasterFormat(DXGI_FORMAT format);
};
