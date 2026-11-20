#!/usr/bin/env python3
"""Select public P4RT outputs without publishing private SDK/runtime paths."""

import argparse
import json
from pathlib import Path
import shutil
import sys
import xml.etree.ElementTree as ElementTree

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import artifact_validation as check
from tools.bazel.ci.public_artifacts import (
    REVISION, bep_summary, dependency_json, digest, integer, label, load_json, relative_name, safe_json,
)
from tools.bazel.ci.p4rt_image import PREPARATION_TARGETS
from tools.bazel.ci.python_packages import test_result


def sdk_preparation_summary(directory, source):
    """Bind the declared SDK inputs to their audit, selected parent and owner inventory.

    Project metadata only: commands, environments and private build paths remain
    in the original receipts. Native preparation owns the TAR byte comparison.
    """
    originals = {}

    def read(name):
        path = directory / name
        check.require(path.is_file() and not path.is_symlink() and
                      path.resolve().is_relative_to(directory.resolve()) and
                      0 < path.stat().st_size <= 2 * 1024**2,
                      'Missing or unsafe SDK evidence: ' + name)
        originals[name] = check.sha(path)
        return load_json(path.read_text())

    prepared = read('owner-preparation.json')
    audit = read('owner-preparation/action-audit.json')
    sdk = read('owner-sdk/sdk-build.json')
    check.require(integer(prepared['schema']) == 1 and prepared['status'] == 'prepared' and
                  prepared['architecture'] == source['architecture'] == sdk['architecture'] == 'amd64' and
                  prepared['owner_revision'] == source['commit'] and source['bazel'] == '6.4.0',
                  'SDK preparation differs from the built owner')
    for revision in (prepared['revision'], source['commit'], sdk['common_source_commit']):
        check.require(isinstance(revision, str) and REVISION.fullmatch(revision), 'Invalid SDK source revision')
    artifacts = prepared['artifacts']
    check.require(set(artifacts) == set(PREPARATION_TARGETS), 'SDK preparation requires exactly ten declared inputs')
    inputs = {}
    for name, target in PREPARATION_TARGETS.items():
        item = artifacts[name]
        check.require(label(item['target']) == target and integer(item['bytes']) > 0,
                      'SDK preparation input differs from its declared target')
        inputs[name] = {'target': target, 'bytes': item['bytes'], 'sha256': digest(item['sha256'])}
    targets = audit['targets']
    check.require(audit == prepared['action_audit'] and integer(audit['schema']) == 1 and
                  isinstance(targets, list) and len(targets) == len(set(targets)) == 10 and
                  set(targets) == set(PREPARATION_TARGETS.values()) and
                  audit['deb_outputs'] == [] and audit['packaging_wrappers'] == [],
                  'SDK preparation needs the exact ten-root no-DEB audit')
    audit_view = {key: integer(audit[key]) for key in ('action_count', 'output_count')}
    audit_view.update(schema=1, targets=targets, deb_outputs=[], packaging_wrappers=[])
    inventory = source['sdk_inventory']
    handoff = prepared['owner_handoff']
    check.require(handoff['source_revision'] == source['commit'] and handoff['sdk_inventory'] == inventory and
                  inventory['sdk-build.json']['kind'] == 'file' and
                  digest(inventory['sdk-build.json']['sha256']) == originals['owner-sdk/sdk-build.json'],
                  'SDK metadata differs from the built owner inventory')
    parent = prepared['parent_common']
    parent_sha = inputs['config-engine.source.tar']['sha256']
    check.require(parent == handoff['parent_common'] and parent['parent_source_tar_sha256'] == parent_sha and
                  parent['common_runtime_tar_sha256'] == inputs['common-runtime.tar']['sha256'] and
                  inventory['lib/libswsscommon.so.0']['kind'] == 'file' and
                  digest(parent['sha256']) == inventory['lib/libswsscommon.so.0']['sha256'],
                  'SDK Common library differs from the selected parent linkage')
    check.require(sdk['kind'] == 'sonic-swss-common-bazel-sdk' and sdk['producer_bazel_version'] == '8.5.1' and
                  sdk['producer_target'] == PREPARATION_TARGETS['common-sdk.tar'] and
                  sdk['archive_sha256'] == inputs['common-sdk.tar']['sha256'] and
                  sdk['common_module_sha256'] == inputs['common.MODULE.bazel']['sha256'] and
                  sdk['image_source_revision'] == prepared['revision'] and
                  sdk['yang_modules'] is True and sdk['common_source_has_tracked_changes'] is False,
                  'SDK producer metadata differs from the declared inputs')
    sdk_view = {key: sdk[key] for key in (
        'kind', 'architecture', 'producer_bazel_version', 'producer_target', 'archive_sha256',
        'common_source_commit', 'common_source_has_tracked_changes', 'common_module_sha256',
        'image_source_revision', 'yang_modules')}
    prefix = 'usr/lib/x86_64-linux-gnu/'
    for component, archives, stems, suffix in (
        ('libnl', ('libnl-nf-sdk.tar', 'libnl-genl-sdk.tar'),
         ('libnl-genl-3', 'libnl-nf-3', 'libnl-route-3', 'libnl-3'), '.so.200'),
        ('libyang', ('libyang-runtime.tar',), ('libyang',), '.so.3'),
    ):
        record = sdk[component]
        producers = {PREPARATION_TARGETS[name]: inputs[name]['sha256'] for name in archives}
        check.require(record['producer_targets'] == producers and record['parent_source_tar_sha256'] == parent_sha,
                      'SDK library producer or parent TAR linkage changed')
        libraries = {prefix + stem + suffix: stem + suffix + ('.26.0' if component == 'libnl' else '.9.1')
                     for stem in stems}
        required = set(libraries) | {prefix + name for name in libraries.values()}
        files = record['files']
        check.require(required <= files.keys(),
                      'SDK library inventory is incomplete')
        if component == 'libnl':
            check.require(files.get('usr/include/libnl3/netlink/netlink.h', {}).get('kind') == 'file',
                          'SDK libnl header inventory is incomplete')
        projected = {}
        for name, item in files.items():
            relative_name(name)
            linker_names = {prefix + stem + '.so' for stem in stems}
            check.require(name in required | linker_names or component == 'libnl' and name.startswith('usr/include/libnl3/'),
                          'Unexpected SDK inventory path')
            sdk_name = 'lib/' + name.removeprefix(prefix) if name.startswith(prefix) else name.removeprefix('usr/')
            installed = inventory[sdk_name]
            check.require(item['kind'] == installed['kind'], 'SDK inventory file kind changed')
            if name in libraries or name in linker_names:
                target = item['linkname']
                linked_soname = name.removesuffix('.so') + suffix
                allowed = {libraries[name]} if name in libraries else {
                    linked_soname.removeprefix(prefix), libraries[linked_soname]}
                check.require(item['kind'] == 'symlink' and Path(target).name == target and
                              target not in ('.', '..') and target == installed['target'] and
                              target in allowed,
                              'SDK SONAME linkage changed')
                projected[name] = {'kind': 'symlink', 'linkname': target}
            else:
                check.require(item['kind'] == 'file' and digest(item['sha256']) == installed['sha256'],
                              'SDK library or header differs from the owner inventory')
                projected[name] = {'kind': 'file', 'sha256': item['sha256']}
                if name in required:
                    check.require(item['elf_machine'] == 62, 'SDK library must be AMD64')
                    projected[name]['elf_machine'] = 62
        sdk_view[component] = {'producer_targets': producers, 'parent_source_tar_sha256': parent_sha, 'files': projected}
    result = {'schema': 1, 'status': 'prepared', 'revision': prepared['revision'],
              'owner_revision': source['commit'], 'architecture': 'amd64', 'artifacts': inputs,
              'action_audit': audit_view, 'sdk_build': sdk_view,
              'parent_common': {key: parent[key] for key in ('sha256', 'common_runtime_tar_sha256', 'parent_source_tar_sha256')},
              'original_sha256': originals}
    safe_json(result)
    return result


def owner_test_summary(directory):
    """Summarize the owner's retained XML and BEP statuses without copying logs."""
    owner = directory / 'owner-output'
    source = json.loads((owner / 'source.json').read_text())
    receipt = json.loads((directory / 'owner-tests.json').read_text())
    check.require(receipt['status'] == 'passed' and receipt['owner_revision'] == source['commit'],
                  'Owner test receipt differs from the built source')
    targets = source['test_targets']
    check.require(isinstance(targets, list) and targets and len(set(targets)) == len(targets),
                  'Expected a unique nonempty owner test selection')
    check.require((owner / 'test-targets.txt').read_text().splitlines() == targets,
                  'Owner test selection changed after build')
    statuses = json.loads((owner / 'test-results.json').read_text())
    check.require(set(statuses) == set(targets) and all(x == 'PASSED' for x in statuses.values()),
                  'Every selected owner test needs a passing BEP status')
    results, total_bytes = {}, 0
    for target in targets:
        label(target)
        check.require(target.startswith(('//p4rt_app/', '//p4rt_app:')) and target.count(':') == 1,
                      'Unexpected owner test target')
        relative = Path(target.removeprefix('//').replace(':', '/')) / 'test.xml'
        path = owner / 'test-results' / relative
        check.require(path.is_file() and not path.is_symlink() and
                      path.resolve().is_relative_to((owner / 'test-results').resolve()),
                      'Owner test XML escaped its retained directory')
        size = path.stat().st_size
        total_bytes += size
        check.require(0 < size <= 2 * 1024**2 and total_bytes <= 16 * 1024**2,
                      'Owner test XML exceeds the summary size limit')
        counts = test_result(path)
        check.require(counts['skipped'] == 0 and not ElementTree.parse(path).getroot().findall('.//skipped'),
                      'Owner tests must execute without skipped cases')
        results[target] = {'status': statuses[target], 'xml_sha256': check.sha(path), **counts}
    return {'status': 'passed', 'owner_revision': source['commit'],
            'target_count': len(results), 'testcase_count': sum(x['tests'] for x in results.values()),
            'tests': results}


def prepare(directory):
    directory.mkdir(parents=True, exist_ok=True)
    public = directory / 'public'
    # Never upload leftovers from an earlier attempt as current evidence.
    public.mkdir(exist_ok=False)

    def write(name, value):
        safe_json(value)
        (public / name).write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')

    receipt_path = directory / 'build-receipt.json'
    if receipt_path.exists():
        receipt = json.loads(receipt_path.read_text())
        write('public-receipt.json', {key: receipt[key] for key in
              ('status', 'architecture', 'revision', 'owner_revision', 'owner_handoff', 'artifacts') if key in receipt})
        for name, record in receipt.get('artifacts', {}).items():
            source = directory / name
            check.require(Path(name).name == name and not source.is_symlink(), 'Unexpected public artifact path')
            check.require(check.sha(source) == record['sha256'], 'Public artifact changed: ' + name)
            if source.suffix == '.json':
                safe_json(json.loads(source.read_text()))
            shutil.copyfile(source, public / name)
    for name in ('validation.json', 'action-audit.json', 'incremental.json'):
        source = directory / name
        if source.is_file():
            write(name, json.loads(source.read_text()))
    for source in directory.glob('*.validation.json'):
        write(source.name, json.loads(source.read_text()))
    for name, kind in (('MODULE.bazel.lock', 'lock'), ('module-graph.json', 'graph')):
        source = directory / name
        if source.is_file():
            dependency_json(json.loads(source.read_text()), kind)
            shutil.copyfile(source, public / name)
    owner = directory / 'owner-output'
    source_receipt = owner / 'source.json'
    if source_receipt.exists():
        source = json.loads(source_receipt.read_text())
        write('owner-source.json', {key: source[key] for key in
              ('commit', 'bazel', 'architecture', 'test_targets', 'sdk_inventory', 'tar_sha256', 'contract_sha256')})
        write('sdk-preparation.json', sdk_preparation_summary(directory, source))
        for name, expected in source['contract_sha256'].items():
            check.require(Path(name).name == name and check.sha(owner / name) == expected,
                          'Owner contract changed before publication')
            write(name, json.loads((owner / name).read_text()))
    for name in ('action-audit.json', 'test-action-audit.json', 'test-results.json', 'installed-packages.json'):
        source = owner / name
        if source.is_file():
            write('owner-' + name, json.loads(source.read_text()))
    tests_path = directory / 'owner-tests.json'
    if tests_path.exists():
        source = json.loads(tests_path.read_text())
        write('owner-tests.json', {key: source[key] for key in ('status', 'owner_revision')})
        if source['status'] == 'passed':
            write('owner-test-summary.json', owner_test_summary(directory))
    summary = {str(path.relative_to(directory)): bep_summary(path.read_text().splitlines(), partial=True)
               for path in directory.rglob('*events.json*') if not path.is_relative_to(public)}
    write('build-summary.json', summary)
    return public


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, required=True)
    args = parser.parse_args()
    print(prepare(args.directory.resolve()))
