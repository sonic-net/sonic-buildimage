"""Stream original lazy and GRUB Debian packages into the platform archive."""
from email.parser import Parser
import gzip
import io
import json
import lzma
from pathlib import Path
import sys
import tarfile


def identity(path):
    with path.open('rb') as stream:
        if stream.read(8) != b'!<arch>\n': raise ValueError('invalid original platform DEB')
        while header := stream.read(60):
            name = header[:16].decode().strip().rstrip('/'); size = int(header[48:58]); offset = stream.tell()
            if name.startswith('control.tar'):
                data = stream.read(size)
                if name.endswith('.gz'): data = gzip.decompress(data)
                elif name.endswith('.xz'): data = lzma.decompress(data)
                elif name != 'control.tar': raise ValueError('unsupported platform DEB control codec')
                with tarfile.open(fileobj=io.BytesIO(data)) as archive:
                    member = next(member for member in archive if member.name.removeprefix('./') == 'control')
                    fields = Parser().parsestr(archive.extractfile(member).read().decode())
                return '{}_{}_{}.deb'.format(fields['Package'], fields['Version'], fields['Architecture'])
            stream.seek(offset + size + size % 2)
    raise ValueError('platform DEB lacks original control')


def build(spec, output):
    with tarfile.open(output, 'w', format=tarfile.PAX_FORMAT) as archive:
        marker = tarfile.TarInfo('./firsttime'); marker.mode = 0o644
        archive.addfile(marker, io.BytesIO())
        for source, destination, platform in spec:
            path = Path(source)
            if path.is_dir():
                packages = list(path.glob('*.deb'))
                if len(packages) != 1: raise ValueError('lazy source must produce exactly one DEB')
                path = packages[0]
            name = identity(path)
            member = tarfile.TarInfo('./' + destination + '/' + name)
            member.mode, member.size = 0o644, path.stat().st_size
            with path.open('rb') as stream: archive.addfile(member, stream)
            if platform:
                link = tarfile.TarInfo('./' + platform + '/' + name)
                link.type, link.mode, link.linkname = tarfile.SYMTYPE, 0o777, '../common/' + name
                archive.addfile(link)


if __name__ == '__main__':
    build(json.loads(Path(sys.argv[1]).read_text()), sys.argv[2])
