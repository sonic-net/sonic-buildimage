#!/usr/bin/env python3
"""Exercise installed Sysmgr with private Redis and a non-rebooting D-Bus mock."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import time
from xmlrpc.client import ServerProxy

import installed_reboot_test as owner
import runtime_smoke
from supervisor.xmlrpc import SupervisorTransport


def require(value, message):
    if not value:
        raise AssertionError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--validation", type=Path, required=True)
    parser.add_argument("--debug", action="store_true")
    args = parser.parse_args()
    for name in ("LD_PRELOAD", "LD_LIBRARY_PATH", "LD_AUDIT", "PYTHONPATH", "DBUS_SESSION_BUS_ADDRESS"):
        os.environ.pop(name, None)
    report = json.loads(args.validation.read_text())
    require(report["status"] == "passed" and report["architecture"] == "amd64", "unvalidated image")
    native = report["native"] if args.debug else report["runtime_native"]
    for name, item in native.items():
        path = Path("/") / name
        info = path.stat()
        require(hashlib.sha256(path.read_bytes()).hexdigest() == item["sha256"], "installed source differs: " + name)
        require((info.st_uid, info.st_gid, info.st_mode & 0o7777) == (item["uid"], item["gid"], item["mode"]),
                "installed source metadata differs: " + name)
    programs = {"usr/bin/rebootbackend", "usr/lib/x86_64-linux-gnu/librebootgnoi.so.0.0.0",
                "usr/lib/x86_64-linux-gnu/libprotobuf.so.32.0.12"}
    probe = runtime_smoke.probe({
        "architecture": "amd64", "elfs": [item for item in report["elfs"] if item["path"] in native],
        "debug_pairs": [item for item in report["source_debug_pairs"] if item["path"] in programs],
        "policy": {
            "python_imports": ["swsscommon.swsscommon"],
            "commands": [{"argv": ["/usr/bin/vim", "--version"], "output_pattern": "Huge version"}]
            if args.debug else [],
        },
    }, "debug" if args.debug else "runtime")
    require(probe["passed"], json.dumps(probe))

    root = Path("/run/sysmgr-fixture")
    root.mkdir()
    db = Path("/run/redis/sonic-db")
    db.mkdir(parents=True)
    settings = json.loads(Path("/probe/services/database_config.json").read_text())
    settings["INSTANCES"]["redis"].update(hostname="127.0.0.1", port=6379, unix_socket_path="/run/redis/redis.sock")
    require(settings["DATABASES"]["CONFIG_DB"]["id"] == 4 and settings["DATABASES"]["STATE_DB"]["id"] == 6,
            "inherited Redis database identities changed")
    (db / "database_config.json").write_text(json.dumps(settings))
    sock, bus = Path("/run/redis/redis.sock"), root / "bus.sock"
    (root / "bus.conf").write_text(
        '<busconfig><type>system</type><listen>unix:path=' + str(bus) + '</listen>'
        '<auth>EXTERNAL</auth><policy context="default"><allow user="*"/>'
        '<allow own="*"/><allow send_destination="*"/>'
        '<allow receive_sender="*"/></policy></busconfig>')
    os.environ["DBUS_SYSTEM_BUS_ADDRESS"] = "unix:path=" + str(bus)
    controls, events, ready = (root / name for name in ("controls", "events", "ready"))
    controls.write_text('1\n0\nfixture rejected reboot\n{}\n')
    for path in ("/var/log/supervisor", "/var/spool/rsyslog"):
        Path(path).mkdir(parents=True, exist_ok=True)
    children, logs, clients, observations = {}, [], [], []

    def start(name, command):
        log = (root / (name + ".log")).open("wb")
        logs.append(log)
        if name in ("redis", "dbus"):
            # Private test services use their declared library closure. The
            # installed Sysmgr, Supervisor and mock still use the image loader.
            command = ["/probe/services/usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2",
                       "--library-path", "/probe/services/usr/lib/x86_64-linux-gnu", *command]
        children[name] = subprocess.Popen(command, stdin=subprocess.DEVNULL, stdout=log,
                                          stderr=subprocess.STDOUT, start_new_session=True)

    def states():
        if not Path("/run/supervisor.sock").exists():
            return None
        transport = SupervisorTransport(None, None, "unix:///var/run/supervisor.sock")
        with ServerProxy("http://localhost", transport=transport) as rpc:
            return {item["name"]: item for item in rpc.supervisor.getAllProcessInfo()}

    try:
        start("redis", ["/probe/services/usr/bin/redis-server", "--bind", "127.0.0.1", "--port", "6379", "--unixsocket", str(sock),
                        "--unixsocketperm", "700", "--save", "", "--appendonly", "no"])
        start("dbus", ["/probe/services/usr/bin/dbus-daemon", "--nofork", "--config-file=" + str(root / "bus.conf")])
        owner.wait_for(lambda: sock.exists() and bus.exists(), children)
        def redis_ready():
            connection = None
            try:
                connection = owner.Redis(sock)
                return connection.call("PING") == "PONG"
            except (OSError, AssertionError):
                return False
            finally:
                if connection is not None:
                    connection.close()

        owner.wait_for(redis_ready, children)
        publisher, subscriber = owner.Redis(sock), owner.Redis(sock)
        clients.extend((publisher, subscriber))
        require(publisher.call("PING") == "PONG", "private Redis is unavailable")
        publisher.call("SELECT", 4)
        publisher.call("SET", "CONFIG_DB_INITIALIZED", "1")
        publisher.call("HSET", "FEATURE|sysmgr", "state", "enabled", "auto_restart", "enabled")
        publisher.call("HSET", "LOGGER|rebootbackend", "LOGOUTPUT", "STDOUT")
        publisher.call("SELECT", 6)
        publisher.call("HSET", "WARM_RESTART_ENABLE_TABLE|system", "enable", "false")
        subscriber.call("SUBSCRIBE", "Reboot_Response_Channel")
        start("mock", ["/probe/dbus-mock", str(controls), str(events), str(ready)])
        owner.wait_for(ready.exists, children)
        start("supervisor", ["/usr/local/bin/supervisord"])

        def started():
            records = states()
            return records if records and all(records.get(name, {}).get("statename") == "RUNNING" for name in
                ("supervisor-proc-exit-listener", "rsyslogd", "rebootbackend")) else None

        records = owner.wait_for(started, children, timeout=45)
        pid = records["rebootbackend"]["pid"]
        require((Path("/proc") / str(pid) / "exe").resolve() == Path("/usr/bin/rebootbackend"),
                "Supervisor started a different daemon")
        owner.wait_for(lambda: publisher.call("PUBSUB", "NUMSUB", "Reboot_Request_Channel")[1] == 1, children)

        def request(op, payload="{}", field="MESSAGE", expected="SWSS_RC_SUCCESS"):
            wire = json.dumps([op, "StatusCode", field, payload])
            require(publisher.call("PUBLISH", "Reboot_Request_Channel", wire) == 1, "missing installed subscriber")
            response = subscriber.receive()
            require(response[:2] == ["message", "Reboot_Response_Channel"], "wrong response channel")
            result = json.loads(response[2])
            require(len(result) == 4 and result[:3] == [op, expected, "MESSAGE"], repr(result))
            observations.append({"operation": op, "expected": expected, "response": result[3]})
            return result[3]

        initial = json.loads(request("RebootStatus"))
        require(not initial.get("active", False), "unexpected initial reboot state")
        request("UnknownOperation", expected="SWSS_RC_INVALID_PARAM")
        request("Reboot", field="NOT_MESSAGE", expected="SWSS_RC_INVALID_PARAM")
        request("CancelReboot", expected="SWSS_RC_UNIMPLEMENTED")
        request("Reboot", '{"method":"COLD","delay":"1"}', expected="SWSS_RC_INVALID_PARAM")
        require(owner.read_events(events) == [], "invalid requests reached D-Bus")
        cold = {"method": "COLD", "message": "installed private image fixture"}
        request("Reboot", json.dumps(cold))

        def rejected():
            value = json.loads(request("RebootStatus"))
            return value if not value.get("active", False) and value.get("count", 0) == 1 else None

        result = owner.wait_for(rejected, children)
        require(result["method"] == "COLD" and result["reason"] == cold["message"], "request identity changed")
        require(result["status"] == {"status": "STATUS_FAILURE", "message": "fixture rejected reboot"},
                "mock rejection was not delivered")
        calls = owner.read_events(events)
        require(len(calls) == 1 and calls[0][0] == "issue_reboot" and json.loads(calls[0][1]) == cold,
                "installed daemon did not reach the private D-Bus service")
        final = started()
        require(final and final["rebootbackend"]["pid"] == pid, "installed daemon exited or restarted")
        print(json.dumps({"status": "passed", "source_elf_count": len(native),
                         "loader_checks": len(probe["loader_checks"]), "debug_checks": len(probe["debug_pairs"]),
                         "debug_tool_checks": len(probe["commands"]),
                         "supervisor_states": {name: item["statename"] for name, item in final.items()},
                         "observations": observations, "dbus_requests": calls,
                         "scope": "Original installed Supervisor startup and Sysmgr Redis-to-D-Bus notifications using a private rejecting mock; no platform reboot or full VS image claim."}))
    except Exception:
        for name in children:
            print(name + ": " + (root / (name + ".log")).read_text(errors="replace")[-8000:])
        raise
    finally:
        for client in clients:
            client.close()
        for process in reversed(list(children.values())):
            if process.poll() is None:
                os.killpg(process.pid, signal.SIGTERM)
        for process in children.values():
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait(timeout=5)
        for log in logs:
            log.close()


if __name__ == "__main__":
    main()
