#!/usr/bin/env python3
"""Validate the staged P4RT image contract and prove configuration invalidation."""

import argparse
import importlib.util
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import artifact_validation as check
from tools.bazel.ci import base_images, build, p4rt_handoff, p4rt_image
from tools.bazel.oci import image_validation, oci_layout, source_base_validation

IMAGES = p4rt_image.IMAGES
SYMBOLS = 'p4rt_with_symbols'
STAGES = {'runtime': IMAGES[0], 'symbols': SYMBOLS, 'debug': IMAGES[1]}
CONFIGURATION = {
    'start.sh': ('usr/bin/start.sh', 0o755),
    'p4rt.sh': ('usr/bin/p4rt.sh', 0o755),
    'p4rt_vars.j2': ('usr/share/sonic/templates/p4rt_vars.j2', 0o644),
    'supervisord.conf': ('etc/supervisor/conf.d/supervisord.conf', 0o644),
    'critical_processes': ('etc/supervisor/critical_processes', 0o644),
}


def clean_source(directory):
    return p4rt_image.source_identity(directory)


def source_identity(directory):
    return {'revision': clean_source(directory)}


def read_json(path):
    return json.loads(path.read_text())


def verify_payload(files, expected, description):
    for name, item in expected.items():
        if item['kind'] != 'directory':
            check.require(all(files.get(name, {}).get(key) == value for key, value in item.items()),
                          description + ': ' + name)


def verify_debug_sai(runtime, symbols, debug, metadata, payloads, hashes, owner):
    """Preserve legacy debug-only SAI Redis and its exact source ELF closure."""
    expected = {
        'libsairedis': ('//lib:libsairedis_pkg', {'libsairedis.so.0.0.0'}),
        'libsaimetadata': ('//meta:libsaimetadata_pkg', {'libsaimeta.so.0.0.0', 'libsaimetadata.so.0.0.0'}),
    }
    check.require(owner.get('schema') == 1 and owner.get('module') == 'sonic-sairedis' and
                  owner.get('architecture') == 'amd64' and set(owner.get('packages', {})) == set(expected),
                  'owner metadata must describe only the two SAI library packages')
    records = {item['package']: item for item in metadata['source_packages']}
    check.require(len(records) == len(metadata['source_packages']), 'duplicate debug source package')
    for package, (target, native_names) in expected.items():
        payload = payloads[package]
        native = {name for name, item in payload.items() if 'elf_machine' in item}
        check.require(native == {'usr/lib/x86_64-linux-gnu/' + name for name in native_names},
                      'debug SAI native inventory differs: ' + package)
        verify_payload(debug, payload, 'debug SAI source payload changed')
        check.require(not any(name in stage for stage in (runtime, symbols)
                              for name, item in payload.items() if item['kind'] != 'directory'),
                      'debug-only SAI package leaked into an earlier stage: ' + package)
        definition = owner['packages'][package]
        check.require(definition.get('runtime_target') == target and
                      definition.get('payload_sha256') == hashes[package],
                      'owner library metadata differs from retained source TAR: ' + package)
        record = records.get(package, {})
        # Installed metadata records Bazel's canonical external label; the
        # owner's package contract records the target within its own repository.
        check.require(record.get('target') == '@@sonic-sairedis+' + target and record.get('source_revision') and
                      record.get('payload_sha256') == hashes[package] and
                      record.get('version') == definition['control_fields']['Version'],
                      'missing source-owned debug package metadata: ' + package)
        rows = record.get('native_files', [])
        check.require(len(rows) == len(native) and {row['path'] for row in rows} == native,
                      'debug SAI metadata native inventory differs: ' + package)
        for row in rows:
            check.require(all(row[key] == payload[row['path']][key] for key in ('sha256', 'mode', 'uid', 'gid')),
                          'debug SAI metadata changed: ' + row['path'])


def inherited_files(parent, child, replacements=(), *, group=False):
    replacements = set(replacements) | {'var/lib/dpkg/status'}
    if group:
        replacements.add('etc/group')
    for name, item in parent.items():
        if item['kind'] != 'directory' and name not in replacements:
            check.require(child.get(name) == item, 'undeclared inherited file change: ' + name)


def verify_groups(runtime, debug, runtime_rows, debug_rows):
    for rows in (runtime_rows, debug_rows):
        names = [row.split(':', 1)[0] for row in rows]
        check.require(len(names) == len(set(names)), 'duplicate group name')
    expected = runtime_rows + ([] if '_ssh:x:104:' in runtime_rows else ['_ssh:x:104:'])
    check.require(debug_rows == expected, 'debug account merge changed existing group rows')
    check.require(all(debug['etc/group'][key] == runtime['etc/group'][key]
                      for key in ('kind', 'uid', 'gid', 'mode')), 'debug group metadata changed')
    for name in ('etc/passwd', 'etc/gshadow'):
        check.require(debug.get(name) == runtime.get(name), 'debug changed inherited account state: ' + name)


def group_rows(layout):
    files, origins = source_base_validation.inventory(layout)
    with tempfile.TemporaryDirectory(prefix='p4rt-accounts-') as temporary:
        root = Path(temporary)
        source_base_validation.extract_selected(layout, files, origins, {'etc/group'}, root)
        return (root / 'etc/group').read_text().splitlines()


def stage_binding(stage, selection, metadata, *, base_digest, apt_sha, contract_sha=None):
    """Do not allow an APT decision or package state to cross an OCI base."""
    check.require(selection.get('image') == 'docker-sonic-p4rt' and
                  selection.get('group') == ('runtime' if stage == 'runtime' else 'debug'),
                  'wrong APT image/stage: ' + stage)
    for record in (selection, metadata):
        check.require(record.get('base_manifest_digest') == base_digest,
                      'stage uses a different completed OCI base: ' + stage)
        check.require(record.get('dependency_check', {}).get('status') == 'satisfied',
                      'unsatisfied stage package dependencies: ' + stage)
    check.require(metadata.get('architecture') == 'amd64' and
                  metadata.get('apt_selection_sha256') == apt_sha,
                  'package state does not bind the checked APT selection: ' + stage)
    if contract_sha is not None:
        check.require(metadata.get('source_contract_sha256') == contract_sha and
                      not metadata.get('source_modules'),
                      'WORKSPACE package state must bind its real owner contract: ' + stage)


def owner_package_state(metadata, contract, payload):
    check.require(len(contract['packages']) == 1 and len(metadata['source_packages']) == 1,
                  'expected one source-owned package in this stage')
    package, definition = next(iter(contract['packages'].items()))
    record = metadata['source_packages'][0]
    for key, expected in {'package': package, 'version': definition['control_fields']['Version'],
                          'target': definition['target'], 'source_revision': definition['source_revision'],
                          'payload_sha256': definition['payload_sha256']}.items():
        check.require(record.get(key) == expected, 'owner package provenance changed: ' + key)
    expected_native = {name: item for name, item in payload.items() if 'elf_machine' in item}
    rows = record['native_files']
    check.require(len(rows) == len(expected_native) and {row['path'] for row in rows} == set(expected_native),
                  'owner package native inventory changed')
    for row in rows:
        check.require(all(row[key] == expected_native[row['path']][key] for key in ('sha256', 'mode', 'uid', 'gid')),
                      'owner package native metadata changed: ' + row['path'])


def verify_config(config, parent, *, runtime=False, manifest=None):
    expected = dict(parent)
    if runtime:
        expected['Entrypoint'] = ['/usr/local/bin/supervisord']
        # The selected rules_oci resets Cmd when an entrypoint is declared.
        expected.pop('Cmd', None)
    def environment(values):
        pairs = [value.split('=', 1) for value in values]
        check.require(all(len(pair) == 2 for pair in pairs) and
                      len({pair[0] for pair in pairs}) == len(pairs), 'invalid or duplicate image environment')
        return dict(pairs)
    old_env = environment(parent.get('Env', []))
    if runtime:
        old_env['DEBIAN_FRONTEND'] = 'noninteractive'
    check.require(environment(config.get('Env', [])) == old_env, 'undeclared image environment change')
    expected.pop('Env', None)
    actual = {key: value for key, value in config.items() if key != 'Env'}
    if manifest is not None:
        expected['Labels'] = {**parent.get('Labels', {}),
                              'com.azure.sonic.manifest': config.get('Labels', {}).get('com.azure.sonic.manifest')}
        check.require(json.loads(expected['Labels']['com.azure.sonic.manifest']) == manifest,
                      'Make manifest label changed')
    check.require(actual == expected, 'undeclared inherited OCI configuration change')


def owner_debug_api(source, revision):
    p4rt_handoff.source_identity(source, revision)
    path = source / 'p4rt_app/runtime_package_test.py'
    spec = importlib.util.spec_from_file_location('selected_p4rt_package_test', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    check.require(len(module.PROGRAMS) == 5, 'expected the five production P4RT programs')
    return module, path


def verify_p4rt_symbols(directory, owner_source, revision, runtime_files, debug_files, debug_layout):
    """Use the owner's actual DWP lookup; do not apply GNU-debuglink rules."""
    api, api_path = owner_debug_api(owner_source, revision)
    runtime_tar = directory / 'owner-output/p4rt_runtime.tar'
    debug_tar = directory / 'owner-output/p4rt_runtime_debug.tar'
    expected_runtime = check.payload(runtime_tar)
    expected_debug = check.payload(debug_tar)
    verify_payload(runtime_files, expected_runtime, 'installed P4RT runtime changed')
    verify_payload(debug_files, expected_runtime, 'debug image changed P4RT runtime')
    verify_payload(debug_files, expected_debug, 'installed P4RT DWP changed')
    check.require(not any(name in runtime_files for name, item in expected_debug.items()
                          if item['kind'] != 'directory'), 'DWP leaked into runtime image')
    results = {}
    with tempfile.TemporaryDirectory(prefix='p4rt-dwp-', dir=directory) as temporary:
        work = Path(temporary)
        runtime, symbols, installed = [work / name for name in ('runtime', 'symbols', 'installed')]
        for path in (runtime, symbols, installed):
            path.mkdir()
        # The owner checks exact paths, regular-file types, modes, timestamps,
        # root ownership, duplicates and the complete five-program inventory.
        api.unpack(runtime_tar, runtime, '', 0o755)
        api.unpack(debug_tar, symbols, '.dwp', 0o644)
        names = {'usr/local/bin/' + name + suffix for name in api.PROGRAMS for suffix in ('', '.dwp')}
        files, origins = source_base_validation.inventory(debug_layout)
        source_base_validation.extract_selected(debug_layout, files, origins, names, installed)
        for name, (source, _) in api.PROGRAMS.items():
            binary, dwp = installed / 'usr/local/bin' / name, installed / 'usr/local/bin' / (name + '.dwp')
            check.require(check.sha(binary) == check.sha(runtime / name) and
                          check.sha(dwp) == check.sha(symbols / (name + '.dwp')),
                          'OCI extraction changed owner bytes: ' + name)
            check.require(runtime_files['usr/local/bin/' + name].get('elf_machine') == 62,
                          'P4RT program is not AMD64: ' + name)
            sections = check.command(['readelf', '-SW', str(dwp)])
            check.require(all(section in sections for section in
                          ('.debug_info.dwo', '.debug_abbrev.dwo', '.debug_cu_index')),
                          'incomplete installed DWP: ' + name)
            isolated = work / name
            isolated.mkdir()
            (isolated / 'empty').mkdir()
            copied = isolated / name
            shutil.copyfile(binary, copied)
            without = api.debug_lookup(copied, isolated)
            check.require(not without['found'], 'loose DWO fallback defeats the missing-DWP control: ' + name)
            (isolated / (name + '.dwp')).symlink_to(dwp)
            found = api.debug_lookup(copied, isolated)
            check.require(found['found'] and found['file'].replace('\\', '/').endswith(source),
                          'matching installed DWP resolves a different source: ' + name)
            lines = (owner_source / source).read_text().splitlines()
            definitions = [number for number, line in enumerate(lines, 1)
                           if re.match(r'\s*int\s+main\s*\(', line)]
            check.require(len(definitions) == 1 and definitions[0] <= found['line'] <= len(lines),
                          'installed DWP main line differs from its owner: ' + name)
            elf = check.elf_info(binary, dynamic=True)
            check.require(elf['build_id'], 'P4RT runtime has no build ID: ' + name)
            results[name] = {**found, 'runtime_path': 'usr/local/bin/' + name,
                             'dwp_path': 'usr/local/bin/' + name + '.dwp',
                             'runtime_sha256': check.sha(binary), 'dwp_sha256': check.sha(dwp),
                             'source_sha256': check.sha(owner_source / source), 'build_id': elf['build_id'],
                             'needed': elf['needed'], 'without_dwp': without, 'with_dwp': found}
    return results, check.sha(api_path)


def validate(args):
    directory = args.directory.resolve()
    receipt = read_json(directory / 'build-receipt.json')
    check.require(receipt.get('status') == 'built' and receipt['revision'] == clean_source(directory),
                  'validation source differs from the completed image build')
    check.require(set(receipt['artifacts']) == set(p4rt_image.TARGETS), 'incomplete exported artifact inventory')
    for name, artifact in receipt['artifacts'].items():
        check.require(check.sha(directory / name) == artifact['sha256'], 'artifact changed: ' + name)
    owner_source = args.owner_source.resolve()
    handoff = p4rt_handoff.verify_stage(ROOT / '.bazel-inputs/p4rt-owner', owner_source,
        receipt['owner_revision'], directory / 'owner-sdk', directory / 'owner-output')
    preparation = read_json(directory / 'owner-preparation.json')
    check.require(preparation.get('status') == 'prepared' and preparation.get('revision') == receipt['revision'] and
                  preparation.get('owner_handoff') == handoff == receipt['owner_handoff'],
                  'image and owner preparation differ')
    for name, item in preparation['artifacts'].items():
        check.require(check.sha(directory / 'owner-preparation' / name) == item['sha256'],
                      'prepared Common input changed: ' + name)
    same_common = p4rt_handoff.same_common_library(directory / 'owner-sdk',
        directory / 'owner-preparation/common-runtime.tar', directory / 'config-engine.source.tar')
    check.require(same_common == handoff['parent_common'] == preparation['parent_common'],
                  'installed parent no longer matches the owner SDK')
    execution_root = build.execution_root_from_bep(directory / 'build-events.jsonl')
    parent = oci_layout.validate_layout(build.resolve_output(execution_root, receipt['base_layout']), 'linux/amd64')
    layouts = {stage: oci_layout.validate_layout(build.resolve_output(execution_root, receipt['layouts'][name]), 'linux/amd64')
               for stage, name in STAGES.items()}
    inventories = {stage: base_images.installed_inventory(layout) for stage, layout in layouts.items()}
    inherited = base_images.installed_inventory(parent)
    configs = check.payload(directory / 'p4rt.files.tar')
    check.require({name for name, item in configs.items() if item['kind'] != 'directory'} ==
                  {name for name, _ in CONFIGURATION.values()}, 'unexpected Docker configuration inventory')
    for source, (name, mode) in CONFIGURATION.items():
        item = configs[name]
        check.require(item['sha256'] == check.sha(ROOT / 'dockers/docker-sonic-p4rt' / source) and
                      (item['kind'], item['uid'], item['gid'], item['mode']) == ('file', 0, 0, mode),
                      'Dockerfile configuration changed: ' + name)
    tools = check.payload(directory / 'p4rt.debug-tools.tar', require_root=False)
    runtime, symbols, debug = [inventories[stage] for stage in STAGES]
    inherited_files(inherited, runtime, configs)
    inherited_files(runtime, symbols)
    inherited_files(symbols, debug, tools, group=True)
    verify_groups(runtime, debug, group_rows(layouts['runtime']), group_rows(layouts['debug']))
    verify_payload(debug, tools, 'debug tools changed')
    check.require(tools.get('usr/bin/vim', {}).get('linkname') == '/etc/alternatives/vim',
                  'Huge Vim must replace the inherited vim-tiny hardlink')
    check.require(debug['usr/bin/ssh-agent']['gid'] == 104 and debug['usr/bin/ssh-agent']['mode'] == 0o2755,
                  'FIPS OpenSSH client permissions differ')
    check.require(runtime[same_common['path']]['sha256'] == same_common['sha256'],
                  'image Common library differs from the P4RT SDK')
    handoff_path = 'usr/share/sonic/bazel/p4rt-handoff.json'
    check.require(runtime.get(handoff_path, {}).get('sha256') ==
                  check.sha(ROOT / '.bazel-inputs/p4rt-owner/handoff.json'), 'installed owner provenance changed')
    stage_reports = {}
    for stage, layout in layouts.items():
        files = inventories[stage]
        base = {'runtime': parent, 'symbols': layouts['runtime'], 'debug': layouts['symbols']}[stage]
        check.require(layout.manifest['layers'][:len(base.manifest['layers'])] == base.manifest['layers'],
                      'image stage does not extend its exact declared base: ' + stage)
        base_images.verify_unique_layer_paths(layout)
        base_images.verify_directory_aliases(files)
        check.require(all(item.get('elf_machine', 62) == 62 for item in files.values()), 'non-AMD64 installed ELF')
        verify_payload(files, configs, 'installed configuration changed')
        apt = directory / (stage + '-apt.json')
        metadata_path = directory / ('p4rt-' + stage + '.packages.json')
        metadata = read_json(metadata_path)
        contract = None if stage == 'debug' else directory / 'owner-output' / (
            'p4rt_runtime' + ('_debug' if stage == 'symbols' else '') + '_contract.json')
        stage_binding(stage, read_json(apt), metadata, base_digest=base.descriptor['digest'],
                      apt_sha=check.sha(apt), contract_sha=check.sha(contract) if contract else None)
        if contract:
            owner_package_state(metadata, read_json(contract), check.payload(
                directory / 'owner-output' / ('p4rt_runtime' + ('_debug' if stage == 'symbols' else '') + '.tar')))
        for path, artifact in (('var/lib/dpkg/status', directory / ('p4rt-' + stage + '.status')),
                               ('usr/share/sonic/bazel/p4rt-' + stage + '-packages.json', metadata_path)):
            check.require(files.get(path, {}).get('sha256') == check.sha(artifact), 'installed package state changed: ' + path)
        config = layout.config.get('config', {})
        manifest = (read_json(ROOT / 'target/bazel-manifests' / STAGES[stage] / 'manifest.json')
                    if stage != 'symbols' else None)
        verify_config(config, base.config.get('config', {}), runtime=stage == 'runtime', manifest=manifest)
        if stage != 'symbols':
            name = STAGES[stage]
            archive = image_validation.docker_archive(directory / (name + '.gz'), name + ':latest',
                layout.manifest['config']['digest'], layout.manifest['layers'])
        else:
            archive = None
        stage_reports[stage] = {'status': 'passed', 'manifest_digest': layout.descriptor['digest'],
                                'base_manifest_digest': base.descriptor['digest'],
                                'config_digest': layout.manifest['config']['digest'],
                                'rootfs': layout.config['rootfs'], 'archive': archive}
        if stage != 'symbols':
            p4rt_image.write(directory / (STAGES[stage] + '.validation.json'), stage_reports[stage])
    owner_runtime = check.payload(directory / 'owner-output/p4rt_runtime.tar')
    check.require(check.sha(directory / 'p4rt.source.tar') == handoff['files']['p4rt_runtime.tar']['sha256'],
                  'source TAR is not the actual owner output')
    owner_debug = check.payload(directory / 'owner-output/p4rt_runtime_debug.tar')
    verify_payload(symbols, owner_debug, 'symbol stage changed owner DWP')
    verify_debug_sai(runtime, symbols, debug, read_json(directory / 'p4rt-debug.packages.json'), {
        'libsairedis': check.payload(directory / 'sairedis.source.tar'),
        'libsaimetadata': check.payload(directory / 'saimetadata.source.tar'),
    }, {
        'libsairedis': check.sha(directory / 'sairedis.source.tar'),
        'libsaimetadata': check.sha(directory / 'saimetadata.source.tar'),
    }, read_json(directory / 'sairedis.library-packages.json'))
    debug_payload = check.payload(directory / 'p4rt.debug.tar')
    verify_payload(debug, debug_payload, 'complete source debug layer changed')
    check.require(not any(name in runtime for name, item in debug_payload.items() if item['kind'] != 'directory'),
                  'detached symbols leaked into the runtime image')
    reports = [read_json(directory / name) for name in
               ('base.packages.json', 'config-engine.packages.json', 'p4rt-debug.packages.json')]
    native = base_images.verify_source_files(debug, reports, 62)
    runtimes = [directory / name for name in ('base.source.tar', 'config-engine.source.tar', 'openssh.source.tar',
                                                     'sairedis.source.tar', 'saimetadata.source.tar')]
    pairs = base_images.verify_symbols(native, runtimes, directory / 'p4rt.native-debug.tar', directory)
    p4rt_pairs, helper_sha = verify_p4rt_symbols(directory, owner_source, receipt['owner_revision'], runtime, debug, layouts['debug'])
    for name, item in owner_runtime.items():
        if 'elf_machine' in item:
            check.require(name not in native, 'duplicate P4RT native source owner')
            native[name] = {**item, 'source_package': 'sonic-p4rt'}
    selected, origins = source_base_validation.inventory(layouts['debug'])
    elfs = []
    with tempfile.TemporaryDirectory(prefix='p4rt-native-', dir=directory) as temporary:
        root = Path(temporary)
        source_base_validation.extract_selected(layouts['debug'], selected, origins, set(native), root)
        for name, item in sorted(native.items()):
            info = check.elf_info(root / name, dynamic=True)
            elfs.append({'path': name, 'needed': info['needed'], 'sha256': item['sha256']})
    check.require(clean_source(directory) == receipt['revision'], 'source changed during static validation')
    result = {'status': 'passed', 'revision': receipt['revision'], 'owner_revision': receipt['owner_revision'],
              'architecture': 'amd64', 'native': native, 'runtime_native': {name: item for name, item in native.items() if name in runtime},
              'elfs': elfs, 'source_debug_pairs': pairs, 'p4rt_debug_pairs': p4rt_pairs,
              'p4rt_owner_test_sha256': helper_sha, 'owner_handoff': handoff,
              'stages': stage_reports, 'artifacts': receipt['artifacts']}
    p4rt_image.write(directory / 'validation.json', result)
    print(json.dumps({'status': 'passed', 'native_pairs': len(pairs), 'p4rt_dwp_pairs': len(p4rt_pairs)}))


def incremental(args):
    directory = args.directory.resolve()
    receipt = read_json(directory / 'build-receipt.json')
    check.require(receipt.get('status') == 'built' and receipt['revision'] == clean_source(directory),
                  'incremental checkout differs from the built source')
    execution_root = build.execution_root_from_bep(directory / 'build-events.jsonl')
    layouts = {name: build.resolve_output(execution_root, path) for name, path in receipt['layouts'].items()}
    base = build.resolve_output(execution_root, receipt['base_layout'])
    options = p4rt_image.root_options()
    outputs = {}
    for name in (*[image + '.gz' for image in IMAGES], 'p4rt.source.tar', 'p4rt.debug.tar'):
        path = subprocess.check_output([args.bazel, 'cquery', *options, '--output=files', p4rt_image.TARGETS[name]],
                                       cwd=ROOT, text=True).strip()
        outputs[name] = build.resolve_output(execution_root, path)
    command = [args.bazel, 'build', *options, p4rt_image.BASE,
               *[p4rt_image.OWNER + name for name in layouts], *[p4rt_image.TARGETS[name] for name in outputs]]
    def snapshot():
        return {'images': {name: oci_layout.validate_layout(path, 'linux/amd64').descriptor['digest'] for name, path in layouts.items()},
                'outputs': {name: check.sha(path) for name, path in outputs.items()},
                'base': oci_layout.validate_layout(base, 'linux/amd64').descriptor['digest'],
                'owner_handoff': check.sha(ROOT / '.bazel-inputs/p4rt-owner/handoff.json')}
    def build_snapshot(stage):
        started = time.monotonic()
        with (directory / ('incremental-' + stage + '.log')).open('w') as log:
            subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)
        return {'seconds': round(time.monotonic() - started, 3), **snapshot()}
    baseline = build_snapshot('baseline')
    no_op = build_snapshot('no-op')
    comparable = lambda value: {key: value[key] for key in ('images', 'outputs', 'base', 'owner_handoff')}
    check.require(comparable(baseline) == comparable(no_op), 'no-op changed P4RT image outputs')
    for name, checksum in baseline['outputs'].items():
        check.require(checksum == receipt['artifacts'][name]['sha256'], 'incremental baseline differs from accepted artifact: ' + name)
    source = ROOT / 'dockers/docker-sonic-p4rt/p4rt_vars.j2'
    original = source.read_bytes()
    try:
        source.write_bytes(original + b'\n{# P4RT image invalidation probe #}\n')
        changed = build_snapshot('configuration-edit')
        check.require(changed['base'] == baseline['base'] and changed['owner_handoff'] == baseline['owner_handoff'],
                      'configuration edit changed the source parent or owner handoff')
        check.require(all(changed['images'][name] != baseline['images'][name] for name in layouts),
                      'configuration edit did not change every dependent image stage')
        for name in outputs:
            check.require((changed['outputs'][name] != baseline['outputs'][name]) == name.endswith('.gz'),
                          'configuration edit changed the wrong artifact: ' + name)
    finally:
        source.write_bytes(original)
        restored = build_snapshot('restored')
    check.require(comparable(restored) == comparable(baseline), 'restoration changed accepted P4RT image bytes')
    check.require(clean_source(directory) == receipt['revision'], 'incremental test did not restore source')
    result = {'status': 'passed', 'revision': receipt['revision'], 'input': str(source.relative_to(ROOT)),
              'baseline': baseline, 'no_op': no_op, 'changed': changed, 'restored': restored,
              'limits': 'Configuration invalidation and exact cache restoration; no fresh compiler reproducibility claim.'}
    p4rt_image.write(directory / 'incremental.json', result)
    print(json.dumps({'status': 'passed', 'timings': {key: value['seconds'] for key, value in result.items() if isinstance(value, dict)}}))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('phase', choices=('validate', 'incremental'))
    parser.add_argument('--directory', required=True, type=Path)
    parser.add_argument('--bazel', default='bazel')
    parser.add_argument('--owner-source', required=True, type=Path)
    args = parser.parse_args()
    {'validate': validate, 'incremental': incremental}[args.phase](args)


if __name__ == '__main__':
    main()
