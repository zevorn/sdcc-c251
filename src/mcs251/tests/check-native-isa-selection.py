#!/usr/bin/env python3
"""Check MCS251-only instruction selection without changing the MCS51 port."""

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
    )
    if completed.returncode:
        raise AssertionError(
            f"command failed ({completed.returncode}): {' '.join(command)}\n"
            f"{completed.stdout}"
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


def require_instruction(body, pattern, description):
    if not re.search(pattern, body, re.MULTILINE | re.IGNORECASE):
        raise AssertionError(f"missing {description}:\n{body}")


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

    with tempfile.TemporaryDirectory(prefix="sdcc-mcs251-native-isa-") as tmp:
        workspace = Path(tmp)
        mcs251_asm = workspace / "mcs251.asm"
        mcs51_asm = workspace / "mcs51.asm"
        mcs251_rel = workspace / "mcs251.rel"

        run(
            [
                str(sdcc),
                "-mmcs251",
                "--model-small",
                "--opt-code-speed",
                "-S",
                "-o",
                str(mcs251_asm),
                str(source),
            ]
        )
        run(
            [
                str(sdcc),
                "-mmcs251",
                "--model-small",
                "--opt-code-speed",
                "-c",
                "-o",
                str(mcs251_rel),
                str(source),
            ]
        )
        run(
            [
                str(sdcc),
                "-mmcs51",
                "--model-small",
                "--opt-code-speed",
                "-S",
                "-o",
                str(mcs51_asm),
                str(source),
            ]
        )

        mcs251 = mcs251_asm.read_text()
        signed = function_body(mcs251, "mcs251_extend_signed_char")
        unsigned = function_body(mcs251, "mcs251_extend_unsigned_add")
        replace_low_word = function_body(
            mcs251, "mcs251_replace_low_word"
        )

        require_instruction(
            signed,
            r"^[ \t]*movs[ \t]+wr(?:0|2|4|6),[ \t]*r[0-7][ \t]*$",
            "MCS251 signed byte-to-word extension",
        )
        require_instruction(
            unsigned,
            r"^[ \t]*movz[ \t]+wr(?:0|2|4|6),[ \t]*r[0-7][ \t]*$",
            "MCS251 unsigned byte-to-word extension",
        )
        legacy_sign_extension = re.search(
            r"^[ \t]*(?:rlc[ \t]+a|subb[ \t]+a,[ \t]*acc)[ \t]*$",
            signed,
            re.MULTILINE | re.IGNORECASE,
        )
        if legacy_sign_extension:
            raise AssertionError(
                "signed extension retained the byte-at-a-time sequence:\n"
                f"{signed}"
            )
        require_instruction(
            replace_low_word,
            r"^[ \t]*movh[ \t]+dr(?:0|4),[ \t]*#(?:"
            r"(?:0x)?7856|\(\((?:0x)?78[ \t]*<<[ \t]*8\)"
            r"[ \t]*\|[ \t]*(?:0x)?56\))[ \t]*$",
            "MCS251 low-word replacement with MOVH",
        )

        mcs51 = mcs51_asm.read_text()
        forbidden = re.search(
            r"^[ \t]*(?:movs|movz|movh)[ \t]+",
            mcs51,
            re.MULTILINE | re.IGNORECASE,
        )
        if forbidden:
            raise AssertionError(
                "MCS251 extension instruction leaked into MCS51 output: "
                f"{forbidden.group(0).strip()}"
            )


if __name__ == "__main__":
    main()
