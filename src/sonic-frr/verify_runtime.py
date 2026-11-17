#!/usr/bin/env python3
"""Verify deployed FRR/SNMP ELF payloads and their matching split symbols."""

import argparse
import json
from pathlib import Path
import re
import shutil
import sys
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import artifact_validation as artifacts
from tools.bazel.oci.runtime_smoke import gdb_lookup


def verify(runtime, symbols, package):
    payload = artifacts.payload(runtime)
    debug = artifacts.payload(symbols)
    documentation = 'usr/share/doc/' + package + '-dbgsym'
    artifacts.require(debug[documentation]['kind'] == 'symlink' and
                      debug[documentation]['linkname'] == package,
                      'incorrect debug documentation link: ' + documentation)
    artifacts.require(all(item['kind'] == 'directory' or name == documentation or
                          (item['kind'] == 'file' and re.fullmatch(
                              r'usr/lib/debug/\.build-id/[0-9a-f]{2}/[0-9a-f]+\.debug', name))
                          for name, item in debug.items()), 'unexpected detached-symbol payload')
    combined = dict(payload)
    for name, item in debug.items():
        artifacts.require(name not in combined or combined[name] == item,
                          'debug archive changes runtime member: ' + name)
        combined[name] = item
    binaries = {name: item for name, item in payload.items() if 'elf_machine' in item}
    artifacts.require(binaries and all(item.get('elf_machine', 62) == 62 for item in combined.values()),
                      'expected native AMD64 ELF payloads')
    for name, item in binaries.items():
        mode = 0o755 if name.startswith(('usr/bin/', 'usr/lib/frr/')) else 0o644
        artifacts.require(item['mode'] == mode, 'incorrect installed ELF mode: ' + name)
    with tempfile.TemporaryDirectory(prefix='frr-installed-') as directory:
        root = Path(directory)
        for archive in (runtime, symbols):
            with tarfile.open(archive) as source:
                source.extractall(root, filter='data')
        pairs, gaps = artifacts.elf_debug(root, combined, set())
        identity = {name: artifacts.elf_info(root / name, dynamic=True) for name in binaries}
        result = {'runtime_sha256': artifacts.sha(runtime), 'symbols_sha256': artifacts.sha(symbols),
                  'elfs': identity, 'debug_pairs': pairs, 'prebuilt_debug_gaps': gaps}
        if package == 'frr':
            sonic = 'usr/lib/x86_64-linux-gnu/frr/modules/dplane_fpm_sonic.so'
            artifacts.require(identity[sonic]['soname'] == 'dplane_fpm_sonic.so',
                              'incorrect SONiC FPM module SONAME')
            # The selected source includes libFIB's public header but makes no
            # API calls. Preserve actual loader requirements instead of forcing
            # an unused library into DT_NEEDED just because Make lists it.
            result['sonic_fpm_libfib_needed'] = 'libnexthopgroup.so.0' in identity[sonic]['needed']
            binary = root / 'usr/lib/frr/bgpd'
            isolated = root / 'isolated'
            isolated.mkdir()
            shutil.copyfile(binary, isolated / 'bgpd')
            empty = root / 'empty-symbols'
            empty.mkdir()
            without = gdb_lookup(isolated / 'bgpd', root, str(empty), 'main')
            artifacts.require(not without.get('found'), 'symbol lookup escaped its isolated directory')
            with_symbols = gdb_lookup(binary, root, str(root / 'usr/lib/debug'), 'main')
            artifacts.require(with_symbols.get('found') and
                              with_symbols.get('source', '').endswith('bgpd/bgp_main.c'),
                              'packaged BGPD symbols do not resolve owner source')
            result.update(with_symbols=with_symbols, without_symbols=without)
        return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('runtime', 'symbols', 'snmp-runtime', 'snmp-symbols', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    report = {'scope': 'Native AMD64 installed ELF/symbol checks; no daemon, container or installer run',
              'frr': verify(args.runtime, args.symbols, 'frr'),
              'snmp': verify(args.snmp_runtime, args.snmp_symbols, 'frr-snmp')}
    args.output.write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
