#!/usr/bin/env python3
"""Check installed HA startup, software bus routing and a real Redis state transition."""
import argparse
import configparser
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import time
from xmlrpc.client import ServerProxy

import redis
import runtime_smoke
from supervisor.xmlrpc import SupervisorTransport


def require(value, message):
    if not value:
        raise AssertionError(message)


def command(*argv, timeout=25):
    result = subprocess.run(argv, capture_output=True, text=True, timeout=timeout)
    require(result.returncode == 0, repr(argv) + "\n" + result.stdout + result.stderr)
    return result.stdout


def stop(process):
    if process is None:
        return
    try:
        os.killpg(process.pid, signal.SIGTERM)
        process.wait(timeout=10)
    except subprocess.TimeoutExpired:
        os.killpg(process.pid, signal.SIGKILL)
        process.wait(timeout=5)
    except ProcessLookupError:
        pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--validation", type=Path, required=True)
    parser.add_argument("--debug", action="store_true")
    args = parser.parse_args()
    for name in ("LD_PRELOAD", "LD_LIBRARY_PATH", "LD_AUDIT", "PYTHONPATH"):
        os.environ.pop(name, None)
    os.environ["DEV"] = "dpu0"
    report = json.loads(args.validation.read_text())
    require(report["status"] == "passed" and report["architecture"] == "amd64", "unvalidated image")
    native = report["native"] if args.debug else report["runtime_native"]
    for name, item in native.items():
        path = Path("/") / name
        info = path.stat()
        require(hashlib.sha256(path.read_bytes()).hexdigest() == item["sha256"], "installed source bytes differ: " + name)
        require((info.st_uid, info.st_gid, info.st_mode & 0o7777) == (item["uid"], item["gid"], item["mode"]),
                "installed source metadata differs: " + name)
    programs = {"usr/bin/hamgrd", "usr/bin/swbusd", "usr/bin/swbus-cli"}
    inputs = {"architecture": "amd64", "elfs": [item for item in report["elfs"] if item["path"] in native],
              "debug_pairs": [item for item in report["source_debug_pairs"] if item["path"] in programs],
              "policy": {"python_imports": ["swsscommon.swsscommon"],
                         "commands": [{"argv": ["/" + name, "--help"], "output_pattern": "Usage:"} for name in sorted(programs)]}}
    if args.debug:
        inputs["policy"]["commands"] += [
            {"argv": ["/usr/bin/gdb", "--version"], "output_pattern": "GNU gdb"},
            {"argv": ["/usr/bin/gdbserver", "--version"], "output_pattern": "GNU gdbserver"},
            {"argv": ["/usr/bin/strace", "--version"], "output_pattern": "strace"},
            {"argv": ["/usr/bin/vim", "--version"], "output_pattern": "Huge version"},
            {"argv": ["/usr/bin/sshpass", "-V"], "output_pattern": "sshpass"},
        ]
    probe = runtime_smoke.probe(inputs, "debug" if args.debug else "runtime")
    require(probe["passed"], json.dumps(probe))
    config = configparser.ConfigParser(interpolation=None)
    config.read("/etc/supervisor/conf.d/supervisord.conf")
    for program, prerequisite in (("wait-for-loopback", "rsyslogd:running"), ("swbusd", "wait-for-loopback:exited"), ("hamgrd", "swbusd:running")):
        require(config["program:" + program]["dependent_startup_wait_for"] == prerequisite, "startup order changed: " + program)
    require(config["program:hamgrd"]["command"] == '/bin/bash -c \'SLOT=${DEV: -1}; exec /usr/bin/hamgrd -s "$SLOT"\'', "native slot argument changed")

    database = Path("/run/redis/sonic-db/database_config.json")
    database.parent.mkdir(parents=True, exist_ok=True)
    settings = json.loads(Path("/probe/redis/database_config.json").read_text())
    settings["INSTANCES"]["redis"].update(hostname="127.0.0.1", port=6379, unix_socket_path="/run/redis/redis.sock")
    # Reproduce the owner's private dpu0 namespace from test_utils/hamgrd;
    # only this disposable fixture gains the additional logical databases.
    for name, number, separator in (("CHASSIS_STATE_DB", 13, "|"), ("DPU_STATE_DB", 14, "|"),
                                    ("DPU_APPL_DB", 15, ":"), ("DPU_COUNTERS_DB", 16, ":")):
        settings["DATABASES"][name] = {"id": number, "separator": separator, "instance": "redis"}
    require(settings["DATABASES"]["CONFIG_DB"]["id"] == 4 and settings["DATABASES"]["STATE_DB"]["id"] == 6,
            "inherited database identities changed")
    database.write_text(json.dumps(settings))
    (database.parent / "database_global.json").write_text(json.dumps({"INCLUDES": [
        {"include": database.name}, {"container_name": "dpu0", "include": database.name}]}))
    executable = Path("/probe/redis/usr/bin/redis-server")
    fixture = {"executable": str(executable), "sha256": hashlib.sha256(executable.read_bytes()).hexdigest(),
               "database_config": str(database), "loopback_tcp": True}
    command("ip", "link", "set", "lo", "up")
    command("ip", "link", "add", "Loopback0", "type", "dummy")
    command("ip", "address", "add", "198.18.0.1/32", "dev", "Loopback0")
    command("ip", "link", "set", "Loopback0", "up")
    for path in ("/var/log/supervisor", "/var/log/dash-ha", "/var/spool/rsyslog"):
        Path(path).mkdir(parents=True, exist_ok=True)
    log_path = Path("/tmp/dash-ha-supervisor.log")
    peer_log_path = Path("/tmp/dash-ha-peer.log")
    supervisor = peer = None
    with runtime_smoke.redis_fixture(fixture):
        clients = {number: redis.Redis(unix_socket_path="/run/redis/redis.sock", db=number,
                                      decode_responses=True, socket_timeout=2) for number in (4, 6, 13)}
        cfg, state, chassis = (clients[number] for number in (4, 6, 13))
        cfg.config_set("notify-keyspace-events", "KEA")
        cfg.set("CONFIG_DB_INITIALIZED", "1")
        cfg.hset("DEVICE_METADATA|localhost", mapping={"hostname": "ha-probe", "region": "region-a", "cluster": "cluster-a"})
        cfg.hset("FEATURE|dash-ha", mapping={"state": "enabled", "owner": "local"})
        cfg.hset("LOOPBACK_INTERFACE|Loopback0|198.18.0.1/32", mapping={"NULL": "NULL"})
        cfg.hset("DPU|switch0_dpu0", mapping={"dpu_id": "0", "swbus_port": "8100", "pa_ipv4": "198.18.0.10",
            "midplane_ipv4": "127.0.0.1", "orchagent_zmq_port": "58888", "vdpu_id": "fixture-vdpu"})
        cfg.hset("DASH_HA_GLOBAL_CONFIG|GLOBAL", mapping={"dpu_vlan": "Vlan100", "dpu_vnet": "fixture-vnet"})
        fields = {"dpu_midplane_link_state": "up", "dpu_control_plane_state": "up", "dpu_data_plane_state": "up"}
        chassis.hset("DPU_STATE|dpu0", mapping=fields)
        with log_path.open("w") as log, peer_log_path.open("w") as peer_log:
            try:
                supervisor = subprocess.Popen(["/usr/local/bin/supervisord"], stdout=log, stderr=subprocess.STDOUT, start_new_session=True)

                def wait_for(predicate, message, timeout=60):
                    deadline = time.monotonic() + timeout
                    while time.monotonic() < deadline:
                        require(supervisor.poll() is None, "supervisor exited: " + log_path.read_text()[-8000:])
                        value = predicate()
                        if value:
                            return value
                        time.sleep(.2)
                    raise AssertionError(message + "\n" + log_path.read_text()[-8000:])

                def services_ready():
                    if not Path("/var/run/supervisor.sock").exists():
                        return None
                    transport = SupervisorTransport(None, None, "unix:///var/run/supervisor.sock")
                    with ServerProxy("http://localhost", transport=transport) as rpc:
                        records = {item["name"]: item for item in rpc.supervisor.getAllProcessInfo()}
                    states = {name: item["statename"] for name, item in records.items()}
                    startup = records.get("dependent-startup", {})
                    # This listener exits successfully once the dependency graph
                    # is running. The other listener and daemons must stay up.
                    startup_done = startup.get("statename") == "RUNNING" or (
                        startup.get("statename") == "EXITED" and startup.get("exitstatus") == 0)
                    loopback = records.get("wait-for-loopback", {})
                    return states if startup_done and all(states.get(name) == "RUNNING" for name in
                        ("supervisor-proc-exit-listener", "rsyslogd", "swbusd", "hamgrd")) and (
                        loopback.get("statename") == "EXITED" and loopback.get("exitstatus") == 0) else None

                states = wait_for(services_ready, "original HA startup did not finish")
                pids = {name: int(command("/usr/local/bin/supervisorctl", "pid", name).strip()) for name in ("swbusd", "hamgrd")}
                for name, pid in pids.items():
                    require((Path("/proc") / str(pid) / "exe").resolve() == Path("/usr/bin") / name, "supervisor started another executable")
                actor_command = ("/usr/bin/swbus-cli", "show", "hamgrd", "actor", "/hamgrd/0/dpu/switch0_dpu0")

                def observed_up():
                    output = command(*actor_command)
                    return output if "Incoming State" in output and re.search(r'dpu_control_plane_state.*up', output) else None

                initial = wait_for(observed_up, "installed actor did not observe the initial PMON state")
                require(not state.exists("DPU_RESET_INFO|DPU0"), "fixture started with a reset record")
                started_ms = int(time.time() * 1000)
                chassis.hset("DPU_STATE|dpu0", "dpu_control_plane_state", "down")
                reset = wait_for(lambda: state.hgetall("DPU_RESET_INFO|DPU0"), "installed actor did not publish the actual reset transition")
                require(reset.get("reset_status") == "true" and reset.get("dpu_id") == "switch0_dpu0" and
                        reset.get("vdpu_id") == "fixture-vdpu" and started_ms <= int(reset["timestamp"]) <= int(time.time() * 1000),
                        "HA reset record lost its native fields")
                chassis.hset("DPU_STATE|dpu0", "dpu_control_plane_state", "up")
                wait_for(observed_up, "installed actor did not observe recovery")
                require(state.hgetall("DPU_RESET_INFO|DPU0") == reset, "hamgrd unexpectedly cleared a latched reset")

                # The second real bus uses its original YAML test interface.
                # No remote DPU actor or dataplane completion is fabricated.
                peer_config = Path("/tmp/dash-ha-peer.yaml")
                peer_config.write_text('endpoint: "127.0.0.1:8101"\nroutes:\n  - key: "region-a.cluster-a.peer-dpu1"\n    scope: "InCluster"\npeers:\n  - endpoint: "198.18.0.1:8100"\n    conn_type: "InCluster"\n')
                peer = subprocess.Popen(["/usr/bin/swbusd", "-c", str(peer_config)], stdout=peer_log, stderr=subprocess.STDOUT, start_new_session=True)

                def routed_ping():
                    require(peer.poll() is None, "software peer exited: " + peer_log_path.read_text())
                    output = command("/usr/bin/swbus-cli", "ping", "-c", "1", "-t", "1", "region-a.cluster-a.peer-dpu1")
                    return output if "Response received: ping_seq=0" in output else None

                ping = wait_for(routed_ping, "installed software peer did not return a routed response")
                negative = command("/usr/bin/swbus-cli", "ping", "-c", "1", "-t", "1", "region-a.cluster-a.missing-dpu9")
                require("Response received:" not in negative, "unreachable peer unexpectedly replied")
                final_states = services_ready()
                require(final_states is not None, "HA service exited during the installed checks")
                require(all(int(command("/usr/local/bin/supervisorctl", "pid", name).strip()) == pid for name, pid in pids.items()),
                        "HA service restarted during the installed checks")
                print(json.dumps({"status": "passed", "source_elf_count": len(native), "loader_checks": len(probe["loader_checks"]),
                    "debug_checks": len(probe["debug_pairs"]), "command_checks": len(probe["commands"]),
                    "supervisor_states": final_states,
                    "observed_actor_state_sha256": hashlib.sha256(initial.encode()).hexdigest(),
                    "reset_transition": reset, "reset_stays_latched_after_recovery": True,
                    "software_peer_ping": {"response_received": True, "stdout_sha256": hashlib.sha256(ping.encode()).hexdigest()},
                    "unreachable_peer_has_no_response": True,
                    "scope": "Original installed HA startup, private Redis PMON reset transition and software bus peer routing. No physical DPU, ASIC forwarding, HA set/scope programming or VS installer claim."}))
            except Exception:
                import sys
                for path in [log_path, peer_log_path, Path("/var/log/syslog"), Path("/var/log/daemon.log"), *Path("/var/log/dash-ha").glob("*"), *Path("/var/log/supervisor").glob("*")]:
                    if path.is_file():
                        print("DIAGNOSTIC " + str(path) + "\n" + path.read_text(errors="replace")[-16000:], file=sys.stderr)
                raise
            finally:
                stop(peer)
                stop(supervisor)
                for client in clients.values():
                    client.close()


if __name__ == "__main__":
    main()
