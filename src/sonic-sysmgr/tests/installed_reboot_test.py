"""Exercise the installed daemon inside the caller's isolated bwrap namespace.

The caller supplies declared executables, loader and complete private library path.
No host Redis/D-Bus socket is used. The service mock only records and responds.
"""

import argparse
import json
import os
from pathlib import Path
import signal
import socket
import subprocess
import time


class Redis:
    """Minimal RESP2 transport for this bounded notification fixture."""

    def __init__(self, path):
        self.socket = socket.socket(socket.AF_UNIX)
        self.socket.settimeout(3)
        self.socket.connect(str(path))
        self.stream = self.socket.makefile("rb")

    def send(self, *items):
        values = [str(x).encode() for x in items]
        self.socket.sendall(b"*%d\r\n" % len(values) + b"".join(
            b"$%d\r\n" % len(x) + x + b"\r\n" for x in values))

    def receive(self):
        line = self.stream.readline()
        if not line.endswith(b"\r\n"):
            raise AssertionError("truncated RESP reply")
        kind, value = line[:1], line[1:-2]
        if kind == b"+":
            return value.decode()
        if kind == b"-":
            raise AssertionError(value.decode())
        if kind == b":":
            return int(value)
        if kind == b"$":
            size = int(value)
            if size == -1:
                return None
            raw = self.stream.read(size + 2)
            assert len(raw) == size + 2 and raw.endswith(b"\r\n")
            return raw[:-2].decode()
        if kind == b"*":
            return [self.receive() for _ in range(int(value))]
        raise AssertionError("unknown RESP type")

    def call(self, *items):
        self.send(*items)
        return self.receive()

    def close(self):
        self.stream.close()
        self.socket.close()


def wait_for(check, processes, timeout=10):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        for name, process in processes.items():
            assert process.poll() is None, f"{name} exited: {process.returncode}"
        result = check()
        if result:
            return result
        time.sleep(0.05)
    raise AssertionError("fixture readiness/state deadline exceeded")


def read_events(path):
    events = []
    if not path.exists():
        return events
    with path.open("rb") as stream:
        while header := stream.readline():
            if not header.endswith(b"\n"):
                break  # The private service is still appending its last event.
            method, size = header.decode().rstrip("\n").split(" ")
            payload = stream.read(int(size))
            if len(payload) != int(size):
                break
            terminator = stream.read(1)
            if not terminator:
                break
            assert terminator == b"\n"
            events.append((method, payload.decode()))
    return events


def main():
    parser = argparse.ArgumentParser()
    for flag in ("loader", "library-path", "rebootbackend", "mock", "redis-server", "dbus-daemon", "evidence"):
        parser.add_argument("--" + flag, required=True)
    args = parser.parse_args()
    # The caller creates /run privately under bwrap --unshare-all; never run on host.
    assert os.environ.get("SONIC_SYSMGR_PRIVATE_FIXTURE") == "1"
    root = Path("/run/sysmgr-fixture")
    root.mkdir(exist_ok=False)
    redis_dir = Path("/run/redis")
    redis_dir.mkdir(exist_ok=True)
    config_dir = redis_dir / "sonic-db"
    config_dir.mkdir()
    assert Path("/var/run/redis/sonic-db").resolve() == config_dir
    sock = redis_dir / "redis.sock"
    config = {
        "INSTANCES": {"redis": {"hostname": "127.0.0.1", "port": 0, "unix_socket_path": str(sock)}},
        "DATABASES": {name: {"id": number, "separator": "|", "instance": "redis"}
                      for name, number in (("CONFIG_DB", 4), ("STATE_DB", 6))},
    }
    (config_dir / "database_config.json").write_text(json.dumps(config))
    bus = root / "bus.sock"
    (root / "bus.conf").write_text(
        '<busconfig><type>system</type><listen>unix:path=' + str(bus) + '</listen>'
        '<auth>EXTERNAL</auth><policy context="default"><allow user="*"/>'
        '<allow own="*"/><allow send_destination="*"/>'
        '<allow receive_sender="*"/></policy></busconfig>')
    env = dict(os.environ, DBUS_SYSTEM_BUS_ADDRESS="unix:path=" + str(bus))
    env.pop("DBUS_SESSION_BUS_ADDRESS", None)
    for variable in ("LD_PRELOAD", "LD_LIBRARY_PATH", "LD_AUDIT"):
        env.pop(variable, None)
    controls, events, ready = (root / x for x in ("controls", "events", "ready"))
    evidence = Path(args.evidence)
    evidence.mkdir(parents=True, exist_ok=True)
    children, logs, clients, observations = {}, [], [], []

    def control(issue=1, query=0, issue_message="fixture rejected reboot", query_message="{}"):
        tmp = controls.with_suffix(".tmp")
        tmp.write_text(f"{issue}\n{query}\n{issue_message}\n{query_message}\n")
        tmp.replace(controls)

    def start(name, executable, *extra):
        log = (evidence / (name + ".log")).open("wb")
        logs.append(log)
        children[name] = subprocess.Popen(
            [args.loader, "--library-path", args.library_path, executable, *extra],
            env=env, stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT,
            start_new_session=True)

    try:
        control()
        start("redis", args.redis_server, "--port", "0", "--unixsocket", str(sock),
              "--unixsocketperm", "700", "--save", "", "--appendonly", "no")
        start("bus", args.dbus_daemon, "--nofork", "--config-file=" + str(root / "bus.conf"))
        wait_for(lambda: sock.exists() and bus.exists(), children)
        def redis_ready():
            probe = None
            try:
                probe = Redis(sock)
                return probe.call("PING") == "PONG"
            except (OSError, AssertionError):
                return False
            finally:
                if probe is not None:
                    probe.close()

        wait_for(redis_ready, children)
        publisher, subscriber = Redis(sock), Redis(sock)
        clients.extend((publisher, subscriber))
        assert publisher.call("PING") == "PONG"
        assert publisher.call("SELECT", 4) == "OK"
        publisher.call("HSET", "LOGGER|rebootbackend", "LOGOUTPUT", "STDOUT")
        assert publisher.call("SELECT", 6) == "OK"
        publisher.call("HSET", "WARM_RESTART_ENABLE_TABLE|system", "enable", "false")
        assert subscriber.call("SUBSCRIBE", "Reboot_Response_Channel")[0] == "subscribe"
        start("mock", args.mock, str(controls), str(events), str(ready))
        wait_for(ready.exists, children)
        start("rebootbackend", args.rebootbackend)
        wait_for(lambda: publisher.call("PUBSUB", "NUMSUB", "Reboot_Request_Channel")[1] == 1, children)

        def request(op, payload="{}", field="MESSAGE", expected="SWSS_RC_SUCCESS"):
            wire = json.dumps([op, "StatusCode", field, payload])
            assert publisher.call("PUBLISH", "Reboot_Request_Channel", wire) == 1
            response = subscriber.receive()
            assert response[:2] == ["message", "Reboot_Response_Channel"]
            result = json.loads(response[2])
            assert len(result) == 4 and result[:3] == [op, expected, "MESSAGE"], result
            observations.append({"op": op, "expected": expected, "response": result[3]})
            return result[3]

        initial = json.loads(request("RebootStatus"))
        assert not initial.get("active", False) and initial.get("count", 0) == 0
        request("Reboot", field="NOT_MESSAGE", expected="SWSS_RC_INVALID_PARAM")
        request("UnknownOperation", expected="SWSS_RC_INVALID_PARAM")
        assert request("CancelReboot", expected="SWSS_RC_UNIMPLEMENTED") == "Cancel reboot isn't supported"
        request("Reboot", "{", expected="SWSS_RC_INTERNAL")
        request("Reboot", '{"method":"POWERDOWN"}', expected="SWSS_RC_INVALID_PARAM")
        request("Reboot", '{"method":"COLD","delay":"1"}', expected="SWSS_RC_INVALID_PARAM")
        assert read_events(events) == []
        cold = {"method": "COLD", "message": "installed private fixture COLD"}
        assert request("Reboot", json.dumps(cold)) == ""

        def failed():
            state = json.loads(request("RebootStatus"))
            return state if not state.get("active", False) and state.get("count", 0) == 1 else None

        state = wait_for(failed, children)
        assert state["method"] == "COLD" and state["reason"] == cold["message"]
        assert state["status"] == {"status": "STATUS_FAILURE", "message": "fixture rejected reboot"}
        captured = read_events(events)
        assert len(captured) == 1 and captured[0][0] == "issue_reboot"
        assert json.loads(captured[0][1]) == cold
        halt_status = {"active": True, "method": "HALT", "count": 23, "reason": "private mock status"}
        control(issue=0, issue_message="", query_message=json.dumps(halt_status))
        halt = {"method": "HALT", "message": "installed private fixture HALT"}
        assert request("Reboot", json.dumps(halt)) == ""
        wait_for(lambda: len(read_events(events)) == 2, children)
        assert json.loads(read_events(events)[1][1]) == halt
        request("Reboot", json.dumps(cold), expected="SWSS_RC_IN_USE")
        assert json.loads(request("RebootStatus")) == halt_status
        control(issue=0, query=1, issue_message="", query_message="fixture status rejected")
        assert request("RebootStatus", expected="SWSS_RC_INTERNAL") == "{}"
        captured = read_events(events)
        assert [method for method, _ in captured] == ["issue_reboot", "issue_reboot", "get_reboot_status", "get_reboot_status"]
        assert all(p.poll() is None for p in children.values())
        (evidence / "result.json").write_text(json.dumps({
            "status": "passed", "scope": "installed daemon/private Redis/private mock D-Bus; no platform reboot",
            "observations": observations, "dbus_requests": captured,
        }, indent=2) + "\n")
    finally:
        for client in clients:
            client.close()
        # Separate process groups and explicit PIDs: never signal host services.
        for process in reversed(list(children.values())):
            if process.poll() is None:
                try:
                    os.killpg(process.pid, signal.SIGTERM)
                except ProcessLookupError:
                    pass
        for process in children.values():
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                try:
                    os.killpg(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                process.wait(timeout=3)
        for log in logs:
            log.close()
        if events.exists():
            (evidence / "dbus-events.raw").write_bytes(events.read_bytes())


if __name__ == "__main__":
    main()
