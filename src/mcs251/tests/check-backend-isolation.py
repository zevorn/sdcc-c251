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


def check_mcs251_source(source):
    text = source.read_text(encoding="utf-8")
    match = MCS51_SOURCE_INCLUDE.search(text)
    if match:
        raise RuntimeError(
            f"{source} directly includes MCS51 implementation source: "
            f"{match.group(0).strip()}"
        )

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


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--mcs251-source", required=True)
    parser.add_argument("--mcs251-peep-source", required=True)
    parser.add_argument("--mcs251-rtrack-source", required=True)
    parser.add_argument("--mcs51-source", required=True)
    parser.add_argument("--mcs51-peep-source", required=True)
    parser.add_argument("--mcs51-rtrack-source", required=True)
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

    for mcs51_source in (
        Path(args.mcs51_source),
        Path(args.mcs51_peep_source),
        Path(args.mcs51_rtrack_source),
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

    print("PASS: MCS251 owns its port, peephole, and rtrack sources")


if __name__ == "__main__":
    main()
