# Tests

Run scripts from the repository root. Each runner has `--help` for tool and output
paths. Images, objects and logs are written under `build/`.

| Suite | Coverage | Requirements |
| --- | --- | --- |
| `host/` | GeeFS image integrity, indirect blocks, failed creation, sparse writes, capacity limits, stream/CLI errors and device I/O counts | C++17 compiler, Python |
| `runtime/` | Actual kernel/user memset, memcmp, heap, Stack, HashMap and memory helpers at O0 and O2 | YuLang, LLVM, QEMU or Fuxi sim |
| `system/` | File reads, chdir, dup3, failed I/O offsets, user buffers, ELF validation/permissions and user exceptions | Built GeeOS objects/libraries, same toolchain, QEMU or Fuxi sim |
| `smoke/` | Boot, shell, process execution, allocation and repeated UART input | GeeOS kernel, QEMU or Fuxi sim |
| `incremental/` | mkfs rebuild regenerates the user image and kernel, then returns to a no-op | GeeOS build toolchain |

## Host and runtime

```sh
python3 tests/host/run.py --cxx clang++
python3 tests/runtime/run.py --yuc ../YuLang/build/yuc --clang clang --lld ld.lld
```

The runtime runner builds the real product modules independently at both O0 and
O2. Use repeatable `--case` options to select cases, for example `--case memset`
for the original 4,864 guarded alignment/length/value checks. Both kernel and user
memcmp cases exhaustively compare every byte pair. Heap tests include size
overflow, exhaustion and preservation of existing allocations. Stack and HashMap
tests check growth, lookups, removal and reuse.

## Kernel integration

Use separate build directories when changing DEBUG or compiler. The system runner
uses the supplied build's objects and libraries; run make first to update them.
Use the YuLang revision pinned in `.github/workflows/build-test.yml` or a newer
compatible revision; older compilers miscompile these boundary cases.
Its `--optimization` controls test user programs and should match the kernel:
`2` for `DEBUG=0`, `0` for `DEBUG=1`.

```sh
mkdir -p build
make -j8 TARGET=virt DEBUG=0 BUILD_DIR="$PWD/build/test-virt" \
  LLVM_BIN=/path/to/llvm/bin LLD=/path/to/ld.lld
python3 tests/system/run.py --build build/test-virt \
  --llvm-bin /path/to/llvm/bin --lld /path/to/ld.lld
python3 tests/smoke/qemu_smoke.py --kernel build/test-virt/geeos.elf
python3 tests/incremental/run.py --build build/test-virt \
  --make-arg TARGET=virt --make-arg DEBUG=0 \
  --make-arg LLVM_BIN=/path/to/llvm/bin --make-arg LLD=/path/to/ld.lld
```

The system runner packages its own user image without modifying `usr/bin` or the
normal kernel image. It reads a generated 53,009-byte pattern across direct,
indirect and double-indirect blocks, runs 19 malformed ELF fixtures, checks actual
exception causes, and requires the shell to return after each test. Use
`--case fs_read dup` to run a subset. Faulting child programs live in `system/fixtures/`
and are launched by the parent tests.

The incremental test touches only the generated mkfs binary. It checks that the
user image, embedding object and kernel rebuild, while other objects remain unchanged.

## Fuxi simulation

To build a simulator from a local Fuxi checkout (including local commits), use the
external source override in verilator-axi-testbench:

```sh
cd ../verilator-axi-testbench
cmake --preset fuxi -B build/geeos-local \
  -DAXI_TB_FUXI_SOURCE_DIR="$(cd ../Fuxi && pwd)"
cmake --build build/geeos-local --target fuxi_sim -j8
cd ../GeeOS
```

Build GeeOS with `TARGET=fuxi_sim` in a separate directory and use that simulator:

```sh
python3 tests/runtime/run.py --simulator /path/to/fuxi_sim \
  --clang /path/to/clang --lld /path/to/ld.lld
python3 tests/system/run.py --build build/test-fuxi \
  --llvm-bin /path/to/llvm/bin --lld /path/to/ld.lld \
  --simulator /path/to/fuxi_sim --stall-probability 0.35
python3 tests/smoke/fuxi_sim_smoke.py --simulator /path/to/fuxi_sim \
  --kernel build/test-fuxi/geeos.elf --boot build/test-fuxi/boot.bin \
  --stall-probability 0.35
```

These runs validate RTL simulation, not FPGA hardware. ELF validation covers format,
file bounds and mapping conflicts; general process resource-exhaustion recovery is
not implemented. The host filesystem tests cover capacity failures, not transactional
recovery from failing storage devices.

## HashMap upstream synchronization

`src/lib/hashmap.yu` follows YuLang's `lib/hashmap.yu` at the compiler revision pinned
in `.github/workflows/build-test.yml`. Internal names, default hash function and all
container behavior match upstream. The only adaptations are module imports and calls
to the kernel allocator/deallocator. `tests/runtime/hashmap_behavior.yu` mirrors the
upstream behavior fixture with its import path adapted; the kernel runner also retains
its own collision-work measurements.

After updating either copy, run:

```sh
python3 tests/host/check_hashmap_sync.py --yulang ../YuLang
python3 tests/runtime/run.py --case hashmap --clang /path/to/clang --lld /path/to/ld.lld
```

The first command checks both sources against the selected YuLang checkout; CI uses
its pinned checkout. Synchronize upstream fixes and behavior tests together, keeping
only the explicit adaptations encoded in the checker.
