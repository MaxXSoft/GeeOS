#!/usr/bin/env python3
"""Build actual GeeOS runtime modules and test them on RV32 at O0 and O2."""
import argparse
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
USER_MODULES = [
    'lib/alloc.yu', 'lib/stack.yu', 'lib/sync/spinlock.yu',
    'lib/except.yu', 'lib/io.yu', 'lib/c/string.yu',
    'lib/sys/syscall.S', 'lib/sync/slimpl.c',
]
CASES = {'stack': ('usr', 'stack.yu', USER_MODULES)}


def run(command):
    subprocess.run([str(arg) for arg in command], check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--yuc', type=Path, default=ROOT.parent / 'YuLang/build/yuc')
    parser.add_argument('--clang', default='clang')
    parser.add_argument('--lld', default='ld.lld')
    parser.add_argument('--qemu', default='qemu-system-riscv32')
    parser.add_argument('--simulator', type=Path, help='run Fuxi instead of QEMU')
    parser.add_argument('--output', type=Path, default=ROOT / 'build/runtime-tests')
    parser.add_argument('--case', action='append', choices=CASES,
                        help='test only selected cases (repeatable)')
    parser.add_argument('--timeout', type=float, default=180)
    args = parser.parse_args()
    target = 'fuxi_sim' if args.simulator else 'virt'
    output = args.output.resolve() / target
    output.mkdir(parents=True, exist_ok=True)
    cc = [args.clang, '--target=riscv32-unknown-elf', '-march=rv32ima',
          '-mabi=ilp32', '-msmall-data-limit=0', '-fno-builtin', '-c']
    definitions = ['-DFUXI_SIM'] if args.simulator else []
    run(cc + definitions + [HERE / 'start.S', '-o', output / 'start.o'])
    run(cc + ['-O2', HERE / 'runtime.c', '-o', output / 'runtime.o'])
    if args.simulator:
        run(cc + [ROOT / 'src/boot/fuxi_sim.S', '-o', output / 'boot.o'])
        run([args.lld, '-melf32lriscv', '--image-base=0', '-Ttext=0x200',
             '--oformat=binary', output / 'boot.o', '-o', output / 'boot.bin'])
    failures = []
    for case in args.case or CASES:
        folder, test, modules = CASES[case]
        for optimization in (0, 2):
            work = output / f'{case}-O{optimization}'
            work.mkdir(exist_ok=True)
            objects = [output / 'start.o', output / 'runtime.o']
            yu = [args.yuc.resolve(), '-I', ROOT / folder,
                  '-D', f'GEEOS_TARGET={target}', '-ot', 'obj',
                  '-tt', 'riscv32-unknown-elf', '-tc', 'generic-rv32',
                  '-tf', '+m,+a', '-O', str(optimization)]
            sources = [HERE / test] + [ROOT / folder / item for item in modules]
            for index, source in enumerate(sources):
                obj = work / f'{index}-{source.name}.o'
                compiler = yu if source.suffix == '.yu' else cc + [f'-O{optimization}']
                run(compiler + [source, '-o', obj])
                objects.append(obj)
            elf = work / 'test.elf'
            run([args.lld, '-melf32lriscv', '-T', HERE / 'runtime.ld',
                 *objects, '-o', elf])
            if args.simulator:
                command = [str(args.simulator.resolve()), '--load',
                           f'0x200={output}/boot.bin', '--elf', str(elf),
                           '--max-cycles', '200000000']
            else:
                command = [args.qemu, '-nographic', '-machine', 'virt', '-bios',
                           'none', '-m', '128m', '-kernel', str(elf)]
            log_path = work / 'run.log'
            with log_path.open('w') as log:
                result = subprocess.run(command, stdin=subprocess.DEVNULL, stdout=log,
                                        stderr=subprocess.STDOUT, timeout=args.timeout)
            passed = result.returncode == 0 and 'PASS\n' in log_path.read_text()
            print(f'{case} O{optimization} on {target}: '
                  f'{"passed" if passed else "FAILED"} ({log_path})', flush=True)
            if not passed:
                failures.append(f'{case}-O{optimization}')
    if failures:
        raise SystemExit('Failed: ' + ', '.join(failures))


if __name__ == '__main__':
    main()
