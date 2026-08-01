#!/usr/bin/env python3

import argparse
from pathlib import Path
import re
import subprocess
import tempfile


def compile_source(sdcc, source, port, mode, expect_success, workspace):
    mode_name = mode if mode else "default"
    object_file = workspace / f"{port}-{mode_name}-{source.stem}.rel"
    command = [str(sdcc), f"-m{port}"]
    if mode:
        command.append(f"--std={mode}")
    command.extend(["-c", "-o", str(object_file), str(source)])

    result = subprocess.run(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        check=False,
    )
    has_error = re.search(r"(?:^|\s)error(?:\s+[0-9]+)?:", result.stdout)
    succeeded = not result.returncode and not has_error
    if expect_success:
        if (
            not succeeded
            or not object_file.is_file()
            or not object_file.stat().st_size
        ):
            raise RuntimeError(
                f"{port} --std={mode_name} rejected {source.name}:\n"
                f"{result.stdout}"
            )
    elif succeeded:
        raise RuntimeError(
            f"{port} --std={mode_name} accepted GNU-only {source.name}"
        )
    result_name = "accepted" if expect_success else "rejected"
    print(
        f"PASS: {port} --std={mode_name} {result_name} {source.name}"
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sdcc", required=True)
    parser.add_argument("--empty-aggregate-source", required=True)
    parser.add_argument("--keyword-alias-source", required=True)
    parser.add_argument("--attribute-source", required=True)
    parser.add_argument("--invalid-attribute-source", required=True)
    parser.add_argument("--empty-declaration-source", required=True)
    parser.add_argument("--compound-literal-source", required=True)
    args = parser.parse_args()

    sdcc = Path(args.sdcc).resolve()
    empty_aggregate_source = Path(args.empty_aggregate_source).resolve()
    keyword_alias_source = Path(args.keyword_alias_source).resolve()
    attribute_source = Path(args.attribute_source).resolve()
    invalid_attribute_source = Path(args.invalid_attribute_source).resolve()
    empty_declaration_source = Path(args.empty_declaration_source).resolve()
    compound_literal_source = Path(args.compound_literal_source).resolve()
    for path in (
        sdcc,
        empty_aggregate_source,
        keyword_alias_source,
        attribute_source,
        invalid_attribute_source,
        empty_declaration_source,
        compound_literal_source,
    ):
        if not path.exists():
            parser.error(f"required path does not exist: {path}")

    with tempfile.TemporaryDirectory(prefix="sdcc-gnu-extensions.") as tmp:
        workspace = Path(tmp)
        for port in ("mcs51", "mcs251"):
            for mode in ("gnu11", "gnu17"):
                compile_source(
                    sdcc,
                    empty_aggregate_source,
                    port,
                    mode,
                    True,
                    workspace,
                )
            for mode in ("c11", "c17", None):
                compile_source(
                    sdcc,
                    empty_aggregate_source,
                    port,
                    mode,
                    False,
                    workspace,
                )
            for mode in ("gnu11", "gnu17", "c11", "c17", None):
                compile_source(
                    sdcc,
                    keyword_alias_source,
                    port,
                    mode,
                    True,
                    workspace,
                )
                compile_source(
                    sdcc,
                    attribute_source,
                    port,
                    mode,
                    True,
                    workspace,
                )
                compile_source(
                    sdcc,
                    invalid_attribute_source,
                    port,
                    mode,
                    False,
                    workspace,
                )
            # Empty declarations at file scope are a GNU extension before
            # C23.
            for mode in ("gnu11", "gnu17"):
                compile_source(
                    sdcc,
                    empty_declaration_source,
                    port,
                    mode,
                    True,
                    workspace,
                )
            for mode in ("c11", "c17", None):
                compile_source(
                    sdcc,
                    empty_declaration_source,
                    port,
                    mode,
                    False,
                    workspace,
                )
            for mode in ("gnu11", "gnu17", "c11", "c17", None):
                compile_source(
                    sdcc,
                    compound_literal_source,
                    port,
                    mode,
                    True,
                    workspace,
                )


if __name__ == "__main__":
    main()
