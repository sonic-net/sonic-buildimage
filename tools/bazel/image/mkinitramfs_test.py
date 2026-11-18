"""Hook expansion and rooted staging regressions; no image tools executed."""
import io
import argparse
import sys
import hashlib
import json
import lzma
import os
from pathlib import Path
import stat
import struct
import tarfile
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from mkinitramfs import ARCH, compress_initrd, Mkinitramfs, Rootfs, gnu_tsort, script_prereqs, read_kernel_metadata, build_initrd
from newc import write_newc


_parser = argparse.ArgumentParser(add_help=False)
_parser.add_argument('--pigz')
TOOLS, _arguments = _parser.parse_known_args()
if TOOLS.pigz:
    TOOLS.pigz = str(Path(TOOLS.pigz).absolute())
sys.argv = [sys.argv[0]] + _arguments


class InitrdTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name) / 'root'
        self.root.mkdir()
        self.fs = Rootfs(str(self.root))

    def test_debian_merged_usr_keeps_directory_alias(self):
        (self.root / 'usr/lib/modules/abi').mkdir(parents=True)
        (self.root / 'lib').symlink_to('usr/lib')
        layer = Path(self.temp.name) / 'legacy-lib.tar'
        with tarfile.open(layer, 'w') as archive:
            directory = tarfile.TarInfo('lib/')
            directory.type, directory.mode = tarfile.DIRTYPE, 0o755
            archive.addfile(directory)
            member = tarfile.TarInfo('lib/initramfs-tools/hook-functions')
            member.size = 4
            archive.addfile(member, io.BytesIO(b'hook'))
        self.fs.apply_layer(layer)
        self.assertTrue((self.root / 'lib').is_symlink())
        self.assertTrue(self.fs.isdir('/lib/modules/abi'))
        self.assertEqual((self.root / 'usr/lib/initramfs-tools/hook-functions').read_bytes(), b'hook')

    def kernel_layer(self, name, directories=(), symlinks=()):
        layer = Path(self.temp.name) / (name + '.tar')
        records = {}
        with tarfile.open(layer, 'w') as archive:
            for path in directories:
                member = tarfile.TarInfo(path)
                member.type, member.mode = tarfile.DIRTYPE, 0o755
                archive.addfile(member)
            for path, target in symlinks:
                member = tarfile.TarInfo(path)
                member.type, member.linkname = tarfile.SYMTYPE, str(target)
                archive.addfile(member)
            for kind in ('vmlinuz', 'config', 'System.map'):
                content = (kind + ' declared bytes').encode()
                path = 'boot/' + kind + '-vendor-release'
                member = tarfile.TarInfo(path)
                member.mode, member.size = 0o644, len(content)
                archive.addfile(member, io.BytesIO(content))
                records[kind] = {'path': path, 'sha256': hashlib.sha256(content).hexdigest()}
        return layer, records

    def test_rooted_absolute_links_and_merged_usr(self):
        (self.root / 'usr/lib').mkdir(parents=True)
        (self.root / 'lib').symlink_to('/usr/lib')
        (self.root / 'usr/lib/actual').write_bytes(b'rootfs bytes')
        (self.root / 'usr/lib/link').symlink_to('/lib/actual')
        self.assertEqual(self.fs.read('/lib/link'), b'rootfs bytes')
        self.assertEqual(self.fs.canon('/../../lib/link'), '/usr/lib/actual')
        self.assertFalse(self.fs.exists('/etc/passwd'))
        (self.root / 'usr/lib/loop').symlink_to('/lib/loop')
        self.assertIsNone(self.fs.canon('/lib/loop'))

    def test_layer_writes_follow_image_links_not_host_links(self):
        layer = Path(self.temp.name) / 'layer.tar'
        with tarfile.open(layer, 'w') as archive:
            for name, kind, target, data in (
                ('usr/lib', tarfile.DIRTYPE, '', None),
                ('lib', tarfile.SYMTYPE, '/usr/lib', None),
                ('lib/staged', tarfile.REGTYPE, '', b'new bytes'),
            ):
                member = tarfile.TarInfo(name)
                member.type, member.linkname, member.mode = kind, target, 0o755
                member.size = len(data) if data else 0
                archive.addfile(member, io.BytesIO(data) if data else None)
        self.fs.apply_layer(layer)
        self.assertEqual((self.root / 'usr/lib/staged').read_bytes(), b'new bytes')
        self.assertEqual(self.fs.rmeta('/usr/lib/staged')[0], stat.S_IFREG | 0o755)

    def test_layer_rejects_parent_components_before_writes(self):
        self.root.parent.chmod(0o755)
        (self.root / 'target').write_bytes(b'target')
        cases = [
            ('..', tarfile.DIRTYPE, '', 'parent component'),
            ('linked', tarfile.LNKTYPE, '../target', 'parent component'),
            ('var/lib/docker/../escape', tarfile.REGTYPE, '', 'parent component'),
            ('.wh.', tarfile.REGTYPE, '', 'invalid rootfs whiteout target'),
            ('.wh..', tarfile.REGTYPE, '', 'invalid rootfs whiteout target'),
            ('.wh...', tarfile.REGTYPE, '', 'invalid rootfs whiteout target'),
        ]
        for index, (name, kind, target, error) in enumerate(cases):
            with self.subTest(name=name, target=target):
                layer = Path(self.temp.name) / ('parent-' + str(index) + '.tar')
                with tarfile.open(layer, 'w') as archive:
                    before = tarfile.TarInfo('before-invalid')
                    before.size = 1
                    archive.addfile(before, io.BytesIO(b'x'))
                    member = tarfile.TarInfo(name)
                    member.type, member.linkname, member.mode = kind, target, 0
                    member.size = 1 if kind == tarfile.REGTYPE else 0
                    archive.addfile(member, io.BytesIO(b'x') if member.size else None)
                with self.assertRaisesRegex(ValueError, error):
                    self.fs.apply_layer(layer, skip_prefixes=('var/lib/docker',))
                self.assertEqual(stat.S_IMODE(self.root.parent.stat().st_mode), 0o755)
                self.assertFalse((self.root / 'before-invalid').exists())

        layer = Path(self.temp.name) / 'relative-symlink.tar'
        with tarfile.open(layer, 'w') as archive:
            member = tarfile.TarInfo('relative-link')
            member.type, member.linkname = tarfile.SYMTYPE, '../usr/bin'
            archive.addfile(member)
        self.fs.apply_layer(layer)
        self.assertEqual(os.readlink(self.root / 'relative-link'), '../usr/bin')

    def test_whiteouts_apply_before_this_layers_normal_entries(self):
        for index, whiteout in enumerate(('d/.wh.sub', 'd/.wh..wh..opq')):
            for order in ('first', 'last'):
                with self.subTest(whiteout=whiteout, order=order):
                    case = self.root / ('whiteout-' + str(index) + '-' + order)
                    case.mkdir()
                    fs = Rootfs(str(case))
                    lower = Path(self.temp.name) / ('lower-' + str(index) + '-' + order + '.tar')
                    with tarfile.open(lower, 'w') as archive:
                        member = tarfile.TarInfo('d/sub/lower')
                        member.size = 5
                        archive.addfile(member, io.BytesIO(b'lower'))
                    fs.apply_layer(lower)

                    upper = Path(self.temp.name) / ('upper-' + str(index) + '-' + order + '.tar')
                    entries = [('d/sub/new', b'first'), ('d/sub/new', b'upper')]
                    entries.insert(0 if order == 'first' else len(entries), (whiteout, b''))
                    with tarfile.open(upper, 'w') as archive:
                        for name, content in entries:
                            member = tarfile.TarInfo(name)
                            member.size = len(content)
                            archive.addfile(member, io.BytesIO(content))
                    fs.apply_layer(upper)
                    files = {path.relative_to(case).as_posix(): path.read_bytes() for path in case.rglob('*') if path.is_file()}
                    self.assertEqual(files, {'d/sub/new': b'upper'})

    def test_script_prereqs_and_tsort(self):
        script = 'PREREQ="udev"\nprereqs() { echo "$PREREQ"; }\ncase "$1" in\nprereqs) prereqs\nexit 0;;\nesac\n'
        self.assertEqual(script_prereqs(script, 'synthetic-hook'), 'udev')
        self.assertEqual(gnu_tsort([('udev', 'dmsetup'), ('dmsetup', 'dmsetup'), ('zz-busybox', 'klibc-utils')]),
                         ['udev', 'zz-busybox', 'dmsetup', 'klibc-utils'])
        with self.assertRaisesRegex(ValueError, 'loop'):
            gnu_tsort([('a', 'b'), ('b', 'a')])
        with self.assertRaisesRegex(ValueError, 'cannot determine'):
            script_prereqs('case "$1" in\nprereqs) /bin/target-tool;;\nesac', 'unknown')

    def test_xz_module_decompression_preserves_signed_bytes_and_mode(self):
        modules = self.root / 'modules'
        modules.mkdir()
        payload = b'module ELF bytes\x00~Module signature appended~'
        source = modules / 'virtio.ko.xz'
        source.write_bytes(lzma.compress(payload))
        source.chmod(0o644)
        mk = object.__new__(Mkinitramfs)
        mk.dmeta = {}
        mk.decompress_modules(str(modules))
        output = modules / 'virtio.ko'
        self.assertEqual(output.read_bytes(), payload)
        self.assertEqual(output.stat().st_mode & 0o777, 0o644)
        self.assertFalse(source.exists())
        self.assertEqual(mk.dmeta[output.stat().st_ino][0], stat.S_IFREG | 0o644)

    def test_unknown_hook_is_rejected_before_any_execution(self):
        layer = Path(self.temp.name) / 'unknown-hook.tar'
        content = b'#!/bin/sh\necho unreviewed\n'
        with tarfile.open(layer, 'w') as archive:
            member = tarfile.TarInfo('etc/initramfs-tools/hooks/unreviewed-hook')
            member.mode, member.size = 0o755, len(content)
            archive.addfile(member, io.BytesIO(content))
        self.fs.apply_layer(layer)
        mk = object.__new__(Mkinitramfs)
        mk.r, mk.hooks_run = self.fs, []
        with self.assertRaisesRegex(ValueError, 'unpinned initramfs hook'):
            mk.run_hooks('/etc/initramfs-tools/hooks')
        self.assertEqual(mk.hooks_run, [])

    def test_trixie_udev_copies_dlopen_library_hwdb_and_merges_link_dropins(self):
        # udev 257's installed hook explicitly requires these resources even
        # when libkmod is absent from systemd-udevd's DT_NEEDED entries.
        files = {
            'usr/lib/systemd/systemd-udevd': b'daemon',
            'usr/bin/udevadm': b'udevadm',
            'usr/lib/x86_64-linux-gnu/libkmod.so.2': b'dlopen library',
            'usr/lib/udev/ata_id': b'ata helper',
            'usr/lib/udev/scsi_id': b'scsi helper',
            'usr/sbin/blkid': b'blkid',
            'usr/lib/udev/hwdb.bin': b'compiled hardware database',
            'usr/lib/systemd/network/10-test.link.d/package.conf': b'package',
            'etc/systemd/network/10-test.link.d/local.conf': b'local',
            'usr/lib/udev/rules.d/60-persistent-storage.rules': b'package rule',
            'etc/udev/rules.d/60-persistent-storage.rules': b'override rule',
            'etc/udev/rules.d/70-local.rules': b'custom rule',
        }
        layer = Path(self.temp.name) / 'udev.tar'
        with tarfile.open(layer, 'w') as archive:
            for path, data in files.items():
                member = tarfile.TarInfo(path)
                member.mode, member.size = 0o644, len(data)
                archive.addfile(member, io.BytesIO(data))
        self.fs.apply_layer(layer)
        (self.root / 'sbin').symlink_to('usr/sbin')
        (self.root / 'staged/usr/sbin').mkdir(parents=True)
        (self.root / 'staged/sbin').symlink_to('usr/sbin')
        mk = object.__new__(Mkinitramfs)
        mk.r, mk.D, mk.dmeta, mk.log = self.fs, '/staged', {}, []
        mk.hook_udev()
        out = self.root / 'staged'
        for path in ('usr/lib/x86_64-linux-gnu/libkmod.so.2', 'usr/lib/udev/hwdb.bin',
                     'usr/lib/systemd/network/10-test.link.d/package.conf'):
            self.assertEqual((out / path).read_bytes(), files[path])
        self.assertEqual((out / 'usr/lib/systemd/network/10-test.link.d/local.conf').read_bytes(), b'local')
        self.assertEqual((out / 'usr/lib/udev/rules.d/60-persistent-storage.rules').read_bytes(), b'override rule')
        self.assertEqual((out / 'usr/lib/udev/rules.d/70-local.rules').read_bytes(), b'custom rule')

    def test_kernel_release_comes_from_producer_metadata(self):
        metadata = Path(self.temp.name) / 'kernel.json'
        release = 'different-release+vendor.7'
        value = {'schema': 1, 'architecture': 'arm64', 'kernel_release': release,
                 'boot': {kind: {'path': 'boot/' + kind + '-' + release, 'sha256': '0' * 64}
                          for kind in ('vmlinuz', 'config', 'System.map')}}
        metadata.write_text(json.dumps(value))
        parsed = read_kernel_metadata(metadata)
        self.assertEqual(parsed['kernel_release'], 'different-release+vendor.7')
        self.assertEqual(parsed['architecture'], 'arm64')
        value['kernel_release'] = '../../host'
        metadata.write_text(json.dumps(value))
        with self.assertRaisesRegex(ValueError, 'invalid kernel release'):
            read_kernel_metadata(metadata)

    def test_copy_exec_rejects_unsupported_elf_architecture(self):
        def header(machine, elfclass=2, encoding=1):
            ident = b'\x7fELF' + bytes([elfclass, encoding, 1]) + bytes(9)
            if elfclass == 1:
                return struct.pack('<16sHHIIIIIHHHHHH', ident, 2, machine, 1, 0, 52, 0, 0, 52, 32, 0, 40, 0, 0)
            return struct.pack('<16sHHIQQQIHHHHHH', ident, 2, machine, 1, 0, 64, 0, 0, 64, 56, 0, 64, 0, 0)

        (self.root / 'usr/bin').mkdir(parents=True)
        (self.root / 'staged/usr/bin').mkdir(parents=True)
        cases = [
            ('amd64', header(183), 'architecture mismatch'),
            ('arm64', header(62), 'architecture mismatch'),
            ('amd64', header(3, elfclass=1), 'class or encoding'),
            ('amd64', header(62, encoding=2), 'class or encoding'),
            ('amd64', header(62), None),
            ('arm64', header(183), None),
            ('amd64', b'#!/bin/sh\nexit 0\n', None),
        ]
        for index, (architecture, content, error) in enumerate(cases):
            with self.subTest(architecture=architecture, error=error):
                name = 'program-' + str(index)
                source = self.root / 'usr/bin' / name
                source.write_bytes(content)
                source.chmod(0o755)
                self.fs.meta['usr/bin/' + name] = (stat.S_IFREG | 0o755, 0, 0, 0)
                mk = object.__new__(Mkinitramfs)
                mk.r, mk.arch, mk.a = self.fs, architecture, ARCH[architecture]
                mk.D, mk.dmeta, mk.log, mk.cache = '/staged', {}, [], None
                if error:
                    with self.assertRaisesRegex(ValueError, error):
                        mk.copy_exec('/usr/bin/' + name)
                else:
                    self.assertEqual(mk.copy_exec('/usr/bin/' + name), 0)
                    self.assertEqual((self.root / 'staged/usr/bin' / name).read_bytes(), content)

    def test_klibc_and_modules_match_target_architecture(self):
        def header(machine, kind):
            ident = b'\x7fELF' + bytes([2, 1, 1]) + bytes(9)
            return struct.pack('<16sHHIQQQIHHHHHH', ident, kind, machine, 1, 0, 64, 0, 0, 64, 56, 0, 64, 0, 0)

        for name in ('usr/lib/klibc/bin', 'staged/bin', 'staged/lib', 'staged/sbin',
                     'lib/modules/vendor-release', 'staged/lib/modules/vendor-release'):
            (self.root / name).mkdir(parents=True, exist_ok=True)
        program = self.root / 'usr/lib/klibc/bin/cat'
        library = self.root / 'usr/lib/klibc-test.so'
        module = self.root / 'lib/modules/vendor-release/test.ko'
        program.write_bytes(header(62, 2))
        library.write_bytes(header(62, 3))
        module.write_bytes(header(62, 1))
        for path in (program, library, module):
            self.fs.meta[path.relative_to(self.root).as_posix()] = (stat.S_IFREG | 0o755, 0, 0, 0)
        mk = object.__new__(Mkinitramfs)
        mk.r, mk.D, mk.dmeta, mk.log = self.fs, '/staged', {}, []
        mk.arch, mk.a, mk.version = 'arm64', ARCH['arm64'], 'vendor-release'
        with self.assertRaisesRegex(ValueError, 'klibc program.*architecture mismatch'):
            mk.hook_klibc_utils()
        program.write_bytes(header(183, 2))
        with self.assertRaisesRegex(ValueError, 'klibc library.*architecture mismatch'):
            mk.hook_klibc_utils()
        library.write_bytes(header(183, 3))
        mk.hook_klibc_utils()
        self.assertEqual((self.root / 'staged/bin/cat').read_bytes(), program.read_bytes())
        self.assertEqual((self.root / 'staged/lib/klibc-test.so').read_bytes(), library.read_bytes())
        modprobe = SimpleNamespace(stdout=('insmod ' + str(module) + '\n').encode())
        with patch.object(mk, 'kmod_tool', return_value=modprobe):
            with self.assertRaisesRegex(ValueError, 'kernel module.*architecture mismatch'):
                mk.manual_add_modules(['test'])
        module.write_bytes(header(183, 1))
        mk.D, mk.dmeta = '/second-staged', {}
        (self.root / 'second-staged').mkdir()
        with patch.object(mk, 'kmod_tool', return_value=modprobe):
            mk.manual_add_modules(['test'])
        self.assertEqual((self.root / 'second-staged/usr/lib/modules/vendor-release/test.ko').read_bytes(), module.read_bytes())
        firmware = self.root / 'lib/firmware/test.bin'
        firmware.parent.mkdir(parents=True)
        firmware.write_bytes(b'opaque device firmware')
        self.fs.meta['lib/firmware/test.bin'] = (stat.S_IFREG | 0o644, 0, 0, 0)
        self.assertTrue(mk.add_firmware('test.bin'))
        self.assertEqual((self.root / 'second-staged/usr/lib/firmware/test.bin').read_bytes(), firmware.read_bytes())

    def test_rootfs_boot_bytes_and_module_directory_must_match_the_producer(self):
        layer, records = self.kernel_layer('kernel')
        metadata = Path(self.temp.name) / 'producer.json'
        value = {'schema': 1, 'architecture': 'amd64', 'kernel_release': 'vendor-release',
                 'module_directory': 'usr/lib/modules/vendor-release', 'boot': records}
        spec = {'layers': [str(layer)], 'kernel_metadata': str(metadata)}
        metadata.write_text(json.dumps(dict(value, boot={})))
        with self.assertRaisesRegex(ValueError, 'missing kernel boot record'):
            build_initrd(spec, str(self.root / 'initrd'), str(self.root / 'audit.json'))
        expected_digest = records['vmlinuz']['sha256']
        records['vmlinuz']['sha256'] = '0' * 64
        metadata.write_text(json.dumps(value))
        with self.assertRaisesRegex(ValueError, 'boot file differs'):
            build_initrd(spec, str(self.root / 'initrd'), str(self.root / 'audit.json'))
        records['vmlinuz']['sha256'] = expected_digest
        metadata.write_text(json.dumps(value))
        with self.assertRaisesRegex(ValueError, 'lacks matching producer module directory'):
            build_initrd(spec, str(self.root / 'initrd'), str(self.root / 'audit.json'))

    def test_module_directory_resolves_to_standard_kmod_tree(self):
        cases = [
            ('alternate', 'opt/vendor/modules/vendor-release',
             ('lib/modules/vendor-release', 'opt/vendor/modules/vendor-release'), (), False),
            ('merged-usr', 'usr/lib/modules/vendor-release',
             ('usr/lib/modules/vendor-release',), (('lib', 'usr/lib'),), True),
        ]
        for name, module_directory, directories, symlinks, accepted in cases:
            with self.subTest(name=name):
                layer, records = self.kernel_layer(name, directories, symlinks)
                metadata = Path(self.temp.name) / (name + '.json')
                metadata.write_text(json.dumps({'schema': 1, 'architecture': 'amd64', 'kernel_release': 'vendor-release',
                                                'module_directory': module_directory, 'boot': records}))
                observed = []

                class ProbeAssembler:
                    def __init__(probe, root, spec, work):
                        observed.append(root.canon('/lib/modules/vendor-release'))

                    def build(probe, confdir):
                        raise RuntimeError('probe stopped before assembly')

                spec = {'layers': [str(layer)], 'kernel_metadata': str(metadata)}
                with patch('mkinitramfs.Mkinitramfs', ProbeAssembler), patch('mkinitramfs.prepare_module_indexes'):
                    if accepted:
                        with self.assertRaisesRegex(RuntimeError, 'probe stopped before assembly'):
                            build_initrd(spec, str(self.root / 'initrd'), str(self.root / 'audit.json'))
                        self.assertEqual(observed, ['/usr/lib/modules/vendor-release'])
                    else:
                        with self.assertRaisesRegex(ValueError, 'lacks matching producer module directory'):
                            build_initrd(spec, str(self.root / 'initrd'), str(self.root / 'audit.json'))
                        self.assertEqual(observed, [])

    def test_extra_config_replaces_final_symlink_inside_rootfs(self):
        sentinel = Path(self.temp.name) / 'outside-rootfs-sentinel'
        sentinel.write_bytes(b'preserve this sentinel')
        extra = Path(self.temp.name) / 'override'
        extra.write_bytes(b'MODULES=list\n')
        layer, records = self.kernel_layer(
            'config-rootfs', ('lib/modules/vendor-release', 'etc/initramfs-tools/conf.d'),
            (('etc/initramfs-tools/conf.d/override', sentinel),))
        metadata = Path(self.temp.name) / 'config-kernel.json'
        metadata.write_text(json.dumps({'schema': 1, 'architecture': 'amd64', 'kernel_release': 'vendor-release',
                                        'module_directory': 'lib/modules/vendor-release', 'boot': records}))
        observed = {}

        class ProbeAssembler:
            def __init__(probe, root, spec, work):
                probe.root = root

            def build(probe, confdir):
                path = probe.root.host(probe.root.canon(confdir + '/conf.d/override', follow=False))
                observed['regular'] = stat.S_ISREG(os.lstat(path).st_mode)
                if observed['regular']:
                    observed['bytes'] = Path(path).read_bytes()
                raise RuntimeError('probe stopped before assembly')

        spec = {'layers': [str(layer)], 'kernel_metadata': str(metadata), 'extra_config': [str(extra)]}
        with patch('mkinitramfs.Mkinitramfs', ProbeAssembler), patch('mkinitramfs.prepare_module_indexes'):
            with self.assertRaisesRegex(RuntimeError, 'probe stopped before assembly'):
                build_initrd(spec, str(self.root / 'initrd'), str(self.root / 'audit.json'))
        self.assertEqual(sentinel.read_bytes(), b'preserve this sentinel')
        self.assertTrue(observed['regular'])
        self.assertEqual(observed['bytes'], extra.read_bytes())

    def test_busybox_source_identity_ignores_only_binary_rebuild_suffix(self):
        (self.root / 'var/lib/dpkg').mkdir(parents=True)
        (self.root / 'staged/bin').mkdir(parents=True)
        status = self.root / 'var/lib/dpkg/status'
        binary = self.root / 'staged/bin/busybox'
        mk = object.__new__(Mkinitramfs)
        mk.r, mk.D, mk.conf = self.fs, '/staged', {}
        mk.busybox_manifest = {'version': '1:1.37.0-6', 'applets': ['bin/cat']}
        for version, valid in (
            ('1:1.37.0-6', True),
            ('1:1.37.0-6+b1', True),
            ('1:1.37.0-6+b12', True),
            ('1:1.37.0-7+b1', False),
            ('1:1.37.0-6+b1extra', False),
        ):
            with self.subTest(version=version):
                status.write_text('Package: busybox\nVersion: ' + version + '\n\n')
                with patch.object(mk, 'copy_exec', side_effect=lambda *args: binary.write_bytes(b'busybox')):
                    if valid:
                        mk.hook_zz_busybox()
                        alias = self.root / 'staged/bin/cat'
                        self.assertEqual(alias.stat().st_ino, binary.stat().st_ino)
                        alias.unlink()
                    else:
                        with self.assertRaisesRegex(ValueError, 'deb/source version mismatch'):
                            mk.hook_zz_busybox()


class CompressionContractTest(unittest.TestCase):
    def test_newc_rejects_invalid_header_values(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / 'root'
            root.mkdir()
            (root / 'file').write_bytes(b'payload')
            for field in ('mode', 'mtime'):
                for index, value in enumerate((-1, 2**32, 1.5, '1', True)):
                    with self.subTest(field=field, value=value):
                        destination = Path(directory) / (field + str(index) + '.cpio')

                        def metadata(path, st):
                            return (value, 0) if field == 'mode' else (st.st_mode, value)

                        with self.assertRaisesRegex(ValueError, 'newc ' + field):
                            write_newc(root, destination, metadata)
                        self.assertFalse(destination.exists() and destination.stat().st_size)

    def test_zstd_module_requires_explicit_decoder(self):
        mk = object.__new__(Mkinitramfs)
        mk.zstd = None
        with self.assertRaisesRegex(ValueError, 'declared zstd executable'):
            mk.module_bytes('module.ko.zst')

    def test_configured_gzip_and_xz_decode_original_cpio(self):
        import gzip
        import lzma
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "input.cpio"
            source.write_bytes(b"070701" + b"cpio payload\0" * 200)
            for kind, decode in [("gzip", gzip.decompress), ("xz", lzma.decompress)]:
                for level in [1, 9]:
                    destination = Path(directory) / (kind + str(level))
                    if kind == "gzip" and not TOOLS.pigz:
                        with self.assertRaisesRegex(ValueError, "declared pigz executable"):
                            compress_initrd(source, destination, {"compression": kind, "compression_level": level})
                        continue
                    compress_initrd(source, destination, {"compression": kind, "compression_level": level, "pigz": TOOLS.pigz})
                    self.assertEqual(decode(destination.read_bytes()), source.read_bytes())


if __name__ == '__main__':
    unittest.main()
