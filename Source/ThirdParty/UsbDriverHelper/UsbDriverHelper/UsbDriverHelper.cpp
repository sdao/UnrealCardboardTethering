// UsbDriverHelper.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


std::string getInfName(const wdi_device_info* device) {
  std::string name(device->desc);
  std::ostringstream os;

  for (int i = 0; i < name.size(); ++i) {
    char c = name[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
      os << c;
    } else {
      os << "_";
    }
  }

  os << "-" << device->vid << "-" << device->pid << ".inf";
  return os.str();
}

int main(int argc, char** argv) {
  if (argc < 3) {
    return -1;
  }

  std::string vidStr(argv[1]);
  std::string pidStr(argv[2]);
  std::string miStr(argv[3]);

  uint16_t vid = std::stoi(vidStr);
  uint16_t pid = std::stoi(pidStr);
  int8_t mi = std::stoi(miStr);

  wdi_options_create_list opts;
  opts.list_all = true;
  opts.list_hubs = false;
  opts.trim_whitespaces = true;

  wdi_device_info *device, *list;
  if (wdi_create_list(&list, &opts) == WDI_SUCCESS) {
    for (device = list; device != nullptr; device = device->next) {
      if (device->vid == vid && device->pid == pid &&
          device->is_composite ? device->mi == mi : -1 == mi) {
        std::string infName = getInfName(device);

        wdi_options_prepare_driver prepareOpts;
        prepareOpts.cert_subject = nullptr;
        prepareOpts.device_guid = nullptr;
        prepareOpts.disable_cat = false;
        prepareOpts.disable_signing = false;
        prepareOpts.driver_type = WDI_WINUSB;
        prepareOpts.use_wcid_driver = false;
        prepareOpts.vendor_name = nullptr;

        int status = wdi_prepare_driver(device, nullptr, infName.c_str(), &prepareOpts);
        if (status != WDI_SUCCESS) {
          return -2;
        }
        
        status = wdi_install_driver(device, nullptr, infName.c_str(), nullptr);
        if (status != WDI_SUCCESS) {
          return -3;
        }

        return 0;
      }
    }
    wdi_destroy_list(list);
  }

  return -4;
}
