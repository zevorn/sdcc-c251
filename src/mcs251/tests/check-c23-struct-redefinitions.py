#!/usr/bin/env python3

import argparse
from pathlib import Path
import re
import subprocess
import tempfile


def compile_source(sdcc, source, port, mode, expect_success, workspace):
    object_file = workspace / f"{port}-{mode}-{source.stem}.rel"
    result = subprocess.run(
        [
            str(sdcc),
            f"-m{port}",
            f"--std={mode}",
            "-c",
            "-o",
            str(object_file),
            str(source),
        ],
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
            f"{port} --std={mode} unexpectedly {outcome} "
            f"{source.name}:\n{result.stdout}"
        )
    if expect_success and (
        not object_file.is_file() or not object_file.stat().st_size
    ):
        raise RuntimeError(f"{port} did not generate {object_file.name}")

    outcome = "accepted" if expect_success else "rejected"
    print(f"PASS: {port} --std={mode} {outcome} {source.name}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sdcc", required=True)
    parser.add_argument("--compatible-c23-struct-source", required=True)
    parser.add_argument("--incompatible-c23-struct-source", required=True)
    args = parser.parse_args()

    sdcc = Path(args.sdcc).resolve()
    compatible_source = Path(args.compatible_c23_struct_source).resolve()
    incompatible_source = Path(
        args.incompatible_c23_struct_source
    ).resolve()
    for path in (sdcc, compatible_source, incompatible_source):
        if not path.exists():
            parser.error(f"required path does not exist: {path}")

    with tempfile.TemporaryDirectory(prefix="sdcc-c23-structs.") as tmp:
        workspace = Path(tmp)
        for port in ("mcs51", "mcs251"):
            compile_source(
                sdcc, compatible_source, port, "c23", True, workspace
            )
            compile_source(
                sdcc, compatible_source, port, "c17", False, workspace
            )
            compile_source(
                sdcc, incompatible_source, port, "c23", False, workspace
            )


if __name__ == "__main__":
    main()
