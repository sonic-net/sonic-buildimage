"""Inspect the real Debian ar/control/data formats and source installation layout."""
from email.parser import Parser
import gzip
import hashlib
import importlib.util
import io
import marshal
from pathlib import Path
import sys
import tarfile

root, source = map(Path, sys.argv[1:])
source = source.parent
debs = list(root.glob('*.deb'))
assert [deb.name for deb in debs] == ['sonic-platform-vs_1.0_amd64.deb']
with debs[0].open('rb') as stream:
    assert stream.read(8) == b'!<arch>\n'
    members = {}
    while header := stream.read(60):
        assert len(header) == 60 and header[58:] == b'`\n'
        name = header[:16].decode().strip().rstrip('/')
        size = int(header[48:58]); members[name] = stream.read(size)
        if size % 2: stream.read(1)
assert members['debian-binary'] == b'2.0\n'
with tarfile.open(fileobj=io.BytesIO(gzip.decompress(members['control.tar.gz']))) as archive:
    controls = {m.name.removeprefix('./'): archive.extractfile(m).read() for m in archive if m.isfile()}
fields = Parser().parsestr(controls['control'].decode())
assert fields['Package'] == 'sonic-platform-vs' and fields['Version'] == '1.0' and fields['Architecture'] == 'amd64'
assert fields['Section'] == 'main' and fields['Priority'] == 'extra' and not fields['Depends']
with tarfile.open(fileobj=io.BytesIO(gzip.decompress(members['data.tar.gz']))) as archive:
    files = {m.name.removeprefix('./'): (m, archive.extractfile(m).read()) for m in archive if m.isfile()}
for name in ['__init__', 'chassis', 'platform']:
    installed = 'usr/lib/python3/dist-packages/sonic_platform/' + name + '.py'
    assert files[installed][1] == (source / (name + '.py')).read_bytes()
    cached = installed.rsplit('/', 1)[0] + '/__pycache__/' + name + '.cpython-313.pyc'
    data = files[cached][1]
    assert data[:4] == importlib.util.MAGIC_NUMBER
    assert marshal.loads(data[16:]).co_filename == '/' + installed
for line in controls['md5sums'].decode().splitlines():
    digest, name = line.split('  ', 1)
    assert hashlib.md5(files[name][1]).hexdigest() == digest
assert gzip.decompress(files['usr/share/doc/sonic-platform-vs/changelog.gz'][1]).startswith(b'sonic-platform-vs (1.0)')
print('real VS platform DEB control, original modules, bytecode and payload checksums passed')
