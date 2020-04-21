#include "iosdev.h"

#include <algorithm>

std::int32_t IOStreamDevice::Read(std::uint8_t *buf, std::size_t len,
                                  std::size_t offset) {
  if (offset >= size_) return -1;
  ios_.seekg(offset);
  auto size = std::min<std::size_t>(size_ - offset, len);
  ios_.read(reinterpret_cast<char *>(buf), size);
  return !ios_ ? -1 : size;
}

std::int32_t IOStreamDevice::Write(const std::uint8_t *buf,
                                   std::size_t len,
                                   std::size_t offset) {
  if (offset >= size_) return -1;
  ios_.seekp(offset);
  auto size = std::min<std::size_t>(size_ - offset, len);
  ios_.write(reinterpret_cast<const char *>(buf), size);
  return !ios_ ? -1 : size;
}

bool IOStreamDevice::Sync() {
  ios_.flush();
  return !!ios_;
}

bool IOStreamDevice::Resize(std::size_t size) {
  ios_.seekp(size - 1);
  ios_ << '\0';
  size_ = size;
  return !!ios_;
}
