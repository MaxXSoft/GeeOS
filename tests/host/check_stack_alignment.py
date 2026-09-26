#!/usr/bin/env python3
"""Check the initial stack symbols against the RV32 ILP32 ABI alignment."""
import argparse
from pathlib import Path
import subprocess


def check(nm, elf, symbol):
    output = subprocess.check_output(
        [nm, '--format=posix', '--defined-only', str(elf)], text=True)
    for line in output.splitlines():
        fields = line.split()
        if fields and fields[0] == symbol:
            address = int(fields[2], 16)
            if address % 16:
                raise SystemExit(f'{elf}: {symbol}=0x{address:x} is not 16-byte aligned')
            print(f'PASS: {elf.name} {symbol}=0x{address:x} is 16-byte aligned')
            return
    raise SystemExit(f'{elf}: missing {symbol}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--llvm-nm', default='llvm-nm')
    parser.add_argument('--kernel', type=Path, required=True)
    parser.add_argument('--boot', type=Path,
                        help='full bootloader ELF; omit for the Fuxi simulator trampoline')
    args = parser.parse_args()
    check(args.llvm_nm, args.kernel, '_stack_top')
    if args.boot:
        check(args.llvm_nm, args.boot, '_stack')


if __name__ == '__main__':
    main()
