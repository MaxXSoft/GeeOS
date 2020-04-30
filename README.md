# GeeOS

GeeOS (寂) is a lightweight, UNIX like operating system, written in [YuLang](https://github.com/MaxXSoft/YuLang), development for [Fuxi](https://github.com/MaxXSoft/Fuxi) processor.

## Getting Started

Before building GeeOS, please make sure you have installed the following dependencies:

* [YuLang](https://github.com/MaxXSoft/YuLang) compiler
* `llvm` toolchain
* C++ compiler supporting C++17

You may want to check the toolchain configuration in `toolchain.mk`. Then you can build this repository by executing the following command lines:

```
$ git clone https://github.com/MaxXSoft/GeeOS.git
$ cd GeeOS
$ make -j8
```

ELF file of GeeOS will be generated in directory `build`. By default, you can run it with QEMU:

```
$ qemu-system-riscv32 -nographic -machine virt -m 128m -kernel geeos.elf
```

## Details

> UNDER CONSTRUCTION...

## Changelog

See [CHANGELOG.md](CHANGELOG.md)

## License

Copyright (C) 2010-2020 MaxXing. License GPLv3.
