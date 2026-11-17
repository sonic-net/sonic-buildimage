"""Verify installed Sysmgr files, symbols and its private Redis/D-Bus protocol."""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import posixpath
import re
import struct
import subprocess
import tarfile
import zlib

FILES = {'usr/bin/rebootbackend': 0o755,
         'usr/lib/x86_64-linux-gnu/librebootgnoi.so.0.0.0': 0o644}
LINKS = {f'usr/lib/x86_64-linux-gnu/librebootgnoi.so{suffix}': 'librebootgnoi.so.0.0.0'
         for suffix in ('', '.0')}


def installed_member(member, destination):
    if member.linkname.startswith('/') and (member.issym() or member.islnk()):
        target = posixpath.normpath('/' + member.linkname.lstrip('/')).lstrip('/')
        if member.issym():
            target = posixpath.relpath(target, str(PurePosixPath(member.name).parent))
        member = member.replace(linkname=target)
    return tarfile.data_filter(member, destination)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for flag in ('runtime-tar', 'debug-tar', 'dependencies', 'mock', 'probe', 'fixture',
                 'metadata', 'control', 'changelog'):
        parser.add_argument('--' + flag, type=Path, required=True)
    args = parser.parse_args()
    metadata = json.loads(args.metadata.read_text())
    assert metadata['schema'] == 1 and metadata['module'] == 'sonic-sysmgr'
    assert metadata['architecture'] == 'amd64'
    assert set(metadata['packages']) == {'sysmgr'}
    package = metadata['packages']['sysmgr']
    assert package['runtime_target'] == '//:sysmgr_pkg'
    assert package['payload_sha256'] == hashlib.sha256(args.runtime_tar.read_bytes()).hexdigest()
    for name in ('control', 'changelog'):
        assert metadata['_generated'][name + '_sha256'] == hashlib.sha256(getattr(args, name).read_bytes()).hexdigest()
    fields = package['control_fields']
    assert fields['Package'] == 'sysmgr' and fields['Version'] == '1.0.0'
    assert '${' not in json.dumps(fields)
    for name in ('libswsscommon', 'libprotobuf32t64', 'libdbus-c++-1-0v5'):
        assert re.search(r'(?:^|, )' + re.escape(name) + r'(?: \(|,|$)', fields['Depends']), fields
    root = Path(os.environ['TEST_TMPDIR']) / 'installed-sysmgr'
    root.mkdir()
    runtime, symbols = root / 'runtime', root / 'symbols'
    runtime.mkdir(); symbols.mkdir()
    evidence = Path(os.environ.get('TEST_UNDECLARED_OUTPUTS_DIR', root / 'evidence'))
    evidence.mkdir(parents=True, exist_ok=True)
    with tarfile.open(args.dependencies) as archive:
        archive.extractall(runtime, filter=installed_member)
    with tarfile.open(args.runtime_tar) as archive:
        members = archive.getmembers()
        names = [member.name.removeprefix('./').rstrip('/') for member in members]
        assert len(names) == len(set(names))
        assert {name for name, member in zip(names, members) if member.isfile()} == set(FILES)
        assert {name: member.linkname for name, member in zip(names, members) if member.issym()} == LINKS
        for name, member in zip(names, members):
            assert member.uid == member.gid == 0 and member.mtime == 0, member.name
            if member.isfile():
                assert member.mode == FILES[name], name
            elif member.isdir():
                assert member.mode == 0o755 and any(p.startswith(name + '/') for p in FILES), name
            else:
                assert member.issym() and name in LINKS
        archive.extractall(runtime, filter='data')

    loaders = {p.resolve() for p in runtime.rglob('ld-linux-x86-64.so.2') if p.is_file()}
    assert len(loaders) == 1
    loader = loaders.pop()
    libraries = sorted({p.parent.resolve() for p in runtime.rglob('lib*.so*') if p.is_file()})
    assert loader.is_relative_to(runtime) and all(p.is_relative_to(runtime) for p in libraries)
    library_path = ':'.join(map(str, libraries))
    env = dict(os.environ, LC_ALL='C', DEBUGINFOD_URLS='', PYTHONHOME=str(runtime / 'usr'))
    for name in ('LD_PRELOAD', 'LD_LIBRARY_PATH', 'LD_AUDIT', 'PYTHONPATH'):
        env.pop(name, None)

    def tool(name):
        executable = runtime / 'usr/bin' / name
        assert executable.is_file(), executable
        return executable

    def command(program, *arguments):
        return [str(loader), '--library-path', library_path, str(program), *map(str, arguments)]

    def run(program, *arguments, accepted=(0,)):
        result = subprocess.run(command(program, *arguments), env=env, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=40)
        assert result.returncode in accepted, result.stdout
        return result.stdout

    def build_id(binary):
        match = re.search(r'Build ID: ([0-9a-f]+)', run(tool('readelf'), '-n', binary))
        assert match is not None
        return match.group(1)

    ids = {}
    for name in FILES:
        binary = runtime / name
        header = binary.read_bytes()[:20]
        assert header[:6] == b'\x7fELF\x02\x01' and struct.unpack_from('<H', header, 18)[0] == 62
        assert '.debug_info' not in run(tool('readelf'), '-SW', binary)
        ids[name] = build_id(binary)
        listing = subprocess.run([str(loader), '--library-path', library_path, '--list', str(binary)],
                                 env=env, capture_output=True, text=True, timeout=40)
        assert listing.returncode == 0 and 'not found' not in listing.stdout, listing.stdout + listing.stderr
        for dependency in re.findall(r'=> (\S+)', listing.stdout):
            assert Path(dependency).resolve().is_relative_to(runtime), dependency
        dynamic = run(tool('readelf'), '-dW', binary)
        assert not re.search(r'\((?:RPATH|RUNPATH)\).*bazel-out', dynamic), dynamic
        if name.endswith('.so.0.0.0'):
            assert re.search(r'\(SONAME\).*\[librebootgnoi.so.0\]', dynamic), dynamic
    expected_debug = {'usr/lib/debug/.build-id/' + value[:2] + '/' + value[2:] + '.debug' for value in ids.values()}
    with tarfile.open(args.debug_tar) as archive:
        members = archive.getmembers()
        names = [m.name.removeprefix('./').rstrip('/') for m in members]
        assert len(names) == len(set(names)), 'Duplicate debug TAR member'
        assert {m.name.removeprefix('./') for m in members if m.isfile()} == expected_debug
        for member in members:
            name = member.name.removeprefix('./').rstrip('/')
            assert member.uid == member.gid == 0
            # Keep tar.bzl's fixed directory epoch distinct from owner symbols.
            expected_time = 946699200 if member.isdir() and name in ('.', 'usr', 'usr/lib', 'usr/lib/debug') else 1672560000
            assert member.mtime == expected_time, (member.name, member.mtime)
            assert member.isdir() and member.mode == 0o755 or member.isfile() and member.mode == 0o644
            if member.isdir():
                assert name == '.' or any(p.startswith(name + '/') for p in expected_debug), name
        archive.extractall(symbols, filter='data')
    report = {'status': 'passed', 'programs': {}, 'scope': 'AMD64 installed Sysmgr; private Redis and mock D-Bus only'}
    for name, identity in ids.items():
        binary = runtime / name
        debug = symbols / 'usr/lib/debug/.build-id' / identity[:2] / (identity[2:] + '.debug')
        assert build_id(debug) == identity and '.debug_info' in run(tool('readelf'), '-SW', debug)
        section = root / (Path(name).name + '.debuglink')
        run(tool('objcopy'), '--dump-section', '.gnu_debuglink=' + str(section), binary)
        data = section.read_bytes(); end = data.index(b'\0')
        assert data[:end].decode() == debug.name
        assert struct.unpack_from('<I', data, (end + 4) & ~3)[0] == zlib.crc32(debug.read_bytes()) & 0xffffffff
        query = 'info line main' if name.endswith('rebootbackend') else 'info functions gnoi::system::RebootRequest'
        common = ['--batch', '--nx', '--quiet', '--data-directory=' + str(runtime / 'usr/share/gdb'),
                  '-iex', 'set auto-load off', '-iex', 'set debuginfod enabled off']
        empty = root / (Path(name).name + '-no-symbols'); empty.mkdir()
        positive = run(tool('gdb'), *common, '-iex', 'set debug-file-directory ' + str(symbols / 'usr/lib/debug'),
                       '-ex', 'file ' + str(binary), '-ex', query)
        negative = run(tool('gdb'), *common, '-iex', 'set debug-file-directory ' + str(empty),
                       '-ex', 'file ' + str(binary), '-ex', query, accepted=(0, 1))
        expected = r'Line \d+ of .*rebootbackend.cpp' if name.endswith('rebootbackend') else r'File .*system.pb.cc:'
        assert re.search(expected, positive), positive
        assert not re.search(expected, negative), negative
        if name.endswith('rebootbackend'):
            assert any(text in negative for text in ('No symbol table', 'No line number information', 'not defined')), negative
        report['programs'][name] = dict(sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                                       build_id=identity, gdb=positive, negative_gdb=negative)
    report['gnoi_probe'] = run(args.probe)
    assert 'gNOI JSON and serialization passed' in report['gnoi_probe']

    # Bind only the declared installed filesystem into an isolated namespace.
    # A mock owns the expected bus name; the host system bus is not mounted.
    for name in ('run', 'tmp', 'proc', 'dev', 'work', 'evidence', 'var', 'etc'):
        (runtime / name).mkdir(exist_ok=True)
    var_run = runtime / 'var/run'
    if not var_run.exists() and not var_run.is_symlink():
        var_run.symlink_to('../run')
    assert var_run.resolve() == runtime / 'run'
    # D-Bus authenticates the private process through NSS; package payloads do
    # not include the base image's generated account and machine-id files.
    (runtime / 'etc/passwd').write_text('root:x:0:0:Private test root:/root:/bin/false\n')
    (runtime / 'etc/group').write_text('root:x:0:\n')
    (runtime / 'etc/nsswitch.conf').write_text('passwd: files\ngroup: files\n')
    (runtime / 'etc/machine-id').write_text('0123456789abcdef0123456789abcdef\n')
    private_loader = '/' + str(loader.relative_to(runtime))
    private_libraries = ':'.join('/' + str(p.relative_to(runtime)) for p in libraries)
    argv = command(tool('bwrap'), '--unshare-all', '--die-with-parent', '--new-session',
                   '--uid', '0', '--gid', '0', '--ro-bind', runtime, '/', '--proc', '/proc', '--dev', '/dev',
                   '--tmpfs', '/run', '--tmpfs', '/tmp', '--tmpfs', '/work', '--bind', evidence, '/evidence',
                   '--ro-bind', args.mock.resolve(), '/work/mock',
                   '--ro-bind', args.fixture.resolve(), '/work/installed_reboot_test.py',
                   '--clearenv', '--setenv', 'SONIC_SYSMGR_PRIVATE_FIXTURE', '1',
                   '--setenv', 'PYTHONHOME', '/usr', '--setenv', 'LC_ALL', 'C',
                   '--', private_loader, '--library-path', private_libraries, '/usr/bin/python3.13',
                   '/work/installed_reboot_test.py', '--loader', private_loader,
                   '--library-path', private_libraries, '--rebootbackend', '/usr/bin/rebootbackend',
                   '--mock', '/work/mock', '--redis-server', '/usr/bin/redis-server',
                   '--dbus-daemon', '/usr/bin/dbus-daemon', '--evidence', '/evidence')
    result = subprocess.run(argv, env=env, capture_output=True, text=True, timeout=120)
    (evidence / 'namespace.log').write_text(result.stdout + result.stderr)
    assert result.returncode == 0, result.stdout + result.stderr
    fixture = json.loads((evidence / 'result.json').read_text())
    assert fixture['status'] == 'passed'
    report['private_fixture'] = fixture
    (evidence / 'installed-sysmgr.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
