#!/usr/bin/env python3
"""Exercise installed Scapy and the real tunnel packet handler in private Redis."""

import hashlib
import importlib.metadata
import importlib.util
import json
from pathlib import Path
import sys
import tempfile

import redis
import runtime_smoke
from scapy.layers.inet import IP, UDP
from scapy.layers.inet6 import IPv6
from scapy.sendrecv import AsyncSniffer
from scapy.utils import wrpcap


def require(value, message):
    if not value:
        raise AssertionError(message)


def main():
    report = json.loads(Path("/probe/validation.json").read_text())
    require(report["status"] == "passed" and report["architecture"] == "amd64", "unvalidated image")
    debug = "--debug" in sys.argv
    native = report["native"] if debug else report["runtime_native"]
    for name, item in native.items():
        path = Path("/") / name
        stat = path.stat()
        require(hashlib.sha256(path.read_bytes()).hexdigest() == item["sha256"], "installed source changed: " + name)
        require((stat.st_uid, stat.st_gid, stat.st_mode & 0o7777) == (item["uid"], item["gid"], item["mode"]), "source permissions changed: " + name)
    for name, digest in report["scapy_files"].items():
        require(hashlib.sha256((Path("/") / name).read_bytes()).hexdigest() == digest, "installed Scapy changed: " + name)
    handler = Path("/usr/bin/tunnel_packet_handler.py")
    require(hashlib.sha256(handler.read_bytes()).hexdigest() == report["config_files"][str(handler)[1:]]["sha256"], "installed handler changed")
    require(importlib.metadata.version("scapy") == "2.6.1.dev0", "installed wheel version differs")
    inputs = {"architecture": "amd64", "elfs": [item for item in report["elfs"] if item["path"] in native],
              "debug_pairs": [item for item in report["source_debug_pairs"] if item["path"] in ("usr/bin/orchagent", "usr/bin/countersyncd", "usr/bin/ssh")],
              "policy": {"python_imports": ["swsscommon.swsscommon", "dash_api.utils", "scapy.all"],
                         "commands": [{"argv": ["/usr/local/bin/scapy", "-h"], "output_pattern": "Usage"},
                                      {"argv": ["/usr/bin/traceroute6", "-V"],
                                       "output_pattern": "TCP & UDP IPv6 traceroute tool"},
                                      {"argv": ["/usr/bin/update-alternatives", "--query", "traceroute6"],
                                       "output_pattern": "Value: /usr/bin/rltraceroute6"}]}}
    if debug:
        inputs["policy"]["commands"] += [
            {"argv": ["/usr/bin/ssh", "-V"], "output_pattern": "OpenSSH"},
            {"argv": ["/usr/bin/ssh", "-G", "-F", "/dev/null", "fixture.invalid"],
             "output_pattern": "hostname fixture.invalid"},
            {"argv": ["/usr/bin/sshpass", "-V"], "output_pattern": "sshpass"},
            {"argv": ["/usr/bin/vim", "--version"], "output_pattern": "Huge version"},
        ]
    result = runtime_smoke.probe(inputs, "debug" if debug else "runtime")
    require(result["passed"], json.dumps(result))
    database = Path("/run/redis/sonic-db/database_config.json")
    database.parent.mkdir(parents=True, exist_ok=True)
    database.write_bytes(Path("/probe/redis/database_config.json").read_bytes())
    socket_path = json.loads(database.read_text())["INSTANCES"]["redis"]["unix_socket_path"]
    executable = Path("/probe/redis/usr/bin/redis-server")
    fixture = {"executable": str(executable), "sha256": hashlib.sha256(executable.read_bytes()).hexdigest(),
               "database_config": str(database), "loopback_tcp": True}
    with runtime_smoke.redis_fixture(fixture), tempfile.TemporaryDirectory(prefix="orchagent-packets-") as temporary:
        clients = {number: redis.Redis(unix_socket_path=socket_path, db=number, decode_responses=True, socket_timeout=1) for number in (4, 6)}
        cfg, state = clients[4], clients[6]
        cfg.set("CONFIG_DB_INITIALIZED", "1")
        cfg.hset("PORTCHANNEL_INTERFACE|PortChannel1|192.0.2.1/31", mapping={"NULL": "NULL"})
        cfg.hset("TUNNEL|fixture", mapping={"tunnel_type": "ipinip", "dst_ip": "192.0.2.10"})
        cfg.hset("PEER_SWITCH|fixture", mapping={"address_ipv4": "192.0.2.11"})
        state.hset("INTERFACE_TABLE|PortChannel1|192.0.2.1/31", mapping={"state": "ok"})
        spec = importlib.util.spec_from_file_location("installed_tunnel_packet_handler", handler)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        service = module.TunnelPacketHandler()
        try:
            require(service.portchannel_intfs == [("PortChannel1", "192.0.2.1/31")], "handler did not read private CONFIG_DB")
            require(service.all_portchannels_established(), "handler did not read private STATE_DB")
            require(service.get_ipinip_tunnel_addrs() == ("192.0.2.10", "192.0.2.11"), "tunnel identity differs")
            service.self_ip = "192.0.2.10"
            packets = [IP(bytes(packet)) for packet in (
                IP(src="192.0.2.11", dst=service.self_ip) / IP(dst="198.51.100.7") / UDP(),
                IP(src="192.0.2.11", dst=service.self_ip) / IPv6(dst="2001:db8::7") / UDP(),
                IP(dst="192.0.2.99") / IP(dst="198.51.100.8"),
                IP(dst=service.self_ip) / UDP(),
            )]
            require(service.get_inner_pkt_type(packets[0]) is IP and service.get_inner_pkt_type(packets[1]) is IPv6,
                    "installed handler did not classify reparsed IPv4/IPv6 tunnel packets")
            capture = Path(temporary) / "packets.pcap"
            wrpcap(str(capture), packets)
            sniffer = AsyncSniffer(offline=str(capture), prn=service.ping_inner_dst, store=False)
            sniffer.start()
            sniffer.join(timeout=10)
            require(not sniffer.running, "offline AsyncSniffer did not finish")
            commands = []
            while not service.pending_cmds.empty():
                commands.append(service.pending_cmds.get_nowait())
            prefix = ["timeout", "0.2", "ping", "-c1", "-W1", "-i0", "-n", "-q"]
            require(commands == [prefix + ["198.51.100.7"], prefix + ["-6", "2001:db8::7"]], "handler queued unexpected commands")
            print(json.dumps({"status": "passed", "scapy_version": importlib.metadata.version("scapy"), "source_elf_count": len(native),
                              "loader_checks": len(result["loader_checks"]), "debug_checks": len(result["debug_pairs"]),
                              "source_openssh_client": debug,
                              "private_redis": True, "actual_tunnel_handler": True, "offline_async_sniffer": True,
                              "packets_decoded": len(packets), "expected_ping_requests": len(commands),
                              "scope": "Installed source libraries and actual tunnel handler/Scapy callback against private Redis and offline packet bytes; no ping execution, live capture, ASIC, FIPS POST or installer claim."}))
        finally:
            service.netlink_api.close()
            for client in clients.values():
                client.close()


if __name__ == "__main__":
    main()
