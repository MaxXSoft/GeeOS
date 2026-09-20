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

std::uint32_t FreeBlocks(const MemoryDevice &device) {
  FreeMapBlockHeader header;
  std::memcpy(&header, device.bytes.data() + 256, sizeof(header));
  return header.unused_num;
}

void SparseWrites() {
  for (auto offset : {1, 255, 256, 257, 512, 12 * 256, 76 * 256, 140 * 256}) {
    MemoryDevice device;
    GeeFS fs(device);
    Check(fs.Create(256, 1, 2), "create image failed");
    Check(fs.CreateFile("file"), "create file failed");
    std::istringstream input("Z");
    Check(fs.Write("file", input, offset, 1) == 1, "sparse write failed");
    auto expected = std::string(offset, '\0') + "Z";
    CheckRead(fs, expected, 0, expected.size());
    GeeFS reopened(device);
    Check(reopened.Open(), "reopen image failed");
    CheckRead(reopened, expected, 0, expected.size());
  }
  MemoryDevice device;
  GeeFS fs(device);
  Check(fs.Create(256, 1, 2), "create image failed");
  Check(fs.CreateFile("file"), "create file failed");
  std::istringstream first("abc"), second("Z"), empty;
  Check(fs.Write("file", first, 0, 3) == 3, "initial write failed");
  Check(fs.Write("file", second, 513, 1) == 1, "nonempty sparse write failed");
  const auto expected = "abc" + std::string(510, '\0') + "Z";
  CheckRead(fs, expected, 0, expected.size());
  const auto before = device.bytes;
  Check(fs.Write("file", empty, 1024, 0) == 0, "zero-length write failed");
  Check(device.bytes == before, "zero-length write changed file");
}

void IndexExhaustion() {
  // Leave too few blocks for the data block and a new indirect table.
  for (auto block_count : {12, 76, 140}) {
    MemoryDevice device;
    GeeFS fs(device);
    Check(fs.Create(256, 1, 2), "create image failed");
    Check(fs.CreateFile("file"), "create file failed");
    const auto data = Pattern(block_count * 256);
    std::istringstream input(data);
    Check(fs.Write("file", input, 0, data.size()) == data.size(),
          "initial boundary write failed");
    Check(fs.CreateFile("filler"), "create filler failed");
    const auto spare = block_count == 76 ? 2 : 1;
    auto blocks = FreeBlocks(device) - spare;
    std::size_t fill_blocks = 0;
    for (std::size_t n = 1; n <= blocks; ++n) {
      auto used = n + (n > 12) + (n > 76 ? 1 + (n - 76 + 63) / 64 : 0);
      if (used <= blocks) fill_blocks = n;
    }
    std::istringstream filler(std::string(fill_blocks * 256, 'F'));
    Check(fs.Write("filler", filler, 0, fill_blocks * 256) == fill_blocks * 256,
          "filler write failed");
    Check(FreeBlocks(device) == spare, "incorrect exhaustion fixture");
    for (int attempt = 0; attempt < 3; ++attempt) {
      std::istringstream extra("X");
      Check(fs.Write("file", extra, data.size(), 1) == 0,
            "index exhaustion did not return a short write");
      Check(FreeBlocks(device) == spare, "failed append leaked blocks");
      CheckRead(fs, data, 0, data.size() + 1);
    }
    std::istringstream distant("Y");
    Check(fs.Write("file", distant, data.size() + 4096, 1) == -1,
          "impossible sparse write was not rejected");
    Check(FreeBlocks(device) == spare, "failed sparse write leaked blocks");
    CheckRead(fs, data, 0, data.size() + 1);
    // The released block must still be usable by an ordinary direct block.
    Check(fs.CreateFile("small"), "create after index exhaustion failed");
    std::istringstream small("S");
    Check(fs.Write("small", small, 0, 1) == 1, "released data block was lost");
  }
}

void ShortInput() {
  MemoryDevice device;
  GeeFS fs(device);
  Check(fs.Create(256, 1, 2), "create image failed");
  Check(fs.CreateFile("file"), "create file failed");
  std::istringstream input("abc");
  Check(fs.Write("file", input, 0, 512) == 3, "short input was not reported");
  CheckRead(fs, "abc", 0, 512);
  const auto before = device.bytes;
  std::istringstream empty;
  Check(fs.Write("file", empty, 512, 1) == 0, "empty input was not reported");
  Check(device.bytes == before, "empty input allocated or extended file");
}

void PartialWrites() {
  for (auto block_size : {128, 256}) {
    MemoryDevice device;
    GeeFS fs(device);
    // 128-byte blocks hit the format's maximum file size; 256-byte blocks
    // exhaust the image first. Both must preserve the successfully written prefix.
    Check(fs.Create(block_size, block_size == 128 ? 2 : 1, 2),
          "create image failed");
    Check(fs.CreateFile("file"), "create file failed");
    const auto length = block_size == 128 ? (12 + 32 + 32 * 32) * 128 : 1983 * 256;
    const auto data = Pattern(length + 1024);
    std::istringstream input(data);
    Check(fs.Write("file", input, 0, data.size()) == length,
          "partial write length was lost");
    GeeFS reopened(device);
    Check(reopened.Open(), "reopen image failed");
    CheckRead(reopened, data.substr(0, length), 0, data.size());
    const auto before = device.bytes;
    std::istringstream extra("X");
    Check(fs.Write("file", extra, length, 1) <= 0, "full file kept growing");
    Check(device.bytes == before, "failed extension changed image");
  }
}

}  // namespace

int main() {
  try {
    IndirectBlocks();
    FailedCreation();
    DirectoryExhaustion();
    SparseWrites();
    IndexExhaustion();
    ShortInput();
    PartialWrites();
    std::cout << "PASS: GeeFS host regressions\n";
  }
  catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
}
