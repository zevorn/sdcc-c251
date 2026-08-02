# SDCC MCS-251 downstream

[![SDCC MCS-51 family CI](https://github.com/zevorn/sdcc-c251/actions/workflows/mcs51-family.yml/badge.svg?branch=main)](https://github.com/zevorn/sdcc-c251/actions/workflows/mcs51-family.yml)

This repository adds an experimental MCS-251 target to the Small Device C
Compiler (SDCC) while preserving the mature MCS-51 target. The compiler target
names are `mcs51` and `mcs251`, selected with `-mmcs51` and `-mmcs251`.

The upstream documentation index remains in
[`doc/README.txt`](doc/README.txt). MCS-251 target notes are collected under
[`doc/mcs251/`](doc/mcs251/README.md), and the parallel Simplified Chinese
manual is under [`doc/zh-CN/`](doc/zh-CN/README.md). All documentation stays in
the original `doc/` tree.

The exact MCS-251 instruction coverage, positive and negative tests, and
current compiler-selection limits are recorded in the
[`MCS-251 instruction-set support`](doc/mcs251/instruction-set.md) document.

## Build from source

Out-of-tree builds keep generated files out of the source checkout. MCS-51 is
enabled by default; MCS-251 must be enabled explicitly.

### Dependencies

On Debian or Ubuntu:

```sh
sudo apt-get update
sudo apt-get install build-essential bison flex gputils libboost-dev \
    libreadline-dev texinfo zlib1g-dev
```

On macOS:

```sh
xcode-select --install
brew install bison boost flex gputils gnu-sed readline texinfo
export PATH="$(brew --prefix bison)/bin:$(brew --prefix flex)/bin:$PATH"
```

### Configure and build

```sh
git clone https://github.com/zevorn/sdcc-c251.git
cd sdcc-c251
mkdir build
cd build
CFLAGS=-std=gnu17 ../configure \
    --enable-mcs251-port \
    --prefix="$PWD/install"
make -j4
make install
```

On macOS, also add Homebrew's Boost include directory:

```sh
CFLAGS=-std=gnu17 \
CPPFLAGS="-I$(brew --prefix boost)/include" \
../configure \
    --enable-mcs251-port \
    --prefix="$PWD/install"
```

`CFLAGS=-std=gnu17` selects the host compiler dialect. It is needed because
some inherited ASxxxx sources still use K&R function definitions rejected by
C23; it does not change the C dialect accepted by SDCC.

The default configuration also builds the other maintained SDCC targets. Use
`../configure --help` and the corresponding `--disable-<target>-port` options
for a smaller target-specific build.

## Verify the compiler

Confirm that both compiler targets were built:

```sh
install/bin/sdcc --version
grep -E '^(mcs51|mcs251)$' ports.build
```

Run the MCS-251 instruction, compiler, code-generation and diagnostic checks,
then compile the MCS-51 regression source through the shared code paths:

```sh
make -C sdas/as251 check
make -C src/mcs251 check
make -C support/valdiag test-mcs251
bin/sdcc -mmcs51 -I../device/include -S \
    -o /tmp/mcs51-smoke.asm \
    ../src/mcs251/tests/mcs51-regression.c
```

A complete MCS-251 library build produces four ABI-matched directories under
`device/lib/build/`:

- `mcs251-small`
- `mcs251-small-stack-auto`
- `mcs251-large`
- `mcs251-large-stack-auto`

## GNU syntax compatibility

MCS-251 uses the common SDCC C frontend, so its language syntax support is the
same as MCS-51. Strict ISO C17 source is accepted with `--std=c17`, and the
preprocessor defines `__STDC_VERSION__` as `201710L`. C17 is a defect-correction
revision of C11 rather than a new syntax revision. SDCC also provides ISO C
modes and SDCC extension modes. This downstream additionally accepts
`--std=gnu11` and `--std=gnu17`; these modes select the corresponding ISO base
revision and enable only the GNU extensions implemented by the frontend. They
do not imply complete GCC compatibility. The default remains the SDCC-extended
C11 mode. Select another mode with, for example, `--std=sdcc23` or
`--std=c23`.

Do not confuse the host-build setting `CFLAGS=-std=gnu17` with the language
mode used for firmware. That `CFLAGS` value only tells GCC or Clang how to
compile SDCC itself. The firmware language mode is passed directly to `sdcc`,
for example `sdcc -mmcs251 --std=gnu17 source.c`.

The current GNU C compatibility boundary is:

- `sdcpp` is based on GNU CPP, but this does not make the C parser
  GCC-compatible.
- `__typeof(type-or-expression)` and `__typeof__(type-or-expression)` are
  accepted, although nontrivial expressions still have implementation limits.
- `__builtin_types_compatible_p(type1, type2)` is accepted in GNU11 and
  GNU17 modes and folds to an integer constant expression. It observes C type
  compatibility for qualifiers, arrays, function prototypes, structures and
  enumerations.
- `__builtin_expect(expression, expected)` is accepted in GNU11 and GNU17.
  Both integer arguments are converted to `long` and evaluated once, and the
  result is the value of `expression`. A constant expectation supplies a
  90/10 branch probability to the common optimizer; it does not change
  program semantics.
- `__builtin_constant_p(expression)` is accepted in GNU11 and GNU17 and
  folds to an integer constant expression. It conservatively reports whether
  the common frontend can prove the operand constant. The operand is analyzed
  but never evaluated; volatile accesses, assignments, increments and
  function calls produce zero.
- The MCS-51 and MCS-251 targets provide the GNU `__builtin_clz*`,
  `__builtin_ctz*`, `__builtin_popcount*` and `__builtin_ffs*` families in
  GNU11 and GNU17. Constant arguments fold in the frontend; dynamic arguments
  are evaluated once and use ABI-matched runtime helpers for `int`, `long` and
  `long long`. As in GNU C, `clz` and `ctz` have undefined results for zero,
  while `ffs` returns zero for zero.
- The same targets provide `__builtin_bswap16`, `__builtin_bswap32` and
  `__builtin_bswap64` with the GNU `uint16_t`, `uint32_t` and `uint64_t`
  signatures. Constant arguments fold in the frontend; dynamic arguments are
  evaluated once and use width-matched runtime helpers.
- The MCS-51 and MCS-251 targets provide the type-generic
  `__builtin_add_overflow`, `__builtin_sub_overflow` and
  `__builtin_mul_overflow` operations in GNU11 and GNU17. They perform the
  calculation with infinite precision, store the wrapped result in a writable
  standard integer object, and return `_Bool` to report whether the exact
  result fits. Each argument is evaluated once, and the runtime helper writes
  the result in the selected target's byte order.
- The same targets also provide all 18 GNU typed add, subtract and multiply
  overflow builtins for signed and unsigned `int`, `long` and `long long`.
  These are the `sadd`, `uadd`, `ssub`, `usub`, `smul` and `umul` families,
  with the `l` and `ll` suffixes. The two operands are converted to the type
  named by the builtin before exact arithmetic; the result pointer must name
  that exact writable integer type. Arguments are evaluated once, and
  `__has_builtin` reports each supported spelling in GNU11 and GNU17.
- `__has_builtin(name)` reports the builtins accepted by the selected
  frontend mode. Unknown names produce zero. `__builtin_offsetof` and
  `__builtin_unreachable` are available in every mode; the GNU-only queries
  and MCS builtins above are reported only in GNU11 and GNU17.
- Statement expressions `({ ... })` are accepted in GNU11 and GNU17. The
  final expression statement supplies the value and type; an empty block or
  a block ending in another statement has type `void`. Declarations and side
  effects retain their normal block scope and source order.
- `__auto_type` is accepted in GNU11 and GNU17. A declaration must contain
  one identifier declarator with an initializer. The initializer determines
  the object type and is evaluated once; the declared name enters scope only
  after its initializer.
- The driver defines target data-model macros such as `__SIZEOF_INT__`,
  `__SIZEOF_POINTER__`, `__INT32_TYPE__`, `__INTPTR_TYPE__` and the integer
  constant macros. Host preprocessor ABI and architecture macros are removed.
  MCS-51 reports little-endian layout; native MCS-251 reports big-endian
  layout. Both keep a 16-bit `int` and a three-byte generic pointer. MCS-51
  retains a 16-bit `size_t`; MCS-251 ABI revision 2 uses a 32-bit `size_t`
  so object-size calculations cover its complete flat address space.
- `__asm__("instruction")` accepts a basic literal using `sdas251`/ASxxxx
  instruction syntax. GCC extended-asm operands and constraints are not
  accepted.
- `case low ... high` is accepted with `--std=sdcc2y`; the default language
  mode rejects it.
- A zero-length trailing array is accepted when it follows another structure
  member. Prefer a standard flexible array in portable code.
- Common `__attribute__((...))` placements are parsed directly, but most
  attribute names are currently syntax-only.
- Nested functions, labels as values and computed `goto` are not supported.
- Keyword aliases including `__inline__`, `__signed__`, `__const__`,
  `__restrict__` and `__volatile__` are accepted. `__extension__` remains
  unsupported.

The `gcc_attr.h` compatibility header only translates attribute syntax. The
frontend implements the C23 `nodiscard`, `maybe_unused`, `deprecated` and
`fallthrough` attributes; other translated GCC attribute names can be warned
about and ignored. In particular, this mechanism is not a promise that
`packed`, `section`, `cleanup`, `vector_size` or target-specific GCC
attributes have GCC semantics.

MCS-251 assembly output is ASxxxx syntax and is assembled by `sdas251`. There
is currently no GNU assembler (GAS) backend for this target. When porting GCC
code, isolate extensions behind `__SDCC` or `__SDCC_mcs251`, replace extended
inline assembly with SDCC/ASxxxx assembly, and use standard C or SDCC language
extensions for the remaining code.

### Zephyr status and target

The current compiler cannot build stock Zephyr. Zephyr 4.4 defaults to C17,
but its common headers and build pipeline also require GNU-compatible
attributes, builtins, statement expressions, `__typeof__`, section placement,
weak symbols and compiler barriers. The implemented GNU language modes remove
part of this frontend gap, including type-compatibility queries and statement
expressions, inferred object types, branch-expectation hints and
constant-expression, bit-counting, byte-swapping, generic overflow and typed
overflow queries; they are not by themselves Zephyr support.

There are two additional compatibility boundaries outside the C parser:

- The normal MCS-251 ABI has a 16-bit `int`, a 32-bit `size_t` and a 24-bit
  pointer. Zephyr requires a 32-bit `int`, and its ELF post-link tools treat
  pointers as either 32 or 64 bits. A Zephyr ABI must be separately selectable
  so the existing MCS-251 and MCS-51 ABIs do not change.
- `sdas251` and `sdld` currently produce ASxxxx `.rel` objects and Intel HEX
  images. Zephyr requires relocatable ELF objects, named sections, archives,
  relocations, symbol tables and a final ELF executable for its generated
  offsets and other post-link processing.

The compatibility target is the GNU C subset required by Zephyr 4.4, with C17
as the first acceptance baseline. Completion also requires an SDCC toolchain
definition and MCS-251 architecture, SoC and board ports in a Zephyr
downstream. The work is complete only when a Zephyr image is compiled, linked
and run on the MCS-251 QEMU target while the existing MCS-51 and MCS-251
regressions remain green. See the
[`Zephyr and GNU C compatibility assessment`](doc/mcs251/zephyr-gnu-compatibility.md)
for the detailed requirements and implementation gates.

## Validate SDCC with QEMU

QEMU is only the execution backend for firmware generated by SDCC. The test
cases, expected results and pass/fail decisions all come from this SDCC
repository. The runtime tests use the MCS-51 family system emulators maintained
in [`processmission/qemu`](https://github.com/processmission/qemu). QEMU
provides `qemu-system-mcs51` with the `stc8g1k08a` machine and
`qemu-system-mcs251` with the `stc32g144k246` machine.

Install the additional QEMU build dependencies on Debian or Ubuntu:

```sh
sudo apt-get install libglib2.0-dev libpixman-1-dev ninja-build \
    pkg-config python3-venv
```

Clone and build the two emulators next to the SDCC checkout:

```sh
cd ../..
git clone --branch devel https://github.com/processmission/qemu.git
cd qemu
./configure \
    --target-list=mcs51-softmmu,mcs251-softmmu \
    --disable-docs \
    --disable-werror
ninja -C build qemu-system-mcs51 qemu-system-mcs251
```

Return to the SDCC build directory and run the minimal runtime checks:

```sh
cd ../sdcc-c251/build
QEMU_BUILD=../../qemu/build

make -C src/mcs251 check-uart-qemu \
    QEMU_MCS251="$QEMU_BUILD/qemu-system-mcs251"
make -C src/mcs251 check-mcs51-qemu \
    QEMU_MCS51="$QEMU_BUILD/qemu-system-mcs51"
```

`check-uart-qemu` builds default and size-optimized MCS-251 firmware and checks
its UART echo protocol. `check-mcs51-qemu` runs an MCS-51 firmware image and
verifies that shared compiler paths still match the upstream SDCC 4.6.0
assembly and image digests. The broader `check-qemu` target remains available
for manual ABI, memory-model, official-example, peripheral and GDB-stub tests;
those hardware-oriented checks are not the primary compiler CI gate.

Run the general compiler regression suites as a broader check:

```sh
make -C support/regression -j4 \
    test-mcs251 \
    test-mcs251-stack-auto \
    test-mcs251-large \
    test-mcs251-large-stack-auto \
    QEMU_MCS251="$QEMU_BUILD/qemu-system-mcs251"

make -C support/regression -j4 test-mcs51
```

The four MCS-251 lanes execute in QEMU. The mature MCS-51 regression suite uses
ucSim for its full set of memory models; `check-mcs51-qemu` supplies the QEMU
runtime and binary-stability cross-check. The inherited generic regression
cases are useful while developing MCS-251, but they are not a CI gate until
their memory-placement expectations have been baselined for the 251 models.

For trace logging and GDB-stub procedures, see
[`doc/mcs251/debugging.md`](doc/mcs251/debugging.md). When a runtime failure is
not explained by generated code or the ABI, check QEMU before changing the
shared MCS-51 implementation.

## Continuous integration

The `SDCC MCS-51 family CI` workflow runs for every pull request targeting
`main` and every push to `main`. It:

1. builds an SDCC configuration containing MCS-51 and MCS-251;
2. checks every recorded MCS-251 instruction form, opcode map, relocation and
   invalid operand case;
3. runs MCS-251 compiler, code-generation and language diagnostics across all
   four memory/stack models;
4. runs the full upstream MCS-51 regression set;
5. builds pinned QEMU execution backends and runs only the MCS-251 UART smoke
   and MCS-51 binary-stability checks;
6. uploads SDCC regression and diagnostic logs when a job fails.

The QEMU revision is recorded in
[`mcs51-family.yml`](.github/workflows/mcs51-family.yml) so a QEMU update is an
explicit, reviewable CI change.

## Download the Windows package

The independent `SDCC Windows package` workflow builds a relocatable 64-bit
Windows ZIP for every pull request and every push to `main`. The package
contains native Windows executables such as `sdcc.exe`, `sdcpp.exe`,
`sdas8051.exe`, `sdas251.exe` and `sdld.exe`, together with the MCS-51 and
MCS-251 headers and runtime libraries.

Open the
[`SDCC Windows package` workflow](https://github.com/zevorn/sdcc-c251/actions/workflows/windows-package.yml),
select a successful run, and download the
`sdcc-mcs251-windows-x64-<commit>.zip` artifact. Extract it to any directory
and either add its `bin` directory to `PATH` or invoke `bin\sdcc.exe` directly.
No MSYS2 installation is required to use the downloaded compiler.

CI verifies the package from ordinary PowerShell, outside the MSYS2 build
shell. It uses the packaged compiler, preprocessor, assemblers, linker,
headers and libraries to produce both MCS-51 and MCS-251 Intel HEX images
before the ZIP is published. Artifacts are retained for 30 days.

## Build the Windows package locally

The Windows package is built with the 64-bit UCRT toolchain supplied by
[MSYS2](https://www.msys2.org/). Install MSYS2, open an **MSYS2 UCRT64**
terminal, and install the same dependencies used by CI:

```sh
pacman -Syu
pacman -S --needed \
    bison flex git make python texinfo \
    mingw-w64-ucrt-x86_64-boost \
    mingw-w64-ucrt-x86_64-toolchain \
    mingw-w64-ucrt-x86_64-zlib
```

If `pacman -Syu` asks to close the terminal, close it, open a new UCRT64
terminal, run `pacman -Syu` again, and then install the dependencies. The
following commands must also run in the UCRT64 terminal, not in the plain
MSYS terminal or PowerShell:

```sh
git config --global core.autocrlf input
git clone https://github.com/zevorn/sdcc-c251.git
cd sdcc-c251
mkdir windows-build
cd windows-build

CFLAGS=-std=gnu17 LDFLAGS=-static ../configure \
    --enable-mcs251-port \
    --prefix=/sdcc \
    --datarootdir=/sdcc \
    'docdir=${datarootdir}/doc' \
    include_dir_suffix=include \
    non_free_include_dir_suffix=non-free/include \
    lib_dir_suffix=lib \
    non_free_lib_dir_suffix=non-free/lib \
    'sdccconf_h_dir_separator=\\' \
    --disable-z80-port \
    --disable-z180-port \
    --disable-r2k-port \
    --disable-r2ka-port \
    --disable-r3ka-port \
    --disable-r4k-port \
    --disable-r5k-port \
    --disable-r6k-port \
    --disable-sm83-port \
    --disable-tlcs90-port \
    --disable-ez80-port \
    --disable-z80n-port \
    --disable-r800-port \
    --disable-ds390-port \
    --disable-ds400-port \
    --disable-pic14-port \
    --disable-pic16-port \
    --disable-hc08-port \
    --disable-s08-port \
    --disable-stm8-port \
    --disable-pdk13-port \
    --disable-pdk14-port \
    --disable-pdk15-port \
    --disable-mos6502-port \
    --disable-mos65c02-port \
    --disable-f8-port \
    --disable-f8l-port \
    --disable-ucsim \
    --disable-sdcdb \
    --disable-non-free

cd ..
```

This configuration builds only the MCS-51 and MCS-251 compiler ports. It
uses static host linking so that the installed executables do not require an
MSYS2 runtime on the destination machine.

Build the compiler and native tools first. Before the device libraries are
built, stage both parts of the native preprocessor in the compiler's build
search directory:

```sh
make -C windows-build -j2 sdcc-base
install -m 755 windows-build/support/cpp/gcc/cpp.exe \
    windows-build/bin/sdcpp.exe
install -m 755 windows-build/support/cpp/gcc/cc1.exe \
    windows-build/bin/cc1.exe
make -C windows-build -j2
```

Install into a staging directory and assemble the relocatable package tree:

```sh
make -C windows-build \
    DESTDIR="$PWD/windows-build/windows-stage" install
mkdir -p windows-package/sdcc-mcs251
cp -a windows-build/windows-stage/sdcc/. \
    windows-package/sdcc-mcs251/
cp README.md COPYING sdas/COPYING3 windows-package/sdcc-mcs251/
```

Keep the complete `sdcc-mcs251` directory: `sdcc.exe` locates the
preprocessor backend, headers and target libraries relative to its own
location. Copying only `bin/sdcc.exe` does not produce a working toolchain.

Open PowerShell in the repository root to verify the package outside MSYS2:

```powershell
$sdcc = Resolve-Path ".\windows-package\sdcc-mcs251\bin\sdcc.exe"
& $sdcc --version
& $sdcc --print-search-dirs

"void main(void) { for (;;) {} }" |
    Set-Content -Encoding ascii -Path .\windows-smoke.c
& $sdcc -mmcs51 --model-small `
    -o .\windows-smoke-mcs51.ihx .\windows-smoke.c
& $sdcc -mmcs251 --model-small `
    -o .\windows-smoke-mcs251.ihx .\windows-smoke.c
```

Finally, create a distributable ZIP from PowerShell:

```powershell
Compress-Archive -Path .\windows-package\sdcc-mcs251 `
    -DestinationPath .\sdcc-mcs251-windows-x64.zip
```

The CI definition in
[`windows-package.yml`](.github/workflows/windows-package.yml) is the
executable reference for this procedure. In addition to the two smoke
commands above, CI verifies all four MCS-251 memory/stack configurations
before publishing the ZIP.

## Build the documentation

The upstream English manual keeps its original LyX layout under `doc/`. Enable
it with `--enable-doc` during configuration and use the normal make target.

Build the Simplified Chinese PDF and HTML manually with:

```sh
make -C doc/zh-CN check all
```

See [`doc/zh-CN/README.md`](doc/zh-CN/README.md) for the Pandoc, XeLaTeX and
CJK font dependencies.
