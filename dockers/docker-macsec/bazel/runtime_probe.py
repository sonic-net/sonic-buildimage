#!/usr/bin/env python3
"""Exercise installed MACsec manager/WPA handoff without emulating an ASIC."""

import argparse
import configparser
import grp
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import tempfile
import time

import redis
import runtime_smoke
from swsscommon import swsscommon


def require(value, message):
    if not value:
        raise AssertionError(message)


def command(*argv):
    result = subprocess.run(argv, capture_output=True, text=True, timeout=20)
    require(result.returncode == 0, repr(argv) + "\n" + result.stdout + result.stderr)
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--validation", type=Path, required=True)
    parser.add_argument("--debug", action="store_true")
    args = parser.parse_args()
    report = json.loads(args.validation.read_text())
    require(report["status"] == "passed" and report["architecture"] == "amd64", "unvalidated image")
    require(grp.getgrnam("netdev").gr_mem == [], "WPA netdev group has unexpected members")
    native = report["native"] if args.debug else report["runtime_native"]
    for name, item in native.items():
        path = Path("/") / name
        stat = path.stat()
        require(hashlib.sha256(path.read_bytes()).hexdigest() == item["sha256"], "source ELF differs: " + name)
        require((stat.st_uid, stat.st_gid, stat.st_mode & 0o7777) ==
                (item["uid"], item["gid"], item["mode"]), "source ELF mode differs: " + name)
    # SONiC package 2.9 selects source whose src/common/version.h reports 2.10-devel.
    inputs = {"architecture": "amd64",
              "elfs": [item for item in report["elfs"] if item["path"] in native],
              "debug_pairs": [item for item in report["source_debug_pairs"] if item["path"] in {
                  "usr/sbin/wpa_supplicant", "usr/sbin/wpa_cli", "usr/bin/wpa_passphrase", "usr/bin/macsecmgrd"}],
              "policy": {"python_imports": ["swsscommon.swsscommon"],
                         "commands": [{"argv": ["/usr/sbin/wpa_supplicant", "-v"], "output_pattern": r"^wpa_supplicant v2\.10-devel\n"}]}}
    if args.debug:
        inputs["policy"]["commands"].append({"argv": ["/usr/bin/vim", "--version"], "output_pattern": "Huge version"})
    result = runtime_smoke.probe(inputs, "debug" if args.debug else "runtime")
    require(result["passed"], json.dumps(result))
    supervisor = configparser.ConfigParser(interpolation=None)
    supervisor.read("/etc/supervisor/conf.d/supervisord.conf")
    require(supervisor["program:macsecmgrd"]["command"] == "/usr/bin/macsecmgrd", "manager command differs")
    require(supervisor["program:macsecmgrd"]["dependent_startup_wait_for"] == "rsyslogd:running", "startup ordering differs")
    database = Path("/run/redis/sonic-db/database_config.json")
    database.parent.mkdir(parents=True, exist_ok=True)
    database.write_bytes(Path("/probe/redis/database_config.json").read_bytes())
    settings = json.loads(database.read_text())["INSTANCES"]["redis"]
    socket_path = settings["unix_socket_path"]
    require(Path(socket_path).resolve() == Path("/run/redis/redis.sock") and settings["hostname"] == "127.0.0.1", "nonprivate Redis configuration")
    executable = Path("/probe/redis/usr/bin/redis-server")
    fixture = {"executable": str(executable), "sha256": hashlib.sha256(executable.read_bytes()).hexdigest(),
               "database_config": str(database), "loopback_tcp": True}
    command("ip", "link", "set", "lo", "up")
    with runtime_smoke.redis_fixture(fixture), tempfile.TemporaryDirectory(prefix="macsec-probe-") as temporary:
        work = Path(temporary)
        clients = {number: redis.Redis(unix_socket_path=socket_path, db=number, decode_responses=True,
                                      socket_timeout=1) for number in (0, 4, 6)}
        app, cfg, state = (clients[number] for number in (0, 4, 6))
        app.config_set("notify-keyspace-events", "KEA")
        manager = None
        log = (work / "macsecmgrd.log").open("w+")
        port = "Ethernet0"
        control = Path("/run") / port

        def diagnostics():
            log.flush()
            return (work / "macsecmgrd.log").read_text()[-6000:]

        def wait_for(predicate, message, timeout=30):
            deadline = time.monotonic() + timeout
            while time.monotonic() < deadline:
                require(manager.poll() is None, message + "\n" + diagnostics())
                value = predicate()
                if value:
                    return value
                time.sleep(.1)
            raise AssertionError(message + "\n" + diagnostics())

        def assert_gate_closed(message):
            # Observe across two iterations of the shipped one-second gate.
            deadline = time.monotonic() + 2.2
            while time.monotonic() < deadline:
                require(manager.poll() is None and not control.exists(), message + "\n" + diagnostics())
                time.sleep(.1)

        try:
            command("ip", "link", "add", port, "type", "veth", "peer", "name", "Ethernet4")
            for name, address in ((port, "02:00:00:00:00:42"), ("Ethernet4", "02:00:00:00:00:43")):
                command("ip", "link", "set", name, "address", address)
                command("ip", "link", "set", name, "up")
            cfg.hset("DEVICE_METADATA|localhost", mapping={"hostname": "bazel-macsec", "mac": "02:00:00:00:00:42"})
            cfg.set("CONFIG_DB_INITIALIZED", "1")
            # Deterministic test-only CAK in the existing Type-7 input format.
            key = "0123456789abcdef0123456789abcdef"
            salt = "dsfd;kfoA,.iyewrkldJKDHSUBsgvca69834ncxv9873254k;fg87"
            encoded = "00" + "".join(f"{ord(value) ^ ord(salt[index % len(salt)]):02x}" for index, value in enumerate(key))
            cfg.hset("MACSEC_PROFILE|bazel", mapping={"cipher_suite": "GCM-AES-128", "primary_cak": encoded,
                     "primary_ckn": "0123456789abcdef0123456789abcdef", "policy": "security", "send_sci": "true"})
            cfg.hset("PORT|" + port, mapping={"macsec": "bazel"})
            # Consume only the real driver's request; never manufacture an
            # orchagent/SAI completion in STATE_DB.
            connection = swsscommon.DBConnector(0, socket_path, 0)
            requests = swsscommon.ConsumerStateTable(connection, "MACSEC_PORT_TABLE")
            manager = subprocess.Popen(["/usr/bin/macsecmgrd"], stdout=log, stderr=subprocess.STDOUT,
                                       start_new_session=True)
            assert_gate_closed("manager ignored missing POST readiness")
            state.hset("FIPS_MACSEC_POST_TABLE|sai", mapping={"post_state": "disabled"})
            assert_gate_closed("manager ignored missing port readiness")
            # The fixture owns this real veth; publish its observed link state
            # as the manager input, without asserting a PortMgr/ASIC test.
            link = json.loads(command("ip", "-json", "link", "show", "dev", port))[0]
            require("UP" in link["flags"], "private test interface is down")
            state.hset("PORT_TABLE|" + port, mapping={"state": "ok", "netdev_oper_status": "up"})
            wait_for(control.is_socket, "manager did not start the actual WPA global socket")
            request = {}

            def received():
                name, operation, fields = requests.pop()
                if not name:
                    return False
                require(name == port and operation == "SET", "unexpected MACsec request identity")
                request.update(dict(fields))
                return True

            wait_for(received, "SONiC WPA driver did not submit a real MACsec port request")
            require(request.get("enable") == "false" and request.get("cipher_suite") == "GCM-AES-128"
                    and request.get("send_sci") == "true", "MACsec request lost selected profile fields")
            require(not state.exists("MACSEC_PORT_TABLE|" + port), "fixture unexpectedly fabricated an ASIC response")
            children = []
            for item in Path("/proc").iterdir():
                if not item.name.isdecimal():
                    continue
                try:
                    argv = (item / "cmdline").read_bytes().split(b"\0")
                    status = (item / "status").read_text()
                except (FileNotFoundError, PermissionError, ProcessLookupError):
                    continue
                if argv and argv[0] in (b"/sbin/wpa_supplicant", b"/usr/sbin/wpa_supplicant"):
                    parent = next(int(line.split()[1]) for line in status.splitlines() if line.startswith("PPid:"))
                    if parent == manager.pid:
                        children.append(int(item.name))
            require(len(children) == 1, "request was not emitted by the manager's actual WPA child")
            print(json.dumps({"status": "passed", "source_elf_count": len(native),
                              "loader_checks": len(result["loader_checks"]), "debug_checks": len(result["debug_pairs"]),
                              "post_readiness_gate": True, "port_readiness_gate": True,
                              "manager_wpa_child": True, "sonic_driver_request": request,
                              "scope": "Installed manager/WPA/native-library handoff against private Redis and veth; stops before an orchagent/SAI response. No encrypted-traffic, ASIC, FIPS POST or installer claim."}))
        finally:
            if manager is not None:
                try:
                    os.killpg(manager.pid, signal.SIGTERM)
                    manager.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    os.killpg(manager.pid, signal.SIGKILL)
                    manager.wait(timeout=5)
                except ProcessLookupError:
                    pass
                finally:
                    # The driver intentionally waits for an absent ASIC reply;
                    # terminate any still-running child in this private group.
                    try:
                        os.killpg(manager.pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass
            log.close()
            for client in clients.values():
                client.close()


if __name__ == "__main__":
    main()
