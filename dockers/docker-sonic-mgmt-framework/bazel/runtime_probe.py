#!/usr/bin/env python3
"""Exercise installed REST, CLI, configuration and native libraries in isolation."""

import configparser
import hashlib
import importlib
import json
import os
from pathlib import Path
import signal
import socket
import ssl
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request
from xmlrpc.client import ServerProxy

import redis
from supervisor.xmlrpc import SupervisorTransport


def require(value, message):
    if not value:
        raise AssertionError(message)


def command(*arguments, env=None, cwd=None):
    result = subprocess.run(arguments, env=env, cwd=cwd, capture_output=True, text=True, timeout=45)
    require(result.returncode == 0, repr(arguments) + "\n" + result.stdout + result.stderr)
    return result.stdout + result.stderr


def services_ready(records, expected):
    if set(records) != expected:
        return False
    start = records["start"]
    dependency = records["dependent-startup"]
    return (start["statename"] == "EXITED" and start["exitstatus"] == 0
            and (dependency["statename"] == "RUNNING" or
                 (dependency["statename"] == "EXITED" and dependency["exitstatus"] == 0))
            and all(item["statename"] == "RUNNING" and item["pid"] > 0
                    for name, item in records.items() if name not in ("start", "dependent-startup")))


def private_redis_identity(unix, tcp, pid):
    require(unix.ping() and tcp.ping(), "both private Redis endpoints must respond")
    observed = {name: client.info("server")["process_id"] for name, client in (("unix", unix), ("tcp", tcp))}
    require(all(value == pid for value in observed.values()), "Redis endpoint reached a different process")
    return observed


def process_record(pid):
    directory = Path("/proc") / str(pid)
    status = dict(line.split(":", 1) for line in (directory / "status").read_text().splitlines())
    return {"pid": pid, "parent_pid": int(status["PPid"]),
            "executable": str((directory / "exe").resolve()),
            "argv": (directory / "cmdline").read_bytes().rstrip(b"\0").decode().split("\0")}


def main():
    report = json.loads(Path(sys.argv[1]).read_text())
    variant = sys.argv[2]
    if variant.endswith("-dbg"):
        require("Huge version" in command("/usr/bin/vim", "--version"), "debug command did not select full Vim")
    require(os.geteuid() == 0, "original global Supervisor configuration requires the image's root service user")
    for name in report["images"][variant]["native_paths"]:
        item = report["native"][name]
        path = Path("/") / name
        require(hashlib.sha256(path.read_bytes()).hexdigest() == item["sha256"], "native bytes differ: " + name)
        loader = ["/lib64/ld-linux-x86-64.so.2"]
        if name.startswith("usr/sbin/cli/"):
            # Match the installed clish_start launcher's private library path.
            loader += ["--library-path", "/usr/local/lib:/usr/sbin/cli/.libs"]
        require("not found" not in command(*loader, "--list", str(path)), name)
    for name in ("requests", "urllib3", "six", "swsscommon.swsscommon", "sonic_py_common", "supervisor"):
        module = importlib.import_module(name)
        require(str(Path(module.__file__).resolve()).startswith(("/usr/lib/", "/usr/local/lib/")), name)
    supervisor = configparser.ConfigParser(interpolation=None)
    supervisor.read(["/etc/supervisor/supervisord.conf",
                     *map(str, sorted(Path("/etc/supervisor/conf.d").glob("*.conf")))])
    require(supervisor["program:start"]["command"] == "/usr/bin/start.sh", "start supervisor entry changed")
    require(supervisor["program:rest-server"]["command"] == "/usr/bin/rest-server.sh", "REST supervisor entry changed")
    require(supervisor["program:rest-server"]["dependent_startup_wait_for"] == "start:exited", "REST startup dependency changed")
    for name in ("/usr/bin/start.sh", "/usr/bin/rest-server.sh"):
        command("/bin/bash", "-n", name)
    data = {"DEVICE_METADATA": {"localhost": {"hostname": "framework-ci"}},
            "REST_SERVER": {"default": {"port": "8443", "client_auth": "none"}}}
    rendered = command("sonic-cfggen", "-a", json.dumps(data), "-t", "/usr/share/sonic/templates/mgmt_vars.j2")
    # Match the existing startup script's conversion of cfggen's Python values.
    settings = json.loads(rendered.replace("'", '"'))
    require(settings["rest_server"] == data["REST_SERVER"]["default"], "management template failed")
    processes, logs = [], []
    with tempfile.TemporaryDirectory(prefix="framework-installed-") as temporary:
        root = Path(temporary)
        database_path = Path("/run/redis/sonic-db/database_config.json")
        database_path.parent.mkdir(parents=True, exist_ok=True)
        env = dict(os.environ, HOME=str(root), DB_CONFIG_PATH=str(database_path),
                   CVL_SCHEMA_PATH="/usr/sbin/schema", CVL_CFG_FILE="/usr/sbin/cvl_cfg.json",
                   YANG_MODELS_PATH="/usr/models/yang")
        database = json.loads(Path("/probe/redis/database_config.json").read_text())
        redis_socket = "/run/redis/redis.sock"
        for instance in database["INSTANCES"].values():
            instance.update(hostname="127.0.0.1", port=6379, unix_socket_path=redis_socket)
        for name, number in (("LOGLEVEL_DB", 3), ("ERROR_DB", 8), ("EVENT_DB", 9)):
            database["DATABASES"].setdefault(name, {"id": number, "separator": "|", "instance": "redis"})
        require(database["DATABASES"]["CONFIG_DB"]["id"] == 4, "CONFIG_DB identity changed")
        database_path.write_text(json.dumps(database))
        (database_path.parent / "database_global.json").write_text(json.dumps({"INCLUDES": [{"include": database_path.name}]}))
        for name in ("/var/log/supervisor", "/var/spool/rsyslog"):
            Path(name).mkdir(parents=True, exist_ok=True)

        def start(name, arguments):
            log = (root / (name + ".log")).open("w+")
            logs.append(log)
            process = subprocess.Popen(arguments, cwd=root, env=env, stdout=log, stderr=subprocess.STDOUT,
                                       start_new_session=True)
            processes.append(process)
            return process

        def details():
            return "\n".join(path.name + ":\n" + path.read_text() for path in root.glob("*.log"))

        try:
            fixture = Path("/probe/redis")
            redis_process = start("redis", [str(fixture / "usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2"), "--library-path",
                  str(fixture / "usr/lib/x86_64-linux-gnu"), str(fixture / "usr/bin/redis-server"),
                  "--bind", "127.0.0.1", "--port", "6379", "--unixsocket", redis_socket, "--save", "", "--appendonly", "no",
                  "--databases", "32", "--dir", str(root)])
            client = redis.Redis(unix_socket_path=redis_socket, db=4, socket_timeout=1)
            tcp_client = redis.Redis(host="127.0.0.1", port=6379, db=4, socket_timeout=1)
            deadline = time.monotonic() + 20
            while True:
                try:
                    if client.ping() and tcp_client.ping():
                        break
                except redis.RedisError:
                    require(time.monotonic() < deadline, details())
                    time.sleep(0.1)
            redis_identity = private_redis_identity(client, tcp_client, redis_process.pid)
            client.set("CONFIG_DB_INITIALIZED", "1")
            client.hset("DEVICE_METADATA|localhost", mapping={"hostname": "framework-ci"})
            client.hset("FEATURE|mgmt-framework", mapping={"state": "enabled", "auto_restart": "enabled"})
            with socket.socket() as reservation:
                reservation.bind(("127.0.0.1", 0))
                port = reservation.getsockname()[1]
            settings = {"port": str(port), "client_auth": "none"}
            client.hset("REST_SERVER|default", mapping=settings)
            rendered = command("sonic-cfggen", "-d", "-t", "/usr/share/sonic/templates/mgmt_vars.j2", env=env)
            require(json.loads(rendered.replace("'", '"'))["rest_server"] == settings, "cfggen did not read the private CONFIG_DB")
            certificate, key = Path("/tmp/cert.pem"), Path("/tmp/key.pem")
            require(not certificate.exists() and not key.exists(), "temporary certificate existed before original startup")
            service = start("supervisor", ["/usr/local/bin/supervisord"])
            expected = {section.split(":", 1)[1] for section in supervisor.sections()
                        if section.startswith(("program:", "eventlistener:"))}
            require({"start", "dependent-startup", "rest-server", "rsyslogd"} <= expected, "missing declared service")

            def service_records():
                transport = SupervisorTransport(None, None, "unix:///var/run/supervisor.sock")
                with ServerProxy("http://localhost", transport=transport) as rpc:
                    return {item["name"]: item for item in rpc.supervisor.getAllProcessInfo()}

            deadline = time.monotonic() + 45
            while True:
                require(service.poll() is None, details())
                try:
                    records = service_records()
                    if services_ready(records, expected):
                        break
                except OSError:
                    pass
                require(time.monotonic() < deadline, "original Supervisor startup did not finish\n" + details())
                time.sleep(0.1)
            require(Path("/var/sonic/config_status").read_bytes() == b"# Config files managed by sonic-config-engine\n", "original start hook failed")
            require(certificate.is_file() and key.is_file(), "original REST launcher did not generate its TLS certificate")
            pids = {name: item["pid"] for name, item in records.items() if name not in ("start", "dependent-startup")}
            processes_before = {name: process_record(pid) for name, pid in pids.items()}
            require(all(item["parent_pid"] == service.pid for item in processes_before.values()), "service is not a direct original Supervisor child")
            rest_argv = ["/usr/sbin/rest_server", "-ui", "/rest_ui", "-logtostderr", "-port", str(port),
                         "-client_auth", "none", "-cert", str(certificate), "-key", str(key)]
            require(processes_before["rest-server"]["argv"] == rest_argv and
                    processes_before["rest-server"]["executable"] == "/usr/sbin/rest_server", "original REST launcher arguments changed")
            context = ssl.create_default_context(cafile=str(certificate))
            opener = urllib.request.build_opener(urllib.request.ProxyHandler({}), urllib.request.HTTPSHandler(context=context))
            base = "https://127.0.0.1:" + str(port)
            deadline = time.monotonic() + 30
            while True:
                try:
                    with opener.open(base + "/restconf/yang-library-version", timeout=1) as response:
                        require(json.load(response)["ietf-restconf:yang-library-version"] == "2016-06-21", "REST version differs")
                    break
                except (OSError, urllib.error.URLError):
                    require(service.poll() is None and time.monotonic() < deadline, details())
                    time.sleep(0.1)
            with opener.open(base + "/.well-known/host-meta", timeout=5) as response:
                require(b"href='/restconf'" in response.read(), "REST discovery differs")
            with opener.open(base + "/restconf/operations", timeout=5) as response:
                require("sonic-show-techsupport:sonic-show-techsupport-info" in json.load(response)["operations"], "generated RPC absent")
            with opener.open(base + "/ui/index.html", timeout=5) as response:
                require(response.read() == Path("/etc/rest_server/ui/index.html").read_bytes(), "installed UI differs")
            cli = "/usr/sbin/cli"
            cli_env = dict(env, SYSTEM_NAME="sonic", PYTHONNOUSERSITE="1")
            launcher = cli + "/clish_start"
            require("2.1.4" in command(launcher, "-v", env=cli_env, cwd=root), "wrong klish version")
            command(launcher, "-k", "-c", "show tech-support", env=cli_env, cwd=root)
            rejected = subprocess.run([launcher, "-k", "-c", "not-a-real-sonic-command"], env=cli_env,
                                      cwd=root, capture_output=True, text=True, timeout=30)
            require(rejected.returncode == 255 and "Syntax error" in rejected.stdout + rejected.stderr,
                    "invalid CLI command was accepted")
            final_records = service_records()
            require(service.poll() is None and services_ready(final_records, expected), "original service stopped during API/CLI checks")
            require({name: final_records[name]["pid"] for name in pids} == pids, "persistent service restarted during API/CLI checks")
            require({name: process_record(pid) for name, pid in pids.items()} == processes_before, "service executable/ancestry changed")
            private_redis_identity(client, tcp_client, redis_process.pid)
            print(json.dumps({"status": "passed", "variant": variant,
                              "source_elf_loader_checks": len(report["images"][variant]["native_paths"]),
                              "redis_endpoints": redis_identity, "supervisor_pid": service.pid,
                              "service_user": {"uid": os.geteuid(), "gid": os.getegid()},
                              "service_processes": {name: {key: value for key, value in item.items() if key != "argv"}
                                                    for name, item in processes_before.items()},
                              "original_rest_arguments_verified": True,
                              "service_states": {name: item["statename"] for name, item in final_records.items()},
                              "checks": ["installed Python dependencies", "supervisor startup contract", "start hook",
                                         "cfggen management settings", "private Redis", "certificate generation", "verified TLS",
                                         "RESTCONF discovery/version", "generated RPC", "installed UI", "installed CLI command-tree parsing"],
                              "limits": "Private fixtures with authentication disabled; no host PAM/TACACS, hardware or VS boot acceptance"}))
        finally:
            for process in reversed(processes):
                if process.poll() is None:
                    try:
                        os.killpg(process.pid, signal.SIGTERM)
                    except ProcessLookupError:
                        pass
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    try:
                        os.killpg(process.pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass
                    process.wait(timeout=5)
            for log in logs:
                log.close()
            if "client" in locals():
                client.close()
            if "tcp_client" in locals():
                tcp_client.close()


if __name__ == "__main__":
    main()
