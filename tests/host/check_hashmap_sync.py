#!/usr/bin/env python3
"""Check that GeeOS tracks YuLang's HashMap and behavioral regression fixtures."""
import argparse
import difflib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def kernel_hashmap(source):
    imports = 'import std.sys.stdlib.{free, malloc}\nimport std.sys.string.memset\n'
    if not source.startswith(imports):
        raise ValueError('YuLang HashMap imports changed; review the kernel adaptation')
    source = source.replace(imports,
                            'import geeos.lib.alloc.heap\n'
                            'import geeos.lib.c.string.memset\n', 1)
    source = source.replace('malloc(', 'heap.alloc(')
    # The kernel allocator accepts mutable pointers for memory it owns.
    for pointer in ('cur', 'this.table', 'this.table.table'):
        source = source.replace(f'free({pointer} as u8*)',
                                f'heap.dealloc({pointer} as u8 var*)')
    return source


def kernel_cases(source):
    return source.replace('import std.hashmap.{HashMap, newHashMap}\n',
                          'import geeos.lib.hashmap.{HashMap, newHashMap}\n', 1)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--yulang', type=Path, default=ROOT.parent / 'YuLang',
                        help='YuLang source checkout at the intended upstream revision')
    args = parser.parse_args()
    pairs = [
        ('lib/hashmap.yu', 'src/lib/hashmap.yu', kernel_hashmap),
        ('tests/runtime/hashmap.yu', 'tests/runtime/hashmap_behavior.yu', kernel_cases),
    ]
    mismatches = []
    for upstream, local, adapt in pairs:
        expected = adapt((args.yulang / upstream).read_text())
        actual = (ROOT / local).read_text()
        if actual != expected:
            mismatches.append(local)
            print(''.join(difflib.unified_diff(
                expected.splitlines(True), actual.splitlines(True),
                fromfile=f'adapted YuLang/{upstream}', tofile=f'GeeOS/{local}')))
    if mismatches:
        raise SystemExit('HashMap synchronization mismatch: ' + ', '.join(mismatches))
    print('PASS: HashMap implementation and behavior tests match adapted YuLang sources')


if __name__ == '__main__':
    main()
