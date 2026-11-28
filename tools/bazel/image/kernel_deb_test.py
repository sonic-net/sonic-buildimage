"""Debian ar/TAR format and source receipt pairing contract."""
import gzip
import hashlib
import io
import json
import lzma
from pathlib import Path
import sys
import tarfile
import tempfile
import unittest

from kernel_deb import unpack


class KernelDebTest(unittest.TestCase):
    def test_debian_payload_and_boot_identity(self):
        for schema, suffix, compress in [(4, '.xz', lzma.compress), (3, '.gz', gzip.compress), (4, '', lambda data: data)]:
            with self.subTest(compression=suffix, schema=schema), tempfile.TemporaryDirectory() as directory:
                root = Path(directory); abi = '6.12.41+deb13-sonic-amd64'
                stream = io.BytesIO()
                with tarfile.open(fileobj=stream, mode='w') as archive:
                    for kind in ['vmlinuz', 'config', 'System.map']:
                        data = (kind + ' kernel producer bytes').encode()
                        info = tarfile.TarInfo('./boot/' + kind + '-' + abi)
                        info.size = len(data); info.uid = 27; info.mode = 0o640
                        archive.addfile(info, io.BytesIO(data))
                    module = tarfile.TarInfo('./usr/lib/modules/' + abi + '/kernel/probe.ko'); module.size = 3
                    archive.addfile(module, io.BytesIO(b'ELF'))
                control_stream = io.BytesIO()
                with tarfile.open(fileobj=control_stream, mode='w') as archive:
                    member = tarfile.TarInfo('./control'); contents = b'Package: linux-image-contract\nVersion: 1\nArchitecture: amd64\n'
                    member.size = len(contents); archive.addfile(member, io.BytesIO(contents))
                control_payload = compress(control_stream.getvalue())
                payload = compress(stream.getvalue()); deb = root / 'kernel.deb'
                # Debian policy ar layout, including an unrelated member after data.
                with deb.open('wb') as output:
                    output.write(b'!<arch>\n')
                    for name, content in [('debian-binary', b'2.0\n'), ('control.tar' + suffix, control_payload), ('data.tar' + suffix, payload), ('ignored', b'not compressed data')]:
                        output.write(f'{name:<16}{0:<12}{0:<6}{0:<6}{100644:<8}{len(content):<10}`\n'.encode())
                        output.write(content)
                        if len(content) % 2: output.write(b'\n')
                receipt = root / 'receipt.json'
                record = {'name': deb.name, 'kind': 'runtime', 'sha256': hashlib.sha256(deb.read_bytes()).hexdigest()}
                manifest = {'schema': schema, 'kernel_abi': abi, 'architecture': 'amd64', 'packages': [record]}
                if schema == 4:
                    manifest['host_architecture'] = 'amd64'
                receipt.write_text(json.dumps(manifest))
                unpack(str(deb), str(receipt), str(root/'runtime.tar'), str(root/'boot'), str(root/'metadata.json'), '/unused', str(root/'control.tar'))
                self.assertEqual((root/'control.tar').read_bytes(), control_stream.getvalue())
                metadata = json.loads((root/'metadata.json').read_text())
                self.assertEqual(metadata['kernel_release'], abi)
                for kind in ['vmlinuz', 'config', 'System.map']:
                    record = metadata['boot'][kind]
                    content = (root/'boot'/record['path']).read_bytes()
                    self.assertEqual(content, (kind + ' kernel producer bytes').encode())
                    self.assertEqual(record['sha256'], hashlib.sha256(content).hexdigest())
                with tarfile.open(root/'runtime.tar') as archive:
                    member = archive.getmembers()[0]
                    self.assertEqual((member.uid, member.mode), (27, 0o640))
                changed = json.loads(receipt.read_text()); changed['packages'][0]['sha256'] = '0' * 64
                receipt.write_text(json.dumps(changed))
                with self.assertRaisesRegex(ValueError, 'differs from producer'):
                    unpack(str(deb), str(receipt), str(root/'bad.tar'), str(root/'bad'), str(root/'bad.json'), '/unused')


if __name__ == '__main__':
    unittest.main()
