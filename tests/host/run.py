#!/usr/bin/env python3
"""Build and run the host GeeFS regressions without a RISC-V toolchain."""

import argparse
from pathlib import Path
import subprocess


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default="clang++")
    parser.add_argument("--output", type=Path, default=root / "build/tests/host")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    binary = args.output.resolve() / "geefs"
    subprocess.run([
        args.cxx, "-std=c++17", "-O2", "-DNDEBUG", "-I", str(root / "mkfs"),
        str(root / "tests/host/geefs.cpp"), str(root / "mkfs/geefs.cpp"),
        "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
    mkfs = args.output.resolve() / "mkfs"
    subprocess.run([
        args.cxx, "-std=c++17", "-O2", "-DNDEBUG", "-I", str(root / "mkfs"),
        str(root / "mkfs/main.cpp"), str(root / "mkfs/geefs.cpp"),
        str(root / "mkfs/iosdev.cpp"), "-o", str(mkfs),
    ], check=True)
    for name, data, success in [
        ("empty", b"", True),
        ("too-large", b"X" * (600 * 1024), False),
    ]:
        source = args.output.resolve() / name
        source.write_bytes(data)
        image = args.output.resolve() / (name + ".img")
        result = subprocess.run([
            str(mkfs), str(image), "-c", "256", "1", "2", "-a", str(source),
        ], capture_output=True, text=True)
        if (result.returncode == 0) != success:
            raise RuntimeError(f"mkfs {name}: unexpected status {result.returncode}: "
                               f"{result.stderr.strip()}")
    print("PASS: mkfs reports empty files and image exhaustion correctly")


if __name__ == "__main__":
    main()
