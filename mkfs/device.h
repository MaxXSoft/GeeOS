#ifndef GEEOS_MKFS_DEVICE_H_
#define GEEOS_MKFS_DEVICE_H_

#include <vector>
#include <utility>
#include <cstddef>
#include <cstdint>

class DeviceBase {
 public:
  virtual ~DeviceBase() = default;

  virtual std::int32_t Read(std::uint8_t *begin, std::uint8_t *end,
                            std::size_t offset) = 0;
  virtual std::int32_t Write(const std::uint8_t *begin,
                            const std::uint8_t *end,
                            std::size_t offset) = 0;
  virtual bool Sync() = 0;

  template <typename T>
  std::int32_t Read(T &object, std::size_t offset) {
    auto begin = reinterpret_cast<std::uint8_t *>(&object);
    auto end = reinterpret_cast<std::uint8_t *>(&object + 1);
    return Read(begin, end, offset);
  }

  std::int32_t Read(std::vector<std::uint8_t> &buffer,
                    std::size_t offset) {
    auto begin = buffer.data(), end = buffer.data() + buffer.size();
    return Read(begin, end, offset);
  }

  template <typename T>
  std::int32_t Write(const T &object, std::size_t offset) {
    auto begin = reinterpret_cast<const std::uint8_t *>(&object);
    auto end = reinterpret_cast<const std::uint8_t *>(&object + 1);
    return Write(begin, end, offset);
  }

  std::int32_t Write(const std::vector<std::uint8_t> &buffer,
                     std::size_t offset) {
    auto begin = buffer.data(), end = buffer.data() + buffer.size();
    return Write(begin, end, offset);
  }

  template <typename... Args>
  bool ReadAssert(std::int32_t read_count, Args &&... args) {
    return Read(std::forward(args)...) == read_count;
  }

  template <typename... Args>
  bool WriteAssert(std::int32_t write_count, Args &&... args) {
    return Write(std::forward(args)...) == write_count;
  }
};

using Device = DeviceBase;

#endif  // GEEOS_MKFS_DEVICE_H_
