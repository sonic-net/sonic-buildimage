#!/usr/bin/env python3
"""Exercise the original OTEL launchers inside the completed isolated image."""

import argparse
import configparser
from contextlib import contextmanager
import grp
import hashlib
import json
import os
from pathlib import Path
import pwd
import shutil
import signal
import socket
import subprocess
import tempfile
import time
from urllib.request import Request, urlopen
from xmlrpc.client import ServerProxy

from supervisor.xmlrpc import SupervisorTransport

import runtime_smoke
import redis


def require(value, message):
    if not value:
        raise AssertionError(message)


def command(argv, **kwargs):
    result = subprocess.run(argv, capture_output=True, text=True, timeout=30, **kwargs)
    require(result.returncode == 0, repr(argv) + "\n" + result.stdout + result.stderr)
    return result.stdout + result.stderr


def port():
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        return listener.getsockname()[1]


def negative_launchers(config):
    config.unlink(missing_ok=True)
    cases = [("missing", None, "OTEL config file not found"),
             ("invalid-yaml", "receivers: [\n", "YAML configuration is invalid"),
             ("invalid-collector", "receivers: {not_a_receiver: {}}\n", "unknown type")]
    result = {}
    for name, contents, expected in cases:
        if contents is not None:
            config.write_text(contents)
        run = subprocess.run(["/usr/bin/otel.sh"], capture_output=True, text=True, timeout=30)
        require(run.returncode != 0 and expected in run.stdout + run.stderr,
                "original launcher did not reject " + name + ":\n" + run.stdout + run.stderr)
        result[name] = {"exit_code": run.returncode, "expected_diagnostic": expected}
    return result


def send_trace(endpoint, output, name):
    stamp = time.time_ns()
    payload = {"resourceSpans": [{"resource": {"attributes": [{"key": "service.name", "value": {"stringValue": "sonic-otel-ci"}}]},
        "scopeSpans": [{"scope": {"name": "sonic-image-probe"}, "spans": [{
            "traceId": "0123456789abcdef0123456789abcdef", "spanId": "0123456789abcdef",
            "name": name, "kind": 1, "startTimeUnixNano": str(stamp), "endTimeUnixNano": str(stamp + 1000000)}]}]}]}
    request = Request(endpoint + "/v1/traces", data=json.dumps(payload).encode(), headers={"Content-Type": "application/json"})
    with urlopen(request, timeout=5) as response:
        require(response.status == 200, "installed collector rejected OTLP HTTP trace")
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline:
        if output.is_file():
            records = [json.loads(line) for line in output.read_text().splitlines() if line]
            names = [span["name"] for record in records for resource in record.get("resourceSpans", [])
                     for scope in resource.get("scopeSpans", []) for span in scope.get("spans", [])]
            if name in names:
                return {"span_name": name, "received_by_installed_file_exporter": True}
        time.sleep(0.1)
    raise AssertionError("installed collector did not export the supplied trace")


@contextmanager
def private_redis(work):
    """Use the declared test helper for the original startup and event listener."""
    fixture = Path("/probe/redis")
    socket_path, tcp_port = str(work / "redis.sock"), port()
    data = json.loads((fixture / "database_config.json").read_text())
    for instance in data["INSTANCES"].values():
        instance.update(hostname="127.0.0.1", port=tcp_port, unix_socket_path=socket_path)
    database = Path("/run/redis/sonic-db/database_config.json")
    database.parent.mkdir(parents=True, exist_ok=True)
    database.write_text(json.dumps(data))
    loader = fixture / "usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2"
    libraries = fixture / "usr/lib/x86_64-linux-gnu"
    argv = [str(loader), "--library-path", str(libraries), str(fixture / "usr/bin/redis-server"),
            "--bind", "127.0.0.1", "--port", str(tcp_port), "--unixsocket", socket_path,
            "--save", "", "--appendonly", "no", "--databases", "32", "--dir", str(work)]
    config = redis.Redis(host="127.0.0.1", port=tcp_port, db=4, socket_timeout=1)
    state = redis.Redis(unix_socket_path=socket_path, db=6, socket_timeout=1)
    with (work / "redis.log").open("w+") as log:
        process = subprocess.Popen(argv, stdout=log, stderr=subprocess.STDOUT)
        try:
            deadline = time.monotonic() + 15
            while process.poll() is None and time.monotonic() < deadline:
                try:
                    if config.info("server")["process_id"] == state.info("server")["process_id"] == process.pid:
                        break
                except redis.RedisError:
                    pass
                time.sleep(0.1)
            else:
                log.flush(); log.seek(0)
                raise AssertionError("private Redis did not become ready: " + log.read())
            config.set("CONFIG_DB_INITIALIZED", "1")
            config.hset("FEATURE|otel", mapping={"set_owner": "local"})
            state.hset("FEATURE|otel", mapping={"system_state": "up"})
            yield state
        finally:
            config.close(); state.close()
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill(); process.wait()


def service(work, config, supervisor=False, state=None):
    receiver, health = port(), port()
    output = work / ("supervisor-traces.json" if supervisor else "launcher-traces.json")
    settings = {
        "receivers": {"otlp": {"protocols": {"http": {"endpoint": "127.0.0.1:" + str(receiver)}}}},
        "exporters": {"file": {"path": str(output), "flush_interval": "100ms"}},
        "extensions": {"health_check": {"endpoint": "127.0.0.1:" + str(health)}},
        "service": {"extensions": ["health_check"], "pipelines": {"traces": {"receivers": ["otlp"], "exporters": ["file"]}}},
    }
    # JSON is valid YAML. Use the installed launcher's fixed path unchanged.
    config.write_text(json.dumps(settings))
    argv = ["/usr/local/bin/supervisord"] if supervisor else ["/usr/bin/otel.sh"]
    with (work / ("supervisor.log" if supervisor else "launcher.log")).open("w+") as log:
        process = subprocess.Popen(argv, stdout=log, stderr=subprocess.STDOUT, start_new_session=True,
                                   env=dict(os.environ, RUNTIME_OWNER="local"))
        try:
            deadline = time.monotonic() + 45
            while process.poll() is None and time.monotonic() < deadline:
                try:
                    with urlopen("http://127.0.0.1:" + str(health), timeout=0.3) as response:
                        if response.status == 200:
                            break
                except OSError:
                    time.sleep(0.1)
            else:
                log.flush(); log.seek(0)
                raise AssertionError("installed launcher did not start the collector:\n" + log.read())
            pid = process.pid
            if supervisor:
                required = {"otel", "rsyslogd", "supervisor-proc-exit-listener"}
                deadline = time.monotonic() + 15
                while time.monotonic() < deadline:
                    transport = SupervisorTransport(None, None, "unix:///var/run/supervisor.sock")
                    with ServerProxy("http://localhost", transport=transport) as rpc:
                        records = {item["name"]: item for item in rpc.supervisor.getAllProcessInfo()}
                    states = {name: item["statename"] for name, item in records.items()}
                    startup = records.get("dependent-startup", {})
                    startup_ok = startup.get("statename") == "RUNNING" or (
                        startup.get("statename") == "EXITED" and startup.get("exitstatus") == 0)
                    if startup_ok and all(states.get(name) == "RUNNING" for name in required):
                        break
                    time.sleep(0.2)
                else:
                    log.flush(); log.seek(0)
                    raise AssertionError("original supervisor/listener state differs: " + repr(states) + "\n" + log.read())
                pid = int(command(["/usr/local/bin/supervisorctl", "pid", "otel"]).strip())
                require(Path("/var/sonic/config_status").read_text().strip() ==
                        "# Config files managed by sonic-config-engine", "original start.sh did not execute")
                require(state.hget("FEATURE|otel", "current_owner") == b"local" and
                        state.hget("FEATURE|otel", "container_version") == os.environ["IMAGE_VERSION"].encode(),
                        "original container_startup state/version differs")
            require(os.path.samefile("/proc/" + str(pid) + "/exe", "/usr/bin/otelcol-contrib"),
                    "launcher did not exec the installed collector")
            trace = send_trace("http://127.0.0.1:" + str(receiver), output,
                               "sonic-supervisor-trace" if supervisor else "sonic-launcher-trace")
            return {"original_supervisor": supervisor, "actual_collector_executable": True,
                    "health_ready": True, "telemetry": trace}
        finally:
            if process.poll() is None:
                os.killpg(process.pid, signal.SIGTERM)
                try:
                    process.wait(timeout=8)
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL); process.wait()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--validation", type=Path, required=True)
    parser.add_argument("--debug", action="store_true")
    args = parser.parse_args()
    report = json.loads(args.validation.read_text())
    require(report["status"] == "passed" and report["architecture"] == "amd64", "unverified input")
    require(os.environ.get("IMAGE_VERSION") == report["image_version"], "image version changed")
    expected = report["native"] if args.debug else report["runtime_native"]
    for name, item in expected.items():
        require(hashlib.sha256((Path("/") / name).read_bytes()).hexdigest() == item["sha256"],
                "loaded source ELF changed: " + name)
    policy = {"architecture": "amd64", "elfs": [item for item in report["elfs"] if item["path"] in expected],
        "debug_pairs": [], "policy": {"commands": [
            {"argv": ["/usr/local/bin/otelcol-contrib", "--version"], "output_pattern": "0.144.0"},
            {"argv": ["curl", "--version"]}, {"argv": ["wget", "--version"]}],
        "python_imports": ["yaml", "supervisor", "supervisord_dependent_startup", "swsscommon.swsscommon"],
        "supervisor_config": "etc/supervisor/conf.d/supervisord.conf"}}
    if args.debug:
        policy["policy"]["commands"].append({"argv": ["/usr/bin/vim", "--version"], "output_pattern": "Huge version"})
    loads = runtime_smoke.probe_installed(policy, "debug" if args.debug else "runtime")
    require(loads["passed"], "installed inherited loader/command/import failed: " + json.dumps(loads["failures"]))
    account, group = pwd.getpwnam("otelcol-contrib"), grp.getgrnam("otelcol-contrib")
    require((account.pw_uid, account.pw_gid, group.gr_gid, account.pw_dir, account.pw_shell) ==
            (999, 999, 999, "/home/otelcol-contrib", "/sbin/nologin"), "service account differs")
    command(["/usr/bin/otelcol-contrib", "--version"], user=999, group=999)
    require(not Path(account.pw_dir).exists(), "unexpected service home")
    config = configparser.ConfigParser(interpolation=None)
    config.read("/etc/supervisor/conf.d/supervisord.conf")
    require(config["program:otel"]["command"] == "/usr/bin/otel.sh" and
            config["program:start"]["command"] == "/usr/bin/start.sh", "original supervisor entrypoints changed")
    require(Path("/etc/supervisor/critical_processes").read_text().strip() == "program:otel", "critical process changed")
    for directory in ("/etc/sonic", "/var/sonic", "/var/log/supervisor", "/var/spool/rsyslog"):
        Path(directory).mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="otel-image-") as temporary:
        work = Path(temporary)
        config_path = Path("/etc/sonic/otel_config.yml")
        negatives = negative_launchers(config_path)
        launcher = service(work, config_path)
        with private_redis(work) as state:
            supervisor = service(work, config_path, supervisor=True, state=state)
        symbols = {}
        if args.debug:
            binary = Path("/usr/bin/otelcol-contrib")
            isolated = work / "collector-without-symbols"
            shutil.copyfile(binary, isolated)
            empty = work / "empty-symbols"; empty.mkdir()
            symbols = {"matching": runtime_smoke.gdb_lookup(binary, work, "/usr/lib/debug", "main.main"),
                       "missing": runtime_smoke.gdb_lookup(isolated, work, str(empty), "main.main")}
            require(symbols["matching"].get("found") and not symbols["missing"].get("found"),
                    "installed collector detached symbols did not provide exclusive line/type lookup")
        print(json.dumps({"status": "passed", "source_elf_loader_checks": len(loads["loader_checks"]),
            "account": "999:999", "negative_launchers": negatives, "launcher": launcher,
            "supervisor": supervisor, "debug": symbols,
            "limits": "Private loopback telemetry and original launchers; no host namespaces, production backend or VS boot."}))


if __name__ == "__main__":
    main()
