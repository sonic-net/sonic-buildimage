#!/usr/bin/env python3
"""Exercise the installed P4RT startup and software RPCs with private Redis."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import tempfile
import time
from xmlrpc.client import ServerProxy

import p4rt_runtime_package as owner
import redis
import runtime_smoke
from supervisor.xmlrpc import SupervisorTransport


def require(value, message):
    if not value:
        raise AssertionError(message)


def digest(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def source_debug_pairs(report, native, debug):
    common = {path for path in native if path.startswith("usr/lib/x86_64-linux-gnu/libswsscommon.so.")}
    require(len(common) == 1, "expected the inherited Common library")
    selected = common
    if debug:
        sai = {"usr/lib/x86_64-linux-gnu/" + name for name in
               ("libsairedis.so.0.0.0", "libsaimeta.so.0.0.0", "libsaimetadata.so.0.0.0")}
        require(sai <= set(native), "missing debug SAI runtime libraries")
        selected = common | sai
    pairs = [item for item in report["source_debug_pairs"] if item["path"] in selected]
    require(len(pairs) == len(selected) and {item["path"] for item in pairs} == selected,
            "missing or duplicate selected source debug pairs")
    return pairs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--validation", type=Path, required=True)
    parser.add_argument("--debug", action="store_true")
    args = parser.parse_args()
    for name in ("LD_PRELOAD", "LD_LIBRARY_PATH", "LD_AUDIT", "PYTHONPATH"):
        os.environ.pop(name, None)
    report = json.loads(args.validation.read_text())
    require(report["status"] == "passed" and report["architecture"] == "amd64", "unvalidated image")
    native = report["native"] if args.debug else report["runtime_native"]
    for name, expected in native.items():
        path = Path("/") / name
        info = path.stat()
        require(digest(path) == expected["sha256"], "installed source differs: " + name)
        require((info.st_uid, info.st_gid, info.st_mode & 0o7777) ==
                (expected["uid"], expected["gid"], expected["mode"]),
                "installed source metadata differs: " + name)
    selected_pairs = source_debug_pairs(report, native, args.debug)
    probe = runtime_smoke.probe({
        "architecture": "amd64",
        "elfs": [item for item in report["elfs"] if item["path"] in native],
        "debug_pairs": selected_pairs,
        "policy": {
            "python_imports": ["swsscommon.swsscommon"],
            "commands": [{"argv": ["/usr/bin/vim", "--version"], "output_pattern": "Huge version"}]
            if args.debug else [],
        },
    }, "debug" if args.debug else "runtime")
    require(probe["passed"], json.dumps(probe))

    split_debug = {}
    for name, (_, flag) in owner.PROGRAMS.items():
        binary = Path("/usr/local/bin") / name
        result = subprocess.run([str(binary), "--helpshort"], text=True, capture_output=True, timeout=30)
        require(result.returncode in (0, 1) and flag in result.stdout + result.stderr,
                "installed CLI differs: " + name)
        if not args.debug:
            require(not binary.with_name(name + ".dwp").exists(), "runtime contains debug DWP: " + name)
            continue
        expected = report["p4rt_debug_pairs"][name]
        symbols = binary.with_name(name + ".dwp")
        require(digest(binary) == expected["runtime_sha256"] and digest(symbols) == expected["dwp_sha256"],
                "installed P4RT split-symbol pair changed: " + name)
        with tempfile.TemporaryDirectory(prefix="p4rt-symbols-") as temporary:
            work = Path(temporary)
            (work / "empty").mkdir()
            copied = work / name
            shutil.copyfile(binary, copied)
            missing = owner.debug_lookup(copied, work)
            require(not missing["found"], "GDB found missing DWP information: " + name)
            (work / (name + ".dwp")).symlink_to(symbols)
            found = owner.debug_lookup(copied, work)
            require(found.get("found") and all(found.get(key) == expected[key]
                    for key in ("file", "line", "parameters")),
                    "GDB did not resolve the installed owner's DWP: " + name)
            split_debug[name] = {"without_dwp": missing, "with_dwp": found}

    root = Path("/run/p4rt-fixture")
    root.mkdir()
    settings = json.loads(Path("/probe/services/database_config.json").read_text())
    settings["INSTANCES"]["redis"].update(hostname="127.0.0.1", port=6379,
                                         unix_socket_path="/run/redis/redis.sock")
    require(settings["DATABASES"]["CONFIG_DB"]["id"] == 4 and
            settings["DATABASES"]["APPL_DB"]["id"] == 0, "inherited Redis database identities changed")
    db = Path("/run/redis/sonic-db")
    db.mkdir(parents=True)
    (db / "database_config.json").write_text(json.dumps(settings))
    for name in ("/var/log/supervisor", "/var/spool/rsyslog"):
        Path(name).mkdir(parents=True, exist_ok=True)
    children, logs = {}, []
    connection = redis.Redis(unix_socket_path="/run/redis/redis.sock", db=4, decode_responses=True,
                             socket_timeout=2, socket_connect_timeout=2)

    def start(name, command):
        log = (root / (name + ".log")).open("wb")
        logs.append(log)
        children[name] = subprocess.Popen(command, stdin=subprocess.DEVNULL, stdout=log,
                                          stderr=subprocess.STDOUT, start_new_session=True)

    def wait_for(check, seconds=60):
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            require(all(child.poll() is None for child in children.values()), "a private service exited")
            try:
                result = check()
                if result:
                    return result
            except (OSError, redis.exceptions.RedisError):
                pass
            time.sleep(0.2)
        raise AssertionError("timed out waiting for the installed service")

    def states():
        if not Path("/run/supervisor.sock").exists():
            return {}
        transport = SupervisorTransport(None, None, "unix:///var/run/supervisor.sock")
        with ServerProxy("http://localhost", transport=transport) as rpc:
            return {item["name"]: item for item in rpc.supervisor.getAllProcessInfo()}

    try:
        start("redis", ["/probe/services/usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2",
                        "--library-path", "/probe/services/usr/lib/x86_64-linux-gnu",
                        "/probe/services/usr/bin/redis-server", "--bind", "127.0.0.1", "--port", "6379",
                        "--unixsocket", "/run/redis/redis.sock", "--unixsocketperm", "700",
                        "--notify-keyspace-events", "KEA", "--save", "", "--appendonly", "no"])
        wait_for(connection.ping)
        connection.set("CONFIG_DB_INITIALIZED", "1")
        connection.hset("FEATURE|p4rt", mapping={"state": "enabled", "auto_restart": "enabled"})
        connection.hset("NODE_CFG|integrated_circuit0", mapping={"node-id": "1"})
        connection.hset("P4RT|p4rt_app", mapping={"port": "9559", "use_genetlink": "false",
                                                 "use_port_ids": "false", "p4rt_unix_socket": "/run/p4rt.sock"})
        start("supervisor", ["/usr/local/bin/supervisord"])

        def ready():
            current = states()
            dependency = current.get("dependent-startup", {})
            startup = current.get("start", {})
            started = startup.get("statename") == "EXITED" and startup.get("exitstatus") == 0
            ordered = dependency.get("statename") == "RUNNING" or (
                dependency.get("statename") == "EXITED" and dependency.get("exitstatus") == 0)
            running = all(current.get(name, {}).get("statename") == "RUNNING" for name in
                          ("p4rt", "rsyslogd", "supervisor-proc-exit-listener"))
            return current if started and ordered and running else None

        before = wait_for(ready)
        require(Path("/var/sonic/config_status").read_text() == "# Config files managed by sonic-config-engine\n",
                "original start.sh did not create its configuration marker")
        pid = before["p4rt"]["pid"]
        argv = Path("/proc") / str(pid) / "cmdline"
        def p4rt_executed():
            command = argv.read_bytes().split(b"\0")
            return command if command[:1] == [b"/usr/local/bin/p4rt"] else None

        command = wait_for(p4rt_executed)
        for value in (b"/usr/local/bin/p4rt", b"--p4rt_grpc_port=9559", b"--use_genetlink=false",
                      b"--p4rt_unix_socket=/run/p4rt.sock", b"--use_insecure_server_credentials"):
            require(value in command, "p4rt.sh did not render the ConfigDB setting: " + value.decode())
        wait_for(lambda: Path("/run/p4rt.sock").exists())
        client = ["/usr/local/bin/p4rt_set_forwarding_pipeline", "--p4rt_server_address=127.0.0.1:9559",
                  "--p4rt_device_id=1", "--use_insecure_credentials", "--default_fbr_p4info"]
        # VERIFY exercises controller arbitration and real server-side P4Info
        # validation without programming an absent ASIC/Orchagent peer.
        verified = subprocess.run([*client, "--forwarding_action=VERIFY"],
                                  text=True, capture_output=True, timeout=60)
        require(verified.returncode == 0 and "Completed successfully." in verified.stdout,
                "installed P4Runtime VERIFY failed: " + verified.stdout + verified.stderr)
        rejected = subprocess.run([*client, "--forwarding_action=VERIFY_AND_SAVE"],
                                  text=True, capture_output=True, timeout=60)
        require(rejected.returncode == 12 and "UNIMPLEMENTED" in rejected.stdout + rejected.stderr,
                "installed P4Runtime did not reject the unsupported operation")
        after = ready()
        require(after and after["p4rt"]["pid"] == pid, "P4Runtime requests disrupted the installed daemon")
        print(json.dumps({"status": "passed", "architecture": "amd64", "source_elf_count": len(native),
                          "loader_checks": len(probe["loader_checks"]),
                          "debug_checks": len(probe["debug_pairs"]),
                          "debug_tool_checks": len(probe["commands"]),
                          "split_debug_checks": split_debug, "supervisor_states": {
                              name: item["statename"] for name, item in after.items()},
                          "p4runtime_verify": "passed", "p4runtime_unsupported_action": "rejected",
                          "configdb_rendered_startup": True,
                          "limits": "Software startup, arbitration and P4Info validation; no ASIC or forwarding claim"}))
    except Exception as error:
        diagnostics = {path.name: path.read_text(errors="replace")[-12000:] for path in root.glob("*.log")}
        raise AssertionError(str(error) + "\n" + json.dumps(diagnostics)) from error
    finally:
        connection.close()
        for process in reversed(list(children.values())):
            if process.poll() is None:
                os.killpg(process.pid, signal.SIGTERM)
        for process in children.values():
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
        for log in logs:
            log.close()


if __name__ == "__main__":
    main()
