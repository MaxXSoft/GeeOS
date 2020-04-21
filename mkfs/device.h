#ifndef GEEOS_MKFS_DEVICE_H_
#define GEEOS_MKFS_DEVICE_H_

#include <vector>
#include <utility>
#include <cstddef>
#include <cstdint>

class DeviceBase {
 public:
  virtual ~DeviceBase() = default;

  virtual std::int32_t Read(std::uint8_t *buf, std::size_t len,
                            std::size_t offset) = 0;
  virtual std::int32_t Write(const std::uint8_t *buf, std::size_t len,
                             std::size_t offset) = 0;
  virtual bool Sync() = 0;
  virtual bool Resize(std::size_t size) = 0;

  template <typename T>
  std::int32_t Read(T &object, std::size_t offset) {
    auto buf = reinterpret_cast<std::uint8_t *>(&object);
    return Read(buf, sizeof(T), offset);
  }

  std::int32_t Read(std::vector<std::uint8_t> &buffer,
                    std::size_t offset) {
    return Read(buffer.data(), buffer.size(), offset);
  }

  template <typename T>
  std::int32_t Write(const T &object, std::size_t offset) {
    auto buf = reinterpret_cast<const std::uint8_t *>(&object);
    return Write(buf, sizeof(T), offset);
  }

  std::int32_t Write(const std::vector<std::uint8_t> &buffer,
                     std::size_t offset) {
    return Write(buffer.data(), buffer.size(), offset);
  }

  template <typename... Args>
  bool ReadAssert(std::int32_t read_count, Args &&... args) {
    return Read(args...) == read_count;
  }

  template <typename... Args>
  bool WriteAssert(std::int32_t write_count, Args &&... args) {
    return Write(args...) == write_count;
  }
};

using Device = DeviceBase;

#endif  // GEEOS_MKFS_DEVICE_H_
