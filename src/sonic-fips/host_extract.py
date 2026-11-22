"""Expose the original mainline prebuilt DEB members as uncompressed TARs."""
import gzip
import hashlib
import io
import json
import lzma
from pathlib import Path
import shutil
import sys
import tarfile
from email.parser import Parser


class BoundedReader(io.RawIOBase):
    def __init__(self, stream, size):
        self.stream, self.remaining = stream, size

    def readable(self):
        return True

    def readinto(self, buffer):
        data = self.stream.read(min(len(buffer), self.remaining))
        if self.remaining and not data:
            raise ValueError('truncated Debian member')
        buffer[:len(data)] = data
        self.remaining -= len(data)
        return len(data)


def extract(deb, data, control, receipt, package, version, architecture):
    outputs = {'data.tar': Path(data), 'control.tar': Path(control)}
    seen = set()
    with open(deb, 'rb') as archive:
        if archive.read(8) != b'!<arch>\n':
            raise ValueError('invalid Debian ar archive')
        while header := archive.read(60):
            if len(header) != 60 or header[58:] != b'`\n':
                raise ValueError('invalid Debian member header')
            name = header[:16].decode().strip().rstrip('/')
            size = int(header[48:58]); offset = archive.tell()
            logical = name.removesuffix('.xz').removesuffix('.gz')
            if logical in outputs:
                if logical in seen:
                    raise ValueError('duplicate Debian member: ' + logical)
                seen.add(logical)
                bounded = io.BufferedReader(BoundedReader(archive, size))
                if name.endswith('.xz'):
                    source = lzma.LZMAFile(bounded)
                elif name.endswith('.gz'):
                    source = gzip.GzipFile(fileobj=bounded)
                elif name == logical:
                    source = bounded
                else:
                    raise ValueError('unsupported Debian member compression: ' + name)
                with outputs[logical].open('wb') as target:
                    shutil.copyfileobj(source, target)
            archive.seek(offset + size + size % 2)
    if seen != set(outputs):
        raise ValueError('missing data/control TAR')
    with tarfile.open(control) as archive:
        member = next(item for item in archive if item.name.removeprefix('./') == 'control')
        fields = Parser().parsestr(archive.extractfile(member).read().decode())
    for key, expected in [('Package', package), ('Version', version), ('Architecture', architecture)]:
        if fields[key] != expected:
            raise ValueError(key + ' differs from the selected Make artifact')
    def sha(path):
        with open(path, 'rb') as stream:
            return hashlib.file_digest(stream, 'sha256').hexdigest()
    Path(receipt).write_text(json.dumps({'package': package, 'version': version,
        'architecture': architecture, 'deb_sha256': sha(deb),
        'data_sha256': sha(data), 'control_sha256': sha(control)}, sort_keys=True) + '\n')


if __name__ == '__main__':
    extract(*sys.argv[1:])
