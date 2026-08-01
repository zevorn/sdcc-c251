#!/usr/bin/env python3

import argparse
import csv
from pathlib import Path
import re
import subprocess
import tempfile


LISTING_BYTES = re.compile(
    r"^\s*[0-9A-Fa-f]{4,8}\s+"
    r"([0-9A-Fa-f]{2}(?: [0-9A-Fa-f]{2})*)(?=\s{2,})"
)
FORM_MARKER = re.compile(r";\s*FORM:([a-z0-9_]+)\s*$")


def read_matrix(path):
    with path.open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream, delimiter="\t"))
    required = {
        "id", "mnemonic", "operands", "assembly", "source_bytes",
        "binary_bytes", "flags", "reference",
    }
    if not rows or set(rows[0]) != required:
        raise SystemExit(f"{path}: unexpected matrix columns")

    identifiers = set()
    for row in rows:
        identifier = row["id"]
        if not re.fullmatch(r"[a-z0-9_]+", identifier):
            raise SystemExit(f"{path}: invalid form id: {identifier!r}")
        if identifier in identifiers:
            raise SystemExit(f"{path}: duplicate form id: {identifier}")
        identifiers.add(identifier)
        for mode in ("source", "binary"):
            value = row[f"{mode}_bytes"]
            if not re.fullmatch(r"[0-9a-f]{2}(?: [0-9a-f]{2})*", value):
                raise SystemExit(
                    f"{path}: invalid {mode} golden bytes for {identifier}"
                )
        if not row["reference"]:
            raise SystemExit(f"{path}: missing manual reference for {identifier}")
    return rows


def assemble_mode(assembler, rows, mode, work_dir):
    source = work_dir / f"instruction-forms-{mode}.asm"
    listing = source.with_suffix(".lst")
    lines = [
        f"        .module instruction_forms_{mode}",
        "        .area MCS251FORMS (ABS,CODE)",
        "        .org 0x120000",
        f"        .{mode}",
        "",
    ]
    for row in rows:
        identifier = row["id"]
        target = f"target_{identifier}"
        assembly = row["assembly"].replace("{target}", target)
        lines.append(f"        {assembly} ; FORM:{identifier}")
        if "{target}" in row["assembly"]:
            lines.append(f"{target}:")
    source.write_text("\n".join(lines) + "\n", encoding="ascii")

    completed = subprocess.run(
        [str(assembler), "-plosg", source.name],
        cwd=work_dir,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    if completed.returncode:
        raise SystemExit(
            f"{mode} instruction-form assembly failed:\n{completed.stdout}"
        )

    actual = {}
    for line in listing.read_text(encoding="ascii").splitlines():
        marker = FORM_MARKER.search(line)
        if not marker:
            continue
        encoded = LISTING_BYTES.match(line)
        if not encoded:
            raise SystemExit(
                f"could not parse {mode} listing bytes for {marker.group(1)}: "
                f"{line}"
            )
        actual[marker.group(1)] = encoded.group(1).lower()

    failures = []
    for row in rows:
        identifier = row["id"]
        expected = row[f"{mode}_bytes"]
        value = actual.get(identifier)
        if value != expected:
            failures.append(
                f"{mode} {identifier}: expected [{expected}], got [{value}]"
            )
    if failures:
        raise SystemExit("instruction-form encoding mismatches:\n" +
                         "\n".join(failures))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("assembler")
    parser.add_argument("matrix")
    args = parser.parse_args()

    assembler = Path(args.assembler).resolve()
    matrix = Path(args.matrix).resolve()
    if not assembler.is_file():
        parser.error(f"assembler does not exist: {assembler}")
    rows = read_matrix(matrix)

    with tempfile.TemporaryDirectory(prefix="sdas251-instruction-forms.") as tmp:
        work_dir = Path(tmp)
        assemble_mode(assembler, rows, "source", work_dir)
        assemble_mode(assembler, rows, "binary", work_dir)

    mnemonics = {row["mnemonic"] for row in rows}
    print(
        f"MCS-251 ISA matrix: {len(rows)} legal operand forms, "
        f"{len(mnemonics)} instruction families, 2 opcode maps passed"
    )


if __name__ == "__main__":
    main()
