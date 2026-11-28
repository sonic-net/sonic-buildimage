#!/usr/bin/env python3
"""Check exact runtime/debug archives in disposable, network-isolated containers."""
import argparse
import hashlib
import io
import json
import platform
from pathlib import Path, PurePosixPath
import re
import subprocess
import sys
import tarfile
import tempfile
import uuid

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import artifact_validation

NATIVE_PLATFORMS = {'amd64': ('x86_64', 62), 'arm64': ('aarch64', 183)}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def docker(*args, timeout=300):
    result = subprocess.run(['docker', *args], capture_output=True, text=True, timeout=timeout)
    require(result.returncode == 0, 'Docker command failed: ' + repr(args[:3]) + '\n' + result.stderr[-4000:])
    return result.stdout


def safe_name(name):
    path = PurePosixPath(name)
    require(not path.is_absolute() and '..' not in path.parts, 'Unsafe Docker archive member: ' + name)
    return str(path)


def load_archive(path, tag, expected_config_digest):
    """Change only the temporary tag; do not overwrite the user's original tags."""
    manifest = None
    config = None
    with tempfile.TemporaryFile() as output, tempfile.TemporaryFile() as errors:
        process = subprocess.Popen(['docker', 'load', '--quiet'], stdin=subprocess.PIPE,
                                   stdout=output, stderr=errors)
        try:
            with tarfile.open(path, 'r|*') as source, tarfile.open(fileobj=process.stdin, mode='w|') as target:
                for member in source:
                    name = safe_name(member.name)
                    require(member.isfile() or member.isdir(), 'Unsupported Docker archive member')
                    if name == 'repositories':
                        continue  # Legacy tag mapping; the verified manifest supplies the unique tag.
                    if name == 'manifest.json':
                        require(manifest is None, 'Duplicate Docker manifest')
                        records = json.load(source.extractfile(member))
                        require(isinstance(records, list) and len(records) == 1, 'Expected exactly one image')
                        manifest = records[0]
                        safe_name(manifest['Config'])
                        for layer in manifest['Layers']:
                            safe_name(layer)
                        manifest['RepoTags'] = [tag]
                        content = json.dumps(records).encode()
                        member.size = len(content)
                        target.addfile(member, io.BytesIO(content))
                    elif member.isfile() and member.size <= 16 * 1024 * 1024:
                        content = source.extractfile(member).read()
                        if 'sha256:' + hashlib.sha256(content).hexdigest() == expected_config_digest:
                            require(config is None, 'Duplicate image configuration')
                            config = json.loads(content)
                        target.addfile(member, io.BytesIO(content))
                    else:
                        target.addfile(member, source.extractfile(member) if member.isfile() else None)
            process.stdin.close()
            require(process.wait(timeout=300) == 0, 'Docker could not load the validation archive')
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
        errors.seek(0)
        diagnostics = errors.read().decode(errors='replace')
    require(manifest is not None and config is not None, 'Docker archive lacks its expected manifest/configuration')
    return json.loads(docker('image', 'inspect', tag))[0], config, diagnostics


def verify_loaded_image(image, config, expected_digest):
    # Docker's containerd-backed store exposes a manifest digest as Id, whereas
    # the classic store exposes the configuration digest. Check actual content
    # in either case; do not equate these two different digests.
    descriptor = image.get('Descriptor', {})
    require(image['Id'] == expected_digest or (
        descriptor.get('digest') == image['Id'] and descriptor.get('mediaType') in (
            'application/vnd.docker.distribution.manifest.v2+json',
            'application/vnd.oci.image.manifest.v1+json')), 'Unexpected Docker image identity')
    meaningful = lambda values: {key: value for key, value in values.items()
                                  if value not in (None, '', [], {}, False)}
    require(meaningful(image['Config']) == meaningful(config.get('config', {})),
            'Loaded image execution configuration differs from the archive')
    require(image['RootFS']['Type'] == config['rootfs']['type'] and
            image['RootFS']['Layers'] == config['rootfs']['diff_ids'],
            'Loaded filesystem layers differ from the archive')
    require((image['Os'], image['Architecture']) == (config['os'], config['architecture']),
            'Loaded image platform differs from the archive')


def probe_input(report, policy):
    native = report['native']
    rows = (native['paired_symbols'] + native['embedded_symbols']
            + native['preserved_make_debug_gaps'] + native['inherited_base_symbol_pairs'])
    require(rows, 'Static validation report has no native ELF inventory')
    by_path = {}
    for row in rows:
        require(row['path'] not in by_path, 'Duplicate native ELF path: ' + row['path'])
        safe_name(row['path'])
        by_path[row['path']] = row
    pairs = native['paired_symbols'] + native['inherited_base_symbol_pairs']
    require(pairs, 'Static validation report has no debug pairs')
    return {'architecture': policy['architecture'], 'elfs': list(by_path.values()),
            'debug_pairs': pairs, 'policy': policy}


def container_arguments(image, name, probe, writable, redis_server=None, *, variant='runtime'):
    require(variant in ('runtime', 'debug'), 'Unknown installed-image variant')
    # GDB expands large P4 compiler symbol tables beyond the runtime probe budget.
    memory = '8g' if variant == 'debug' else '4g'
    args = ['run', '--rm', '--name', name, '--interactive', '--network=none',
            '--cap-drop=ALL', '--security-opt=no-new-privileges', '--read-only',
            '--pids-limit=128', '--memory=' + memory, '--cpus=2',
            '--mount', 'type=bind,src=' + str(probe.resolve()) + ',dst=/probe/runtime_smoke.py,readonly']
    if redis_server is not None:
        args += ['--mount', 'type=bind,src=' + str(redis_server.resolve()) + ',dst=/probe/redis-server,readonly',
                 '--mount', 'type=volume,dst=/run/redis']
    for path in ['/tmp', '/var/log', '/var/tmp', *writable]:
        require(path.startswith('/') and path != '/' and '..' not in PurePosixPath(path).parts,
                'Unsafe writable image path')
        require(not any(path == prefix or path.startswith(prefix + '/') for prefix in ['/dev', '/proc', '/sys']),
                'Writable kernel paths are not allowed')
        args += ['--tmpfs', path + ':rw,nosuid,nodev,noexec,size=128m']
    return args + ['--entrypoint=/usr/bin/python3', image, '/probe/runtime_smoke.py']


def check_images_policy(policy):
    """Keep image relationships and installed paths explicit, before loading anything."""
    require(isinstance(policy, dict) and set(policy) <= {'architecture', 'images', 'symbol_pairs'},
            'Unknown installed-images policy fields')
    architecture = policy.get('architecture')
    require(architecture in NATIVE_PLATFORMS, 'Unsupported image architecture')
    require(platform.machine() == NATIVE_PLATFORMS[architecture][0],
            'Native host architecture differs from the image policy')
    require(isinstance(policy.get('images'), list) and policy['images'], 'No images declared')
    names, tags, archives = {}, set(), set()
    for spec in policy['images']:
        require(isinstance(spec, dict) and set(spec) <= {
            'name', 'archive', 'tag', 'parent', 'files', 'directory_aliases', 'inherit_files',
            'archive_sha256', 'config_digest', 'expected_config', 'expected_rootfs'},
            'Unknown image policy fields')
        name, tag, archive = spec.get('name'), spec.get('tag'), spec.get('archive')
        require(isinstance(name, str) and name and name not in names, 'Duplicate or empty image name')
        require(isinstance(tag, str) and tag and tag not in tags, 'Duplicate or empty image tag')
        require(isinstance(archive, str) and archive and Path(archive).name == archive and
                archive not in ('.', '..') and archive not in archives, 'Invalid or duplicate image archive')
        require(re.fullmatch(r'[0-9a-f]{64}', spec.get('archive_sha256', '')) is not None and
                re.fullmatch(r'sha256:[0-9a-f]{64}', spec.get('config_digest', '')) is not None and
                isinstance(spec.get('expected_config'), dict) and
                isinstance(spec.get('expected_rootfs'), dict), 'Missing static image/archive identity')
        files, aliases = spec.get('files', {}), spec.get('directory_aliases', {})
        require(isinstance(files, dict) and isinstance(aliases, dict), 'Invalid installed-file expectations')
        for path in [*files, *aliases]:
            require(isinstance(path, str) and safe_name(path) == path and path != '.',
                    'Noncanonical installed path: ' + str(path))
        for expected in files.values():
            require(isinstance(expected, dict) and expected.keys() <= {'kind', 'sha256', 'mode', 'uid', 'gid'},
                    'Unknown installed-file metadata')
        require(all(isinstance(target, str) and target for target in aliases.values()),
                'Invalid directory-alias target')
        parent = spec.get('parent')
        require(parent is None or parent in names, 'Image parent must appear before its child')
        inherited = spec.get('inherit_files', [])
        require(isinstance(inherited, list) and len(inherited) == len(set(inherited)) and
                set(inherited) <= files.keys(), 'Inherited paths must be declared installed files')
        require(not inherited or (parent is not None and
                set(inherited) <= names[parent].get('files', {}).keys()),
                'Inherited paths must be declared by the parent image')
        names[name] = spec
        tags.add(tag)
        archives.add(archive)
    pairs = policy.get('symbol_pairs', [])
    require(isinstance(pairs, list), 'Invalid symbol pairs')
    for pair in pairs:
        require(isinstance(pair, dict) and set(pair) == {'runtime', 'debug', 'paths'}, 'Invalid symbol pair')
        require(pair['runtime'] in names and pair['debug'] in names, 'Symbol pair names an unknown image')
        paths = pair['paths']
        require(isinstance(paths, list) and paths and len(paths) == len(set(paths)) and
                set(paths) <= names[pair['runtime']].get('files', {}).keys(),
                'Symbol paths must be declared runtime files')


def merge_images_policies(policies):
    """Join owner policies built in their native configurations, binding shared images."""
    require(policies, 'No installed-images policies provided')
    merged, names = None, {}
    for policy in policies:
        require(isinstance(policy, dict), 'Invalid installed-images policy')
        if merged is None:
            merged = {'architecture': policy.get('architecture'), 'images': [], 'symbol_pairs': []}
        require(policy.get('architecture') == merged['architecture'],
                'Installed-images policies have different architectures')
        check_images_policy(policy)
        for spec in policy['images']:
            name = spec['name']
            if name in names:
                require(spec == names[name], 'Installed-images policies disagree on shared image: ' + name)
            else:
                names[name] = spec
                merged['images'].append(spec)
        for pair in policy.get('symbol_pairs', []):
            if pair not in merged['symbol_pairs']:
                merged['symbol_pairs'].append(pair)
    check_images_policy(merged)
    return merged


def verify_image_reports(policy, images, reports):
    """Compare loaded ancestry and effective files with owner declarations."""
    for spec in policy['images']:
        name = spec['name']
        image, report = images[name], reports[name]
        require((image.get('Os'), image.get('Architecture')) == ('linux', policy['architecture']),
                'Loaded image platform differs: ' + name)
        parent = spec.get('parent')
        if parent:
            layers = images[parent]['RootFS']['Layers']
            require(layers and image['RootFS']['Layers'][:len(layers)] == layers,
                    "Image does not retain its parent's exact layers: " + name)
        require(report['files'].keys() == spec.get('files', {}).keys() and
                report['directory_aliases'] == spec.get('directory_aliases', {}),
                'Installed paths or directory aliases differ: ' + name)
        for path, expected in spec.get('files', {}).items():
            actual = report['files'][path]
            require(actual.get('kind') == 'file' and
                    all(actual.get(key) == value for key, value in expected.items()),
                    'Installed file metadata differs: ' + name + ':' + path)
        for path in spec.get('inherit_files', []):
            require(report['files'][path] == reports[parent]['files'][path],
                    'Image changed an inherited file: ' + name + ':' + path)


def validate_images(policy, target):
    """Load named archives with standard Docker in a private validation daemon.

    This intentionally uses the archive's existing tags. Callers must provide a
    disposable/private daemon; only this invocation's inspection containers are
    removed. Static OCI checks and service/boot probes remain separate.
    """
    check_images_policy(policy)
    images, reports, containers = {}, {}, {}
    probe = ROOT / 'tools/bazel/oci/runtime_smoke.py'
    paths = {spec['name']: target / spec['archive'] for spec in policy['images']}
    require(all(path.is_file() for path in paths.values()), 'A declared image archive is missing')
    try:
        for spec in policy['images']:
            name = spec['name']
            require(sha(paths[name]) == spec['archive_sha256'],
                    'Image archive differs from its static validation: ' + name)
            docker('load', '--input', str(paths[name]))
            image = json.loads(docker('image', 'inspect', spec['tag']))[0]
            verify_loaded_image(image, {'os': 'linux', 'architecture': policy['architecture'],
                                'config': spec['expected_config'], 'rootfs': spec['expected_rootfs']},
                                spec['config_digest'])
            images[name] = image
            inputs = {'files': list(spec.get('files', {})),
                      'directory_aliases': list(spec.get('directory_aliases', {}))}
            container = docker(
                'create', '--name', 'sonic-image-check-' + uuid.uuid4().hex,
                '--network=none', '--read-only', '--cap-drop=ALL',
                '--security-opt=no-new-privileges', '--pids-limit=32', '--memory=256m',
                '--mount', 'type=bind,src=' + str(probe) + ',dst=/probe/runtime_smoke.py,readonly',
                '--entrypoint=/usr/bin/python3', image['Id'], '-B', '/probe/runtime_smoke.py',
                '--inspect-files', json.dumps(inputs),
            ).strip()
            containers[name] = container
            report = docker('start', '--attach', container)
            state = json.loads(docker('inspect', container))[0]['State']
            require(state['ExitCode'] == 0, 'Installed-image file probe failed: ' + name)
            reports[name] = json.loads(report)
        verify_image_reports(policy, images, reports)
        pairs = []
        with tempfile.TemporaryDirectory(prefix='installed-image-symbols-') as temporary:
            for pair in policy.get('symbol_pairs', []):
                for name in pair['paths']:
                    extracted = Path(temporary) / (str(len(pairs)))
                    binary = extracted / name
                    binary.parent.mkdir(parents=True)
                    docker('cp', containers[pair['runtime']] + ':/' + name, str(binary))
                    require(sha(binary) == reports[pair['runtime']]['files'][name]['sha256'],
                            'Copied ELF differs from its runtime image: ' + name)
                    notes = artifact_validation.command(['readelf', '-n', str(binary)])
                    match = re.search(r'Build ID: ([0-9a-f]+)', notes)
                    require(match is not None, 'Installed ELF lacks a build ID: ' + name)
                    identifier = match[1]
                    debug_name = 'usr/lib/debug/.build-id/' + identifier[:2] + '/' + identifier[2:] + '.debug'
                    symbols = extracted / debug_name
                    symbols.parent.mkdir(parents=True, exist_ok=True)
                    docker('cp', containers[pair['debug']] + ':/' + debug_name, str(symbols))
                    require(not symbols.is_symlink() and symbols.is_file(),
                            'Installed debug symbol is not a regular file: ' + debug_name)
                    with binary.open('rb') as source:
                        metadata = artifact_validation.file_metadata(source, name)
                    require(metadata.get('elf_machine') == NATIVE_PLATFORMS[policy['architecture']][1],
                            'Installed ELF architecture differs: ' + name)
                    checked, _ = artifact_validation.elf_debug(extracted, {name: metadata, debug_name: {}}, set())
                    pairs.extend({**item, 'runtime': pair['runtime'], 'debug': pair['debug']} for item in checked)
        return {'status': 'passed', 'architecture': policy['architecture'],
                'scope': 'Docker-loaded files, image inheritance and split symbols; no service boot',
                'images': {name: {'id': image['Id'], 'archive_sha256': sha(paths[name]), **reports[name]}
                           for name, image in images.items()}, 'debug_pairs': pairs}
    finally:
        for container in containers.values():
            subprocess.run(['docker', 'rm', '--force', container], check=False, timeout=30,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def validate(runtime, debug, report_path, policy_path, output, redis_server=None):
    static = json.loads(report_path.read_text())
    policy = json.loads(policy_path.read_text())
    require(policy['architecture'] in ('amd64', 'arm64'), 'Unsupported smoke architecture')
    host_machine = platform.machine()
    require(host_machine == {'amd64': 'x86_64', 'arm64': 'aarch64'}[policy['architecture']],
            'Native host architecture differs from the image policy')
    inputs = probe_input(static, policy)
    script_source = Path(__file__).resolve().parents[1] / 'oci/runtime_smoke.py'
    probe_directory = tempfile.TemporaryDirectory(prefix='sonic-runtime-probe-')
    script = Path(probe_directory.name) / 'runtime_smoke.py'
    script.write_bytes(script_source.read_bytes())
    script.chmod(0o444)
    redis_copy = None
    if policy.get('redis_database_config'):
        require(redis_server is not None and redis_server.is_file(), 'This policy requires --redis-server as an explicit test helper')
        redis_copy = Path(probe_directory.name) / 'redis-server'
        redis_copy.write_bytes(redis_server.read_bytes())
        redis_copy.chmod(0o555)
        inputs['redis_fixture'] = {'executable': '/probe/redis-server', 'sha256': sha(redis_copy),
                                   'database_config': policy['redis_database_config']}
    record = {'_generated': {'notice': 'AUTO-GENERATED. DO NOT EDIT MANUALLY.',
                             'generator': 'tools/bazel/ci/validate_runtime.py'},
              'schema': 1, 'static_validation_sha256': sha(report_path),
              'policy_sha256': sha(policy_path), 'probe_sha256': sha(script),
              'architecture': policy['architecture'], 'host_machine': host_machine, 'images': {},
              'preserved_debug_gaps': static['native']['preserved_make_debug_gaps'],
              'scope': 'Isolated installed-image loading, CLI/import/startup probes and GDB metadata lookup; no guest boot or forwarding.'}
    output.parent.mkdir(parents=True, exist_ok=True)
    try:
        for variant, archive in [('runtime', runtime), ('debug', debug)]:
            expected = static['images']['archives'][variant]
            digest = sha(archive)
            require(digest == expected['sha256'] and archive.stat().st_size == expected['bytes'],
                    variant + ' archive differs from the static validation report')
            token = uuid.uuid4().hex
            tag, name = 'sonic-runtime-validation:' + token, 'sonic-runtime-validation-' + token
            try:
                print('Checking ' + variant + ' archive ' + digest, flush=True)
                image, config, diagnostics = load_archive(archive, tag, expected['config_digest'])
                verify_loaded_image(image, config, expected['config_digest'])
                require(image['Architecture'] == policy['architecture'] and image['Os'] == 'linux', 'Loaded platform differs')
                args = container_arguments(image['Id'], name, script, policy.get('writable_paths', []), redis_copy, variant=variant) + ['--variant', variant]
                result = subprocess.run(['docker', *args], input=json.dumps(inputs), text=True,
                                        capture_output=True, timeout=1800)
                try:
                    probes = json.loads(result.stdout)
                except ValueError as error:
                    raise ValueError('Image probe did not return JSON: ' + result.stderr[-4000:]) from error
                record['images'][variant] = {'archive_sha256': digest, 'config_digest': expected['config_digest'], 'docker_image_id': image['Id'],
                                             'docker_platform': image['Os'] + '/' + image['Architecture'],
                                             'probe_exit': result.returncode, 'probes': probes,
                                             'isolation': {'network': 'none', 'read_only_root': True,
                                                           'capabilities': [], 'no_new_privileges': True}}
                output.write_text(json.dumps(record, indent=2, sort_keys=True) + '\n')
            finally:
                # Remove only this invocation's uniquely named container/tag.
                subprocess.run(['docker', 'rm', '--force', '--volumes', name], capture_output=True, timeout=30)
                subprocess.run(['docker', 'image', 'rm', tag], capture_output=True, timeout=60)
        record['passed'] = all(x['probe_exit'] == 0 and x['probes']['passed'] for x in record['images'].values())
    finally:
        output.write_text(json.dumps(record, indent=2, sort_keys=True) + '\n')
        probe_directory.cleanup()
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['runtime', 'debug', 'static-report', 'policy']:
        parser.add_argument('--' + name, type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--images-policy', type=Path, action='append',
                        help='Named archives and installed-file policy; repeat for separately configured owners; requires a private Docker daemon')
    parser.add_argument('--target', type=Path, default=Path('target'))
    parser.add_argument('--redis-server', type=Path, help='Existing native Redis binary used only as an isolated test helper; its bytes/version are recorded')
    args = parser.parse_args()
    if args.images_policy is not None:
        if any((args.runtime, args.debug, args.static_report, args.policy, args.redis_server)):
            parser.error('--images-policy cannot be combined with runtime-service validation arguments')
        policy = merge_images_policies([json.loads(path.read_text()) for path in args.images_policy])
        result = validate_images(policy, args.target)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + '\n')
        print(json.dumps(result, sort_keys=True))
        return 0
    if not all((args.runtime, args.debug, args.static_report, args.policy)):
        parser.error('runtime-service validation requires --runtime, --debug, --static-report and --policy')
    result = validate(args.runtime, args.debug, args.static_report, args.policy, args.output, args.redis_server)
    print(json.dumps({'passed': result['passed'], 'report': str(args.output),
                      'failures': {k:v['probes']['failures'] for k,v in result['images'].items()}}, indent=2))
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
