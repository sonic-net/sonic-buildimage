#!/usr/bin/env python3
"""Audit Sysmgr source builds and retain installed/native-test acceptance."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys
import time
import urllib.parse
import xml.etree.ElementTree as ET

MODULE = Path(__file__).resolve().parents[1]
REPO = MODULE.parents[1]
sys.path.insert(0, str(REPO))
from tools.bazel.ci.bazel_commands import audit_targets

TESTS = ['//rebootbackend:gnoi_runtime_test', '//tests:installed_sysmgr_test', '//tests:original_unit_tests']
PACKAGES = {'runtime.tar': '//:sysmgr_pkg', 'debug.tar': '//:sysmgr_debug_pkg',
            'runtime_packages.json': '//:runtime_packages.json'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bazel', default='bazel')
    parser.add_argument('--registry', default='https://raw.githubusercontent.com/securely1g/sonic-bazel-registry/main')
    parser.add_argument('--artifacts', type=Path, required=True)
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--startup-arg', action='append', default=[])
    args = parser.parse_args()
    assert platform.machine() == 'x86_64'
    assert platform.freedesktop_os_release()['VERSION_CODENAME'] == 'trixie'
    output = args.artifacts.resolve(); output.mkdir(parents=True, exist_ok=True)
    # Retain setup-bazel's home cache settings. Ignore the workspace registry
    # defaults so this review uses exactly one catalog followed by BCR.
    start = ['--nosystem_rc', '--noworkspace_rc', *args.startup_arg]
    resolution = ['--registry=' + args.registry, '--registry=https://bcr.bazel.build/']
    options = [*resolution, '--check_direct_dependencies=off', '--lockfile_mode=update',
               '--platforms=@sonic_build_infra//platforms:x86_64_trixie',
               '--host_platform=@sonic_build_infra//platforms:x86_64_trixie',
               '--compilation_mode=opt', '--copt=-g', '--jobs=' + str(args.jobs),
               '--remote_download_regex=(^|.*/)tools/build_tools/kernel_runtime_(amd64|arm64)(/.*)?$',
               '--spawn_strategy=linux-sandbox', '--strategy=Genrule=linux-sandbox',
               '--strategy=TestRunner=linux-sandbox', '--noincompatible_legacy_local_fallback',
               '--color=no', '--curses=no']

    def bazel(command, *values, capture=False):
        argv = [args.bazel, *start, command, *values]
        if capture:
            return subprocess.check_output(argv, cwd=MODULE, text=True)
        subprocess.run(argv, cwd=MODULE, check=True)

    def git(*values):
        return subprocess.check_output(['git', '-c', 'safe.directory=' + str(REPO), *values], cwd=REPO, text=True).strip()

    # Existing legacy Make/sanitizer builds are preserved. This lane validates
    # the selected unsanitized AMD64 producer and its original native cases.
    assert not git('status', '--porcelain', '--untracked-files=no')
    assert not git('status', '--porcelain', '--untracked-files=all', '--', 'src/sonic-sysmgr')
    head = git('rev-parse', 'HEAD')
    expected_head = os.environ.get('CI_HEAD_SHA')
    if expected_head and head != expected_head:
        subprocess.run(['git', '-c', 'safe.directory=' + str(REPO), 'merge-base', '--is-ancestor', expected_head, head], cwd=REPO, check=True)
    files = git('ls-files', 'src/sonic-sysmgr').splitlines()
    sources = {name: hashlib.sha256((REPO / name).read_bytes()).hexdigest()
               for name in files if (REPO / name).is_file()}
    gnoi = MODULE / 'gnoi'
    gnoi_revision = git('rev-parse', 'HEAD:src/sonic-sysmgr/gnoi')
    actual_gnoi = subprocess.check_output(['git', '-c', 'safe.directory=' + str(gnoi), 'rev-parse', 'HEAD'], cwd=gnoi, text=True).strip()
    assert actual_gnoi == gnoi_revision
    assert not subprocess.check_output(['git', '-c', 'safe.directory=' + str(gnoi), 'status', '--porcelain', '--untracked-files=no'], cwd=gnoi, text=True).strip()
    proto_sources = {name: hashlib.sha256((gnoi / name).read_bytes()).hexdigest()
                     for name in ('types/types.proto', 'common/common.proto', 'system/system.proto')}
    selected_tests = set(bazel('query', *resolution, 'kind(".*_test rule", //...)', '--output=label', capture=True).splitlines())
    assert selected_tests == set(TESTS), selected_tests
    selected_sources = bazel('query', *resolution,
        'labels(srcs, kind("cc_.* rule", //rebootbackend:*))', '--output=label', capture=True).splitlines()
    actual_sources = {label.split(':', 1)[1] for label in selected_sources
                      if label.startswith('//rebootbackend:') and label.endswith('.cpp')}
    original_sources = set(re.findall(r'\b\w+\.cpp\b', (MODULE / 'rebootbackend/Makefile.am').read_text()))
    assert actual_sources == original_sources | {'gnoi_runtime_test.cpp'}, (actual_sources, original_sources)
    audit_targets(args.bazel, options, [*PACKAGES.values(), *TESTS], workspace=MODULE,
                  output=output / 'no-deb-audit.json', startup_options=start)
    (output / 'module-graph.json').write_text(bazel('mod', 'graph', *resolution, '--output=json', capture=True))
    begun = time.monotonic()
    report = {'status': 'failed', 'checkout': head, 'pr_head': expected_head, 'gnoi_revision': gnoi_revision,
              'architecture': 'amd64', 'os': 'trixie', 'sources': sources, 'proto_sources': proto_sources,
              'production_sources': sorted(original_sources),
              'selected_tests': TESTS, 'catalog': args.registry}
    failure = None
    try:
        bazel('test', *options, '--nocache_test_results', '--test_output=errors',
              '--build_event_json_file=' + str(output / 'test-events.jsonl'),
              '--profile=' + str(output / 'profile.json.gz'), *TESTS, *PACKAGES.values())
    except Exception as error:
        failure = error
        report['build_error'] = repr(error)
    try:
        lock = MODULE / 'MODULE.bazel.lock'
        if lock.is_file():
            shutil.copyfile(lock, output / 'MODULE.bazel.lock')
        elif failure is None:
            raise AssertionError('Successful build did not retain generated resolution state')
        if failure is None:
            assert (output / 'test-events.jsonl').is_file()
        if (output / 'test-events.jsonl').exists():
            events = [json.loads(line) for line in (output / 'test-events.jsonl').read_text().splitlines()]
            summaries = {e['id']['testSummary']['label']: e['testSummary']['overallStatus']
                         for e in events if 'testSummary' in e}
            report['test_statuses'] = summaries
            for event in events:
                if 'testResult' not in event:
                    continue
                identity = event['id']['testResult']
                label = identity['label']
                suffix = '_'.join(str(identity.get(k, 0)) for k in ('run', 'shard', 'attempt'))
                config = identity.get('configuration', {}).get('id', 'unknown')
                destination = output / 'testlogs' / label.removeprefix('//').replace(':', '_') / (config + '_' + suffix)
                destination.mkdir(parents=True, exist_ok=True)
                for entry in event['testResult'].get('testActionOutput', []):
                    uri = entry.get('uri', '')
                    if not uri.startswith('file://'):
                        continue
                    source = Path(urllib.parse.unquote(urllib.parse.urlparse(uri).path))
                    if source.is_file():
                        shutil.copyfile(source, destination / source.name)
                        if source.name == 'test.log':
                            outputs = source.parent / 'test.outputs'
                            if outputs.is_dir():
                                shutil.copytree(outputs, destination / 'outputs', dirs_exist_ok=True)
            if failure is None:
                assert summaries == {label: 'PASSED' for label in TESTS}, summaries
                cases = {}
                for label in TESTS:
                    folder = output / 'testlogs' / label.removeprefix('//').replace(':', '_')
                    xmls = list(folder.glob('*/test.xml'))
                    assert len(xmls) == 1, (label, xmls)
                    xml = ET.parse(xmls[0])
                    assert not any(list(xml.iter(k)) for k in ('failure', 'error', 'skipped'))
                    cases[label] = len(list(xml.iter('testcase')))
                    assert cases[label] == (52 if label.endswith(':original_unit_tests') else 1), cases
                report['test_cases'] = cases
        if failure is None:
            report['packages'] = {}
            for name, target in PACKAGES.items():
                paths = bazel('cquery', *options, target, '--output=files', capture=True).splitlines()
                assert len(paths) == 1
                shutil.copyfile(MODULE / paths[0], output / name)
                report['packages'][target] = {'path': name, 'sha256': hashlib.sha256((output / name).read_bytes()).hexdigest()}
            assert sources == {name: hashlib.sha256((REPO / name).read_bytes()).hexdigest() for name in sources}
            assert proto_sources == {name: hashlib.sha256((gnoi / name).read_bytes()).hexdigest() for name in proto_sources}
            assert not git('status', '--porcelain', '--untracked-files=no')
            assert not git('status', '--porcelain', '--untracked-files=all', '--', 'src/sonic-sysmgr')
    except Exception as error:
        report['collection_error'] = repr(error)
        if failure is None:
            failure = error
    finally:
        report['status'] = 'passed' if failure is None else 'failed'
        report['elapsed_seconds'] = round(time.monotonic() - begun, 3)
        (output / 'validation.json').write_text(json.dumps(report, indent=2) + '\n')
    if failure is not None:
        raise failure


if __name__ == '__main__':
    main()
