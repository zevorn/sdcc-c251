#!/bin/sh

set -eu

if test "$#" -ne 2; then
    echo "usage: $0 /path/to/sdas251 /path/to/sdld" >&2
    exit 2
fi

absolute_program()
{
    case $1 in
        /*) printf '%s\n' "$1" ;;
        *)
            program_dir=$(CDPATH= cd -- "$(dirname -- "$1")" && pwd)
            printf '%s/%s\n' "$program_dir" "$(basename -- "$1")"
            ;;
    esac
}

assembler=$(absolute_program "$1")
linker=$(absolute_program "$2")
test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
work_dir=$(mktemp -d "${TMPDIR:-/tmp}/sdas251-errors.XXXXXX")

cleanup()
{
    rm -rf "$work_dir"
}
trap cleanup 0 HUP INT TERM

for source in "$test_dir"/errors/*.asm; do
    name=$(basename -- "$source" .asm)
    if test "$name" = branch-range; then
        continue
    fi
    cp "$source" "$work_dir/$name.asm"
    if (cd "$work_dir" && "$assembler" -plosg "$name.asm") \
            >"$work_dir/$name.log" 2>&1; then
        echo "$name unexpectedly assembled successfully" >&2
        exit 1
    fi
    if ! grep -Eq '[0-9]+ error|Error|error' "$work_dir/$name.log"; then
        echo "$name failed without an assembler diagnostic" >&2
        cat "$work_dir/$name.log" >&2
        exit 1
    fi
done

cp "$test_dir"/errors/branch-range.asm "$work_dir/"
cp "$test_dir"/link/branch-range.lk "$work_dir/"
(
    cd "$work_dir"
    "$assembler" -plosg branch-range.asm
)
if (cd "$work_dir" && "$linker" -nf branch-range.lk) \
        >"$work_dir/branch-range.log" 2>&1; then
    echo "out-of-range relative branch unexpectedly linked successfully" >&2
    exit 1
fi
if ! grep -q 'Byte PCR relocation error' "$work_dir/branch-range.log"; then
    echo "relative branch failed without the expected diagnostic" >&2
    cat "$work_dir/branch-range.log" >&2
    exit 1
fi

cp "$test_dir"/link/cross-a.asm "$work_dir/"
cp "$test_dir"/link/cross-b.asm "$work_dir/"
cp "$test_dir"/link/cross-region.lk "$work_dir/"
(
    cd "$work_dir"
    "$assembler" -plosg cross-a.asm
    "$assembler" -plosg cross-b.asm
)
if (cd "$work_dir" && "$linker" -nf cross-region.lk) \
        >"$work_dir/cross-region.log" 2>&1; then
    echo "cross-region LCALL/LJMP unexpectedly linked successfully" >&2
    exit 1
fi
if ! grep -q '64K Region relocation error' "$work_dir/cross-region.log"; then
    echo "cross-region link failed without the expected diagnostic" >&2
    cat "$work_dir/cross-region.log" >&2
    exit 1
fi

cp "$test_dir"/link/page-a.asm "$work_dir/"
cp "$test_dir"/link/page-b.asm "$work_dir/"
cp "$test_dir"/link/cross-page.lk "$work_dir/"
(
    cd "$work_dir"
    "$assembler" -plosg page-a.asm
    "$assembler" -plosg page-b.asm
)
if (cd "$work_dir" && "$linker" -nf cross-page.lk) \
        >"$work_dir/cross-page.log" 2>&1; then
    echo "cross-page ACALL/AJMP unexpectedly linked successfully" >&2
    exit 1
fi
if ! grep -q '2K Page relocation error' "$work_dir/cross-page.log"; then
    echo "cross-page link failed without the expected diagnostic" >&2
    cat "$work_dir/cross-page.log" >&2
    exit 1
fi
