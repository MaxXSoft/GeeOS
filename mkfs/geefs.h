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

  // create an empty GeeFS image on device
  bool Create(std::uint32_t block_size, std::uint32_t free_map_num,
              std::uint32_t inode_blk_num);
  // open GeeFS image on device
  bool Open();
  // sync all modifications to device
  bool Sync();

  // list all files/dirs in cwd
  void List(std::ostream &os);
  // create new file in cwd
  bool CreateFile(std::string_view file_name);
  // create new directory in cwd
  bool MakeDir(std::string_view dir_name);
  // change cwd
  bool ChangeDir(std::string_view dir_name);
  // remove file in cwd
  bool Remove(std::string_view file_name);
  // read file in cwd to output stream
  std::int32_t Read(std::string_view file_name, std::ostream &os,
                    std::size_t offset, std::size_t len);
  // write input stream to file in cwd
  std::int32_t Write(std::string_view file_name, std::istream &is,
                     std::size_t offset, std::size_t len);

 private:
  // allocate a data block, returns block offset
  std::optional<std::uint32_t> AllocDataBlock();
  // allocate an inode, returns inode id
  std::optional<std::uint32_t> AllocINode();
  // initialize data block of directory
  void InitDirBlock(std::uint32_t blk_ofs, std::uint32_t cur_id,
                    std::uint32_t parent_id);
  // update inode by id
  void UpdateINode(const INode &inode, std::uint32_t id);
  // read inode by id
  bool ReadINode(INode &inode, std::uint32_t id);
  // get nth data block offset of inode
  std::optional<std::uint32_t> GetBlockOffset(const INode &inode,
                                              std::size_t n);

  // low-level device
  Device &dev_;
  // super block of disk
  SuperBlockHeader super_block_;
  // current working directory
  INode cwd_;
};

#endif  // GEEOS_MKFS_GEEFS_H_
