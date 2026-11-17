#!/usr/bin/env python3
"""Exercise installed router advertisement startup on an isolated virtual link."""

import configparser
import hashlib
import ipaddress
import json
import os
from pathlib import Path
import pwd
import signal
import socket
import struct
import subprocess
import sys
import time

import redis

sys.path.insert(0, str(Path(__file__).parent))
import runtime_smoke


def require(value, message):
    if not value:
        raise AssertionError(message)


def command(arguments, timeout=40):
    result = subprocess.run(arguments, capture_output=True, text=True, timeout=timeout)
    require(result.returncode == 0, repr(arguments) + "\n" + result.stdout + result.stderr)
    return result.stdout


def wait_until(callback, message, timeout=30):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        value = callback()
        if value:
            return value
        time.sleep(0.1)
    raise AssertionError(message)


def advertisement(packet):
    """Inspect the real Ethernet/IPv6/ICMPv6 packet from the private peer."""
    if len(packet) < 70 or packet[12:14] != b"\x86\xdd":
        return None
    ipv6 = packet[14:54]
    payload = packet[54:]
    if ipv6[6] != 58 or payload[0] != 134:
        return None
    require(ipv6[7] == 255 and payload[1] == 0, "invalid router advertisement header")
    require(ipaddress.IPv6Address(ipv6[8:24]).is_link_local, "RA source is not link-local")
    require(payload[5] & 0x80 and not payload[5] & 0x40, "RA managed/other flags differ")
    prefix, mtu = None, None
    offset = 16
    while offset < len(payload):
        require(offset + 2 <= len(payload), "truncated RA option header")
        size = payload[offset + 1] * 8
        require(size > 0 and offset + size <= len(payload), "invalid RA option length")
        option = payload[offset:offset + size]
        if option[0] == 3:
            require(size == 32 and option[2] == 64 and option[3] & 0x80 and not option[3] & 0x40,
                    "RA prefix flags differ")
            prefix = str(ipaddress.IPv6Address(option[16:32])) + "/64"
            require(struct.unpack("!II", option[4:12]) == (0xffffffff, 0xffffffff), "RA lifetimes differ")
        elif option[0] == 5:
            require(size == 8, "invalid RA MTU option")
            mtu = struct.unpack("!I", option[4:8])[0]
        offset += size
    require(prefix == "2001:db8:100::/64" and mtu == 9100, "RA prefix/MTU differs")
    return {"prefix": prefix, "mtu": mtu, "managed": True, "autonomous": False, "hop_limit": 255}


def main():
    report = json.loads(Path(sys.argv[1]).read_text())
    variant = sys.argv[2]
    expected = report["images"][variant]
    require(os.environ.get("IMAGE_VERSION") == report["image_version"], "image version changed")
    user = pwd.getpwnam("radvd")
    require((user.pw_uid, user.pw_gid, user.pw_dir, user.pw_shell) ==
            (101, 65534, "/run/radvd", "/usr/sbin/nologin"), "radvd account contract differs")
    for name in expected["native_paths"]:
        path = Path("/") / name
        require(hashlib.sha256(path.read_bytes()).hexdigest() == report["native"][name]["sha256"], "native bytes differ: " + name)
        require("not found" not in command(["/lib64/ld-linux-x86-64.so.2", "--list", str(path)]), "native loader failed: " + name)
    require("not found" not in command(["/lib64/ld-linux-x86-64.so.2", "--list", "/usr/sbin/radvd"]), "radvd loader failed")
    debug_checks = []
    if variant.endswith("-dbg"):
        selected = [item for item in report["source_debug_pairs"] if item["path"] in
                    {"usr/bin/sonic-db-cli", "usr/bin/redis-cli", "usr/bin/ssh"}]
        require(selected, "missing parent source debugger inputs")
        debug_checks = [runtime_smoke.debug_pair(item) for item in selected]
        # The public receipt needs the verified source filename, line and type,
        # not build-machine paths embedded in inherited debug information.
        for item in debug_checks:
            location = item["with_symbols"]
            location["source"] = Path(location["source"]).name
    print("Native files and account passed", file=sys.stderr, flush=True)

    database = Path("/run/redis/sonic-db/database_config.json")
    database.parent.mkdir(parents=True, exist_ok=True)
    database.write_bytes(Path("/probe/redis/database_config.json").read_bytes())
    config = json.loads(database.read_text())
    socket_path = config["INSTANCES"]["redis"]["unix_socket_path"]
    require(Path(socket_path).resolve() == Path("/run/redis/redis.sock"), "unexpected private Redis socket")
    redis_binary = Path("/probe/redis/usr/bin/redis-server")
    fixture = {"executable": str(redis_binary), "sha256": hashlib.sha256(redis_binary.read_bytes()).hexdigest(),
               "database_config": str(database), "loopback_tcp": True}
    require(config["INSTANCES"]["redis"]["hostname"] == "127.0.0.1", "Redis fixture is not loopback")
    template = Path("/usr/share/sonic/templates")
    cases = []
    with runtime_smoke.redis_fixture(fixture):
        client = redis.Redis(unix_socket_path=socket_path, db=4, decode_responses=True)
        state = redis.Redis(unix_socket_path=socket_path, db=6, decode_responses=True)
        require(client.ping(), "private CONFIG_DB not available")
        client.set("CONFIG_DB_INITIALIZED", "1")
        for device, ipv6, enabled in (("ToRRouter", True, True), ("EPMS", True, True),
                                      ("MgmtTsToR", True, True), ("SpineRouter", True, False),
                                      ("ToRRouter", False, False)):
            client.flushdb()
            client.set("CONFIG_DB_INITIALIZED", "1")
            client.hset("DEVICE_METADATA|localhost", mapping={"type": device, "hostname": "radv-probe", "hwsku": "Force10-S6000"})
            client.hset("VLAN|Vlan100", mapping={"vlanid": "100"})
            client.hset("VLAN_INTERFACE|Vlan100", mapping={"NULL": "NULL"})
            address = "2001:db8:100::1/64" if ipv6 else "192.0.2.1/24"
            client.hset("VLAN_INTERFACE|Vlan100|" + address, mapping={"NULL": "NULL"})
            rendered = command(["/usr/local/bin/sonic-cfggen", "-d", "-t", str(template / "docker-router-advertiser.supervisord.conf.j2")])
            parsed = configparser.ConfigParser(interpolation=None)
            parsed.read_string(rendered)
            require(parsed.has_section("program:radvd") == enabled and parsed.has_section("program:wait_for_link") == enabled,
                    "installed supervisor template condition differs: " + device)
            cases.append({"device": device, "ipv6_vlan": ipv6, "radvd_enabled": enabled})
        # Restore the enabled configuration and run the original entrypoint.
        client.hdel("VLAN_INTERFACE|Vlan100|192.0.2.1/24", "NULL")
        client.delete("VLAN_INTERFACE|Vlan100|192.0.2.1/24")
        client.hset("VLAN_INTERFACE|Vlan100|2001:db8:100::1/64", mapping={"NULL": "NULL"})
        client.hset("FEATURE|radv", mapping={"set_owner": "local"})
        state.hset("FEATURE|radv", mapping={"system_state": "up"})
        for path in ("/var/log/supervisor", "/var/spool/rsyslog"):
            Path(path).mkdir(parents=True, exist_ok=True)
        # These are the two private writable fixture mounts. Match the root
        # ownership of files the real entrypoint creates in the deployed image.
        for path in ("/etc/radvd.conf", "/usr/bin/wait_for_link.sh"):
            os.chown(path, 0, 0)
        for args in (["link", "add", "Vlan100", "type", "veth", "peer", "name", "RadvPeer"],
                     ["link", "set", "Vlan100", "mtu", "9100", "up"],
                     ["link", "set", "RadvPeer", "mtu", "9100", "up"],
                     ["-6", "addr", "add", "2001:db8:100::1/64", "dev", "Vlan100", "nodad"]):
            command(["/usr/sbin/ip", *args])
        with socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.htons(0x86dd)) as capture:
            capture.bind(("RadvPeer", 0))
            capture.settimeout(1)
            log_path = Path("/tmp/radv-startup.log")
            with log_path.open("w") as log:
                process = subprocess.Popen(["/usr/bin/docker-init.sh"], stdout=log, stderr=subprocess.STDOUT,
                                           start_new_session=True, env=dict(os.environ, RUNTIME_OWNER="local"))
                try:
                    def waiting():
                        require(process.poll() is None, "entrypoint exited:\n" + log_path.read_text())
                        result = subprocess.run(["/usr/local/bin/supervisorctl", "status"], capture_output=True, text=True, timeout=5)
                        return "wait_for_link" in result.stdout and "RUNNING" in next(
                            (line for line in result.stdout.splitlines() if line.startswith("wait_for_link ")), "")
                    wait_until(waiting, "wait_for_link did not run before VLAN readiness")
                    before = subprocess.run(["/usr/local/bin/supervisorctl", "status", "radvd"], capture_output=True, text=True, timeout=5)
                    require("RUNNING" not in before.stdout, "radvd started before VLAN readiness")
                    state.hset("VLAN_TABLE|Vlan100", mapping={"state": "ok"})
                    observed = command(["/usr/bin/sonic-db-cli", "STATE_DB", "HGET", "VLAN_TABLE|Vlan100", "state"])
                    require(observed.strip() == "ok", "installed VLAN readiness query differs: " + repr(observed))
                    print("Installed VLAN readiness query passed", file=sys.stderr, flush=True)
                    packet_result = None
                    deadline = time.monotonic() + 35
                    while time.monotonic() < deadline and packet_result is None:
                        require(process.poll() is None, "entrypoint exited:\n" + log_path.read_text())
                        try:
                            packet_result = advertisement(capture.recv(65535))
                        except socket.timeout:
                            pass
                    if packet_result is None:
                        status = subprocess.run(["/usr/local/bin/supervisorctl", "status"],
                                                capture_output=True, text=True, timeout=5)
                        logs = {str(path): path.read_text(errors="replace")[-6000:]
                                for path in Path("/var/log").rglob("*") if path.is_file()}
                        raise AssertionError("no actual router advertisement:\n" + log_path.read_text() +
                                             "\nsupervisor status:\n" + status.stdout + status.stderr +
                                             "\nservice logs:\n" + json.dumps(logs))
                    pid = int(command(["/usr/local/bin/supervisorctl", "pid", "radvd"]).strip())
                    require(pid > 0 and os.path.samefile("/proc/" + str(pid) + "/exe", "/usr/sbin/radvd"),
                            "supervisor did not execute installed radvd")
                    require(state.hget("FEATURE|radv", "current_owner") == "local" and
                            state.hget("FEATURE|radv", "container_version") == report["image_version"],
                            "original container_startup state update failed")
                    command(["/usr/sbin/radvd", "--configtest", "-C", "/etc/radvd.conf"])
                finally:
                    if process.poll() is None:
                        os.killpg(process.pid, signal.SIGTERM)
                        try:
                            process.wait(timeout=8)
                        except subprocess.TimeoutExpired:
                            os.killpg(process.pid, signal.SIGKILL)
                            process.wait()
    print(json.dumps({"status": "passed", "variant": variant, "native_files": len(expected["native_paths"]),
                      "debug_checks": debug_checks, "template_cases": cases, "actual_entrypoint": True,
                      "vlan_readiness_gate": True, "container_startup_state": True, "advertisement": packet_result,
                      "private_redis": True, "host_network_used": False, "host_services_used": False}))


if __name__ == "__main__":
    main()
