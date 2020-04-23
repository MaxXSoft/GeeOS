#include <fstream>
#include <string_view>
#include <iostream>
#include <sstream>
#include <cstddef>

#include "geefs.h"
#include "iosdev.h"

using namespace std;

namespace {

void PrintHelp() {
  cout << "mkfs utility for GeeFS, by MaxXing" << endl;
  cout << "usage: mkfs [-h] image [-i]" << endl;
  cout << "            [-c blk_size free_map_num inode_blk_num]" << endl;
  cout << "            [-a file ...]" << endl << endl;
  cout << "options:" << endl;
  cout << "  -h         display this message" << endl;
  cout << "  -i         interactive mode" << endl;
  cout << "  -c         create a new GeeFS image" << endl;
  cout << "  -a         add files to current image" << endl;
}

int LogError(string_view msg) {
  cerr << msg << endl;
  return 1;
}

IOStreamDevice GetDeviceFromFile(fstream &fs, string_view file_name) {
  fs.open(file_name, ios::binary | ios::in | ios::out);
  if (!fs.is_open()) {
    fs.clear();
    fs.open(file_name, ios::out);
    fs.close();
    fs.open(file_name, ios::binary | ios::in | ios::out);
  }
  return IOStreamDevice(fs);
}

size_t GetStreamSize(istream &is) {
  auto pos = is.tellg();
  is.seekg(0, ios::end);
  auto size = is.tellg() - pos;
  is.seekg(0);
  return size;
}

bool GetInteger(const char *str, uint32_t &num) {
  istringstream iss(str);
  iss >> num;
  return !!iss;
}

string_view GetFileName(string_view path) {
  auto pos = path.find_last_of('/');
  if (pos == string_view::npos) return path;
  return path.substr(pos + 1);
}

int EnterIMode(GeeFS &geefs) {
  string line;
  // print prompt
  cout << geefs.cur_path() << "> ";
  while (getline(cin, line)) {
    if (!line.empty()) {
      // parse input
      if (line == "ls") {
        geefs.List(cout);
      }
      else if (line == "quit") {
        return 0;
      }
      else if (line.substr(0, 6) == "create") {
        if (!geefs.CreateFile(line.substr(7))) {
          LogError("failed to create file");
        }
      }
      else if (line.substr(0, 5) == "mkdir") {
        if (!geefs.MakeDir(line.substr(6))) {
          LogError("failed to create directory");
        }
      }
      else if (line.substr(0, 2) == "cd") {
        if (!geefs.ChangeDir(line.substr(3))) {
          LogError("failed to change directory");
        }
      }
      else if (line.substr(0, 2) == "rm") {
        if (!geefs.Remove(line.substr(3))) {
          LogError("failed to remove file");
        }
      }
      else if (line.substr(0, 4) == "read") {
        geefs.Read(line.substr(5), cout, 0, -1);
      }
      else {
        LogError("unknown command");
      }
    }
    // print next prompt
    cout << geefs.cur_path() << "> ";
  }
  cout << endl;
  return 0;
}

}  // namespace

int main(int argc, const char *argv[]) {
  // print help message
  if (argc < 2 || argv[1] == "-h"sv) {
    PrintHelp();
    return argc < 2;
  }

  // create GeeFS object
  auto fs = fstream();
  auto dev = GetDeviceFromFile(fs, argv[1]);
  auto geefs = GeeFS(dev);

  // read arguments
  bool imode = false, opened = false;
  for (int i = 2; i < argc; ++i) {
    if (argv[i][0] == '-') {
      switch (argv[i][1]) {
        case 'i': {
          imode = true;
          break;
        }
        case 'c': {
          // read arguments
          uint32_t blk_size, free_map_num, inode_blk_num;
          if (argc - i - 1 < 3) return LogError("insufficient argument");
          if (!GetInteger(argv[++i], blk_size) ||
              !GetInteger(argv[++i], free_map_num) ||
              !GetInteger(argv[++i], inode_blk_num)) {
            return LogError("invalid argument");
          }
          opened = true;
          if (!geefs.Create(blk_size, free_map_num, inode_blk_num)) {
            return LogError("can not create image");
          }
          break;
        }
        case 'a': {
          // open image
          if (!opened && !geefs.Open()) {
            return LogError("can not open image");
          }
          opened = true;
          // add files
          ++i;
          while (i < argc && argv[i][0] != '-') {
            auto file = GetFileName(argv[i]);
            // create file
            if (!geefs.CreateFile(file)) {
              return LogError("can not create file in image");
            }
            // create file stream
            ifstream ifs(argv[i]);
            auto size = GetStreamSize(ifs);
            if (!ifs || !geefs.Write(file, ifs, 0, size)) {
              return LogError("can not write file in image");
            }
            ++i;
          }
          break;
        }
      }
    }
  }

  // enter interactive mode
  if (imode) {
    if (!opened && !geefs.Open()) return LogError("can not open image");
    return EnterIMode(geefs);
  }
  return 0;
}
