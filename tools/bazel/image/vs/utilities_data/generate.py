"""Run the unchanged installed-path CLI generator in a declared read-only view."""
import argparse
import json
import os
from pathlib import Path, PurePosixPath
import runpy
import subprocess
import sys
import tempfile

parser = argparse.ArgumentParser()
for name in ['generator', 'output', 'apt-runtime', 'filesystem', 'cfggen', 'database-config', 'ca-bundle', 'hosts', 'hostname']:
    parser.add_argument('--' + name, required=True)
parser.add_argument('--bwrap')
parser.add_argument('--source-tree', action='append', default=[])
parser.add_argument('--python-import', action='append', default=[])
parser.add_argument('--inside', action='store_true')
args = parser.parse_args()


def declared_tree(path):
    """Find the same TreeArtifact behind Bazel's per-file sandbox symlinks."""
    path = Path(path).resolve(strict=True)
    files = {str(file.relative_to(path)): file for file in path.rglob('*') if file.is_file()}
    checked = set()
    for name, file in sorted(files.items(), key=lambda item: (len(PurePosixPath(item[0]).parts), item[0])):
        physical = file.resolve(strict=True)
        if physical.name != file.name:
            continue
        root = physical.parents[len(PurePosixPath(name).parts) - 1]
        if root in checked:
            continue
        checked.add(root)
        try:
            same_files = all(file.resolve(strict=True) == (root / name).resolve(strict=True) for name, file in files.items())
        except FileNotFoundError:
            continue
        if not same_files:
            continue
        # Bazel expands directory aliases such as lib -> usr/lib into files.
        # Compare canonical payloads after checking every declared name's map.
        actual = {file.resolve(strict=True) for file in root.rglob('*') if file.is_file()}
        if actual == {file.resolve(strict=True) for file in files.values()}:
            return root
    raise ValueError('Cannot resolve the exact declared TreeArtifact: ' + str(path))


def enter_namespace():
    runfiles = Path(os.environ['RUNFILES_DIR']).absolute()
    runtime = declared_tree(args.apt_runtime)
    output = Path(args.output).absolute()
    output.mkdir(parents=True, exist_ok=True)
    def guest(path):
        return '/runfiles/' + str(Path(path).absolute().relative_to(runfiles))
    options = ['--unshare-user', '--unshare-pid', '--unshare-ipc', '--unshare-uts', '--die-with-parent', '--new-session', '--uid', '0', '--gid', '0', '--cap-drop', 'ALL', '--clearenv', '--dir', '/usr', '--dir', '/usr/local', '--dir', '/usr/local/bin']
    for path in sorted((runtime / 'usr').iterdir()):
        if path.name == 'bin':
            options += ['--dir', '/usr/bin']
            for program in sorted(path.iterdir()):
                options += ['--ro-bind', str(program), '/usr/bin/' + program.name]
            # Debian dash's postinst supplies the normal POSIX shell link.
            if not (path / 'sh').exists():
                if not (path / 'dash').is_file():
                    raise ValueError('The declared APT runtime requires dash for /bin/sh')
                options += ['--symlink', 'dash', '/usr/bin/sh']
        elif path.name != 'local':
            options += ['--ro-bind', str(path), '/usr/' + path.name]
    # The checked host Python TAR includes the main wheel installations here.
    options += ['--ro-bind', str(runtime / 'usr/local/lib'), '/usr/local/lib']
    options += ['--dir', '/etc']
    for path in sorted((runtime / 'etc').rglob('*')):
        destination = '/' + str(path.relative_to(runtime))
        if path.is_symlink():
            options += ['--symlink', os.readlink(path), destination]
        elif path.is_dir():
            options += ['--dir', destination]
        else:
            options += ['--ro-bind', str(path), destination]
    options += ['--dir', '/etc/ssl', '--dir', '/etc/ssl/certs', '--ro-bind', str(Path(args.ca_bundle).resolve()), '/etc/ssl/certs/ca-certificates.crt']
    for name in ['hosts', 'hostname']:
        options += ['--ro-bind', str(Path(getattr(args, name)).resolve()), '/etc/' + name]
    for destination, target in [('bin', 'usr/bin'), ('sbin', 'usr/sbin'), ('lib', 'usr/lib'), ('lib64', 'usr/lib64')]:
        options += ['--symlink', target, '/' + destination]
    options += ['--dir', '/var', '--dir', '/var/run', '--dir', '/var/run/redis', '--dir', '/var/run/redis/sonic-db', '--ro-bind', str(Path(args.database_config).resolve()), '/var/run/redis/sonic-db/database_config.json', '--ro-bind', str(Path(args.cfggen).resolve()), '/usr/local/bin/sonic-cfggen', '--dev', '/dev', '--proc', '/proc', '--tmpfs', '/tmp', '--dir', '/runfiles']
    filesystem = json.loads(Path(args.filesystem).read_text())
    bindings = {name: declared_tree(runfiles / name) if directory else (runfiles / name).resolve(strict=True) for name, directory in filesystem.items()}
    # Collapse only complete declared subtrees with identical relative layout.
    # Comparing names (not reading extra payload) prevents exposing undeclared
    # package files or empty directories while staying below bwrap's arg limit.
    candidates = set()
    for import_path in args.python_import:
        prefix = str(Path(import_path).absolute().relative_to(runfiles))
        for name in filesystem:
            if name.startswith(prefix + '/'):
                candidates.update(str(parent) for parent in PurePosixPath(name).parents if str(parent).startswith(prefix))
    for prefix in sorted(candidates, key=lambda value: (len(PurePosixPath(value).parts), value)):
        members = [name for name in bindings if name.startswith(prefix + '/')]
        if not members or any(filesystem[name] for name in members):
            continue
        first = members[0]
        relative = PurePosixPath(first).relative_to(prefix)
        source_root = bindings[first].parents[len(relative.parts) - 1]
        if not all(bindings[name] == source_root / PurePosixPath(name).relative_to(prefix) for name in members):
            continue
        selected = {str(PurePosixPath(name).relative_to(prefix)) for name in members}
        selected_dirs = {str(parent) for name in selected for parent in PurePosixPath(name).parents if str(parent) != '.'}
        actual = list(source_root.rglob('*'))
        if any(path.is_symlink() for path in actual):
            continue
        if {str(path.relative_to(source_root)) for path in actual if path.is_file()} != selected or {str(path.relative_to(source_root)) for path in actual if path.is_dir()} != selected_dirs:
            continue
        for name in members:
            del bindings[name]
            del filesystem[name]
        bindings[prefix] = source_root
        filesystem[prefix] = True
    print('Read-only view: %d file/tree mounts after exact declared-subtree grouping' % len(bindings), flush=True)
    directories = set()
    for name, directory in filesystem.items():
        path = PurePosixPath(name)
        if path.is_absolute() or '..' in path.parts:
            raise ValueError('Invalid runfiles member: ' + name)
        directories.update(str(parent) for parent in path.parents if str(parent) != '.')
        if directory:
            directories.add(name)
    for name in sorted(directories, key=lambda value: (len(PurePosixPath(value).parts), value)):
        options += ['--dir', '/runfiles/' + name]
    for name in sorted(filesystem, key=lambda value: (len(PurePosixPath(value).parts), value)):
        options += ['--ro-bind', str(bindings[name]), '/runfiles/' + name]
    options += ['--bind', str(output), '/output', '--setenv', 'RUNFILES_DIR', '/runfiles', '--setenv', 'PYTHONHOME', '/usr', '--setenv', 'PYTHONDONTWRITEBYTECODE', '1', '--setenv', 'LANG', 'C', '--setenv', 'LC_ALL', 'C', '--setenv', 'PATH', '/usr/bin:/bin', '--chdir', '/output', '--remount-ro', '/']
    inner = ['/usr/bin/python3.13', '-B', guest(__file__), '--inside', '--generator', guest(args.generator), '--output', '/output', '--apt-runtime', guest(args.apt_runtime), '--filesystem', guest(args.filesystem), '--cfggen', '/usr/local/bin/sonic-cfggen', '--database-config', '/var/run/redis/sonic-db/database_config.json', '--ca-bundle', '/etc/ssl/certs/ca-certificates.crt', '--hosts', '/etc/hosts', '--hostname', '/etc/hostname']
    for path in args.source_tree:
        inner += ['--source-tree', guest(path)]
    for path in args.python_import:
        inner += ['--python-import', guest(path)]
    # Large runfiles closures exceed argv limits; bwrap accepts NUL-separated
    # options through a declared inherited descriptor without copying payloads.
    with tempfile.TemporaryFile() as descriptor:
        descriptor.write(b'\0'.join(os.fsencode(value) for value in options) + b'\0')
        descriptor.flush()
        descriptor.seek(0)
        result = subprocess.run([args.bwrap, '--args', str(descriptor.fileno()), '--'] + inner, pass_fds=(descriptor.fileno(),))
    return result.returncode


if not args.inside:
    raise SystemExit(enter_namespace())
paths = [str(Path(tree) / 'usr/local/lib/python3.13/dist-packages') for tree in args.source_tree]
paths += args.python_import
# Debian's normal site startup has already processed the real setuptools .pth.
sys.path[:0] = paths
sys.argv = [args.generator, '-o', args.output]
runpy.run_path(args.generator, run_name='__main__')
