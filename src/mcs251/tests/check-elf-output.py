#!/usr/bin/env python3
"""Verify that the MCS-251 linker can emit a pyelftools-readable ELF32
executable with a symbol table.

Zephyr's post-link tools (gen_isr_tables.py, gen_offset_elf) read the final
kernel image with pyelftools: they need a .symtab mapping symbol names to
addresses and named data sections.  Before this test existed the MCS-251
linker rejected ``-E`` entirely and could not produce an ELF at all.

This test compiles a small MCS-251 program, links it with ``--out-fmt-elf``,
and asserts that the output is a big-endian ELF32 ET_EXEC with e_machine
EM_8051 (165) and that its .symtab contains the expected function symbols.
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sdcc", required=True)
    parser.add_argument("--source", required=True)
    parser.add_argument("--out-dir", default=None)
    args = parser.parse_args()

    work = args.out_dir
    cleanup = work is None
    if work is None:
        work = tempfile.mkdtemp(prefix="sdcc-mcs251-elf.")
    else:
        os.makedirs(work, exist_ok=True)

    try:
        # pyelftools is a host-side dependency, transparent to SDCC.
        from elftools.elf.elffile import ELFFile
        from elftools.elf.sections import SymbolTableSection
    except ImportError as exc:
        print(f"SKIP: pyelftools not available ({exc})")
        return 0

    try:
        rel = os.path.join(work, "probe.rel")
        elf = os.path.join(work, "probe.elf")

        # Compile to a relocatable, then link with ELF output.
        subprocess.run(
            [args.sdcc, "-mmcs251", "--stack-auto", "--model-small",
             "-c", "-o", rel, args.source],
            check=True, capture_output=True, text=True, errors="replace")
        subprocess.run(
            [args.sdcc, "-mmcs251", "--stack-auto", "--model-small",
             "--out-fmt-elf", "-o", elf, rel],
            check=True, capture_output=True, text=True, errors="replace")

        with open(elf, "rb") as fp:
            loaded = ELFFile(fp)
            # pyelftools returns the e_machine as its symbolic name
            # ("EM_8051"); the value 165 is the ELF constant for 8051.
            machine = str(loaded["e_machine"])
            assert machine == "EM_8051", (
                f"expected EM_8051, got {machine}")
            data = str(loaded.header.e_ident.EI_DATA)
            assert data == "ELFDATA2MSB", (
                f"expected big-endian (ELFDATA2MSB), got {data}")

            syms = {}
            for section in loaded.iter_sections():
                if isinstance(section, SymbolTableSection):
                    for sym in section.iter_symbols():
                        if sym.name:
                            syms[sym.name] = sym.entry.st_value

            for name in ("_elf_probe_add", "_elf_probe_main"):
                assert name in syms, f"symbol {name} missing from .symtab"

        print("PASS: MCS-251 ELF output is big-endian EM_8051 with .symtab")
        return 0
    finally:
        if cleanup:
            shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())