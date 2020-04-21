#ifndef GEEOS_MKFS_IOSDEV_H_
#define GEEOS_MKFS_IOSDEV_H_

#include <iostream>

#include "device.h"

class IOStreamDevice : public DeviceBase {
 public:
  IOStreamDevice(std::iostream &ios) : ios_(ios) {
    auto pos = ios_.tellg();
    ios_.seekg(0, std::ios::end);
    size_ = ios_.tellg() - pos;
  }

  std::int32_t Read(std::uint8_t *buf, std::size_t len,
                    std::size_t offset) override;
  std::int32_t Write(const std::uint8_t *buf, std::size_t len,
                     std::size_t offset) override;
  bool Sync() override;
  bool Resize(std::size_t size) override;

 private:
  std::iostream &ios_;
  std::size_t size_;
};

#endif  // GEEOS_MKFS_IOSDEV_H_
