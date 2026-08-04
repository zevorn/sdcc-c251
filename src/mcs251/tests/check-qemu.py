#!/usr/bin/env python3

import argparse
from contextlib import nullcontext
import os
from pathlib import Path
import selectors
import subprocess
import sys
import tempfile
import time

from qemu_trace import capture_instruction_trace


MACHINE_CANDIDATES = ("stc32g144k246", "stc32g144k246-evb")


def run(command, *, env=None):
    subprocess.run(command, check=True, env=env)


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
        f"{qemu} provides neither supported MCS251 machine: "
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
        stdin=subprocess.DEVNULL,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    output = bytearray()
    selector = selectors.DefaultSelector()
    selector.register(process.stdout, selectors.EVENT_READ)
    deadline = time.monotonic() + 3

    try:
        while time.monotonic() < deadline:
            events = selector.select(deadline - time.monotonic())
            if not events:
                break
            chunk = os.read(process.stdout.fileno(), 4096)
            if not chunk:
                break
            output.extend(chunk)
            normalized = bytes(output).replace(b"\r\n", b"\n")
            if b"PASS\n" in normalized or b"FAIL\n" in normalized:
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

    normalized_output = bytes(output).replace(b"\r\n", b"\n")
    if b"PASS\n" not in normalized_output:
        if trace_log is not None:
            plugin_trace = capture_instruction_trace(
                qemu, command, trace_log
            )
            trace_kind = "execlog plugin" if plugin_trace else "QEMU -d"
            sys.stderr.write(f"{trace_kind} trace: {trace_log}\n")
        sys.stderr.buffer.write(bytes(output))
        raise SystemExit(f"QEMU did not emit PASS for {image.name}")
    if b"FAIL" in normalized_output:
        if trace_log is not None:
            plugin_trace = capture_instruction_trace(
                qemu, command, trace_log
            )
            trace_kind = "execlog plugin" if plugin_trace else "QEMU -d"
            sys.stderr.write(f"{trace_kind} trace: {trace_log}\n")
        sys.stderr.buffer.write(bytes(output))
        raise SystemExit(f"QEMU emitted FAIL for {image.name}")
    return bytes(output)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sdcc", required=True)
    parser.add_argument("--sdas251", required=True)
    parser.add_argument("--qemu", required=True)
    parser.add_argument("--machine")
    parser.add_argument("--main-source", required=True)
    parser.add_argument("--far-source", required=True)
    parser.add_argument("--crt0", required=True)
    parser.add_argument("--runtime-source", required=True)
    parser.add_argument("--statement-expression-source", required=True)
    parser.add_argument("--bit-builtins-source", required=True)
    parser.add_argument("--byte-swap-builtins-source", required=True)
    parser.add_argument("--size-type-source", required=True)
    parser.add_argument("--register-pressure-source", required=True)
    parser.add_argument("--wide-shift-source", required=True)
    parser.add_argument("--overflow-builtins-source", required=True)
    parser.add_argument("--typed-overflow-builtins-source", required=True)
    parser.add_argument("--optimization-runtime-source", required=True)
    parser.add_argument("--abi-regression-source", required=True)
    parser.add_argument("--longlong-source", required=True)
    parser.add_argument("--startup-memory-source", required=True)
    parser.add_argument("--setjmp-spx-source", required=True)
    parser.add_argument("--endianness-runtime-source", required=True)
    parser.add_argument("--aggregate-source", required=True)
    parser.add_argument("--memory-model-main-source", required=True)
    parser.add_argument("--memory-model-source", required=True)
    parser.add_argument("--runtime-cflag", action="append", default=[])
    parser.add_argument("--device-include", required=True)
    parser.add_argument("--library-dir", required=True)
    parser.add_argument("--stack-auto-library-dir", required=True)
    parser.add_argument("--large-library-dir", required=True)
    parser.add_argument("--large-stack-auto-library-dir", required=True)
    parser.add_argument("--work-dir")
    parser.add_argument("--trace-dir")
    args = parser.parse_args()

    sdcc = Path(args.sdcc).resolve()
    sdas251 = Path(args.sdas251).resolve()
    qemu = Path(args.qemu).resolve()
    main_source = Path(args.main_source).resolve()
    far_source = Path(args.far_source).resolve()
    crt0 = Path(args.crt0).resolve()
    runtime_source = Path(args.runtime_source).resolve()
    statement_expression_source = \
        Path(args.statement_expression_source).resolve()
    bit_builtins_source = Path(args.bit_builtins_source).resolve()
    byte_swap_builtins_source = Path(
        args.byte_swap_builtins_source
    ).resolve()
    size_type_source = Path(args.size_type_source).resolve()
    register_pressure_source = \
        Path(args.register_pressure_source).resolve()
    wide_shift_source = Path(args.wide_shift_source).resolve()
    overflow_builtins_source = Path(args.overflow_builtins_source).resolve()
    typed_overflow_builtins_source = Path(
        args.typed_overflow_builtins_source
    ).resolve()
    optimization_runtime_source = \
        Path(args.optimization_runtime_source).resolve()
    abi_regression_source = Path(args.abi_regression_source).resolve()
    longlong_source = Path(args.longlong_source).resolve()
    startup_memory_source = Path(args.startup_memory_source).resolve()
    setjmp_spx_source = Path(args.setjmp_spx_source).resolve()
    endianness_runtime_source = \
        Path(args.endianness_runtime_source).resolve()
    aggregate_source = Path(args.aggregate_source).resolve()
    memory_model_main_source = Path(args.memory_model_main_source).resolve()
    memory_model_source = Path(args.memory_model_source).resolve()
    device_include = Path(args.device_include).resolve()
    library_dir = Path(args.library_dir).resolve()
    stack_auto_library_dir = Path(args.stack_auto_library_dir).resolve()
    large_library_dir = Path(args.large_library_dir).resolve()
    large_stack_auto_library_dir = \
        Path(args.large_stack_auto_library_dir).resolve()
    trace_dir = Path(args.trace_dir).resolve() if args.trace_dir else None

    for path in (sdcc, sdas251, qemu, main_source, far_source, crt0,
                 runtime_source, optimization_runtime_source,
                 statement_expression_source,
                 bit_builtins_source,
                 byte_swap_builtins_source,
                 size_type_source,
                 register_pressure_source,
                 wide_shift_source,
                 overflow_builtins_source,
                 typed_overflow_builtins_source,
                 abi_regression_source,
                 longlong_source,
                 startup_memory_source, setjmp_spx_source,
                 endianness_runtime_source,
                 aggregate_source,
                 memory_model_main_source,
                 memory_model_source, device_include, library_dir,
                 stack_auto_library_dir, large_library_dir,
                 large_stack_auto_library_dir):
        if not path.exists():
            parser.error(f"required path does not exist: {path}")
    try:
        machine = resolve_machine(qemu, args.machine)
    except RuntimeError as error:
        parser.error(str(error))

    env = os.environ.copy()
    env["PATH"] = f"{sdcc.parent}{os.pathsep}{env.get('PATH', '')}"
    board_link_flags = [
        "--code-loc", "0xff0000", "-Wl-b GSINIT0=0xfc2800",
    ]

    if args.work_dir:
        work_dir = Path(args.work_dir).resolve()
        work_dir.mkdir(parents=True, exist_ok=True)
        workspace = nullcontext(work_dir)
    else:
        workspace = tempfile.TemporaryDirectory(prefix="sdcc-mcs251-qemu.")

    with workspace as tmp:
        tmpdir = Path(tmp)

        if trace_dir is not None:
            trace_dir.mkdir(parents=True, exist_ok=True)

        def trace_for(image_path):
            if trace_dir is None:
                return None
            return trace_dir / f"{image_path.stem}.trace"

        main_rel = tmpdir / "far-main.rel"
        far_rel = tmpdir / "far-callee.rel"
        crt_rel = tmpdir / "crt0.rel"
        image = tmpdir / "far-call.hex"
        map_file = image.with_suffix(".map")

        run([
            str(sdcc), "-mmcs251", "--no-xinit-opt", "-c", "-o", str(main_rel),
            str(main_source),
        ], env=env)
        run([
            str(sdcc), "-mmcs251", "--no-xinit-opt", "--codeseg", "FARCODE",
            "-c", "-o", str(far_rel), str(far_source),
        ], env=env)
        run([
            str(sdas251), "-plosgffw", "-o", str(crt_rel), str(crt0),
        ], env=env)
        run([
            str(sdcc), "-mmcs251", "--nostdlib", "--no-xinit-opt",
            *board_link_flags, "-Wl-b FARCODE=0xfe0000",
            f"-L{library_dir}", "mcs251.lib",
            "-o", str(image), str(main_rel), str(far_rel), str(crt_rel),
        ], env=env)

        map_text = map_file.read_text()
        if "00FE0000  _mcs251_far_add_one" not in map_text:
            raise SystemExit("far callee was not linked at 0xfe0000")
        if "00FC2800  __sdcc_gsinit_startup" not in map_text:
            raise SystemExit("startup code was not linked at flash base")
        if "__sdcc_mcs251_reset_trampoline" not in map_text:
            raise SystemExit("reset trampoline is missing from HOME")

        sys.stdout.buffer.write(run_qemu(
            qemu, machine, image, trace_for(image),
        ))

        startup_memory_image = tmpdir / "startup-memory.hex"
        run([
            str(sdcc), "-mmcs251", *board_link_flags,
            f"-I{device_include}", f"-I{device_include / 'mcs51'}",
            f"-L{library_dir}", "-o", str(startup_memory_image),
            str(startup_memory_source),
        ], env=env)
        startup_map = startup_memory_image.with_suffix(".map").read_text()
        for symbol in ("__mcs51_genXINIT", "__mcs51_genXRAMCLEAR"):
            if symbol not in startup_map:
                raise SystemExit(
                    f"startup-memory image did not link {symbol}"
                )
        sys.stdout.buffer.write(run_qemu(
            qemu, machine, startup_memory_image,
            trace_for(startup_memory_image),
        ))

        setjmp_spx_configurations = (
            ("small", (), library_dir),
            ("small-stack-auto", ("--stack-auto",),
             stack_auto_library_dir),
            ("large", ("--model-large",), large_library_dir),
            ("large-stack-auto", ("--model-large", "--stack-auto"),
             large_stack_auto_library_dir),
        )
        for setjmp_name, model_flags, setjmp_library_dir in \
                setjmp_spx_configurations:
            setjmp_spx_image = tmpdir / f"setjmp-spx-{setjmp_name}.hex"
            run([
                str(sdcc), "-mmcs251", *model_flags, "--no-xinit-opt",
                *board_link_flags, f"-I{device_include}",
                f"-I{device_include / 'mcs51'}", f"-L{setjmp_library_dir}",
                "-o", str(setjmp_spx_image), str(setjmp_spx_source),
            ], env=env)
            setjmp_spx_map = \
                setjmp_spx_image.with_suffix(".map").read_text()
            for symbol in ("___setjmp", "_longjmp"):
                if symbol not in setjmp_spx_map:
                    raise SystemExit(
                        f"{setjmp_name} SPX setjmp image did not link {symbol}"
                    )
            sys.stdout.buffer.write(run_qemu(
                qemu, machine, setjmp_spx_image, trace_for(setjmp_spx_image),
            ))

        runtime_configurations = (
            ("small", (), library_dir),
            ("small-stack-auto", ("--stack-auto",),
             stack_auto_library_dir),
            ("large", ("--model-large",), large_library_dir),
            ("large-stack-auto", ("--model-large", "--stack-auto"),
             large_stack_auto_library_dir),
        )
        for runtime_name, model_flags, runtime_library_dir in \
                runtime_configurations:
            runtime_image = tmpdir / f"runtime-{runtime_name}.hex"
            run([
                str(sdcc), "-mmcs251", *model_flags, "--no-xinit-opt",
                *board_link_flags, f"-I{device_include}",
                f"-I{device_include / 'mcs51'}", f"-L{runtime_library_dir}",
                *args.runtime_cflag, "-o", str(runtime_image),
                str(runtime_source),
            ], env=env)

            runtime_map = runtime_image.with_suffix(".map").read_text()
            skipped = set(args.runtime_cflag)
            required_symbols = []
            if "-DMCS251_SKIP_DIVISION" not in skipped:
                required_symbols.append("__divulong")
            if "-DMCS251_SKIP_FLOAT" not in skipped:
                required_symbols.append("_atof")
            if "-DMCS251_SKIP_SETJMP" not in skipped:
                required_symbols.extend(("___setjmp", "_longjmp"))
            if "-DMCS251_SKIP_BSEARCH" not in skipped:
                required_symbols.append("_bsearch")
            if not args.runtime_cflag:
                required_symbols.append("__gptrget")
            for symbol in required_symbols:
                if symbol not in runtime_map:
                    raise SystemExit(
                        f"{runtime_name} runtime image did not link {symbol}"
                    )

            sys.stdout.buffer.write(run_qemu(
                qemu, machine, runtime_image, trace_for(runtime_image),
            ))

        for statement_name, model_flags, statement_library_dir in \
                runtime_configurations:
            statement_image = \
                tmpdir / f"statement-expression-{statement_name}.hex"
            run([
                str(sdcc), "-mmcs251", "--std=gnu17", *model_flags,
                "--no-xinit-opt", *board_link_flags,
                f"-L{statement_library_dir}", "-o", str(statement_image),
                str(statement_expression_source),
            ], env=env)
            sys.stdout.buffer.write(run_qemu(
                qemu, machine, statement_image,
                trace_for(statement_image),
            ))

        for bit_name, model_flags, bit_library_dir in \
                runtime_configurations:
            bit_image = tmpdir / f"bit-builtins-{bit_name}.hex"
            run([
                str(sdcc), "-mmcs251", "--std=gnu17", *model_flags,
                "--no-xinit-opt", *board_link_flags,
                f"-L{bit_library_dir}", "-o", str(bit_image),
                str(bit_builtins_source),
            ], env=env)
            sys.stdout.buffer.write(run_qemu(
                qemu, machine, bit_image, trace_for(bit_image),
            ))

            byte_swap_image = tmpdir / f"byte-swap-{bit_name}.hex"
            run([
                str(sdcc), "-mmcs251", "--std=gnu17", *model_flags,
                "--no-xinit-opt", *board_link_flags,
                f"-I{device_include}", f"-L{bit_library_dir}",
                "-o", str(byte_swap_image),
                str(byte_swap_builtins_source),
            ], env=env)
            sys.stdout.buffer.write(run_qemu(
                qemu, machine, byte_swap_image,
                trace_for(byte_swap_image),
            ))

            size_type_image = tmpdir / f"size-type-{bit_name}.hex"
            run([
                str(sdcc), "-mmcs251", "--std=gnu17", *model_flags,
                "--no-xinit-opt", *board_link_flags,
                f"-I{device_include}", f"-L{bit_library_dir}",
                "-o", str(size_type_image), str(size_type_source),
            ], env=env)
            sys.stdout.buffer.write(run_qemu(
                qemu, machine, size_type_image,
                trace_for(size_type_image),
            ))

            register_pressure_image = \
                tmpdir / f"register-pressure-{bit_name}.hex"
            run([
                str(sdcc), "-mmcs251", "--std=gnu17", *model_flags,
                "--no-xinit-opt", "-DMCS_REGISTER_PRESSURE_RUNTIME",
                *board_link_flags, f"-I{device_include}",
                f"-L{bit_library_dir}", "-o",
                str(register_pressure_image),
                str(register_pressure_source),
            ], env=env)
            sys.stdout.buffer.write(run_qemu(
                qemu, machine, register_pressure_image,
                trace_for(register_pressure_image),
            ))

            wide_shift_image = \
                tmpdir / f"wide-shift-{bit_name}.hex"
            run([
                str(sdcc), "-mmcs251", "--std=gnu17", *model_flags,
                "--opt-code-size", "--no-xinit-opt",
                "-DMCS251_WIDE_SHIFT_RUNTIME", *board_link_flags,
                f"-L{bit_library_dir}", "-o", str(wide_shift_image),
                str(wide_shift_source),
            ], env=env)
            sys.stdout.buffer.write(run_qemu(
                qemu, machine, wide_shift_image,
                trace_for(wide_shift_image),
            ))

        for overflow_name, model_flags, overflow_library_dir in \
                runtime_configurations:
            overflow_image = \
                tmpdir / f"overflow-builtins-{overflow_name}.hex"
            run([
                str(sdcc), "-mmcs251", "--std=gnu17", *model_flags,
                "--no-xinit-opt", *board_link_flags,
                f"-L{overflow_library_dir}", "-o", str(overflow_image),
                str(overflow_builtins_source),
            ], env=env)
            sys.stdout.buffer.write(run_qemu(
                qemu, machine, overflow_image,
                trace_for(overflow_image),
            ))

            typed_overflow_image = \
                tmpdir / f"typed-overflow-builtins-{overflow_name}.hex"
            run([
                str(sdcc), "-mmcs251", "--std=gnu17", *model_flags,
                "--no-xinit-opt", *board_link_flags,
                f"-L{overflow_library_dir}",
                "-o", str(typed_overflow_image),
                str(typed_overflow_builtins_source),
            ], env=env)
            sys.stdout.buffer.write(run_qemu(
                qemu, machine, typed_overflow_image,
                trace_for(typed_overflow_image),
            ))

        optimization_modes = (
            ("default", ()),
            ("size", ("--opt-code-size",)),
            ("speed", ("--opt-code-speed",)),
        )
        for optimization_name, model_flags, optimization_library_dir in \
                runtime_configurations:
            for optimization_mode, optimization_flags in optimization_modes:
                optimization_image = tmpdir / (
                    f"optimization-{optimization_name}-{optimization_mode}.hex"
                )
                run([
                    str(sdcc), "-mmcs251", *model_flags,
                    *optimization_flags, "--no-xinit-opt",
                    *board_link_flags, f"-I{device_include}",
                    f"-I{device_include / 'mcs51'}",
                    f"-L{optimization_library_dir}",
                    "-o", str(optimization_image),
                    str(optimization_runtime_source),
                ], env=env)
                sys.stdout.buffer.write(run_qemu(
                    qemu, machine, optimization_image,
                    trace_for(optimization_image),
                ))

        for abi_name, model_flags, abi_library_dir in runtime_configurations:
            abi_image = tmpdir / f"abi-regression-{abi_name}.hex"
            run([
                str(sdcc), "-mmcs251", *model_flags, "--no-xinit-opt",
                *board_link_flags, f"-I{device_include}",
                f"-I{device_include / 'mcs51'}", f"-L{abi_library_dir}",
                "-o", str(abi_image), str(abi_regression_source),
            ], env=env)
            sys.stdout.buffer.write(run_qemu(
                qemu, machine, abi_image, trace_for(abi_image),
            ))

        longlong_configurations = (
            runtime_configurations[1],
            runtime_configurations[3],
        )
        for longlong_name, model_flags, longlong_library_dir in \
                longlong_configurations:
            longlong_image = tmpdir / f"longlong-{longlong_name}.hex"
            run([
                str(sdcc), "-mmcs251", *model_flags, "--no-xinit-opt",
                *board_link_flags, f"-I{device_include}",
                f"-I{device_include / 'mcs51'}", f"-L{longlong_library_dir}",
                "-o", str(longlong_image), str(longlong_source),
            ], env=env)
            longlong_map = longlong_image.with_suffix(".map").read_text()
            for symbol in (
                "__mullonglong",
                "__divulonglong",
                "__modulonglong",
                "__divslonglong",
                "__modslonglong",
            ):
                if symbol not in longlong_map:
                    raise SystemExit(
                        f"{longlong_name} 64-bit image did not link {symbol}"
                    )
            sys.stdout.buffer.write(run_qemu(
                qemu, machine, longlong_image, trace_for(longlong_image),
            ))

        for endian_name, model_flags, endian_library_dir in \
                runtime_configurations:
            endianness_image = \
                tmpdir / f"endianness-runtime-{endian_name}.hex"
            run([
                str(sdcc), "-mmcs251", *model_flags, "--no-xinit-opt",
                *board_link_flags, f"-I{device_include}",
                f"-I{device_include / 'mcs51'}",
                f"-L{endian_library_dir}", "-o", str(endianness_image),
                str(endianness_runtime_source),
            ], env=env)
            sys.stdout.buffer.write(run_qemu(
                qemu, machine, endianness_image,
                trace_for(endianness_image),
            ))

        aggregate_image = tmpdir / "aggregate-return.hex"
        run([
            str(sdcc), "-mmcs251", "--no-xinit-opt", *board_link_flags,
            f"-I{device_include}", f"-I{device_include / 'mcs51'}",
            f"-L{library_dir}", "-o", str(aggregate_image),
            str(aggregate_source),
        ], env=env)
        sys.stdout.buffer.write(run_qemu(
            qemu, machine, aggregate_image, trace_for(aggregate_image),
        ))

        memory_model_image = tmpdir / "memory-model-large.hex"
        memory_model_main_rel = tmpdir / "memory-model-runtime.rel"
        memory_model_rel = tmpdir / "memory-model.rel"
        run([
            str(sdcc), "-mmcs251", "--model-large", "--no-xinit-opt",
            f"-I{device_include}", f"-I{device_include / 'mcs51'}", "-c",
            "-o", str(memory_model_main_rel), str(memory_model_main_source),
        ], env=env)
        run([
            str(sdcc), "-mmcs251", "--model-large", "--no-xinit-opt", "-c",
            "-o", str(memory_model_rel), str(memory_model_source),
        ], env=env)
        run([
            str(sdcc), "-mmcs251", "--model-large", "--no-xinit-opt",
            *board_link_flags, f"-L{large_library_dir}",
            "-o", str(memory_model_image), str(memory_model_main_rel),
            str(memory_model_rel),
        ], env=env)
        sys.stdout.buffer.write(run_qemu(
            qemu, machine, memory_model_image, trace_for(memory_model_image),
        ))


if __name__ == "__main__":
    main()
