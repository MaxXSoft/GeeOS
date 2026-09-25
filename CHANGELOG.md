# Changelog

All notable changes to the Gee OS will be documented in this file.

## 0.0.2 - 2026-09-25

### Added

- New target `fuxi_sim` for the verilator simulator of Fuxi.
- CI for building and testing GeeOS on both QEMU and Fuxi simulator.
- Host GeeFS/mkfs, RV32 runtime, kernel syscall/ELF, smoke, and incremental-build regression suites, with dedicated test documentation.
- A CI guard that keeps the kernel HashMap implementation and behavior fixtures synchronized with YuLang upstream.

### Changed

- Generate object file and Makefile dependencies directly with new YuLang compiler.
- Switch target platform of GeeOS by specifying `make TARGET=xxx`.
- Optimize `memset` to reduce boot time on Fuxi simulator.
- Reorganize tests by suite, unify runtime cases under one runner, and update build and test documentation.
- Synchronize the kernel HashMap with YuLang upstream and grow it by entry count so collision-heavy maps resize correctly.
- Read GeeFS and host mkfs data in block-sized chunks instead of byte by byte.
- Preserve final ELF segment permissions while loading user pages through the kernel's physical mapping.

### Fixed

- PMP, PTE A/D bit, PLIC issues on the newest riscv32 QEMU.
- Missing fences after task switching and page table updates.
- Host mkfs double-indirect allocation at block boundaries, reservation leaks after failed file creation, and inconsistent state after failed or sparse writes.
- GeeFS double-indirect reads using absolute instead of relative indices.
- User stack frame loss and an ineffective stack-pointer reset when crossing frame boundaries.
- `chdir` dereferencing missing paths and replacing the current directory in an unsafe order.
- `dup3` accepting invalid descriptors or allocating a descriptor other than the requested target.
- Integer overflow in kernel and user heap request-size calculations.
- File offsets advancing after failed reads or writes.
- `memcmp` wrapping byte differences instead of returning the correct signed difference.
- User ELF loading now rejects malformed headers, out-of-bounds segments, overlapping mappings, reserved-address conflicts, invalid entry points, and non-writable segment writes.
- Page-specific TLB invalidation now flushes the selected virtual address across all ASIDs.
- Synchronous user exceptions now terminate only the faulting thread, while kernel exceptions still panic.
- Syscalls now validate user buffers and NUL-terminated paths against mapped permissions before supervisor-mode access.
- Memory areas now detect page-granularity overlap, and empty or reversed page ranges remain empty.
- Clearing a page's dirty state now clears the D bit without altering the A bit.
- User filesystem images now rebuild when the mkfs executable changes.

## 0.0.1 - 2021-06-28
