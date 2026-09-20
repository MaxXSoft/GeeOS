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


if __name__ == "__main__":
    main()
