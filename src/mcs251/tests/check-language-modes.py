#!/usr/bin/env python3

import argparse
from pathlib import Path
import re
import subprocess


def check_mode(
    sdcc,
    source,
    port,
    mode,
    stdc_version,
    gnu_extensions,
    strict_ansi,
):
    command = [
        str(sdcc),
        f"-m{port}",
        f"--std={mode}",
        "--syntax-only",
        f"-DEXPECTED_STDC_VERSION={stdc_version}L",
        f"-DEXPECT_GNU_EXTENSIONS={int(gnu_extensions)}",
        f"-DEXPECT_STRICT_ANSI={int(strict_ansi)}",
        str(source),
    ]
    result = subprocess.run(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        errors="replace",
        check=False,
    )
    has_error = re.search(r"(?:^|\s)error(?:\s+[0-9]+)?:", result.stdout)
    if result.returncode or has_error:
        raise RuntimeError(
            f"{port} --std={mode} failed:\n{result.stdout}"
        )
    print(f"PASS: {port} --std={mode}")


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

    modes = (
        ("c11", 201112, False, True),
        ("c17", 201710, False, True),
        ("sdcc11", 201112, False, False),
        ("sdcc17", 201710, False, False),
        ("gnu11", 201112, True, False),
        ("gnu17", 201710, True, False),
    )
    for port in ("mcs51", "mcs251"):
        for mode, stdc_version, gnu_extensions, strict_ansi in modes:
            check_mode(
                sdcc,
                source,
                port,
                mode,
                stdc_version,
                gnu_extensions,
                strict_ansi,
            )


if __name__ == "__main__":
    main()
