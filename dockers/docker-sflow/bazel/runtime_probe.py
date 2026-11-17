#!/usr/bin/env python3
"""Check installed sFlow services against private Redis, veth and UDP fixtures."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import signal
import socket
import subprocess
import sys
import tempfile
import time

import redis
import runtime_smoke


def require(value, message):
    if not value:
        raise AssertionError(message)


def command(*argv):
    result = subprocess.run(argv, capture_output=True, text=True, timeout=40)
    require(result.returncode == 0, repr(argv) + "\n" + result.stdout + result.stderr)
    return result.stdout


def wait_for(predicate, message, timeout=45):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        value = predicate()
        if value:
            return value
        time.sleep(.15)
    raise AssertionError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--validation", type=Path, required=True)
    parser.add_argument("--debug", action="store_true")
    args = parser.parse_args()
    report = json.loads(args.validation.read_text())
    require(report["status"] == "passed" and report["architecture"] == "amd64", "unvalidated image")
    native = report["native"] if args.debug else report["runtime_native"]
    for name, item in native.items():
        path = Path("/") / name
        info = path.stat()
        require(hashlib.sha256(path.read_bytes()).hexdigest() == item["sha256"], "source ELF differs: " + name)
        require((info.st_uid, info.st_gid, info.st_mode & 0o7777) ==
                (item["uid"], item["gid"], item["mode"]), "source ELF mode differs: " + name)
    service_paths = {"usr/sbin/hsflowd", "usr/bin/sflowtool", "usr/bin/psample", "usr/bin/sflowmgrd",
                     "usr/lib/x86_64-linux-gnu/libpsample.so.1.0"} | {
        name for name in native if name.startswith("etc/hsflowd/modules/")}
    inputs = {"architecture": "amd64", "elfs": [item for item in report["elfs"] if item["path"] in native],
              "debug_pairs": [item for item in report["source_debug_pairs"] if item["path"] in service_paths],
              "policy": {"python_imports": ["swsscommon.swsscommon", "sonic_py_common.port_util"],
                         "commands": [{"argv": ["/usr/sbin/dmidecode", "--version"], "output_pattern": r"\d+\.\d+"}]}}
    if args.debug:
        inputs["policy"]["commands"].append({"argv": ["/usr/bin/vim", "--version"], "output_pattern": "Huge version"})
    checked = runtime_smoke.probe(inputs, "debug" if args.debug else "runtime")
    require(checked["passed"], json.dumps(checked))

    # The selected SONiC database configuration is copied from the source owner;
    # all listeners remain inside Docker's isolated network namespace.
    database = Path("/run/redis/sonic-db/database_config.json")
    database.parent.mkdir(parents=True, exist_ok=True)
    database.write_bytes(Path("/probe/redis/database_config.json").read_bytes())
    fixture_database = json.loads(database.read_text())
    settings = fixture_database["INSTANCES"]["redis"]
    # mod_sonic connects every declared instance, including chassis Redis.
    # Alias those descriptors only in this disposable isolated fixture; retain
    # every database name, ID and separator from the verified source owner.
    for instance in fixture_database["INSTANCES"].values():
        instance.update(settings)
    database.write_text(json.dumps(fixture_database, indent=2) + "\n")
    require(settings["hostname"] == "127.0.0.1", "Redis fixture is not loopback")
    server = Path("/probe/redis/usr/bin/redis-server")
    fixture = {"executable": str(server), "sha256": hashlib.sha256(server.read_bytes()).hexdigest(),
               "database_config": str(database), "loopback_tcp": True}
    command("ip", "link", "set", "lo", "up")
    command("ip", "link", "add", "Ethernet0", "type", "veth", "peer", "name", "SflowPeer")
    command("ip", "addr", "add", "192.0.2.1/24", "dev", "Ethernet0")
    for interface in ("Ethernet0", "SflowPeer"):
        command("ip", "link", "set", interface, "up")
    version = Path("/etc/sonic/sonic_version.yml")
    version.parent.mkdir(parents=True, exist_ok=True)
    version.write_text("asic_type: vs\nbuild_version: bazel-sflow-probe\n")
    installed_config = Path("/etc/hsflowd.conf")
    original_config = installed_config.read_bytes()
    command("/usr/bin/enable_mod_vpp.sh")
    require(installed_config.read_bytes() == original_config, "VS startup unexpectedly enables VPP")
    # Test the real script's other configuration branch without starting VPP.
    version.write_text("asic_type: vpp\n")
    command("/usr/bin/enable_mod_vpp.sh")
    first_vpp = installed_config.read_bytes()
    command("/usr/bin/enable_mod_vpp.sh")
    require(installed_config.read_bytes() == first_vpp and first_vpp.count(b"vpp { osIndex=on }") == 1,
            "VPP configuration is not idempotent")
    installed_config.write_bytes(original_config)
    version.write_text("asic_type: vs\nbuild_version: bazel-sflow-probe\n")

    with runtime_smoke.redis_fixture(fixture), tempfile.TemporaryDirectory(prefix="sflow-probe-") as temporary:
        root = Path(temporary)
        cfg = redis.Redis(unix_socket_path=settings["unix_socket_path"], db=4, decode_responses=True)
        state = redis.Redis(unix_socket_path=settings["unix_socket_path"], db=6, decode_responses=True)
        cfg.config_set("notify-keyspace-events", "KEA")
        cfg.set("CONFIG_DB_INITIALIZED", "1")
        cfg.hset("DEVICE_METADATA|localhost", mapping={"hostname": "sflow-probe", "mac": "02:00:00:00:00:01", "bgp_asn": "65000"})
        cfg.hset("PORT|Ethernet0", mapping={"admin_status": "up", "speed": "10000"})
        cfg.hset("SFLOW|global", mapping={"admin_state": "down", "polling_interval": "5", "agent_id": "Ethernet0"})
        state.hset("SYSTEM_READY|SYSTEM_STATE", mapping={"Status": "UP"})
        for path in ("/var/log/supervisor", "/var/spool/rsyslog"):
            Path(path).mkdir(parents=True, exist_ok=True)
        log_path = root / "startup.log"
        supervisor = decoder = None
        fifo_fd = None
        log = log_path.open("w")
        decoded = root / "decoded.jsonl"
        decoder_log = decoded.open("w")
        try:
            supervisor = subprocess.Popen(["/usr/local/bin/supervisord"], stdout=log, stderr=subprocess.STDOUT,
                                          start_new_session=True)

            def supervised(name):
                require(supervisor.poll() is None, "supervisor exited: " + log_path.read_text()[-4000:])
                result = subprocess.run(["/usr/local/bin/supervisorctl", "pid", name], capture_output=True, text=True, timeout=5)
                return int(result.stdout.strip()) if result.returncode == 0 and result.stdout.strip().isdigit() else 0

            manager = wait_for(lambda: supervised("sflowmgrd"), "original startup did not start sflowmgrd")
            mapper = wait_for(lambda: supervised("port_index_mapper"), "original startup did not start port mapper")
            require(os.path.samefile("/proc/" + str(manager) + "/exe", "/usr/bin/sflowmgrd"), "wrong manager binary")
            require("RUNNING" in command("/usr/local/bin/supervisorctl", "status", "rsyslogd"), "rsyslog startup dependency failed")
            state.hset("PORT_TABLE|Ethernet0", mapping={"state": "ok", "netdev_oper_status": "up", "speed": "10000"})
            mapping = wait_for(lambda: state.hgetall("PORT_INDEX_TABLE|Ethernet0"), "installed mapper did not publish port indexes")
            require(int(mapping["ifindex"]) == socket.if_nametoindex("Ethernet0"), "mapper did not use the actual interface")
            require(int(mapping["index"]) > 0, "mapper produced an invalid SONiC port index")

            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as reservation:
                reservation.bind(("127.0.0.1", 0))
                port = reservation.getsockname()[1]
            cfg.hset("SFLOW_COLLECTOR|fixture", mapping={"collector_ip": "127.0.0.1", "collector_port": str(port), "collector_vrf": "default"})
            cfg.hset("SFLOW|global", "admin_state", "up")
            pidfile = Path("/run/hsflowd.pid")
            automatic = Path("/etc/hsflowd.auto")

            def running_daemon():
                if not pidfile.exists():
                    return 0
                pid = int(pidfile.read_text().strip())
                return pid if Path("/proc/" + str(pid)).exists() else 0

            daemon = wait_for(running_daemon, "sflowmgrd did not start hsflowd through its installed init script")
            require(os.path.samefile("/proc/" + str(daemon) + "/exe", "/usr/sbin/hsflowd"), "wrong hsflowd binary")
            argv = Path("/proc/" + str(daemon) + "/cmdline").read_bytes().split(b"\0")
            require(b"-c" in argv and b"/var/log/hsflowd.crash" in argv, "init script lost its crash-file option")
            wait_for(lambda: automatic.exists() and "polling=5" in automatic.read_text(), "installed SONiC module did not read Redis configuration")
            cfg.hset("SFLOW|global", "polling_interval", "7")
            wait_for(lambda: "polling=7" in automatic.read_text(), "installed SONiC module did not follow Redis updates")
            cfg.hset("SFLOW|global", "admin_state", "down")
            wait_for(lambda: not running_daemon(), "sflowmgrd did not stop hsflowd")

            # A declared test-only JSON input exercises installed mod_json and
            # UDP export. The first startup above uses the original config.
            fifo = root / "metrics.fifo"
            os.mkfifo(fifo, 0o666)
            fifo.chmod(0o666)
            root.chmod(0o755)
            fifo_fd = os.open(fifo, os.O_RDWR | os.O_NONBLOCK)
            text = original_config.decode().replace("sflow {", "sflow {\n  json { fifo=" + str(fifo) + " }", 1)
            installed_config.write_text(text)
            decoder = subprocess.Popen(["stdbuf", "-oL", "/usr/bin/sflowtool", "-j", "-p", str(port)],
                                       stdout=decoder_log, stderr=subprocess.STDOUT)
            cfg.hset("SFLOW|global", "admin_state", "up")
            wait_for(running_daemon, "manager did not restart the installed daemon")

            def metric(value):
                message = json.dumps({"rtmetric": {"datasource": "bazel_fixture", "probe": {"type": "counter32", "value": value}}}).encode()
                def delivered():
                    require(decoder.poll() is None and running_daemon(), "installed exporter or decoder exited")
                    os.write(fifo_fd, message)
                    for line in decoded.read_text().splitlines():
                        if not line.startswith("{"):
                            continue
                        record = json.loads(line)
                        for sample in record.get("samples", []):
                            if sample.get("rtmetric_datasource_name") == "bazel_fixture" and int(sample.get("rtmetric_counter32_probe", -1)) == value:
                                require(int(record["datagramVersion"]) == 5, "wrong sFlow wire protocol")
                                return True
                    return False
                wait_for(delivered, "installed hsflowd/sflowtool did not export/decode metric " + str(value), 35)
            metric(42)
            metric(84)
            command("ip", "link", "del", "Ethernet0")
            state.delete("PORT_TABLE|Ethernet0")
            wait_for(lambda: not state.exists("PORT_INDEX_TABLE|Ethernet0"), "mapper did not remove the deleted interface")
            require(supervised("sflowmgrd") == manager and supervised("port_index_mapper") == mapper,
                    "installed services restarted during validation")
            print(json.dumps({"status": "passed", "source_elf_count": len(native),
                              "loader_checks": len(checked["loader_checks"]), "debug_checks": len(checked["debug_pairs"]),
                              "original_supervisor": True, "manager_init_start_stop": True,
                              "port_mapper_create_delete": True, "vs_vpp_config_branches": True,
                              "polling_changes": [5, 7], "decoded_metrics": [42, 84],
                              "scope": "Installed startup/control, private Redis/veth and real JSON-to-sFlow UDP export/decode; no kernel packet sampling, VPP dataplane, ASIC or installer claim."}))
        except Exception:
            for path in (log_path, decoded, Path("/var/log/syslog"), Path("/etc/hsflowd.auto"), Path("/var/log/hsflowd.crash"), Path("/var/log/supervisor/hsflowd.log")):
                if path.is_file():
                    print(str(path) + ":\n" + path.read_text(errors="replace")[-8000:], file=sys.stderr)
            raise
        finally:
            subprocess.run(["service", "hsflowd", "stop"], capture_output=True, timeout=40, check=False)
            if fifo_fd is not None:
                os.close(fifo_fd)
            if decoder is not None and decoder.poll() is None:
                decoder.terminate()
                decoder.wait(timeout=5)
            if supervisor is not None and supervisor.poll() is None:
                os.killpg(supervisor.pid, signal.SIGTERM)
                try:
                    supervisor.wait(timeout=8)
                except subprocess.TimeoutExpired:
                    os.killpg(supervisor.pid, signal.SIGKILL)
                    supervisor.wait()
            installed_config.write_bytes(original_config)
            log.close()
            decoder_log.close()
            cfg.close()
            state.close()


if __name__ == "__main__":
    main()
