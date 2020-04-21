#include "geefs.h"

#include <vector>
#include <algorithm>
#include <cstring>
#include <cassert>

namespace {

//

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
                      sizeof(INode)) + j;
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
    assert(n < kOfsPerBlock * kOfsPerBlock);
    auto offset = inode.indirect2 * super_block_.block_size;
    offset += (n / kOfsPerBlock) * kBlockOfsSize;
    if (!dev_.ReadAssert(kBlockOfsSize, offset, offset)) return {};
    offset *= super_block_.block_size;
    offset += (n % kOfsPerBlock) * kBlockOfsSize;
    if (!dev_.ReadAssert(kBlockOfsSize, offset, offset)) return {};
    return offset;
  }
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
  UpdateINode(cwd_, *inode_id);
  InitDirBlock(*blk_ofs, *inode_id, *inode_id);
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
  return cwd_.type == INodeType::Dir;
}

bool GeeFS::Sync() {
  return dev_.Sync();
}

void GeeFS::List(std::ostream &os) {
  assert(cwd_.type == INodeType::Dir);
  const auto kEntNum = cwd_.size / sizeof(Entry);
  const auto kEntPerBlock = super_block_.block_size / sizeof(Entry);
  // traverse data blocks
  for (int i = 0; i < cwd_.block_num; ++i) {
    // get offset
    auto blk_ofs = GetBlockOffset(cwd_, i);
    assert(blk_ofs);
    auto offset = *blk_ofs * super_block_.block_size;
    // traverse entries in current block
    auto entry_num = std::min(kEntNum - i * kEntPerBlock, kEntPerBlock);
    for (int j = 0; j < entry_num; ++j) {
      // read entry
      auto ent_ofs = offset + j * sizeof(Entry);
      Entry entry;
      auto ret = dev_.ReadAssert(sizeof(Entry), entry, ent_ofs);
      assert(ret);
      // print to stream
      os << entry.filename << std::endl;
    }
  }
}

bool GeeFS::CreateFile(std::string_view file_name) {
  // TODO
  return false;
}

bool GeeFS::MakeDir(std::string_view dir_name) {
  // TODO
  return false;
}

bool GeeFS::ChangeDir(std::string_view dir_name) {
  // TODO
  return false;
}

bool GeeFS::Remove(std::string_view file_name) {
  // TODO
  return false;
}

std::int32_t GeeFS::Read(std::string_view file_name, std::ostream &os,
                         std::size_t offset, std::size_t len) {
  // TODO
  return false;
}

std::int32_t GeeFS::Write(std::string_view file_name, std::istream &is,
                          std::size_t offset, std::size_t len) {
  // TODO
  return false;
}
