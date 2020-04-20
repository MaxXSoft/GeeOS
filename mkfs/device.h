#ifndef GEEOS_MKFS_DEVICE_H_
#define GEEOS_MKFS_DEVICE_H_

#include <vector>
#include <cstddef>
#include <cstdint>

class DeviceInterface {
 public:
  virtual ~DeviceInterface() = default;

  virtual std::int32_t Read(std::vector<std::uint8_t> &buffer,
                            std::size_t offset) = 0;
  virtual std::int32_t Write(const std::vector<std::uint8_t> &buffer,
                             std::size_t offset) = 0;
  virtual bool Sync() = 0;
};

#endif  // GEEOS_MKFS_DEVICE_H_
