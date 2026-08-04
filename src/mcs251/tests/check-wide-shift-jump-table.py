#!/usr/bin/env python3
"""Check that wide fixed shifts do not poison MCS251 jump tables."""

import argparse
import re
import subprocess
import tempfile
from pathlib import Path


def compile_source(sdcc, source, port, output, extra_flags=(), assemble=False):
    command = [
        str(sdcc),
        f"-m{port}",
        "--std=gnu17",
        "--model-small",
        "--opt-code-size",
        *extra_flags,
        "-c" if assemble else "-S",
        "-o",
        str(output),
        str(source),
    ]
    completed = subprocess.run(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        errors="replace",
    )
    if completed.returncode:
        raise AssertionError(
            f"command failed ({completed.returncode}): "
            f"{' '.join(command)}\n{completed.stdout}"
        )


def function_body(assembly, name):
    match = re.search(
        rf"^_{re.escape(name)}:$\n(.*?)(?=^;-{{20,}}$|\Z)",
        assembly,
        re.MULTILINE | re.DOTALL,
    )
    if not match:
        raise AssertionError(f"function {name} is missing from assembly")
    return match.group(1)


def check_component_table(body, name):
    if not re.search(
        r"^[ \t]*ejmp[ \t]+@dr28[ \t]*$",
        body,
        re.IGNORECASE | re.MULTILINE,
    ):
        raise AssertionError(f"{name} does not use a 24-bit jump table")

    suffixes = (r"\$", r"\$>>8", r"\$>>16")
    for suffix in suffixes:
        if not re.search(
            rf"^[ \t]*\.db[ \t]+[0-9]+{suffix}[ \t]*$",
            body,
            re.IGNORECASE | re.MULTILINE,
        ):
            raise AssertionError(
                f"{name} is missing the {suffix} jump-table component"
            )


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

    with tempfile.TemporaryDirectory(
        prefix="sdcc-mcs251-wide-shift-jump-table-"
    ) as temporary:
        workspace = Path(temporary)
        configurations = (
            ("mcs251", "mcs251", ()),
            ("mcs251-stack-auto", "mcs251", ("--stack-auto",)),
            ("mcs51", "mcs51", ()),
            ("mcs51-stack-auto", "mcs51", ("--stack-auto",)),
        )
        assemblies = {}

        for name, port, extra_flags in configurations:
            asm_path = workspace / f"{name}.asm"
            rel_path = workspace / f"{name}.rel"
            compile_source(sdcc, source, port, asm_path, extra_flags)
            compile_source(
                sdcc,
                source,
                port,
                rel_path,
                extra_flags,
                assemble=True,
            )
            assemblies[name] = asm_path.read_text()

        for configuration in ("mcs251", "mcs251-stack-auto"):
            assembly = assemblies[configuration]
            for function in (
                "mcs251_large_switch",
                "mcs251_shift_then_switch",
                "mcs251_right_shift_then_switch",
            ):
                check_component_table(
                    function_body(assembly, function),
                    f"{configuration}:{function}",
                )


if __name__ == "__main__":
    main()
