#!/usr/bin/env python3
"""Prepare the owner SDK/TAR handoff and build P4RT OCI images in separate stages."""

import argparse
from functools import partial
import importlib.util
import json
import os
from pathlib import Path
import platform
import re
import shlex
import subprocess
import sys
import tarfile
import tempfile
import time

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import artifact_validation as check
from tools.bazel.ci import bazel_commands, build, command_log, p4rt_handoff, resolution, time_vs_image
from tools.bazel.p4rt_common_sdk import prepare_repository
from tools.bazel.build_helpers import cache_options, remote_cache_options
from tools.bazel.ci.preserve_setup_bazel_cache import PRIVATE_CACHE_ENDPOINT
from tools.bazel.oci.oci_inventory import resolve_path

OWNER = '//dockers/docker-sonic-p4rt:'
IMAGES = ('docker-sonic-p4rt', 'docker-sonic-p4rt-dbg')
BASE = '//dockers/docker-config-engine-trixie:docker-config-engine-trixie'
RUNTIME_INPUTS = ('owner-test-runtime.tar',)
COMMON_REVISION = '334389eae54afd1f4f5ee69f89c3a7ad2062ee61'
OPTIONS = ['--jobs=2', '--local_resources=cpu=2', '--local_resources=memory=8000',
           '--@sonic_swss_common//tools/bazel:yang_modules=True',
           '--platforms=@sonic_build_infra//platforms:x86_64_trixie',
           '--host_platform=@sonic_build_infra//platforms:x86_64_trixie']
PREPARATION_TARGETS = {
    'common-sdk.tar': '@sonic_swss_common//dist:libswsscommon_sdk',
    'libnl-nf-sdk.tar': '@libnl3//:libnl-nf-3-dev_pkg',
    'libnl-genl-sdk.tar': '@libnl3//:libnl-genl-3-dev_pkg',
    'libyang-runtime.tar': '@libyang//:libyang_pkg',
    'common-runtime.tar': '@sonic_swss_common//dist:libswsscommon_pkg',
    'common-runtime-runner.py': OWNER + 'owner_runtime_runner',
    'common.MODULE.bazel': OWNER + 'owner_common_module',
    'owner-test-runtime.tar': OWNER + 'owner_test_runtime',
    'runtime-patchelf': OWNER + 'owner_patchelf',
    'config-engine.source.tar': '//dockers/docker-config-engine-trixie:source_runtime',
}
TARGETS = {
    **{name + '.gz': OWNER + name + '.gz' for name in IMAGES},
    'test-services.tar': OWNER + 'test_services',
    'p4rt.source.tar': OWNER + 'source_runtime',
    'p4rt.debug.tar': OWNER + 'all_debug_symbols',
    'p4rt.native-debug.tar': OWNER + 'native_debug_symbols',
    'p4rt.debug-tools.tar': OWNER + 'debug_tools',
    'p4rt.files.tar': OWNER + 'files',
    'openssh.source.tar': OWNER + 'debug_client',
    'sairedis.source.tar': '@sonic_sairedis//lib:libsairedis_pkg',
    'saimetadata.source.tar': '@sonic_sairedis//meta:libsaimetadata_pkg',
    'sairedis.debug.tar': '@sonic_sairedis//lib:libsairedis_pkg.debug_symbols',
    'saimetadata.debug.tar': '@sonic_sairedis//meta:libsaimetadata_pkg.debug_symbols',
    'sairedis.library-packages.json': '@sonic_sairedis//:library_runtime_packages.json',
    **{stage + '-apt.json': OWNER + stage + '_apt_selection' for stage in ('runtime', 'symbols', 'debug')},
    **{'p4rt-' + stage + '.packages.json': OWNER + stage + '_package_metadata' for stage in ('runtime', 'symbols', 'debug')},
    **{'p4rt-' + stage + '.status': OWNER + stage + '_installed_packages' for stage in ('runtime', 'symbols', 'debug')},
    'base.packages.json': '//dockers/docker-base-trixie:runtime_package_metadata',
    'base.source.tar': '//dockers/docker-base-trixie:source_runtime',
    'config-engine.packages.json': '//dockers/docker-config-engine-trixie:runtime_package_metadata',
    'config-engine.source.tar': '//dockers/docker-config-engine-trixie:source_runtime',
}


def root_options(*, make_build=False):
    cache = os.environ.get('BAZEL_CONTAINER_CACHE_DIR', os.environ.get('BAZEL_SWSS_CACHE_DIR', ''))
    arguments = os.environ.get('BAZEL_CONTAINER_ARGS', os.environ.get('BAZEL_SWSS_ARGS', ''))
    extra = cache_options(cache) + shlex.split(arguments)
    if make_build:
        # Use the ordinary Make image producer's final read/write policy.
        extra += remote_cache_options()
    check.require(all(value and not any(char in value for char in '\r\n\0') for value in extra),
                  'Bazel options must be nonempty single-line arguments')
    # Preserve cache/custom options, then enforce the recorded YANG/native profile.
    return [*OPTIONS[:3], *extra, *OPTIONS[3:], build.MAKE_METADATA_OPTION]


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


def native():
    check.require(platform.machine() == 'x86_64' and
                  platform.freedesktop_os_release().get('VERSION_CODENAME') == 'trixie',
                  'P4RT image acceptance requires native AMD64/Trixie')


def locations(args):
    directory = args.directory.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    return directory, directory / 'owner-preparation', directory / 'owner-sdk', directory / 'owner-output'


def owner_command(args, sdk, output, phase, runtime=None, *, bazel=None):
    command = [sys.executable, '-B', str(args.owner_source.resolve() / 'ci/production_runtime.py'),
               '--phase=' + phase, '--bazel=' + (bazel or args.owner_bazel),
               '--buildimage=' + str(ROOT), '--sdk=' + str(sdk),
               '--output-base=' + str(args.owner_output_base.resolve()),
               '--output=' + str(output), '--jobs=' + str(args.jobs)]
    if runtime is not None:
        command.append('--runtime-receipt=' + str(runtime))
    return command


def server_process(pid):
    try:
        fields = Path('/proc/' + str(pid) + '/stat').read_text().rsplit(')', 1)[1].split()
    except FileNotFoundError:
        return None
    return {'state': fields[0], 'started': int(fields[19]), 'rss_pages': int(fields[21])}


def make_owner_launcher(directory, bazel):
    # Keep PINS' workspace rc and the slave's system rc for its mounted
    # repository/disk caches. Its Bazel 6 client cannot consume root Bzlmod
    # settings, --local_resources, or Bazel 8's spelling of --downloader_config.
    config = directory / 'make-owner.bazelrc'
    settings = remote_cache_options()
    arguments = shlex.split(os.environ.get('BAZEL_CONTAINER_ARGS', os.environ.get('BAZEL_SWSS_ARGS', '')))
    downloader = '--experimental_remote_downloader=' + PRIVATE_CACHE_ENDPOINT
    if '--remote_cache=' + PRIVATE_CACHE_ENDPOINT in settings and downloader in arguments:
        settings.append(downloader)
        if '--experimental_remote_downloader_local_fallback' in arguments:
            settings.append('--experimental_remote_downloader_local_fallback')
    settings.append('--experimental_downloader_config=' + str(ROOT / 'src/sonic-p4rt/source_mirrors.cfg'))
    # Prevent rc-line injection. Follow the image producers' upload policy, so
    # only builds that may write the selected cache (master pushes) upload.
    check.require(all(not any(c in option for c in '\n\r\0') for option in settings), 'Invalid owner cache option')
    upload = 'true' if '--remote_upload_local_results=true' in settings else 'false'
    config.write_text(''.join('common ' + shlex.quote(option) + '\n' for option in settings
                              if not option.startswith('--remote_upload_local_results=')) +
                      'build --remote_upload_local_results=' + upload + '\n')
    launcher = directory / 'bazel'
    launcher.write_text('#!/bin/sh\nexec ' + shlex.join([bazel, '--bazelrc=' + str(config)]) + ' "$@"\n')
    launcher.chmod(0o755)
    return str(launcher)


def run_owner(args, sdk, output, phase, directory, receipt, runtime=None, *, make_build=False):
    execute = partial(command_log.execute, cwd=ROOT)
    if make_build:
        # The normal Make recipes may use the checkout's root server in
        # parallel. Only this PINS invocation owns its output base/shutdown.
        with tempfile.TemporaryDirectory(prefix='p4rt-owner-client-', dir=directory) as scratch:
            client = make_owner_launcher(Path(scratch), args.owner_bazel)
            return execute(owner_command(args, sdk, output, phase, runtime, bazel=client),
                           directory, receipt, 'owner-build')

    # Dedicated image CI owns the two bounded JVMs. Release its root client
    # before starting the owner without dropping either client's cached data.
    pid = int(execute([args.bazel, 'info', 'server_pid'], directory, receipt, 'root-server-pid').strip())
    before = server_process(pid)
    check.require(before is not None and before['state'] != 'Z', 'Root Bazel server identity is unavailable')
    try:
        execute([args.bazel, 'shutdown'], directory, receipt, 'root-server-shutdown')
    except subprocess.CalledProcessError as error:
        # Some container PID1 implementations leave an exited JVM as a zombie.
        # Accept only the observed container wait failure with this exact PID gone
        # or the same process defunct and using no resident memory.
        if error.returncode != 36:
            raise
    after = server_process(pid)
    check.require(after is None or (after['started'] == before['started'] and
                  after['state'] == 'Z' and after['rss_pages'] == 0),
                  'Root Bazel server remains alive; refusing overlapping owner compilation')
    receipt['root_server_shutdown'] = {'pid': pid, 'before': before, 'after': after,
                                       'caches_and_outputs_retained': True}
    return execute(owner_command(args, sdk, output, phase, runtime), directory, receipt,
                   'owner-tests' if phase == 'test' else 'owner-build')


def prepare_manifests(execute, directory, receipt):
    execute(['make', '-f', 'tools/bazel/prepare_manifests.mk',
             'MANIFEST_METADATA=rules/docker-base-trixie.mk rules/docker-config-engine-trixie.mk rules/docker-p4rt.mk',
             'BUILD_WITH_BAZEL_WHEN_AVAILABLE=y', 'BLDENV=trixie', 'CONFIGURED_PLATFORM=vs',
             'CONFIGURED_ARCH=amd64', 'INCLUDE_FIPS=y', 'ENABLE_ASAN=n', 'DBG_IMAGE_MARK=dbg',
             'DOCKERS_PATH=dockers'], directory, receipt, 'manifests')


def prepare_owner(args, *, make_build=False):
    native()
    directory, preparation, sdk, output = locations(args)
    check_directory(directory)
    p4rt_handoff.source_identity(args.owner_source, args.owner_revision)
    preparation.mkdir(parents=True, exist_ok=True)
    receipt = {'schema': 1, 'status': 'preparing', 'architecture': 'amd64',
               'owner_revision': args.owner_revision, 'commands': []}
    execute = partial(command_log.execute, cwd=ROOT)
    options = root_options(make_build=make_build)
    started = time.monotonic()
    try:
        version = execute([args.bazel, '--version'], preparation, receipt, 'root-version')
        check.require([line for line in version.splitlines() if line.startswith('bazel ')] == ['bazel 8.5.1'],
                      'The selected image/Common builder must remain Bazel 8.5.1')
        # A shared Make checkout may contain local edits and outputs from parallel
        # packages. Its revision identifies the base commit only. The source owner,
        # SDK, parent library and published TAR bytes are checked in both modes.
        identity = checkout_revision if make_build else partial(source_identity, directory)
        revision = receipt['revision'] = identity()
        if not make_build:
            prepare_manifests(execute, preparation, receipt)
        phase = partial(time_vs_image.log_make_phase, vs_make=make_build)
        with phase('p4rt-audit'):
            receipt['action_audit'] = bazel_commands.audit_targets(args.bazel, options, list(PREPARATION_TARGETS.values()),
                workspace=ROOT, output=preparation / 'action-audit.json')
        with phase('p4rt-sdk'):
            build.collect_archives(ROOT, preparation, receipt, PREPARATION_TARGETS, bazel=args.bazel, options=options, use_execution_root=True)
            module = (preparation / 'common.MODULE.bazel').read_text()
            check.require(re.search(r'version\s*=\s*[\"\']0\.0\.9-' + COMMON_REVISION + r'[\"\']', module),
                          'The fetched Common owner differs from the selected image parent')
            prepare_repository(preparation / 'common-sdk.tar', sdk, 'amd64', {
                'producer_bazel_version': '8.5.1', 'producer_target': PREPARATION_TARGETS['common-sdk.tar'],
                'common_source_commit': COMMON_REVISION, 'common_source_has_tracked_changes': False,
                'common_module_sha256': check.sha(preparation / 'common.MODULE.bazel'),
                'image_source_revision': revision, 'yang_modules': True,
            }, libnl_archives={PREPARATION_TARGETS[name]: preparation / name
                               for name in ('libnl-nf-sdk.tar', 'libnl-genl-sdk.tar')},
                libyang_archive=preparation / 'libyang-runtime.tar',
                parent_archive=preparation / 'config-engine.source.tar')
            receipt['parent_common'] = p4rt_handoff.same_common_library(sdk,
                preparation / 'common-runtime.tar', preparation / 'config-engine.source.tar')
        with phase('p4rt-owner'):
            run_owner(args, sdk, output, 'build', preparation, receipt, make_build=make_build)
        with phase('p4rt-handoff'):
            receipt['owner_handoff'] = p4rt_handoff.stage(args.owner_source, args.owner_revision, sdk, output,
                ROOT / '.bazel-inputs/p4rt-owner', receipt['parent_common'])
            check.require(identity() == revision, 'Image source changed during owner preparation')
        # CI validation and publication accept only the clean-checkout 'prepared' receipt.
        receipt.update(status='prepared-for-make' if make_build else 'prepared', elapsed_seconds=time.monotonic() - started)
    except Exception as error:
        receipt.update(status='failed', error=str(error))
        raise
    finally:
        write(directory / 'owner-preparation.json', receipt)


def prepared(args):
    directory, preparation, sdk, output = locations(args)
    receipt = json.loads((directory / 'owner-preparation.json').read_text())
    check.require(receipt.get('status') == 'prepared' and receipt.get('revision') == source_identity(directory) and
                  receipt.get('owner_revision') == args.owner_revision, 'Owner preparation belongs to different inputs')
    for name, record in receipt['artifacts'].items():
        check.require(check.sha(preparation / name) == record['sha256'], 'Prepared artifact changed: ' + name)
    check.require(p4rt_handoff.verify_stage(ROOT / '.bazel-inputs/p4rt-owner', args.owner_source,
                  args.owner_revision, sdk, output) == receipt['owner_handoff'], 'Staged handoff metadata changed')
    check.require(p4rt_handoff.same_common_library(sdk, preparation / 'common-runtime.tar',
                  preparation / 'config-engine.source.tar') == receipt['parent_common'], 'Selected Common bytes changed')
    return directory, preparation, sdk, output, receipt


def runtime_inventory(preparation):
    selected = {}
    for name in RUNTIME_INPUTS:
        for path, item in check.payload(preparation / name).items():
            if path in selected:
                check.require(item == selected[path], 'Conflicting selected runtime payload: ' + path)
            selected[path] = item
    return selected


def runtime_libraries(destination, selected):
    expected = {}
    for name in selected:
        if '.so' not in Path(name).name:
            continue
        target = resolve_path(name, selected)
        item = selected.get(target, {})
        if item.get('kind') == 'file' and 'elf_machine' in item:
            expected[str(destination / name)] = item['sha256']
    actual = {}
    for path in destination.rglob('*.so*'):
        if not path.is_file():
            continue
        check.require(path.resolve().is_relative_to(destination), 'Prepared library escapes runtime root')
        with path.open('rb') as stream:
            if stream.read(4) == b'\x7fELF':
                actual[str(path)] = check.sha(path)
    check.require(actual and actual == expected, 'Prepared libraries differ from the declared runtime TARs')
    return dict(sorted(actual.items()))


def loaded_libraries(output, libraries):
    result = {}
    for match in re.finditer(r'^\s*(\S+) => (/\S+) \(', output, re.MULTILINE):
        name, path = match.groups()
        selected_hashes = {checksum for selected, checksum in libraries.items() if Path(selected).name == name}
        checksum = check.sha(Path(path))
        check.require(selected_hashes and selected_hashes == {checksum},
                      'Common resolved an ambient library instead of its declared runtime: ' + name)
        result[name] = checksum
    return result


def prepare_runtime(args):
    native()
    directory, preparation, sdk, output, receipt = prepared(args)
    # Use the selected Common owner's confinement filter for Debian absolute
    # symlinks. Extraction stays entirely in this private test root.
    spec = importlib.util.spec_from_file_location('selected_common_runtime', preparation / 'common-runtime-runner.py')
    common_runtime = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(common_runtime)
    destination = directory / 'owner-test-runtime'
    check.require(not destination.is_symlink(), 'Private runtime root must not be a link')
    if destination.exists():
        marker = destination / '.p4rt-runtime.json'
        check.require(marker.is_file() and json.loads(marker.read_text()).get('kind') == 'p4rt-owner-test-runtime',
                      'Refusing to replace an unmarked runtime directory')
        import shutil
        shutil.rmtree(destination)
    destination.mkdir()
    write(destination / '.p4rt-runtime.json', {'kind': 'p4rt-owner-test-runtime', 'status': 'preparing'})
    selected = runtime_inventory(preparation)
    for name in RUNTIME_INPUTS:
        with tarfile.open(preparation / name) as archive:
            archive.extractall(destination, filter=common_runtime.runtime_member_filter)
    libraries = runtime_libraries(destination, selected)
    directories = {str(Path(name).parent) for name in libraries}
    environment = {key: value for key, value in os.environ.items()
                   if key not in ('LD_LIBRARY_PATH', 'LD_PRELOAD', 'LD_AUDIT')}
    environment['LC_ALL'] = 'C'
    loaders = {path.resolve() for path in destination.rglob('ld-linux-x86-64.so.2') if path.is_file()}
    check.require(len(loaders) == 1, 'Expected one declared AMD64 runtime interpreter')
    loader = loaders.pop()
    check.require(loader.is_relative_to(destination), 'Runtime interpreter escapes the declared root')
    # Native Python/Bazel/tools keep their host runtime. Only the declared loader
    # inspects Common here; the owner uses Common's runner for actual ELF tests.
    loaded = subprocess.check_output([str(loader), '--library-path', ':'.join(sorted(directories)),
                                     '--list', str(sdk / 'lib/libswsscommon.so.0')],
                                     env=environment, text=True, timeout=30)
    check.require('not found' not in loaded, 'Prepared Common runtime is incomplete')
    patchelf = preparation / 'runtime-patchelf'
    check.require(check.sha(patchelf) == receipt['artifacts']['runtime-patchelf']['sha256'],
                  'Declared patchelf artifact changed')
    patchelf.chmod(0o555)
    common_libraries = loaded_libraries(loaded, libraries)
    runtime = {'schema': 1, 'architecture': 'amd64', 'kind': 'p4rt-owner-test-runtime',
               'sdk_inventory': p4rt_handoff.owner_api(args.owner_source, args.owner_revision).sdk_inventory(sdk),
               'library_path': sorted(directories), 'libraries': libraries,
               'input_artifacts': {name: check.sha(preparation / name) for name in RUNTIME_INPUTS},
               'loader': {'path': str(loader), 'sha256': check.sha(loader)},
               'common_runner': {'path': str(preparation / 'common-runtime-runner.py'),
                                 'sha256': check.sha(preparation / 'common-runtime-runner.py')},
               'patchelf': {'path': str(patchelf), 'sha256': check.sha(patchelf)},
               'runtime_archive': {'path': str(preparation / 'owner-test-runtime.tar'),
                                   'sha256': check.sha(preparation / 'owner-test-runtime.tar')},
               'common_ldd': common_libraries}
    write(destination / '.p4rt-runtime.json', runtime)
    write(directory / 'prepared-owner-runtime.json', runtime)
    p4rt_handoff.owner_api(args.owner_source, args.owner_revision).runtime_environment(
        directory / 'prepared-owner-runtime.json', sdk)


def build_images(args):
    native()
    options = root_options()
    directory, preparation, sdk, output, owner_receipt = prepared(args)
    receipt = {'status': 'building', 'architecture': 'amd64', 'commands': [],
               'revision': source_identity(directory), 'owner_revision': args.owner_revision,
               'owner_handoff': owner_receipt['owner_handoff']}
    execute = partial(command_log.execute, cwd=ROOT)
    try:
        prepare_manifests(execute, directory, receipt)
        layout_names = (*IMAGES, 'p4rt_with_symbols')
        layouts = [BASE, *[OWNER + name for name in layout_names]]
        receipt['action_audit'] = bazel_commands.audit_targets(args.bazel, options, [*TARGETS.values(), *layouts],
            workspace=ROOT, output=directory / 'action-audit.json')
        build.collect_archives(ROOT, directory, receipt, TARGETS, bazel=args.bazel, options=options, use_execution_root=True)
        execute([args.bazel, 'build', *options, *layouts], directory, receipt, 'layouts')
        receipt['layouts'] = {name: execute([args.bazel, 'cquery', *options, '--output=files', OWNER + name],
            directory, receipt, name + '.layout').strip() for name in layout_names}
        receipt['base_layout'] = execute([args.bazel, 'cquery', *options, '--output=files', BASE],
            directory, receipt, 'base.layout').strip()
        receipt['resolution'] = resolution.collect(ROOT, directory, bazel=[args.bazel])
        prepared(args)
        check.require(source_identity(directory) == receipt['revision'], 'Image checkout changed')
        receipt['status'] = 'built'
    except Exception as error:
        receipt.update(status='failed', error=str(error))
        raise
    finally:
        write(directory / 'build-receipt.json', receipt)


def owner_tests(args):
    native()
    directory, preparation, sdk, output, _ = prepared(args)
    runtime = directory / 'prepared-owner-runtime.json'
    check.require(runtime.is_file(), 'Run the explicit prepare-owner-runtime step before owner-tests')
    selected_runtime = json.loads(runtime.read_text())
    check.require(selected_runtime.get('input_artifacts') ==
                  {name: check.sha(preparation / name) for name in RUNTIME_INPUTS}, 'Runtime inputs differ from preparation')
    check.require(selected_runtime.get('libraries') == runtime_libraries(directory / 'owner-test-runtime',
                  runtime_inventory(preparation)), 'Prepared runtime receipt differs from declared payloads')
    receipt = {'status': 'testing', 'owner_revision': args.owner_revision, 'commands': []}
    try:
        run_owner(args, sdk, output, 'test', directory, receipt, runtime)
        prepared(args)
        receipt['status'] = 'passed'
    except Exception as error:
        receipt.update(status='failed', error=str(error))
        raise
    finally:
        write(directory / 'owner-tests.json', receipt)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('phase', choices=('prepare-owner', 'prepare-make-owner', 'prepare-owner-runtime', 'build', 'owner-tests'))
    parser.add_argument('--directory', required=True, type=Path)
    parser.add_argument('--bazel', default='bazel', help='Root image/Common Bazel 8.5.1 launcher')
    parser.add_argument('--owner-bazel', default='bazel', help='Separate owner Bazelisk or Bazel 6.4.0 launcher')
    parser.add_argument('--owner-source', required=True, type=Path)
    parser.add_argument('--owner-revision', required=True)
    parser.add_argument('--owner-output-base', required=True, type=Path)
    parser.add_argument('--jobs', type=int, default=1)
    args = parser.parse_args()
    check.require(args.jobs > 0, 'Owner jobs must be positive')
    actions = {'prepare-owner': prepare_owner, 'prepare-make-owner': partial(prepare_owner, make_build=True),
               'prepare-owner-runtime': prepare_runtime, 'build': build_images, 'owner-tests': owner_tests}
    with time_vs_image.log_make_phase('p4rt-prep', vs_make=args.phase == 'prepare-make-owner'):
        actions[args.phase](args)


if __name__ == '__main__':
    main()
