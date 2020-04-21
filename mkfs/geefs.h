#ifndef GEEOS_MKFS_GEEFS_H_
#define GEEOS_MKFS_GEEFS_H_

#include <istream>
#include <ostream>
#include <string_view>
#include <optional>
#include <cstddef>

#include "device.h"
#include "structs.h"

class GeeFS {
 public:
  GeeFS(Device &dev) : dev_(dev) {}
  ~GeeFS() { Sync(); }

  bool Create(std::uint32_t block_size, std::uint32_t free_map_num,
              std::uint32_t inode_blk_num);
  bool Open();
  bool Sync();

  void List(std::ostream &os);
  bool Create(std::string_view file_name);
  bool MakeDir(std::string_view dir_name);
  bool ChangeDir(std::string_view dir_name);
  bool Remove(std::string_view file_name);
  std::int32_t Read(std::string_view file_name, std::ostream &os,
                    std::size_t offset, std::size_t len);
  std::int32_t Write(std::string_view file_name, std::istream &is,
                     std::size_t offset, std::size_t len);

 private:
  // allocate a data block, returns block offset
  std::optional<std::uint32_t> AllocDataBlock();
  // allocate an inode, returns inode id
  std::optional<std::uint32_t> AllocINode();
  // initialize data block of directory
  void InitDirBlock(std::uint32_t blk_ofs, std::uint32_t parent_inode);
  // update inode by id
  void UpdateINode(const INode &inode, std::uint32_t id);

  // low-level device
  Device &dev_;
  // super block of disk
  SuperBlockHeader super_block_;
  // current working directory
  INode cwd_;
};

#endif  // GEEOS_MKFS_GEEFS_H_
