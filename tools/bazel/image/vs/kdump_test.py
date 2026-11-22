"""Verify crash initramfs sysctls and real kdump kernel-hook installation layout."""
import json
from pathlib import Path
import subprocess
import sys
import tarfile

normal, crash, installed, metadata, sysctl, zstd = sys.argv[1:]
release = json.loads(Path(metadata).read_text())['kernel_release']
expected = Path(sysctl).read_bytes()

def entries(path):
    with open(path, 'rb') as source:
        payload = subprocess.check_output([zstd, '-d', '-c'], stdin=source)
    position, result = 0, {}
    while True:
        header = payload[position:position + 110]
        assert header[:6] == b'070701'
        fields = [int(header[i:i + 8], 16) for i in range(6, 110, 8)]
        size, length = fields[6], fields[11]
        start = position + 110
        name = payload[start:start + length - 1].decode().removeprefix('./')
        start = (start + length + 3) & ~3
        content = payload[start:start + size]
        position = (start + size + 3) & ~3
        if name == 'TRAILER!!!': return result
        assert name not in result
        result[name] = content

ordinary, emergency = entries(normal), entries(crash)
assert 'etc/kdump/sysctl.conf' not in ordinary
assert emergency['etc/kdump/sysctl.conf'] == expected
assert 'scripts/local-bottom/kdump-sysctl' in emergency
assert any(name.endswith('.ko') and release in name for name in emergency)
assert Path(normal).read_bytes() != Path(crash).read_bytes()
with tarfile.open(installed) as archive:
    members = {member.name.removeprefix('./'): member for member in archive}
    prefix = 'var/lib/kdump/'
    assert archive.extractfile(members[prefix + 'initrd.img-' + release]).read() == Path(crash).read_bytes()
    assert archive.extractfile(members[prefix + 'latest_sysctls-' + release]).read() == expected
    assert int(archive.extractfile(members[prefix + 'size_initrd.img-' + release]).read()) > 0
    assert members[prefix + 'vmlinuz'].linkname == '/boot/vmlinuz-' + release
    assert members[prefix + 'initrd.img'].linkname == '/var/lib/kdump/initrd.img-' + release
print('distinct real crash initramfs, sysctl hook, size and ABI-bound installed links passed')
