#!/bin/sh

set -eu

test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
missing=0

if test "$(wc -l < "$test_dir/instruction-families.txt" | tr -d ' ')" -ne 65; then
    echo "official instruction-family manifest must contain 65 entries" >&2
    exit 1
fi

matrix_families=$(awk -F '\t' 'NR > 1 { print $2 }' \
    "$test_dir/instruction-forms.tsv" | sort -u)
manifest_families=$(sort -u "$test_dir/instruction-families.txt")
if test "$matrix_families" != "$manifest_families"; then
    echo "instruction-form matrix and family manifest differ" >&2
    echo "matrix families:" >&2
    printf '%s\n' "$matrix_families" >&2
    echo "manifest families:" >&2
    printf '%s\n' "$manifest_families" >&2
    exit 1
fi

if ! "${PYTHON:-python3}" "$test_dir/generate-instruction-forms.py" | \
        cmp -s - "$test_dir/instruction-forms.tsv"; then
    echo "instruction-form matrix is stale; regenerate and review it" >&2
    exit 1
fi

implemented=$(sed -n \
    's/.*{[[:space:]]*NULL,[[:space:]]*"\([a-z][a-z0-9]*\)",[[:space:]]*S_.*$/\1/p' \
    "$test_dir/../mcs251pst.c" | sort -u)

while IFS= read -r mnemonic; do
    case "$mnemonic" in
        a|ab|cy|dptr|dpx|pc|r[0-9]*|spx|wr[0-9]*|dr[0-9]*)
            continue
            ;;
    esac
    if ! grep -qx "$mnemonic" "$test_dir/instruction-families.txt" &&
            ! grep -qx "$mnemonic" "$test_dir/instruction-aliases.txt"; then
        echo "implemented instruction missing from family/alias manifest: $mnemonic" >&2
        missing=1
    fi
done <<EOF
$implemented
EOF

if test "$missing" -ne 0; then
    exit 1
fi
