#!/usr/bin/env python3
"""Exercise installed database startup, private Redis and the native DASH API."""

import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import threading
import time

from importlib.metadata import version
import redis

sys.path.insert(0, str(Path(__file__).parent))
import runtime_smoke


def require(value, message):
    if not value:
        raise AssertionError(message)


def command(arguments, data=None, env=None):
    result = subprocess.run(arguments, input=data, capture_output=True, timeout=45, env=env)
    require(result.returncode == 0, repr(arguments) + "\n" + result.stdout.decode(errors="replace") + result.stderr.decode(errors="replace"))
    return result.stdout


def main():
    report = json.loads(Path(sys.argv[1]).read_text())
    variant, profile = sys.argv[2:4]
    expected = report["images"][variant]
    print("Checking installed native files", file=sys.stderr, flush=True)
    require(os.environ.get("INCLUDE_SYSTEM_EVENTD") == report["include_system_eventd"], "event database selection changed")
    for name in expected["native_paths"]:
        path = Path("/") / name
        require(hashlib.sha256(path.read_bytes()).hexdigest() == report["native"][name]["sha256"], "native bytes differ: " + name)
        require(b"not found" not in command(["/lib64/ld-linux-x86-64.so.2", "--list", str(path)]), "missing native dependency: " + name)
    import dash_api.utils
    from dash_api import vnet_pb2
    from swsscommon import swsscommon
    value = vnet_pb2.Vnet(vni=4321)
    require(vnet_pb2.Vnet.FromString(value.SerializeToString()).vni == 4321, "generated Python API failed")
    # The owner's C++ JSON API deliberately prints default primitive/repeated
    # fields. Include those fields so this compares its canonical JSON contract.
    original = {"vni": 4321, "version": "database-image-probe", "address_space": [], "peer_list": []}
    encoded = command(["/usr/bin/dash_api_utils", "--to_proto", "-t", "DASH_VNET_TABLE"], json.dumps(original).encode())
    decoded = command(["/usr/bin/dash_api_utils", "--to_json", "-t", "DASH_VNET_TABLE"], encoded)
    require(json.loads(decoded) == original, "installed DASH CLI/C++/Protobuf roundtrip failed: " + decoded.decode())
    parsed = vnet_pb2.Vnet.FromString(encoded)
    require(parsed.vni == original["vni"] and parsed.version == original["version"],
            "generated Python schema disagrees with the native CLI")
    print("Native loaders and DASH roundtrip passed", file=sys.stderr, flush=True)
    require(version("click") == "8.1.8", "Click lock changed")
    debug_checks = []
    if variant.endswith("-dbg") and profile == "single":
        debug_checks = [runtime_smoke.debug_pair(item) for item in report["own_debug_pairs"]]
    # Render the unchanged startup templates with the installed upstream tool.
    # No network interfaces or hardware services are mocked into the image.
    template = Path("/usr/share/sonic/templates")
    fixtures = []
    with tempfile.TemporaryDirectory(prefix="database-templates-") as temporary:
        for eventd in ("y", "n"):
            env = dict(os.environ, HOST_IP="127.0.0.1", REDIS_PORT="6379", DATABASE_TYPE="", BMP_DB_PORT="6400", DEV="",
                       include_system_eventd=eventd)
            rendered = json.loads(command(["/usr/local/bin/jinjanate", str(template / "database_config.json.j2")], env=env))
            require(("EVENT_DB" in rendered["DATABASES"]) == (eventd == "y"), "event database template condition changed")
            fixtures.append({"template": "single", "eventd": eventd})
        env = dict(os.environ, NAMESPACE_COUNT="2", NAMESPACE_PREFIX="asic", NUM_DPU="0")
        global_data = json.loads(command(["/usr/local/bin/jinjanate", str(template / "database_global.json.j2")], env=env))
        require({row.get("namespace") for row in global_data["INCLUDES"]} == {None, "asic0", "asic1"}, "multi-ASIC global includes differ")
        path = Path(temporary) / "chassis.json"
        path.write_text(json.dumps(rendered))
        command(["/usr/local/bin/update_chassisdb_config", "-j", str(path), "-k", "-p", "6380"])
        chassis = json.loads(path.read_text())
        require(set(chassis["INSTANCES"]) == {"redis_chassis"} and chassis["INSTANCES"]["redis_chassis"]["port"] == 6380,
                "installed chassis selector changed")
        for item in chassis["INSTANCES"].values():
            item["is_protected_mode"] = False
        path.write_text(json.dumps(chassis))
        rendered_supervisor = command(["/usr/local/bin/sonic-cfggen", "-j", str(path), "-t", str(template / "supervisord.conf.j2")])
        require(b"[program:redis_chassis]" in rendered_supervisor and b"--port 6380" in rendered_supervisor,
                "installed chassis supervisor rendering failed")
    # Run the real entrypoint. Only runtime write locations are mounted tmpfs.
    # Its Redis processes stay in this network-isolated container.
    if profile == "multi":
        Path("/etc/sonic/enable_multidb").touch()
    require(profile in ("single", "multi"), "unknown fixture profile")
    Path("/var/log/supervisor").mkdir(parents=True, exist_ok=True)
    Path("/var/spool/rsyslog").mkdir(parents=True, exist_ok=True)
    database_path = Path("/run/redis/sonic-db/database_config.json")
    stopped = threading.Event()
    observed, invalid = [], []

    def observe_publication():
        while not stopped.is_set():
            try:
                data = json.loads(database_path.read_bytes())
                require("DATABASES" in data and "INSTANCES" in data, "incomplete published database structure")
                observed.append(1)
            except FileNotFoundError:
                pass
            except Exception as error:
                invalid.append(str(error))
            stopped.wait(0.002)

    reader = threading.Thread(target=observe_publication, daemon=True)
    reader.start()
    log_path = Path("/tmp/database-startup.log")
    with log_path.open("w") as log:
        process = subprocess.Popen(["/usr/local/bin/docker-database-init.sh"], stdout=log, stderr=subprocess.STDOUT,
                                   start_new_session=True, env=dict(os.environ, NAMESPACE_ID="", NAMESPACE_COUNT="1", NUM_DPU="0", DATABASE_TYPE="", DEV=""))
        try:
            deadline = time.monotonic() + 45
            while True:
                require(process.poll() is None, "database entrypoint exited:\n" + log_path.read_text())
                try:
                    server = redis.Redis(unix_socket_path="/run/redis/redis.sock", socket_timeout=1, decode_responses=True)
                    if server.ping():
                        break
                except redis.RedisError:
                    pass
                require(time.monotonic() < deadline, "installed Redis did not become ready:\n" + log_path.read_text())
                time.sleep(0.1)
            published = json.loads(database_path.read_bytes())
            expected_names = {"redis", "redis_bmp"} | ({"redis1", "redis2", "redis3", "redis4"} if profile == "multi" else set())
            require(set(published["INSTANCES"]) == expected_names, "startup database instance set differs")
            require("redis_chassis" not in published["INSTANCES"], "ordinary database unexpectedly starts chassis server")
            if profile == "single":
                require(("EVENT_DB" in published["DATABASES"]) == (report["include_system_eventd"] == "y"), "startup event DB condition differs")
            for setting, wanted in (("databases", "100"), ("save", ""), ("daemonize", "no"), ("logfile", ""),
                                    ("notify-keyspace-events", "AKE"), ("client-output-buffer-limit", "normal 0 0 0 slave 268435456 67108864 60 pubsub 0 0 0")):
                got = server.config_get(setting)[setting]
                if setting == "client-output-buffer-limit":
                    require(got.endswith("pubsub 0 0 0"), "pubsub buffering policy changed: " + got)
                else:
                    require(str(got) == wanted, setting + " differs: " + str(got))
            # Read procfs as Redis's own UID. The root probe intentionally has
            # no ptrace capability, so it cannot dereference another UID's exe.
            # samefile also handles Debian's executable hard-link aliases.
            identity = subprocess.run(
                ["/usr/bin/python3", "-c",
                 "import os,sys; assert os.path.samefile(sys.argv[1], sys.argv[2]); print(os.readlink(sys.argv[1]))",
                 "/proc/" + str(server.info("server")["process_id"]) + "/exe", "/usr/bin/redis-server"],
                user=100, group=101, extra_groups=[], capture_output=True, text=True, timeout=15)
            require(identity.returncode == 0, "Redis is not the installed process: " + identity.stderr)
            for name, instance in published["INSTANCES"].items():
                client = redis.Redis(unix_socket_path=instance["unix_socket_path"], socket_timeout=1, decode_responses=True)
                while True:
                    try:
                        if client.ping():
                            break
                    except redis.RedisError:
                        pass
                    require(time.monotonic() < deadline, "instance did not start: " + name)
                    time.sleep(0.1)
                info = client.info("server")
                status = Path("/proc/" + str(info["process_id"]) + "/status").read_text()
                require("Uid:\t100\t100\t100\t100" in status, "Redis is not running as its configured user")
            # Common's database-specific CLI follows redis-py's boolean PING
            # format; its separate no-database PING operation prints PONG.
            ping = command(["/usr/bin/sonic-db-cli", "CONFIG_DB", "PING"]).strip()
            require(ping == b"True", "source sonic-db-cli cannot connect: " + repr(ping))
            command(["/usr/bin/sonic-db-cli", "CONFIG_DB", "HSET", "DATABASE_IMAGE_PROBE|state", "value", "accepted"])
            require(redis.Redis(unix_socket_path="/run/redis/redis.sock", db=4).hget("DATABASE_IMAGE_PROBE|state", "value") == b"accepted",
                    "sonic-db-cli write not observed in CONFIG_DB")
            # SELECT99 proves the configured database limit is actually active.
            high = redis.Redis(unix_socket_path="/run/redis/redis.sock", db=99)
            require(high.set("database-image-probe", "last-db"), "logical database99 is unavailable")
            require(not list(database_path.parent.glob("*.tmp")) and not list(database_path.parent.glob("*.new")), "unfinished atomic DB publication")
            require(Path("/etc/supervisor/critical_processes").is_file(), "critical service list was not generated")
        finally:
            stopped.set()
            reader.join(timeout=5)
            if process.poll() is None:
                os.killpg(process.pid, signal.SIGTERM)
                try:
                    process.wait(timeout=8)
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL)
                    process.wait()
    require(observed and not invalid, "invalid intermediate database publication: " + repr(invalid))
    print(json.dumps({"status": "passed", "variant": variant, "profile": profile, "native_files": len(expected["native_paths"]),
                      "debug_checks": debug_checks, "dash_native_cli_roundtrip": True, "generated_python_proto_roundtrip": True,
                      "template_fixtures": fixtures, "multi_asic_global_config": True, "chassis_config_and_supervisor": True,
                      "actual_entrypoint": True, "redis_instances": sorted(expected_names), "logical_databases": 100,
                      "source_sonic_db_cli_write_observed": True, "atomic_database_reads": len(observed),
                      "host_services_used": False}))


if __name__ == "__main__":
    main()
