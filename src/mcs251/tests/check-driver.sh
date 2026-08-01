#!/bin/sh
set -eu

if test "$#" -lt 4; then
    echo "usage: $0 SDCC SMOKE_SOURCE DEVICE_INCLUDE RUNTIME_SOURCE..." >&2
    exit 2
fi

sdcc=$1
smoke_source=$2
device_include=$3
shift 3
test_dir=${TMPDIR:-/tmp}/sdcc-mcs251-driver.$$
trap 'rm -rf "$test_dir"' 0 HUP INT TERM
mkdir -p "$test_dir"

"$sdcc" -mmcs251 -I"$device_include" -S \
    -o "$test_dir/compiler-smoke.asm" "$smoke_source"
test -s "$test_dir/compiler-smoke.asm"
grep -q 'optsdcc -mmcs251' "$test_dir/compiler-smoke.asm"

"$sdcc" -mmcs251 -I"$device_include" -c \
    -o "$test_dir/compiler-smoke.rel" "$smoke_source"
test -s "$test_dir/compiler-smoke.rel"

runtime_index=0
for runtime_source in "$@"; do
    for configuration in small small-stack-auto large large-stack-auto; do
        model=${configuration%%-*}
        stack_flag=
        case $configuration in
            *-stack-auto) stack_flag=--stack-auto ;;
        esac
        runtime_output=$test_dir/runtime-$runtime_index-$configuration.rel
        runtime_asm=$test_dir/runtime-$runtime_index-$configuration.asm
        "$sdcc" -mmcs251 --model-$model $stack_flag \
            -I"$device_include" -I"$device_include/mcs51" -S \
            -o "$runtime_asm" "$runtime_source"
        if grep -Eq '^[[:space:]]*(lcall|ret)([[:space:];]|$)' "$runtime_asm"; then
            echo "MCS251 runtime emitted a near call or return: $runtime_source ($configuration)" >&2
            exit 1
        fi
        case $runtime_source in
            */_setjmp.c|_setjmp.c)
                grep -q '^___mcs251_longjmp_restore:' "$runtime_asm"
                grep -q 'mov[[:space:]][[:space:]]*spx,dpx' "$runtime_asm"
                ;;
        esac
        "$sdcc" -mmcs251 --model-$model $stack_flag \
            -I"$device_include" -I"$device_include/mcs51" -c \
            -o "$runtime_output" "$runtime_source"
        test -s "$runtime_output"
    done
    runtime_index=$((runtime_index + 1))
done
