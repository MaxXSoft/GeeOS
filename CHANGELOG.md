# Changelog

All notable changes to the Gee OS will be documented in this file.

## Unreleased

### Added

- CI for building and testing GeeOS.
- New target `fuxi_sim` for the verilator simulator of Fuxi.

### Changed

- Generate object file and Makefile dependencies directly with new YuLang compiler.
- Switch target platform of GeeOS by specifying `make TARGET=xxx`.
- Optimize `memset` to reduce boot time on Fuxi simulator.

### Fixed

- PMP, PTE A/D bit, PLIC issues on the newest riscv32 QEMU.
- Missing fences after task switching and page table updates.

## 0.0.1 - 2021-06-28
