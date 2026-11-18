"""Read the disk action's qcow2 format and configured geometry independently."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import struct
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--disk', required=True)
parser.add_argument('--qemu-img', required=True)
parser.add_argument('--size-mib', type=int, required=True)
args = parser.parse_args()
tool = str(Path(args.qemu_img).absolute())
info = json.loads(subprocess.check_output([tool, 'info', '--output=json', args.disk]))
if info['format'] != 'qcow2' or info['virtual-size'] != args.size_mib * 1024 * 1024:
    raise AssertionError('Disk output format or virtual size differs: ' + repr(info))
subprocess.run([tool, 'check', args.disk], check=True)
# GRUB 2.12 include/grub/i386/pc/boot.h places the embedded core LBA at
# offset 0x5c.  The fixture's EF02 partition occupies LBA 2048..4095.
# Reading those sectors also detects a no-op bootloader installer.
with tempfile.TemporaryDirectory() as directory:
    header = Path(directory) / 'boot-sectors'
    subprocess.run([tool, 'dd', 'if=' + args.disk, 'of=' + str(header), 'bs=512', 'count=4096'], check=True)
    sectors = header.read_bytes()
    if sectors[510:512] != b'\x55\xaa' or sectors[512:520] != b'EFI PART':
        raise AssertionError('Missing BIOS boot signature or GPT header')
    core_lba = struct.unpack_from('<Q', sectors, 0x5c)[0]
    if not 2048 <= core_lba < 4096 or not any(sectors[core_lba * 512:(core_lba + 1) * 512]):
        raise AssertionError('GRUB core was not embedded in the declared BIOS boot partition')
path = Path(args.disk)
receipt = {'format': info['format'], 'virtual_size': info['virtual-size'],
           'sha256': hashlib.sha256(path.read_bytes()).hexdigest(), 'size': path.stat().st_size,
           'scope': 'BIOS disk assembly format and qcow2 integrity; boot is validated by the product target.'}
Path(os.environ['TEST_UNDECLARED_OUTPUTS_DIR'], 'disk-action-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
