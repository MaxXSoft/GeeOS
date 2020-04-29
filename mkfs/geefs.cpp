#include "geefs.h"

#include <vector>
#include <algorithm>
#include <string>
#include <iomanip>
#include <cstring>
#include <cassert>

namespace {

const char *kTypeStr[] = {"unused", "file", "dir"};

void PrintFileSize(std::ostream &os, std::size_t size) {
  if (size < 1024) {
    os << size << 'B';
  }
  else {
    os << std::fixed << std::setprecision(1);
    if (size < 1024 * 1024) {
      os << size / 1024.0 << 'K';
    }
    else if (size < 1024 * 1024 * 1024) {
      os << size / 1024.0 / 1024.0 << 'M';
    }
    else {
      os << size / 1024.0 / 1024.0 / 1024.0 << 'G';
    }
  }
}

}  // namespace

std::optional<std::uint32_t> GeeFS::AllocDataBlock() {
  // traverse all free maps
  for (int i = 0; i < super_block_.free_map_num; ++i) {
    // read free map header
    auto offset = super_block_.block_size * (1 + i);
    FreeMapBlockHeader hdr;
    auto ret = dev_.ReadAssert(sizeof(hdr), hdr, offset);
    assert(ret);
    // check for free blocks
    if (hdr.unused_num) {
      // update header
      --hdr.unused_num;
      ret = dev_.WriteAssert(sizeof(hdr), hdr, offset);
      assert(ret);
      // read free map to buffer
      std::vector<std::uint8_t> buf;
      buf.resize(super_block_.block_size - sizeof(hdr));
      ret = dev_.ReadAssert(buf.size(), buf, offset + sizeof(hdr));
      assert(ret);
      // find next free bit
      for (int j = 0; j < buf.size(); ++j) {
        for (int k = 7; k >= 0; --k) {
          if (!(buf[j] & (1 << k))) {
            // set free bit as allocated
            buf[j] |= (1 << k);
            // update free map
            auto byte_ofs = offset + sizeof(hdr) + j;
            auto ret = dev_.WriteAssert(1, buf[j], byte_ofs);
            static_cast<void>(ret);
            assert(ret);
            // return block offset
            auto blk_ofs = 1 + super_block_.free_map_num;
            blk_ofs += super_block_.inode_blk_num;
            blk_ofs += i * (super_block_.block_size - sizeof(hdr)) * 8;
            blk_ofs += j * 8 + (7 - k);
            return blk_ofs;
          }
        }
      }
      assert(false);
    }
  }
  return {};
}

std::optional<std::uint32_t> GeeFS::AllocINode() {
  // traverse all inode blocks
  for (int i = 0; i < super_block_.inode_blk_num; ++i) {
    // read inode block header
    auto offset = super_block_.block_size *
                  (1 + super_block_.free_map_num + i);
    INodeBlockHeader hdr;
    auto ret = dev_.ReadAssert(sizeof(hdr), hdr, offset);
    static_cast<void>(ret);
    assert(ret);
    // check for free inodes
    if (hdr.unused_num) {
      // update header
      --hdr.unused_num;
      auto ret = dev_.WriteAssert(sizeof(hdr), hdr, offset);
      assert(ret);
      // read inodes to buffer
      std::vector<std::uint8_t> buf;
      buf.resize(super_block_.block_size - sizeof(hdr));
      ret = dev_.ReadAssert(buf.size(), buf, offset + sizeof(hdr));
      assert(ret);
      // find next free inode
      for (int j = 0; j < buf.size(); j += sizeof(INode)) {
        auto inode = reinterpret_cast<INode *>(buf.data() + j);
        if (inode->type == INodeType::Unused) {
          // return inode id
          return i * ((super_block_.block_size - sizeof(hdr)) /
                      sizeof(INode)) + (j / sizeof(INode));
        }
      }
      assert(false);
    }
  }
  return {};
}

void GeeFS::InitDirBlock(std::uint32_t blk_ofs, std::uint32_t cur_id,
                         std::uint32_t parent_id) {
  Entry ent;
  auto offset = blk_ofs * super_block_.block_size;
  // write entry '.'
  ent.inode_id = cur_id;
  std::strcpy(reinterpret_cast<char *>(ent.filename), ".");
  auto ret = dev_.WriteAssert(sizeof(ent), ent, offset);
  assert(ret);
  // write entry '..'
  ent.inode_id = parent_id;
  std::strcpy(reinterpret_cast<char *>(ent.filename), "..");
  ret = dev_.WriteAssert(sizeof(ent), ent, offset + sizeof(ent));
  assert(ret);
}

void GeeFS::UpdateINode(const INode &inode, std::uint32_t id) {
  auto in_per_blk = (super_block_.block_size - sizeof(INodeBlockHeader)) /
                    sizeof(INode);
  auto offset = 1 + super_block_.free_map_num + id / in_per_blk;
  offset *= super_block_.block_size;
  offset += sizeof(INodeBlockHeader) + (id % in_per_blk) * sizeof(INode);
  auto ret = dev_.WriteAssert(sizeof(INode), inode, offset);
  static_cast<void>(ret);
  assert(ret);
}

bool GeeFS::ReadINode(INode &inode, std::uint32_t id) {
  auto in_per_blk = (super_block_.block_size - sizeof(INodeBlockHeader)) /
                    sizeof(INode);
  auto offset = 1 + super_block_.free_map_num + id / in_per_blk;
  offset *= super_block_.block_size;
  offset += sizeof(INodeBlockHeader) + (id % in_per_blk) * sizeof(INode);
  return dev_.ReadAssert(sizeof(INode), inode, offset);
}

std::optional<std::uint32_t> GeeFS::ReadINode(INode &inode,
                                              std::string_view name) {
  if (name.size() > kFileNameMaxLen - 1) return {};
  std::optional<std::uint32_t> id = {};
  WalkEntry([this, &inode, &name, &id](const Entry &entry) {
    if (name == reinterpret_cast<const char *>(entry.filename)) {
      if (ReadINode(inode, entry.inode_id)) id = entry.inode_id;
      return false;
    }
    return true;
  });
  return id;
}

std::optional<std::uint32_t> GeeFS::GetBlockOffset(const INode &inode,
                                                   std::size_t n) {
  const auto kOfsPerBlock = super_block_.block_size / kBlockOfsSize;
  if (n >= inode.block_num) return {};
  if (n < kDirectBlockNum) {
    return inode.direct[n];
  }
  else if (n - kDirectBlockNum < kOfsPerBlock) {
    auto offset = inode.indirect * super_block_.block_size;
    offset += (n - kDirectBlockNum) * kBlockOfsSize;
    if (!dev_.ReadAssert(kBlockOfsSize, offset, offset)) return {};
    return offset;
  }
  else {
    n -= kDirectBlockNum + kOfsPerBlock;
    if (n >= kOfsPerBlock * kOfsPerBlock) return {};
    auto offset = inode.indirect2 * super_block_.block_size;
    offset += (n / kOfsPerBlock) * kBlockOfsSize;
    if (!dev_.ReadAssert(kBlockOfsSize, offset, offset)) return {};
    offset *= super_block_.block_size;
    offset += (n % kOfsPerBlock) * kBlockOfsSize;
    if (!dev_.ReadAssert(kBlockOfsSize, offset, offset)) return {};
    return offset;
  }
}

bool GeeFS::AppendBlock(INode &inode, std::uint32_t blk_ofs) {
  const auto kOfsPerBlock = super_block_.block_size / kBlockOfsSize;
  auto n = inode.block_num++;
  if (n < kDirectBlockNum) {
    inode.direct[n] = blk_ofs;
    return true;
  }
  else if (n - kDirectBlockNum < kOfsPerBlock) {
    if (n == kDirectBlockNum) {
      // allocate indirect block
      auto blk_ofs = AllocDataBlock();
      if (!blk_ofs) return false;
      inode.indirect = *blk_ofs;
    }
    auto offset = inode.indirect * super_block_.block_size;
    offset += (n - kDirectBlockNum) * kBlockOfsSize;
    return dev_.WriteAssert(kBlockOfsSize, blk_ofs, offset);
  }
  else if (n - kDirectBlockNum - kOfsPerBlock <
           kOfsPerBlock * kOfsPerBlock) {
    n -= kDirectBlockNum + kOfsPerBlock;
    if (!n) {
      // allocate 2nd indirect block
      auto blk_ofs = AllocDataBlock();
      if (!blk_ofs) return false;
      inode.indirect2 = *blk_ofs;
    }
    auto offset = inode.indirect2 * super_block_.block_size;
    offset += (n / kOfsPerBlock) * kBlockOfsSize;
    if (n % kOfsPerBlock) {
      // initialize 2nd indirect block
      auto blk_ofs = AllocDataBlock();
      if (!blk_ofs) return false;
      if (!dev_.WriteAssert(kBlockOfsSize, *blk_ofs, offset)) return false;
      offset = *blk_ofs;
    }
    else {
      if (!dev_.ReadAssert(kBlockOfsSize, offset, offset)) return false;
    }
    offset *= super_block_.block_size;
    offset += (n % kOfsPerBlock) * kBlockOfsSize;
    return dev_.WriteAssert(kBlockOfsSize, blk_ofs, offset);
  }
  else {
    return false;
  }
}

bool GeeFS::WalkEntry(std::function<bool(const Entry &)> callback) {
  assert(cwd_.type == INodeType::Dir);
  const auto kEntNum = cwd_.size / sizeof(Entry);
  const auto kEntPerBlock = super_block_.block_size / sizeof(Entry);
  // traverse data blocks
  for (int i = 0; i < cwd_.block_num; ++i) {
    // get offset
    auto blk_ofs = GetBlockOffset(cwd_, i);
    if (!blk_ofs) return false;
    auto offset = *blk_ofs * super_block_.block_size;
    // traverse entries in current block
    auto entry_num = std::min(kEntNum - i * kEntPerBlock, kEntPerBlock);
    for (int j = 0; j < entry_num; ++j) {
      // read entry
      auto ent_ofs = offset + j * sizeof(Entry);
      Entry entry;
      if (!dev_.ReadAssert(sizeof(Entry), entry, ent_ofs)) return false;
      // invoke callback function
      if (!callback(entry)) return false;
    }
  }
  return true;
}

bool GeeFS::AddEntry(std::uint32_t inode_id, std::string_view file_name) {
  if (file_name.size() > kFileNameMaxLen - 1) return false;
  // check if conflicted
  auto ret = WalkEntry([&file_name](const Entry &entry) {
    return file_name != reinterpret_cast<const char *>(entry.filename);
  });
  if (!ret) return false;
  // get offset of entry that will be inserted
  auto blk_ofs = GetBlockOffset(cwd_, cwd_.block_num - 1);
  if (!blk_ofs) return false;
  auto offset = *blk_ofs * super_block_.block_size;
  auto ent_count = cwd_.size / sizeof(Entry);
  assert(ent_count != 0);
  auto ent_per_blk = super_block_.block_size / sizeof(Entry);
  auto inblk_ofs = (ent_count % ent_per_blk) * sizeof(Entry);
  if (inblk_ofs) {
    offset += inblk_ofs;
  }
  else {
    // allocate new block
    auto blk_ofs = AllocDataBlock();
    if (!blk_ofs || !AppendBlock(cwd_, *blk_ofs)) return false;
    offset = *blk_ofs * super_block_.block_size;
  }
  // insert entry
  Entry entry;
  entry.inode_id = inode_id;
  std::strcpy(reinterpret_cast<char *>(entry.filename),
              std::string(file_name).c_str());
  if (!dev_.WriteAssert(sizeof(Entry), entry, offset)) return false;
  // update inode of cwd
  cwd_.size += sizeof(Entry);
  UpdateINode(cwd_, cwd_id_);
  return true;
}

bool GeeFS::Create(std::uint32_t block_size, std::uint32_t free_map_num,
                   std::uint32_t inode_blk_num) {
  if (block_size < sizeof(SuperBlockHeader) ||
      block_size - sizeof(INodeBlockHeader) < sizeof(INode) ||
      block_size < 2 * sizeof(Entry)) {
    return false;
  }
  // resize device to image size
  auto blk_num = 1 + free_map_num + inode_blk_num;
  blk_num += (block_size - sizeof(FreeMapBlockHeader)) * 8 * free_map_num;
  if (!dev_.Resize(blk_num * block_size)) return false;
  // create buffer of empty block
  std::vector<std::uint8_t> empty_blk;
  empty_blk.resize(block_size, 0);
  // initialize super block
  super_block_ = {kMagicNum, sizeof(SuperBlockHeader), block_size,
                  free_map_num, inode_blk_num};
  if (!dev_.WriteAssert(sizeof(super_block_), super_block_, 0)) {
    return false;
  }
  // initialize free map
  for (int i = 0; i < free_map_num; ++i) {
    auto offset = block_size * (1 + i);
    if (!dev_.WriteAssert(block_size, empty_blk, offset)) return false;
    FreeMapBlockHeader hdr = {
      static_cast<std::uint32_t>((block_size - sizeof(hdr)) * 8)
    };
    if (!dev_.WriteAssert(sizeof(hdr), hdr, offset)) return false;
  }
  // initialize inode block
  for (int i = 0; i < inode_blk_num; ++i) {
    auto offset = block_size * (1 + free_map_num + i);
    if (!dev_.WriteAssert(block_size, empty_blk, offset)) return false;
    INodeBlockHeader hdr = {
      static_cast<std::uint32_t>((block_size - sizeof(hdr)) / sizeof(INode))
    };
    if (!dev_.WriteAssert(sizeof(hdr), hdr, offset)) return false;
  }
  // initialize data blocks
  auto data_blk_num = (block_size - sizeof(FreeMapBlockHeader)) * 8 *
                      free_map_num;
  for (int i = 0; i < data_blk_num; ++i) {
    auto offset = block_size * (1 + free_map_num + inode_blk_num + i);
    if (!dev_.WriteAssert(block_size, empty_blk, offset)) return false;
  }
  // initialize cwd as root directory
  auto blk_ofs = AllocDataBlock(), inode_id = AllocINode();
  assert(blk_ofs && inode_id);
  cwd_ = {INodeType::Dir, 2 * sizeof(Entry), 1};
  cwd_.direct[0] = *blk_ofs;
  cwd_id_ = *inode_id;
  UpdateINode(cwd_, cwd_id_);
  InitDirBlock(*blk_ofs, cwd_id_, cwd_id_);
  // reset current path
  cur_path_.clear();
  // sync
  return dev_.Sync();
}

bool GeeFS::Open() {
  // read super block header
  if (!dev_.ReadAssert(sizeof(super_block_), super_block_, 0)) {
    return false;
  }
  // set root directory as cwd
  if (!ReadINode(cwd_, 0)) return false;
  cwd_id_ = 0;
  // reset current path
  cur_path_.clear();
  return cwd_.type == INodeType::Dir;
}

bool GeeFS::Sync() {
  return dev_.Sync();
}

void GeeFS::List(std::ostream &os) {
  auto ret = WalkEntry([this, &os](const Entry &entry) {
    // get inode info
    INode inode;
    if (!ReadINode(inode, entry.inode_id)) return false;
    // print to stream
    os << std::left;
    os << std::setw(7) << kTypeStr[static_cast<int>(inode.type)] << ' ';
    os << std::setw(kFileNameMaxLen + 1) << entry.filename << ' ';
    PrintFileSize(os, inode.size);
    os << std::endl;
    return true;
  });
  static_cast<void>(ret);
  assert(ret);
}

bool GeeFS::CreateFile(std::string_view file_name) {
  // allocate new inode for file
  auto inode_id = AllocINode();
  if (!inode_id) return false;
  // create new entry
  if (!AddEntry(*inode_id, file_name)) return false;
  // update allocated inode
  INode inode = {INodeType::File};
  UpdateINode(inode, *inode_id);
  return true;
}

bool GeeFS::MakeDir(std::string_view dir_name) {
  // allocate new inode for directory
  auto inode_id = AllocINode();
  if (!inode_id) return false;
  // create new entry
  if (!AddEntry(*inode_id, dir_name)) return false;
  // allocate data block for directory
  auto blk_ofs = AllocDataBlock();
  if (!blk_ofs) return false;
  // update allocated inode
  INode inode = {INodeType::Dir, 2 * sizeof(Entry), 1, {*blk_ofs}};
  UpdateINode(inode, *inode_id);
  // initialize data block
  InitDirBlock(*blk_ofs, *inode_id, cwd_id_);
  return true;
}

bool GeeFS::ChangeDir(std::string_view dir_name) {
  // get inode by directory name
  INode inode;
  auto id = ReadINode(inode, dir_name);
  if (!id || inode.type != INodeType::Dir) return false;
  // change cwd
  cwd_ = inode;
  cwd_id_ = *id;
  // update current path
  if (dir_name != ".") {
    if (dir_name == ".." && !cur_path_.empty()) {
      cur_path_.pop_back();
    }
    else {
      cur_path_.push_back(std::string(dir_name));
    }
  }
  return true;
}

bool GeeFS::Remove(std::string_view file_name) {
  // TODO
  return false;
}

std::int32_t GeeFS::Read(std::string_view file_name, std::ostream &os,
                         std::size_t offset, std::size_t len) {
  // get inode
  INode inode;
  if (!ReadINode(inode, file_name)) return -1;
  // read file
  std::int32_t data_len = 0;
  auto end_len = std::min<std::size_t>(offset + len, inode.size);
  for (int i = offset; i < end_len; ++i, ++data_len) {
    // get block offset
    auto n = i / super_block_.block_size;
    if (n >= inode.block_num) break;
    auto blk_ofs = GetBlockOffset(inode, n);
    // get offset
    auto ofs = *blk_ofs * super_block_.block_size;
    ofs += i % super_block_.block_size;
    // read to stream
    std::uint8_t byte;
    if (!dev_.ReadAssert(1, byte, ofs)) break;
    os.write(reinterpret_cast<const char *>(&byte), 1);
  }
  return data_len;
}

std::int32_t GeeFS::Write(std::string_view file_name, std::istream &is,
                          std::size_t offset, std::size_t len) {
  // get inode
  INode inode;
  auto id = ReadINode(inode, file_name);
  if (!id) return -1;
  // expand file size if necessary
  if (offset > inode.size) {
    std::vector<std::uint8_t> buffer;
    buffer.resize(super_block_.block_size, 0);
    // allocate empty data blocks
    auto blk_num = (offset + (super_block_.block_size - 1)) /
                   super_block_.block_size;
    for (int i = inode.block_num + 1; i < blk_num; ++i) {
      auto blk_ofs = AllocDataBlock();
      if (!blk_ofs || !AppendBlock(inode, *blk_ofs) ||
          !dev_.WriteAssert(buffer.size(), buffer,
                            *blk_ofs * super_block_.block_size)) {
        return -1;
      }
    }
    // update file size
    inode.size = offset;
  }
  // write to file
  std::int32_t data_len = 0;
  for (int i = offset; i < offset + len; ++i, ++data_len) {
    // get block offset
    auto n = i / super_block_.block_size;
    std::optional<std::uint32_t> blk_ofs;
    if (n < inode.block_num) {
      blk_ofs = GetBlockOffset(inode, n);
      if (!blk_ofs) return -1;
    }
    else {
      // allocate a new data block
      blk_ofs = AllocDataBlock();
      if (!blk_ofs) break;
      if (!AppendBlock(inode, *blk_ofs)) return -1;
    }
    // write to block
    auto ofs = *blk_ofs * super_block_.block_size;
    ofs += i % super_block_.block_size;
    std::uint8_t byte;
    is.read(reinterpret_cast<char *>(&byte), 1);
    if (!dev_.WriteAssert(1, byte, ofs)) break;
  }
  // update inode
  if (offset + data_len > inode.size) inode.size = offset + data_len;
  UpdateINode(inode, *id);
  return data_len;
}
