#!/usr/bin/env python3
"""Run the source owner's installed gNMI checks on the completed image."""

import configparser
import ctypes
import hashlib
import importlib
import importlib.util
import json
import os
from pathlib import Path
import re
import secrets
import shutil
import signal
import socket
import subprocess
import sys
import tempfile
import time

import redis
from xmlrpc.client import ServerProxy
from supervisor.xmlrpc import SupervisorTransport


def require(value, message):
    if not value:
        raise AssertionError(message)


def command(*arguments, env=None):
    result = subprocess.run(arguments, env=env, capture_output=True, text=True, timeout=30)
    require(result.returncode == 0, repr(arguments) + "\n" + result.stdout + result.stderr)
    return result.stdout + result.stderr


def installed_debug(names, work):
    """Check the actual debug image rather than an extracted SDK tree."""
    empty = work / "no-symbols"
    empty.mkdir()
    records = {}
    for name in names:
        binary = Path("/usr/sbin") / name
        isolated = work / name
        shutil.copyfile(binary, isolated)
        outputs = {}
        for case, path, symbols in (("matching", binary, "/usr/lib/debug"),
                                    ("missing", isolated, str(empty))):
            result = subprocess.run([
                "gdb", "--batch", "--nx", "--quiet",
                "-ex", "set debuginfod enabled off", "-ex", "set auto-load off",
                "-ex", "set debug-file-directory " + symbols,
                "-ex", "file " + str(path), "-ex", "info line main.main",
            ], env=dict(os.environ, DEBUGINFOD_URLS="", HOME=str(work)),
                stdin=subprocess.DEVNULL, capture_output=True, text=True, timeout=30)
            outputs[case] = result.stdout + result.stderr
        lines = re.findall(r'Line ([0-9]+) of "([^"]+\.go)"', outputs["matching"])
        require(lines and not re.search(r"Line [0-9]+ of ", outputs["missing"]),
                "installed detached symbols do not provide exclusive lookup: " + name)
        records[name] = {"line": int(lines[0][0]), "source": Path(lines[0][1]).name,
                         "missing_debug_rejected": True}
    return records


def supervisor_startup(work, environment, config_client, state_client, dialout):
    """Run the unchanged launcher chain against isolated CONFIG_DB data."""
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        port = listener.getsockname()[1]
    config_client.set("CONFIG_DB_INITIALIZED", "1")
    config_client.hset("FEATURE|gnmi", mapping={"set_owner": "local"})
    state_client.hset("FEATURE|gnmi", mapping={"system_state": "up"})
    # Explicitly disable authentication only inside this private, network-none
    # fixture. Production launcher defaults and authentication are unchanged.
    config_client.hset("GNMI|gnmi", mapping={"port": str(port), "user_auth": "none",
                                            "client_auth": "false", "log_level": "1"})
    for name in ("/var/log/supervisor", "/var/spool/rsyslog", "/var/sonic"):
        Path(name).mkdir(parents=True, exist_ok=True)
    log_path = work / "supervisor.log"
    services = ("rsyslogd", "gnmi-native", "supervisor-proc-exit-listener", *(("dialout",) if dialout else ()))
    env = dict(environment, RUNTIME_OWNER="local")
    with log_path.open("w") as log:
        process = subprocess.Popen(["/usr/local/bin/supervisord"], env=env,
                                   stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
        try:
            deadline = time.monotonic() + 45
            last = ""
            while process.poll() is None and time.monotonic() < deadline:
                try:
                    transport = SupervisorTransport(None, None, "unix:///var/run/supervisor.sock")
                    with ServerProxy("http://localhost", transport=transport) as rpc:
                        records = {item["name"]: item for item in rpc.supervisor.getAllProcessInfo()}
                    last = json.dumps(records)
                    startup = records.get("dependent-startup", {})
                    start = records.get("start", {})
                    if (all(records.get(name, {}).get("statename") == "RUNNING" for name in services)
                            and (startup.get("statename") == "RUNNING" or
                                 (startup.get("statename") == "EXITED" and startup.get("exitstatus") == 0))
                            and start.get("statename") == "EXITED" and start.get("exitstatus") == 0):
                        break
                except (OSError, ConnectionError):
                    pass
                time.sleep(0.2)
            else:
                raise AssertionError("installed supervisor chain did not start:\n" + last + "\n" + log_path.read_text())
            pid = int(command("/usr/local/bin/supervisorctl", "pid", "gnmi-native", env=env).strip())
            require(pid > 0 and os.path.samefile("/proc/" + str(pid) + "/exe", "/usr/sbin/telemetry"),
                    "supervisor did not execute installed telemetry")
            if dialout:
                child = int(command("/usr/local/bin/supervisorctl", "pid", "dialout", env=env).strip())
                require(child > 0 and os.path.samefile("/proc/" + str(child) + "/exe", "/usr/sbin/dialout_client_cli"),
                        "supervisor did not execute installed dialout client")
            require(Path("/var/sonic/config_status").read_text().strip() ==
                    "# Config files managed by sonic-config-engine", "original start.sh did not execute")
            require(state_client.hget("FEATURE|gnmi", "current_owner") == b"local" and
                    state_client.hget("FEATURE|gnmi", "container_version") == environment["IMAGE_VERSION"].encode(),
                    "original container_startup state/version differs")
            values = ["gnmi-launcher-first", "gnmi-launcher-second"]
            get = ["/usr/sbin/gnmi_get", "-notls", "-target_addr", "127.0.0.1:" + str(port),
                   "-time_out", "2s", "-xpath", "/sonic-db:CONFIG_DB/localhost/DEVICE_METADATA/localhost/hostname"]
            for value in values:
                config_client.hset("DEVICE_METADATA|localhost", "hostname", value)
                require(value in command(*get, env=env), "launcher-backed Get did not reflect CONFIG_DB")
            require(process.poll() is None, "supervisor exited after service startup")
            transport = SupervisorTransport(None, None, "unix:///var/run/supervisor.sock")
            with ServerProxy("http://localhost", transport=transport) as rpc:
                final = {item["name"]: item for item in rpc.supervisor.getAllProcessInfo()}
            require(all(final[name]["statename"] == "RUNNING" and
                        final[name]["pid"] == records[name]["pid"] for name in services),
                    "a persistent service exited or restarted during the Get checks")
            return {"actual_supervisor": True, "services": list(services), "real_get_values": values,
                    "container_startup_state": True, "host_namespaces_used": False}
        finally:
            if process.poll() is None:
                os.killpg(process.pid, signal.SIGTERM)
                try:
                    process.wait(timeout=8)
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL)
                    process.wait()


def main():
    report = json.loads(Path(sys.argv[1]).read_text())
    variant = sys.argv[2]
    image = report["images"][variant]
    for name in image["native_paths"]:
        expected = report["native"][name]
        path = Path("/") / name
        require(hashlib.sha256(path.read_bytes()).hexdigest() == expected["sha256"],
                "installed native bytes differ: " + name)
        require("not found" not in command("/lib64/ld-linux-x86-64.so.2", "--list", str(path)), name)
    for name in ("swsscommon.swsscommon", "sonic_py_common", "supervisor", "redis"):
        module = importlib.import_module(name)
        require(str(Path(module.__file__).resolve()).startswith(("/usr/lib/", "/usr/local/lib/")), name)
    supervisor = configparser.ConfigParser(interpolation=None)
    supervisor.read("/etc/supervisor/conf.d/supervisord.conf")
    require(supervisor["program:gnmi-native"]["command"] == "/usr/bin/gnmi-native.sh",
            "gNMI supervisor command changed")
    dialout = "program:dialout" in supervisor
    require(dialout == report["dialout_enabled"], "supervisor dialout selection differs")
    require(Path("/usr/sbin/dialout_client_cli").exists() == dialout, "dialout executable differs")
    require(Path("/usr/bin/dialout.sh").exists() == dialout, "dialout launcher differs")
    require(os.environ.get("IMAGE_VERSION") == report["image_version"], "image version environment differs")
    for name in ("start.sh", "gnmi-native.sh", *(["dialout.sh"] if dialout else [])):
        command("/bin/bash", "-n", "/usr/bin/" + name)
    settings = {"GNMI": {"gnmi": {"port": "50051"}}, "DEVICE_METADATA": {"localhost": {"hostname": "gnmi-ci"}}}
    rendered = command("sonic-cfggen", "-a", json.dumps(settings), "-t", "/usr/share/sonic/templates/telemetry_vars.j2")
    require(json.loads(rendered.replace("'", '"'))["gnmi"] == settings["GNMI"]["gnmi"],
            "installed telemetry template differs")
    require(Path("/etc/supervisor/critical_processes").read_text().strip() == "program:gnmi-native",
            "critical process contract differs")
    if variant.endswith("-dbg"):
        require("Huge version" in command("/usr/bin/vim", "--version"), "debug image still selects vim.tiny")
    owner_probe = Path("/probe/installed_probe.py")
    require(hashlib.sha256(owner_probe.read_bytes()).hexdigest() == report["artifacts"]["gnmi-installed-probe.py"]["sha256"],
            "source owner probe changed")
    spec = importlib.util.spec_from_file_location("gnmi_installed_probe", owner_probe)
    probe = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(probe)
    with tempfile.TemporaryDirectory(prefix="gnmi-image-", dir="/tmp") as temporary:
        work = Path(temporary)
        binaries = work / "build/bin"
        binaries.mkdir(parents=True)
        names = ["telemetry", "gnmi_get", "gnmi_set", "gnmi_cli", "gnoi_client",
                 "gnoi_openconfig_client", "gnoi_sonic_client", "gnmi_dump"]
        if dialout:
            names.append("dialout_client_cli")
        for name in names:
            (binaries / name).symlink_to("/usr/sbin/" + name)
        with socket.socket() as reservation:
            reservation.bind(("127.0.0.1", 0))
            port = reservation.getsockname()[1]
        unix_socket = str(work / "redis.sock")
        config = json.loads(Path("/probe/redis/database_config.json").read_text())
        for instance in config["INSTANCES"].values():
            instance.update(hostname="127.0.0.1", port=port, unix_socket_path=unix_socket)
        config_path = work / "database_config.json"
        config_path.write_text(json.dumps(config))
        installed_config = Path("/run/redis/sonic-db/database_config.json")
        installed_config.parent.mkdir(parents=True, exist_ok=True)
        installed_config.write_bytes(config_path.read_bytes())
        checkpoint = work / "checkpoint"
        checkpoint.mkdir()
        environment = dict(os.environ, DB_CONFIG_PATH=str(config_path), HOME=str(work),
                           CVL_SCHEMA_PATH="/usr/sbin/schema", CVL_CFG_FILE="/usr/sbin/cvl_cfg.json",
                           YANG_MODELS_PATH="/usr/models/yang", SWSS_COMMON_LUA_DIR="/usr/share/swss",
                           SONIC_TEST_NATIVE_BIN="/usr/sbin", SONIC_GNMI_CHECKPOINT_DIR=str(checkpoint),
                           DBUS_SYSTEM_BUS_ADDRESS="unix:path=" + str(work / "no-dbus.sock"))
        fixture = Path("/probe/redis")
        loader = str(fixture / "usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2")
        libraries = str(fixture / "usr/lib/x86_64-linux-gnu")
        # A bounded executable tmpfs holds only this fixture's redis-cli
        # wrapper; the work directory and service state remain noexec.
        private_bin = Path("/run/gnmi-probe-bin")
        private_bin.mkdir(exist_ok=True)
        redis_cli = private_bin / "redis-cli"
        redis_cli.write_text("#!/bin/sh\nexec " + loader + " --library-path " + libraries +
                             " " + str(fixture / "usr/bin/redis-cli") + " -s " + unix_socket + ' "$@"\n')
        redis_cli.chmod(0o755)
        environment["PATH"] = str(private_bin) + ":" + environment.get("PATH", "")
        libc = ctypes.CDLL(None, use_errno=True)
        shared_memory = -1
        for _ in range(20):
            key = secrets.randbelow(2**30 - 1) + 1
            shared_memory = libc.shmget(key, 1024, 0o1000 | 0o2000 | 0o600)
            if shared_memory >= 0:
                environment["SONIC_GNMI_SHM_KEY"] = str(key)
                break
        require(shared_memory >= 0, "could not reserve private telemetry shared memory")
        service = None
        clients = []
        try:
            debug = installed_debug(names, work) if variant.endswith("-dbg") else {}
            with (work / "redis.log").open("w+") as log:
                service = subprocess.Popen([loader, "--library-path", libraries,
                    str(fixture / "usr/bin/redis-server"), "--bind", "127.0.0.1", "--port", str(port),
                    "--unixsocket", unix_socket, "--save", "", "--appendonly", "no", "--databases", "32",
                    "--dir", str(work)], env=environment, stdout=log, stderr=subprocess.STDOUT)
                unix = redis.Redis(unix_socket_path=unix_socket, db=4, socket_timeout=1)
                tcp = redis.Redis(host="127.0.0.1", port=port, socket_timeout=1)
                state = redis.Redis(unix_socket_path=unix_socket, db=6, socket_timeout=1)
                clients = [unix, tcp, state]
                deadline = time.monotonic() + 20
                while service.poll() is None and time.monotonic() < deadline:
                    try:
                        if unix.info("server")["process_id"] == tcp.info("server")["process_id"] == service.pid:
                            break
                    except redis.RedisError:
                        pass
                    time.sleep(0.1)
                else:
                    log.seek(0)
                    raise AssertionError("private Redis did not start:\n" + log.read())
                unix.hset("DEVICE_METADATA|localhost", mapping={"hostname": "gnmi-ci", "type": "LeafRouter"})
                # This is the same positive/negative source-owner contract, now
                # running the actual image payload without an SDK/runfiles tree.
                probe.run(work, environment, environment)
                startup = supervisor_startup(work, environment, unix, state, dialout)
                print(json.dumps({"status": "passed", "variant": variant,
                    "source_elf_loader_checks": len(image["native_paths"]),
                    "dialout_enabled": dialout, "image_version_preserved": True,
                    "debug_checks": debug, "startup": startup,
                    "checks": ["installed owner binaries", "original supervisor and launcher startup",
                               "cfggen telemetry settings", "private Redis and shared memory",
                               "both write flags", "two real Get values", "invalid database rejected"],
                    "limits": "Private loopback fixtures; no host namespaces, PAM/TACACS, hardware or VS boot"}))
        finally:
            if service is not None:
                if service.poll() is None:
                    service.terminate()
                try:
                    service.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    service.kill()
                    service.wait(timeout=5)
            for client in clients:
                client.close()
            libc.shmctl(shared_memory, 0, None)


if __name__ == "__main__":
    main()
