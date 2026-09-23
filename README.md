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

`YU_BIN` defaults to the `build` directory of YuLang repository; `LLVM_BIN` is detected from `llvm-config` on `PATH`. Override these paths as needed. If LLD is installed separately, set `LLD` to the path of `ld.lld`. For example, with Homebrew LLVM and LLD installed:

```sh
make -j LLVM_BIN="$(brew --prefix llvm)/bin" \
  LLD="$(brew --prefix lld)/bin/ld.lld" YU_BIN=/path/to/YuLang/build
```

ELF file of GeeOS will be generated in directory `build`. By default, you can run it with QEMU:

```
$ qemu-system-riscv32 -nographic -machine virt -bios none -m 128m -kernel build/geeos.elf
```

Run this command from the repository root. `-bios none` is required because GeeOS starts in machine mode at `0x80000000` and provides its own initialization; QEMU's default OpenSBI firmware occupies the same address range. The default CPU can be used with PMP enabled. Tested with QEMU 11.1.1.

After the shell appears, try `hello`, `alloc`, or `notepad`. Press Ctrl-A, then X to quit QEMU. Run `python3 tests/smoke/qemu_smoke.py` for an automated smoke test covering shell commands, heap allocation and repeated UART input. See [tests/README.md](tests/README.md) for test suites and build requirements.

The default target is `virt`. For Fuxi, run `make -j TARGET=fuxi`; use `make -j TARGET=virt` to switch back. Changing `TARGET` automatically rebuilds the library, bootloader and kernel YuLang objects with `-D GEEOS_TARGET=$(TARGET)`. Run `make clean` when changing toolchain paths or optimization settings; object files are shared between configurations. The QEMU ELF cannot be used unchanged on Fuxi because the peripheral maps differ.

## Fuxi simulation

Use `TARGET=fuxi_sim` for the Fuxi example in [verilator-axi-testbench](https://github.com/MaxXSoft/verilator-axi-testbench). Build an optimized image for practical simulation speed. Clean when changing `DEBUG` or compiler options; switching `TARGET` alone needs no clean:

```sh
make clean
make -j TARGET=fuxi_sim DEBUG=0 LLVM_BIN=/path/to/llvm/bin LLD=/path/to/ld.lld
```

Configure and build the simulator with its `fuxi` preset, then run from GeeOS:

```sh
/path/to/verilator-axi-testbench/build/fuxi/examples/fuxi/fuxi_sim \
  --load 0x200=build/boot.bin --elf build/geeos.elf --max-cycles 2000000000
```

The simulation target uses the first 4 MiB of RAM and reserves 128 KiB for the kernel heap. The `virt` and FPGA `fuxi` targets continue to use 128 MiB. Free pages are still filled with debug values during initialization. An interactive smoke test uses the same shell, allocation, process and repeated UART-input checks as the QEMU test:

```sh
python3 tests/smoke/fuxi_sim_smoke.py \
  --simulator /path/to/verilator-axi-testbench/build/fuxi/examples/fuxi/fuxi_sim
```

The test records output in `build/fuxi-sim-smoke.log`; `--timeout` controls the wall-clock timeout per expected response, and `--max-cycles` controls the simulated cycle budget. Use `--stall-probability 0.35` to add AXI backpressure. The test terminates the simulator after the checks; the shell normally runs until the host stops it.

## Memory primitive regression

The `memset` regression compiles the actual YuLang implementation at O0 and O2 and runs 4,864 guarded cases per build, covering unaligned destinations, zero and boundary lengths, page-sized fills, and conversion of `int` to byte:

```sh
python3 tests/runtime/run.py --case memset --yuc /path/to/YuLang/build/yuc \
  --clang /path/to/llvm/bin/clang --lld /path/to/ld.lld
```

This uses QEMU virt by default. Add `--simulator /path/to/fuxi_sim` to run the same checks on Fuxi. Test images and logs are saved in `build/runtime-tests/`.

## Changelog

See [CHANGELOG.md](CHANGELOG.md)

## References

GeeOS is heavily influenced by [rCore](https://github.com/rcore-os/rCore) and [xv6](https://github.com/mit-pdos/xv6-riscv).

## License

Copyright (C) 2020-2026 MaxXing. License GPLv3.
