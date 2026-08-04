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
        errors="replace",
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


def forbid_instruction(body, pattern, description):
    if re.search(pattern, body, re.MULTILINE | re.IGNORECASE):
        raise AssertionError(f"found {description}:\n{body}")


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
        mcs251_stack_auto_asm = workspace / "mcs251-stack-auto.asm"
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
                "--stack-auto",
                "--opt-code-speed",
                "-S",
                "-o",
                str(mcs251_stack_auto_asm),
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
        replace_high_word = function_body(
            mcs251, "mcs251_replace_high_word"
        )
        add_longs = function_body(mcs251, "mcs251_add_register_longs")
        reverse_add_longs = function_body(
            mcs251, "mcs251_reverse_add_register_longs"
        )
        subtract_longs = function_body(
            mcs251, "mcs251_subtract_register_longs"
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
            replace_high_word,
            r"^[ \t]*movh[ \t]+dr(?:0|4),[ \t]*#(?:"
            r"(?:0x)?5678|\(\((?:0x)?56[ \t]*<<[ \t]*8\)"
            r"[ \t]*\|[ \t]*(?:0x)?78\))[ \t]*$",
            "MCS251 high-word replacement with MOVH",
        )
        require_instruction(
            add_longs,
            r"^[ \t]*add[ \t]+dr(?:0|4|8|12|16|20|24|28),[ \t]*"
            r"dr(?:0|4|8|12|16|20|24|28)[ \t]*$",
            "MCS251 native 32-bit register addition",
        )
        forbid_instruction(
            add_longs,
            r"^[ \t]*addc[ \t]+a,",
            "byte-at-a-time carry chain in MCS251 long addition",
        )
        require_instruction(
            reverse_add_longs,
            r"^[ \t]*add[ \t]+dr0,[ \t]*dr4[ \t]*$",
            "MCS251 native addition with the right-hand result tuple",
        )
        require_instruction(
            subtract_longs,
            r"^[ \t]*sub[ \t]+dr(?:0|4|8|12|16|20|24|28),[ \t]*"
            r"dr(?:0|4|8|12|16|20|24|28)[ \t]*$",
            "MCS251 native 32-bit register subtraction",
        )
        forbid_instruction(
            subtract_longs,
            r"^[ \t]*subb[ \t]+a,",
            "byte-at-a-time borrow chain in MCS251 long subtraction",
        )

        mcs251_stack_auto = mcs251_stack_auto_asm.read_text()
        stack_auto_high_word = function_body(
            mcs251_stack_auto, "mcs251_replace_high_word"
        )
        require_instruction(
            stack_auto_high_word,
            r"^[ \t]*mov[ \t]+r7,[ \t]*dpl[ \t]*$",
            "stack-auto low byte preservation before MOVH",
        )
        require_instruction(
            stack_auto_high_word,
            r"^[ \t]*mov[ \t]+r6,[ \t]*dph[ \t]*$",
            "stack-auto low word preservation before MOVH",
        )

        mcs51 = mcs51_asm.read_text()
        forbidden = re.search(
            r"^[ \t]*(?:(?:movs|movz|movh)[ \t]+|"
            r"(?:add|sub)[ \t]+dr(?:0|4|8|12|16|20|24|28),)",
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
