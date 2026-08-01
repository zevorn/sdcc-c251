#!/usr/bin/env python3

import argparse
from pathlib import Path
import re
import subprocess
import tempfile


COMMON_MACROS = {
    "__STDC_HOSTED__": "0",
    "__CHAR_BIT__": "8",
    "__SIZEOF_CHAR__": "1",
    "__SIZEOF_SHORT__": "2",
    "__SIZEOF_INT__": "2",
    "__SIZEOF_LONG__": "4",
    "__SIZEOF_LONG_LONG__": "8",
    "__SIZEOF_FLOAT__": "4",
    "__SIZEOF_DOUBLE__": "4",
    "__SIZEOF_LONG_DOUBLE__": "4",
    "__SIZEOF_POINTER__": "3",
    "__POINTER_WIDTH__": "24",
    "__SCHAR_MAX__": "127",
    "__SHRT_MAX__": "32767",
    "__INT_MAX__": "32767",
    "__LONG_MAX__": "2147483647L",
    "__LONG_LONG_MAX__": "9223372036854775807LL",
    "__SCHAR_WIDTH__": "8",
    "__SHRT_WIDTH__": "16",
    "__INT_WIDTH__": "16",
    "__LONG_WIDTH__": "32",
    "__LLONG_WIDTH__": "64",
    "__INT8_TYPE__": "signed char",
    "__UINT8_TYPE__": "unsigned char",
    "__INT16_TYPE__": "short int",
    "__UINT16_TYPE__": "unsigned short int",
    "__INT32_TYPE__": "long int",
    "__UINT32_TYPE__": "unsigned long int",
    "__INT64_TYPE__": "long long int",
    "__UINT64_TYPE__": "unsigned long long int",
    "__INT8_C(c)": "c",
    "__INT16_C(c)": "c",
    "__INT32_C(c)": "c ## L",
    "__INT64_C(c)": "c ## LL",
    "__UINT8_C(c)": "c ## U",
    "__UINT16_C(c)": "c ## U",
    "__UINT32_C(c)": "c ## UL",
    "__UINT64_C(c)": "c ## ULL",
    "__INTMAX_C(c)": "c ## LL",
    "__UINTMAX_C(c)": "c ## ULL",
    "__INT_FAST8_TYPE__": "signed char",
    "__UINT_FAST8_TYPE__": "unsigned char",
    "__INT_FAST16_TYPE__": "int",
    "__UINT_FAST16_TYPE__": "unsigned int",
    "__INT_FAST32_TYPE__": "long int",
    "__UINT_FAST32_TYPE__": "unsigned long int",
    "__INT_FAST64_TYPE__": "long long int",
    "__UINT_FAST64_TYPE__": "unsigned long long int",
    "__INT_LEAST8_TYPE__": "signed char",
    "__UINT_LEAST8_TYPE__": "unsigned char",
    "__INT_LEAST16_TYPE__": "short int",
    "__UINT_LEAST16_TYPE__": "unsigned short int",
    "__INT_LEAST32_TYPE__": "long int",
    "__UINT_LEAST32_TYPE__": "unsigned long int",
    "__INT_LEAST64_TYPE__": "long long int",
    "__UINT_LEAST64_TYPE__": "unsigned long long int",
    "__INT8_MAX__": "127",
    "__UINT8_MAX__": "255",
    "__INT16_MAX__": "32767",
    "__UINT16_MAX__": "65535U",
    "__INT32_MAX__": "2147483647L",
    "__UINT32_MAX__": "4294967295UL",
    "__INT64_MAX__": "9223372036854775807LL",
    "__UINT64_MAX__": "18446744073709551615ULL",
    "__INT_FAST8_MAX__": "127",
    "__UINT_FAST8_MAX__": "255",
    "__INT_FAST16_MAX__": "32767",
    "__UINT_FAST16_MAX__": "65535U",
    "__INT_FAST32_MAX__": "2147483647L",
    "__UINT_FAST32_MAX__": "4294967295UL",
    "__INT_FAST64_MAX__": "9223372036854775807LL",
    "__UINT_FAST64_MAX__": "18446744073709551615ULL",
    "__INT_LEAST8_MAX__": "127",
    "__UINT_LEAST8_MAX__": "255",
    "__INT_LEAST16_MAX__": "32767",
    "__UINT_LEAST16_MAX__": "65535U",
    "__INT_LEAST32_MAX__": "2147483647L",
    "__UINT_LEAST32_MAX__": "4294967295UL",
    "__INT_LEAST64_MAX__": "9223372036854775807LL",
    "__UINT_LEAST64_MAX__": "18446744073709551615ULL",
    "__INT_FAST8_WIDTH__": "8",
    "__INT_FAST16_WIDTH__": "16",
    "__INT_FAST32_WIDTH__": "32",
    "__INT_FAST64_WIDTH__": "64",
    "__INT_LEAST8_WIDTH__": "8",
    "__INT_LEAST16_WIDTH__": "16",
    "__INT_LEAST32_WIDTH__": "32",
    "__INT_LEAST64_WIDTH__": "64",
    "__INTMAX_TYPE__": "long long int",
    "__UINTMAX_TYPE__": "unsigned long long int",
    "__INTMAX_MAX__": "9223372036854775807LL",
    "__UINTMAX_MAX__": "18446744073709551615ULL",
    "__INTMAX_WIDTH__": "64",
    "__UINTMAX_WIDTH__": "64",
    "__SIZEOF_INTMAX__": "8",
    "__SIZEOF_UINTMAX__": "8",
    "__INTPTR_TYPE__": "long int",
    "__UINTPTR_TYPE__": "unsigned long int",
    "__INTPTR_MAX__": "2147483647L",
    "__UINTPTR_MAX__": "4294967295UL",
    "__INTPTR_WIDTH__": "32",
    "__PTRDIFF_TYPE__": "long int",
    "__PTRDIFF_MAX__": "2147483647L",
    "__PTRDIFF_WIDTH__": "32",
    "__SIZEOF_PTRDIFF_T__": "4",
    "__SIZE_TYPE__": "unsigned int",
    "__SIZE_MAX__": "65535U",
    "__SIZE_WIDTH__": "16",
    "__SIZEOF_SIZE_T__": "2",
    "__WCHAR_TYPE__": "unsigned long int",
    "__WCHAR_MAX__": "4294967295UL",
    "__WCHAR_WIDTH__": "32",
    "__SIZEOF_WCHAR_T__": "4",
    "__WINT_TYPE__": "unsigned long int",
    "__WINT_MAX__": "4294967295UL",
    "__WINT_WIDTH__": "32",
    "__SIZEOF_WINT_T__": "4",
    "__ORDER_LITTLE_ENDIAN__": "1234",
    "__ORDER_BIG_ENDIAN__": "4321",
    "__ORDER_PDP_ENDIAN__": "3412",
}

PORT_MACROS = {
    "mcs51": {
        "__BYTE_ORDER__": "__ORDER_LITTLE_ENDIAN__",
        "__STDC_ENDIAN_NATIVE__": "__STDC_ENDIAN_LITTLE__",
    },
    "mcs251": {
        "__BYTE_ORDER__": "__ORDER_BIG_ENDIAN__",
        "__STDC_ENDIAN_NATIVE__": "__STDC_ENDIAN_BIG__",
    },
}

HOST_MACROS = (
    "__GNUC__",
    "__clang__",
    "__llvm__",
    "__APPLE__",
    "__MACH__",
    "__linux__",
    "__unix__",
    "_WIN32",
    "_WIN64",
    "__aarch64__",
    "__arm64__",
    "__x86_64__",
    "__i386__",
    "__ARM_ARCH",
    "__LP64__",
    "_LP64",
    "__GCC_ATOMIC_INT_LOCK_FREE",
    "__GCC_HAVE_DWARF2_CFI_ASM",
    "__CLANG_ATOMIC_INT_LOCK_FREE",
)


def read_macros(sdcc, source, port, mode, options=()):
    mode_name = mode if mode else "default"
    command = [str(sdcc), f"-m{port}"]
    if mode:
        command.append(f"--std={mode}")
    command.extend(options)
    command.extend(["-E", "-dM", str(source)])
    result = subprocess.run(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
    )
    if result.returncode:
        raise RuntimeError(
            f"{port} --std={mode_name} could not dump macros:\n"
            f"{result.stdout}{result.stderr}"
        )
    if "-Wbuiltin-macro-redefined" in result.stderr:
        raise RuntimeError(
            f"{port} --std={mode_name} exposed host-macro reset warnings:\n"
            f"{result.stderr}"
        )

    macros = {}
    for line in result.stdout.splitlines():
        match = re.fullmatch(r"#define\s+(\S+)(?:\s+(.*))?", line)
        if match:
            macros[match.group(1)] = match.group(2) or ""
    return macros


def resolve_macro(macros, name):
    value = macros.get(name)
    seen = {name}
    while value and re.fullmatch(r"__[A-Z0-9_]+__", value):
        if value in seen or value not in macros:
            break
        seen.add(value)
        value = macros[value]
    return value


def normalize_macro(value):
    if value is None:
        return None
    value = re.sub(r"\s*##\s*", "##", value)
    return " ".join(value.split())


def check_macros(sdcc, source, port, mode):
    mode_name = mode if mode else "default"
    macros = read_macros(sdcc, source, port, mode)
    expected = COMMON_MACROS | {
        "__BYTE_ORDER__": PORT_MACROS[port]["__BYTE_ORDER__"],
    }

    failures = []
    for name, value in expected.items():
        actual = macros.get(name)
        if (
            normalize_macro(actual) != normalize_macro(value)
            and normalize_macro(resolve_macro(macros, name))
            != normalize_macro(value)
        ):
            failures.append(f"{name}: expected {value!r}, got {actual!r}")
    for name in HOST_MACROS:
        if name in macros:
            failures.append(f"{name}: leaked host definition {macros[name]!r}")
    if failures:
        details = "\n".join(f"  {failure}" for failure in failures)
        raise RuntimeError(
            f"{port} --std={mode_name} target macros are invalid:\n"
            f"{details}"
        )

    print(f"PASS: {port} --std={mode_name} target macros match its ABI")


def check_char_signedness(sdcc, source, port):
    default_macros = read_macros(sdcc, source, port, "gnu17")
    signed_macros = read_macros(
        sdcc, source, port, "gnu17", ("--fsigned-char",)
    )
    if default_macros.get("__CHAR_UNSIGNED__") != "1":
        raise RuntimeError(f"{port} does not advertise its unsigned char ABI")
    if "__CHAR_UNSIGNED__" in signed_macros:
        raise RuntimeError(f"{port} advertises unsigned char in signed-char mode")
    print(f"PASS: {port} char signedness macros follow the selected ABI")


def check_macro_semantics(sdcc, source, port):
    expected_byte_order = PORT_MACROS[port]["__BYTE_ORDER__"]
    expected_stdc_endian = PORT_MACROS[port]["__STDC_ENDIAN_NATIVE__"]
    source.write_text(
        "\n".join(
            (
                "#include <stdbit.h>",
                "_Static_assert(sizeof(__INT8_TYPE__) == 1, \"int8\");",
                "_Static_assert(sizeof(__INT16_TYPE__) == 2, \"int16\");",
                "_Static_assert(sizeof(__INT32_TYPE__) == 4, \"int32\");",
                "_Static_assert(sizeof(__INT64_TYPE__) == 8, \"int64\");",
                "_Static_assert(sizeof(__INTPTR_TYPE__) == 4, \"intptr\");",
                "_Static_assert(sizeof(__SIZE_TYPE__) == 2, \"size_t\");",
                "_Static_assert(__INT32_C(1) == 1L, \"int32 constant\");",
                "_Static_assert(__UINT64_C(1) == 1ULL, \"uint64 constant\");",
                f"_Static_assert(__BYTE_ORDER__ == {expected_byte_order}, "
                '"byte order");',
                f"_Static_assert(__STDC_ENDIAN_NATIVE__ == "
                f"{expected_stdc_endian}, \"stdbit byte order\");",
                "",
            )
        )
    )
    command = [
        str(sdcc),
        f"-m{port}",
        "--std=gnu17",
        "--Werror",
        "--syntax-only",
        str(source),
    ]
    result = subprocess.run(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=False,
    )
    if result.returncode:
        raise RuntimeError(
            f"{port} cannot use its target macros as C expressions:\n"
            f"{result.stdout}{result.stderr}"
        )
    print(f"PASS: {port} target type and constant macros compile")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sdcc", required=True)
    args = parser.parse_args()

    sdcc = Path(args.sdcc).resolve()
    if not sdcc.is_file():
        parser.error(f"compiler does not exist: {sdcc}")

    with tempfile.TemporaryDirectory(prefix="sdcc-target-macros.") as tmp:
        source = Path(tmp) / "empty.c"
        source.write_text("\n")
        semantic_source = Path(tmp) / "target-macro-semantics.c"
        for port in PORT_MACROS:
            for mode in (None, "gnu17"):
                check_macros(sdcc, source, port, mode)
            check_char_signedness(sdcc, source, port)
            check_macro_semantics(sdcc, semantic_source, port)


if __name__ == "__main__":
    main()
