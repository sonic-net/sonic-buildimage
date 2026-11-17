#!/usr/bin/env python3
"""Exercise unchanged DASH engine startup and installed software protocols."""

import argparse
from contextlib import closing
import hashlib
import json
import os
from pathlib import Path
import platform
import queue
import shutil
import signal
import struct
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, str(Path(__file__).parent))
import runtime_smoke


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def command(argv, timeout=30):
    result = runtime_smoke.command(argv, timeout)
    require(result["returncode"] == 0, "installed command failed: " + json.dumps(result))
    return result["stdout"]


def wait_until(callback, message, timeout=30):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        value = callback()
        if value:
            return value
        time.sleep(0.1)
    raise AssertionError(message)


def stop(process):
    if process.poll() is None:
        os.killpg(process.pid, signal.SIGTERM)
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            process.wait(timeout=5)


def descendants(pid):
    yield pid
    children = Path(f"/proc/{pid}/task/{pid}/children")
    if children.exists():
        for child in children.read_text().split():
            yield from descendants(int(child))


def installed_symbol(record, symbol):
    """Resolve a named real function, with the same missing-symbol control."""
    binary = Path(runtime_smoke.absolute_path(record["path"]))
    symbols = Path(runtime_smoke.absolute_path(record["debug_path"]))
    require(binary.is_file() and symbols.is_file(), "missing installed debug pair")
    with tempfile.TemporaryDirectory(prefix="engine-gdb-") as temporary:
        root = Path(temporary)
        isolated = root / binary.name
        shutil.copyfile(binary, isolated)
        empty = root / "empty"
        empty.mkdir()
        missing = runtime_smoke.gdb_lookup(isolated, root, str(empty), symbol)
        require(not missing.get("found"), "function resolved without packaged symbols: " + symbol)
        found = runtime_smoke.gdb_lookup(binary, root, "/usr/lib/debug", symbol)
        names = {symbol}
        if record["path"] == "usr/bin/simple_switch_grpc" and symbol == "main":
            # GDB reports this source-owned C++ main with its parameter spelling.
            names.add("main(int, char**)")
        require(found.get("found") and found.get("function") in names and found.get("line", 0) > 0
                and found.get("source") and found.get("type"), "installed function lacks source/line/type: " + symbol)
        found["source"] = Path(found["source"]).name
    return {"path": record["path"], "debug_path": record["debug_path"], "symbol": symbol,
            "without_symbols": missing, "with_symbols": found}


def installed_debug_checks(report):
    selected = {"usr/bin/simple_switch_grpc": "main", "usr/lib/python3/dist-packages/_nnpy.so": "_cffi_f_nn_socket"}
    pairs = {item["path"]: item for item in report["source_debug_pairs"]}
    required = set(selected) | {"usr/bin/pi_CLI_dummy"}
    require(len(pairs) == len(report["source_debug_pairs"]) and required <= pairs.keys(),
            "missing or duplicate installed debugger contracts")
    launcher = pairs["usr/bin/pi_CLI_dummy"]
    forwarding = launcher.get("debug_forwarding", {})
    require(all(forwarding.get(key) == value for key, value in {
        "provider": "usr/lib/x86_64-linux-gnu/libpicli.so.0", "symbol": "main", "source": "main.c"}.items()),
        "PI launcher lacks its declared implementation contract")
    checks = [installed_symbol(pairs[name], symbol) for name, symbol in selected.items()]
    # The PI owner exports a CRT launcher whose main is defined by libpicli.
    # Reuse the shared installed provider/symbol positive and negative controls.
    forwarded = runtime_smoke.debug_pair(launcher)
    forwarded.update(path=launcher["path"], debug_path=launcher["debug_path"], symbol="main")
    forwarded["with_symbols"]["source"] = Path(forwarded["with_symbols"]["source"]).name
    checks.append(forwarded)
    return checks


def installed_closure(report, debug):
    native = report["native"] if debug else report["runtime_native"]
    elfs = {item["path"]: item for item in report["elfs"]}
    require(len(elfs) == len(report["elfs"]) and set(elfs) == set(report["native"]),
            "incomplete or duplicate ELF inventory")
    require(native, "no installed source ELF inventory")
    for name, expected in native.items():
        path = Path(runtime_smoke.absolute_path(name))
        require(path.is_file() and hashlib.sha256(path.read_bytes()).hexdigest() == expected["sha256"],
                "installed source ELF differs: " + name)
        with path.open("rb") as stream:
            header = stream.read(20)
        require(header[:6] == b"\x7fELF\x02\x01" and int.from_bytes(header[18:20], "little") == 62,
                "installed ELF is not AMD64: " + name)
        result = runtime_smoke.command(["/lib64/ld-linux-x86-64.so.2", "--list", str(path)])
        require(runtime_smoke.loader_result(result, elfs[name]["needed"]),
                "installed ELF closure failed: " + json.dumps(result))
    checks = []
    if debug:
        checks = installed_debug_checks(report)
        require("Huge version" in command(["/usr/bin/vim", "--version"]), "debug Vim is not full Vim")
    return {"source_elf_count": len(native), "loader_checks": len(native), "debug_checks": checks}


def p4runtime_check():
    import grpc
    from p4.v1 import p4runtime_pb2 as messages, p4runtime_pb2_grpc as services

    requests = queue.Queue()
    with grpc.insecure_channel("127.0.0.1:9559") as channel:
        grpc.channel_ready_future(channel).result(timeout=30)
        client = services.P4RuntimeStub(channel)
        version = client.Capabilities(messages.CapabilitiesRequest(), timeout=10).p4runtime_api_version
        require(version == "1.3.0", "selected PI P4Runtime API version differs")
        request = messages.StreamMessageRequest()
        request.arbitration.device_id = 0
        request.arbitration.election_id.low = 1
        requests.put(request)
        stream = client.StreamChannel(iter(requests.get, None), timeout=30)
        try:
            response = next(stream)
            require(response.HasField("arbitration"), "server did not arbitrate the installed controller")
            arbitration = response.arbitration
            require(arbitration.device_id == 0 and arbitration.election_id.high == 0
                    and arbitration.election_id.low == 1 and arbitration.status.code == 0,
                    "P4Runtime primary arbitration failed")
            # This is the selected PI owner's test_no_pipeline_config contract.
            # Do not load a synthetic pipeline into the original --no-p4 process.
            try:
                client.GetForwardingPipelineConfig(
                    messages.GetForwardingPipelineConfigRequest(device_id=0), timeout=10)
            except grpc.RpcError as error:
                require(error.code() == grpc.StatusCode.FAILED_PRECONDITION,
                        "unexpected no-pipeline response: " + str(error))
            else:
                raise AssertionError("no-p4 startup unexpectedly had a forwarding pipeline")
        finally:
            requests.put(None)
            stream.cancel()
    return {"api_version": version, "device_id": 0, "primary_arbitration": "passed",
            "no_pipeline_get_config": "FAILED_PRECONDITION"}


def nnpy_clients():
    """Reuse the producer's real, private nanomsg client protocol fixtures."""
    import nnpy
    import _nnpy

    site = Path("/usr/lib/python3/dist-packages")
    require(Path(nnpy.__file__).resolve().is_relative_to(site) and
            Path(_nnpy.__file__).resolve().is_relative_to(site), "nnpy is not the installed owner payload")
    for name in ("bm_p4dbg", "bm_nanomsg_events"):
        require("--socket" in command([sys.executable, "-I", "/usr/bin/" + name, "--help"]),
                "installed BMv2 client interface differs")
    results = {}
    with tempfile.TemporaryDirectory(prefix="engine-nnpy-") as temporary:
        root = Path(temporary)
        config = root / "empty.json"
        config.write_text("{}\n")
        endpoint = "ipc://" + str(root / "events")
        with closing(nnpy.Socket(nnpy.AF_SP, nnpy.PUB)) as publisher:
            publisher.bind(endpoint)
            log_path = root / "events.log"
            with log_path.open("w") as log:
                process = subprocess.Popen([sys.executable, "-I", "-u", "/usr/bin/bm_nanomsg_events",
                    "--socket", endpoint, "--json", str(config)], stdout=log, stderr=subprocess.STDOUT,
                    start_new_session=True)
                try:
                    def decoded():
                        require(process.poll() is None, "installed events client exited: " + log_path.read_text())
                        publisher.send(struct.pack("<iQIQQQi", 0, 7, 2, 11, 13, 17, 23))
                        return "port_in: 23" in log_path.read_text()
                    wait_until(decoded, "installed events client did not decode the private message", 10)
                    publisher.send(struct.pack("<iQIQQQ", 999, 7, 2, 11, 13, 17))
                    require(process.wait(timeout=10) == 0, "installed events client failed")
                finally:
                    stop(process)
            output = log_path.read_text()
            require("type: PACKET_IN, switch_id: 7, cxt_id: 2, sig: 11, id: 13, copy_id: 17, port_in: 23" in output
                    and "The JSON config has changed" in output, "installed events decoded different fields")
            results["bm_nanomsg_events"] = {"packet_in_decoded": True, "config_change_observed": True}
        endpoint = "ipc://" + str(root / "debugger")
        with closing(nnpy.Socket(nnpy.AF_SP, nnpy.REP)) as server:
            server.bind(endpoint)
            server.setsockopt(nnpy.SOL_SOCKET, nnpy.RCVTIMEO, 10000)
            server.setsockopt(nnpy.SOL_SOCKET, nnpy.SNDTIMEO, 10000)
            log_path = root / "debugger.log"
            with log_path.open("w") as log:
                process = subprocess.Popen([sys.executable, "-I", "-u", "/usr/bin/bm_p4dbg", "--socket",
                    endpoint, "--json", str(config)], stdin=subprocess.DEVNULL, stdout=log,
                    stderr=subprocess.STDOUT, start_new_session=True)
                try:
                    for kind, request_id in ((18, 0), (19, 1)):
                        data = server.recv()
                        require(len(data) == 20 and struct.unpack("<iQQ", data) == (kind, 0, request_id),
                                "installed debugger request differs")
                        server.send(struct.pack("<iQQiQ", 15, 0, request_id, 0, 0))
                    require(process.wait(timeout=10) == 0, "installed debugger client failed")
                finally:
                    stop(process)
            output = log_path.read_text()
            require("Connection established" in output and "Detaching from switch" in output,
                    "installed debugger did not attach/detach")
            results["bm_p4dbg"] = {"attach_detach": True, "request_types": [18, 19]}
    return results


def startup():
    # These interfaces exist only inside Docker's private network namespace.
    require(not Path("/sys/class/net/eth1").exists() and not Path("/sys/class/net/eth2").exists(),
            "network fixture was not isolated")
    command(["ip", "link", "set", "lo", "up"])
    command(["ip", "link", "add", "eth1", "type", "veth", "peer", "name", "eth2"])
    for name in ("eth1", "eth2"):
        require(Path("/sys/class/net", name, "operstate").read_text().strip() == "down",
                "fixture interface was already up")
    executable = Path("/usr/bin/simple_switch_grpc")
    wanted = ["simple_switch_grpc", "--interface", "0@eth1", "--interface", "1@eth2", "--log-console", "--no-p4"]
    log_path = Path("/tmp/dash-engine-startup.log")
    with log_path.open("w") as log:
        process = subprocess.Popen(["/usr/bin/start.sh"], stdin=subprocess.DEVNULL,
            stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
        try:
            def installed_pid():
                require(process.poll() is None, "original start.sh exited")
                selected = []
                for pid in descendants(process.pid):
                    try:
                        if os.path.samefile(Path(f"/proc/{pid}/exe"), executable):
                            selected.append(pid)
                    except FileNotFoundError:
                        pass
                require(len(selected) <= 1, "original startup launched multiple switches")
                return selected[0] if selected else 0
            pid = wait_until(installed_pid, "original start.sh did not run the installed switch")
            argv = Path(f"/proc/{pid}/cmdline").read_bytes().rstrip(b"\0").decode().split("\0")
            require(argv == wanted, "original switch arguments differ: " + repr(argv))
            def links_up():
                return all(Path("/sys/class/net", name, "operstate").read_text().strip() == "up"
                           for name in ("eth1", "eth2"))
            wait_until(links_up, "original start.sh did not raise both interfaces")
            rpc = p4runtime_check()
            clients = nnpy_clients()
            for _ in range(10):
                time.sleep(0.2)
                require(installed_pid() == pid and links_up(), "installed switch or interfaces became unstable")
            return {"entrypoint": "/usr/bin/start.sh", "switch_arguments": argv, "stable_installed_process": True,
                    "interfaces_up": ["eth1", "eth2"], "p4runtime": rpc, "nanomsg_clients": clients}
        except Exception as error:
            raise AssertionError(str(error) + "\n" + log_path.read_text(errors="replace")[-12000:]) from error
        finally:
            stop(process)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--validation", type=Path, required=True)
    parser.add_argument("--debug", action="store_true")
    args = parser.parse_args()
    for key in ("LD_PRELOAD", "LD_LIBRARY_PATH", "LD_AUDIT", "PYTHONPATH"):
        os.environ.pop(key, None)
    os.environ.update(LC_ALL="C", DEBUGINFOD_URLS="", PYTHONDONTWRITEBYTECODE="1")
    require(platform.machine() == "x86_64", "installed probe requires native AMD64")
    report = json.loads(args.validation.read_text())
    require(report["status"] == "passed" and report["architecture"] == "amd64", "unverified static report")
    closure = installed_closure(report, args.debug)
    result = {"status": "passed", "architecture": "amd64", "variant": "debug" if args.debug else "runtime",
              **closure, "startup": startup(),
              "limits": "Private software startup, no-pipeline P4Runtime and installed nanomsg clients; no forwarding or full VS claim"}
    print(json.dumps(result))


if __name__ == "__main__":
    main()
