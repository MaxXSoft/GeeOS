#!/usr/bin/env python3
"""Exercise GeeOS on the verilator-axi-testbench Fuxi platform."""
import argparse
from pathlib import Path

from qemu_smoke import run_smoke


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--simulator', type=Path, required=True)
    parser.add_argument('--kernel', type=Path, default=Path('build/geeos.elf'))
    parser.add_argument('--boot', type=Path, default=Path('build/boot.bin'))
    parser.add_argument('--log', type=Path, default=Path('build/fuxi-sim-smoke.log'))
    parser.add_argument('--timeout', type=float, default=1800,
                        help='wall-clock seconds allowed for each expected response')
    parser.add_argument('--max-cycles', type=int, default=2000000000)
    parser.add_argument('--stall-probability', type=float, default=0)
    args = parser.parse_args()
    run_smoke([
        str(args.simulator.resolve()), '--load', f'0x200={args.boot.resolve()}',
        '--elf', str(args.kernel.resolve()), '--max-cycles', str(args.max_cycles),
        '--stall-probability', str(args.stall_probability),
    ], args.log, args.timeout)
    print(f'Fuxi simulation smoke test passed; log: {args.log}')


if __name__ == '__main__':
    main()
