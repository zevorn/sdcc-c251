#!/usr/bin/env python3

import argparse
import os
from pathlib import Path
import re
import selectors
import subprocess
import sys
import time


sys.path.insert(
    0,
    str(Path(__file__).resolve().parents[4] / "src" / "mcs251" / "tests"),
)
from qemu_trace import capture_instruction_trace


MACHINE_CANDIDATES = ("stc32g144k246", "stc32g144k246-evb")
SUMMARY_RE = re.compile(
    rb"^--- Summary:\s*(x[0-9A-Fa-f]+|[0-9]+)/"
    rb"(x[0-9A-Fa-f]+|[0-9]+)/(x[0-9A-Fa-f]+|[0-9]+)"
    rb"(?::[^\r\n]*)?\r?\n",
    re.MULTILINE,
)
FAILURE_RE = re.compile(rb"^--- FAIL:", re.MULTILINE)


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


def summary_number(value):
    if value[:1].lower() == b"x":
        return int(value[1:], 16)
    return int(value, 10)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--qemu", required=True)
    parser.add_argument("--machine")
    parser.add_argument("image")
    parser.add_argument("--timeout", type=float, default=20.0)
    parser.add_argument("--trace-log")
    parser.add_argument("--trace-dir")
    args = parser.parse_args()

    qemu = Path(args.qemu).resolve()
    image = Path(args.image).resolve()
    for path in (qemu, image):
        if not path.exists():
            parser.error(f"required path does not exist: {path}")
    try:
        machine = resolve_machine(qemu, args.machine)
    except RuntimeError as error:
        parser.error(str(error))

    if args.trace_log and args.trace_dir:
        parser.error("trace-log and trace-dir are mutually exclusive")
    trace_log = Path(args.trace_log).resolve() if args.trace_log else None
    if args.trace_dir:
        trace_dir = Path(args.trace_dir).resolve()
        trace_log = trace_dir / f"{image.stem}.trace"
    command = [
        str(qemu), "-M", machine, "-bios", str(image),
        "-accel", "tcg", "-icount", "shift=0,align=off,sleep=off",
        "-display", "none", "-monitor", "none", "-serial", "stdio",
    ]
    process = subprocess.Popen(
        command,
        stdin=subprocess.DEVNULL,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    output = bytearray()
    selector = selectors.DefaultSelector()
    selector.register(process.stdout, selectors.EVENT_READ)
    deadline = time.monotonic() + args.timeout
    completed = False
    failures = None
    failure_line_seen = False

    try:
        while time.monotonic() < deadline:
            events = selector.select(deadline - time.monotonic())
            if not events:
                break
            chunk = os.read(process.stdout.fileno(), 4096)
            if not chunk:
                break
            output.extend(chunk)
            sys.stdout.buffer.write(chunk)
            sys.stdout.buffer.flush()
            failure_line_seen = FAILURE_RE.search(output) is not None
            summary = SUMMARY_RE.search(output)
            if summary is not None:
                completed = True
                failures = summary_number(summary.group(1))
                break
    finally:
        selector.close()
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=1)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()

    if not completed:
        if trace_log is not None:
            plugin_trace = capture_instruction_trace(
                qemu, command, trace_log
            )
            trace_kind = "execlog plugin" if plugin_trace else "QEMU -d"
            sys.stderr.write(f"{trace_kind} trace: {trace_log}\n")
        if not output:
            sys.stderr.write("MCS251 QEMU produced no regression output\n")
        else:
            sys.stderr.write("MCS251 QEMU stopped before the summary line\n")
        if trace_log is not None:
            sys.stderr.write(f"MCS251 QEMU trace: {trace_log}\n")
        return 1
    if failures != 0:
        if trace_log is not None:
            plugin_trace = capture_instruction_trace(
                qemu, command, trace_log
            )
            trace_kind = "execlog plugin" if plugin_trace else "QEMU -d"
            sys.stderr.write(f"{trace_kind} trace: {trace_log}\n")
        sys.stderr.write(f"MCS251 regression reported {failures} failure(s)\n")
        if trace_log is not None:
            sys.stderr.write(f"MCS251 QEMU trace: {trace_log}\n")
        # The regression framework derives pass/fail from the summary in
        # stdout.  Returning a process failure here makes its generic runner
        # mislabel an ordinary assertion as a simulator timeout and append a
        # second, synthetic summary.
        return 0
    if failure_line_seen:
        sys.stderr.write(
            "MCS251 regression printed a failure line despite a zero summary\n"
        )
        if trace_log is not None:
            sys.stderr.write(f"MCS251 QEMU trace: {trace_log}\n")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
