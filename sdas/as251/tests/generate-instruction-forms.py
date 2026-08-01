#!/usr/bin/env python3

"""Generate the reviewed MCS-251 instruction-form golden matrix.

The encodings below are a direct transcription of Intel's MCS-251
Programmer's Reference, Appendix A, Tables A-6 through A-17 and the form
summaries in Tables A-19 through A-27.  This program is a maintenance aid;
the test suite consumes the checked-in TSV, not this generator, so accidental
changes to the reference model cannot silently change the test oracle.
"""

from dataclasses import dataclass


@dataclass(frozen=True)
class Form:
    identifier: str
    mnemonic: str
    operands: str
    assembly: str
    source: tuple[int, ...]
    binary: tuple[int, ...]
    flags: str
    reference: str


forms: list[Form] = []


def mapped(opcode: int, tail: tuple[int, ...], native: bool):
    encoding = (opcode, *tail)
    conflict = opcode & 0x0f >= 6
    if not conflict:
        return encoding, encoding
    if native:
        return encoding, (0xa5, *encoding)
    return (0xa5, *encoding), encoding


def add(identifier, mnemonic, operands, assembly, encoding, *, native=False,
        flags="-", reference=""):
    source, binary = mapped(encoding[0], tuple(encoding[1:]), native)
    forms.append(Form(identifier, mnemonic, operands, assembly, source,
                      binary, flags, reference))


def add_maps(identifier, mnemonic, operands, assembly, source, binary, *,
             flags="-", reference=""):
    forms.append(Form(identifier, mnemonic, operands, assembly, tuple(source),
                      tuple(binary), flags, reference))


def data_forms(mnemonic, high_nibble, *, dword_signed=False,
               allow_dword=True, reference, flags):
    op = high_nibble << 4
    add(f"{mnemonic}_rm_rm", mnemonic, "Rmd,Rms", f"{mnemonic} r3,r12",
        (op | 0x0c, 0x3c), native=True, flags=flags, reference=reference)
    add(f"{mnemonic}_wr_wr", mnemonic, "WRjd,WRjs",
        f"{mnemonic} wr4,wr10", (op | 0x0d, 0x25), native=True,
        flags=flags, reference=reference)
    if allow_dword:
        add(f"{mnemonic}_dr_dr", mnemonic, "DRkd,DRks",
            f"{mnemonic} dr12,dr28", (op | 0x0f, 0x37), native=True,
            flags=flags, reference=reference)
    add(f"{mnemonic}_rm_imm8", mnemonic, "Rm,#data",
        f"{mnemonic} r3,#0x5a", (op | 0x0e, 0x30, 0x5a), native=True,
        flags=flags, reference=reference)
    add(f"{mnemonic}_wr_imm16", mnemonic, "WRj,#data16",
        f"{mnemonic} wr4,#0x1234", (op | 0x0e, 0x24, 0x12, 0x34),
        native=True, flags=flags, reference=reference)
    if allow_dword:
        add(f"{mnemonic}_dr_imm16_zero", mnemonic, "DRk,#0data16",
            f"{mnemonic} dr12,#0x1234",
            (op | 0x0e, 0x38, 0x12, 0x34), native=True, flags=flags,
            reference=reference)
        if dword_signed:
            add(f"{mnemonic}_dr_imm16_one", mnemonic, "DRk,#1data16",
                f"{mnemonic} dr12,#-3",
                (op | 0x0e, 0x3c, 0xff, 0xfd), native=True, flags=flags,
                reference=reference)
    add(f"{mnemonic}_rm_dir8", mnemonic, "Rm,dir8",
        f"{mnemonic} r3,0x30", (op | 0x0e, 0x31, 0x30), native=True,
        flags=flags, reference=reference)
    add(f"{mnemonic}_wr_dir8", mnemonic, "WRj,dir8",
        f"{mnemonic} wr4,0x30", (op | 0x0e, 0x25, 0x30), native=True,
        flags=flags, reference=reference)
    add(f"{mnemonic}_rm_dir16", mnemonic, "Rm,dir16",
        f"{mnemonic} r3,0x1234", (op | 0x0e, 0x33, 0x12, 0x34),
        native=True, flags=flags, reference=reference)
    add(f"{mnemonic}_wr_dir16", mnemonic, "WRj,dir16",
        f"{mnemonic} wr4,0x1234", (op | 0x0e, 0x27, 0x12, 0x34),
        native=True, flags=flags, reference=reference)
    add(f"{mnemonic}_rm_at_wr", mnemonic, "Rm,@WRj",
        f"{mnemonic} r3,@wr6", (op | 0x0e, 0x39, 0x30), native=True,
        flags=flags, reference=reference)
    add(f"{mnemonic}_rm_at_dr", mnemonic, "Rm,@DRk",
        f"{mnemonic} r3,@dr16", (op | 0x0e, 0x4b, 0x30), native=True,
        flags=flags, reference=reference)


ARITH = "CY,AC,OV,N,Z"
NZ = "N,Z"

# Table A-19: add and subtract.
add("add_a_rn", "add", "A,Rn", "add a,r6", (0x2e,), flags=ARITH,
    reference="Intel A-19/A-28")
add("add_a_dir8", "add", "A,dir8", "add a,0x30", (0x25, 0x30),
    flags=ARITH, reference="Intel A-19/A-28")
add("add_a_at_ri", "add", "A,@Ri", "add a,@r1", (0x27,),
    flags=ARITH, reference="Intel A-19/A-28")
add("add_a_imm8", "add", "A,#data", "add a,#0x5a", (0x24, 0x5a),
    flags=ARITH, reference="Intel A-19/A-28")
data_forms("add", 0x2, reference="Intel A-8/A-19", flags=ARITH)
data_forms("sub", 0x9, reference="Intel A-8/A-19", flags=ARITH)

for mnemonic, base in (("addc", 0x30), ("subb", 0x90)):
    ref = "Intel A-19"
    add(f"{mnemonic}_a_rn", mnemonic, "A,Rn", f"{mnemonic} a,r6",
        (base | 0x0e,), flags=ARITH, reference=ref)
    add(f"{mnemonic}_a_dir8", mnemonic, "A,dir8",
        f"{mnemonic} a,0x30", (base | 0x05, 0x30), flags=ARITH,
        reference=ref)
    add(f"{mnemonic}_a_at_ri", mnemonic, "A,@Ri",
        f"{mnemonic} a,@r1", (base | 0x07,), flags=ARITH,
        reference=ref)
    add(f"{mnemonic}_a_imm8", mnemonic, "A,#data",
        f"{mnemonic} a,#0x5a", (base | 0x04, 0x5a), flags=ARITH,
        reference=ref)

# Table A-20: compare, including both dword immediate extensions.
data_forms("cmp", 0xb, dword_signed=True, reference="Intel A-8/A-20",
           flags=ARITH)

# Tables A-21 and A-15/A-16: increment/decrement.
for mnemonic, classic_base, native_op in (("inc", 0x00, 0x0b),
                                          ("dec", 0x10, 0x1b)):
    ref = "Intel A-15/A-16/A-21"
    add(f"{mnemonic}_a", mnemonic, "A", f"{mnemonic} a",
        (classic_base | 0x04,), flags=NZ, reference=ref)
    # The assembler deliberately selects the shorter native spelling in
    # Source mode and the classic spelling in Binary mode for R0..R7.
    add_maps(f"{mnemonic}_rn", mnemonic, "Rn", f"{mnemonic} r5",
             (native_op, 0x50), (classic_base | 0x0d,), flags=NZ,
             reference=ref)
    add(f"{mnemonic}_dir8", mnemonic, "dir8", f"{mnemonic} 0x30",
        (classic_base | 0x05, 0x30), flags=NZ, reference=ref)
    add(f"{mnemonic}_at_ri", mnemonic, "@Ri", f"{mnemonic} @r1",
        (classic_base | 0x07,), flags=NZ, reference=ref)
    for suffix, operand, specifier in (("rm", "r10", 0xa1),
                                       ("wr", "wr4", 0x25),
                                       ("dr", "dr12", 0x3e)):
        short = 2 if suffix != "dr" else 4
        add(f"{mnemonic}_{suffix}_short", mnemonic,
            {"rm": "Rm,#short", "wr": "WRj,#short",
             "dr": "DRk,#short"}[suffix],
            f"{mnemonic} {operand},#{short}", (native_op, specifier),
            native=True, flags=NZ, reference=ref)

add("inc_dptr", "inc", "DPTR", "inc dptr", (0xa3,),
    reference="Intel A-21")

# Table A-22: multiply, divide, and decimal adjust.
for mnemonic, classic_op, byte_op, word_op in (
        ("mul", 0xa4, 0xac, 0xad), ("div", 0x84, 0x8c, 0x8d)):
    flags = "CY,OV,N,Z"
    add(f"{mnemonic}_ab", mnemonic, "AB", f"{mnemonic} ab",
        (classic_op,), flags=flags, reference="Intel A-22")
    add(f"{mnemonic}_rm_rm", mnemonic, "Rmd,Rms",
        f"{mnemonic} r3,r12", (byte_op, 0x3c), native=True, flags=flags,
        reference="Intel A-8/A-22")
    add(f"{mnemonic}_wr_wr", mnemonic, "WRjd,WRjs",
        f"{mnemonic} wr4,wr10", (word_op, 0x25), native=True,
        flags=flags, reference="Intel A-8/A-22")
add("da_a", "da", "A", "da a", (0xd4,), flags="CY",
    reference="Intel A-22")

# Table A-23: logical operations and shifts.
for mnemonic, high in (("orl", 0x4), ("anl", 0x5), ("xrl", 0x6)):
    op = high << 4
    ref = "Intel A-8/A-23"
    add(f"{mnemonic}_a_rn", mnemonic, "A,Rn", f"{mnemonic} a,r6",
        (op | 0x0e,), flags=NZ, reference=ref)
    add(f"{mnemonic}_a_dir8", mnemonic, "A,dir8",
        f"{mnemonic} a,0x30", (op | 0x05, 0x30), flags=NZ,
        reference=ref)
    add(f"{mnemonic}_a_at_ri", mnemonic, "A,@Ri",
        f"{mnemonic} a,@r1", (op | 0x07,), flags=NZ, reference=ref)
    add(f"{mnemonic}_a_imm8", mnemonic, "A,#data",
        f"{mnemonic} a,#0x5a", (op | 0x04, 0x5a), flags=NZ,
        reference=ref)
    add(f"{mnemonic}_dir8_a", mnemonic, "dir8,A",
        f"{mnemonic} 0x30,a", (op | 0x02, 0x30), flags=NZ,
        reference=ref)
    add(f"{mnemonic}_dir8_imm8", mnemonic, "dir8,#data",
        f"{mnemonic} 0x30,#0x5a", (op | 0x03, 0x30, 0x5a),
        flags=NZ, reference=ref)
    data_forms(mnemonic, high, allow_dword=False, reference=ref, flags=NZ)

add("clr_a", "clr", "A", "clr a", (0xe4,), flags=NZ,
    reference="Intel A-23")
add("cpl_a", "cpl", "A", "cpl a", (0xf4,), flags=NZ,
    reference="Intel A-23")
for mnemonic, opcode, flags in (("rl", 0x23, NZ), ("rlc", 0x33, "CY,N,Z"),
                                ("rr", 0x03, NZ), ("rrc", 0x13, "CY,N,Z")):
    add(f"{mnemonic}_a", mnemonic, "A", f"{mnemonic} a", (opcode,),
        flags=flags, reference="Intel A-23")
for mnemonic, opcode in (("sra", 0x0e), ("srl", 0x1e), ("sll", 0x3e)):
    add(f"{mnemonic}_rm", mnemonic, "Rm", f"{mnemonic} r3",
        (opcode, 0x30), native=True, flags="CY,N,Z",
        reference="Intel A-17/A-23")
    add(f"{mnemonic}_wr", mnemonic, "WRj", f"{mnemonic} wr4",
        (opcode, 0x24), native=True, flags="CY,N,Z",
        reference="Intel A-17/A-23")
add("swap_a", "swap", "A", "swap a", (0xc4,), flags=NZ,
    reference="Intel A-23")

# Table A-24: classic MOV forms.
ref = "Intel A-6/A-24"
add("mov_a_rn", "mov", "A,Rn", "mov a,r6", (0xee,), reference=ref)
add("mov_a_dir8", "mov", "A,dir8", "mov a,0x30", (0xe5, 0x30),
    reference=ref)
add("mov_a_at_ri", "mov", "A,@Ri", "mov a,@r1", (0xe7,),
    reference=ref)
add("mov_a_imm8", "mov", "A,#data", "mov a,#0x5a", (0x74, 0x5a),
    reference=ref)
add("mov_rn_a", "mov", "Rn,A", "mov r5,a", (0xfd,), reference=ref)
add_maps("mov_rn_dir8", "mov", "Rn,dir8", "mov r5,0x30",
         (0x7e, 0x51, 0x30), (0xad, 0x30), reference=ref)
add_maps("mov_rn_imm8", "mov", "Rn,#data", "mov r5,#0x5a",
         (0x7e, 0x50, 0x5a), (0x7d, 0x5a), reference=ref)
add("mov_dir8_a", "mov", "dir8,A", "mov 0x30,a", (0xf5, 0x30),
    reference=ref)
add_maps("mov_dir8_rn", "mov", "dir8,Rn", "mov 0x30,r5",
         (0x7a, 0x51, 0x30), (0x8d, 0x30), reference=ref)
add("mov_dir8_dir8", "mov", "dir8,dir8", "mov 0x30,0x31",
    (0x85, 0x31, 0x30), reference=ref)
add("mov_dir8_at_ri", "mov", "dir8,@Ri", "mov 0x30,@r1",
    (0x87, 0x30), reference=ref)
add("mov_dir8_imm8", "mov", "dir8,#data", "mov 0x30,#0x5a",
    (0x75, 0x30, 0x5a), reference=ref)
add("mov_at_ri_a", "mov", "@Ri,A", "mov @r1,a", (0xf7,),
    reference=ref)
add("mov_at_ri_dir8", "mov", "@Ri,dir8", "mov @r1,0x30",
    (0xa7, 0x30), reference=ref)
add("mov_at_ri_imm8", "mov", "@Ri,#data", "mov @r1,#0x5a",
    (0x77, 0x5a), reference=ref)
add("mov_dptr_imm16", "mov", "DPTR,#data16", "mov dptr,#0x1234",
    (0x90, 0x12, 0x34), reference=ref)

# Tables A-8 and A-14: native MOV forms.
ref = "Intel A-8/A-14/A-24"
add("mov_rm_rm", "mov", "Rmd,Rms", "mov r3,r12", (0x7c, 0x3c),
    native=True, reference=ref)
add("mov_wr_wr", "mov", "WRjd,WRjs", "mov wr4,wr10", (0x7d, 0x25),
    native=True, reference=ref)
add("mov_dr_dr", "mov", "DRkd,DRks", "mov dr12,dr28", (0x7f, 0x37),
    native=True, reference=ref)
for identifier, operands, assembly, encoding in (
        ("mov_rm_imm8", "Rm,#data", "mov r10,#0x5a", (0x7e, 0xa0, 0x5a)),
        ("mov_wr_imm16", "WRj,#data16", "mov wr4,#0x1234",
         (0x7e, 0x24, 0x12, 0x34)),
        ("mov_dr_imm16_zero", "DRk,#0data16", "mov dr12,#0x1234",
         (0x7e, 0x38, 0x12, 0x34)),
        ("mov_dr_imm16_one", "DRk,#1data16", "mov dr12,#-3",
         (0x7e, 0x3c, 0xff, 0xfd)),
        ("mov_dr_dir8", "DRk,dir8", "mov dr12,0x30",
         (0x7e, 0x3d, 0x30)),
        ("mov_dr_dir16", "DRk,dir16", "mov dr12,0x1234",
         (0x7e, 0x3f, 0x12, 0x34)),
        ("mov_rm_dir8", "Rm,dir8", "mov r10,0x30", (0x7e, 0xa1, 0x30)),
        ("mov_wr_dir8", "WRj,dir8", "mov wr4,0x30", (0x7e, 0x25, 0x30)),
        ("mov_rm_dir16", "Rm,dir16", "mov r3,0x1234",
         (0x7e, 0x33, 0x12, 0x34)),
        ("mov_wr_dir16", "WRj,dir16", "mov wr4,0x1234",
         (0x7e, 0x27, 0x12, 0x34)),
        ("mov_rm_at_wr", "Rm,@WRj", "mov r3,@wr6", (0x7e, 0x39, 0x30)),
        ("mov_rm_at_dr", "Rm,@DRk", "mov r3,@dr16", (0x7e, 0x4b, 0x30)),
        ("mov_wr_at_wr", "WRjd,@WRjs", "mov wr4,@wr10",
         (0x0b, 0x58, 0x20)),
        ("mov_wr_at_dr", "WRj,@DRk", "mov wr4,@dr16",
         (0x0b, 0x4a, 0x20)),
        ("mov_dir8_rm", "dir8,Rm", "mov 0x30,r10", (0x7a, 0xa1, 0x30)),
        ("mov_dir8_wr", "dir8,WRj", "mov 0x30,wr4", (0x7a, 0x25, 0x30)),
        ("mov_dir16_rm", "dir16,Rm", "mov 0x1234,r3",
         (0x7a, 0x33, 0x12, 0x34)),
        ("mov_dir16_wr", "dir16,WRj", "mov 0x1234,wr4",
         (0x7a, 0x27, 0x12, 0x34)),
        ("mov_at_wr_rm", "@WRj,Rm", "mov @wr6,r3", (0x7a, 0x39, 0x30)),
        ("mov_at_dr_rm", "@DRk,Rm", "mov @dr16,r3", (0x7a, 0x4b, 0x30)),
        ("mov_at_wr_wr", "@WRjd,WRjs", "mov @wr6,wr4",
         (0x1b, 0x38, 0x20)),
        ("mov_at_dr_wr", "@DRk,WRj", "mov @dr16,wr4",
         (0x1b, 0x4a, 0x20)),
        ("mov_dir8_dr", "dir8,DRk", "mov 0x30,dr12", (0x7a, 0x3d, 0x30)),
        ("mov_dir16_dr", "dir16,DRk", "mov 0x1234,dr12",
         (0x7a, 0x3f, 0x12, 0x34)),
        ("mov_rm_idx_wr", "Rm,@WRj+dis16", "mov r10,@wr6+0x1234",
         (0x09, 0xa3, 0x12, 0x34)),
        ("mov_wr_idx_wr", "WRj,@WRj+dis16", "mov wr4,@wr6+0x1234",
         (0x49, 0x23, 0x12, 0x34)),
        ("mov_rm_idx_dr", "Rm,@DRk+dis24", "mov r10,@dr16+0x1234",
         (0x29, 0xa4, 0x12, 0x34)),
        ("mov_wr_idx_dr", "WRj,@DRk+dis24", "mov wr4,@dr16+0x1234",
         (0x69, 0x24, 0x12, 0x34)),
        ("mov_idx_wr_rm", "@WRj+dis16,Rm", "mov @wr6+0x1234,r10",
         (0x19, 0xa3, 0x12, 0x34)),
        ("mov_idx_wr_wr", "@WRj+dis16,WRj", "mov @wr6+0x1234,wr4",
         (0x59, 0x23, 0x12, 0x34)),
        ("mov_idx_dr_rm", "@DRk+dis24,Rm", "mov @dr16+0x1234,r10",
         (0x39, 0xa4, 0x12, 0x34)),
        ("mov_idx_dr_wr", "@DRk+dis24,WRj", "mov @dr16+0x1234,wr4",
         (0x79, 0x24, 0x12, 0x34))):
    add(identifier, "mov", operands, assembly, encoding, native=True,
        reference=ref)

add("movh_dr_imm16", "movh", "DRk(hi),#data16",
    "movh dr12,#0x1234", (0x7a, 0x3c, 0x12, 0x34), native=True,
    reference="Intel A-8/A-24")
add("movs_wr_rm", "movs", "WRj,Rm", "movs wr4,r3", (0x1a, 0x23),
    native=True, flags=NZ, reference="Intel A-14/A-24")
add("movz_wr_rm", "movz", "WRj,Rm", "movz wr4,r3", (0x0a, 0x23),
    native=True, flags=NZ, reference="Intel A-14/A-24")
add("movc_a_at_a_dptr", "movc", "A,@A+DPTR", "movc a,@a+dptr",
    (0x93,), reference="Intel A-24")
add("movc_a_at_a_pc", "movc", "A,@A+PC", "movc a,@a+pc", (0x83,),
    reference="Intel A-24")
for identifier, operands, assembly, encoding in (
        ("movx_a_at_ri", "A,@Ri", "movx a,@r1", (0xe3,)),
        ("movx_a_at_dptr", "A,@DPTR", "movx a,@dptr", (0xe0,)),
        ("movx_at_ri_a", "@Ri,A", "movx @r1,a", (0xf3,)),
        ("movx_at_dptr_a", "@DPTR,A", "movx @dptr,a", (0xf0,))):
    add(identifier, "movx", operands, assembly, encoding,
        reference="Intel A-6/A-24")

# Table A-25: exchange and stack operations.
add("xch_a_rn", "xch", "A,Rn", "xch a,r5", (0xcd,),
    reference="Intel A-25")
add("xch_a_dir8", "xch", "A,dir8", "xch a,0x30", (0xc5, 0x30),
    reference="Intel A-25")
add("xch_a_at_ri", "xch", "A,@Ri", "xch a,@r1", (0xc7,),
    reference="Intel A-25")
add("xchd_a_at_ri", "xchd", "A,@Ri", "xchd a,@r1", (0xd7,),
    reference="Intel A-25")
add("push_dir8", "push", "dir8", "push 0x30", (0xc0, 0x30),
    reference="Intel A-12/A-25")
for identifier, operands, assembly, encoding in (
        ("push_imm8", "#data", "push #0x5a", (0xca, 0x02, 0x5a)),
        ("push_imm16", "#data16", "push #0x1234",
         (0xca, 0x06, 0x12, 0x34)),
        ("push_rm", "Rm", "push r3", (0xca, 0x38)),
        ("push_wr", "WRj", "push wr4", (0xca, 0x29)),
        ("push_dr", "DRk", "push dr12", (0xca, 0x3b))):
    add(identifier, "push", operands, assembly, encoding, native=True,
        reference="Intel A-12/A-25")
add("pop_dir8", "pop", "dir8", "pop 0x30", (0xd0, 0x30),
    reference="Intel A-12/A-25")
for identifier, operands, assembly, encoding in (
        ("pop_rm", "Rm", "pop r3", (0xda, 0x38)),
        ("pop_wr", "WRj", "pop wr4", (0xda, 0x29)),
        ("pop_dr", "DRk", "pop dr12", (0xda, 0x3b))):
    add(identifier, "pop", operands, assembly, encoding, native=True,
        reference="Intel A-12/A-25")

# Table A-26: classic bit51 and extended MCS251 bit forms.
for mnemonic, carry_op, bit_op, extended_op in (
        ("clr", 0xc3, 0xc2, 0xc0),
        ("setb", 0xd3, 0xd2, 0xd0),
        ("cpl", 0xb3, 0xb2, 0xb0)):
    add(f"{mnemonic}_cy", mnemonic, "CY", f"{mnemonic} cy", (carry_op,),
        flags="CY", reference="Intel A-10/A-11/A-26")
    add(f"{mnemonic}_bit51", mnemonic, "bit51", f"{mnemonic} 0x0f",
        (bit_op, 0x0f), reference="Intel A-10/A-11/A-26")
    add(f"{mnemonic}_bit", mnemonic, "bit", f"{mnemonic} 0x30.5",
        (0xa9, extended_op | 0x05, 0x30), native=True,
        reference="Intel A-10/A-11/A-26")

for mnemonic, classic_plain, classic_inverted, extended_plain, extended_inv in (
        ("anl", 0x82, 0xb0, 0x80, 0xf0),
        ("orl", 0x72, 0xa0, 0x70, 0xe0)):
    ref = "Intel A-10/A-11/A-26"
    add(f"{mnemonic}_cy_bit51", mnemonic, "CY,bit51",
        f"{mnemonic} cy,0x0f", (classic_plain, 0x0f), flags="CY",
        reference=ref)
    add(f"{mnemonic}_cy_bit", mnemonic, "CY,bit",
        f"{mnemonic} cy,0x30.5", (0xa9, extended_plain | 0x05, 0x30),
        native=True, flags="CY", reference=ref)
    add(f"{mnemonic}_cy_not_bit51", mnemonic, "CY,/bit51",
        f"{mnemonic} cy,/0x0f", (classic_inverted, 0x0f), flags="CY",
        reference=ref)
    add(f"{mnemonic}_cy_not_bit", mnemonic, "CY,/bit",
        f"{mnemonic} cy,/0x30.5", (0xa9, extended_inv | 0x05, 0x30),
        native=True, flags="CY", reference=ref)

for identifier, operands, assembly, encoding, native in (
        ("mov_cy_bit51", "CY,bit51", "mov cy,0x0f", (0xa2, 0x0f), False),
        ("mov_cy_bit", "CY,bit", "mov cy,0x30.5",
         (0xa9, 0xa5, 0x30), True),
        ("mov_bit51_cy", "bit51,CY", "mov 0x0f,cy", (0x92, 0x0f), False),
        ("mov_bit_cy", "bit,CY", "mov 0x30.5,cy",
         (0xa9, 0x95, 0x30), True)):
    add(identifier, "mov", operands, assembly, encoding, native=native,
        flags="CY" if operands.startswith("CY") else "-",
        reference="Intel A-10/A-11/A-26")

# Table A-27: control transfers.  {target} is replaced with a label placed
# immediately after the instruction, producing a fixed zero displacement.
add("acall_addr11", "acall", "addr11", "acall 0x120600", (0xd1, 0x00),
    reference="Intel A-13/A-27")
add("ecall_at_dr", "ecall", "@DRk", "ecall @dr16", (0x99, 0x48),
    native=True, reference="Intel A-13/A-27")
add("ecall_addr24", "ecall", "addr24", "ecall 0x123456",
    (0x9a, 0x12, 0x34, 0x56), native=True, reference="Intel A-13/A-27")
add("lcall_at_wr", "lcall", "@WRj", "lcall @wr4", (0x99, 0x24),
    native=True, reference="Intel A-13/A-27")
add("lcall_addr16", "lcall", "addr16", "lcall 0x123456",
    (0x12, 0x34, 0x56), reference="Intel A-13/A-27")
add("ret", "ret", "-", "ret", (0x22,), reference="Intel A-27")
add("eret", "eret", "-", "eret", (0xaa,), native=True,
    reference="Intel A-13/A-27")
add("reti", "reti", "-", "reti", (0x32,), reference="Intel A-27")
add("ajmp_addr11", "ajmp", "addr11", "ajmp 0x120600", (0xc1, 0x00),
    reference="Intel A-13/A-27")
add("ejmp_addr24", "ejmp", "addr24", "ejmp 0x123456",
    (0x8a, 0x12, 0x34, 0x56), native=True, reference="Intel A-13/A-27")
add("ejmp_at_dr", "ejmp", "@DRk", "ejmp @dr16", (0x89, 0x48),
    native=True, reference="Intel A-13/A-27")
add("ljmp_at_wr", "ljmp", "@WRj", "ljmp @wr4", (0x89, 0x24),
    native=True, reference="Intel A-13/A-27")
add("ljmp_addr16", "ljmp", "addr16", "ljmp 0x123456",
    (0x02, 0x34, 0x56), reference="Intel A-13/A-27")
add("sjmp_rel", "sjmp", "rel", "sjmp {target}", (0x80, 0x00),
    reference="Intel A-27")
add("jmp_at_a_dptr", "jmp", "@A+DPTR", "jmp @a+dptr", (0x73,),
    reference="Intel A-27")

for mnemonic, opcode in (("jc", 0x40), ("jnc", 0x50), ("jz", 0x60),
                         ("jnz", 0x70), ("je", 0x68), ("jne", 0x78),
                         ("jg", 0x38), ("jle", 0x28), ("jsl", 0x48),
                         ("jsle", 0x08), ("jsg", 0x18), ("jsge", 0x58)):
    add(f"{mnemonic}_rel", mnemonic, "rel", f"{mnemonic} {{target}}",
        (opcode, 0x00), native=mnemonic not in {"jc", "jnc", "jz", "jnz"},
        reference="Intel A-13/A-27")

for mnemonic, classic_op, extended_op in (("jbc", 0x10, 0x10),
                                           ("jb", 0x20, 0x20),
                                           ("jnb", 0x30, 0x30)):
    add(f"{mnemonic}_bit51_rel", mnemonic, "bit51,rel",
        f"{mnemonic} 0x0f,{{target}}", (classic_op, 0x0f, 0x00),
        reference="Intel A-10/A-11/A-27")
    add(f"{mnemonic}_bit_rel", mnemonic, "bit,rel",
        f"{mnemonic} 0x30.5,{{target}}",
        (0xa9, extended_op | 0x05, 0x30, 0x00), native=True,
        reference="Intel A-10/A-11/A-27")

for identifier, operands, assembly, encoding in (
        ("cjne_a_dir8_rel", "A,dir8,rel", "cjne a,0x30,{target}",
         (0xb5, 0x30, 0x00)),
        ("cjne_a_imm8_rel", "A,#data,rel", "cjne a,#0x5a,{target}",
         (0xb4, 0x5a, 0x00)),
        ("cjne_rn_imm8_rel", "Rn,#data,rel", "cjne r6,#0x5a,{target}",
         (0xbe, 0x5a, 0x00)),
        ("cjne_at_ri_imm8_rel", "@Ri,#data,rel",
         "cjne @r1,#0x5a,{target}", (0xb7, 0x5a, 0x00))):
    add(identifier, "cjne", operands, assembly, encoding,
        reference="Intel A-27")
add("djnz_rn_rel", "djnz", "Rn,rel", "djnz r5,{target}",
    (0xdd, 0x00), reference="Intel A-27")
add("djnz_dir8_rel", "djnz", "dir8,rel", "djnz 0x30,{target}",
    (0xd5, 0x30, 0x00), reference="Intel A-27")
add("trap", "trap", "-", "trap", (0xb9,), native=True,
    reference="Intel A-13/A-27")
add("nop", "nop", "-", "nop", (0x00,), reference="Intel A-27")
add("esc", "esc", "-", "esc", (0xa5,), reference="Intel A-6")


def hex_bytes(values):
    return " ".join(f"{value:02x}" for value in values)


print("id\tmnemonic\toperands\tassembly\tsource_bytes\tbinary_bytes\tflags\treference")
seen = set()
for form in forms:
    if form.identifier in seen:
        raise SystemExit(f"duplicate form id: {form.identifier}")
    seen.add(form.identifier)
    print("\t".join((form.identifier, form.mnemonic, form.operands,
                     form.assembly, hex_bytes(form.source),
                     hex_bytes(form.binary), form.flags, form.reference)))
