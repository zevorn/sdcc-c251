# Official-example semantic tests

These are original SDCC tests.  They do not contain or compile Arm/Keil or
STC example sources, project files, headers, libraries, or binaries.  Each
case extracts a small observable behavior from a first-party example and
re-expresses it using the SDCC MCS251 ABI and syntax.

| Test | First-party semantic source | QEMU feature |
|---|---|---|
| `memory.c` | [STC32G144K246 internal 148 KiB SRAM test](https://www.stcaimcu.com/thread-24754-1-1.html), [Keil MCB251 RAM test](https://www.keil.com/download/docs/46.asp) | edata, 24-bit xdata, executable-RAM data alias |
| `timer01.c` | [STC32G144K246 timer examples](https://www.stcaimcu.com/thread-24457-1-1.html) | Timer0/1 mode 1, overflow IRQ, MCS251 interrupt return |
| `uart1-echo.c` | [STC32G144K246 UART1 interrupt example](https://www.stcaimcu.com/thread-24465-1-1.html) | UART1 receive/transmit, RI/TI, UART IRQ |
| `gpio-interrupt.c` | [STC32G examples](https://www.stcmicro.com/cn/slcx.html) and [K246 example index](https://www.stcaimcu.com/forum-108-5.html) | GPIO mode/latch/pin behavior and INT0/1 |
| `gpio-modes.c` | [STC32G GPIO examples](https://www.stcmicro.com/cn/slcx.html) and [K246 example index](https://www.stcaimcu.com/forum-108-5.html) | P0-P7 quasi-bidirectional, push-pull, high-impedance, and open-drain pin reads |
| `counter01.c` | [K246 example index](https://www.stcaimcu.com/forum-108-5.html) | P3.4/P3.5 falling edges routed to external Timer0/1 counters |
| `timer-mode2-gate.c` | [STC32G144K246 timer examples](https://www.stcaimcu.com/thread-24457-1-1.html) and [K246 gate topics](https://www.stcaimcu.com/forum-108-5.html) | Timer0/1 mode 2 auto-reload and P3.2/P3.3 gate inputs |
| `rom-checksum.c` | [Keil MCB251 ROM Checksum](https://www.keil.com/download/docs/47.asp) | bounded `__code` traversal through a flat 24-bit code pointer |
| `dsp32.c` | [STC32G DSP instruction examples](https://www.stcaimcu.com/forum-108-5.html) | DSP32 conversion, normalization, exchange, arithmetic, multiply/divide, unary, logical, shift, status, and multiply-accumulate categories |
| `tfpu.c` | [STC32G TFPU examples](https://www.stcaimcu.com/forum-108-5.html) | IEEE-754 arithmetic, conversion, comparison, classification, trigonometry, status/control, and clock-selection commands |

The exact external package hashes, licensing boundary, deferred peripherals,
and clean-room derivation rules are recorded in
`doc/mcs251/official-example-plan.md`.  Expected values come from the hardware
manuals; QEMU is the execution environment, not the oracle.

Additional existing MCS251 conformance tests carry the non-peripheral semantics
from the public Keil set: `far-main.c`/`far-callee.c` cover AN116 cross-region
calls, `memory-model*.c` and the SRAM case cover AN117 plus MCB251 RAM Test,
the startup/runtime tests cover AN114, GPIO output covers the finite behavior
of MCB251 Blinky, and UART/GPIO/SRAM together cover the modeled portions of
MCB251 Diagnostics and Serial I/O.  Packages for peripherals absent from the
QEMU machine remain compile/design inputs rather than falsely passing runtime
tests.
