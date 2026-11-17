#!/usr/bin/env python3
"""Exercise installed gearbox startup against private Redis and a VS PHY context."""

import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time
from xmlrpc.client import ServerProxy

import redis
from supervisor.xmlrpc import SupervisorTransport

sys.path.insert(0, str(Path(__file__).parent))
import runtime_smoke


def require(value, message):
    if not value:
        raise AssertionError(message)


def command(argv, timeout=30):
    result = subprocess.run(argv, text=True, capture_output=True, timeout=timeout)
    require(result.returncode == 0, repr(argv) + "\n" + result.stdout + result.stderr)
    return result.stdout


def wait_until(callback, message, timeout=30):
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        value = callback()
        if value:
            return value
        time.sleep(0.1)
    raise AssertionError(message)


def stop(process):
    if process.poll() is None:
        os.killpg(process.pid, signal.SIGTERM)
        try:
            process.wait(timeout=8)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            process.wait(timeout=5)


def full_vim_receipt(result):
    require(result["returncode"] == 0 and "Huge version" in result["stdout"],
            "debug command did not select full Vim")
    return {"returncode": 0, "huge_version": True}


def main():
    report = json.loads(Path(sys.argv[1]).read_text()); variant = sys.argv[2]
    expected = report["images"][variant]
    for name in expected["native_paths"]:
        path = Path("/") / name
        require(hashlib.sha256(path.read_bytes()).hexdigest() == report["native"][name]["sha256"], "native bytes differ: " + name)
        run = runtime_smoke.command(["/lib64/ld-linux-x86-64.so.2", "--list", str(path)])
        pair = next(item for item in report["source_debug_pairs"] if item["path"] == name)
        require(runtime_smoke.loader_result(run, pair.get("needed", [])), "installed loader failed: " + name + repr(run))
    debug_checks = []
    vim = None
    if variant.endswith("-dbg"):
        vim = full_vim_receipt(runtime_smoke.command(["/usr/bin/vim", "--version"]))
        targets = {"usr/bin/syncd", "usr/bin/syncd_dash", "usr/bin/pi_CLI_dummy", "usr/bin/sonic-db-cli"}
        selected = [pair for pair in report["source_debug_pairs"] if pair["path"] in targets]
        require(targets == {pair["path"] for pair in selected}, "missing installed debugger contracts")
        debug_checks = [runtime_smoke.debug_pair(pair) for pair in selected]
        for item in debug_checks:
            item["with_symbols"]["source"] = Path(item["with_symbols"]["source"]).name

    hwsku = Path("/usr/share/sonic/hwsku"); hwsku.mkdir(parents=True, exist_ok=True)
    database = Path("/run/redis/sonic-db/database_config.json"); database.parent.mkdir(parents=True, exist_ok=True)
    config = json.loads(Path("/probe/redis/database_config.json").read_text())
    socket_path = config["INSTANCES"]["redis"]["unix_socket_path"]
    require(Path(socket_path).resolve() == Path("/run/redis/redis.sock"), "unexpected private Redis socket")
    contexts = json.loads(Path("/probe/context_config_gearbox.json").read_text())
    for index, name in enumerate(("ASIC_DB_MAIN", "COUNTERS_DB_MAIN", "FLEX_DB_MAIN", "ASIC_DB_GB", "COUNTERS_DB_GB", "FLEX_DB_GB"), 10):
        config["DATABASES"][name] = {"id": index, "separator": ":", "instance": "redis"}
    require(config["DATABASES"]["CONFIG_DB"]["id"] == 4 and config["DATABASES"]["STATE_DB"]["id"] == 6,
            "original default database identities changed")
    database.write_text(json.dumps(config))
    (hwsku / "context_config.json").write_text(json.dumps(contexts))
    # The original manager exits successfully when this optional hardware file
    # is absent. This tests the actual installed script before supervisor starts.
    missing = subprocess.run(["/usr/bin/gbsyncdmgrd"], capture_output=True, text=True, timeout=8)
    require(missing.returncode == 0, "manager missing-platform behavior changed")
    (hwsku / "gearbox_config.json").write_text(json.dumps({"phys": [{}]}))
    profile = "SAI_VS_SWITCH_TYPE=SAI_VS_SWITCH_TYPE_BCM81724\nSAI_VS_HOSTIF_USE_TAP_DEVICE=false\n"
    (hwsku / "pai.profile").write_text(profile)
    command(["/usr/bin/start.sh"])
    require(Path("/etc/sai.d/pai.profile").read_text() == profile, "original copied profile differs")
    redis_binary = Path("/probe/redis/usr/bin/redis-server")
    fixture = {"executable": str(redis_binary), "sha256": hashlib.sha256(redis_binary.read_bytes()).hexdigest(),
               "database_config": str(database), "loopback_tcp": True}
    with runtime_smoke.redis_fixture(fixture) as redis_report:
        connection = redis.Redis(unix_socket_path=socket_path, db=4, decode_responses=True)
        tcp = redis.Redis(host="127.0.0.1", port=config["INSTANCES"]["redis"]["port"], db=4, socket_timeout=1)
        try:
            require(tcp.info("server")["process_id"] == connection.info("server")["process_id"] == redis_report["process_id"],
                    "TCP and Unix endpoints do not belong to the private Redis child")
        finally:
            tcp.close()
        connection.set("CONFIG_DB_INITIALIZED", "1")
        connection.hset("DEVICE_METADATA|localhost", mapping={"hostname": "gearbox-probe", "type": "ToRRouter"})
        # Exercise sonic-cfggen through the unchanged installed Jinja branch too.
        (hwsku / "pai.profile.j2").write_text(profile + "# {{ DEVICE_METADATA['localhost']['hostname'] }}\n")
        command(["/usr/bin/start.sh"])
        require(Path("/etc/sai.d/pai.profile").read_text() == profile + "# gearbox-probe\n", "installed cfggen profile rendering differs")
        primary = redis.Redis(unix_socket_path=socket_path, db=10, decode_responses=True)
        primary.set("gearbox-test:sentinel", "unchanged")
        for name in ("/var/log/supervisor", "/var/spool/rsyslog"):
            Path(name).mkdir(parents=True, exist_ok=True)
        log_path = Path("/tmp/gearbox-entrypoint.log")
        with log_path.open("w") as log:
            process = subprocess.Popen(["/usr/local/bin/supervisord"], stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
            try:
                def manager_pid():
                    require(process.poll() is None, "supervisor exited:\n" + log_path.read_text())
                    result = subprocess.run(["/usr/local/bin/supervisorctl", "pid", "gbsyncdmgrd"], text=True, capture_output=True, timeout=5)
                    return int(result.stdout.strip()) if result.returncode == 0 and result.stdout.strip().isdigit() else 0
                manager = wait_until(manager_pid, "supervisor did not start installed gearbox manager")
                args = Path(f"/proc/{manager}/cmdline").read_bytes().split(b"\0")
                require(b"/usr/bin/gbsyncdmgrd" in args, "supervisor ran a different manager")
                def syncd_child():
                    children = Path(f"/proc/{manager}/task/{manager}/children").read_text().split()
                    values = [int(pid) for pid in children if Path(f"/proc/{pid}/exe").exists() and os.path.samefile(f"/proc/{pid}/exe", "/usr/bin/syncd")]
                    require(len(values) <= 1, "one PHY unexpectedly started multiple Syncd children")
                    return values[0] if values else 0
                child = wait_until(syncd_child, "manager did not execute installed Syncd")
                expected_args = [b"/usr/bin/syncd", b"-s", b"-p", b"/etc/sai.d/pai.profile", b"-x", b"/usr/share/sonic/hwsku/context_config.json", b"-g", b"1"]
                require(Path(f"/proc/{child}/cmdline").read_bytes().rstrip(b"\0").split(b"\0") == expected_args, "manager changed gearbox arguments")
                def gearbox_connections():
                    return {int(item["db"]) for item in connection.client_list()} >= {13, 15}
                wait_until(gearbox_connections, "Syncd did not open the selected gearbox ASIC/FLEX databases")
                for _ in range(10):
                    time.sleep(0.5)
                    require(manager_pid() == manager and syncd_child() == child, "gearbox startup did not remain alive")
                require(primary.keys("*") == ["gearbox-test:sentinel"] and primary.get("gearbox-test:sentinel") == "unchanged",
                        "gearbox daemon changed the main ASIC database")
                require(not any(Path("/tmp/" + name).exists() for name in
                                ("main_zmq_ep", "gb_zmq_ep", "main_zmq_ntf_ep", "gb_zmq_ntf_ep")),
                        "gearbox incorrectly selected ZMQ")
                service_states = {}
                transport = SupervisorTransport(None, None, "unix:///var/run/supervisor.sock")
                with ServerProxy("http://localhost", transport=transport) as rpc:
                    services = {item["name"]: item for item in rpc.supervisor.getAllProcessInfo()}
                for service in ("dependent-startup", "supervisor-proc-exit-listener", "rsyslogd", "gbsyncdmgrd"):
                    status = services.get(service, {})
                    running = status.get("statename") == "RUNNING"
                    # The original startup listener exits successfully once its
                    # dependency graph has started. Persistent services must run.
                    completed = (service == "dependent-startup" and status.get("statename") == "EXITED"
                                 and status.get("exitstatus") == 0)
                    require(running or completed,
                            "required installed service failed: " + service + repr(status))
                    service_states[service] = {"state": status["statename"], "exitstatus": status["exitstatus"]}
                # A child failure must terminate the original manager. Stop via
                # supervisor's group boundary afterward; never signal host PIDs.
                os.kill(child, signal.SIGTERM)
                wait_until(lambda: not Path(f"/proc/{manager}").exists(), "manager did not notice its child exit")
            finally:
                stop(process)
    print(json.dumps({"status": "passed", "variant": variant, "native_files": len(expected["native_paths"]),
                      "debug_checks": debug_checks, "full_vim": vim, "private_redis": redis_report, "actual_entrypoint": True,
                      "actual_manager": True, "actual_syncd": True, "service_states": service_states, "context": 1, "gb_databases": [13, 15],
                      "main_database_unchanged": True, "profile_copy_and_render": True, "child_exit_observed": True,
                      "scope": "Installed VS gearbox manager/context and private Redis routing; no physical PHY or packet forwarding"}))


if __name__ == "__main__":
    main()
