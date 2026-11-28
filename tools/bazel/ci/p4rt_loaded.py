#!/usr/bin/env python3
"""Load accepted P4RT archives and exercise their installed startup and RPCs."""

import argparse
import json
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import uuid

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci.vs_probe_policy import P4RT_OWNER_TEST_INPUTS, selected_images, selected_artifacts
from tools.bazel.ci import artifact_validation as check
from tools.bazel.ci import p4rt_image, validate_runtime
from tools.bazel.ci.public_artifacts import safe_json


def verify_download(directory, artifacts, release_inputs):
    check.require(set(artifacts) == selected_artifacts(directory, p4rt_image.TARGETS), 'Incomplete downloaded P4RT artifacts')
    for name, record in artifacts.items():
        source = directory / name
        check.require(not source.is_symlink() and check.sha(source) == record['sha256'],
                      'Accepted image artifact changed: ' + name)
    if release_inputs:
        # These two downloaded TARs replicate layers in the archives about to
        # be loaded. The original native job artifacts stay available in Actions.
        for name in ('p4rt.debug.tar', 'p4rt.source.tar'):
            (directory / name).unlink()


def owner_package_probe(tag, container_name, source, directory):
    """Exercise PINS' native runtime/debug TAR interface on the VS validator."""
    command = ['docker', 'run', '--rm', '--name', container_name, '--network=none', '--cap-drop=ALL',
               '--security-opt=no-new-privileges', '--read-only', '--pids-limit=256',
               '--memory=8g', '--cpus=2', '--tmpfs', '/tmp:rw,nosuid,nodev,exec,size=6g,mode=1777']
    mounts = {
        'owner_probe.py': ROOT / 'dockers/docker-sonic-p4rt/bazel/owner_probe.py',
        'source': source,
        **{name: directory / name for name in ('p4rt.source.tar', *P4RT_OWNER_TEST_INPUTS)},
    }
    for name, path in mounts.items():
        command += ['--mount', f'type=bind,src={path},dst=/probe/{name},readonly']
    command += ['--entrypoint', '/usr/bin/python3', tag, '-B', '/probe/owner_probe.py']
    return json.loads(subprocess.check_output(command, text=True, timeout=1800))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, required=True)
    parser.add_argument('--owner-source', type=Path, required=True)
    parser.add_argument('--release-downloaded-inputs', action='store_true',
                        help='Remove the two verified intermediate P4RT TARs before loading the installed images')
    args = parser.parse_args()
    directory = args.directory.resolve()
    revision = p4rt_image.source_identity(directory)
    report_path = directory / 'validation.json'
    report = json.loads(report_path.read_text())
    check.require(report.get('status') == 'passed' and report.get('architecture') == 'amd64',
                  'P4RT static validation did not pass')
    check.require(report['revision'] == revision, 'Loaded checkout differs from the built source')
    owner_source = args.owner_source.resolve()
    p4rt_image.verify_owner_source(owner_source, report['owner_revision'])
    verify_download(directory, report['artifacts'], args.release_downloaded_inputs)

    results = {}
    with tempfile.TemporaryDirectory(prefix='p4rt-redis-') as temporary:
        fixture = Path(temporary)
        with tarfile.open(directory / 'test-services.tar') as archive:
            archive.extractall(fixture, filter='data')
        fixture.chmod(0o755)
        with tarfile.open(directory / 'config-engine.source.tar') as archive:
            members = [member for member in archive.getmembers()
                       if str(Path(member.name)) == 'run/redis/sonic-db/database_config.json']
            check.require(len(members) == 1 and members[0].isfile(),
                          'Missing source-owned database configuration')
            (fixture / 'database_config.json').write_bytes(archive.extractfile(members[0]).read())
        for name in selected_images(directory, p4rt_image.IMAGES):
            expected = json.loads((directory / (name + '.validation.json')).read_text())
            check.require(expected.get('status') == 'passed', 'Missing accepted OCI validation')
            archive = directory / (name + '.gz')
            probe_id = uuid.uuid4().hex
            tag = 'sonic-p4rt-check:' + probe_id
            container_name = 'sonic-p4rt-probe-' + probe_id
            try:
                loaded, config, _ = validate_runtime.load_archive(archive, tag, expected['config_digest'])
                validate_runtime.verify_loaded_image(loaded, config, expected['config_digest'])
                check.require((loaded['Os'], loaded['Architecture']) == ('linux', 'amd64'),
                              'Loaded P4RT platform differs')
                check.require(config['rootfs'] == expected['rootfs'],
                              'Loaded layers differ from the validated OCI image')
                command = ['docker', 'run', '--rm', '--name', container_name, '--network=none',
                           '--cap-drop=ALL', '--security-opt=no-new-privileges', '--read-only',
                           '--user=0:0', '--pids-limit=256', '--memory=8g', '--cpus=2']
                for path, size, mode in (('/tmp', '256m', '1777'), ('/run', '64m', '0755'),
                                         ('/var/sonic', '16m', '0755'), ('/var/log', '32m', '0755'),
                                         ('/var/spool/rsyslog', '16m', '0755'), ('/zmq_swss', '16m', '0755')):
                    command += ['--tmpfs', path + ':rw,nosuid,nodev,noexec,size=' + size + ',mode=' + mode]
                mounts = {
                    'runtime_probe.py': ROOT / 'dockers/docker-sonic-p4rt/bazel/runtime_probe.py',
                    'runtime_smoke.py': ROOT / 'tools/bazel/oci/runtime_smoke.py',
                    'validation.json': report_path,
                    'services': fixture,
                }
                for destination, source in mounts.items():
                    command += ['--mount', 'type=bind,src=' + str(source) + ',dst=/probe/' + destination + ',readonly']
                command += ['--entrypoint', '/usr/bin/python3', tag, '-B', '/probe/runtime_probe.py',
                            '--validation', '/probe/validation.json']
                if name.endswith('-dbg'):
                    command.append('--debug')
                completed = subprocess.run(command, text=True, capture_output=True, timeout=1800)
                check.require(completed.returncode == 0,
                              name + ' installed probe failed:\n' + completed.stdout + completed.stderr)
                records = [json.loads(line) for line in completed.stdout.splitlines() if line.startswith('{"status":')]
                check.require(len(records) == 1 and records[0]['status'] == 'passed', 'Missing loaded P4RT receipt')
                results[name] = {**records[0], 'config_and_layers_verified': True,
                                 'archive_sha256': check.sha(archive), 'revision': revision,
                                 'owner_revision': report['owner_revision']}
                if name.endswith('-dbg') and (directory / 'bundle.json').is_file():
                    results[name]['owner_package_test'] = owner_package_probe(tag, container_name, owner_source, directory)
                safe_json(results[name])
                (directory / (name + '.runtime.json')).write_text(json.dumps(results[name], indent=2) + '\n')
            finally:
                subprocess.run(['docker', 'rm', '--force', container_name], capture_output=True, check=False)
                subprocess.run(['docker', 'image', 'rm', tag], capture_output=True, check=False)
    check.require(p4rt_image.source_identity(directory) == revision, 'Image source changed during loaded checks')
    p4rt_image.verify_owner_source(owner_source, report['owner_revision'])
    print(json.dumps({'status': 'passed', 'images': list(results)}))


if __name__ == '__main__':
    main()
