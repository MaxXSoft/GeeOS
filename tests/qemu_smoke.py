#!/usr/bin/env python3
"""Exercise boot, process execution, allocation and UART input on QEMU virt."""
import argparse
from pathlib import Path
import selectors
import subprocess
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--kernel', type=Path, default=Path('build/geeos.elf'))
    parser.add_argument('--qemu', default='qemu-system-riscv32')
    parser.add_argument('--log', type=Path, default=Path('build/qemu-smoke.log'))
    args = parser.parse_args()
    output = bytearray()
    consumed = 0
    args.log.parent.mkdir(parents=True, exist_ok=True)
    with subprocess.Popen(
        [args.qemu, '-nographic', '-machine', 'virt', '-bios', 'none',
         '-m', '128m', '-kernel', str(args.kernel.resolve())],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    ) as proc, selectors.DefaultSelector() as selector:
        selector.register(proc.stdout, selectors.EVENT_READ)

        def expect(marker):
            nonlocal consumed
            deadline = time.monotonic() + 30
            while True:
                found = output.find(marker, consumed)
                if found >= 0:
                    consumed = found + len(marker)
                    return
                if time.monotonic() >= deadline:
                    raise RuntimeError(f'timed out waiting for {marker!r}')
                for key, _ in selector.select(0.2):
                    data = key.fileobj.read1(65536)
                    if not data:
                        raise RuntimeError('QEMU exited before the test completed')
                    output.extend(data)

        def send(data):
            proc.stdin.write(data)
            proc.stdin.flush()

        try:
            expect(b'Welcome to GeeOS shell!')
            expect(b'$ ')
            for command, expected in [
                (b'hello', b'Hello world!'),
                (b'alloc', b'deallocated all'),
                (b'missing', b'command not found: missing'),
                (b'hello', b'Hello world!'),
            ]:
                send(command + b'\r')
                expect(expected)
                expect(b'$ ')
            send(b'notepad\r')
            expect(b'try to type something:')
            # Separate submissions exercise repeated UART interrupts.
            for line in [b'GeeOS input test', b'second input line']:
                send(line + b'\r')
                expect(line + b'\r\n')
        finally:
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
            args.log.write_bytes(output)
    print(f'QEMU smoke test passed; log: {args.log}')


if __name__ == '__main__':
    main()
