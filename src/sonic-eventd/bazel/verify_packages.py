#!/usr/bin/env python3
"""Check installed paths, payload bytes and the four native debug-symbol pairs."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tarfile
import tempfile
import zlib


PACKAGES = {
    'eventd': {
        'usr/bin/eventd': None, 'usr/bin/eventdb': None, 'usr/bin/events_tool': None,
        'usr/bin/events_publish_tool.py': 'tools/events_publish_tool.py',
        'etc/eventd.json': 'etc/eventd.json', 'etc/evprofile/default.json': 'var/evprofile/default.json',
    },
    'rsyslog_plugin': {
        'usr/bin/rsyslog_plugin': None, 'usr/bin/events_monit_test.py': 'tools/events_monit_test.py',
        'etc/monit/conf.d/monit_events': 'tools/monit_events',
    },
}


def read_archive(path):
    result = {}
    with tarfile.open(path) as archive:
        for member in archive:
            if member.isdir():
                continue
            name = member.name.removeprefix('./')
            if not member.isfile() or name.startswith('/') or '..' in Path(name).parts or name in result:
                raise ValueError('Unexpected archive member: ' + name)
            if member.uid != 0 or member.gid != 0:
                raise ValueError('Payload is not owned by root: ' + name)
            result[name] = (archive.extractfile(member).read(), member.mode)
    return result


def readelf(path, *args):
    return subprocess.check_output(['readelf', *args, str(path)], text=True)


def build_id(path):
    ids = re.findall(r'Build ID: ([0-9a-f]+)', readelf(path, '-n'))
    if len(ids) != 1:
        raise ValueError('Expected one ELF build ID: ' + str(path))
    return ids[0]


def verify(root, archives, architecture):
    result = {'architecture': architecture, 'packages': {}, 'pairs': []}
    machine = {'amd64': 'Advanced Micro Devices X86-64', 'arm64': 'AArch64'}[architecture]
    with tempfile.TemporaryDirectory(prefix='eventd-packages-') as temp:
        directory = Path(temp)
        for package, (runtime_tar, debug_tar) in archives.items():
            runtime, debug = read_archive(runtime_tar), read_archive(debug_tar)
            expected = PACKAGES[package]
            manifest = root/'debian'/('sonic-eventd.install' if package == 'eventd' else 'sonic-rsyslog-plugin.install')
            if set(manifest.read_text().splitlines()) != set(expected):
                raise ValueError('Update the Bazel payload for the changed Debian install manifest: ' + str(manifest))
            if set(runtime) != set(expected):
                raise ValueError('Installed payload differs: ' + repr(set(runtime) ^ set(expected)))
            debug_used = set()
            for name, source in expected.items():
                content, mode = runtime[name]
                if mode != (0o755 if name.startswith('usr/bin/') else 0o644):
                    raise ValueError('Wrong installed mode: ' + name)
                if source:
                    if content != (root/source).read_bytes():
                        raise ValueError('Installed configuration/script differs: ' + name)
                    continue
                binary = directory/Path(name).name; binary.write_bytes(content)
                headers = readelf(binary, '-h')
                if machine not in headers or not re.search(r'Type:\s+DYN', headers):
                    raise ValueError('Binary platform or PIE differs: ' + name)
                sections = readelf(binary, '-S', '--wide')
                if '.debug_info' in sections or '.gnu_debuglink' not in sections:
                    raise ValueError('Runtime binary is not correctly split: ' + name)
                identifier = build_id(binary)
                symbol_path = 'usr/lib/debug/.build-id/' + identifier[:2] + '/' + identifier[2:] + '.debug'
                symbols, unused_mode = debug[symbol_path]; debug_used.add(symbol_path)
                symbol = directory/(binary.name + '.debug'); symbol.write_bytes(symbols)
                if build_id(symbol) != identifier or '.debug_info' not in readelf(symbol, '-S', '--wide'):
                    raise ValueError('Matching DWARF symbols are missing: ' + name)
                debuglink = directory/(binary.name + '.debuglink')
                subprocess.run(['objcopy', '--dump-section', '.gnu_debuglink=' + str(debuglink), str(binary)], check=True, capture_output=True)
                link = debuglink.read_bytes(); end = link.index(b'\0'); offset = (end + 4) & ~3
                if link[:end].decode() != Path(symbol_path).name or int.from_bytes(link[offset:offset+4], 'little') != zlib.crc32(symbols):
                    raise ValueError('GNU debuglink does not identify the packaged symbols: ' + name)
                result['pairs'].append({'path':name,'build_id':identifier,'debug_path':symbol_path,
                                        'runtime_sha256':hashlib.sha256(content).hexdigest(),
                                        'debug_sha256':hashlib.sha256(symbols).hexdigest()})
            if set(debug) != debug_used:
                raise ValueError('Unexpected debug payload: ' + repr(set(debug) ^ debug_used))
            result['packages'][package] = {'runtime_sha256':hashlib.sha256(runtime_tar.read_bytes()).hexdigest(),
                                           'debug_sha256':hashlib.sha256(debug_tar.read_bytes()).hexdigest(),
                                           'installed_paths':sorted(runtime)}
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--architecture', choices=['amd64','arm64'], required=True)
    parser.add_argument('--output', type=Path, required=True)
    for name in ['eventd-runtime','eventd-debug','rsyslog-runtime','rsyslog-debug']:
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    result = verify(Path(__file__).resolve().parents[1], {
        'eventd':(args.eventd_runtime,args.eventd_debug),
        'rsyslog_plugin':(args.rsyslog_runtime,args.rsyslog_debug)}, args.architecture)
    result['_generated'] = {'notice':'AUTO-GENERATED. DO NOT EDIT MANUALLY.','generator':'src/sonic-eventd/bazel/verify_packages.py'}
    args.output.write_text(json.dumps(result, indent=2, sort_keys=True)+'\n')
    print('Verified four position-independent programs, matching split symbols and both install manifests')


if __name__ == '__main__':
    main()
