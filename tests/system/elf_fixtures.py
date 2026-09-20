"""Malformed ELF32 fixtures built from a real linked user program."""
from pathlib import Path
import struct


def generate_elf_fixtures(source: Path, output: Path) -> list[Path]:
    image = source.read_bytes()
    phoff = struct.unpack_from('<I', image, 28)[0]
    phnum = struct.unpack_from('<H', image, 44)[0]
    loads = [phoff + i * 32 for i in range(phnum)
             if struct.unpack_from('<I', image, phoff + i * 32)[0] == 1]
    fixtures = {'elf_empty': b'', 'elf_short': image[:20],
                'elf_phshort': image[:phoff + phnum * 32 - 1]}

    def change(name, offset, fmt, value):
        data = bytearray(image)
        struct.pack_into(fmt, data, offset, value)
        fixtures[name] = data

    change('elf_class', 4, '<B', 2)
    change('elf_endian', 5, '<B', 2)
    change('elf_ehsize', 40, '<H', 1)
    change('elf_phsize', 42, '<H', 1)
    change('elf_phoffset', 28, '<I', 0xfffffff0)
    change('elf_phalign', 28, '<I', phoff + 1)
    change('elf_phcount', 44, '<H', 0xffff)
    change('elf_fileoff', loads[0] + 4, '<I', len(image) + 1)
    change('elf_filesize', loads[0] + 16, '<I', 0xffffffff)
    change('elf_memsize', loads[0] + 20, '<I', 1)
    change('elf_kernel', loads[0] + 8, '<I', 0x80000000)
    change('elf_stack', loads[0] + 8, '<I', 0x7ffef000)
    change('elf_uart', loads[0] + 8, '<I', 0x10000000)
    change('elf_wrap', loads[0] + 8, '<I', 0xfffffff0)
    change('elf_entry', 24, '<I', 0x70000000)
    change('elf_overlap', loads[1] + 8, '<I',
           struct.unpack_from('<I', image, loads[0] + 8)[0])
    paths = []
    for name, data in fixtures.items():
        path = output / name
        path.write_bytes(data)
        paths.append(path)
    return paths
