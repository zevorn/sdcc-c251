#!/usr/bin/env python3

import argparse
from pathlib import Path
import re


MCS51_SOURCE_INCLUDE = re.compile(
    r'^\s*#\s*include\s*["<][^">]*mcs51/[^">]*\.c[">]',
    re.MULTILINE,
)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--mcs251-source", required=True)
    args = parser.parse_args()

    source = Path(args.mcs251_source)
    text = source.read_text(encoding="utf-8")
    match = MCS51_SOURCE_INCLUDE.search(text)
    if match:
        raise RuntimeError(
            f"{source} directly includes MCS51 implementation source: "
            f"{match.group(0).strip()}"
        )

    print("PASS: MCS251 owns its port implementation source")


if __name__ == "__main__":
    main()
