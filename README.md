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

The default target is `virt`. For Fuxi, run `make -j TARGET=fuxi`; use `make -j TARGET=virt` to switch back. Changing `TARGET` automatically rebuilds the library, bootloader and kernel YuLang objects with `-D GEEOS_TARGET=$(TARGET)`. Run `make clean` when changing toolchain paths or optimization settings; object files are shared between configurations. The QEMU ELF cannot be used unchanged on Fuxi because the peripheral maps differ.

## Fuxi simulation

Use `TARGET=fuxi_sim` for the Fuxi example in
[verilator-axi-testbench](https://github.com/MaxXSoft/verilator-axi-testbench).
It uses 128 MiB RAM at `0x80000000`, byte-addressed ns16550a UART at
`0x10000000` (PLIC IRQ 10), CLINT at `0x11000000`, PLIC at `0x12000000`
(S-mode context 1), and the simulator exit register at `0x10001000`.
The FPGA `fuxi` target retains its existing peripheral layout.

Build an optimized image for practical simulation speed. Clean when changing
`DEBUG` or compiler options; switching `TARGET` alone needs no clean:

```sh
make clean
make -j TARGET=fuxi_sim DEBUG=0 LLVM_BIN=/path/to/llvm/bin LLD=/path/to/ld.lld
```

Configure and build the simulator with its `fuxi` preset, then run from GeeOS:

```sh
../verilator-axi-testbench/build/fuxi/examples/fuxi/fuxi_sim \
  --load 0x200=build/boot.bin --elf build/geeos.elf --max-cycles 2000000000
```

For this target, `boot.bin` is an eight-byte ROM stub linked at Fuxi's reset
PC `0x200`. The simulator preloads the kernel ELF (including its user filesystem)
into RAM, and the stub jumps to `0x80000000`. Do not load this stub at ROM address
zero or use the FPGA flash/UART bootloader. GeeOS begins in M-mode, configures
its own timer handler, then enters S-mode without SBI firmware.

Memory initialization fills almost all 128 MiB before printing the next
message; RTL simulation takes substantially longer than QEMU. The default
simulator cycle budget is too small. An interactive smoke test uses the same
shell, allocation, process and repeated UART-input checks as the QEMU test:

```sh
python3 tests/fuxi_sim_smoke.py \
  --simulator ../verilator-axi-testbench/build/fuxi/examples/fuxi/fuxi_sim
```

The test records output in `build/fuxi-sim-smoke.log`; `--timeout` controls the
wall-clock timeout per expected response, and `--max-cycles` controls the
simulated cycle budget. Use `--stall-probability 0.35` to add AXI backpressure.
The test terminates the simulator after the checks; the shell normally runs
until the host stops it.

## Details

> UNDER CONSTRUCTION...

## Changelog

See [CHANGELOG.md](CHANGELOG.md)

## References

GeeOS is heavily influenced by [rCore](https://github.com/rcore-os/rCore) and [xv6](https://github.com/mit-pdos/xv6-riscv).

## License

Copyright (C) 2020-2026 MaxXing. License GPLv3.
