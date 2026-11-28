#!/usr/bin/env python3
"""Exercise installed LLDP and DBSyncd against private interfaces and Redis."""

import configparser
import grp
import hashlib
import importlib.machinery
import importlib.util
import json
import os
from pathlib import Path
import pwd
import signal
import subprocess
import sys
import tempfile
import time

import redis
import runtime_smoke


def require(value, message):
    if not value:
        raise AssertionError(message)


def command(argv, timeout=30):
    try:
        result = subprocess.run(argv, capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired as error:
        output = (error.stdout or b"") + (error.stderr or b"")
        raise AssertionError(repr(argv) + " timed out:\n" + output.decode("utf-8", errors="replace")) from error
    require(result.returncode == 0, repr(argv) + "\n" + result.stdout + result.stderr)
    return result.stdout


def main():
    report = json.loads(Path(sys.argv[1]).read_text())
    variant = sys.argv[2]
    require(report["status"] == "passed" and report["architecture"] == "amd64", "unvalidated image")
    require(os.environ.get("IMAGE_VERSION") == report["image_version"], "image version differs")
    account = pwd.getpwnam("_lldpd")
    require((account.pw_uid, account.pw_gid, account.pw_dir, account.pw_shell) ==
            (101, 102, "/run/lldpd", "/usr/sbin/nologin"), "LLDP service account differs")
    require(grp.getgrnam("adm").gr_gid == 4, "LLDP control group differs")
    for name in report["images"][variant]["native_paths"]:
        path = Path("/") / name
        actual = path.stat()
        item = report["native"][name]
        require(hashlib.sha256(path.read_bytes()).hexdigest() == item["sha256"], "installed source ELF differs: " + name)
        require((actual.st_uid, actual.st_gid, actual.st_mode & 0o7777) ==
                (item["uid"], item["gid"], item["mode"]), "installed source ELF permissions differ: " + name)
        command(["/lib64/ld-linux-x86-64.so.2", "--list", str(path)])
    require("1.0.16" in command(["/usr/sbin/lldpd", "-v"]), "LLDP version differs")
    debugger = []
    if variant.endswith("-dbg"):
        require(grp.getgrnam("_ssh").gr_gid == 104, "FIPS OpenSSH reserved group differs")
        selected = [item for item in report["source_debug_pairs"]
                    if item["path"] in {"usr/sbin/lldpd", "usr/sbin/lldpcli", "usr/lib/x86_64-linux-gnu/liblldpctl.so.4.9.1"}]
        require(len(selected) == 3, "missing LLDP debugger pairs")
        debugger = [runtime_smoke.debug_pair(item) for item in selected]
        for item in debugger:
            item["with_symbols"]["source"] = Path(item["with_symbols"]["source"]).name

    database = Path("/run/redis/sonic-db/database_config.json")
    database.parent.mkdir(parents=True, exist_ok=True)
    database.write_bytes(Path("/probe/redis/database_config.json").read_bytes())
    settings = json.loads(database.read_text())["INSTANCES"]["redis"]
    socket_path = settings["unix_socket_path"]
    require(Path(socket_path).resolve() == Path("/run/redis/redis.sock") and
            settings["hostname"] == "127.0.0.1", "Redis fixture is not private")
    executable = Path("/probe/redis/usr/bin/redis-server")
    fixture = {"executable": str(executable), "sha256": hashlib.sha256(executable.read_bytes()).hexdigest(),
               "database_config": str(database), "loopback_tcp": True}
    command(["ip", "link", "set", "lo", "up"])
    with runtime_smoke.redis_fixture(fixture), tempfile.TemporaryDirectory(prefix="lldp-") as temporary:
        work = Path(temporary)
        # The separate peer daemon drops to _lldpd; its installed setuid client
        # must traverse the fixture directory to read config and open its socket.
        work.chmod(0o755)
        clients = {number: redis.Redis(unix_socket_path=socket_path, db=number,
                    decode_responses=True, socket_timeout=1) for number in (0, 4, 6)}
        app, cfg, state = (clients[number] for number in (0, 4, 6))
        active, all_processes, streams = [], [], []

        def start(name, argv):
            stream = (work / (name + ".log")).open("w+")
            streams.append(stream)
            process = subprocess.Popen(argv, stdout=stream, stderr=subprocess.STDOUT,
                                       start_new_session=True)
            active.append(process)
            all_processes.append(process)
            return process

        def diagnostics():
            result = []
            for path in work.glob("*.log"):
                text = path.read_text()
                result.append(path.name + ":\n" +
                              (text if len(text) <= 9000 else text[:4000] + "\n...\n" + text[-5000:]))
            return "\n".join(result)

        def wait_for(callback, message, timeout=45):
            deadline = time.monotonic() + timeout
            while time.monotonic() < deadline:
                require(all(process.poll() is None for process in active), message + "\n" + diagnostics())
                value = callback()
                if value:
                    return value
                time.sleep(.1)
            raise AssertionError(message + "\n" + diagnostics())

        def cli(*arguments, peer=False):
            argv = ["/usr/sbin/lldpcli"]
            if peer:
                argv += ["-u", str(work / "peer.sock")]
            return command(argv + list(arguments))

        try:
            command(["ip", "link", "add", "Ethernet0", "type", "veth", "peer", "name", "Ethernet4"])
            for name, address in (("Ethernet0", "02:00:00:00:00:42"), ("Ethernet4", "02:00:00:00:00:43")):
                command(["ip", "link", "set", name, "address", address])
                command(["ip", "link", "set", name, "up"])
            cfg.hset("DEVICE_METADATA|localhost", mapping={"hostname": "bazel-lldp", "hwsku": "Force10-S6000"})
            cfg.set("CONFIG_DB_INITIALIZED", "1")
            cfg.hset("FEATURE|lldp", mapping={"set_owner": "local"})
            state.hset("FEATURE|lldp", mapping={"system_state": "up"})
            Path("/etc/sonic/sonic_version.yml").write_text("build_version: bazel-lldp\ndebian_version: 13\nkernel_version: probe\n")
            rendered = command(["sonic-cfggen", "-d", "-a", '{"namespace_id":""}', "-t",
                                "/usr/share/sonic/templates/supervisord.conf.j2"])
            supervisor = configparser.ConfigParser(interpolation=None)
            supervisor.read_string(rendered)
            require(supervisor["program:lldp-syncd"]["command"] == "/usr/bin/env python3 -m lldp_syncd" and
                    supervisor["program:lldpmgrd"]["command"] == "/usr/bin/lldpmgrd", "installed supervisor contract differs")
            # Match the private fixture's local owner. The unchanged startup
            # guard deliberately blocks a kube launch when CONFIG_DB says local.
            startup = subprocess.run(["/usr/bin/env", "RUNTIME_OWNER=local", "/usr/bin/start.sh"],
                                     capture_output=True, text=True, timeout=30)
            require(startup.returncode == 0, "startup failed:\n" + startup.stdout + startup.stderr)
            require(state.hget("FEATURE|lldp", "current_owner") == "local",
                    "startup did not accept and record the local container owner")
            require("bazel-lldp" in Path("/etc/lldpd.conf").read_text(),
                    "startup did not render LLDP configuration:\n" + startup.stdout + startup.stderr)
            require(Path("/var/sonic/config_status").is_file(), "startup state is absent")
            peer_config = work / "peer.conf"
            peer_config.write_text("configure system hostname peer-before\nconfigure lldp tx-interval 1\nconfigure lldp tx-hold 2\n")
            start("lldpd", ["/usr/sbin/lldpd", "-dddd", "-I", "Ethernet0", "-C", "Ethernet0",
                             "-p", str(work / "main.pid"), "-O", "/etc/lldpd.conf"])
            start("peer", ["/usr/sbin/lldpd", "-dddd", "-I", "Ethernet4", "-C", "Ethernet4",
                            "-p", str(work / "peer.pid"), "-u", str(work / "peer.sock"), "-O", str(peer_config)])
            wait_for(lambda: Path("/run/lldpd.socket").is_socket() and (work / "peer.sock").is_socket(),
                     "LLDP control sockets did not appear")
            command(["/usr/bin/waitfor_lldp_ready.sh"], timeout=30)
            cli("configure", "lldp", "tx-interval", "1")
            cli("resume")
            cli("resume", peer=True)
            start("dbsyncd", ["/usr/bin/python3", "-m", "lldp_syncd", "-d", "20", "-f", "1"])
            key = "LLDP_ENTRY_TABLE:Ethernet0"
            wait_for(lambda: app.hget(key, "lldp_rem_sys_name") == "peer-before", "real LLDP neighbor did not reach APPL_DB")
            before = app.hgetall(key)
            cli("configure", "system", "hostname", "peer-after", peer=True)
            wait_for(lambda: app.hget(key, "lldp_rem_sys_name") == "peer-after", "updated LLDP neighbor did not reach APPL_DB")
            # Load the shipped manager executable and invoke its native control
            # path with real Common DB connectors and the installed lldpcli.
            loader = importlib.machinery.SourceFileLoader("installed_lldpmgrd", "/usr/bin/lldpmgrd")
            spec = importlib.util.spec_from_loader(loader.name, loader)
            manager_module = importlib.util.module_from_spec(spec)
            loader.exec_module(manager_module)
            manager = manager_module.LldpManager("bazel-lldp-probe")
            manager.update_hostname("managed-lldp")
            require(manager.hostname == "managed-lldp" and "managed-lldp" in cli("-f", "json", "show", "chassis"),
                    "installed manager could not configure the real LLDP daemon")
            command(["ip", "link", "set", "Ethernet4", "down"])
            wait_for(lambda: not app.exists(key), "removed LLDP neighbor remained in APPL_DB")
            require(app.hlen("LLDP_LOC_CHASSIS") > 0, "local chassis data was not retained")
            print(json.dumps({"status": "passed", "variant": variant, "source_native_files": len(report["images"][variant]["native_paths"]),
                              "private_veth_neighbor": True, "private_redis": True,
                              "neighbor_add_update_delete": True, "original_startup_templates": True,
                              "installed_manager_control": True, "initial_neighbor_fields": sorted(before),
                              "debugger": debugger,
                              "scope": "Two installed LLDP daemons over a private veth; actual DBSyncd CLI and manager control method; no external interfaces."}))
        except Exception as error:
            raise AssertionError(str(error) + "\n" + diagnostics()) from error
        finally:
            for process in reversed(all_processes):
                if process.poll() is None:
                    os.killpg(process.pid, signal.SIGTERM)
                    try:
                        process.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        os.killpg(process.pid, signal.SIGKILL)
                        process.wait()
            for stream in streams:
                stream.close()
            for client in clients.values():
                client.close()
            subprocess.run(["ip", "link", "del", "Ethernet0"], capture_output=True, check=False)


if __name__ == "__main__":
    main()
