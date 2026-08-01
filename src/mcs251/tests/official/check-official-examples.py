#!/usr/bin/env python3

import argparse
from contextlib import contextmanager, nullcontext
import os
from pathlib import Path
import socket
import subprocess
import tempfile
import time


MACHINE_CANDIDATES = ("stc32g144k246", "stc32g144k246-evb")
GPIO_QOM_PATH = "/machine/soc/gpio"
QEMU_TRACE_DIR = None


def run(command, *, env):
    subprocess.run(command, check=True, env=env)


def resolve_machine(qemu, requested):
    if requested:
        return requested
    result = subprocess.run(
        [str(qemu), "-machine", "help"],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        text=True, check=False,
    )
    available = {
        line.split()[0] for line in result.stdout.splitlines() if line.strip()
    }
    for candidate in MACHINE_CANDIDATES:
        if candidate in available:
            return candidate
    raise RuntimeError(
        f"{qemu} provides neither supported MCS251 machine: "
        f"{', '.join(MACHINE_CANDIDATES)}"
    )


def connect_unix(path, process, timeout=5.0):
    deadline = time.monotonic() + timeout
    last_error = None
    while time.monotonic() < deadline:
        if process.poll() is not None:
            output = process.communicate()[0].decode(errors="replace")
            raise RuntimeError(f"QEMU exited before creating {path}:\n{output}")
        client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        try:
            client.connect(path)
            return client
        except OSError as error:
            last_error = error
            client.close()
            time.sleep(0.01)
    raise RuntimeError(f"timed out connecting to {path}: {last_error}")


class ByteStream:
    def __init__(self, sock):
        self.sock = sock
        self.data = bytearray()

    def send(self, data):
        self.sock.sendall(data)

    def wait_for(self, marker, timeout=5.0):
        deadline = time.monotonic() + timeout
        while marker not in self.data:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise RuntimeError(
                    f"timed out waiting for {marker!r}; received {bytes(self.data)!r}"
                )
            self.sock.settimeout(remaining)
            try:
                chunk = self.sock.recv(4096)
            except socket.timeout as error:
                raise RuntimeError(
                    f"timed out waiting for {marker!r}; "
                    f"received {bytes(self.data)!r}"
                ) from error
            if not chunk:
                raise RuntimeError(
                    f"serial socket closed before {marker!r}; "
                    f"received {bytes(self.data)!r}"
                )
            self.data.extend(chunk)
        return bytes(self.data)


class QTestClient:
    def __init__(self, sock):
        self.sock = sock
        self.buffer = bytearray()
        self.events = []

    def _line(self, timeout):
        deadline = time.monotonic() + timeout
        while b"\n" not in self.buffer:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TimeoutError
            self.sock.settimeout(remaining)
            chunk = self.sock.recv(4096)
            if not chunk:
                raise RuntimeError("qtest socket closed")
            self.buffer.extend(chunk)
        line, _, rest = self.buffer.partition(b"\n")
        self.buffer = bytearray(rest)
        return line.decode(errors="replace")

    def command(self, command, timeout=3.0):
        self.sock.sendall(command.encode() + b"\n")
        deadline = time.monotonic() + timeout
        while True:
            line = self._line(max(0.001, deadline - time.monotonic()))
            if line.startswith("IRQ "):
                self.events.append(line)
            elif line.startswith("OK"):
                return line
            elif line.startswith("FAIL"):
                raise RuntimeError(f"qtest command failed: {command}: {line}")

    def set_gpio(self, pin, level):
        self.command(
            f"set_irq_in {GPIO_QOM_PATH} gpio-in {pin} {level}"
        )

    def pulse_falling(self, pin):
        self.set_gpio(pin, 1)
        self.set_gpio(pin, 0)

    def drain(self, timeout=0.1):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            try:
                line = self._line(deadline - time.monotonic())
            except (TimeoutError, socket.timeout):
                return
            if line.startswith("IRQ "):
                self.events.append(line)


@contextmanager
def qemu_session(qemu, machine, image, socket_dir, name, with_qtest=False):
    uart_path = str(socket_dir / f"{name}.uart.sock")
    # Keep timer-backed peripheral cases independent of host scheduling.
    command = [
        str(qemu), "-M", machine, "-bios", str(image),
        "-accel", "tcg", "-icount", "shift=0,align=off,sleep=off",
        "-display", "none", "-monitor", "none",
        "-chardev", f"socket,id=uart,path={uart_path},server=on,wait=on",
        "-serial", "chardev:uart",
    ]
    qtest_path = None
    if with_qtest:
        qtest_path = str(socket_dir / f"{name}.qtest.sock")
        command.extend([
            "-qtest", f"unix:{qtest_path},server=on,wait=off",
        ])
    if QEMU_TRACE_DIR is not None:
        command.extend([
            "-d", "in_asm,cpu",
            "-D", str(QEMU_TRACE_DIR / f"{name}.trace"),
        ])

    process = subprocess.Popen(
        command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    )
    uart_socket = connect_unix(uart_path, process)
    qtest_socket = connect_unix(qtest_path, process) if qtest_path else None
    try:
        yield process, ByteStream(uart_socket), (
            QTestClient(qtest_socket) if qtest_socket else None
        )
    finally:
        uart_socket.close()
        if qtest_socket:
            qtest_socket.close()
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=1)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()


def build_case(sdcc, source, output, include_dir, device_include,
               library_dir, cflags, env):
    run([
        str(sdcc), "-mmcs251", "--no-xinit-opt", "--code-loc", "0xff0000",
        f"-I{include_dir}", f"-I{device_include}",
        f"-I{device_include / 'mcs51'}", f"-L{library_dir}", *cflags,
        "-o", str(output), str(source),
    ], env=env)


def run_simple(qemu, machine, image, work_dir, session_name, signature):
    with qemu_session(
            qemu, machine, image, work_dir, session_name
    ) as (_, serial, _):
        output = serial.wait_for(signature)
        if b"FAIL" in output:
            raise RuntimeError(f"{session_name} emitted FAIL: {output!r}")


def run_uart(qemu, machine, image, work_dir, session_name):
    payload = bytes((0x00, 0x7f, 0x80, 0xff))
    with qemu_session(
            qemu, machine, image, work_dir, session_name
    ) as (_, serial, _):
        serial.wait_for(b"READY:official-uart1\n")
        before = len(serial.data)
        serial.send(payload)
        output = serial.wait_for(b"CASE:official-uart1:PASS\n")
        if payload not in output[before:] or b"FAIL" in output:
            raise RuntimeError(f"UART1 echo mismatch: {output!r}")


def run_gpio(qemu, machine, image, work_dir, session_name):
    with qemu_session(
            qemu, machine, image, work_dir, session_name, with_qtest=True
    ) as (_, serial, qtest):
        qtest.command(f"irq_intercept_out {GPIO_QOM_PATH} gpio-out")
        serial.wait_for(b"READY:official-gpio-output\n")
        qtest.drain()
        qtest.events.clear()
        serial.send(b"G")
        serial.wait_for(b"DONE:official-gpio-output\n")
        qtest.drain()
        required_events = {
            "IRQ raise 48", "IRQ lower 48",
            "IRQ raise 49", "IRQ lower 49",
        }
        missing = required_events.difference(qtest.events)
        if missing:
            raise RuntimeError(
                f"missing GPIO output events {sorted(missing)}; "
                f"received {qtest.events}"
            )

        serial.wait_for(b"READY:official-gpio-input-high\n")
        qtest.set_gpio(0, 1)
        serial.send(b"H")
        serial.wait_for(b"READY:official-gpio-input-low\n")
        qtest.set_gpio(0, 0)
        serial.send(b"L")

        serial.wait_for(b"READY:official-gpio-interrupt\n")
        qtest.set_gpio(3 * 8 + 2, 1)
        qtest.set_gpio(3 * 8 + 3, 1)
        qtest.set_gpio(3 * 8 + 2, 0)
        qtest.set_gpio(3 * 8 + 3, 0)
        output = serial.wait_for(b"CASE:official-gpio-interrupt:PASS\n")
        if b"FAIL" in output:
            raise RuntimeError(f"GPIO/interrupt case failed: {output!r}")


def run_gpio_modes(qemu, machine, image, work_dir, session_name):
    patterns = (0x00, 0xff, 0x11, 0x22, 0x44, 0x55, 0xaa, 0xee)
    with qemu_session(
            qemu, machine, image, work_dir, session_name, with_qtest=True
    ) as (_, serial, qtest):
        serial.wait_for(b"READY:official-gpio-modes\n")
        for port, pattern in enumerate(patterns):
            for pin in range(8):
                qtest.set_gpio(port * 8 + pin, (pattern >> pin) & 1)
        serial.send(b"M")
        output = serial.wait_for(b"CASE:official-gpio-modes:PASS\n")
        if b"FAIL" in output:
            raise RuntimeError(f"GPIO mode case failed: {output!r}")


def run_counter(qemu, machine, image, work_dir, session_name):
    with qemu_session(
            qemu, machine, image, work_dir, session_name, with_qtest=True
    ) as (_, serial, qtest):
        serial.wait_for(b"READY:official-counter01\n")
        for _ in range(3):
            qtest.pulse_falling(3 * 8 + 4)
        for _ in range(5):
            qtest.pulse_falling(3 * 8 + 5)
        serial.send(b"C")
        output = serial.wait_for(b"CASE:official-counter01:PASS\n")
        if b"FAIL" in output:
            raise RuntimeError(f"external-counter case failed: {output!r}")


def run_timer_mode2_gate(qemu, machine, image, work_dir, session_name):
    with qemu_session(
            qemu, machine, image, work_dir, session_name, with_qtest=True
    ) as (_, serial, qtest):
        serial.wait_for(b"READY:official-timer-mode2-gate-low\n")
        qtest.set_gpio(3 * 8 + 2, 0)
        qtest.set_gpio(3 * 8 + 3, 0)
        serial.send(b"L")

        serial.wait_for(b"READY:official-timer-mode2-gate0\n")
        qtest.set_gpio(3 * 8 + 2, 1)
        serial.send(b"0")

        serial.wait_for(b"READY:official-timer-mode2-gate1\n")
        qtest.set_gpio(3 * 8 + 3, 1)
        serial.send(b"1")
        output = serial.wait_for(
            b"CASE:official-timer-mode2-gate:PASS\n"
        )
        if b"FAIL" in output:
            raise RuntimeError(f"timer mode-2/gate case failed: {output!r}")


def check_interrupt_vectors(assembly, handlers):
    text = assembly.read_text()
    for handler in handlers:
        if f"\tejmp\t_{handler}\n" not in text:
            raise RuntimeError(
                f"{assembly.name} lacks extended vector jump for {handler}"
            )
        if f"\tljmp\t_{handler}\n" in text:
            raise RuntimeError(
                f"{assembly.name} uses region-limited vector jump for {handler}"
            )


def main():
    global QEMU_TRACE_DIR

    parser = argparse.ArgumentParser()
    parser.add_argument("--sdcc", required=True)
    parser.add_argument("--qemu", required=True)
    parser.add_argument("--machine")
    parser.add_argument("--source-dir", required=True)
    parser.add_argument("--device-include", required=True)
    parser.add_argument("--library-dir", required=True)
    parser.add_argument("--work-dir")
    parser.add_argument("--trace-dir")
    parser.add_argument("--uart-only", action="store_true")
    args = parser.parse_args()

    sdcc = Path(args.sdcc).resolve()
    qemu = Path(args.qemu).resolve()
    source_dir = Path(args.source_dir).resolve()
    device_include = Path(args.device_include).resolve()
    library_dir = Path(args.library_dir).resolve()
    if args.trace_dir:
        QEMU_TRACE_DIR = Path(args.trace_dir).resolve()
        QEMU_TRACE_DIR.mkdir(parents=True, exist_ok=True)
    required = (sdcc, qemu, source_dir, device_include, library_dir)
    for path in required:
        if not path.exists():
            parser.error(f"required path does not exist: {path}")
    try:
        machine = resolve_machine(qemu, args.machine)
    except RuntimeError as error:
        parser.error(str(error))

    env = os.environ.copy()
    env["PATH"] = f"{sdcc.parent}{os.pathsep}{env.get('PATH', '')}"
    if args.work_dir:
        work_dir = Path(args.work_dir).resolve()
        work_dir.mkdir(parents=True, exist_ok=True)
        workspace = nullcontext(work_dir)
    else:
        workspace = tempfile.TemporaryDirectory(
            prefix="sdcc-mcs251-official-examples."
        )

    cases = {
        "memory": (b"CASE:official-sram:PASS\n", ()),
        "timer01": (
            b"CASE:official-timer01:PASS\n",
            ("timer0_interrupt", "timer1_interrupt"),
        ),
        "uart1-echo": (
            b"CASE:official-uart1:PASS\n", ("uart1_interrupt",),
        ),
        "gpio-interrupt": (
            b"CASE:official-gpio-interrupt:PASS\n",
            ("int0_interrupt", "int1_interrupt"),
        ),
        "gpio-modes": (b"CASE:official-gpio-modes:PASS\n", ()),
        "counter01": (b"CASE:official-counter01:PASS\n", ()),
        "timer-mode2-gate": (
            b"CASE:official-timer-mode2-gate:PASS\n",
            ("timer0_interrupt", "timer1_interrupt"),
        ),
        "rom-checksum": (b"CASE:official-rom-checksum:PASS\n", ()),
        "dsp32": (b"CASE:official-dsp32:PASS\n", ()),
        "tfpu": (b"CASE:official-tfpu:PASS\n", ()),
    }
    if args.uart_only:
        cases = {"uart1-echo": cases["uart1-echo"]}
    lanes = (("default", ()), ("size", ("--opt-code-size",)))

    socket_parent = Path("/tmp") if Path("/tmp").is_dir() else None
    socket_workspace = tempfile.TemporaryDirectory(
        prefix="mcs251-qemu.", dir=socket_parent,
    )

    with workspace as tmp, socket_workspace as socket_tmp:
        output_dir = Path(tmp)
        socket_dir = Path(socket_tmp)
        for lane, cflags in lanes:
            for stem, (_, handlers) in cases.items():
                source = source_dir / f"{stem}.c"
                image = output_dir / f"{lane}-{stem}.hex"
                build_case(
                    sdcc, source, image, source_dir, device_include,
                    library_dir, cflags, env,
                )
                if handlers:
                    check_interrupt_vectors(image.with_suffix(".asm"), handlers)

            if args.uart_only:
                run_uart(
                    qemu, machine,
                    output_dir / f"{lane}-uart1-echo.hex", socket_dir,
                    f"{lane}-uart1-echo",
                )
                continue

            run_simple(
                qemu, machine, output_dir / f"{lane}-memory.hex", socket_dir,
                f"{lane}-memory", cases["memory"][0],
            )
            run_simple(
                qemu, machine, output_dir / f"{lane}-timer01.hex", socket_dir,
                f"{lane}-timer01", cases["timer01"][0],
            )
            run_uart(
                qemu, machine, output_dir / f"{lane}-uart1-echo.hex", socket_dir,
                f"{lane}-uart1-echo",
            )
            run_gpio(
                qemu, machine, output_dir / f"{lane}-gpio-interrupt.hex", socket_dir,
                f"{lane}-gpio-interrupt",
            )
            run_gpio_modes(
                qemu, machine, output_dir / f"{lane}-gpio-modes.hex", socket_dir,
                f"{lane}-gpio-modes",
            )
            run_counter(
                qemu, machine, output_dir / f"{lane}-counter01.hex", socket_dir,
                f"{lane}-counter01",
            )
            run_timer_mode2_gate(
                qemu, machine,
                output_dir / f"{lane}-timer-mode2-gate.hex", socket_dir,
                f"{lane}-timer-mode2-gate",
            )
            run_simple(
                qemu, machine, output_dir / f"{lane}-rom-checksum.hex", socket_dir,
                f"{lane}-rom-checksum", cases["rom-checksum"][0],
            )
            run_simple(
                qemu, machine, output_dir / f"{lane}-dsp32.hex", socket_dir,
                f"{lane}-dsp32", cases["dsp32"][0],
            )
            run_simple(
                qemu, machine, output_dir / f"{lane}-tfpu.hex", socket_dir,
                f"{lane}-tfpu", cases["tfpu"][0],
            )

    if args.uart_only:
        print("MCS251 UART smoke: 2 optimization lanes passed")
    else:
        print(
            "MCS251 official-example semantics: "
            "10 cases x 2 optimization lanes passed"
        )


if __name__ == "__main__":
    main()
