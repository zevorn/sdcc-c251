#!/usr/bin/env python3
"""Check that each MCS target uses only its architectural byte registers."""

import argparse
import re
import subprocess
import tempfile
from pathlib import Path


HIGH_MCS251_REGISTER = re.compile(
    r"^[ \t]*[a-z]+[ \t]+[^;\n]*\br(?:8|9|1[0-5])\b",
    re.IGNORECASE | re.MULTILINE,
)
INVALID_BYTE_REGISTER = re.compile(
    r"^[ \t]*[a-z]+[ \t]+[^;\n]*\br(?:1[6-9]|2[0-9]|3[01])\b",
    re.IGNORECASE | re.MULTILINE,
)
INVALID_HIGH_CJNE = re.compile(
    r"^[ \t]*cjne[ \t]+[^;\n]*\br(?:8|9|1[0-5])\b",
    re.IGNORECASE | re.MULTILINE,
)
INVALID_HIGH_CARRY_SOURCE = re.compile(
    r"^[ \t]*(?:addc|subb)[ \t]+a,[ \t]*r(?:8|9|1[0-5])[ \t]*$",
    re.IGNORECASE | re.MULTILINE,
)
INVALID_HIGH_XCH = re.compile(
    r"^[ \t]*xch[ \t]+a,[ \t]*r(?:8|9|1[0-5])[ \t]*$",
    re.IGNORECASE | re.MULTILINE,
)
INVALID_HIGH_DJNZ = re.compile(
    r"^[ \t]*djnz[ \t]+r(?:8|9|1[0-5])(?:[ \t]*,|\b)",
    re.IGNORECASE | re.MULTILINE,
)
ALIASED_REGISTER_ALLOCATION = re.compile(
    r"Allocated to registers[^\n]*\br1[01]\b",
    re.IGNORECASE,
)
INVALID_HIGH_BANK_ALIAS = re.compile(
    r"\bar(?:8|9|1[0-5])\b",
    re.IGNORECASE,
)
# In MCS-251 Source mode these accumulator-memory forms only encode R0-R7 as
# the high operand (or none at all): ADD/ADDC/SUBB A,Rn, ANL/ORL/XRL A,Rn,
# CJNE A,Rn, and the CJNE Rn,#imm forms.  Any emitted occurrence with an
# R8-R15 operand is an illegal instruction the assembler would reject.
INVALID_HIGH_FIXED_ACC_OP = re.compile(
    r"^[ \t]*(?:(?:add|addc|subb|anl|orl|xrl)[ \t]+a,[ \t]*|"
    r"cjne[ \t]+(?:a,[ \t]*)?)r(?:8|9|1[0-5])\b",
    re.IGNORECASE | re.MULTILINE,
)


def compile_source(
    sdcc, source, port, output, extra_flags=(), assemble=False
):
    command = [
        str(sdcc),
        f"-m{port}",
        "--std=gnu17",
        "--model-small",
        "--opt-code-speed",
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
        prefix="sdcc-mcs251-register-allocation-"
    ) as temporary:
        workspace = Path(temporary)
        assembly = {}

        configurations = (
            ("mcs251", "mcs251", ()),
            ("mcs251-stack-auto", "mcs251", ("--stack-auto",)),
            (
                "mcs251-xstack-size",
                "mcs251",
                ("--xstack", "--opt-code-size"),
            ),
            ("mcs51", "mcs51", ()),
            ("mcs51-stack-auto", "mcs51", ("--stack-auto",)),
        )
        for name, port, extra_flags in configurations:
            asm_path = workspace / f"{name}.asm"
            rel_path = workspace / f"{name}.rel"
            compile_source(
                sdcc, source, port, asm_path, extra_flags
            )
            compile_source(
                sdcc, source, port, rel_path, extra_flags, assemble=True
            )
            assembly[name] = asm_path.read_text()

        if not HIGH_MCS251_REGISTER.search(assembly["mcs251"]):
            raise AssertionError(
                "MCS251 register pressure did not allocate R8-R15"
            )
        for name in ("mcs51", "mcs51-stack-auto"):
            if HIGH_MCS251_REGISTER.search(assembly[name]):
                raise AssertionError(
                    f"MCS251 byte registers leaked into {name}"
                )

        isr = function_body(
            assembly["mcs251"], "mcs251_register_pressure_isr"
        )
        for register in (8, 9, 12, 13, 14, 15):
            if not re.search(
                rf"^[ \t]*push[ \t]+r{register}[ \t]*$",
                isr,
                re.IGNORECASE | re.MULTILINE,
            ):
                raise AssertionError(
                    f"MCS251 calling ISR did not preserve R{register}"
                )
            if not re.search(
                rf"^[ \t]*pop[ \t]+r{register}[ \t]*$",
                isr,
                re.IGNORECASE | re.MULTILINE,
            ):
                raise AssertionError(
                    f"MCS251 calling ISR did not restore R{register}"
                )
        for register in ("b", "acc"):
            if not re.search(
                rf"^[ \t]*push[ \t]+{register}[ \t]*$",
                isr,
                re.IGNORECASE | re.MULTILINE,
            ):
                raise AssertionError(
                    f"MCS251 calling ISR did not preserve {register}"
                )
            if not re.search(
                rf"^[ \t]*pop[ \t]+{register}[ \t]*$",
                isr,
                re.IGNORECASE | re.MULTILINE,
            ):
                raise AssertionError(
                    f"MCS251 calling ISR did not restore {register}"
                )

        caller = function_body(
            assembly["mcs251"], "mcs251_byte_register_call_pressure"
        )
        if not HIGH_MCS251_REGISTER.search(caller):
            raise AssertionError(
                "MCS251 call pressure did not keep values in R8-R15"
            )
        if not re.search(
            r"^[ \t]*push[ \t]+r(?:8|9|1[0-5])[ \t]*$",
            caller,
            re.IGNORECASE | re.MULTILINE,
        ):
            raise AssertionError(
                "MCS251 caller did not preserve a live fixed register"
            )
        if not re.search(
            r"^[ \t]*pop[ \t]+r(?:8|9|1[0-5])[ \t]*$",
            caller,
            re.IGNORECASE | re.MULTILINE,
        ):
            raise AssertionError(
                "MCS251 caller did not restore a live fixed register"
            )

        dword_pressure = function_body(
            assembly["mcs251"], "mcs251_dword_register_pressure"
        )
        if not re.search(
            r"^[ \t]*(?:add|sub)[ \t]+(?:"
            r"dr12,[ \t]*dr(?:0|4|8|12|16|20|24|28)|"
            r"dr(?:0|4|8|12|16|20|24|28),[ \t]*dr12)[ \t]*$",
            dword_pressure,
            re.IGNORECASE | re.MULTILINE,
        ):
            raise AssertionError(
                "MCS251 dword pressure did not use DR12 in native "
                f"arithmetic:\n{dword_pressure}"
            )

        word_pressure = function_body(
            assembly["mcs251"], "mcs251_word_register_pressure"
        )
        if not re.search(
            r"^[ \t]*(?:add|sub)[ \t]+(?:"
            r"wr(?:8|12|14),[ \t]*wr(?:0|2|4|6|8|12|14)|"
            r"wr(?:0|2|4|6|8|12|14),[ \t]*wr(?:8|12|14))[ \t]*$",
            word_pressure,
            re.IGNORECASE | re.MULTILINE,
        ):
            raise AssertionError(
                "MCS251 word pressure did not use a fixed WR in native "
                f"tuple:\n{word_pressure}"
            )

        word_multiply = function_body(
            assembly["mcs251"], "mcs251_unsigned_word_multiply"
        )
        if not re.search(
            r"^[ \t]*mul[ \t]+wr(?:0|2|4|6|8|12|14),[ \t]*"
            r"wr(?:0|2|4|6|8|12|14)[ \t]*$",
            word_multiply,
            re.IGNORECASE | re.MULTILINE,
        ):
            raise AssertionError(
                "MCS251 unsigned 16x16 multiply did not select MUL WR,WR:"
                f"\n{word_multiply}"
            )
        if re.search(r"\bmul[ \t]+dr", word_multiply, re.IGNORECASE):
            raise AssertionError(
                "MCS251 emitted the nonexistent MUL DR form:\n"
                f"{word_multiply}"
            )

        high_register_shift = function_body(
            assembly["mcs251"], "mcs251_high_register_shift"
        )
        if not re.search(
            r"^[ \t]*push[ \t]+acc[ \t]*$\n"
            r"^[ \t]*mov[ \t]+a,[ \t]*(r(?:8|9|1[2-5]))[ \t]*$\n"
            r"^[ \t]*pop[ \t]+\1[ \t]*$",
            high_register_shift,
            re.IGNORECASE | re.MULTILINE,
        ):
            raise AssertionError(
                "MCS251 high-register shift did not lower accumulator "
                f"exchange through the stack:\n{high_register_shift}"
            )

        for port, text in assembly.items():
            invalid = INVALID_BYTE_REGISTER.search(text)
            if invalid:
                raise AssertionError(
                    f"{port} emitted an invalid byte register: "
                    f"{invalid.group(0).strip()}"
                )
            invalid_alias = INVALID_HIGH_BANK_ALIAS.search(text)
            if invalid_alias:
                raise AssertionError(
                    f"{port} emitted an invalid bank alias: "
                    f"{invalid_alias.group(0)}"
                )

        for name in (
            "mcs251",
            "mcs251-stack-auto",
            "mcs251-xstack-size",
        ):
            invalid_cjne = INVALID_HIGH_CJNE.search(assembly[name])
            if invalid_cjne:
                raise AssertionError(
                    f"{name} emitted CJNE with an R8-R15 operand: "
                    f"{invalid_cjne.group(0).strip()}"
                )

            invalid_carry = INVALID_HIGH_CARRY_SOURCE.search(assembly[name])
            if invalid_carry:
                raise AssertionError(
                    f"{name} emitted a carry instruction with R8-R15: "
                    f"{invalid_carry.group(0).strip()}"
                )

            invalid_xch = INVALID_HIGH_XCH.search(assembly[name])
            if invalid_xch:
                raise AssertionError(
                    f"{name} emitted XCH with R8-R15: "
                    f"{invalid_xch.group(0).strip()}"
                )

            invalid_djnz = INVALID_HIGH_DJNZ.search(assembly[name])
            if invalid_djnz:
                raise AssertionError(
                    f"{name} emitted DJNZ with R8-R15: "
                    f"{invalid_djnz.group(0).strip()}"
                )

            invalid_acc = INVALID_HIGH_FIXED_ACC_OP.search(assembly[name])
            if invalid_acc:
                raise AssertionError(
                    f"{name} emitted an R0-R7 accumulator op with an "
                    f"R8-R15 operand: {invalid_acc.group(0).strip()}"
                )

            aliased = ALIASED_REGISTER_ALLOCATION.search(assembly[name])
            if aliased:
                raise AssertionError(
                    f"{name} allocated the R10/B or R11/ACC alias as an "
                    f"independent register: {aliased.group(0).strip()}"
                )

        print(
            "PASS: MCS251 allocates fixed byte registers without "
            "changing MCS51"
        )


if __name__ == "__main__":
    main()
