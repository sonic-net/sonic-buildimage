"""Read the Linux newc format independently of the initramfs writer."""
import json
from pathlib import Path
import stat
import subprocess
import sys
import tarfile

image, metadata_path, zstd = sys.argv[1:4]
metadata = json.loads(Path(metadata_path).read_text())
with open(image, 'rb') as source:
    payload = subprocess.run([zstd, '-d', '-c'], stdin=source, check=True, stdout=subprocess.PIPE).stdout
position = 0; entries = {}
while True:
    header = payload[position:position + 110]
    assert header[:6] == b'070701', 'initramfs is not Linux newc'
    fields = [int(header[offset:offset + 8], 16) for offset in range(6, 110, 8)]
    size, namesize = fields[6], fields[11]
    start = position + 110
    name = payload[start:start + namesize - 1].decode().removeprefix('./')
    assert payload[start + namesize - 1] == 0
    start = (start + namesize + 3) & ~3
    content = payload[start:start + size]
    assert len(content) == size
    position = (start + size + 3) & ~3
    if name == 'TRAILER!!!': break
    assert name not in entries, 'duplicate initramfs member: ' + name
    entries[name] = (fields[1], content)
assert stat.S_ISREG(entries['init'][0]) and entries['init'][0] & 0o111
assert 'usr/bin/busybox' in entries or 'bin/busybox' in entries
release = metadata['kernel_release']
assert any(name.endswith('.ko') and '/modules/' + release + '/' in name for name in entries)
assert any(name.endswith('/modules/' + release + '/modules.dep') for name in entries)
assert 'conf/initramfs.conf' in entries

if len(sys.argv) > 4:
    with tarfile.open(sys.argv[4]) as archive:
        links = {member.name.removeprefix('./'): member for member in archive}
    assert set(links) == {'vmlinuz', 'initrd.img'}
    for name, target in [('vmlinuz', 'boot/vmlinuz-' + metadata['kernel_release']), ('initrd.img', 'boot/initrd.img-' + metadata['kernel_release'])]:
        assert links[name].issym() and links[name].linkname == target
        assert (links[name].uid, links[name].gid, links[name].mode) == (0, 0, 0o777)
