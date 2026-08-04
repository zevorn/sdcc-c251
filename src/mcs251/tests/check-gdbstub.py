#!/usr/bin/env python3

import argparse
from contextlib import nullcontext
import os
from pathlib import Path
import re
import socket
import subprocess
import tempfile
import time
import xml.etree.ElementTree as ET


MACHINE_CANDIDATES = ("stc32g144k246", "stc32g144k246-evb")


def run(command, *, env):
    subprocess.run(command, check=True, env=env)


def rsp_escape(payload):
    escaped = bytearray()
    for value in payload:
        if value in b"#$}*":
            escaped.extend((ord("}"), value ^ 0x20))
        else:
            escaped.append(value)
    return bytes(escaped)


def rsp_unescape(payload):
    decoded = bytearray()
    index = 0
    while index < len(payload):
        value = payload[index]
        if value == ord("}"):
            index += 1
            if index >= len(payload):
                raise RuntimeError("truncated RSP escape")
            decoded.append(payload[index] ^ 0x20)
        elif value == ord("*"):
            index += 1
            if not decoded or index >= len(payload):
                raise RuntimeError("invalid RSP run-length encoding")
            decoded.extend([decoded[-1]] * (payload[index] - 29))
        else:
            decoded.append(value)
        index += 1
    return bytes(decoded)


class RspConnection:
    def __init__(self, connection):
        self.connection = connection

    def send(self, text):
        payload = rsp_escape(text.encode("ascii"))
        checksum = sum(payload) & 0xff
        packet = b"$" + payload + f"#{checksum:02x}".encode("ascii")
        self.connection.sendall(packet)

    def receive(self):
        while True:
            marker = self.connection.recv(1)
            if not marker:
                raise RuntimeError("QEMU closed the GDB connection")
            if marker in (b"+", b"-"):
                continue
            if marker != b"$":
                continue

            encoded = bytearray()
            while True:
                value = self.connection.recv(1)
                if not value:
                    raise RuntimeError("QEMU closed an incomplete RSP packet")
                if value == b"#":
                    break
                encoded.extend(value)
            received_checksum = self.connection.recv(2)
            expected_checksum = f"{sum(encoded) & 0xff:02x}".encode("ascii")
            if received_checksum.lower() != expected_checksum:
                self.connection.sendall(b"-")
                continue
            self.connection.sendall(b"+")
            return rsp_unescape(bytes(encoded)).decode("ascii")

    def request(self, text):
        self.send(text)
        return self.receive()

    def read_feature(self, name):
        chunks = []
        offset = 0
        while True:
            response = self.request(
                f"qXfer:features:read:{name}:{offset:x},400"
            )
            if not response or response[0] not in "ml":
                raise RuntimeError(f"cannot read GDB feature {name}: {response!r}")
            chunk = response[1:]
            chunks.append(chunk)
            offset += len(chunk.encode("ascii"))
            if response[0] == "l":
                return "".join(chunks)


def reserve_tcp_port():
    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listener.bind(("127.0.0.1", 0))
    port = listener.getsockname()[1]
    listener.close()
    return port


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


def connect_to_qemu(port, deadline):
    while time.monotonic() < deadline:
        connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        connection.settimeout(max(0.1, deadline - time.monotonic()))
        try:
            connection.connect(("127.0.0.1", port))
            connection.settimeout(5)
            return connection
        except OSError:
            connection.close()
            time.sleep(0.02)
    raise RuntimeError("QEMU did not open its GDB endpoint")


def register_numbers(xml_text):
    root = ET.fromstring(xml_text)
    numbers = {}
    next_number = 0
    for element in root.iter():
        if element.tag.rsplit("}", 1)[-1] != "reg":
            continue
        if "regnum" in element.attrib:
            next_number = int(element.attrib["regnum"], 0)
        numbers[element.attrib["name"]] = next_number
        next_number += 1
    return numbers


def read_register(rsp, number):
    reply = rsp.request(f"p{number:x}")
    try:
        return bytes.fromhex(reply)
    except ValueError as error:
        raise RuntimeError(
            f"invalid register {number} reply: {reply!r}"
        ) from error


def parse_main_address(cdb, map_file):
    cdb_match = re.search(
        r"^L:G\$main\$0\$0:([0-9a-fA-F]{6,8})$",
        cdb.read_text(),
        re.MULTILINE,
    )
    if not cdb_match:
        raise RuntimeError("SDCC CDB output has no 24-bit main label")
    cdb_address = int(cdb_match.group(1), 16)

    map_match = re.search(
        r"^C:\s+([0-9a-fA-F]{8})\s+_main(?:\s|$)",
        map_file.read_text(),
        re.MULTILINE,
    )
    if not map_match:
        raise RuntimeError("linker map has no _main symbol")
    map_address = int(map_match.group(1), 16)
    if cdb_address != map_address:
        raise RuntimeError(
            f"CDB main address 0x{cdb_address:06x} differs from "
            f"map address 0x{map_address:06x}"
        )
    return cdb_address


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sdcc", required=True)
    parser.add_argument("--qemu", required=True)
    parser.add_argument("--machine")
    parser.add_argument("--source", required=True)
    parser.add_argument("--device-include", required=True)
    parser.add_argument("--library-dir", required=True)
    parser.add_argument("--work-dir")
    parser.add_argument("--trace-log")
    args = parser.parse_args()

    sdcc = Path(args.sdcc).resolve()
    qemu = Path(args.qemu).resolve()
    source = Path(args.source).resolve()
    device_include = Path(args.device_include).resolve()
    library_dir = Path(args.library_dir).resolve()
    for path in (sdcc, qemu, source, device_include, library_dir):
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
        workspace = tempfile.TemporaryDirectory(prefix="sdcc-mcs251-gdbstub.")

    with workspace as temporary:
        output_dir = Path(temporary)
        image = output_dir / "debug-smoke.hex"
        run([
            str(sdcc), "-mmcs251", "--debug", "--no-xinit-opt",
            "--code-loc", "0xff0000", "-Wl-b GSINIT0=0xfc2800",
            f"-I{device_include}", f"-I{device_include / 'mcs51'}",
            f"-L{library_dir}", "-o", str(image), str(source),
        ], env=env)

        cdb = image.with_suffix(".cdb")
        map_file = image.with_suffix(".map")
        main_address = parse_main_address(cdb, map_file)

        port = reserve_tcp_port()
        command = [
            str(qemu), "-M", machine, "-bios", str(image),
            "-display", "none", "-monitor", "none", "-serial", "null",
            "-S", "-gdb", f"tcp:127.0.0.1:{port}",
        ]
        if args.trace_log:
            trace_log = Path(args.trace_log).resolve()
            command.extend([
                "-d", "in_asm,cpu,nochain", "-D", str(trace_log),
            ])

        process = subprocess.Popen(
            command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        )
        connection = None
        try:
            connection = connect_to_qemu(port, time.monotonic() + 5)
            rsp = RspConnection(connection)
            supported = rsp.request("qSupported:multiprocess+")
            if "qXfer:features:read+" not in supported:
                raise RuntimeError(
                    f"QEMU did not advertise target XML: {supported!r}"
                )

            target_xml = rsp.read_feature("target.xml")
            include_match = re.search(
                r'href=["\']([^"\']*mcs251-core\.xml)["\']', target_xml
            )
            if not include_match:
                raise RuntimeError("QEMU target.xml does not include mcs251-core.xml")
            core_xml = rsp.read_feature(include_match.group(1))
            registers = register_numbers(core_xml)
            # QEMU exposes the byte positions that make up DPX (R56-R59)
            # and SPX (R60-R63), matching the architectural register file.
            for required in ("pc", "r56", "r59", "r60", "r63"):
                if required not in registers:
                    raise RuntimeError(f"QEMU XML has no {required} register")

            if rsp.request(f"Z0,{main_address:x},1") != "OK":
                raise RuntimeError("QEMU rejected the _main breakpoint")
            stop = rsp.request("c")
            if not stop.startswith(("S05", "T05")):
                raise RuntimeError(f"unexpected GDB stop reply: {stop!r}")

            pc_bytes = read_register(rsp, registers["pc"])
            decoded_pc = {
                int.from_bytes(pc_bytes, "little") & 0xffffff,
                int.from_bytes(pc_bytes, "big") & 0xffffff,
            }
            if main_address not in decoded_pc:
                raise RuntimeError(
                    f"QEMU stopped at PC bytes {pc_bytes.hex()}, expected "
                    f"0x{main_address:06x}"
                )

            r0_number = registers["r0"]
            original_r0 = read_register(rsp, r0_number)
            if rsp.request(f"P{r0_number:x}=5a") != "OK":
                raise RuntimeError("QEMU rejected an R0 register write")
            if read_register(rsp, r0_number) != b"\x5a":
                raise RuntimeError("QEMU did not preserve the R0 write")
            if rsp.request(
                    f"P{r0_number:x}={original_r0.hex()}") != "OK":
                raise RuntimeError("QEMU rejected the R0 restore")

            if rsp.request(f"z0,{main_address:x},1") != "OK":
                raise RuntimeError("QEMU rejected breakpoint removal")
            step_stop = rsp.request("s")
            if not step_stop.startswith(("S05", "T05")):
                raise RuntimeError(
                    f"unexpected GDB single-step reply: {step_stop!r}"
                )
            stepped_pc = read_register(rsp, registers["pc"])
            if stepped_pc == pc_bytes:
                raise RuntimeError("QEMU single-step did not advance PC")
            print(
                f"MCS251 GDB stub PASS: _main=0x{main_address:06x}, "
                f"registers={len(registers)}, breakpoint/register/step=OK"
            )
        finally:
            if connection is not None:
                connection.close()
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=1)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
            output = process.stdout.read() if process.stdout else b""
            if process.returncode not in (0, -15) and output:
                print(output.decode(errors="replace"), end="")


if __name__ == "__main__":
    main()
