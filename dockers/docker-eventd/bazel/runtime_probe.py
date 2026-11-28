#!/usr/bin/env python3
"""Check installed Eventd services using private Redis and container-local ZMQ."""

import configparser
import hashlib
import json
import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import time

import redis

sys.path.insert(0, str(Path(__file__).parent))
import runtime_smoke


def require(value, message):
    if not value:
        raise AssertionError(message)


def command(*arguments, cwd=None):
    result = subprocess.run(arguments, cwd=cwd, capture_output=True, text=True, timeout=45)
    require(result.returncode == 0, repr(arguments) + "\n" + result.stdout + result.stderr)
    return result.stdout


def main():
    report = json.loads(Path(sys.argv[1]).read_text())
    variant = sys.argv[2]
    expected = report["images"][variant]
    print("Checking installed native files", file=sys.stderr, flush=True)
    require(os.environ.get("IMAGE_VERSION") == report["image_version"], "Make image version was not retained")
    for name in expected["native_paths"]:
        path = Path("/") / name
        require(hashlib.sha256(path.read_bytes()).hexdigest() == report["native"][name]["sha256"], "native bytes differ: " + name)
        require("not found" not in command("/lib64/ld-linux-x86-64.so.2", "--list", str(path)), name)
    print("Native loaders passed", file=sys.stderr, flush=True)
    supervisor = configparser.ConfigParser(interpolation=None)
    supervisor.read("/etc/supervisor/conf.d/supervisord.conf")
    for name, executable in (("eventd", "/usr/bin/eventd"), ("eventdb", "/usr/bin/eventdb_wrapper.sh")):
        require(supervisor["program:" + name]["command"] == executable, "service command changed: " + name)
        require(supervisor["program:" + name]["dependent_startup_wait_for"] == "start:exited", "service order changed")
    require(Path("/etc/supervisor/critical_processes").read_text().strip() == "program:eventd", "critical process changed")
    for name in ("/usr/bin/start.sh", "/usr/bin/eventdb_wrapper.sh"):
        command("/bin/bash", "-n", name)
    command("/usr/bin/start.sh")
    debug_checks = []
    if variant.endswith("-dbg"):
        require("Huge version" in command("/usr/bin/vim", "--version"), "debug image still selects vim.tiny")
        pairs = [item for item in report["source_debug_pairs"]
                 if item["path"] in {"usr/bin/eventd", "usr/bin/eventdb", "usr/bin/events_tool"}]
        require(len(pairs) == 3, "Eventd debugger inputs are incomplete")
        debug_checks = [runtime_smoke.debug_pair(item) for item in pairs]
    print("Startup and debugger checks passed", file=sys.stderr, flush=True)

    # Compare the build-time tool with the actual installed upstream j2 command,
    # using the original Make JSON/template inputs and byte-for-byte output.
    source = Path("/probe/source")
    template = source / "rsyslog_plugin.conf.j2"
    for event in ("host", "bgp", "dhcp_relay", "swss", "syncd"):
        rendered = command("/usr/local/bin/j2", "--quiet", "-f", "json", str(template), str(source / (event + "_events_info.json")))
        installed = Path("/etc/rsyslog.d/rsyslog_plugin_conf") / (event + "_events.conf")
        require(installed.read_bytes() == rendered.encode(), "installed host rsyslog config differs: " + event)

    processes, logs = [], []
    with tempfile.TemporaryDirectory(prefix="eventd-installed-") as temporary:
        root = Path(temporary)
        invalid = root / "missing-proclist.json"
        invalid.write_text('{"yang_module": "eventd-ci"}\n')
        failed = subprocess.run(["/usr/local/bin/j2", "--quiet", "-f", "json", str(template), str(invalid)],
                                text=True, capture_output=True, timeout=15)
        require(failed.returncode != 0 and "proclist" in failed.stderr, "renderer accepted missing required data")
        fixture = Path("/probe/redis")
        database = json.loads((fixture / "database_config.json").read_text())
        redis_socket = "/run/redis/redis.sock"
        for instance in database["INSTANCES"].values():
            instance.update(hostname="127.0.0.1", port=6379, unix_socket_path=redis_socket)
        database["DATABASES"].setdefault("EVENT_DB", {"id": 9, "separator": "|", "instance": "redis"})
        config = Path("/run/redis/sonic-db/database_config.json")
        config.parent.mkdir(parents=True, exist_ok=True)
        config.write_text(json.dumps(database))
        profile = Path("/etc/evprofile/default.json")
        profile.write_bytes((fixture / "default.json").read_bytes())
        # The production default is empty. The real wrapper must skip eventdb.
        require("Skipping eventdb start" in command("/usr/bin/eventdb_wrapper.sh"), "empty profile did not skip eventdb")

        def start(name, arguments):
            log = (root / (name + ".log")).open("w+")
            logs.append(log)
            process = subprocess.Popen(arguments, cwd=root, stdout=log, stderr=subprocess.STDOUT)
            processes.append(process)
            return process

        def details():
            return "\n".join(path.name + ":\n" + path.read_text() for path in root.glob("*.log"))

        def wait_until(predicate, description, seconds=20):
            deadline = time.monotonic() + seconds
            while True:
                try:
                    if predicate():
                        return
                except (OSError, redis.RedisError):
                    pass
                require(all(process.poll() is None for process in processes) and time.monotonic() < deadline,
                        description + "\n" + details())
                time.sleep(0.1)

        try:
            server = start("redis", [str(fixture / "usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2"), "--library-path",
                str(fixture / "usr/lib/x86_64-linux-gnu"), str(fixture / "usr/bin/redis-server"),
                "--bind", "127.0.0.1", "--port", "6379", "--unixsocket", redis_socket,
                "--save", "", "--appendonly", "no", "--databases", "32", "--dir", str(root)])
            event_db = redis.Redis(unix_socket_path=redis_socket, db=database["DATABASES"]["EVENT_DB"]["id"],
                                   decode_responses=True, socket_timeout=1)
            wait_until(event_db.ping, "private Redis did not start")
            print("Private Redis started", file=sys.stderr, flush=True)
            require(event_db.info("server")["process_id"] == server.pid, "connected to another Redis process")
            eventd = start("eventd", ["/usr/bin/eventd"])

            def listening():
                with socket.create_connection(("127.0.0.1", 5571), timeout=0.2):
                    return True

            wait_until(listening, "installed eventd proxy did not start")
            print("Eventd proxy started", file=sys.stderr, flush=True)
            profile.write_text(json.dumps({"events": [{"name": "SYSTEM_STATE", "severity": "INFORMATIONAL",
                                                        "enable": "true", "message": ""}]}))
            eventdb = start("eventdb", ["/usr/bin/eventdb_wrapper.sh"])
            separator = database["DATABASES"]["EVENT_DB"]["separator"]
            wait_until(lambda: event_db.hget("EVENT_STATS" + separator + "state", "events") == "0",
                       "nonempty profile did not start the installed eventdb")
            # Allow the real ZMQ subscription handshake before sending distinct
            # events. The deadline below checks database effects, not just exit.
            time.sleep(1)
            inputs = root / "events.jsonl"
            messages = ["Eventd installed-image probe one", "Eventd installed-image probe two"]
            inputs.write_text("".join(json.dumps({"sonic-events-eventd:system-state": {
                "type-id": "SYSTEM_STATE", "resource": "eventd-ci-" + str(index), "text": message}}) + "\n"
                for index, message in enumerate(messages)))
            output = command("/usr/bin/events_tool", "-s", "-i", str(inputs), "-n", "2", "-p", "100", cwd=root)
            require("Sent 2 events" in output, "installed event publisher did not send both events")

            def delivered():
                rows = [event_db.hgetall(key) for key in event_db.scan_iter("EVENT" + separator + "*")]
                return set(messages).issubset({row.get("text") for row in rows})

            wait_until(delivered, "published events did not reach EVENT_DB")
            require(eventd.poll() is None and eventdb.poll() is None, "event service exited after delivery")
        finally:
            for process in reversed(processes):
                if process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait()
            for log in logs:
                log.close()
    print(json.dumps({"status": "passed", "variant": variant, "native_files": len(expected["native_paths"]),
                      "debug_checks": debug_checks,
                      "host_config_parity": 5, "missing_template_data_rejected": True,
                      "empty_profile_skipped": True, "nonempty_profile_started_eventdb": True,
                      "published_events_observed_in_redis": 2, "private_redis": True, "host_services_used": False}))


if __name__ == "__main__":
    main()
