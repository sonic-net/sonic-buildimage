"""Project declared package payloads and original controls into the dpkg database."""
from email.parser import Parser
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import re
import sys
import tarfile

from tools.bazel.image.merge_layers import Tree, apply_layer, split_path


def normalized(name):
    path = PurePosixPath(name.removeprefix('./'))
    if path.is_absolute() or '..' in path.parts:
        raise ValueError('package path escapes root')
    return str(path)


def add(archive, name, data, mode=0o644):
    member = tarfile.TarInfo('./' + name); member.mode, member.size = mode, len(data)
    archive.addfile(member, io.BytesIO(data))


def unpacked(spec, output, receipt):
    statuses, records, packages = [], [], set()
    layout = Tree()
    if spec.get('layout'):
        with tarfile.open(spec['layout']) as archive:
            apply_layer(layout, archive, spec['layout'])

    def deployed_path(name):
        parts = split_path(name)
        if not parts: return '.'
        # Use the assembler's chroot-style resolution and the same declared
        # skeleton, rather than duplicating merged-/usr alias constants.
        _, parent = layout._resolve(parts[:-1], True, 0)
        return '/'.join(parent + [parts[-1]])

    with tarfile.open(output, 'w', format=tarfile.PAX_FORMAT) as destination:
        for expected, payload, control in spec['packages']:
            with tarfile.open(control) as archive:
                controls = {normalized(m.name): (m, archive.extractfile(m).read()) for m in archive if m.isfile()}
            raw = controls['control'][1].decode().rstrip('\n')
            fields = Parser().parsestr(raw)
            package = fields['Package']
            if not package or not re.fullmatch(r'[a-z0-9][a-z0-9+.-]*', package) or package in packages:
                raise ValueError('duplicate or invalid final package identity: ' + str(package))
            if expected and expected != package:
                raise ValueError('control package differs from selected payload owner')
            if fields['Architecture'] not in ('amd64', 'all') or not fields['Version'] or '${' in raw:
                raise ValueError('unresolved or foreign final control: ' + package)
            packages.add(package)
            paths, sums = {'.'}, {}
            with tarfile.open(payload) as archive:
                for member in archive:
                    name = normalized(member.name)
                    if name == '.': continue
                    if any(part.startswith('.wh.') for part in PurePosixPath(name).parts):
                        raise ValueError('package state requires original package payloads, not OCI whiteouts')
                    paths.add(name)
                    paths.update(str(parent) for parent in PurePosixPath(name).parents)
                    if member.isfile() or member.islnk():
                        source = archive.extractfile(member)
                        digest = hashlib.md5()
                        for block in iter(lambda: source.read(1 << 20), b''): digest.update(block)
                        sums[name] = digest.hexdigest()
            installed_sums = {deployed_path(name): digest for name, digest in sums.items()}
            installed_paths = {deployed_path(name) for name in paths}
            if 'md5sums' in controls:
                for line in controls['md5sums'][1].decode().splitlines():
                    if not line.strip(): continue
                    digest, name = line.split(None, 1); name = normalized(name.strip())
                    if installed_sums.get(deployed_path(name)) != digest:
                        raise ValueError('original control checksum differs from selected payload: ' + package + ':' + name)
            prefix = package + (':' + fields['Architecture'] if fields['Multi-Arch'] == 'same' else '')
            for name, (member, data) in controls.items():
                if name in ('control', 'md5sums'): continue
                if '/' in name: raise ValueError('control member must be a flat regular file')
                add(destination, 'var/lib/dpkg/info/' + prefix + '.' + name, data, member.mode)
            add(destination, 'var/lib/dpkg/info/' + prefix + '.list', ('\n'.join('/.' if path == '.' else '/' + path for path in sorted(paths)) + '\n').encode())
            add(destination, 'var/lib/dpkg/info/' + prefix + '.md5sums', ''.join(digest + '  ' + name + '\n' for name, digest in sorted(sums.items())).encode())
            status = raw + '\nStatus: install ok unpacked\n'
            if 'conffiles' in controls:
                conffiles = []
                for line in controls['conffiles'][1].decode().splitlines():
                    if not line.strip(): continue
                    entry = line.strip()
                    removal = not entry.startswith('/')
                    if removal:
                        parts = entry.split(None, 1)
                        if len(parts) != 2 or parts[0] != 'remove-on-upgrade':
                            raise ValueError('unsupported conffile flag: ' + entry)
                        entry = parts[1]
                    if not entry.startswith('/'):
                        raise ValueError('conffile path must be absolute: ' + entry)
                    name = normalized(entry[1:])
                    if removal:
                        if deployed_path(name) in installed_paths:
                            raise ValueError('remove-on-upgrade conffile exists in payload: ' + name)
                        # deb-conffiles(5): dpkg retains this fresh-install
                        # marker even after configuration; there is no MD5.
                        conffiles.append(' /' + name + ' newconffile remove-on-upgrade')
                        continue
                    digest = installed_sums.get(deployed_path(name))
                    if digest is None: raise ValueError('conffile absent from selected payload: ' + name)
                    conffiles.append(' /' + name + ' ' + digest)
                status += 'Conffiles:\n' + '\n'.join(conffiles) + '\n'
            statuses.append(status)
            records.append({'package': package, 'version': fields['Version'], 'architecture': fields['Architecture'],
                            'payload_files': len(sums), 'paths': len(paths), 'control_sha256': hashlib.sha256(controls['control'][1]).hexdigest()})
        add(destination, 'var/lib/dpkg/status', ('\n'.join(statuses) + '\n').encode())
        for path in ['var/lib/dpkg/updates', 'var/lib/dpkg/triggers', 'var/lib/dpkg/parts']:
            member = tarfile.TarInfo('./' + path); member.type, member.mode = tarfile.DIRTYPE, 0o755
            destination.addfile(member)
    Path(receipt).write_text(json.dumps({'schema': 1, 'state': 'unpacked', 'packages': records}, indent=2) + '\n')


def configured(spec, output, receipt):
    with tarfile.open(spec['unpacked']) as archive:
        status = next(member for member in archive if normalized(member.name) == 'var/lib/dpkg/status')
        data = archive.extractfile(status).read()
    if not spec['effects']: raise ValueError('configured state requires declared installation effects')
    count = data.count(b'\nStatus: install ok unpacked\n')
    if not count: raise ValueError('initial package state contains no unpacked packages')
    data = data.replace(b'\nStatus: install ok unpacked\n', b'\nStatus: install ok installed\n')
    with tarfile.open(output, 'w', format=tarfile.PAX_FORMAT) as archive: add(archive, 'var/lib/dpkg/status', data)
    Path(receipt).write_text(json.dumps({'schema': 1, 'state': 'installed', 'package_count': count,
        'effects': [{'path': path, 'sha256': hashlib.sha256(Path(path).read_bytes()).hexdigest()} for path in spec['effects']],
        'meaning': 'Declared installation effects completed; their behavioral audits establish postinst coverage.'}, indent=2) + '\n')


if __name__ == '__main__':
    spec = json.loads(Path(sys.argv[1]).read_text())
    (configured if 'unpacked' in spec else unpacked)(spec, *sys.argv[2:])
