#!/usr/bin/env python3
"""Link test user programs into a built GeeOS kernel, then exercise real syscalls."""
import argparse
from pathlib import Path
import selectors
import subprocess
import time

from elf_fixtures import generate_elf_fixtures


def run(command):
    subprocess.run([str(arg) for arg in command], check=True)


def exercise(command, cases, log, timeout):
    log.parent.mkdir(parents=True, exist_ok=True)
    output = bytearray()
    consumed = 0
    with subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT) as proc, \
            selectors.DefaultSelector() as selector, log.open('wb') as stream:
        selector.register(proc.stdout, selectors.EVENT_READ)

        def expect(marker):
            nonlocal consumed
            deadline = time.monotonic() + timeout
            while True:
                pending = output[consumed:]
                for failure in (b'FAIL:', b'message:'):
                    start = pending.find(failure)
                    if start >= 0 and b'\n' in pending[start:]:
                        raise RuntimeError(f'kernel/test failure; see {log}')
                index = output.find(marker, consumed)
                if index >= 0:
                    consumed = index + len(marker)
                    return
                if time.monotonic() >= deadline:
                    raise RuntimeError(f'timed out waiting for {marker!r}; see {log}')
                for key, _ in selector.select(0.2):
                    data = key.fileobj.read1(65536)
                    if not data:
                        raise RuntimeError(f'simulator exited early; see {log}')
                    output.extend(data)
                    stream.write(data)
                    stream.flush()

        try:
            started = time.monotonic()
            expect(b'Welcome to GeeOS shell!')
            expect(b'$ ')
            print(f'Boot: {time.monotonic() - started:.2f}s', flush=True)
            for case in cases:
                started = time.monotonic()
                proc.stdin.write(case.encode() + b'\r')
                proc.stdin.flush()
                if case == 'elf_permissions':
                    for region in ('text', 'data'):
                        expect(f'CHECK: readonly {region} store'.encode())
                        expect(b'scause  = 15')
                        expect(b'ERROR: user thread memory access violation')
                elif case == 'user_faults':
                    for name, cause in (('illegal instruction', 2), ('breakpoint', 3)):
                        expect(f'CHECK: user {name}'.encode())
                        expect(f'scause  = {cause}'.encode())
                        expect(b'ERROR: user thread exception')
                expect(f'PASS: {case}'.encode())
                expect(b'$ ')
                print(f'PASS: {case} ({time.monotonic() - started:.2f}s)', flush=True)
        finally:
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()


def main():
    root = Path(__file__).resolve().parents[2]
    sources = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True,
                        help='up-to-date make BUILD_DIR, including objects and libraries')
    parser.add_argument('--output', type=Path,
                        help='default: BUILD_DIR/system-tests')
    parser.add_argument('--yuc', type=Path, default=root.parent / 'YuLang/build/yuc')
    parser.add_argument('--llvm-bin', type=Path, default=Path('/usr/bin'))
    parser.add_argument('--lld', default='ld.lld')
    parser.add_argument('--qemu', default='qemu-system-riscv32')
    parser.add_argument('--simulator', type=Path)
    parser.add_argument('--optimization', type=int, choices=(0, 2), default=2,
                        help='optimization of test user programs; match the kernel build')
    parser.add_argument('--case', nargs='+', choices=sorted(p.stem for p in sources.glob('*.yu')))
    parser.add_argument('--stall-probability', type=float, default=0)
    parser.add_argument('--timeout', type=float, default=180)
    args = parser.parse_args()
    base = args.build.resolve()
    target = 'fuxi_sim' if args.simulator else 'virt'
    if (base / 'obj/geeos-target').read_text().strip() != target:
        parser.error(f'kernel build must target {target}')
    out = (args.output or base / 'system-tests').resolve()
    out.mkdir(parents=True, exist_ok=True)
    cases = args.case or sorted(p.stem for p in sources.glob('*.yu'))
    started = time.monotonic()
    binaries = []
    programs = [sources / f'{case}.yu' for case in cases]
    programs += sorted((sources / 'fixtures').glob('*.yu'))
    for source in programs:
        obj, binary = out / f'{source.stem}.o', out / source.stem
        run([args.yuc.resolve(), '-I', root / 'usr', '-ot', 'obj',
             '-tt', 'riscv32-unknown-elf', '-tc', 'generic-rv32', '-tf', '+m,+a',
             '-O', args.optimization, '-o', obj, source])
        run([args.lld, '-nostdlib', '-melf32lriscv', '-L' + str(base), '-lgrt',
             '-o', binary, obj])
        run([args.llvm_bin / 'llvm-strip', '--strip-unneeded', '--strip-sections', binary])
        binaries.append(binary)
    if 'elf_reject' in cases:
        binaries += generate_elf_fixtures(out / 'elf_reject', out)
    # Exercise direct/single/double addressing and a second-level table switch.
    (out / 'big').write_bytes(bytes((i * 37 + i // 256) % 251 for i in range(53009)))
    run([base / 'mkfs', out / 'user.img', '-c', '256', '1', str(max(8, (len(binaries) + 5) // 3)), '-a',
         base / 'usr/shell', *binaries, out / 'big'])
    run([args.llvm_bin / 'clang', '--target=riscv32-unknown-elf', '-march=rv32ima',
         '-mabi=ilp32', '-fno-builtin', '-fno-pic', '-c', '-I' + str(out),
         '-o', out / 'init.o', root / 'src/init.S'])
    objects = [obj for obj in sorted((base / 'obj/src').rglob('*.o'))
               if obj.relative_to(base / 'obj/src').parts[0] not in ('arch', 'lib', 'boot')
               and obj.name != 'init.S.o']
    run([args.lld, '-nostdlib', '-melf32lriscv', '-T' + str(root / 'src/linker.ld'),
         '-L' + str(base), '-lgee', '-o', out / 'geeos.elf', out / 'init.o', *objects])
    print(f'Build system tests: {time.monotonic() - started:.2f}s', flush=True)
    if args.simulator:
        command = [str(args.simulator.resolve()), '--load', f'0x200={base}/boot.bin',
                   '--elf', str(out / 'geeos.elf'), '--max-cycles', '2000000000',
                   '--stall-probability', str(args.stall_probability)]
    else:
        command = [args.qemu, '-nographic', '-machine', 'virt', '-bios', 'none',
                   '-m', '128m', '-kernel', str(out / 'geeos.elf')]
    exercise(command, cases, out / 'run.log', args.timeout)


if __name__ == '__main__':
    main()
