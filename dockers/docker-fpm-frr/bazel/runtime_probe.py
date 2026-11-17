#!/usr/bin/env python3
"""Exercise installed FRR and Python interfaces against private local services."""

import argparse
import grp
import hashlib
import importlib
import importlib.metadata
import json
from pathlib import Path
import pwd
import re
import subprocess
import tempfile
import time

import redis
from swsscommon import swsscommon


def require(value, message):
    if not value:
        raise AssertionError(message)


def command(*args, env=None):
    result = subprocess.run(args, env=env, capture_output=True, text=True, timeout=30)
    require(result.returncode == 0, result.stdout + result.stderr)
    return result.stdout + result.stderr


def installed_python():
    for name, expected in (("sonic-bgpcfgd", {"bgpcfgd", "staticroutebfd", "bgpmon", "bfdmon"}),
                           ("sonic-frr-mgmt-framework", {"frrcfgd"})):
        distribution = importlib.metadata.distribution(name)
        require(distribution.version == "1.0", "wrong installed wheel: " + name)
        entries = {item.name: item for item in distribution.entry_points if item.group == "console_scripts"}
        require(set(entries) == expected, "entrypoint set differs: " + name)
        for entry in entries.values():
            module = importlib.import_module(entry.module)
            require(Path(module.__file__).resolve().is_relative_to('/usr/local/lib/python3.13/dist-packages'),
                    "import escaped installed wheel: " + str(module.__file__))
            require(callable(entry.load()), "unloadable entrypoint: " + entry.name)
    from bgpcfgd.directory import Directory
    from bgpcfgd.managers_bgp import is_bgp_identifier_valid
    from bgpcfgd.template import TemplateFabric
    from frrcfgd.frrcfgd import CachedDataWithOp, extract_cmd_daemons
    from jinja2 import Environment, FileSystemLoader
    directory = Directory()
    notifications = []
    directory.subscribe([("CONFIG_DB", "BGP_NEIGHBOR", "192.0.2.1")], lambda: notifications.append("changed"))
    directory.put("CONFIG_DB", "BGP_NEIGHBOR", "192.0.2.1", {"asn": "64512"})
    require(notifications == ["changed"], "installed BGP Directory did not notify")
    directory.remove("CONFIG_DB", "BGP_NEIGHBOR", "192.0.2.1")
    require(not directory.path_exist("CONFIG_DB", "BGP_NEIGHBOR", "192.0.2.1"), "BGP Directory removal failed")
    require(is_bgp_identifier_valid("192.0.2.1") and not is_bgp_identifier_valid("invalid address"), "BGP address validation changed")
    rendered = TemplateFabric().from_file("common/daemons.common.conf.j2").render(
        DEVICE_METADATA={"localhost": {"hostname": "bazel-frr-image"}})
    require("hostname bazel-frr-image" in rendered, "installed container template failed")
    require(extract_cmd_daemons("[bgpd,zebra]router bgp 64512") == (["bgpd", "zebra"], "router bgp 64512"), "FRR daemon routing changed")
    require(repr(CachedDataWithOp("peer", CachedDataWithOp.OP_UPDATE)) == "(peer, UPDATE)", "FRR cached update changed")
    environment = Environment(loader=FileSystemLoader('/usr/local/sonic/frrcfgd'))
    bfd = environment.get_template("bfdd.conf.j2").render(BFD_PEER_SINGLE_HOP={
        ("192.0.2.1", "Ethernet0", "default", "192.0.2.2"): {
            "enabled": "true", "desired-minimum-tx-interval": "500"}})
    require("peer 192.0.2.1 interface Ethernet0 vrf default local-address 192.0.2.2" in bfd and
            "transmit-interval 500" in bfd and "no shutdown" in bfd, "installed BFD wheel data failed")
    return {"entrypoints": 5, "container_template": "common/daemons.common.conf.j2", "wheel_template": "bfdd.conf.j2"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--debug', action='store_true')
    parser.add_argument('--validation', type=Path, required=True)
    args = parser.parse_args()
    user = pwd.getpwnam('frr')
    require((user.pw_uid, user.pw_gid, user.pw_dir, user.pw_shell) == (300,300,'/home/frr','/bin/false'), 'FRR account differs')
    require(grp.getgrnam('frrvty').gr_gid == 102 and 'frr' in grp.getgrnam('frrvty').gr_mem, 'FRR VTY group differs')
    python = installed_python()
    report = json.loads(args.validation.read_text())
    require(report['status'] == 'passed' and report['architecture'] == 'amd64', 'unvalidated image report')
    native = report['native'] if args.debug else report['runtime_native']
    critical = {'usr/lib/frr/bgpd', 'usr/lib/frr/zebra', 'usr/bin/vtysh',
                'usr/lib/x86_64-linux-gnu/frr/modules/bgpd_snmp.so',
                'usr/lib/x86_64-linux-gnu/frr/modules/dplane_fpm_sonic.so'}
    require(critical <= native.keys(), 'native closure omits an FRR program or module')
    for name, item in sorted(native.items()):
        path = Path('/') / name
        require(hashlib.sha256(path.read_bytes()).hexdigest() == item['sha256'],
                'installed source ELF differs from validated owner: ' + name)
        output = command('/lib64/ld-linux-x86-64.so.2', '--list', str(path))
        require('not found' not in output, 'missing installed runtime library: ' + name)
    processes, logs = [], []
    with tempfile.TemporaryDirectory(prefix='frr-image-') as temporary:
        work = Path(temporary)
        def start(name, argv):
            stream = (work / (name + '.log')).open('w+')
            logs.append(stream)
            process = subprocess.Popen(argv, cwd=work, stdout=stream, stderr=subprocess.STDOUT)
            processes.append(process)
            return process
        def diagnostics():
            return '\n'.join(path.name + ':\n' + path.read_text() for path in work.glob('*.log'))
        try:
            fixture = Path('/probe/redis')
            database_socket = work / 'redis.sock'
            start('redis', [str(fixture/'usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2'),
                  '--library-path', str(fixture/'usr/lib/x86_64-linux-gnu'), str(fixture/'usr/bin/redis-server'),
                  '--port', '0', '--unixsocket', str(database_socket), '--save', '', '--appendonly', 'no'])
            client = redis.Redis(unix_socket_path=str(database_socket), db=4, socket_timeout=1)
            deadline = time.monotonic() + 15
            while True:
                try:
                    if client.ping(): break
                except redis.RedisError:
                    require(time.monotonic() < deadline, diagnostics())
                    time.sleep(.1)
            client.hset('BGP_NEIGHBOR|192.0.2.1', mapping={'asn':'64513','name':'bazel-peer'})
            connection = swsscommon.DBConnector(4, str(database_socket), 0)
            found, fields = swsscommon.Table(connection, 'BGP_NEIGHBOR').get('192.0.2.1')
            require(found and dict(fields) == {'asn':'64513','name':'bazel-peer'}, 'installed Common/Redis table exchange failed')
            config = work / 'bgpd.conf'
            config.write_text('hostname bazel-frr-image\nrouter bgp 64512\n bgp router-id 192.0.2.2\n neighbor 192.0.2.1 remote-as 64513\n')
            vty = work / 'vty'; vty.mkdir()
            # No privilege changes, Zebra connection, kernel updates, BGP
            # listener or TCP VTY listener. The Unix VTY still serves real CLI.
            bgpd = start('bgpd', ['/usr/lib/frr/bgpd', '-S', '-Z', '-n', '-p', '0', '-P', '0',
                         '-M', 'snmp', '-f', str(config), '-i', str(work/'bgpd.pid'), '--vty_socket', str(vty)])
            deadline = time.monotonic() + 30
            while not (vty / 'bgpd.vty').exists():
                require(bgpd.poll() is None and time.monotonic() < deadline, diagnostics())
                time.sleep(.1)
            # The socket can appear before the daemon has applied its config.
            while True:
                result = subprocess.run(['/usr/bin/vtysh', '--vty_socket', str(vty), '-d', 'bgpd',
                    '-c', 'show bgp summary json'], capture_output=True, text=True, timeout=5)
                try:
                    summary = json.loads(result.stdout)
                    ipv4 = summary.get('ipv4Unicast', {})
                except (ValueError, AttributeError):
                    summary, ipv4 = {}, {}
                if (result.returncode == 0 and ipv4.get('routerId') == '192.0.2.2' and
                        ipv4.get('as') == 64512 and '192.0.2.1' in ipv4.get('peers', {})):
                    break
                require(bgpd.poll() is None and time.monotonic() < deadline,
                        result.stdout + result.stderr + diagnostics())
                time.sleep(.1)
            mapped = Path('/proc') / str(bgpd.pid) / 'maps'
            require('/usr/lib/x86_64-linux-gnu/frr/modules/bgpd_snmp.so' in mapped.read_text(),
                    'BGPD did not map the installed source SNMP plugin: ' + diagnostics())
            running = command('/usr/bin/vtysh', '--vty_socket', str(vty), '-d', 'bgpd', '-c', 'show running-config')
            require('router bgp 64512' in running and 'neighbor 192.0.2.1 remote-as 64513' in running, running)
            log = diagnostics()
            require(not re.search(r'(cannot|failed to|unable to) load.*snmp', log, re.I), log)
            if args.debug:
                outputs = []
                for symbols in ('/usr/lib/debug', str(work/'empty-symbols')):
                    result = subprocess.run(['gdb','-nx','-nh','--batch','-iex','set auto-load off',
                        '-iex','set debuginfod enabled off','-iex','set debug-file-directory '+symbols,
                        '-ex','file /usr/lib/frr/bgpd','-ex','info line main','-ex','ptype main'],
                        capture_output=True,text=True,timeout=30)
                    require(result.returncode in (0,1), result.stderr)
                    outputs.append(result.stdout+result.stderr)
                require('bgpd/bgp_main.c' in outputs[0] and re.search(r'Line [0-9]+ of', outputs[0]) and 'type = ' in outputs[0], outputs)
                require(not re.search(r'Line [0-9]+ of', outputs[1]), outputs)
            print(json.dumps({'status':'passed','python':python,'common_redis':True,'bgp_summary':summary,
                'source_elf_count':len(native),'debug_lookup':args.debug,'scope':'Private BGPD/VTY and Redis; no Zebra, kernel routing, network peers or full SONiC startup'}))
        finally:
            for process in reversed(processes):
                if process.poll() is None: process.terminate()
                try: process.wait(timeout=5)
                except subprocess.TimeoutExpired: process.kill(); process.wait(timeout=5)
            for stream in logs: stream.close()
            if 'client' in locals(): client.close()


if __name__ == '__main__':
    main()
