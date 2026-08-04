#!/usr/bin/env python3

import argparse
from pathlib import Path
import re
import subprocess
import tempfile


def compile_source(sdcc, source, port, expect_success, workspace):
    object_file = workspace / f"{port}-{source.stem}.rel"
    command = [str(sdcc), f"-m{port}"]
    command.extend(["-c", "-o", str(object_file), str(source)])
    result = subprocess.run(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        errors="replace",
        check=False,
    )
    has_error = re.search(r"(?:^|\s)error(?:\s+[0-9]+)?:", result.stdout)
    succeeded = not result.returncode and not has_error
    if succeeded != expect_success:
        outcome = "rejected" if expect_success else "accepted"
        raise RuntimeError(
            f"{port} unexpectedly {outcome} {source.name}:\n"
            f"{result.stdout}"
        )
    if expect_success and (
        not object_file.is_file() or not object_file.stat().st_size
    ):
        raise RuntimeError(f"{port} did not generate {object_file.name}")

    outcome = "accepted" if expect_success else "rejected"
    print(f"PASS: {port} {outcome} {source.name}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sdcc", required=True)
    parser.add_argument("--incompatible-extern-source", required=True)
    parser.add_argument("--compatible-extern-array-source", required=True)
    parser.add_argument("--compatible-block-extern-source", required=True)
    parser.add_argument(
        "--incompatible-shadowed-struct-source", required=True
    )
    parser.add_argument("--invalid-wide-bitfield-source", required=True)
    args = parser.parse_args()

    sdcc = Path(args.sdcc).resolve()
    incompatible_source = Path(args.incompatible_extern_source).resolve()
    compatible_source = Path(args.compatible_extern_array_source).resolve()
    compatible_block_source = \
        Path(args.compatible_block_extern_source).resolve()
    incompatible_shadowed_source = \
        Path(args.incompatible_shadowed_struct_source).resolve()
    invalid_wide_bitfield_source = \
        Path(args.invalid_wide_bitfield_source).resolve()
    for path in (
        sdcc,
        incompatible_source,
        compatible_source,
        compatible_block_source,
        incompatible_shadowed_source,
        invalid_wide_bitfield_source,
    ):
        if not path.exists():
            parser.error(f"required path does not exist: {path}")

    with tempfile.TemporaryDirectory(prefix="sdcc-diagnostics.") as tmp:
        workspace = Path(tmp)
        for port in ("mcs51", "mcs251"):
            compile_source(
                sdcc, incompatible_source, port, False, workspace
            )
            compile_source(
                sdcc, compatible_source, port, True, workspace
            )
            compile_source(
                sdcc, compatible_block_source, port, True, workspace
            )
            compile_source(
                sdcc, incompatible_shadowed_source, port, False, workspace
            )
            compile_source(
                sdcc, invalid_wide_bitfield_source, port, False, workspace
            )


if __name__ == "__main__":
    main()
