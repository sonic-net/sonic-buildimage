"""Original SmartCmd is the only declared static i386 input in AMD64 PMON.

Dockerfile.j2:113 retains the vendor bytes. The VS kernel enables IA32 emulation;
ELF program header PT_INTERP and dynamic tag DT_NEEDED describe dynamic loading.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path
import struct
import tempfile
import unittest

from tools.bazel.ci.artifact_validation import file_metadata
from tools.bazel.oci.apt_selection import read_policy

parser = argparse.ArgumentParser()
parser.add_argument('--smartcmd', type=Path, required=True)
parser.add_argument('--policy', type=Path, required=True)
args, remaining = parser.parse_known_args()
NAME = 'usr/bin/SmartCmd'


def elf32(machine=3, program=2, dynamic_tag=0):
    ident = b'\x7fELF\x01\x01\x01' + bytes(9)
    header = struct.pack('<16sHHIIIIIHHHHHH', ident, 2, machine, 1, 0,
                         52, 0, 0, 52, 32, 1, 0, 0, 0)
    segment = struct.pack('<IIIIIIII', program, 84, 0, 0, 16, 16, 4, 4)
    return header + segment + struct.pack('<iIiI', dynamic_tag, 0, 0, 0)


class SmartCmdCompatibility(unittest.TestCase):
    def test_original_bytes_require_the_declared_path_and_hash(self):
        data = args.smartcmd.read_bytes()
        policy = read_policy(args.policy)
        self.assertEqual(policy['architecture'], 'amd64')
        self.assertEqual(policy['static_i386'], {NAME: hashlib.sha256(data).hexdigest()})
        result = file_metadata(io.BytesIO(data), NAME, static_i386=policy['static_i386'])
        self.assertEqual((result['elf_type'], result['elf_machine']), (2, 3))
        for name, allowed in [(NAME, None), ('usr/bin/other', policy['static_i386']),
                              (NAME, {NAME: '0' * 64})]:
            with self.subTest(name=name, allowed=allowed):
                with self.assertRaisesRegex(ValueError, 'expected little-endian ELF64'):
                    file_metadata(io.BytesIO(data), name, static_i386=allowed)

    def test_matched_hash_does_not_allow_dynamic_or_wrong_architecture(self):
        for data, error in [(elf32(machine=40), 'i386 executable'),
                            (elf32(program=3), 'PT_INTERP'),
                            (elf32(dynamic_tag=1), 'DT_NEEDED')]:
            with self.subTest(error=error):
                with self.assertRaisesRegex(ValueError, error):
                    file_metadata(io.BytesIO(data), NAME,
                                  static_i386={NAME: hashlib.sha256(data).hexdigest()})

    def test_compatibility_is_rejected_for_an_arm64_target(self):
        policy = json.loads(args.policy.read_text())
        policy['architecture'] = 'arm64'
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'policy.json'
            path.write_text(json.dumps(policy))
            with self.assertRaisesRegex(ValueError, 'requires target amd64'):
                read_policy(path)


if __name__ == '__main__':
    unittest.main(argv=[__file__] + remaining)
