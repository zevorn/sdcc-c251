#!/usr/bin/env python3

import argparse
from pathlib import Path
import re
import subprocess
import tempfile

DEVICE_INCLUDE = None


def compile_source(
    sdcc,
    source,
    port,
    mode,
    expect_success,
    workspace,
    extra_options=(),
):
    mode_name = mode if mode else "default"
    configuration = f"{port} --std={mode_name}"
    if extra_options:
        configuration += " " + " ".join(extra_options)
    object_file = workspace / f"{port}-{mode_name}-{source.stem}.rel"
    command = [str(sdcc), f"-m{port}"]
    if mode:
        command.append(f"--std={mode}")
    command.extend(extra_options)
    if DEVICE_INCLUDE:
        command.extend([f"-I{DEVICE_INCLUDE}"])
    command.extend(["-c", "-o", str(object_file), str(source)])

    result = subprocess.run(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        errors="replace",
        check=False,
    )
    has_error = re.search(r"(?:^|\s)error(?:\s+[0-9]+)?:", result.stdout)
    has_failed_assertion = "static assertion failed" in result.stdout
    succeeded = (
        not result.returncode
        and not has_error
        and not has_failed_assertion
    )
    if expect_success:
        if (
            not succeeded
            or not object_file.is_file()
            or not object_file.stat().st_size
        ):
            raise RuntimeError(
                f"{configuration} rejected {source.name}:\n"
                f"{result.stdout}"
            )
    elif succeeded:
        raise RuntimeError(
            f"{configuration} accepted GNU-only {source.name}"
        )
    result_name = "accepted" if expect_success else "rejected"
    print(
        f"PASS: {configuration} {result_name} {source.name}"
    )


def check_expect_hints(sdcc, source, port, mode, workspace):
    dump_dir = workspace / f"{port}-{mode}-expect-dump"
    dump_dir.mkdir()
    command = [
        str(sdcc),
        f"-m{port}",
        f"--std={mode}",
        "--dump-i-code",
        "-c",
        str(source),
    ]
    result = subprocess.run(
        command,
        cwd=dump_dir,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        errors="replace",
        check=False,
    )
    dump_file = dump_dir / f"{source.stem}.dumpraw0"
    if result.returncode or not dump_file.is_file():
        raise RuntimeError(
            f"{port} --std={mode} did not dump {source.name}:\n"
            f"{result.stdout}"
        )

    dump = dump_file.read_text()
    side_effects = re.search(
        r"proc _gnu_expect_side_effects\b.*?"
        r"eproc _gnu_expect_side_effects\b",
        dump,
        re.DOTALL,
    )
    if not side_effects:
        raise RuntimeError(
            f"{port} --std={mode} lost the side-effect test function"
        )
    for symbol in (
        "_gnu_expect_value_count",
        "_gnu_expect_hint_count",
    ):
        if len(re.findall(rf"\b{symbol}\b", side_effects.group())) != 2:
            raise RuntimeError(
                f"{port} --std={mode} did not evaluate {symbol} once"
            )
    for expectation in ("true", "false"):
        marker = f"expect {expectation}"
        if marker not in dump:
            raise RuntimeError(
                f"{port} --std={mode} lost {marker} in {dump_file.name}"
            )
    print(f"PASS: {port} --std={mode} preserved branch expectations")


def check_constant_p_side_effects(sdcc, source, port, mode, workspace):
    dump_dir = workspace / f"{port}-{mode}-constant-p-dump"
    dump_dir.mkdir()
    command = [
        str(sdcc),
        f"-m{port}",
        f"--std={mode}",
        "--dump-i-code",
        "-c",
        str(source),
    ]
    result = subprocess.run(
        command,
        cwd=dump_dir,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        errors="replace",
        check=False,
    )
    dump_file = dump_dir / f"{source.stem}.dumpraw0"
    if result.returncode or not dump_file.is_file():
        raise RuntimeError(
            f"{port} --std={mode} did not dump {source.name}:\n"
            f"{result.stdout}"
        )

    dump = dump_file.read_text()
    function = re.search(
        r"proc _gnu_constant_p_side_effects\b.*?"
        r"eproc _gnu_constant_p_side_effects\b",
        dump,
        re.DOTALL,
    )
    if not function:
        raise RuntimeError(
            f"{port} --std={mode} lost the constant-p test function"
        )
    for symbol in (
        "_gnu_constant_p_counter",
        "_gnu_constant_p_runtime_value",
    ):
        if symbol in function.group():
            raise RuntimeError(
                f"{port} --std={mode} evaluated "
                f"__builtin_constant_p's operand {symbol}"
            )
    print(f"PASS: {port} --std={mode} discarded constant-p operands")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sdcc", required=True)
    parser.add_argument("--device-include", default=None)
    parser.add_argument("--empty-aggregate-source", required=True)
    parser.add_argument("--keyword-alias-source", required=True)
    parser.add_argument("--attribute-source", required=True)
    parser.add_argument("--invalid-attribute-source", required=True)
    parser.add_argument("--types-compatible-source", required=True)
    parser.add_argument("--invalid-types-compatible-source", required=True)
    parser.add_argument("--builtin-expect-source", required=True)
    parser.add_argument("--invalid-builtin-expect-source", required=True)
    parser.add_argument("--builtin-constant-p-source", required=True)
    parser.add_argument("--invalid-builtin-constant-p-source", required=True)
    parser.add_argument("--has-builtin-source", required=True)
    parser.add_argument("--bit-builtins-source", required=True)
    parser.add_argument("--invalid-bit-builtins-source", required=True)
    parser.add_argument("--byte-swap-builtins-source", required=True)
    parser.add_argument(
        "--invalid-byte-swap-builtins-source", required=True
    )
    parser.add_argument("--overflow-builtins-source", required=True)
    parser.add_argument("--invalid-overflow-builtins-source", required=True)
    parser.add_argument("--typed-overflow-builtins-source", required=True)
    parser.add_argument(
        "--invalid-typed-overflow-builtins-source", required=True
    )
    parser.add_argument("--auto-type-source", required=True)
    parser.add_argument("--invalid-auto-type-source", required=True)
    parser.add_argument("--multiple-auto-type-source", required=True)
    parser.add_argument("--declarator-auto-type-source", required=True)
    parser.add_argument("--empty-declaration-source", required=True)
    parser.add_argument("--compound-literal-source", required=True)
    parser.add_argument("--statement-expression-source", required=True)
    parser.add_argument("--case-range-source", required=True)
    parser.add_argument("--extension-source", required=True)
    args = parser.parse_args()

    global DEVICE_INCLUDE
    if args.device_include:
        DEVICE_INCLUDE = str(Path(args.device_include).resolve())

    sdcc = Path(args.sdcc).resolve()
    empty_aggregate_source = Path(args.empty_aggregate_source).resolve()
    keyword_alias_source = Path(args.keyword_alias_source).resolve()
    attribute_source = Path(args.attribute_source).resolve()
    invalid_attribute_source = Path(args.invalid_attribute_source).resolve()
    types_compatible_source = Path(args.types_compatible_source).resolve()
    invalid_types_compatible_source = Path(
        args.invalid_types_compatible_source
    ).resolve()
    builtin_expect_source = Path(args.builtin_expect_source).resolve()
    invalid_builtin_expect_source = Path(
        args.invalid_builtin_expect_source
    ).resolve()
    builtin_constant_p_source = Path(
        args.builtin_constant_p_source
    ).resolve()
    invalid_builtin_constant_p_source = Path(
        args.invalid_builtin_constant_p_source
    ).resolve()
    has_builtin_source = Path(args.has_builtin_source).resolve()
    bit_builtins_source = Path(args.bit_builtins_source).resolve()
    invalid_bit_builtins_source = Path(
        args.invalid_bit_builtins_source
    ).resolve()
    byte_swap_builtins_source = Path(
        args.byte_swap_builtins_source
    ).resolve()
    invalid_byte_swap_builtins_source = Path(
        args.invalid_byte_swap_builtins_source
    ).resolve()
    overflow_builtins_source = Path(
        args.overflow_builtins_source
    ).resolve()
    invalid_overflow_builtins_source = Path(
        args.invalid_overflow_builtins_source
    ).resolve()
    typed_overflow_builtins_source = Path(
        args.typed_overflow_builtins_source
    ).resolve()
    invalid_typed_overflow_builtins_source = Path(
        args.invalid_typed_overflow_builtins_source
    ).resolve()
    auto_type_source = Path(args.auto_type_source).resolve()
    invalid_auto_type_source = Path(args.invalid_auto_type_source).resolve()
    multiple_auto_type_source = Path(
        args.multiple_auto_type_source
    ).resolve()
    declarator_auto_type_source = Path(
        args.declarator_auto_type_source
    ).resolve()
    empty_declaration_source = Path(args.empty_declaration_source).resolve()
    compound_literal_source = Path(args.compound_literal_source).resolve()
    statement_expression_source = Path(
        args.statement_expression_source
    ).resolve()
    case_range_source = Path(args.case_range_source).resolve()
    extension_source = Path(args.extension_source).resolve()
    for path in (
        sdcc,
        empty_aggregate_source,
        keyword_alias_source,
        attribute_source,
        invalid_attribute_source,
        types_compatible_source,
        invalid_types_compatible_source,
        builtin_expect_source,
        invalid_builtin_expect_source,
        builtin_constant_p_source,
        invalid_builtin_constant_p_source,
        has_builtin_source,
        bit_builtins_source,
        invalid_bit_builtins_source,
        byte_swap_builtins_source,
        invalid_byte_swap_builtins_source,
        overflow_builtins_source,
        invalid_overflow_builtins_source,
        typed_overflow_builtins_source,
        invalid_typed_overflow_builtins_source,
        auto_type_source,
        invalid_auto_type_source,
        multiple_auto_type_source,
        declarator_auto_type_source,
        empty_declaration_source,
        compound_literal_source,
        statement_expression_source,
        case_range_source,
        extension_source,
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
                    extension_source,
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
            for mode in ("gnu11", "gnu17"):
                compile_source(
                    sdcc,
                    case_range_source,
                    port,
                    mode,
                    True,
                    workspace,
                )
            for mode in ("c11", "c17", None):
                compile_source(
                    sdcc,
                    case_range_source,
                    port,
                    mode,
                    False,
                    workspace,
                )
            for mode in ("gnu11", "gnu17"):
                compile_source(
                    sdcc,
                    typed_overflow_builtins_source,
                    port,
                    mode,
                    True,
                    workspace,
                )
                compile_source(
                    sdcc,
                    typed_overflow_builtins_source,
                    port,
                    mode,
                    True,
                    workspace,
                    ("--stack-auto",),
                )
                for invalid_case in range(1, 12):
                    compile_source(
                        sdcc,
                        invalid_typed_overflow_builtins_source,
                        port,
                        mode,
                        False,
                        workspace,
                        (
                            "-DGNU_TYPED_OVERFLOW_INVALID_CASE="
                            f"{invalid_case}",
                        ),
                    )
            for mode in ("c11", "c17", None):
                compile_source(
                    sdcc,
                    typed_overflow_builtins_source,
                    port,
                    mode,
                    False,
                    workspace,
                )
            for mode in ("gnu11", "gnu17"):
                compile_source(
                    sdcc,
                    statement_expression_source,
                    port,
                    mode,
                    True,
                    workspace,
                )
                compile_source(
                    sdcc,
                    statement_expression_source,
                    port,
                    mode,
                    True,
                    workspace,
                    ("--stack-auto",),
                )
            for mode in ("c11", "c17", None):
                compile_source(
                    sdcc,
                    statement_expression_source,
                    port,
                    mode,
                    False,
                    workspace,
                )
            for mode in ("gnu11", "gnu17"):
                compile_source(
                    sdcc,
                    types_compatible_source,
                    port,
                    mode,
                    True,
                    workspace,
                )
                compile_source(
                    sdcc,
                    invalid_types_compatible_source,
                    port,
                    mode,
                    False,
                    workspace,
                )
            for mode in ("c11", "c17", None):
                compile_source(
                    sdcc,
                    types_compatible_source,
                    port,
                    mode,
                    False,
                    workspace,
                )
            for mode in ("gnu11", "gnu17"):
                compile_source(
                    sdcc,
                    builtin_expect_source,
                    port,
                    mode,
                    True,
                    workspace,
                )
                compile_source(
                    sdcc,
                    invalid_builtin_expect_source,
                    port,
                    mode,
                    False,
                    workspace,
                )
                check_expect_hints(
                    sdcc,
                    builtin_expect_source,
                    port,
                    mode,
                    workspace,
                )
            for mode in ("c11", "c17", None):
                compile_source(
                    sdcc,
                    builtin_expect_source,
                    port,
                    mode,
                    False,
                    workspace,
                )
            for mode in ("gnu11", "gnu17"):
                compile_source(
                    sdcc,
                    builtin_constant_p_source,
                    port,
                    mode,
                    True,
                    workspace,
                )
                compile_source(
                    sdcc,
                    invalid_builtin_constant_p_source,
                    port,
                    mode,
                    False,
                    workspace,
                )
                check_constant_p_side_effects(
                    sdcc,
                    builtin_constant_p_source,
                    port,
                    mode,
                    workspace,
                )
            for mode in ("c11", "c17", None):
                compile_source(
                    sdcc,
                    builtin_constant_p_source,
                    port,
                    mode,
                    False,
                    workspace,
                )
            for mode in ("gnu11", "gnu17", "c11", "c17", None):
                compile_source(
                    sdcc,
                    has_builtin_source,
                    port,
                    mode,
                    True,
                    workspace,
                )
            for mode in ("gnu11", "gnu17"):
                compile_source(
                    sdcc,
                    bit_builtins_source,
                    port,
                    mode,
                    True,
                    workspace,
                )
                compile_source(
                    sdcc,
                    bit_builtins_source,
                    port,
                    mode,
                    True,
                    workspace,
                    ("--stack-auto",),
                )
                compile_source(
                    sdcc,
                    invalid_bit_builtins_source,
                    port,
                    mode,
                    False,
                    workspace,
                )
            for mode in ("c11", "c17", None):
                compile_source(
                    sdcc,
                    bit_builtins_source,
                    port,
                    mode,
                    False,
                    workspace,
                )
            for mode in ("gnu11", "gnu17"):
                compile_source(
                    sdcc,
                    byte_swap_builtins_source,
                    port,
                    mode,
                    True,
                    workspace,
                )
                compile_source(
                    sdcc,
                    byte_swap_builtins_source,
                    port,
                    mode,
                    True,
                    workspace,
                    ("--stack-auto",),
                )
                for invalid_case in range(1, 4):
                    compile_source(
                        sdcc,
                        invalid_byte_swap_builtins_source,
                        port,
                        mode,
                        False,
                        workspace,
                        (
                            "-DGNU_BYTE_SWAP_INVALID_CASE="
                            f"{invalid_case}",
                        ),
                    )
            for mode in ("c11", "c17", None):
                compile_source(
                    sdcc,
                    byte_swap_builtins_source,
                    port,
                    mode,
                    False,
                    workspace,
                )
            for mode in ("gnu11", "gnu17"):
                compile_source(
                    sdcc,
                    overflow_builtins_source,
                    port,
                    mode,
                    True,
                    workspace,
                )
                compile_source(
                    sdcc,
                    overflow_builtins_source,
                    port,
                    mode,
                    True,
                    workspace,
                    ("--stack-auto",),
                )
                for invalid_case in range(1, 10):
                    compile_source(
                        sdcc,
                        invalid_overflow_builtins_source,
                        port,
                        mode,
                        False,
                        workspace,
                        (
                            "-DGNU_OVERFLOW_INVALID_CASE="
                            f"{invalid_case}",
                        ),
                    )
            for mode in ("c11", "c17", None):
                compile_source(
                    sdcc,
                    overflow_builtins_source,
                    port,
                    mode,
                    False,
                    workspace,
                )
            for mode in ("gnu11", "gnu17"):
                compile_source(
                    sdcc,
                    auto_type_source,
                    port,
                    mode,
                    True,
                    workspace,
                )
                compile_source(
                    sdcc,
                    auto_type_source,
                    port,
                    mode,
                    True,
                    workspace,
                    ("--stack-auto",),
                )
                compile_source(
                    sdcc,
                    invalid_auto_type_source,
                    port,
                    mode,
                    False,
                    workspace,
                )
                compile_source(
                    sdcc,
                    multiple_auto_type_source,
                    port,
                    mode,
                    False,
                    workspace,
                )
                compile_source(
                    sdcc,
                    declarator_auto_type_source,
                    port,
                    mode,
                    False,
                    workspace,
                )
            for mode in ("c11", "c17", None):
                compile_source(
                    sdcc,
                    auto_type_source,
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
