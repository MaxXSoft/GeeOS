# GeeOS

GeeOS (寂) is a lightweight, UNIX like operating system, written in [YuLang](https://github.com/MaxXSoft/YuLang), developed for [Fuxi](https://github.com/MaxXSoft/Fuxi) processor.

## Getting Started

Before building GeeOS, please make sure you have installed the following dependencies:

* [YuLang](https://github.com/MaxXSoft/YuLang) compiler
* LLVM toolchain and LLD linker
* C++ compiler supporting C++17
* Python 3

You may want to check the toolchain configuration in `toolchain.mk`. Then you can build this repository by executing the following command lines:

```
$ git clone https://github.com/MaxXSoft/GeeOS.git
$ cd GeeOS
$ make -j
```

`YU_BIN` defaults to the sibling `../YuLang/build` directory; `LLVM_BIN` is detected from `llvm-config` on `PATH`. Override these paths as needed. If LLD is installed separately, set `LLD` to the path of `ld.lld`. For example, with Homebrew LLVM and LLD installed:

```sh
make -j LLVM_BIN="$(brew --prefix llvm)/bin" \
  LLD="$(brew --prefix lld)/bin/ld.lld" YU_BIN=/path/to/YuLang/build
```

ELF file of GeeOS will be generated in directory `build`. By default, you can run it with QEMU:

```
$ qemu-system-riscv32 -nographic -machine virt -bios none -m 128m -kernel build/geeos.elf
```

Run this command from the repository root. `-bios none` is required because GeeOS starts in machine mode at `0x80000000` and provides its own initialization; QEMU's default OpenSBI firmware occupies the same address range. The default CPU can be used with PMP enabled. Tested with QEMU 11.1.1.

After the shell appears, try `hello`, `alloc`, or `notepad`. Press Ctrl-A, then X to quit QEMU. Run `python3 tests/qemu_smoke.py` for an automated smoke test covering shell commands, heap allocation and repeated UART input.

For Fuxi, change the target import in `src/arch/arch.yu` to `arch.target.fuxi` and run `make -j` to rebuild the affected library, bootloader and kernel objects. Run `make clean` when changing toolchain paths or optimization settings; object files are shared between configurations. The QEMU ELF cannot be used unchanged on Fuxi because the peripheral maps differ.

## Details

> UNDER CONSTRUCTION...

## Changelog

See [CHANGELOG.md](CHANGELOG.md)

## References

GeeOS is heavily influenced by [rCore](https://github.com/rcore-os/rCore) and [xv6](https://github.com/mit-pdos/xv6-riscv).

## License

Copyright (C) 2020-2026 MaxXing. License GPLv3.
