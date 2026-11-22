#!/usr/bin/env python3
"""Publish and verify kernel DEBs built in the root Bazel module graph."""

import argparse
import ast
import fcntl
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel import build_helpers
from tools.bazel.kernel import remote_bundle

TARGET = '@sonic_linux_kernel//:kernel_packages'
MANIFEST = 'kernel-packages.json'
PROVENANCE = 'kernel-provenance.json'


def sha256(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def module_version(workspace):
    for node in ast.parse((workspace / 'MODULE.bazel').read_text()).body:
        if isinstance(node, ast.Expr) and isinstance(node.value, ast.Call):
            call = node.value
            if isinstance(call.func, ast.Name) and call.func.id == 'bazel_dep':
                values = {item.arg: ast.literal_eval(item.value) for item in call.keywords}
                if values.get('name') == 'sonic-linux-kernel':
                    return values['version']
    raise ValueError('Root MODULE.bazel must declare sonic-linux-kernel')


def output_files(paths):
    """Read sonic_deb directory outputs in place, keeping binary basenames."""
    outputs = []
    for path in paths:
        if path.is_dir():
            members = list(path.iterdir())
            if (len(members) != 1 or members[0].suffix != '.deb'
                    or members[0].is_symlink() or not members[0].is_file()):
                raise ValueError('Bazel package directory must contain exactly one regular DEB')
            path = members[0]
        if not path.is_file():
            raise ValueError('Bazel kernel output must be a regular file')
        outputs.append(path)
    if len({path.name for path in outputs}) != len(outputs):
        raise ValueError('Bazel kernel outputs contain duplicate basenames')
    return outputs


def check_packages(directory, architecture=None, kernel_config=None, package_paths=None):
    manifest = json.loads((directory / MANIFEST).read_text())
    if architecture and manifest['architecture'] != architecture:
        raise ValueError('Kernel package architecture differs from request')
    if kernel_config:
        for name, value in kernel_config.items():
            if manifest['kernel_config'].get(name) != value:
                raise ValueError('Kernel configuration differs: ' + name)
    entries = manifest['packages']
    if len(entries) != 5 or {item['kind'] for item in entries} != {'runtime', 'debug', 'headers', 'headers_common', 'kbuild'}:
        raise ValueError('Kernel manifest must declare all five Debian packages')
    for item in entries:
        name = item['name']
        if Path(name).name != name or not name.endswith('.deb'):
            raise ValueError('Invalid kernel package filename')
        path = package_paths.get(name) if package_paths is not None else directory / name
        if path is None:
            raise ValueError('Bazel kernel outputs differ from manifest')
        if path.stat().st_size != item['size'] or sha256(path) != item['sha256']:
            raise ValueError('Kernel package digest differs: ' + name)
    return manifest


def verify(directory, workspace, architecture=None, kernel_config=None):
    directory, workspace = Path(directory), Path(workspace)
    manifest = check_packages(directory, architecture, kernel_config)
    provenance = json.loads((directory / PROVENANCE).read_text())
    if provenance.get('schema') != 3 or provenance.get('target') != TARGET:
        raise ValueError('Unsupported direct Bazel kernel provenance')
    if provenance['module_version'] != module_version(workspace):
        raise ValueError('Published kernel module differs from root MODULE.bazel')
    if provenance['manifest_sha256'] != sha256(directory / MANIFEST):
        raise ValueError('Kernel manifest differs from publication record')
    return {'manifest': manifest, 'provenance': provenance}


def publish(paths, directory, workspace, architecture, kernel_config, *, resolution=None):
    paths = output_files(paths)
    manifests = [path for path in paths if path.name == MANIFEST]
    if len(manifests) != 1:
        raise ValueError('Bazel must report exactly one kernel manifest')
    source = manifests[0].parent
    manifest = check_packages(source, architecture,
                              package_paths={path.name: path for path in paths})
    expected = {item['name'] for item in manifest['packages']} | {MANIFEST}
    if {path.name for path in paths} != expected:
        raise ValueError('Bazel kernel outputs differ from manifest')
    version = module_version(workspace)
    commit = version.rsplit('-', 1)[-1]
    if len(commit) != 40 or any(c not in '0123456789abcdef' for c in commit):
        raise ValueError('Registry kernel version must identify its source commit')
    provenance = {
        'schema': 3, 'target': TARGET, 'module_version': version,
        'kernel_gitlink': commit,
        'source_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=workspace, text=True).strip(),
        'launcher': {'architecture': architecture},
        'manifest_sha256': sha256(source / MANIFEST),
    }
    if resolution is not None:
        provenance['resolution'] = resolution
    # Validate every input before publishing. The lock serializes Make callers.
    for path in paths:
        build_helpers.export_archive(path, directory / path.name)
    destination = directory / PROVENANCE
    if destination.is_file():
        previous = json.loads(destination.read_text())
        if (previous.get('schema') == 3 and previous.get('module_version') == version
                and previous.get('manifest_sha256') == provenance['manifest_sha256']):
            provenance['source_commit'] = previous['source_commit']
    encoded = json.dumps(provenance, indent=2, sort_keys=True) + '\n'
    if not destination.exists() or destination.read_text() != encoded:
        with tempfile.NamedTemporaryFile(mode='w', dir=directory, delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(encoded)
        try:
            temporary.replace(destination)
        finally:
            temporary.unlink(missing_ok=True)
    return verify(directory, workspace, architecture, manifest["kernel_config"])


def verify_make_contract(directory, result, expected, packages):
    if any(result['manifest'].get(field) != value for field, value in expected.items()):
        raise ValueError('Kernel manifest differs from the requested Make configuration')
    provenance = result['provenance']
    if (provenance['kernel_gitlink'] != provenance['module_version'].rsplit('-', 1)[-1]
            or provenance['launcher']['architecture'] != expected['architecture']):
        raise ValueError('Kernel provenance differs from the requested producer')
    for entry in result['manifest']['packages']:
        name = entry['name']
        metadata = packages[name]
        if any(entry.get(k) != v for k, v in metadata.items()):
            raise ValueError('Kernel DEB metadata differs from the Make contract: ' + name)
        control = subprocess.check_output(
            ['dpkg-deb', '--field', str(directory / name), 'Package', 'Version', 'Architecture'], text=True)
        if dict(line.split(': ', 1) for line in control.splitlines()) != {
            field.title(): metadata[field] for field in ('package', 'version', 'architecture')
        }:
            raise ValueError('Kernel DEB control fields differ from the Make contract: ' + name)


def restore_root(workspace, directory, architecture, config, endpoint, key, current_key):
    from tools.bazel.kernel import prepare, root_resolution

    expected, packages = prepare.contract(workspace, architecture, config)
    with tempfile.TemporaryDirectory(prefix='.kernel-restore-', dir=directory) as staging:
        incoming = Path(staging)
        index = remote_bundle.restore_bundle(
            workspace, incoming, set(packages), endpoint, key=key, schema=remote_bundle.ROOT_SCHEMA,
            validate_lock=lambda data: root_resolution.validate(data, workspace))
        verified = verify(incoming, workspace, architecture)
        verify_make_contract(incoming, verified, expected, packages)
        if current_key() != key:
            raise ValueError('Kernel root inputs changed during cache restore')
        remote_bundle.save_index(workspace, index, remote_bundle.RESTORED_INDEX)
        prepare.install_files(incoming, directory, set(packages) | {MANIFEST})
    return index


def build_packages(workspace, directory, architecture, config, bazel, options, endpoint, upload):
    from tools.bazel.kernel import prepare, root_request, root_resolution

    def current_key():
        return root_request.request_key(workspace, architecture, config, options=options, bazel=bazel)

    opted_in = endpoint and os.environ.get('SONIC_VS_KERNEL_REMOTE_REUSE') == '1'
    candidate = workspace / remote_bundle.INPUT_INDEX
    key = None
    if opted_in and (upload or candidate.is_file()):
        try:
            key = current_key()
            if candidate.is_file():
                index = restore_root(workspace, directory, architecture, config, endpoint, key, current_key)
                if upload:
                    try:
                        remote_bundle.save_index(workspace, index)
                    except OSError as error:
                        print(f'Could not retain the kernel cache index: {error}', file=sys.stderr)
                print('Restored five verified kernel DEBs from the root Bazel cache.')
                return
        except remote_bundle.RESTORE_ERRORS as error:
            print(f'Kernel root cache unavailable; using Bazel: {error}', file=sys.stderr)

    (workspace / remote_bundle.RESTORED_INDEX).unlink(missing_ok=True)
    subprocess.run([bazel, 'build', *options, TARGET], cwd=workspace, check=True)
    resolution = None
    lock_path = workspace / 'MODULE.bazel.lock'
    if lock_path.exists():
        # Another target or the following cquery may update the shared root lock.
        # Retain what the build actually resolved, including for read-only PR jobs.
        resolution = {'lock_sha256': sha256(prepare.regular(lock_path))}
        prepare.retain_resolution(workspace, directory, resolution, prepare.ROOT_RESOLUTION_RECORDS)
    if resolution and key and upload:
        try:
            with (directory / prepare.RESOLUTION_DIRECTORY / resolution['lock_sha256']).open('rb') as stream:
                lock = stream.read(remote_bundle.cache_identity.MAX_LOCK_BYTES + 1)
            root_resolution.validate(lock, workspace)
            resolution['root_request_key'] = key
        except remote_bundle.RESTORE_ERRORS as error:
            print(f'Kernel root index publication skipped: {error}', file=sys.stderr)
    output = subprocess.check_output([bazel, 'cquery', *options, '--output=files', TARGET], cwd=workspace, text=True)
    verified = publish([workspace / line for line in output.splitlines()], directory, workspace,
                       architecture, config, resolution=resolution)
    if resolution and 'root_request_key' in resolution:
        try:
            expected, packages = prepare.contract(workspace, architecture, config)
            verify_make_contract(directory, verified, expected, packages)
            if current_key() != key:
                raise ValueError('Kernel root inputs changed during the Bazel build')
            index = remote_bundle.upload_bundle(directory, lock, verified, endpoint, key=key, schema=remote_bundle.ROOT_SCHEMA)
            remote_bundle.save_index(workspace, index)
        except remote_bundle.RESTORE_ERRORS as error:
            print(f'Kernel root index publication skipped: {error}', file=sys.stderr)
    print('Kernel DEBs published from the root Bazel graph.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=['build', 'verify'])
    parser.add_argument('--workspace', type=Path, default=Path.cwd())
    parser.add_argument('--debs-directory', type=Path, default=Path('target/debs/trixie'))
    parser.add_argument('--architecture', choices=['amd64', 'arm64'])
    parser.add_argument('--kernel-config', action='append', default=[])
    parser.add_argument('--cache-directory', type=Path)
    parser.add_argument('--remote-cache')
    parser.add_argument('--upload-local-results', action='store_true')
    parser.add_argument('--vs-handoff-sha256', help='consume the verified same-run VS artifact without rebuilding it')
    args = parser.parse_args()
    workspace = args.workspace.resolve()
    directory = args.debs_directory.resolve()
    config = dict(item.split('=', 1) for item in args.kernel_config)
    host_architecture = {'x86_64': 'amd64', 'aarch64': 'arm64'}[os.uname().machine]
    args.architecture = args.architecture or config.get('CONFIGURED_ARCH') or host_architecture
    if args.command == 'verify' and args.vs_handoff_sha256 is None:
        verify(directory, workspace, args.architecture, config)
        print('Kernel DEBs and root module provenance verified.')
        return
    if args.vs_handoff_sha256 is None:
        directory.mkdir(parents=True, exist_ok=True)
    with (directory / '.bazel-kernel.lock').open('a') as lock:
        if args.vs_handoff_sha256 is not None:
            from tools.bazel.ci import kernel_artifacts
            fcntl.flock(lock, fcntl.LOCK_SH)
            kernel_artifacts.verify_make_handoff(workspace, directory, args.architecture, config,
                                                args.vs_handoff_sha256)
            print('Make consumed the verified same-run VS kernel DEBs.')
            return
        fcntl.flock(lock, fcntl.LOCK_EX)
        platform = '@sonic_build_infra//platforms:' + {'amd64': 'x86_64', 'arm64': 'aarch64'}[args.architecture] + '_trixie'
        host_platform = '@sonic_build_infra//platforms:' + {'amd64': 'x86_64', 'arm64': 'aarch64'}[host_architecture] + '_trixie'
        options = ['--platforms=' + platform, '--host_platform=' + host_platform,
                   '--@sonic_linux_kernel//tools/bazel:kernel_config=' + json.dumps(config, sort_keys=True)]
        if args.cache_directory:
            options += build_helpers.cache_options(args.cache_directory / "kernel-linux-sandbox")
        options += build_helpers.remote_cache_options()
        if args.remote_cache:
            options += ['--remote_cache=' + args.remote_cache,
                        '--remote_upload_local_results=' + str(args.upload_local_results).lower()]
        bazel = os.environ.get('BAZEL', 'bazel')
        build_packages(workspace, directory, args.architecture, config, bazel, options,
                       args.remote_cache, args.upload_local_results)


if __name__ == '__main__':
    main()
