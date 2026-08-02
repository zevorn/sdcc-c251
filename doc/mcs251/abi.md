# SDCC MCS251 ABI revision 2

This document freezes the object and calling convention emitted by
`sdcc -mmcs251`.  It describes the SDCC port implemented in this repository;
it is not the Arm/Keil MCS251 ABI and does not claim OMF-251 object or library
interoperability.

## Compatibility identity

- Target option: `-mmcs251`
- Predefined target macro: `__SDCC_mcs251`
- ABI revision reported by the port: `2`
- Object format: SDCC ASxxxx `.rel` plus Intel HEX output
- Default instruction map: MCS-251 Source mode
- Direct calls and tail calls: `ECALL` and `EJMP`
- Normal function return: `ERET`

Objects built for a different MCS251 ABI revision must not be linked together.
The MCS251 runtime libraries are built specifically for this ABI.

## Scalar representation

| C type | Size |
|---|---:|
| `char` / `_Bool` | 1 byte |
| `short` / `int` | 2 bytes |
| `long` / `float` | 4 bytes |
| `long long` / `double` | 8 bytes |
| `size_t` | 4 bytes (`unsigned long`) |
| `__bit` | 1 bit |

Revision 2 uses the native MCS-251 big-endian scalar layout: the
most-significant byte is at the lowest memory address.  A scalar held in an
architectural WR or DR tuple likewise places its most-significant byte in the
lowest-numbered byte register.  For example, WR6 holds bits `15:8` in R6 and
bits `7:0` in R7.  This is the only object layout provided by `-mmcs251`;
there is no little-endian MCS251 mode.  The separate `-mmcs51` target retains
its established little-endian ABI.

MCS251 defines `size_t` as the 32-bit `unsigned long`, although its flat
pointers contain 24 address bits.  C requires `size_t` to represent object
sizes and `sizeof` results; it does not require `size_t` to represent a
converted pointer.  The latter role belongs to `uintptr_t`, which is also
32 bits in this ABI.  The 32-bit `size_t` covers the complete flat object
space without inventing a 24-bit integer type.  MCS51 retains its established
16-bit `unsigned int` definition.  All translation units and runtime
libraries in an MCS251 program must use the revision 2 definition.

## Pointer representation

| Pointer kind | Size | Representation |
|---|---:|---|
| `__data` / `__idata` near pointer | 1 byte | page-zero byte address |
| `__pdata` pointer | 1 byte | offset in the live `MXAX:P2` MOVX page |
| `__xdata` / `__far` pointer | 3 bytes | flat 24-bit address |
| `__code` pointer | 3 bytes | flat 24-bit address |
| unqualified/generic pointer | 3 bytes | flat 24-bit address |
| function pointer | 3 bytes | flat 24-bit code address |

All 3-byte pointer objects store address bits `23:16`, `15:8`, and `7:0` at
ascending addresses.  Revision 2 has no generic-pointer address-space tag.
Pointer arithmetic propagates carries through all 24 bits, including across a
64 KiB boundary.  A generic pointer cannot represent the separate direct SFR
window; SFRs remain accessible through `__sfr` declarations and direct
addressing.

Converting a `__pdata` pointer to a flat pointer snapshots its byte offset,
`P2`, and the STC `MXAX` region byte to form the numeric address
`MXAX:P2:offset`.  Its 3-byte object representation remains high byte first.
This makes the converted address refer to the same byte as `MOVX @Ri`;
changing either page register after the conversion does not retarget the
resulting flat pointer.

DPX is the canonical hardware address register for flat pointers.  DPL,
DPH, and DPXL hold bits `7:0`, `15:8`, and `23:16`, respectively.  Indirect
function calls load a zero-extended 24-bit target into DR28 and use
`ECALL @DR28`.

## Arguments and return values

Revision 2 extends the SDCC MCS-51 calling convention rather than adopting
the Keil register allocator.

- The first scalar argument and scalar return value use the normal SDCC
  return registers: byte in DPL, word in DPL/DPH, and four-byte scalar in
  DPL/DPH/B/A.  These names are listed from the logical least- to the
  most-significant byte; a word is the numeric DPH:DPL pair and a four-byte
  scalar is A:B:DPH:DPL when written most-significant byte first.
- A 3-byte pointer passed or returned in the primary register slot uses
  DPL/DPH/B from its logical least- to most-significant byte, or B:DPH:DPL
  when written most-significant byte first.
- A scalar assigned to a native WR or DR register tuple uses the architectural
  big-endian order: the lowest-numbered register holds its most-significant
  byte and the highest-numbered register holds its least-significant byte.
- Remaining non-reentrant arguments use SDCC overlayable parameter areas.
- Reentrant stack arguments use the ascending hardware stack.
- Carry is used for bit results as in the MCS-51 port.
- Large aggregate returns use the SDCC hidden-result-pointer convention.

The default keeps SDCC's static/overlay convention.  Consequently an
indirect call whose parameters do not all fit the register argument slots
must use a `__reentrant` function-pointer type and callee; an indirect caller
cannot name a callee-specific overlay area.  `--stack-auto` makes all normal
functions use the reentrant stack convention and removes that restriction.
Every translation unit and runtime library in one program must use the same
choice.  Matching libraries are installed as `mcs251-small-stack-auto` and
`mcs251-large-stack-auto` in addition to the default model libraries.

All direct C calls use a 24-bit return frame (`ECALL`/`ERET`), even when the
caller and callee happen to reside in one 64 KiB region.  This keeps separately
compiled objects and linker placement independent of code-region boundaries.
Explicit assembly `LCALL`/`LJMP` remains available and the linker rejects a
target outside the region of the following instruction.

## Stack and startup

The MCS251 startup module initializes SPX to `__start__stack - 1`.  The hardware
stack grows toward higher addresses.  Calls use three return-address bytes;
the configured call overhead accounts for the pre-incrementing push behavior.

ABI revision 2 uses the complete 16-bit hardware SPX for stack-resident
automatic variables, spills, and reentrant arguments.  Scalar stack slots use
the MCS251 signed `@SPX+dis16` addressing forms.  Taking the address of a stack
object materializes a three-byte flat pointer in region `00:`, so arrays and
aggregate objects remain addressable when a frame crosses a 256-byte boundary.

The architectural stack can span region `00:` but its usable capacity is a
property of the selected device and linker layout.  For example, the
STC32G144K246 implements 16 KiB of edata at `00:0000`-`00:3fff`; builds for
that device must reserve the startup stack and account for the deepest call,
interrupt, and reentrant-frame nesting within that RAM.

`jmp_buf` is 15 bytes in every MCS251 data model: two big-endian bytes for
SPX, three big-endian bytes for the complete `ECALL` return PC, two
big-endian private scratch bytes used to carry the normalized `longjmp`
result into the naked restore helper, and eight bytes for R0-R7.  `setjmp`
snapshots the frame and the currently allocatable general registers with
interrupts excluded.  `longjmp` re-creates the three-byte frame, restores
SPX, R0-R7, and the previous interrupt-enable state, and returns the requested
nonzero value.
The QEMU conformance image sets SPX to `0x0120` before `setjmp` and verifies
the restored SPX, the `0x1234` result, and live register values with all four
small/large and default/stack-auto library combinations.

## Memory spaces

The language address spaces retain their SDCC meanings:

- `__data` and `__idata` select page-zero edata/direct storage.
- `__xdata` / `__far` select the flat MCS251 data address space through DPX.
- `__code` selects the flat code address space and is read-only to C.
- Unqualified pointers use the flat 24-bit generic representation.
- `__sfr` and `__sbit` use direct SFR/bit addressing and are not reached by a
  generic pointer conversion.

The SDCC model options are allocation policies, not names from the processor
manual:

- `--model-small` places ordinary objects in page-zero edata and uses direct
  byte addressing where possible.
- `--model-large` places ordinary objects in XSEG and accesses them through
  24-bit DPX pointers.
- `--stack-auto`, orthogonal to either data model, places automatic objects
  and non-register parameters on the 16-bit SPX stack.  Without it, normal
  non-reentrant functions retain SDCC's overlayable static allocation;
  explicitly `__reentrant` functions always use SPX.

MCS251 defaults XSEG to `0x010000`.  This prevents flat generic/far pointers
from aliasing page-zero edata and matches the STC32G on-chip XRAM window used
by the QEMU machine.  A board with a different map can override the location
with `--xram-loc`.  QEMU integration tests place code at `0xff0000`, exercise
xdata above `0x010000`, and verify pointer arithmetic across `0x01ffff` to
`0x020000`.

## Interrupt functions

MCS251 interrupt functions use the target's extended control-flow sequence and
return with `RETI`.  The STC32G machine uses the documented four-byte
interrupt frame.  Application startup must leave the processor in Source
mode; the compiler does not change `AUXR2.CPUMODE`.

## Required conformance tests

Any ABI-affecting change must update the revision and cover, at minimum:

1. independent-unit direct and indirect calls above 64 KiB;
2. all pointer sizes and high-byte relocation;
3. far and generic loads/stores above 64 KiB and across a region boundary;
4. runtime-library pointer consumers such as `atof` and `bsearch`;
5. `setjmp`/`longjmp` with the three-byte return frame and SPX above
   `0x00ff`;
6. unchanged `-mmcs51` object layout, generic-pointer tags, and QEMU behavior.
