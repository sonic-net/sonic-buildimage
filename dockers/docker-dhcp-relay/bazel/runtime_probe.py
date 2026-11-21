#!/usr/bin/env python3
"""Exercise installed relay startup against private Redis and virtual links."""

import configparser
import hashlib
import importlib
import importlib.metadata
import json
import os
from pathlib import Path
import shlex
import signal
import socket
import subprocess
import sys
import time
import xmlrpc.client

import redis
from supervisor.xmlrpc import SupervisorTransport

sys.path.insert(0, str(Path(__file__).parent))
import runtime_smoke


def require(value, message):
    if not value:
        raise AssertionError(message)


def command(argv, timeout=40):
    result = subprocess.run(argv, text=True, capture_output=True, timeout=timeout)
    require(result.returncode == 0, repr(argv) + "\n" + result.stdout + result.stderr)
    return result.stdout


def wait_until(callback, message, timeout=45):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        result = callback()
        if result:
            return result
        time.sleep(0.2)
    raise AssertionError(message)


def fixture(native, vlans=True, dualtor=False):
    metadata = {"hostname": "relay-probe", "type": "ToRRouter", "deployment_id": "1",
                "mac": "02:00:00:00:00:01", "has_sonic_dhcpv4_relay": str(native)}
    if dualtor:
        metadata["subtype"] = "DualToR"
    # Empty tables vanish in Redis; startup needs a real port for its alias map.
    data = {"DEVICE_METADATA": {"localhost": metadata}, "PORT": {"Ethernet0": {"alias": "etp1"}},
            "INTERFACE": {"Ethernet0": {}, "Ethernet0|198.51.100.1/24": {},
                          "Ethernet0|2001:db8:1::1/64": {}},
            "PORTCHANNEL_INTERFACE": {}, "MGMT_INTERFACE": {}, "VLAN": {}, "VLAN_INTERFACE": {},
            "DHCP_RELAY": {}, "FEATURE": {"dhcp_relay": {"state": "enabled", "set_owner": "local"},
                                           "dhcp_server": {"state": "disabled"}}}
    if vlans:
        data.update({"VLAN": {"Vlan1000": {"vlanid": "1000", "dhcp_servers": ["198.51.100.2"]}},
                     "VLAN_INTERFACE": {"Vlan1000": {}, "Vlan1000|192.0.2.1/24": {},
                                        "Vlan1000|2001:db8::1/64": {}},
                     "DHCP_RELAY": {"Vlan1000": {"dhcpv6_servers": ["2001:db8:1::2"]}}})
    return data


def render_case(name, data, expected):
    path = Path("/tmp") / (name + ".json")
    path.write_text(json.dumps(data))
    rendered = command(["sonic-cfggen", "-j", str(path), "-t",
                        "/usr/share/sonic/templates/docker-dhcp-relay.supervisord.conf.j2"])
    parser = configparser.ConfigParser(interpolation=None, strict=True)
    parser.read_string(rendered)
    programs = {section.removeprefix("program:"): parser.get(section, "command")
                for section in parser.sections() if section.startswith("program:")}
    require(set(programs) == expected | {"rsyslogd", "start", "dhcprelayd"},
            name + " selected wrong daemons: " + repr(programs))
    for executable in (shlex.split(value)[0] for value in programs.values()):
        require(Path(executable).is_file() and os.access(executable, os.X_OK),
                "installed supervisor executable missing: " + executable)
    if data["DEVICE_METADATA"]["localhost"].get("subtype") == "DualToR":
        for daemon in ("dhcp4relay", "dhcp6relay", "dhcpmon-Vlan1000"):
            require("-u Loopback0" in programs[daemon], "DualToR option missing: " + daemon)
    return programs


def write_config(client, data):
    client.flushdb()
    for table, entries in data.items():
        for key, fields in entries.items():
            values = {name + "@" if isinstance(value, list) else name:
                      ",".join(value) if isinstance(value, list) else value for name, value in fields.items()}
            client.hset(table + "|" + key, mapping=values or {"NULL": "NULL"})
    client.set("CONFIG_DB_INITIALIZED", "1")


def write_interface_state(client, data):
    """Mirror readiness for the probe's already configured private interfaces."""
    client.flushdb()
    for table in ("INTERFACE", "VLAN_INTERFACE", "PORTCHANNEL_INTERFACE"):
        for key in data.get(table, {}):
            if "|" in key:
                client.hset("INTERFACE_TABLE|" + key, mapping={"state": "ok"})


def installed_startup(name, data, programs, config_db, counter_db, state_db):
    write_config(config_db, data)
    # The installed start.sh waits for these STATE_DB records before relays
    # launch. Configuring the namespace's links alone does not publish them.
    write_interface_state(state_db, data)
    counter_db.hset("DHCPV4_COUNTER_TABLE:probe", mapping={"RX": "1"})
    log_path = Path("/tmp") / (name + "-startup.log")
    for directory in ("/var/log/supervisor", "/var/spool/rsyslog"):
        Path(directory).mkdir(parents=True, exist_ok=True)
    # Each profile starts fresh instead of caching the prior supervisor groups.
    Path("/etc/supervisor/conf.d/docker-dhcp-relay.supervisord.conf").unlink(missing_ok=True)
    endpoint = configparser.ConfigParser(interpolation=None, inline_comment_prefixes=(";", "#"))
    endpoint.read("/etc/supervisor/supervisord.conf")
    server_url = endpoint.get("supervisorctl", "serverurl")
    require(server_url.startswith("unix://"), "Supervisor endpoint must be a private Unix socket")
    transport = SupervisorTransport(serverurl=server_url)
    supervisor = xmlrpc.client.ServerProxy("http://localhost", transport=transport).supervisor
    with log_path.open("w") as log:
        process = subprocess.Popen(["/usr/bin/docker_init.sh"], stdout=log, stderr=subprocess.STDOUT,
                                   start_new_session=True)
        try:
            config = configparser.ConfigParser(interpolation=None)
            groups = {}

            def pids():
                require(process.poll() is None, "entrypoint exited:\n" + log_path.read_text())
                # Bound the actual Unix socket, including connect/read, rather than
                # spawning one Python supervisorctl process for each daemon.
                previous_timeout = socket.getdefaulttimeout()
                socket.setdefaulttimeout(5)
                try:
                    processes = supervisor.getAllProcessInfo()
                except (OSError, xmlrpc.client.Error):
                    return None
                finally:
                    transport.close()
                    socket.setdefaulttimeout(previous_timeout)
                if not groups:
                    config.read("/etc/supervisor/conf.d/docker-dhcp-relay.supervisord.conf")
                    for section in config.sections():
                        if section.startswith("group:"):
                            for member in config.get(section, "programs").split(","):
                                groups[member.strip()] = section.removeprefix("group:")
                statuses = {(item["group"], item["name"]): item for item in processes}
                require(len(statuses) == len(processes), "Supervisor returned duplicate program identities")
                observed = {}
                for daemon in programs:
                    if daemon == "start":
                        continue
                    status = statuses.get((groups.get(daemon, daemon), daemon))
                    if not status or status["statename"] != "RUNNING" or status["pid"] <= 0:
                        return None
                    observed[daemon] = status["pid"]
                return observed

            observed = wait_until(pids, "supervisor did not start declared daemons: " + name)
            require(Path("/tmp/port-name-alias-map.txt").read_text().split() == ["Ethernet0", "etp1"],
                    "entrypoint did not render the private port alias")
            for daemon, pid in observed.items():
                binary = shlex.split(programs[daemon])[0]
                if daemon != "dhcprelayd":
                    require(os.path.samefile("/proc/" + str(pid) + "/exe", binary),
                            "supervisor did not execute installed daemon: " + daemon)
                else:
                    require(binary.encode() in Path("/proc/" + str(pid) + "/cmdline").read_bytes(),
                            "supervisor did not execute installed DHCP utility")
            require(not counter_db.exists("DHCPV4_COUNTER_TABLE:probe"), "start.sh did not clear old counters")
            for _ in range(20):
                time.sleep(0.5)
                require(pids() == observed, "daemon restarted during acceptance: " + name)
            return {"daemons": sorted(observed), "startup_cleared_counters": True, "stable_seconds": 10}
        finally:
            transport.close()
            if process.poll() is None:
                os.killpg(process.pid, signal.SIGTERM)
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL)
                    process.wait()


def main():
    report = json.loads(Path(sys.argv[1]).read_text())
    variant = sys.argv[2]
    expected = report["images"][variant]
    for name in expected["native_paths"]:
        path = Path("/") / name
        require(hashlib.sha256(path.read_bytes()).hexdigest() == report["native"][name]["sha256"],
                "native installed bytes differ: " + name)
        require("not found" not in command(["/lib64/ld-linux-x86-64.so.2", "--list", str(path)]),
                "native loader failed: " + name)
    for module in ("dhcp_utilities.dhcprelayd.dhcprelayd", "dhcp_utilities.dhcpservd.dhcpservd",
                   "swsscommon.swsscommon", "psutil", "freezegun", "jinja2"):
        importlib.import_module(module)
    distribution = importlib.metadata.distribution("sonic-dhcp-utilities")
    require(distribution.version == "1.0", "DHCP utilities metadata version differs")
    require({entry.name for entry in distribution.entry_points if entry.group == "console_scripts"}
            == {"dhcprelayd", "dhcpservd"}, "DHCP utilities console scripts differ")
    require(distribution.locate_file("dhcp_utilities/dhcpservd/dhcp_option.csv").is_file(),
            "DHCP option resource missing")
    debug_checks = []
    if variant.endswith("-dbg"):
        required = {"usr/sbin/" + name for name in ("dhcrelay", "dhcp4relay", "dhcp6relay", "dhcpmon")}
        selected = [pair for pair in report["source_debug_pairs"] if pair["path"] in required]
        require({pair["path"] for pair in selected} == required, "relay debug symbols incomplete")
        debug_checks = [runtime_smoke.debug_pair(pair) for pair in selected]
        for record in debug_checks:
            record["with_symbols"]["source"] = Path(record["with_symbols"]["source"]).name

    cases = [("no-vlan", fixture(False, False), set()),
             ("isc", fixture(False), {"isc-dhcpv4-relay-Vlan1000", "dhcp6relay", "dhcpmon-Vlan1000"}),
             ("native", fixture(True), {"dhcp4relay", "dhcp6relay", "dhcpmon-Vlan1000"})]
    rendered = {name: render_case(name, data, daemons) for name, data, daemons in cases}
    render_case("dualtor", fixture(True, dualtor=True), {"dhcp4relay", "dhcp6relay", "dhcpmon-Vlan1000"})
    database = Path("/run/redis/sonic-db/database_config.json")
    database.parent.mkdir(parents=True, exist_ok=True)
    database.write_bytes(Path("/probe/redis/database_config.json").read_bytes())
    config = json.loads(database.read_text())
    socket_path = config["INSTANCES"]["redis"]["unix_socket_path"]
    require(Path(socket_path).resolve() == Path("/run/redis/redis.sock"), "Redis socket is not private")
    require(config["INSTANCES"]["redis"]["hostname"] == "127.0.0.1", "Redis hostname is not loopback")
    binary = Path("/probe/redis/usr/bin/redis-server")
    redis_fixture = {"executable": str(binary), "sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
                     "database_config": str(database), "loopback_tcp": True}
    for args in (["link", "add", "Vlan1000", "type", "veth", "peer", "name", "Ethernet0"],
                 ["link", "set", "Vlan1000", "up"], ["link", "set", "Ethernet0", "up"],
                 ["addr", "add", "198.51.100.1/24", "dev", "Ethernet0"],
                 ["-6", "addr", "add", "2001:db8:1::1/64", "dev", "Ethernet0", "nodad"],
                 ["addr", "add", "192.0.2.1/24", "dev", "Vlan1000"],
                 ["-6", "addr", "add", "2001:db8::1/64", "dev", "Vlan1000", "nodad"]):
        command(["ip", *args])
    startup = {}
    with runtime_smoke.redis_fixture(redis_fixture):
        databases = {name: redis.Redis(unix_socket_path=socket_path,
                                      db=config["DATABASES"][name]["id"], decode_responses=True)
                     for name in ("CONFIG_DB", "COUNTERS_DB", "STATE_DB")}
        for name, data, _ in cases:
            startup[name] = installed_startup(name, data, rendered[name], databases["CONFIG_DB"],
                                              databases["COUNTERS_DB"], databases["STATE_DB"])
    print(json.dumps({"status": "passed", "variant": variant, "native_files": len(expected["native_paths"]),
                      "debug_checks": debug_checks, "startup": startup, "dualtor_template": True,
                      "host_network_used": False, "host_services_used": False,
                      "scope": "Installed entrypoint, relay/monitor startup, Python metadata, and native/debug loading; protocol unit tests remain with source owners"}))


if __name__ == "__main__":
    main()
