#!/usr/bin/env python3
"""Verify rebuilding mkfs regenerates the image and kernel, then becomes a no-op."""
import argparse
from pathlib import Path
import subprocess
import time


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--make-arg', action='append', default=[],
                        help='make variable assignment, repeat for toolchain/DEBUG overrides')
    args = parser.parse_args()
    build = args.build.resolve()
    build.parent.mkdir(parents=True, exist_ok=True)
    command = ['make', '-j2', f'BUILD_DIR={build}', *args.make_arg]

    def make():
        subprocess.run(command, cwd=root, check=True)

    def snapshot():
        paths = list((build / 'obj').rglob('*.o'))
        paths += [build / 'user.img', build / 'geeos.elf', build / 'libgee.a',
                  build / 'libgrt.a', build / 'boot.bin']
        return {p.relative_to(build).as_posix(): p.stat().st_mtime_ns for p in paths}

    make()
    before = snapshot()
    make()
    if snapshot() != before:
        raise RuntimeError('unchanged build was not a no-op')
    # Make 3.81 compares whole seconds. Touch the generated tool, not its source.
    time.sleep(1.1)
    (build / 'mkfs').touch()
    make()
    after = snapshot()
    changed = {p for p in before if before[p] != after[p]}
    expected = {'user.img', 'obj/src/init.S.o', 'geeos.elf'}
    if changed != expected:
        raise RuntimeError(f'mkfs rebuild changed {changed}; expected {expected}')
    make()
    if snapshot() != after:
        raise RuntimeError('rebuild did not return to a no-op')
    print('PASS: mkfs -> user.img -> init object -> kernel; subsequent build is a no-op')


if __name__ == '__main__':
    main()
