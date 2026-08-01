#!/bin/sh

set -eu

if test "$#" -ne 1; then
    echo "usage: $0 /path/to/sdas251" >&2
    exit 2
fi

assembler=$1
case $assembler in
    /*) ;;
    *)
        assembler_dir=$(CDPATH= cd -- "$(dirname -- "$assembler")" && pwd)
        assembler=$assembler_dir/$(basename -- "$assembler")
        ;;
esac
test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
work_dir=$(mktemp -d "${TMPDIR:-/tmp}/sdas251-source-smoke.XXXXXX")

cleanup()
{
    rm -rf "$work_dir"
}
trap cleanup 0 HUP INT TERM

for expected_file in "$test_dir"/*.expected; do
    case_name=$(basename -- "$expected_file" .expected)
    cp "$test_dir/$case_name.asm" "$work_dir/$case_name.asm"
    (
        cd "$work_dir"
        "$assembler" -plosg "$case_name.asm"
    )

    perl -ne '
        if (/^\s*[0-9A-Fa-f]{4,8} ([0-9A-Fa-f]{2}(?: [0-9A-Fa-f]{2})*)(?=\s{2,})/) {
            $bytes = $1;
            $bytes =~ s/\s+/ /g;
            $bytes =~ s/ $//;
            print lc($bytes), "\n";
        }
    ' "$work_dir/$case_name.lst" \
        | tr '[:upper:]' '[:lower:]' \
        | tr '\n' ' ' \
        | tr -s ' ' \
        | sed 's/^ //; s/ $//' > "$work_dir/$case_name.actual"

    expected=$(sed 's/[[:space:]]*$//' "$expected_file")
    actual=$(sed 's/[[:space:]]*$//' "$work_dir/$case_name.actual")

    if test "$actual" != "$expected"; then
        echo "$case_name encoding mismatch" >&2
        echo "expected: $expected" >&2
        echo "actual:   $actual" >&2
        exit 1
    fi
done
