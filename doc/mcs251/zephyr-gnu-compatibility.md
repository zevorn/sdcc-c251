# Zephyr and GNU C compatibility assessment

Assessment date: 2026-08-02

This document separates C frontend compatibility from the ABI, object-format
and architecture work required to build Zephyr. The first reproducible target
is Zephyr 4.4 with its default C17 configuration.

## Result

SDCC accepts strict ISO C17 for MCS-51 and MCS-251. This downstream also
accepts `--std=gnu11` and `--std=gnu17`, provides common GNU keyword aliases
and attribute syntax, implements statement expressions and constant
type-compatibility and constant-expression queries, and preserves GNU
branch-expectation hints. The GNU subset implemented by the common frontend
is still not sufficient for Zephyr.

The two language modes alone do not provide Zephyr support. Stock Zephyr is
also incompatible with the normal MCS-251 ABI and with the ASxxxx
`.rel`/`.ihx` build pipeline. The complete prerequisite is:

1. C17 plus the GNU C subset actually used by Zephyr;
2. a separately selected ABI with a 32-bit `int` and a 32-bit C pointer
   representation;
3. ELF32 relocatable objects, archives, named sections, symbols, relocations
   and a final ELF executable;
4. an SDCC toolchain definition and an MCS-251 architecture, SoC and board
   port in a Zephyr downstream.

The default MCS-51 and MCS-251 language and ABI behavior must not change.

## Current SDCC boundary

The common frontend accepts `c17`, `sdcc17` and `gnu17` as C17 revision names
in [`SDCCmain.c`](../../src/SDCCmain.c). `sdcpp` defines
`__STDC_VERSION__` as `201710L` for C17 and GNU17 in
[`libcpp/init.cc`](../../support/cpp/libcpp/init.cc). GNU11 similarly selects
the C11 base revision and defines `201112L`.

Direct probes against the current MCS-251 compiler give this baseline:

| Construct | Current result |
|---|---|
| `--std=c17`, `--std=sdcc17` | accepted |
| `--std=gnu11`, `--std=gnu17` | accepted; enable the tested GNU subset |
| `__typeof(...)`, `__typeof__(...)` | accepted, with expression limits |
| `__builtin_types_compatible_p(type1, type2)` | accepted in GNU11/GNU17; integer constant expression |
| `__builtin_expect(expression, expected)` | accepted in GNU11/GNU17; returns `long` expression and supplies a 90/10 branch hint when expected is constant |
| `__builtin_constant_p(expression)` | accepted in GNU11/GNU17; conservatively folds to zero or one without evaluating the operand |
| `__has_builtin(name)` | reports common builtins in every mode, GNU-only builtins in GNU11/GNU17, and zero for unknown names |
| `__builtin_unreachable()` | accepted in every mode; no external call is emitted |
| target data-model macros | MCS-51/MCS-251 sizes, types, limits, constants and byte order are predefined; host ABI macros are suppressed |
| basic `__asm__("instruction")` | accepted |
| extended asm operands, constraints and clobbers | rejected |
| statement expression `({ ... })` | accepted in GNU11/GNU17; the final expression statement supplies the value and type |
| `__auto_type` and `__extension__` | rejected |
| nested function and computed `goto` | rejected |
| `case low ... high` | accepted only in the C2y extension mode |
| common GCC `__attribute__((...))` placements | parsed; most names are syntax-only |

The optional [`gcc_attr.h`](../../device/include/gcc_attr.h) header can remove
GCC attributes outside C23 mode or rewrite their syntax to C23 attributes in
C23 mode. It does not implement the placement, linkage, layout or calling
semantics of GCC attributes. The frontend currently gives first-class meaning
to only `nodiscard`, `maybe_unused`, `deprecated` and `fallthrough`.

MCS-251 output uses `sdas251` and `sdld`, not GAS and GNU ld. Inline assembly
therefore uses ASxxxx syntax even after the parser accepts the GCC spelling.

The preprocessor data model follows the selected SDCC target rather than the
machine on which SDCC runs. MCS-51 advertises its existing little-endian ABI;
MCS-251 advertises its native big-endian ABI. Both report a 16-bit `int`, a
32-bit `long`, a 64-bit `long long` and a three-byte generic pointer. The
driver also supplies the GCC-style exact, least, fast, pointer and maximum
integer type and constant macros used by portable compiler abstraction
headers. Tests dump the real preprocessor macro set and compile expressions
that consume those definitions for both targets.

## What Zephyr requires from the C frontend

Zephyr 4.4 selects C17 by default. Its Kconfig also exposes GNU extensions,
and its compiler abstraction expects a real `-std=` language-mode mapping.
See the official [Zephyr Kconfig](https://github.com/zephyrproject-rtos/zephyr/blob/v4.4.0/Kconfig.zephyr)
and [GCC compiler properties](https://github.com/zephyrproject-rtos/zephyr/blob/v4.4.0/cmake/compiler/gcc/compiler_flags.cmake).

Even when application code is written in standard C, common Zephyr headers
need compiler-specific equivalents for these facilities:

- `__typeof__`, `_Generic` and statement expressions (implemented in
  GNU11/GNU17), plus `__auto_type` and variadic macro comma elision;
- `__builtin_types_compatible_p`, `__builtin_expect` and
  `__builtin_constant_p` (implemented in GNU11/GNU17), and
  `__builtin_unreachable` (implemented in every mode), plus the remaining
  bit-counting and overflow builtins;
- `section`, `used`, `weak`, `packed`, `aligned`, `always_inline`, `noinline`,
  `noreturn`, `alias` and related attribute semantics;
- compiler barriers and target operations used by context switching and
  interrupt code;
- global, weak and absolute assembly symbols.

The concrete definitions are visible in Zephyr's official
[`toolchain/gcc.h`](https://github.com/zephyrproject-rtos/zephyr/blob/v4.4.0/include/zephyr/toolchain/gcc.h).
Not every GCC extension has to be implemented literally. Zephyr supports a
custom compiler header, so an SDCC-specific equivalent can replace extended
asm or a builtin where that preserves the required semantics. Defining
`__GNUC__` to make Zephyr include `gcc.h` is not acceptable: it would claim
many semantics that SDCC does not provide.

## ABI boundary

The current MCS-251 port defines these C type sizes in
[`mcs51/main.c`](../../src/mcs51/main.c):

| Type | Normal MCS-251 ABI |
|---|---:|
| `char` | 1 byte |
| `short` | 2 bytes |
| `int` | 2 bytes |
| `long` | 4 bytes |
| generic/data/code pointer | 3 bytes |

Zephyr's public kernel header asserts that `int32_t` has the same size as
`int`, `int64_t` has the same size as `long long`, and `intptr_t` has the same
size as `long`. See
[`kernel.h`](https://github.com/zephyrproject-rtos/zephyr/blob/v4.4.0/include/zephyr/kernel.h).
Its minimal libc likewise describes a 32-bit `int` data model.

The preferred design is an explicitly selected Zephyr ABI. It keeps the
existing 16-bit `int` ABI as the default, makes `int` 32 bits for Zephyr, and
stores a 24-bit hardware address in a 32-bit C pointer representation. The
unused pointer bits must be specified and tested. This ABI needs its own
runtime libraries, calling-convention tests and QEMU tests.

Changing stock Zephyr to support a 16-bit `int` and three-byte pointers would
touch generic kernel and libc assumptions as well as post-link tools. That is
the larger and less maintainable route.

## Object format and linker boundary

Zephyr unconditionally builds an offsets object and extracts absolute symbols
from it. Its generator reads the object with `pyelftools`; subsequent stages
also inspect ELF sections and symbols. See
[`gen_offset_header.py`](https://github.com/zephyrproject-rtos/zephyr/blob/v4.4.0/scripts/build/gen_offset_header.py)
and the official [top-level build](https://github.com/zephyrproject-rtos/zephyr/blob/v4.4.0/CMakeLists.txt).

Consequently, an Intel HEX final image is not enough. The MCS-251 toolchain
must supply:

- ELF32 relocatable objects and static archives;
- target relocation records and a stable ELF machine/ABI definition;
- local, global, weak and absolute symbols;
- arbitrary named code, data, read-only, BSS and metadata sections;
- linker-script placement, section retention and generated boundary symbols;
- a final ELF executable from which Intel HEX can be derived for QEMU.

A late `.ihx`-to-ELF wrapper cannot reconstruct relocations, discarded
symbols or input-section identity. Either the assembler/linker must gain ELF
support, or a lossless conversion must happen before linking.

## Zephyr integration boundary

Zephyr's official [custom toolchain guide](https://docs.zephyrproject.org/latest/develop/toolchains/custom_cmake.html)
allows an out-of-tree toolchain variant with its own compiler, linker,
bintools and `toolchain/other.h`. This is the correct integration point for
SDCC; it avoids pretending that SDCC is GCC.

There is no MCS-51 or MCS-251 architecture in stock Zephyr. The official
[architecture porting guide](https://docs.zephyrproject.org/latest/hardware/porting/arch.html)
requires early boot, interrupt entry/exit, context switching, thread frames,
atomic and IRQ primitives, a system timer, linker integration and stack
alignment. An STC32G144K246 SoC/board definition and the minimal UART driver
needed for test output are required after the architecture port exists.

## Implementation gates

The following order keeps failures attributable and protects MCS-51:

1. Keep positive and negative frontend tests for `gnu11` and `gnu17`. The
   modes set the correct `__STDC_VERSION__` and enable only implemented
   extensions.
2. Continue implementing the required keyword aliases, expressions,
   attributes and builtins in the common frontend. Run the same probes for
   MCS-51 and MCS-251 because they share the parser.
3. Add an opt-in Zephyr ABI and build separate MCS-251 runtime libraries.
   Preserve byte-for-byte output for existing default-ABI regression inputs.
4. Add ELF32 assembly/link support and tests for every relocation, symbol
   binding and section-placement rule used by Zephyr.
5. Add the out-of-tree SDCC toolchain and MCS-251 architecture/SoC/board port
   to a Zephyr downstream.
6. Compile and run progressively larger samples: freestanding C, Zephyr
   `hello_world`, interrupt/timer smoke, cooperative threads, preemptive
   threads and synchronization primitives.
7. Run the full MCS-51 regression suite and the MCS-51 and MCS-251 QEMU tests
   in every CI change. Use QEMU TCG `icount` for deterministic timeouts; use
   UART output for test results rather than treating QEMU as the test oracle.

Accepting the `gnu17` mode describes a tested compatibility subset, not full
GCC compatibility. Zephyr support is considered implemented only after a
stock-baseline Zephyr image is compiled, linked and executed on the MCS-251
QEMU machine.
