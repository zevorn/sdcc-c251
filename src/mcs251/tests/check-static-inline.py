#!/usr/bin/env python3

import argparse
from pathlib import Path
import re
import subprocess
import tempfile


def check_port(sdcc, source, port, extra_options, workspace):
    suffix = "-".join(option.lstrip("-") for option in extra_options)
    name = port if not suffix else f"{port}-{suffix}"
    assembly = workspace / f"{name}-static-inline.asm"
    command = [
        str(sdcc),
        f"-m{port}",
        *extra_options,
        "-S",
        "-o",
        str(assembly),
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
    if result.returncode or not assembly.is_file():
        raise RuntimeError(
            f"{name} did not compile {source.name}:\n{result.stdout}"
        )

    text = assembly.read_text()
    expected_labels = (
        "addressed_inline_wrapper",
        "conditional_inline_wrapper",
    )
    forbidden = (
        "_unused_inline_wrapper:",
        "_called_inline_wrapper:",
        "_unavailable_inline_dependency",
    )
    for label in expected_labels:
        pattern = re.compile(rf"^_{label}:\s*$", re.MULTILINE)
        if len(pattern.findall(text)) != 1:
            raise RuntimeError(
                f"{name} did not emit address-taken inline {label}"
            )
    for fragment in forbidden:
        if fragment in text:
            raise RuntimeError(
                f"{name} unexpectedly emitted or referenced {fragment}"
            )
    print(f"PASS: {name} deferred static inline emission")


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
        configurations = (
            ("mcs51", ()),
            ("mcs251", ()),
            ("mcs51", ("--stack-auto", "--std-gnu17")),
            ("mcs251", ("--stack-auto", "--std-gnu17")),
        )
        for port, extra_options in configurations:
            check_port(sdcc, source, port, extra_options, workspace)


if __name__ == "__main__":
    main()
