#!/usr/bin/env python3

import argparse
from pathlib import Path
import re
import subprocess
import tempfile


INVALID_IMMEDIATES = (
    re.compile(
        r"^[ \t]*mov[ \t]+[^,\n]+,#[ \t]*$", re.MULTILINE
    ),
    re.compile(
        r"^[ \t]*mov[ \t]+[^,\n]+,#[ \t]*"
        r"\([ \t]*>>[ \t]*(?:8|16)\)[ \t]*$",
        re.MULTILINE,
    ),
)


def run(command):
    result = subprocess.run(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        errors="replace",
        check=False,
    )
    if result.returncode:
        raise RuntimeError(
            f"command failed ({result.returncode}): {' '.join(command)}\n"
            f"{result.stdout}"
        )


def check_lane(sdcc, source, workspace, port, stack_auto):
    lane = f"{port}-{'stack-auto' if stack_auto else 'default'}"
    assembly = workspace / f"{lane}.asm"
    obj = workspace / f"{lane}.rel"
    options = ["--stack-auto"] if stack_auto else []

    run(
        [
            str(sdcc),
            f"-m{port}",
            "--std=c17",
            *options,
            "-S",
            "-o",
            str(assembly),
            str(source),
        ]
    )
    text = assembly.read_text()
    for pattern in INVALID_IMMEDIATES:
        match = pattern.search(text)
        if match:
            raise RuntimeError(
                f"{lane} emitted an empty constant address: "
                f"{match.group(0).strip()}"
            )

    run(
        [
            str(sdcc),
            f"-m{port}",
            "--std=c17",
            *options,
            "-c",
            "-o",
            str(obj),
            str(source),
        ]
    )
    if not obj.is_file():
        raise RuntimeError(f"{lane} did not produce an object file")
    print(f"PASS: {lane} assembled a compound-literal argument")


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

    with tempfile.TemporaryDirectory(prefix="sdcc-compound-argument.") as tmp:
        workspace = Path(tmp)
        for port in ("mcs51", "mcs251"):
            for stack_auto in (False, True):
                check_lane(
                    sdcc, source, workspace, port, stack_auto
                )


if __name__ == "__main__":
    main()
