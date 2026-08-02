# MCS-251 instruction-set support

This document records what "all MCS-251 instructions are supported" means in
this port.  It separates assembler completeness from compiler instruction
selection and from hardware validation.

## Current status

The `sdas251` assembler covers the complete MCS-251 CPU instruction set
represented by the Intel and STC instruction tables tracked in this
repository:

- 65 of 65 instruction families;
- 269 of 269 recorded legal operand forms;
- Source and Binary opcode maps for every form;
- zero skipped forms in the encoding matrix;
- 24-bit relocation and Intel HEX output; and
- range and operand diagnostics for representative invalid input.

Therefore the assembler-level answer is **yes**: every instruction family and
legal operand form in the repository's reviewed ISA matrix is accepted and
checked against golden bytes in both opcode maps.

This does not mean that the C compiler deliberately emits all 269 forms.  A C
backend selects instructions according to types, the ABI, register allocation
and its cost model.  Forms which are not useful compiler output remain
available to inline and standalone assembly and are tested at the assembler
layer.  Compiler completeness is reported separately below.

## ISA matrix

The canonical machine-readable matrix is
[`sdas/as251/tests/instruction-forms.tsv`](../../sdas/as251/tests/instruction-forms.tsv).
Its `reference` column ties each form to the Intel instruction tables.  The
family manifest is
[`instruction-families.txt`](../../sdas/as251/tests/instruction-families.txt).

| Group | Families | Legal forms |
|---|---:|---:|
| Arithmetic and compare | 10 | 71 |
| Logic and bit operations | 6 | 67 |
| Shift, rotate and swap | 8 | 11 |
| Data transfer and stack | 10 | 78 |
| Conditional control flow | 17 | 24 |
| Jump, call and return | 11 | 15 |
| Miscellaneous | 3 | 3 |
| **Total** | **65** | **269** |

The complete family list, with the number of tested forms in parentheses, is:

- Arithmetic and compare: `ADD` (16), `ADDC` (4), `SUB` (12), `SUBB` (4),
  `CMP` (13), `INC` (8), `DEC` (7), `MUL` (3), `DIV` (3), `DA` (1).
- Logic and bit operations: `ANL` (20), `ORL` (20), `XRL` (16), `CLR` (4),
  `CPL` (4), `SETB` (3).
- Shift, rotate and swap: `RL` (1), `RLC` (1), `RR` (1), `RRC` (1), `SLL`
  (2), `SRA` (2), `SRL` (2), `SWAP` (1).
- Data transfer and stack: `MOV` (55), `MOVC` (2), `MOVH` (1), `MOVS` (1),
  `MOVX` (4), `MOVZ` (1), `XCH` (3), `XCHD` (1), `PUSH` (6), `POP` (4).
- Conditional control flow: `CJNE` (4), `DJNZ` (2), `JB` (2), `JBC` (2),
  `JNB` (2), `JC` (1), `JNC` (1), `JZ` (1), `JNZ` (1), `JE` (1), `JNE`
  (1), `JG` (1), `JLE` (1), `JSG` (1), `JSGE` (1), `JSL` (1), `JSLE` (1).
- Jump, call and return: `ACALL` (1), `AJMP` (1), `LCALL` (2), `LJMP` (2),
  `ECALL` (2), `EJMP` (2), `JMP` (1), `SJMP` (1), `RET` (1), `RETI` (1),
  `ERET` (1).
- Miscellaneous: `NOP` (1), `ESC` (1), `TRAP` (1).

`CALL` and `PUSHW` are accepted convenience aliases.  They are not additional
architectural instruction families and are consequently excluded from the
65-family total.

## Positive and negative assembler tests

Run the complete assembler gate from a configured build directory:

```sh
make -C sdas/as251 check
```

The target performs five independent checks:

1. assembles the reviewed golden-byte source;
2. assembles all 269 legal forms in Source and Binary modes and compares every
   emitted byte;
3. verifies that the generated matrix, family manifest and assembler mnemonic
   table agree;
4. checks cross-object 24-bit relocations and Intel HEX addresses; and
5. rejects invalid branch/page/region ranges, register numbers and widths,
   indirect widths, displacement widths, increment steps, bit numbers and
   push operands with a diagnostic.

The negative suite is representative of each constrained operand class.  It
does not claim to enumerate every possible malformed source string.

## Compiler-generated instructions

The compiler gate is:

```sh
make -C src/mcs251 check
make -C support/valdiag test-mcs251
```

It compiles the port and runtime sources in small, large, small stack-auto and
large stack-auto configurations.  Current code-generation assertions cover:

- native stepped `INC`/`DEC`;
- native `MOVS`/`MOVZ` byte extension and `MOVH` high-word replacement;
- native 32-bit `ADD DR,DR` and `SUB DR,DR` for aligned register tuples,
  including cross-byte carry and borrow runtime cases;
- native unsigned 16-by-16 multiplication with a 32-bit result through
  `MUL WR,WR`, including full big-endian product validation;
- 16-bit `ANL`, `ORL` and `XRL`, plus selected 32-bit `XRL` operations;
- immediate 16-bit `CMP`;
- legal DPX and SPX addressing without unsupported operand forms;
- 24-bit `ECALL`, `EJMP`, indirect calls and `ERET`;
- extended interrupt vectors;
- flat 24-bit data, code and function-pointer relocations;
- model-correct spill placement and 16-bit SPX frames; and
- unchanged MCS-51 pointer layout through the shared compiler paths.

The validation-diagnostics suite also runs its positive and negative C cases
under all four supported MCS-251 configurations.  These checks establish the
currently promised basic compiler optimizations.  They do not establish that
every legal assembler form is an optimization target, nor that instruction
selection is optimal for every C expression.

## Boundaries and known limitations

- The unqualified three-byte pointer is a flat address without an address-space
  tag.  It cannot represent the separate direct SFR window; taking the address
  of an `__sfr` as an unqualified pointer is intentionally diagnosed.
- Native 32-bit register addition and subtraction currently require two
  aligned DR tuples and a destructive result that already aliases a legal
  input.  Memory operands and arbitrary overlapping tuples retain the tested
  byte-at-a-time fallback.
- Native `MUL WR,WR` selection currently covers unsigned 16-bit operands with
  a 32-bit result.  Signed and other-width multiplication retains the existing
  compiler or runtime-library path.
- DSP32, TFPU, MDU32 and chip peripherals are STC SoC facilities controlled
  through registers.  They are not MCS-251 CPU opcodes and are not included in
  the 65-family ISA total.
- Golden bytes come from the reviewed instruction tables and do not make QEMU
  an independent hardware oracle.  Silicon differential testing is still
  desirable for release qualification.
- QEMU UART smoke tests prove that representative compiler output executes and
  communicates.  They do not prove cycle accuracy, peripheral completeness or
  optimal instruction selection.

In short: assembler ISA coverage is complete against the tracked official
matrix; compiler instruction selection and hardware validation have narrower,
explicitly tested scopes.
