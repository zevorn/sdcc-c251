#!/usr/bin/env python3

import argparse
from pathlib import Path
import re
import subprocess
import tempfile


def check_port(sdcc, source, port, workspace):
    assembly = workspace / f"{port}-static-inline.asm"
    command = [
        str(sdcc), f"-m{port}", "-S", "-o", str(assembly), str(source),
    ]
    result = subprocess.run(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        check=False,
    )
    if result.returncode or not assembly.is_file():
        raise RuntimeError(
            f"{port} did not compile {source.name}:\n{result.stdout}"
        )

    text = assembly.read_text()
    expected_label = re.compile(
        r"^_addressed_inline_wrapper:\s*$", re.MULTILINE
    )
    forbidden = (
        "_unused_inline_wrapper:",
        "_called_inline_wrapper:",
        "_unavailable_inline_dependency",
    )
    if len(expected_label.findall(text)) != 1:
        raise RuntimeError(
            f"{port} did not emit the address-taken inline definition"
        )
    for fragment in forbidden:
        if fragment in text:
            raise RuntimeError(
                f"{port} unexpectedly emitted or referenced {fragment}"
            )
    print(f"PASS: {port} deferred static inline emission")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sdcc", required=True)
    parser.add_argument("--source", required=True)
    args = parser.parse_args()

    sdcc = Path(args.sdcc).resolve()
    source = Path(args.source).resolve()
    for path in (sdcc, source):
        if not path.exists():
            parser.error(f"required path does not exist: {path}")

    with tempfile.TemporaryDirectory(prefix="sdcc-static-inline.") as tmp:
        workspace = Path(tmp)
        for port in ("mcs51", "mcs251"):
            check_port(sdcc, source, port, workspace)


if __name__ == "__main__":
    main()
