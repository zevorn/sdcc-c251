#!/usr/bin/env python3
"""Check the public MCS251 and MCS51 scalar object layouts."""

import argparse
import re
import subprocess
import tempfile
from pathlib import Path


def run(command):
    completed = subprocess.run(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        errors="replace",
    )
    if completed.returncode:
        raise AssertionError(
            f"command failed ({completed.returncode}): {' '.join(command)}\n"
            f"{completed.stdout}"
        )


def initialized_bytes(assembly, symbol, size):
    values = []
    for offset in range(size):
        destination = (
            rf"_{re.escape(symbol)}"
            if offset == 0
            else rf"\(_{re.escape(symbol)} \+ {offset}\)"
        )
        match = re.search(
            rf"^[ \t]*mov[ \t]+{destination},[ \t]*#0x([0-9a-f]{{2}})"
            rf"[ \t]*$",
            assembly,
            re.MULTILINE | re.IGNORECASE,
        )
        if not match:
            raise AssertionError(
                f"missing initializer byte {offset} for {symbol}:\n{assembly}"
            )
        values.append(int(match.group(1), 16))
    return values


def compile_target(sdcc, source, target, workspace):
    assembly = workspace / f"{target}.asm"
    rel = workspace / f"{target}.rel"
    common = [str(sdcc), f"-m{target}", "--model-small"]
    run(common + ["-S", "-o", str(assembly), str(source)])
    run(common + ["-c", "-o", str(rel), str(source)])
    return assembly.read_text()


def assert_hidden_return_pointer_is_big_endian(assembly):
    pattern = re.compile(
        r"mov[ \t]+a,[ \t]*#\((?P<symbol>[_.$A-Za-z0-9]+)[ \t]*"
        r">>[ \t]*16\)[ \t]*\n[ \t]*push[ \t]+acc[ \t]*\n"
        r"[ \t]*mov[ \t]+a,[ \t]*#\((?P=symbol)[ \t]*"
        r">>[ \t]*8\)[ \t]*\n[ \t]*push[ \t]+acc[ \t]*\n"
        r"[ \t]*mov[ \t]+a,[ \t]*#(?P=symbol)[ \t]*\n"
        r"[ \t]*push[ \t]+acc[ \t]*\n[ \t]*ecall[ \t]+"
        r"_make_big_result",
        re.IGNORECASE,
    )
    if not pattern.search(assembly):
        raise AssertionError(
            "MCS251 hidden return pointer is not pushed high byte first"
        )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sdcc", required=True)
    parser.add_argument("--source", required=True)
    parser.add_argument("--aggregate-source", required=True)
    args = parser.parse_args()

    sdcc = Path(args.sdcc).resolve()
    source = Path(args.source).resolve()
    aggregate_source = Path(args.aggregate_source).resolve()
    for path in (sdcc, source, aggregate_source):
        if not path.exists():
            parser.error(f"required path does not exist: {path}")

    with tempfile.TemporaryDirectory(prefix="sdcc-mcs251-endian-") as tmp:
        workspace = Path(tmp)
        mcs251 = compile_target(sdcc, source, "mcs251", workspace)
        mcs51 = compile_target(sdcc, source, "mcs51", workspace)
        aggregate = compile_target(sdcc, aggregate_source, "mcs251", workspace)

        assert_hidden_return_pointer_is_big_endian(aggregate)

        if initialized_bytes(mcs251, "mcs251_endian_word", 2) != [
            0x12,
            0x34,
        ]:
            raise AssertionError("MCS251 word objects are not big-endian")
        if initialized_bytes(mcs251, "mcs251_endian_dword", 4) != [
            0x12,
            0x34,
            0x56,
            0x78,
        ]:
            raise AssertionError("MCS251 dword objects are not big-endian")

        if initialized_bytes(mcs51, "mcs251_endian_word", 2) != [
            0x34,
            0x12,
        ]:
            raise AssertionError("MCS51 word object layout changed")
        if initialized_bytes(mcs51, "mcs251_endian_dword", 4) != [
            0x78,
            0x56,
            0x34,
            0x12,
        ]:
            raise AssertionError("MCS51 dword object layout changed")


if __name__ == "__main__":
    main()
