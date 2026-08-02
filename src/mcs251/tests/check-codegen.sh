#!/bin/sh
set -eu

if test "$#" -ne 10; then
    echo "usage: $0 SDCC OPTIMIZATION_SOURCE CALL_SOURCE INTERRUPT_SOURCE INITIALIZER_SOURCE MEMORY_MODEL_SOURCE BSEARCH_SOURCE MULLONGLONG_SOURCE DIVUINT_SOURCE DEVICE_INCLUDE" >&2
    exit 2
fi

sdcc=$1
optimization_source=$2
call_source=$3
interrupt_source=$4
initializer_source=$5
memory_model_source=$6
bsearch_source=$7
mullonglong_source=$8
divuint_source=$9
device_include=${10}
test_dir=${TMPDIR:-/tmp}/sdcc-mcs251-codegen.$$
trap 'rm -rf "$test_dir"' 0 HUP INT TERM
mkdir -p "$test_dir"

"$sdcc" -mmcs251 -S -o "$test_dir/native-optimization.asm" \
    "$optimization_source"
"$sdcc" -mmcs251 --stack-auto -S \
    -o "$test_dir/native-optimization-stack-auto.asm" "$optimization_source"
"$sdcc" -mmcs251 --model-large -S \
    -o "$test_dir/native-optimization-large.asm" "$optimization_source"
"$sdcc" -mmcs251 --model-large -S -I"$device_include" \
    -o "$test_dir/bsearch-large.asm" "$bsearch_source"
"$sdcc" -mmcs251 -S -o "$test_dir/call-24bit.asm" "$call_source"
"$sdcc" -mmcs251 --model-large -S \
    -o "$test_dir/call-24bit-large.asm" "$call_source"
"$sdcc" -mmcs251 -S -o "$test_dir/interrupt-vector.asm" "$interrupt_source"
"$sdcc" -mmcs251 --model-small --no-xinit-opt -S \
    -o "$test_dir/aggregate-initializer-mcs251.asm" "$initializer_source"
"$sdcc" -mmcs251 --model-small --no-xinit-opt -c \
    -o "$test_dir/aggregate-initializer-mcs251.rel" "$initializer_source"
"$sdcc" -mmcs51 --model-small --no-xinit-opt -S \
    -o "$test_dir/aggregate-initializer-mcs51.asm" "$initializer_source"
"$sdcc" -mmcs251 --model-small -S \
    -o "$test_dir/memory-model-small.asm" "$memory_model_source"
"$sdcc" -mmcs251 --model-large -S \
    -o "$test_dir/memory-model-large.asm" "$memory_model_source"
"$sdcc" -mmcs251 --stack-auto -I"$device_include" \
    -I"$device_include/mcs51" -S -o "$test_dir/mullonglong.asm" \
    "$mullonglong_source"
"$sdcc" -mmcs251 --stack-auto -I"$device_include" \
    -I"$device_include/mcs51" -S -o "$test_dir/divuint.asm" \
    "$divuint_source"

# Indexed MCS251 stack operands are loaded through A.  An in-place increment
# must store the incremented byte back before the following comparison.
# A loop-counter increment must update either the indexed stack slot or a
# native fixed byte register when it remains allocated in R8-R15.
if ! awk '
    previous ~ /^[[:space:]]*inc[[:space:]]+a[[:space:]]*$/ &&
        $0 ~ /^[[:space:]]*mov[[:space:]]+@spx[-+][^,]*,[[:space:]]*a[[:space:]]*$/ {
        found = 1
    }
    { previous = $0 }
    END { exit !found }
' "$test_dir/mullonglong.asm" &&
        ! grep -Eq '^[[:space:]]*inc[[:space:]]+r(8|9|1[0-5])[[:space:]]*$' \
            "$test_dir/mullonglong.asm"; then
    echo "MCS251 increment lost its register or stack write-back" >&2
    exit 1
fi

# A decrement must update either the indexed stack slot or a native fixed
# byte register when register allocation keeps the loop counter in R8-R15.
if ! awk '
    previous ~ /^[[:space:]]*dec[[:space:]]+a[[:space:]]*$/ &&
        $0 ~ /^[[:space:]]*mov[[:space:]]+@spx[-+][^,]*,[[:space:]]*a[[:space:]]*$/ {
        found = 1
    }
    { previous = $0 }
    END { exit !found }
' "$test_dir/divuint.asm" &&
        ! grep -Eq '^[[:space:]]*dec[[:space:]]+r(8|9|1[0-5])[[:space:]]*$' \
            "$test_dir/divuint.asm"; then
    echo "MCS251 decrement lost its register or stack write-back" >&2
    exit 1
fi

grep -Eq '^[[:space:]]*inc[[:space:]]+a,#[^;]*(2|0x02)$' \
    "$test_dir/native-optimization.asm"
grep -Eq '^[[:space:]]*dec[[:space:]]+a,#[^;]*(2|0x02)$' \
    "$test_dir/native-optimization.asm"
grep -Eq '^[[:space:]]*mov[[:space:]]+r7,[[:space:]]*dpl$' \
    "$test_dir/native-optimization.asm"
grep -Eq '^[[:space:]]*mov[[:space:]]+r6,[[:space:]]*dph$' \
    "$test_dir/native-optimization.asm"
grep -Eq '^[[:space:]]*anl[[:space:]]+wr6,[[:space:]]*_and16_PARM_2$' \
    "$test_dir/native-optimization.asm"
grep -Eq '^[[:space:]]*orl[[:space:]]+wr6,[[:space:]]*_or16_PARM_2$' \
    "$test_dir/native-optimization.asm"
grep -Eq '^[[:space:]]*xrl[[:space:]]+wr6,[[:space:]]*_xor16_PARM_2$' \
    "$test_dir/native-optimization.asm"
grep -Eq '^[[:space:]]*xrl[[:space:]]+wr4,[[:space:]]*_xor32_PARM_2$' \
    "$test_dir/native-optimization.asm"
grep -Eq '^[[:space:]]*xrl[[:space:]]+wr6,[[:space:]]*\(_xor32_PARM_2[[:space:]]*\+[[:space:]]*2\)$' \
    "$test_dir/native-optimization.asm"

sed -n '/^_and16_literal:/,/^[[:space:]]*\.area[[:space:]]/p' \
    "$test_dir/native-optimization.asm" > "$test_dir/and16-literal.asm"
sed -n '/^_or16_literal:/,/^[[:space:]]*\.area[[:space:]]/p' \
    "$test_dir/native-optimization.asm" > "$test_dir/or16-literal.asm"
sed -n '/^_xor16_literal:/,/^[[:space:]]*\.area[[:space:]]/p' \
    "$test_dir/native-optimization.asm" > "$test_dir/xor16-literal.asm"
sed -n '/^_less_than16_literal:/,/^[[:space:]]*\.area[[:space:]]/p' \
    "$test_dir/native-optimization.asm" > "$test_dir/less-than16-literal.asm"
grep -Eq '^[[:space:]]*anl[[:space:]]+wr6,[[:space:]]*#' \
    "$test_dir/and16-literal.asm"
grep -Eq '^[[:space:]]*orl[[:space:]]+wr6,[[:space:]]*#' \
    "$test_dir/or16-literal.asm"
grep -Eq '^[[:space:]]*xrl[[:space:]]+wr6,[[:space:]]*#' \
    "$test_dir/xor16-literal.asm"
grep -Eq '^[[:space:]]*cmp[[:space:]]+wr6,[[:space:]]*#' \
    "$test_dir/less-than16-literal.asm"

sed -n '/^_less_than16_literal:/,/^[[:space:]]*\.area[[:space:]]/p' \
    "$test_dir/native-optimization-stack-auto.asm" \
    > "$test_dir/less-than16-stack-auto.asm"
grep -Eq '^[[:space:]]*mov[[:space:]]+r7,[[:space:]]*dpl$' \
    "$test_dir/less-than16-stack-auto.asm"
grep -Eq '^[[:space:]]*mov[[:space:]]+r6,[[:space:]]*dph$' \
    "$test_dir/less-than16-stack-auto.asm"
grep -Eq '^[[:space:]]*cmp[[:space:]]+wr6,[[:space:]]*#' \
    "$test_dir/less-than16-stack-auto.asm"

# A MCS251 stack operand is loaded through A, so an in-place compound logical
# operation must explicitly write A back after ANL, ORL, and XRL.  The first
# store initializes local; the other three preserve the compound assignments.
sed -n '/^_stack_compound_literals:/,/^[[:space:]]*\.area[[:space:]]/p' \
    "$test_dir/native-optimization.asm" > "$test_dir/stack-compound.asm"
if test "$(grep -Ec '^[[:space:]]*mov[[:space:]]+@spx([^,]*)?,[[:space:]]*a$' \
        "$test_dir/stack-compound.asm")" -lt 4; then
    echo "MCS251 stack compound logical operation lost its write-back" >&2
    exit 1
fi

if grep -Eq '^[[:space:]]*addc[[:space:]]+a,[[:space:]]*@dpx$' \
        "$test_dir/native-optimization-large.asm"; then
    echo "MCS251 peephole emitted an unsupported ADDC @DPX operand" >&2
    exit 1
fi
if grep -Eiq '^[[:space:]]*(add|addc|subb)[[:space:]]+[^,]+,[[:space:]]*@spx' \
        "$test_dir"/*.asm; then
    echo "MCS251 peephole emitted an unsupported arithmetic @SPX operand" >&2
    exit 1
fi
if grep -Eiq '^[[:space:]]*mov[[:space:]]+b,[[:space:]]*@(dpx|spx)' \
        "$test_dir"/*.asm; then
    echo "MCS251 peephole emitted an unsupported native-pointer load into B" >&2
    exit 1
fi
grep -Eq '^[[:space:]]*mov[[:space:]]+a,[[:space:]]*@dpx$' \
    "$test_dir/native-optimization-large.asm"
grep -Eq '^[[:space:]]*addc[[:space:]]+a,[[:space:]]*#(0x)?0+$' \
    "$test_dir/native-optimization-large.asm"

# With a 32-bit size_t, bsearch multiplies two unsigned longs.  Capture all
# four __mullong return registers before materializing a far address in DPTR;
# loading the address first destroys DPL and DPH.
awk '
    /^[[:space:]]*ecall[[:space:]]+__mullong$/ {
        after_call = 1
        next
    }
    after_call && /^[[:space:]]*mov[[:space:]]+dptr,/ {
        checked = dpl && dph && b && a
        after_call = 0
    }
    after_call && /^[[:space:]]*mov[[:space:]]+r([0-9]|1[0-5]),[[:space:]]*dpl$/ {
        dpl = 1
    }
    after_call && /^[[:space:]]*mov[[:space:]]+r([0-9]|1[0-5]),[[:space:]]*dph$/ {
        dph = 1
    }
    after_call && /^[[:space:]]*mov[[:space:]]+r([0-9]|1[0-5]),[[:space:]]*b$/ {
        b = 1
    }
    after_call && /^[[:space:]]*mov[[:space:]]+r([0-9]|1[0-5]),[[:space:]]*a$/ {
        a = 1
    }
    END { exit !checked }
' "$test_dir/bsearch-large.asm" || {
    echo "MCS251 large-model store clobbered a long return byte" >&2
    exit 1
}

grep -Eq '^[[:space:]]*ecall[[:space:]]+_mcs251_callee$' \
    "$test_dir/call-24bit.asm"
grep -Eq '^[[:space:]]*ejmp[[:space:]]+_mcs251_stop$' \
    "$test_dir/call-24bit.asm"
grep -Eq '^[[:space:]]*ecall[[:space:]]+_mcs251_stop_with_stack_argument$' \
    "$test_dir/call-24bit.asm"
if grep -Eq '^[[:space:]]*ejmp[[:space:]]+_mcs251_stop_with_stack_argument$' \
        "$test_dir/call-24bit.asm"; then
    echo "MCS251 tail-jumped to a noreturn callee with stack arguments" >&2
    exit 1
fi
grep -Eq '^[[:space:]]*ecall[[:space:]]+@dr28$' \
    "$test_dir/call-24bit.asm"
grep -Eq '^[[:space:]]*eret$' "$test_dir/call-24bit.asm"
grep -Eq '^[[:space:]]*\.byte[[:space:]]+\(_mcs251_callee[[:space:]]*>>[[:space:]]*16\),[[:space:]]*\(_mcs251_callee[[:space:]]*>>[[:space:]]*8\),[[:space:]]*_mcs251_callee$' \
    "$test_dir/call-24bit.asm"

sed -n '/^_mcs251_forward_four_bytes:/,/^[[:space:]]*\.area[[:space:]]/p' \
    "$test_dir/call-24bit-large.asm" > "$test_dir/forward-four-bytes.asm"
grep -Eq '^[[:space:]]*mov[[:space:]]+dpl,[[:space:]]*r7$' \
    "$test_dir/forward-four-bytes.asm"
grep -Eq '^[[:space:]]*mov[[:space:]]+dph,[[:space:]]*r6$' \
    "$test_dir/forward-four-bytes.asm"
grep -Eq '^[[:space:]]*mov[[:space:]]+b,[[:space:]]*r5$' \
    "$test_dir/forward-four-bytes.asm"
grep -Eq '^[[:space:]]*mov[[:space:]]+a,[[:space:]]*r4$' \
    "$test_dir/forward-four-bytes.asm"
grep -Eq '^[[:space:]]*ejmp[[:space:]]+_mcs251_two_argument_callee$' \
    "$test_dir/forward-four-bytes.asm"

if grep -Eq '^[[:space:]]*(lcall|ljmp)[[:space:]]+_mcs251_(callee|stop)$' \
    "$test_dir/call-24bit.asm"; then
    echo "MCS251 code generator emitted a near direct call or jump" >&2
    exit 1
fi

grep -Eq '^[[:space:]]*ljmp[[:space:]]+__sdcc_mcs251_reset_trampoline$' \
    "$test_dir/interrupt-vector.asm"
grep -Eq '^[[:space:]]*ejmp[[:space:]]+__sdcc_gsinit_startup$' \
    "$test_dir/interrupt-vector.asm"
grep -Eq '^[[:space:]]*ejmp[[:space:]]+_timer0_interrupt$' \
    "$test_dir/interrupt-vector.asm"
if grep -Eq '^[[:space:]]*ljmp[[:space:]]+__sdcc_gsinit_startup$' \
        "$test_dir/interrupt-vector.asm"; then
    echo "MCS251 reset vector bypassed the full-address trampoline" >&2
    exit 1
fi
if grep -Eq '^[[:space:]]*ljmp[[:space:]]+_timer0_interrupt$' \
        "$test_dir/interrupt-vector.asm"; then
    echo "MCS251 interrupt vector emitted a region-limited jump" >&2
    exit 1
fi

# Anonymous-union members overlap in the front end's promoted field list.
# The MCS251-specific initializer path must consume one initializer for that
# storage and leave the following values for the tail structure.
grep -Eq '^.*\(_initializer_value[[:space:]]*\+[[:space:]]*0x0004\).*,#0x03$' \
    "$test_dir/aggregate-initializer-mcs251.asm"
grep -Eq '^.*\(_initializer_value[[:space:]]*\+[[:space:]]*0x0006\).*,#0x04$' \
    "$test_dir/aggregate-initializer-mcs251.asm"

# MCS251 far and generic pointers are flat 24-bit values even outside the
# DS390-only MODEL_FLAT24 mode.  Keep all three relocation bytes in const data.
grep -Eq '^[[:space:]]*\.byte[[:space:]]+\(_initializer_target[[:space:]]*>>[[:space:]]*16\),[[:space:]]*\(_initializer_target[[:space:]]*>>[[:space:]]*8\),[[:space:]]*_initializer_target$' \
    "$test_dir/aggregate-initializer-mcs251.asm"

# Each high byte of a flat MCS251 function address needs its own relocation.
# Combining both bytes with a peephole-time bitwise OR creates an expression
# that ASxxxx cannot relocate.
grep -Eq '^[[:space:]]*orl[[:space:]]+a,[[:space:]]*#\(_initialized_function_symbol_is_nonnull[[:space:]]*>>[[:space:]]*8\)$' \
    "$test_dir/aggregate-initializer-mcs251.asm"
grep -Eq '^[[:space:]]*orl[[:space:]]+a,[[:space:]]*#\(_initialized_function_symbol_is_nonnull[[:space:]]*>>[[:space:]]*16\)$' \
    "$test_dir/aggregate-initializer-mcs251.asm"
if grep -Eq '^[[:space:]]*orl[[:space:]]+a,[[:space:]]*#\(.*>>[[:space:]]*8.*\|.*>>[[:space:]]*16.*\)$' \
        "$test_dir/aggregate-initializer-mcs251.asm"; then
    echo "MCS251 peephole combined independent address-byte relocations" >&2
    exit 1
fi

# The MCS-51 generic-pointer layout remains two address bytes plus its
# address-space tag; it must not acquire a MCS251 address-high relocation.
grep -Eq '^[[:space:]]*\.byte[[:space:]]+_initializer_target,[[:space:]]*\(_initializer_target[[:space:]]*>>[[:space:]]*8\),[[:space:]]*#0x40$' \
    "$test_dir/aggregate-initializer-mcs51.asm"
if grep -Eq '^[[:space:]]*\.byte[[:space:]]+_initializer_target,[[:space:]]*\(_initializer_target[[:space:]]*>>[[:space:]]*8\),[[:space:]]*\(_initializer_target[[:space:]]*>>[[:space:]]*16\)$' \
        "$test_dir/aggregate-initializer-mcs51.asm"; then
    echo "MCS251 pointer-width change leaked into MCS-51 output" >&2
    exit 1
fi

# Static spill slots are non-register automatic objects.  They follow the
# selected MCS251 data model: direct/overlay DATA in small, XDATA in large.
sed -n '/^[[:space:]]*\.area[[:space:]][[:space:]]*OSEG/,/^[[:space:]]*\.area[[:space:]][[:space:]]*ISEG/p' \
    "$test_dir/memory-model-small.asm" > "$test_dir/memory-model-small-spills.asm"
grep -Eq '^_mcs251_spill_pressure_sloc[0-9]+_' \
    "$test_dir/memory-model-small-spills.asm"

sed -n '/^[[:space:]]*\.area[[:space:]][[:space:]]*XSEG/,/^[[:space:]]*\.area[[:space:]][[:space:]]*XABS/p' \
    "$test_dir/memory-model-large.asm" > "$test_dir/memory-model-large-spills.asm"
grep -Eq '^_mcs251_spill_pressure_sloc[0-9]+_' \
    "$test_dir/memory-model-large-spills.asm"
if sed -n '/^[[:space:]]*\.area[[:space:]][[:space:]]*DSEG/,/^[[:space:]]*\.area[[:space:]][[:space:]]*XSEG/p' \
        "$test_dir/memory-model-large.asm" |
        grep -Eq '^_mcs251_spill_pressure_sloc[0-9]+_'; then
    echo "MCS251 large-model spill slot was allocated in internal DATA" >&2
    exit 1
fi

# Reentrant MCS251 frames use the complete hardware SPX and indexed SPX
# addressing.  They must remain valid when recursive frames cross 0x00ff.
sed -n '/^_mcs251_spx_probe:/,/^[[:space:]]*\.area[[:space:]]/p' \
    "$test_dir/memory-model-large.asm" > "$test_dir/spx-probe.asm"
grep -Eq '^[[:space:]]*inc[[:space:]]+spx([[:space:]]*,|$)' \
    "$test_dir/spx-probe.asm"
grep -Eq '@spx[+-]' "$test_dir/spx-probe.asm"
grep -Eq '^[[:space:]]*mov[[:space:]]+a,[[:space:]]*@dpx$' \
    "$test_dir/spx-probe.asm"
grep -Eq '^[[:space:]]*sub[[:space:]]+dpx,[[:space:]]*#[^;]*(47|0x0*2[fF])$' \
    "$test_dir/spx-probe.asm"
grep -Eq '^[[:space:]]*dec[[:space:]]+spx([[:space:]]*,|$)' \
    "$test_dir/spx-probe.asm"
if grep -Eq '^[[:space:]]*add[[:space:]]+dpx,[[:space:]]*#-' \
        "$test_dir/spx-probe.asm"; then
    echo "MCS251 stack-address materialization zero-extended a negative offset" >&2
    exit 1
fi
if grep -Eq '(_bp|@r[01]|^[[:space:]]*mov[[:space:]]+(a,[[:space:]]*sp|sp,[[:space:]]*a)$)' \
        "$test_dir/spx-probe.asm"; then
    echo "MCS251 reentrant frame or array address fell back to 8 bits" >&2
    exit 1
fi
