#ifndef GEEOS_MKFS_STRUCTS_H_
#define GEEOS_MKFS_STRUCTS_H_

#include <cstdint>

constexpr auto kMagicNum        = 0x9eef5000;
constexpr auto kDirectBlockNum  = 12;
constexpr auto kBlockOfsSize    = sizeof(std::uint32_t);
constexpr auto kFileNameMaxLen  = 28;

enum class INodeType : std::uint32_t {
  Unused = 0,
  File = 1,
  Dir = 2,
};

struct SuperBlockHeader {
  std::uint32_t magic_num;                  // magic number
  std::uint32_t header_size;                // size of current header
  std::uint32_t block_size;                 // size of block
  std::uint32_t free_map_num;               // number of free map blocks
  std::uint32_t inode_blk_num;              // number of inode blocks
};

struct FreeMapBlockHeader {
  std::uint32_t unused_num;                 // number of unused blocks
};

struct INodeBlockHeader {
  std::uint32_t unused_num;                 // number of unused inodes
};

struct INode {
  INodeType     type;                       // type of inode
  std::uint32_t size;                       // size of file
  std::uint32_t block_num;                  // number of blocks
  std::uint32_t direct[kDirectBlockNum];    // direct blocks
  std::uint32_t indirect;                   // indirect block id
  std::uint32_t indirect2;                  // 2nd indirect block id
};

struct Entry {
  std::uint32_t inode_id;                   // inode id of file
  std::uint8_t  filename[kFileNameMaxLen];  // file name, ends with '\0'
};

#endif  // GEEOS_MKFS_STRUCTS_H_
