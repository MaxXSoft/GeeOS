# Tests

- `host/`: C++ tests for the image builder, using an in-memory device.
- `runtime/`: bare-metal tests of the actual YuLang kernel/user libraries at O0 and O2.
- `system/`: user programs exercising the running kernel and filesystem.
- `smoke/`: boot, shell, process, allocation and repeated UART-input checks.

Run scripts from the repository root. Generated images and logs belong under `build/`.
Each Python runner has `--help` for tool paths and output locations.

```sh
python3 tests/host/run.py
python3 tests/smoke/qemu_smoke.py --kernel build/geeos.elf
python3 tests/runtime/memset_test.py --yuc ../YuLang/build/yuc --clang clang --lld ld.lld
```

For Fuxi, build GeeOS with `TARGET=fuxi_sim`, then pass its kernel and boot image
with the simulator built from the desired Fuxi checkout:

```sh
python3 tests/smoke/fuxi_sim_smoke.py --simulator /path/to/fuxi_sim \
  --kernel build/geeos.elf --boot build/boot.bin --stall-probability 0.35
python3 tests/runtime/memset_test.py --simulator /path/to/fuxi_sim
```

Use separate build directories (or `make clean`) when changing DEBUG or toolchain.
Fuxi simulation tests do not validate FPGA hardware.
