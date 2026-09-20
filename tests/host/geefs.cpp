#include "geefs.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}

class MemoryDevice : public Device {
 public:
  std::vector<std::uint8_t> bytes;
  std::size_t reads = 0;

  std::int32_t Read(std::uint8_t *buf, std::size_t len,
                    std::size_t offset) override {
    ++reads;
    if (offset > bytes.size() || len > bytes.size() - offset) return -1;
    std::memcpy(buf, bytes.data() + offset, len);
    return len;
  }

  std::int32_t Write(const std::uint8_t *buf, std::size_t len,
                     std::size_t offset) override {
    if (offset > bytes.size() || len > bytes.size() - offset) return -1;
    std::memcpy(bytes.data() + offset, buf, len);
    return len;
  }

  bool Sync() override { return true; }
  bool Resize(std::size_t size) override {
    bytes.resize(size);
    return true;
  }
};

std::string Pattern(std::size_t size) {
  std::string data(size, '\0');
  for (std::size_t i = 0; i < size; ++i) {
    data[i] = (i * 37 + i / 256) % 251;
  }
  return data;
}

void CheckRead(GeeFS &fs, const std::string &expected,
               std::size_t offset, std::size_t length) {
  std::ostringstream output;
  auto count = fs.Read("file", output, offset, length);
  auto want = expected.substr(offset, length);
  Check(count == want.size(), "read length mismatch");
  Check(output.str() == want, "read data mismatch");
}

void IndirectBlocks() {
  MemoryDevice device;
  GeeFS fs(device);
  Check(fs.Create(256, 1, 2), "create image failed");
  Check(fs.CreateFile("file"), "create file failed");
  // Reach three second-level tables, including an incompletely filled last one.
  const auto data = Pattern((12 + 64 + 2 * 64) * 256 + 17);
  std::istringstream input(data);
  Check(fs.Write("file", input, 0, data.size()) == data.size(),
        "large write failed");
  std::uint32_t magic;
  std::memcpy(&magic, device.bytes.data(), sizeof(magic));
  Check(magic == kMagicNum, "large write corrupted superblock");
  CheckRead(fs, data, 0, data.size());
  for (auto block : {12, 12 + 64, 12 + 64 + 64, 12 + 64 + 128}) {
    CheckRead(fs, data, block * 256 - 1, 3);
  }
  GeeFS reopened(device);
  Check(reopened.Open(), "reopen image failed");
  CheckRead(reopened, data, 0, data.size());
}

void FailedCreation() {
  MemoryDevice device;
  GeeFS fs(device);
  Check(fs.Create(256, 1, 2), "create image failed");
  Check(fs.CreateFile("file"), "create file failed");
  for (int i = 0; i < 8; ++i) {
    Check(!fs.CreateFile("file"), "duplicate file accepted");
    Check(!fs.MakeDir("file"), "conflicting directory accepted");
    Check(!fs.CreateFile(std::string(28, 'x')), "long filename accepted");
    Check(!fs.MakeDir(std::string(28, 'x')), "long directory name accepted");
  }
  Check(fs.MakeDir("dir"), "failed creates leaked inode or data blocks");
  Check(fs.ChangeDir("dir"), "new directory is inaccessible");
  Check(fs.CreateFile("nested"), "nested file creation failed");
  Check(fs.ChangeDir(".."), "parent directory is inaccessible");
  Check(fs.CreateFile("remaining1"), "failed creates leaked inodes");
  Check(fs.CreateFile("remaining2"), "last free inode was lost");
  Check(!fs.CreateFile("full"), "inode exhaustion was not reported");
}

void DirectoryExhaustion() {
  MemoryDevice device;
  GeeFS fs(device);
  Check(fs.Create(256, 1, 2), "create image failed");
  Check(fs.CreateFile("file"), "create file failed");
  // The root uses one of 2016 blocks. This file uses 1983 data blocks and
  // 32 index blocks, leaving no space for a new directory's first block.
  const auto data = Pattern(1983 * 256);
  std::istringstream input(data);
  Check(fs.Write("file", input, 0, data.size()) == data.size(),
        "filling image failed");
  for (int i = 0; i < 8; ++i) {
    Check(!fs.MakeDir("dir"), "directory creation on full image succeeded");
    Check(!fs.ChangeDir("dir"), "failed directory creation left an entry");
  }
  Check(fs.CreateFile("dir"), "failed directory creation leaked its name or inode");
  CheckRead(fs, data, 0, data.size());
}

}  // namespace

int main() {
  try {
    IndirectBlocks();
    FailedCreation();
    DirectoryExhaustion();
    std::cout << "PASS: GeeFS host regressions\n";
  }
  catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
}
