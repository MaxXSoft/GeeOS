#!/usr/bin/env python3
"""Run the actual YuLang memset against alignment, size and guard-byte checks."""
import argparse
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--yuc', type=Path, default=Path('../YuLang/build/yuc'))
    parser.add_argument('--clang', default='clang')
    parser.add_argument('--lld', default='ld.lld')
    parser.add_argument('--qemu', default='qemu-system-riscv32')
    parser.add_argument('--simulator', type=Path,
                        help='run on Fuxi instead of QEMU virt')
    parser.add_argument('--output', type=Path, default=Path('build/memset-test'))
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    target = 'fuxi_sim' if args.simulator else 'virt'
    output = args.output.resolve() / target
    output.mkdir(parents=True, exist_ok=True)
    cc = [args.clang, '--target=riscv32-unknown-elf', '-march=rv32ima',
          '-mabi=ilp32', '-msmall-data-limit=0', '-O2', '-fno-builtin', '-c']
    subprocess.run(cc + [str(root / 'tests/runtime/memset.c'), '-o', str(output / 'test.o')],
                   check=True)
    definitions = ['-DFUXI_SIM'] if args.simulator else []
    subprocess.run(cc + definitions + [str(root / 'tests/runtime/memset_start.S'),
                   '-o', str(output / 'start.o')], check=True)
    if args.simulator:
        subprocess.run(cc + [str(root / 'src/boot/fuxi_sim.S'),
                       '-o', str(output / 'boot.o')], check=True)
        subprocess.run([args.lld, '-melf32lriscv', '--image-base=0', '-Ttext=0x200',
                        '--oformat=binary', str(output / 'boot.o'),
                        '-o', str(output / 'boot.bin')], check=True)
    for optimization in (0, 2):
        obj = output / f'string-O{optimization}.o'
        elf = output / f'memset-O{optimization}.elf'
        subprocess.run([str(args.yuc.resolve()), '-I', str(root / 'src'),
                        '-D', f'GEEOS_TARGET={target}', '-ot', 'obj',
                        '-tt', 'riscv32-unknown-elf', '-tc', 'generic-rv32',
                        '-tf', '+m,+a', '-O', str(optimization),
                        '-o', str(obj), str(root / 'src/lib/c/string.yu')], check=True)
        subprocess.run([args.lld, '-melf32lriscv', '-T', str(root / 'tests/runtime/memset.ld'),
                        str(output / 'start.o'), str(output / 'test.o'), str(obj),
                        '-o', str(elf)], check=True)
        if args.simulator:
            command = [str(args.simulator.resolve()), '--load',
                       f'0x200={output}/boot.bin', '--elf', str(elf),
                       '--max-cycles', '200000000']
        else:
            command = [args.qemu, '-nographic', '-machine', 'virt', '-bios', 'none',
                       '-m', '4m', '-kernel', str(elf)]
        with (output / f'O{optimization}.log').open('w') as log:
            subprocess.run(command, stdin=subprocess.DEVNULL, stdout=log,
                           stderr=subprocess.STDOUT, check=True, timeout=180)
        print(f'memset O{optimization} passed on {target}: 4864 guarded cases', flush=True)


if __name__ == '__main__':
    main()
