# Debugging SDCC MCS251 programs with QEMU

## Current integration status

The QEMU MCS-251 target provides a GDB Remote Serial Protocol endpoint.  It
supports the normal `-S`, `-s`, and `-gdb` options and publishes
`mcs251-core.xml` through `qXfer:features:read`.  The target description
contains 43 registers:

- R0 through R31;
- R56 through R63, the byte positions containing DPX and SPX;
- PSW and PSW1;
- a 32-bit GDB PC whose architectural value is masked to 24 bits.

SDCC's `--debug` option currently emits CDB/ADB debug information rather than
an ELF file with DWARF.  Consequently, QEMU's machine-level GDB endpoint is
usable now, but an unmodified GDB cannot provide complete C source-level
debugging from an SDCC CDB file.  The linker map and CDB labels do provide
the exact 24-bit addresses needed for breakpoints.  Full source-level GDB
support will require either a CDB-to-GDB bridge or an RSP transport and MCS251
stack unwinder in `sdcdb`; the existing `sdcdb` MCS-51 unwinder assumes an
8-bit SP and two-byte return addresses and must not be reused unchanged.

## Automated GDB-stub check

`check-gdbstub.py` builds a program with `--debug`, verifies that `_main` has
the same 24-bit address in the CDB and linker map, starts QEMU halted, reads
the target XML, installs an RSP breakpoint, continues, verifies the PC when
QEMU stops, round-trips R0 through the register-write packet, and performs a
single-instruction step:

```sh
make -C build/src/mcs251 check-gdbstub \
    QEMU_MCS251="$HOME/oss/qemu/builds/build-mcs251/qemu-system-mcs251" \
    MCS251_LIBRARY_DIR=/path/to/device/lib/build/mcs251-small
```

The check deliberately implements the small RSP subset it needs.  It does
not depend on the host having a GDB build with an MCS-251 disassembler.

To retain a bounded startup trace during the same check, invoke the script
directly and add `--trace-log`:

```sh
python3 src/mcs251/tests/check-gdbstub.py \
    --sdcc build/bin/sdcc \
    --qemu "$HOME/oss/qemu/builds/build-mcs251/qemu-system-mcs251" \
    --source src/mcs251/tests/debug-smoke.c \
    --device-include device/include \
    --library-dir build/device/lib/build/mcs251-small \
    --trace-log /tmp/mcs251-startup.trace
```

The end-to-end runtime check can retain a separate bounded trace for every
image with `--trace-dir`:

```sh
python3 src/mcs251/tests/check-qemu.py \
    ...normal source, tool, and board arguments... \
    --runtime-source src/mcs251/tests/runtime-main.c \
    --optimization-runtime-source \
        src/mcs251/tests/optimization-runtime.c \
    --setjmp-spx-source src/mcs251/tests/setjmp-spx-runtime.c \
    --library-dir build/device/lib/build/mcs251-small \
    --stack-auto-library-dir build/device/lib/build/mcs251-small-stack-auto \
    --large-library-dir build/device/lib/build/mcs251-large \
    --large-stack-auto-library-dir \
        build/device/lib/build/mcs251-large-stack-auto \
    --work-dir /tmp/mcs251-images \
    --trace-dir /tmp/mcs251-traces
```

The runner terminates QEMU as soon as it sees a complete `PASS` or `FAIL`
line, so an intentional firmware idle loop does not make these traces grow
without bound.

The regression-port runner accepts the same kind of single-image evidence:

```sh
python3 support/regression/ports/mcs251/run-qemu.py \
    --qemu "$HOME/oss/qemu/builds/build-mcs251/qemu-system-mcs251" \
    --timeout 2 --trace-log /tmp/tst-abs.trace \
    build/support/regression/gen/mcs251-large/tst_abs.hex
```

## Manual QEMU session

Build with SDCC debug records and preserve the map and CDB beside the image:

```sh
sdcc -mmcs251 --debug --no-xinit-opt \
    --code-loc 0xff0000 '-Wl-b GSINIT0=0xfc2800' \
    -o firmware.hex firmware.c
```

Start the CPU before its reset instruction and expose an RSP endpoint:

```sh
qemu-system-mcs251 \
    -M stc32g144k246-evb -bios firmware.hex \
    -display none -monitor none -serial stdio \
    -S -gdb tcp::1234
```

An RSP or GDB client can then connect to port 1234.  Use the `_main` address
from `firmware.map`, or the matching CDB record of this form:

```text
L:G$main$0$0:FC2818
```

RSP register 42 is PC.  Registers 32 through 35 expose R56 through R59
(DPX), and registers 36 through 39 expose R60 through R63 (SPX).  The
hardware stack address is the low 16-bit SPH:SP part of SPX.

## Trace-driven stack diagnosis

For call/return, interrupt-frame, or bad-address failures, enable instruction
and CPU-state logging:

```sh
qemu-system-mcs251 \
    -M stc32g144k246-evb -bios firmware.hex \
    -display none -monitor none -serial stdio \
    -d in_asm,cpu,nochain -D /tmp/mcs251.trace
```

Each trace block records the 24-bit PC, SPX, DPX, PSW/PSW1, and R0-R31 next
to the decoded instruction.  For an exception or corrupted return, work
backwards from the first unexpected PC and compare SPX before and after every
`ECALL`, `ERET`, `PUSH`, `POP`, and interrupt entry.  Use the GDB stub's
memory-read packet to inspect bytes around SPX when the trace alone cannot
show the saved frame.

`cpu,nochain` produces data very quickly, especially after firmware reaches
an intentional infinite loop.  Prefer `-S` plus a breakpoint, a short test
timeout, or the automated check above; stop QEMU as soon as the relevant
state has been captured.

The dedicated `setjmp-spx-{small,large}.hex` images separate the compiler ABI
from indexed-displacement emulation.  They set SPX to `0x0120` without using
a negative indexed operand, call the installed `setjmp`/`longjmp` library,
and require both the restored SPX (`0x0120`) and return value (`0x1234`).  A
trace of the passing image shows SPX at `0x0123` inside the three-byte ECALL
frame, the restore helper rebuilding that frame, and `ERET` resuming with SPX
back at `0x0120`.  A C subtest also keeps values live in R0-R7 across the two
returns.  The same test passes with all four small/large and
default/stack-auto library combinations.

## Confirmed signed-displacement diagnostic

The aggregate-return regression provides a concrete example of this method.
At PC `0xfc2837`, SPX is `0x0039` and the legal instruction is encoded as
`mov r11,@dr60+0xfffb`, i.e. `mov a,@spx-5`.  The intended address is
`0x0034`, where the caller pushed the three-byte hidden result pointer.  A
QEMU implementation that zero-extends `0xfffb` instead reads `0x010034`; the
trace then shows three zero bytes entering DPL/DPH/DPXL and the test prints
`FAIL`.  This distinguishes an emulator displacement bug from an SDCC stack
frame or `ECALL`/`ERET` mismatch.

The ordinary `tst_abs` regression reaches the same diagnosis through a
different call path.  Its formatting code executes
`mov r11,@dr60+0xfffa` with SPX `0x001f`; the architectural address is
`0x0019`, but zero extension selects `0x010019`.  The UART stream then becomes
corrupt before the summary and firmware reaches its normal `__exitEmu` loop.
Seeing that final loop in a trace is therefore not itself a compiler hang:
the first wrong stack-relative load is the useful failure boundary.

For a diagnostic run on a QEMU revision with this bug, the MCS251 regression
port accepts `MCS251_NO_VARARGS=1`.  This restores the framework's historical
`NO_VARARGS` define so that small/large static-model tests which do not need
negative SPX displacements can still be checked:

```sh
make -C build/support/regression test-mcs251 \
    MCS251_NO_VARARGS=1 \
    QEMU_MCS251="$HOME/oss/qemu/builds/build-mcs251/qemu-system-mcs251"
```

This switch is diagnostic only.  It is off by default, and a release-quality
QEMU validation must run the normal variadic lane plus both stack-auto lanes
without it.
