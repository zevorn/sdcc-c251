#!/usr/bin/env python3

import argparse
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from qemu_trace import capture_instruction_trace


MACHINE_CANDIDATES = ("stc8g1k08a", "stc8g1k08a-evb")
LONGLONG_SYMBOLS = (
    "__mullonglong",
    "__divulonglong",
    "__modulonglong",
    "__divslonglong",
    "__modslonglong",
)


def run(command, *, env=None):
    subprocess.run(command, check=True, env=env)


def build(
    sdcc, source, device_include, library_dir, output_dir, stem,
    extra_flags=(),
):
    env = os.environ.copy()
    env["PATH"] = f"{sdcc.parent}{os.pathsep}{env.get('PATH', '')}"
    assembly = output_dir / f"{stem}.asm"
    image = output_dir / f"{stem}.hex"

    run([
        str(sdcc), "-mmcs51", *extra_flags,
        f"-I{device_include}", f"-I{device_include / 'mcs51'}",
        "-S", "-o", str(assembly), str(source),
    ], env=env)
    run([
        str(sdcc), "-mmcs51", *extra_flags, f"-I{device_include}",
        f"-I{device_include / 'mcs51'}", f"-L{library_dir}",
        "-o", str(image), str(source),
    ], env=env)
    return assembly, image


def build_longlong(sdcc, source, device_include, library_dir, output_dir):
    env = os.environ.copy()
    env["PATH"] = f"{sdcc.parent}{os.pathsep}{env.get('PATH', '')}"
    image = output_dir / "longlong.hex"

    run([
        str(sdcc), "-mmcs51", "--stack-auto", "--no-xinit-opt",
        f"-I{device_include}", f"-I{device_include / 'mcs51'}",
        f"-L{library_dir}", "-o", str(image), str(source),
    ], env=env)
    link_map = image.with_suffix(".map").read_text()
    for symbol in LONGLONG_SYMBOLS:
        if symbol not in link_map:
            raise SystemExit(
                f"MCS-51 64-bit image did not link {symbol}"
            )
    return image


def resolve_machine(qemu, requested):
    if requested:
        return requested
    result = subprocess.run(
        [str(qemu), "-machine", "help"],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        text=True, errors="replace", check=False,
    )
    available = {
        line.split()[0] for line in result.stdout.splitlines() if line.strip()
    }
    for candidate in MACHINE_CANDIDATES:
        if candidate in available:
            return candidate
    raise RuntimeError(
        f"{qemu} provides neither supported MCS-51 machine: "
        f"{', '.join(MACHINE_CANDIDATES)}"
    )


def run_qemu(qemu, machine, image, trace_log=None):
    command = [
        str(qemu), "-M", machine, "-bios", str(image),
        "-accel", "tcg", "-icount", "shift=0,align=off,sleep=off",
        "-display", "none", "-monitor", "none", "-serial", "stdio",
    ]
    process = subprocess.Popen(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    try:
        output, _ = process.communicate(timeout=3)
    except subprocess.TimeoutExpired as error:
        output = error.output or b""
        process.terminate()
        try:
            completed_output, _ = process.communicate(timeout=1)
        except subprocess.TimeoutExpired:
            process.kill()
            completed_output, _ = process.communicate()
        if completed_output:
            output = completed_output

    normalized = output.replace(b"\r\n", b"\n")
    if b"PASS\n" not in normalized or b"FAIL" in normalized:
        if trace_log is not None:
            plugin_trace = capture_instruction_trace(
                qemu, command, trace_log
            )
            trace_kind = "execlog plugin" if plugin_trace else "QEMU -d"
            sys.stderr.write(f"{trace_kind} trace: {trace_log}\n")
        sys.stderr.buffer.write(output)
        raise SystemExit(f"MCS-51 QEMU regression failed for {image.name}")
    sys.stdout.buffer.write(output)


def normalized_assembly(path):
    lines = path.read_text().splitlines()
    return "\n".join(
        re.sub(
            r"^(;\s*).*[\\/](?=[^\\/]+\.c:[0-9]+:)", r"\1", line
        ) for line in lines
        if not line.startswith("; Version ")
    ) + "\n"


def read_baseline_digests(path):
    digests = {}
    for line in path.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        name, digest = line.split()
        digests[name] = digest.lower()
    if set(digests) != {"assembly", "image"}:
        raise RuntimeError(
            f"{path} must contain assembly and image SHA-256 digests"
        )
    return digests


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sdcc", required=True)
    parser.add_argument("--qemu", required=True)
    parser.add_argument("--machine")
    parser.add_argument("--source", required=True)
    parser.add_argument("--statement-expression-source", required=True)
    parser.add_argument("--bit-builtins-source", required=True)
    parser.add_argument("--byte-swap-builtins-source", required=True)
    parser.add_argument("--size-type-source", required=True)
    parser.add_argument("--register-pressure-source", required=True)
    parser.add_argument("--overflow-builtins-source", required=True)
    parser.add_argument("--typed-overflow-builtins-source", required=True)
    parser.add_argument("--longlong-source", required=True)
    parser.add_argument("--device-include", required=True)
    parser.add_argument("--library-dir", required=True)
    parser.add_argument("--stack-auto-library-dir", required=True)
    parser.add_argument("--baseline-digest")
    parser.add_argument("--baseline-sdcc")
    parser.add_argument("--baseline-library-dir")
    parser.add_argument("--trace-dir")
    args = parser.parse_args()

    sdcc = Path(args.sdcc).resolve()
    qemu = Path(args.qemu).resolve()
    source = Path(args.source).resolve()
    statement_expression_source = \
        Path(args.statement_expression_source).resolve()
    bit_builtins_source = Path(args.bit_builtins_source).resolve()
    byte_swap_builtins_source = Path(
        args.byte_swap_builtins_source
    ).resolve()
    size_type_source = Path(args.size_type_source).resolve()
    register_pressure_source = \
        Path(args.register_pressure_source).resolve()
    overflow_builtins_source = Path(args.overflow_builtins_source).resolve()
    typed_overflow_builtins_source = Path(
        args.typed_overflow_builtins_source
    ).resolve()
    longlong_source = Path(args.longlong_source).resolve()
    device_include = Path(args.device_include).resolve()
    library_dir = Path(args.library_dir).resolve()
    stack_auto_library_dir = Path(args.stack_auto_library_dir).resolve()
    trace_dir = Path(args.trace_dir).resolve() if args.trace_dir else None
    required = (
        sdcc, qemu, source, statement_expression_source,
        bit_builtins_source, longlong_source, device_include, library_dir,
        byte_swap_builtins_source,
        size_type_source,
        register_pressure_source,
        overflow_builtins_source, stack_auto_library_dir,
        typed_overflow_builtins_source,
    )
    for path in required:
        if not path.exists():
            parser.error(f"required path does not exist: {path}")
    try:
        machine = resolve_machine(qemu, args.machine)
    except RuntimeError as error:
        parser.error(str(error))

    if bool(args.baseline_sdcc) != bool(args.baseline_library_dir):
        parser.error("baseline SDCC and library directory must be used together")
    baseline_digest = None
    if args.baseline_digest:
        baseline_digest = Path(args.baseline_digest).resolve()
        if not baseline_digest.exists():
            parser.error(
                f"baseline digest does not exist: {baseline_digest}"
            )

    with tempfile.TemporaryDirectory(prefix="sdcc-mcs51-qemu.") as tmp:
        output_dir = Path(tmp)
        if trace_dir is not None:
            trace_dir.mkdir(parents=True, exist_ok=True)

        def trace_for(image_path):
            if trace_dir is None:
                return None
            return trace_dir / f"{image_path.stem}.trace"

        assembly, image = build(
            sdcc, source, device_include, library_dir, output_dir, "current",
        )
        run_qemu(qemu, machine, image, trace_for(image))

        if baseline_digest is not None:
            expected = read_baseline_digests(baseline_digest)
            actual_assembly = hashlib.sha256(
                normalized_assembly(assembly).encode()
            ).hexdigest()
            actual_image = hashlib.sha256(image.read_bytes()).hexdigest()
            if actual_assembly != expected["assembly"]:
                raise SystemExit(
                    "modified compiler changed MCS-51 assembly versus "
                    "the upstream 4.6.0 digest"
                )
            if actual_image != expected["image"]:
                raise SystemExit(
                    "modified toolchain changed MCS-51 Intel HEX versus "
                    "the upstream 4.6.0 digest"
                )

        if args.baseline_sdcc:
            baseline_sdcc = Path(args.baseline_sdcc).resolve()
            baseline_library_dir = Path(args.baseline_library_dir).resolve()
            for path in (baseline_sdcc, baseline_library_dir):
                if not path.exists():
                    parser.error(f"baseline path does not exist: {path}")
            baseline_assembly, baseline_image = build(
                baseline_sdcc, source, device_include, baseline_library_dir,
                output_dir, "baseline",
            )
            if normalized_assembly(assembly) != normalized_assembly(
                    baseline_assembly):
                raise SystemExit(
                    "modified compiler changed MCS-51 assembly versus baseline"
                )
            if image.read_bytes() != baseline_image.read_bytes():
                raise SystemExit(
                    "modified toolchain changed MCS-51 Intel HEX versus baseline"
                )
            run_qemu(
                qemu, machine, baseline_image, trace_for(baseline_image)
            )

        if baseline_digest is not None:
            print("MCS-51 upstream 4.6.0 digest comparison passed")

        statement_configurations = (
            ("statement-expression", ("--std=gnu17",), library_dir),
            ("statement-expression-stack-auto",
             ("--std=gnu17", "--stack-auto"), stack_auto_library_dir),
        )
        for statement_name, statement_flags, statement_library in \
                statement_configurations:
            _, statement_image = build(
                sdcc, statement_expression_source, device_include,
                statement_library, output_dir, statement_name,
                statement_flags,
            )
            run_qemu(
                qemu, machine, statement_image,
                trace_for(statement_image),
            )

        bit_configurations = (
            ("bit-builtins", ("--std=gnu17",), library_dir),
            ("bit-builtins-stack-auto",
             ("--std=gnu17", "--stack-auto"),
             stack_auto_library_dir),
        )
        for bit_name, bit_flags, bit_library in bit_configurations:
            _, bit_image = build(
                sdcc, bit_builtins_source, device_include,
                bit_library, output_dir, bit_name, bit_flags,
            )
            run_qemu(qemu, machine, bit_image, trace_for(bit_image))

            _, byte_swap_image = build(
                sdcc, byte_swap_builtins_source, device_include,
                bit_library, output_dir, f"byte-swap-{bit_name}",
                bit_flags,
            )
            run_qemu(
                qemu, machine, byte_swap_image,
                trace_for(byte_swap_image),
            )

            _, size_type_image = build(
                sdcc, size_type_source, device_include,
                bit_library, output_dir, f"size-type-{bit_name}",
                bit_flags,
            )
            run_qemu(
                qemu, machine, size_type_image,
                trace_for(size_type_image),
            )

            _, register_pressure_image = build(
                sdcc, register_pressure_source, device_include,
                bit_library, output_dir, f"register-pressure-{bit_name}",
                (*bit_flags, "-DMCS_REGISTER_PRESSURE_RUNTIME"),
            )
            run_qemu(
                qemu, machine, register_pressure_image,
                trace_for(register_pressure_image),
            )

        overflow_configurations = (
            ("overflow-builtins", ("--std=gnu17",), library_dir),
            ("overflow-builtins-stack-auto",
             ("--std=gnu17", "--stack-auto"),
             stack_auto_library_dir),
        )
        for overflow_name, overflow_flags, overflow_library in \
                overflow_configurations:
            _, overflow_image = build(
                sdcc, overflow_builtins_source, device_include,
                overflow_library, output_dir, overflow_name,
                overflow_flags,
            )
            run_qemu(
                qemu, machine, overflow_image, trace_for(overflow_image)
            )

            _, typed_overflow_image = build(
                sdcc, typed_overflow_builtins_source, device_include,
                overflow_library, output_dir, f"typed-{overflow_name}",
                overflow_flags,
            )
            run_qemu(
                qemu, machine, typed_overflow_image,
                trace_for(typed_overflow_image),
            )

        longlong_image = build_longlong(
            sdcc, longlong_source, device_include,
            stack_auto_library_dir, output_dir,
        )
        run_qemu(qemu, machine, longlong_image, trace_for(longlong_image))


if __name__ == "__main__":
    main()
