#!/usr/bin/env python3

import argparse
from pathlib import Path
import re


MCS51_SOURCE_INCLUDE = re.compile(
    r'^\s*#\s*include\s*["<][^">]*mcs51/[^">]*\.c[">]',
    re.MULTILINE,
)
TRANSITIONAL_TARGET_SELECTORS = (
    "MCS251_PORT",
    "TARGET_IS_MCS251",
)
COMPILE_TIME_POLICY_LITERAL = re.compile(
    r"\bif\s*\(\s*!?(?:true|false)\b"
    r"|\b(?:true|false)\s*(?:&&|\|\||\?)"
    r"|!(?:true|false)\b"
)
MCS251_PEEP_EXPORTS = (
    "mcs251DeadMove",
    "mcs251notUsed",
    "mcs251notUsedFrom",
    "mcs251CanAssign",
)
MCS251_RTRACK_EXPORTS = (
    "_mcs251_rtrackUpdate",
    "mcs251_rtrackGetLit",
    "mcs251_rtrackMoveALit",
    "mcs251_rtrackLoadDptrWithSym",
)
MCS251_GEN_EXPORTS = (
    "mcs251_genCode",
    "mcs251IsParmInCall",
    "mcs251IsRegArg",
    "mcs251IsReturned",
    "mcs251_emitDebuggerSymbol",
    "mcs251_init_asmops",
)
MCS251_RALLOC_EXPORTS = (
    "mcs251_assignRegisters",
    "mcs251_rUmaskForOp",
    "mcs251_regWithIdx",
    "mcs251_regname_to_idx",
)


def read_owned_source(source):
    text = source.read_text(encoding="utf-8")
    match = MCS51_SOURCE_INCLUDE.search(text)
    if match:
        raise RuntimeError(
            f"{source} directly includes MCS51 implementation source: "
            f"{match.group(0).strip()}"
        )

    return text


def check_mcs251_source(source):
    text = read_owned_source(source)

    selectors = [
        selector
        for selector in TRANSITIONAL_TARGET_SELECTORS
        if re.search(rf"\b{selector}\b", text)
    ]
    if selectors:
        raise RuntimeError(
            f"{source} still uses transitional target selectors: "
            f"{', '.join(selectors)}"
        )

    return text


def check_bound_policy(source, text):
    match = COMPILE_TIME_POLICY_LITERAL.search(text)
    if match:
        line = text.count("\n", 0, match.start()) + 1
        raise RuntimeError(
            f"{source}:{line} still contains a compile-time target-policy "
            "literal"
        )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--mcs251-source", required=True)
    parser.add_argument("--mcs251-peep-source", required=True)
    parser.add_argument("--mcs251-rtrack-source", required=True)
    parser.add_argument("--mcs251-gen-source", required=True)
    parser.add_argument("--mcs251-gen-lower-source", default=None)
    parser.add_argument("--mcs251-ralloc-source", required=True)
    parser.add_argument("--mcs51-source", required=True)
    parser.add_argument("--mcs51-peep-source", required=True)
    parser.add_argument("--mcs51-rtrack-source", required=True)
    parser.add_argument("--mcs51-gen-source", required=True)
    parser.add_argument("--mcs51-ralloc-source", required=True)
    args = parser.parse_args()

    source = Path(args.mcs251_source)
    text = check_mcs251_source(source)

    peep_source = Path(args.mcs251_peep_source)
    peep_text = check_mcs251_source(peep_source)
    missing_exports = [
        symbol
        for symbol in MCS251_PEEP_EXPORTS
        if not re.search(rf"\b{symbol}\s*\(", peep_text)
    ]
    if missing_exports:
        raise RuntimeError(
            f"{peep_source} does not define MCS251 peephole helpers: "
            f"{', '.join(missing_exports)}"
        )

    missing_callbacks = [
        symbol for symbol in MCS251_PEEP_EXPORTS if symbol not in text
    ]
    if missing_callbacks:
        raise RuntimeError(
            f"{source} does not use MCS251 peephole helpers: "
            f"{', '.join(missing_callbacks)}"
        )

    rtrack_source = Path(args.mcs251_rtrack_source)
    rtrack_text = check_mcs251_source(rtrack_source)
    missing_exports = [
        symbol
        for symbol in MCS251_RTRACK_EXPORTS
        if not re.search(rf"\b{symbol}\s*\(", rtrack_text)
    ]
    if missing_exports:
        raise RuntimeError(
            f"{rtrack_source} does not define MCS251 register tracking: "
            f"{', '.join(missing_exports)}"
        )

    if "_mcs251_rtrackUpdate" not in text:
        raise RuntimeError(
            f"{source} does not use the MCS251 register tracking callback"
        )

    gen_source = Path(args.mcs251_gen_source)
    gen_text = check_mcs251_source(gen_source)
    if args.mcs251_gen_lower_source:
        gen_lower_source = Path(args.mcs251_gen_lower_source)
        gen_lower_text = gen_lower_source.read_text(encoding="utf-8")
        gen_text += "\n" + gen_lower_text
    check_bound_policy(gen_source, gen_text)
    missing_exports = [
        symbol
        for symbol in MCS251_GEN_EXPORTS
        if not re.search(rf"\b{symbol}\s*\(", gen_text)
    ]
    if missing_exports:
        raise RuntimeError(
            f"{gen_source} does not define MCS251 code generation: "
            f"{', '.join(missing_exports)}"
        )

    ralloc_source = Path(args.mcs251_ralloc_source)
    ralloc_text = check_mcs251_source(ralloc_source)
    check_bound_policy(ralloc_source, ralloc_text)
    missing_exports = [
        symbol
        for symbol in MCS251_RALLOC_EXPORTS
        if not re.search(rf"\b{symbol}\s*\(", ralloc_text)
    ]
    if missing_exports:
        raise RuntimeError(
            f"{ralloc_source} does not define MCS251 allocation: "
            f"{', '.join(missing_exports)}"
        )

    mcs51_gen_source = Path(args.mcs51_gen_source)
    mcs51_ralloc_source = Path(args.mcs51_ralloc_source)
    for mcs51_source in (
        Path(args.mcs51_source),
        Path(args.mcs51_peep_source),
        Path(args.mcs51_rtrack_source),
        mcs51_gen_source,
        mcs51_ralloc_source,
    ):
        mcs51_text = mcs51_source.read_text(encoding="utf-8")
        selectors = [
            selector
            for selector in TRANSITIONAL_TARGET_SELECTORS
            if re.search(rf"\b{selector}\b", mcs51_text)
        ]
        if selectors:
            raise RuntimeError(
                f"{mcs51_source} still contains MCS251 selectors: "
                f"{', '.join(selectors)}"
            )

    check_bound_policy(mcs51_gen_source, mcs51_gen_source.read_text(
        encoding="utf-8"))
    check_bound_policy(mcs51_ralloc_source, mcs51_ralloc_source.read_text(
        encoding="utf-8"))

    print("PASS: MCS251 owns all backend implementation sources")


if __name__ == "__main__":
    main()
