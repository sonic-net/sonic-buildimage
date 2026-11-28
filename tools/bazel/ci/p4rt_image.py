#!/usr/bin/env python3
"""Build P4RT OCI images and run unified @sonic_pins tests natively under Bazel 8."""

import argparse
from functools import partial
import json
import os
from pathlib import Path
import platform
import shlex
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.build_helpers import cache_options, remote_cache_options
from tools.bazel.ci import artifact_validation as check
from tools.bazel.ci import bazel_commands, build, command_log, container, resolution

OWNER = '//dockers/docker-sonic-p4rt:'
IMAGES = ('docker-sonic-p4rt', 'docker-sonic-p4rt-dbg')
BASE = '//dockers/docker-config-engine-trixie:docker-config-engine-trixie'
TEST_SCOPE = '@sonic_pins//p4rt_app:runtime_package_test'
OPTIONS = ['--@sonic_swss_common//tools/bazel:yang_modules=True',
           '--platforms=@sonic_build_infra//platforms:x86_64_trixie',
           '--host_platform=@sonic_build_infra//platforms:x86_64_trixie']
TARGETS = {
    **{name + '.gz': OWNER + name + '.gz' for name in IMAGES},
    'test-services.tar': OWNER + 'test_services',
    'p4rt.source.tar': '@sonic_pins//p4rt_app:p4rt_runtime',
    'p4rt.debug.tar': OWNER + 'all_debug_symbols',
    'p4rt.debug-tools.tar': OWNER + 'debug_tools',
    'p4rt.files.tar': OWNER + 'files',
    'openssh.source.tar': OWNER + 'debug_client',
    'sairedis.source.tar': '@sonic_sairedis//lib:libsairedis_pkg',
    'saimetadata.source.tar': '@sonic_sairedis//meta:libsaimetadata_pkg',
    'sairedis.debug.tar': '@sonic_sairedis//lib:libsairedis_pkg.debug_symbols',
    'saimetadata.debug.tar': '@sonic_sairedis//meta:libsaimetadata_pkg.debug_symbols',
    'sairedis.library-packages.json': '@sonic_sairedis//:library_runtime_packages.json',
    **{stage + '-apt.json': OWNER + stage + '_apt_selection' for stage in ('runtime', 'debug')},
    **{'p4rt-' + stage + '.packages.json': OWNER + stage + '_package_metadata' for stage in ('runtime', 'debug')},
    **{'p4rt-' + stage + '.status': OWNER + stage + '_installed_packages' for stage in ('runtime', 'debug')},
    'base.packages.json': '//dockers/docker-base-trixie:runtime_package_metadata',
    'base.source.tar': '//dockers/docker-base-trixie:source_runtime',
    'config-engine.packages.json': '//dockers/docker-config-engine-trixie:runtime_package_metadata',
    'config-engine.source.tar': '//dockers/docker-config-engine-trixie:source_runtime',
}


def root_options(extra_args=(), *, make_build=False):
    cache = os.environ.get('BAZEL_CONTAINER_CACHE_DIR', os.environ.get('BAZEL_SWSS_CACHE_DIR', ''))
    arguments = os.environ.get('BAZEL_CONTAINER_ARGS', os.environ.get('BAZEL_SWSS_ARGS', ''))
    extra = cache_options(cache) + shlex.split(arguments) + list(extra_args)
    if make_build:
        # Use the ordinary Make image producer's final read/write policy.
        extra += remote_cache_options()
    check.require(all(value and not any(char in value for char in '\r\n\0') for value in extra),
                  'Bazel options must be nonempty single-line arguments')
    # Preserve cache/custom options, then enforce the recorded YANG/native profile.
    return [*extra, *OPTIONS, build.MAKE_METADATA_OPTION]


def write(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')


def check_directory(directory):
    check.require(not ROOT.is_relative_to(directory), 'Artifacts cannot contain the checkout')
    if directory.is_relative_to(ROOT):
        check.require(directory.relative_to(ROOT).parts[0] == 'artifacts', 'Use artifacts/<name> for in-checkout evidence')


def source_identity(directory):
    check_directory(directory)
    subprocess.run(['git', 'diff', '--exit-code', '--ignore-submodules=none', 'HEAD'], cwd=ROOT, check=True)
    untracked = subprocess.check_output(['git', 'ls-files', '--others', '--exclude-standard', '-z'], cwd=ROOT).decode().split('\0')
    # Full VS Make and preceding CI steps keep their independent receipts here.
    # This reserved evidence tree supplies no Bazel source inputs; tracked files
    # and submodules remain covered by the complete HEAD diff above.
    check.require(not [name for name in untracked if name and not (ROOT / name).is_relative_to(ROOT / 'artifacts')],
                  'Image source checkout has undeclared inputs')
    return checkout_revision()


def checkout_revision():
    return subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()


def verify_owner_source(source, expected_revision):
    check.require(not source.is_symlink() and source.is_dir(),
                  'Owner checkout directory is unavailable: ' + str(source))
    head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=source, text=True).strip()
    check.require(head == expected_revision,
                  f'Owner checkout HEAD {head} differs from expected {expected_revision}')
    subprocess.run(['git', 'diff', '--exit-code', '--ignore-submodules=none', 'HEAD'], cwd=source, check=True)
    untracked = subprocess.check_output(
        ['git', 'ls-files', '--others', '--exclude-standard', '-z'], cwd=source).decode().split('\0')
    check.require(not [name for name in untracked if name], 'Owner checkout has untracked files')
    return head


def native():
    check.require(platform.machine() == 'x86_64' and
                  platform.freedesktop_os_release().get('VERSION_CODENAME') == 'trixie',
                  'P4RT image acceptance requires native AMD64/Trixie')


def prepare_manifests(execute, directory, receipt):
    execute(['make', '-f', 'tools/bazel/prepare_manifests.mk',
             'MANIFEST_METADATA=rules/docker-base-trixie.mk rules/docker-config-engine-trixie.mk rules/docker-p4rt.mk',
             'BUILD_WITH_BAZEL_WHEN_AVAILABLE=y', 'BLDENV=trixie', 'CONFIGURED_PLATFORM=vs',
             'CONFIGURED_ARCH=amd64', 'INCLUDE_FIPS=y', 'ENABLE_ASAN=n', 'DBG_IMAGE_MARK=dbg',
             'DOCKERS_PATH=dockers'], directory, receipt, 'manifests')


def build_images(args):
    native()
    options = root_options(args.bazel_arg)
    directory = args.directory.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    revision = source_identity(directory)
    owner_source = args.owner_source.resolve()
    verify_owner_source(owner_source, args.owner_revision)
    receipt = {'schema': 2, 'status': 'building', 'architecture': 'amd64', 'commands': [],
               'revision': revision, 'owner_revision': args.owner_revision}
    execute = partial(command_log.execute, cwd=ROOT)
    started = time.monotonic()
    try:
        version = execute([args.bazel, '--version'], directory, receipt, 'root-version')
        check.require([line for line in version.splitlines() if line.startswith('bazel ')] == ['bazel 8.5.1'],
                      'The selected image builder must remain Bazel 8.5.1')
        prepare_manifests(execute, directory, receipt)
        layouts = [BASE, *[OWNER + name for name in IMAGES]]
        receipt['action_audit'] = bazel_commands.audit_targets(
            args.bazel, options, [*TARGETS.values(), *layouts],
            workspace=ROOT, output=directory / 'action-audit.json')
        build.collect_archives(ROOT, directory, receipt, TARGETS, bazel=args.bazel, options=options,
                               use_execution_root=True)
        execute([args.bazel, 'build', *options, *layouts], directory, receipt, 'layouts')
        receipt['layouts'] = {name: execute(
            [args.bazel, 'cquery', *options, '--output=files', OWNER + name],
            directory, receipt, name + '.layout').strip() for name in IMAGES}
        receipt['base_layout'] = execute(
            [args.bazel, 'cquery', *options, '--output=files', BASE],
            directory, receipt, 'base.layout').strip()
        receipt['resolution'] = resolution.collect(ROOT, directory, bazel=[args.bazel])
        check.require(source_identity(directory) == receipt['revision'], 'Image checkout changed')
        verify_owner_source(owner_source, args.owner_revision)
        receipt.update(status='built', elapsed_seconds=time.monotonic() - started)
    except Exception as error:
        receipt.update(status='failed', error=str(error))
        raise
    finally:
        write(directory / 'build-receipt.json', receipt)


def owner_tests(args):
    native()
    directory = args.directory.resolve()
    build_receipt = json.loads((directory / 'build-receipt.json').read_text())
    check.require(build_receipt.get('status') == 'built' and
                  build_receipt.get('revision') == source_identity(directory) and
                  build_receipt.get('owner_revision') == args.owner_revision,
                  'Built P4RT receipt does not match this checkout and owner revision')
    owner_source = args.owner_source.resolve()
    verify_owner_source(owner_source, args.owner_revision)
    options = root_options(args.bazel_arg)
    execute = partial(command_log.execute, cwd=ROOT)
    receipt = {'schema': 2, 'status': 'testing', 'revision': build_receipt['revision'],
               'owner_revision': args.owner_revision, 'commands': [], 'targets': []}
    started = time.monotonic()
    try:
        tests = sorted({line.strip() for line in execute(
            [args.bazel, 'query', f'tests({TEST_SCOPE})'],
            directory, receipt, 'query-tests').splitlines() if line.strip()})
        check.require(tests, 'No P4RT owner tests matched ' + TEST_SCOPE)
        labels = container.configured_test_labels(tests, args.bazel, options, directory, receipt, execute)
        events = directory / 'owner-test-events.jsonl'
        events.unlink(missing_ok=True)
        events.touch(mode=0o600, exist_ok=False)
        execute([args.bazel, 'test', *options, '--nocache_test_results', '--test_output=errors',
                 '--nozip_undeclared_test_outputs', '--build_event_json_file=' + str(events),
                 *tests], directory, receipt, 'owner-tests')
        test_outputs, _ = container.collect_test_outputs(events, directory, tests, labels)
        receipt['targets'] = list(labels)
        receipt['results'] = {canonical: 'passed' for canonical in labels}
        receipt['test_outputs'] = test_outputs
        verify_owner_source(owner_source, args.owner_revision)
        receipt.update(status='passed', elapsed_seconds=time.monotonic() - started)
    except Exception as error:
        receipt.update(status='failed', error=str(error))
        raise
    finally:
        write(directory / 'owner-test-results.json', receipt)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('phase', choices=('build', 'owner-tests'))
    parser.add_argument('--directory', required=True, type=Path)
    parser.add_argument('--bazel', default='bazel', help='Root image Bazel 8.5.1 launcher')
    parser.add_argument('--bazel-arg', action='append', default=[])
    parser.add_argument('--owner-source', required=True, type=Path)
    parser.add_argument('--owner-revision', required=True)
    args = parser.parse_args()
    {'build': build_images, 'owner-tests': owner_tests}[args.phase](args)


if __name__ == '__main__':
    main()
