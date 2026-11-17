#!/usr/bin/env python3
"""Exercise installed Teamd, SWSS and libteam against private kernel/Redis state."""

import argparse
import configparser
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import time

import redis
import runtime_smoke


def require(value, message):
    if not value:
        raise AssertionError(message)


def command(*argv):
    result = subprocess.run(argv, capture_output=True, text=True, timeout=30)
    require(result.returncode == 0, result.stdout + result.stderr)
    return result.stdout


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
        require(hashlib.sha256(path.read_bytes()).hexdigest() == item["sha256"], "source ELF differs: " + name)
        stat = path.stat()
        require((stat.st_uid, stat.st_gid, stat.st_mode & 0o7777) ==
                (item["uid"], item["gid"], item["mode"]), "source ELF permissions differ: " + name)
    inputs = {"architecture": "amd64",
              "elfs": [item for item in report["elfs"] if item["path"] in native],
              "debug_pairs": [item for item in report["source_debug_pairs"]
                              if item["path"] in {"usr/bin/teamd", "usr/bin/teamnl", "usr/bin/teamdctl"}],
              "policy": {"python_imports": ["swsscommon.swsscommon", "dash_api.utils"],
                         "commands": [{"argv": ["/usr/bin/teamd", "--version"], "output_pattern": "1.31"}]}}
    result = runtime_smoke.probe(inputs, "debug" if args.debug else "runtime")
    require(result["passed"], json.dumps(result))
    processes, streams = [], []
    clients = []
    with tempfile.TemporaryDirectory(prefix="teamd-image-") as temporary:
        work = Path(temporary)

        def start(name, argv):
            stream = (work / (name + ".log")).open("w+")
            streams.append(stream)
            process = subprocess.Popen(argv, cwd=work, stdout=stream, stderr=subprocess.STDOUT)
            processes.append(process)
            return process

        def diagnostics():
            return "\n".join(path.name + ":\n" + path.read_text()[-6000:] for path in work.glob("*.log"))

        def wait_for(predicate, message, timeout=30):
            deadline = time.monotonic() + timeout
            while not predicate():
                require(all(process.poll() is None for process in processes), message + "\n" + diagnostics())
                require(time.monotonic() < deadline, message + "\n" + diagnostics())
                time.sleep(.1)

        try:
            # /run/redis is an anonymous container volume copied from the image.
            # The supplied Redis helper is external test data, not an OCI layer.
            database = Path("/run/redis/sonic-db/database_config.json")
            config = json.loads(database.read_text())
            socket_path = config["INSTANCES"]["redis"]["unix_socket_path"]
            require(Path(socket_path).resolve() == Path("/run/redis/redis.sock"), "unexpected database socket")
            fixture = Path("/probe/redis")
            start("redis", [str(fixture / "usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2"),
                            "--library-path", str(fixture / "usr/lib/x86_64-linux-gnu"),
                            str(fixture / "usr/bin/redis-server"), "--port", "0",
                            "--unixsocket", socket_path, "--unixsocketperm", "700",
                            "--save", "", "--appendonly", "no", "--databases", "32",
                            "--notify-keyspace-events", "KEA"])
            state = redis.Redis(unix_socket_path=socket_path, db=6, decode_responses=True, socket_timeout=1)
            cfg = redis.Redis(unix_socket_path=socket_path, db=4, decode_responses=True, socket_timeout=1)
            clients.extend((state, cfg))

            def ready():
                try:
                    return state.ping()
                except redis.RedisError:
                    return False

            wait_for(ready, "private Redis did not start")
            cfg.hset("DEVICE_METADATA|localhost", mapping={"hostname": "bazel-teamd", "mac": "02:00:00:00:00:42"})
            # ConfigDBConnector.connect() in the shipped sonic-cfggen waits
            # for normal database initialization before reading these values.
            cfg.set("CONFIG_DB_INITIALIZED", "1")
            # Exercise the shipped startup renderer with the real private DB.
            rendered = command("/usr/local/bin/sonic-cfggen", "-d", "-t",
                               "/usr/share/sonic/templates/supervisord.conf.j2")
            supervisor = configparser.ConfigParser(interpolation=None)
            supervisor.read_string(rendered)
            for program in ("teammgrd", "teamsyncd", "tlm_teamd"):
                require(supervisor["program:" + program]["command"] == "/usr/bin/" + program,
                        "installed supervisor command differs: " + program)
            command("/usr/bin/start.sh")
            require(Path("/var/warmboot/teamd").is_dir(), "startup did not prepare the warmboot directory")
            # TeamMgr clears stale LAG state in its constructor. Wait for that
            # actual cleanup before creating the live team, avoiding a startup
            # race that could otherwise erase teamsyncd's first publication.
            readiness = "LAG_TABLE|BazelTeamdReadiness"
            state.hset(readiness, mapping={"fixture_readiness": "pending"})
            start("teammgrd", ["/usr/bin/teammgrd"])
            # No CONFIG_DB port-channel request is injected: this manager remains
            # idle while the fixture controls one private test team interface.
            wait_for(lambda: not state.exists(readiness), "teammgrd did not complete startup cleanup")
            require(all(process.poll() is None for process in processes), diagnostics())
            device = "PortChannel42"
            daemon_config = {"device": device, "hwaddr": "02:00:00:00:00:42",
                             "runner": {"name": "lacp", "active": True, "fast_rate": True, "fallback": True},
                             "ports": {}}
            teamd = start("teamd", ["/usr/bin/teamd", "-n", "-g", "-U", "-t", device,
                                    "-c", json.dumps(daemon_config), "-p", str(work / "teamd.pid"),
                                    "-l", "stderr"])
            control = {}

            def control_ready():
                value = subprocess.run(["/usr/bin/teamdctl", "-U", device, "state", "dump"],
                                       capture_output=True, text=True, timeout=3)
                (work / "teamdctl.log").write_text(
                    json.dumps({"returncode": value.returncode, "expected_pid": teamd.pid,
                                "socket_exists": Path("/var/run/teamd", device + ".sock").is_socket(),
                                "interface_exists": Path("/sys/class/net", device).exists(),
                                "stdout": value.stdout, "stderr": value.stderr}, indent=2) + "\n")
                if value.returncode:
                    return False
                control.update(json.loads(value.stdout))
                return control["setup"]["pid"] == teamd.pid

            wait_for(control_ready, "real teamd control did not return the running daemon's PID")
            observed_config = json.loads(command("/usr/bin/teamdctl", "-U", device, "config", "dump"))
            require(observed_config["runner"]["name"] == "lacp" and observed_config["device"] == device,
                    "teamdctl did not return the live daemon configuration")
            kernel_mode = command("/usr/bin/teamnl", device, "getoption", "mode").strip()
            require(kernel_mode == control["setup"]["kernel_team_mode_name"],
                    "teamnl did not read the same live kernel mode as teamdctl")
            start("teamsyncd", ["/usr/bin/teamsyncd"])
            key = "LAG_TABLE|" + device
            wait_for(lambda: state.hget(key, "state") == "ok", "teamsyncd did not publish the real kernel team")
            start("tlm_teamd", ["/usr/bin/tlm_teamd"])
            # A real netlink change exercises teamsyncd's update path and wakes
            # telemetry even when it starts after the initial LAG publication.
            command("/usr/bin/ip", "link", "set", "dev", device, "mtu", "1450")
            wait_for(lambda: state.hget(key, "mtu") == "1450", "teamsyncd missed the kernel MTU update")
            wait_for(lambda: state.hget(key, "setup.pid") == str(teamd.pid), "tlm_teamd did not read the real daemon")
            telemetry = state.hgetall(key)
            require(telemetry["setup.kernel_team_mode_name"] == control["setup"]["kernel_team_mode_name"],
                    "telemetry kernel mode differs from teamdctl")
            require(telemetry["runner.active"] == "true" and telemetry["runner.fast_rate"] == "true",
                    "telemetry lost the selected LACP configuration")
            print(json.dumps({"status": "passed", "source_elf_count": len(native),
                              "loader_checks": len(result["loader_checks"]), "debug_checks": len(result["debug_pairs"]),
                              "teamd_control": True, "teamsyncd_mtu": telemetry["mtu"],
                              "teamnl_kernel_mode": kernel_mode,
                              "tlm_teamd": True, "startup_template": True, "team_device": device,
                              "scope": "Private Redis, Unix control socket and no-port kernel team; no host network, traffic forwarding or installer boot"}, indent=2))
        finally:
            for process in reversed(processes):
                if process.poll() is None:
                    process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=5)
            for stream in streams:
                stream.close()
            for client in clients:
                client.close()


if __name__ == "__main__":
    main()
