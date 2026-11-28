"""Adapt the source-owned kernel DEB and verify its producer receipt without repacking."""
import gzip
import hashlib
import json
import io
import lzma
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile


class MemberReader(io.RawIOBase):
    def __init__(self, stream, size):
        self.stream, self.remaining = stream, size

    def readable(self):
        return True

    def readinto(self, buffer):
        count = min(len(buffer), self.remaining)
        if not count: return 0
        data = self.stream.read(count)
        if not data: raise ValueError('truncated Debian data member')
        buffer[:len(data)] = data
        self.remaining -= len(data)
        return len(data)


def sha256(path):
    with open(path, 'rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def unpack(deb, receipt, runtime, boot, metadata, zstd, control=None):
    manifest = json.loads(Path(receipt).read_text())
    if manifest.get('schema') not in (3, 4):
        raise ValueError('unsupported kernel package receipt')
    records = [p for p in manifest['packages'] if p['name'] == Path(deb).name and p['kind'] == 'runtime']
    digest = sha256(deb)
    if len(records) != 1 or records[0]['sha256'] != digest:
        raise ValueError('kernel DEB differs from producer receipt')
    with open(deb, 'rb') as archive:
        if archive.read(8) != b'!<arch>\n':
            raise ValueError('invalid Debian ar archive')
        found = set()
        while header := archive.read(60):
            if len(header) != 60 or header[58:] != b'`\n':
                raise ValueError('invalid Debian ar member')
            name = header[:16].decode().rstrip().rstrip('/')
            size = int(header[48:58]); offset = archive.tell()
            kind = 'data' if name.startswith('data.tar') else 'control' if name.startswith('control.tar') and control else None
            if kind:
                if kind in found: raise ValueError('duplicate Debian payload member: ' + kind)
                found.add(kind)
                with open(runtime if kind == 'data' else control, 'wb') as out:
                    if name.endswith('.zst'):
                        proc = subprocess.Popen([zstd, '-d', '-c'], stdin=subprocess.PIPE, stdout=out)
                        remaining = size
                        while remaining:
                            chunk = archive.read(min(1 << 20, remaining))
                            if not chunk: raise ValueError('truncated Debian data member')
                            proc.stdin.write(chunk); remaining -= len(chunk)
                        proc.stdin.close()
                        if proc.wait(): raise ValueError('kernel DEB zstd decompression failed')
                    else:
                        if name.endswith(('.xz', '.gz')):
                            bounded = io.BufferedReader(MemberReader(archive, size))
                            reader = gzip.GzipFile(fileobj=bounded, mode='rb') if name.endswith('.gz') else lzma.LZMAFile(bounded, 'rb')
                            with reader as payload:
                                shutil.copyfileobj(payload, out)
                        elif name in ('data.tar', 'control.tar'):
                            remaining = size
                            while remaining:
                                chunk = archive.read(min(1 << 20, remaining))
                                if not chunk: raise ValueError('truncated Debian data member')
                                out.write(chunk); remaining -= len(chunk)
                        else: raise ValueError('unsupported Debian payload compression: ' + name)
                if 'data' in found and (control is None or 'control' in found):
                    break
            archive.seek(offset + size + size % 2)
        else: raise ValueError('missing Debian data or control member')
    release = manifest['kernel_abi']; records = {}; module_directories = set()
    with tarfile.open(runtime) as payload:
        for item in payload:
            name = item.name.removeprefix('./')
            parts = Path(name).parts
            for index, part in enumerate(parts[:-1]):
                if part == 'modules' and parts[index + 1] == release:
                    module_directories.add('/'.join(parts[:index + 2]))
            if not name.startswith('boot/') or not item.isfile(): continue
            path = Path(name)
            if '..' in path.parts or path.is_absolute(): raise ValueError('unsafe boot path')
            dest = Path(boot) / path; dest.parent.mkdir(parents=True, exist_ok=True)
            with payload.extractfile(item) as source, dest.open('wb') as out:
                shutil.copyfileobj(source, out)
            for kind in ('vmlinuz', 'config', 'System.map'):
                if name == 'boot/' + kind + '-' + release:
                    records[kind] = {'path': name, 'sha256': sha256(dest)}
    if len(records) != 3: raise ValueError('kernel boot artifacts missing or mismatched with producer ABI')
    if len(module_directories) != 1: raise ValueError('kernel module directory missing or ambiguous')
    Path(metadata).write_text(json.dumps({'module_directory': module_directories.pop(), 'schema': 1, 'architecture': manifest['architecture'], 'kernel_release': release, 'boot': records}, sort_keys=True) + '\n')


if __name__ == '__main__':
    unpack(*sys.argv[1:])
