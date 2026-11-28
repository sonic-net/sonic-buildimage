#!/usr/bin/env python3
"""Run installed MUX startup with a private Redis and a virtual MUX port."""

import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time

import redis

sys.path.insert(0, str(Path(__file__).parent))
import runtime_smoke


def require(value, message):
    if not value:
        raise AssertionError(message)


def command(arguments, timeout=40):
    result = subprocess.run(arguments, capture_output=True, text=True, timeout=timeout)
    require(result.returncode == 0, repr(arguments) + "\n" + result.stdout + result.stderr)
    return result.stdout


def wait_until(callback, message, timeout=35):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        value = callback()
        if value:
            return value
        time.sleep(0.1)
    raise AssertionError(message)


def main():
    report = json.loads(Path(sys.argv[1]).read_text())
    variant = sys.argv[2]
    expected = report["images"][variant]
    for name in expected["native_paths"]:
        path = Path("/") / name
        require(hashlib.sha256(path.read_bytes()).hexdigest() == report["native"][name]["sha256"],
                "native bytes differ: " + name)
        require("not found" not in command(["/lib64/ld-linux-x86-64.so.2", "--list", str(path)]),
                "native loader failed: " + name)
    debug_checks = []
    if variant.endswith("-dbg"):
        selected = [item for item in report["source_debug_pairs"] if item["path"] in
                    {"usr/sbin/linkmgrd", "usr/bin/sonic-db-cli", "usr/bin/ssh"}]
        require(any(item["path"] == "usr/sbin/linkmgrd" for item in selected), "missing Linkmgrd symbols")
        debug_checks = [runtime_smoke.debug_pair(item) for item in selected]
        for item in debug_checks:
            item["with_symbols"]["source"] = Path(item["with_symbols"]["source"]).name

    database = Path("/run/redis/sonic-db/database_config.json")
    database.parent.mkdir(parents=True, exist_ok=True)
    database.write_bytes(Path("/probe/redis/database_config.json").read_bytes())
    config = json.loads(database.read_text())
    socket_path = config["INSTANCES"]["redis"]["unix_socket_path"]
    require(Path(socket_path).resolve() == Path("/run/redis/redis.sock"), "unexpected private Redis socket")
    require(config["INSTANCES"]["redis"]["hostname"] == "127.0.0.1", "Redis fixture is not loopback")
    redis_binary = Path("/probe/redis/usr/bin/redis-server")
    fixture = {"executable": str(redis_binary), "sha256": hashlib.sha256(redis_binary.read_bytes()).hexdigest(),
               "database_config": str(database)}

    with runtime_smoke.redis_fixture(fixture):
        config_db = redis.Redis(unix_socket_path=socket_path, db=4, decode_responses=True)
        state_db = redis.Redis(unix_socket_path=socket_path, db=6, decode_responses=True)
        require(config_db.ping() and state_db.ping(), "private databases are unavailable")
        config_db.set("CONFIG_DB_INITIALIZED", "1")
        config_db.hset("DEVICE_METADATA|localhost", mapping={
            "mac": "02:00:00:00:00:01", "hostname": "mux-probe", "type": "ToRRouter"})
        config_db.hset("LOOPBACK_INTERFACE|Loopback2|192.0.2.10/32", mapping={"NULL": "NULL"})
        config_db.hset("LOOPBACK_INTERFACE|Loopback3|192.0.2.11/32", mapping={"NULL": "NULL"})
        config_db.hset("MUX_CABLE|Ethernet0", mapping={
            "server_ipv4": "192.0.2.2/32", "cable_type": "active-standby", "state": "standby"})
        for args in (["link", "add", "Ethernet0", "type", "veth", "peer", "name", "MuxPeer"],
                     ["link", "set", "Ethernet0", "address", "02:00:00:00:00:01"],
                     ["link", "set", "Ethernet0", "up"], ["link", "set", "MuxPeer", "up"],
                     ["addr", "add", "192.0.2.1/24", "dev", "Ethernet0"]):
            command(["/usr/sbin/ip", *args])
        for path in ("/var/log/supervisor", "/var/log/mux", "/var/spool/rsyslog"):
            Path(path).mkdir(parents=True, exist_ok=True)
        log_path = Path("/tmp/mux-startup.log")
        with log_path.open("w") as log:
            process = subprocess.Popen(["/usr/bin/docker-init.sh"], stdout=log, stderr=subprocess.STDOUT,
                                       start_new_session=True)
            try:
                def daemon_pid():
                    require(process.poll() is None, "entrypoint exited:\n" + log_path.read_text())
                    result = subprocess.run(["/usr/local/bin/supervisorctl", "pid", "linkmgrd"],
                                            capture_output=True, text=True, timeout=5)
                    return int(result.stdout.strip()) if result.returncode == 0 and result.stdout.strip().isdigit() else 0
                pid = wait_until(daemon_pid, "installed supervisor did not start Linkmgrd")
                require(os.path.samefile("/proc/" + str(pid) + "/exe", "/usr/sbin/linkmgrd"),
                        "supervisor did not execute installed Linkmgrd")
                arguments = Path("/proc/" + str(pid) + "/cmdline").read_bytes().split(b"\0")
                require(arguments[1:5] == [b"-v", b"warning", b"-d", b"-l"], "daemon arguments changed")
                state = wait_until(lambda: state_db.hget("MUX_LINKMGR_TABLE|Ethernet0", "state"),
                                   "installed Linkmgrd did not publish its port state")
                require(state in ("uninitialized", "unhealthy", "healthy"), "invalid Linkmgrd state")
                # The isolated Redis fixture deliberately has TCP disabled.
                observed = command(["/usr/bin/sonic-db-cli", "-s", "STATE_DB", "HGET",
                                    "MUX_LINKMGR_TABLE|Ethernet0", "state"]).strip()
                require(observed == state, "installed database client disagrees with the daemon state")
                require(daemon_pid() == pid, "daemon restarted during acceptance")
                # Constructor state publication alone is insufficient: retain the
                # same installed daemon while its startup callbacks settle.
                for _ in range(10):
                    time.sleep(0.5)
                    require(daemon_pid() == pid, "daemon did not remain alive after port creation")
                status = command(["/usr/local/bin/supervisorctl", "status", "rsyslogd"])
                require("RUNNING" in status, "dependent startup did not run rsyslogd")
            finally:
                if process.poll() is None:
                    os.killpg(process.pid, signal.SIGTERM)
                    try:
                        process.wait(timeout=8)
                    except subprocess.TimeoutExpired:
                        os.killpg(process.pid, signal.SIGKILL)
                        process.wait()

    print(json.dumps({"status": "passed", "variant": variant, "native_files": len(expected["native_paths"]),
                      "debug_checks": debug_checks, "actual_entrypoint": True, "actual_daemon": True,
                      "private_redis": True, "virtual_port_state": state, "database_cli": True,
                      "host_network_used": False, "host_services_used": False,
                      "scope": "Installed startup, native dependencies and real Redis state publication; no physical dual-ToR or hardware response simulation"}))


if __name__ == "__main__":
    main()
