#!/usr/bin/env python3
"""Check exact runtime/debug archives in disposable, network-isolated containers."""
import argparse
import hashlib
import io
import json
import platform
from pathlib import Path, PurePosixPath
import subprocess
import tarfile
import tempfile
import uuid


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
    for name in ['runtime', 'debug', 'static-report', 'policy', 'output']:
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--redis-server', type=Path, help='Existing native Redis binary used only as an isolated test helper; its bytes/version are recorded')
    args = parser.parse_args()
    result = validate(args.runtime, args.debug, args.static_report, args.policy, args.output, args.redis_server)
    print(json.dumps({'passed': result['passed'], 'report': str(args.output),
                      'failures': {k:v['probes']['failures'] for k,v in result['images'].items()}}, indent=2))
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
