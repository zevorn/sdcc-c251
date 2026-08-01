from pathlib import Path
import subprocess


DEFAULT_INSTRUCTION_LIMIT = 1000000
PLUGIN_SUFFIXES = (".so", ".dylib", ".dll")


def _plugin_path(qemu, name):
    plugin_dir = Path(qemu).resolve().parent / "contrib" / "plugins"
    for suffix in PLUGIN_SUFFIXES:
        candidate = plugin_dir / f"lib{name}{suffix}"
        if candidate.is_file():
            return candidate
    return None


def capture_instruction_trace(
    qemu,
    command,
    trace_log,
    instruction_limit=DEFAULT_INSTRUCTION_LIMIT,
):
    trace_log = Path(trace_log).resolve()
    trace_log.parent.mkdir(parents=True, exist_ok=True)
    trace_log.unlink(missing_ok=True)
    execlog = _plugin_path(qemu, "execlog")
    stoptrigger = _plugin_path(qemu, "stoptrigger")

    trace_command = list(command)
    plugin_trace = execlog is not None and stoptrigger is not None
    if plugin_trace:
        trace_command.extend([
            "-plugin",
            f"file={execlog}",
            "-plugin",
            f"file={stoptrigger},icount={instruction_limit}:124",
            "-d",
            "plugin",
            "-D",
            str(trace_log),
        ])
    else:
        trace_command.extend([
            "-d",
            "in_asm,exec,nochain",
            "-D",
            str(trace_log),
        ])

    process = subprocess.Popen(
        trace_command,
        stdin=subprocess.DEVNULL,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
    )
    timeout = 10 if plugin_trace else 1
    try:
        process.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        process.terminate()
        try:
            process.communicate(timeout=1)
        except subprocess.TimeoutExpired:
            process.kill()
            process.communicate()

    return plugin_trace
