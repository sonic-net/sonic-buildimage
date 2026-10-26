"""Check that layer adaptation preserves imports and the base's directory aliases."""
import io
from pathlib import Path
import tarfile
import sys
import tempfile
import unittest

from tools.bazel.oci.tests.imported_symbols_test import layout
from tools.bazel.oci.normalize_layer import DIRECTORY_ALIASES, base_aliases, normalize, normalized_member, normalized_path


def archive(path, entries):
    with tarfile.open(path, 'w', format=tarfile.PAX_FORMAT) as output:
        for member, contents in entries:
            output.addfile(member, io.BytesIO(contents) if member.isfile() else None)


def member(name, kind=tarfile.REGTYPE, *, contents=b'value', link='', mode=0o644, uid=0, gid=0):
    result = tarfile.TarInfo(name)
    result.type, result.mode, result.uid, result.gid = kind, mode, uid, gid
    result.linkname, result.mtime = link, 1700000000
    result.size = len(contents) if kind == tarfile.REGTYPE else 0
    return result, contents


def aliases():
    entries = [member(name, tarfile.DIRTYPE, mode=0o755) for name in sorted({v['target'] for v in DIRECTORY_ALIASES.values()})]
    entries += [member(name, tarfile.SYMTYPE, link=value['linkname'], mode=0o777) for name, value in DIRECTORY_ALIASES.items()]
    return entries


class NormalizeLayerTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)

    def base(self, entries=None, extra=(), architecture='amd64'):
        path = self.root / 'base.tar'
        archive(path, aliases() if entries is None else entries)
        layers = [path]
        for index, entries in enumerate(extra):
            path = self.root / f'extra{index}.tar'
            archive(path, entries)
            layers.append(path)
        result = self.root / 'base'
        layout(result, layers, architecture=architecture)
        return result

    def test_rewrites_paths_links_and_preserves_imported_bytes_and_ownership(self):
        """An imported payload follows merged usr without replacing directory links or owners."""
        base = self.base()
        payload = self.root / 'input.tar'
        archive(payload, [member('lib', tarfile.DIRTYPE, mode=0o755),
                          member('lib/example.so', uid=101, gid=102, mode=0o640),
                          member('lib/hard', tarfile.LNKTYPE, link='lib/example.so'),
                          member('lib/symbolic', tarfile.SYMTYPE, link='example.so', mode=0o777),
                          member('var/run/example', contents=b'run')])
        output = self.root / 'output.tar'
        receipt = normalize(payload, base, output, expected_platform='linux/amd64')
        self.assertEqual(receipt['skipped_alias_entries'], 1)
        self.assertEqual(receipt['rewritten_members'], 4)
        with tarfile.open(output) as result:
            entries = result.getmembers()
            self.assertEqual([m.name for m in entries], ['./usr/lib/example.so', './usr/lib/hard', './usr/lib/symbolic', './run/example'])
            self.assertEqual((entries[0].uid, entries[0].gid, entries[0].mode, entries[0].mtime), (101, 102, 0o640, 1700000000))
            self.assertEqual(result.extractfile(entries[0]).read(), b'value')
            self.assertEqual(entries[1].linkname, './usr/lib/example.so')
            self.assertEqual(entries[2].linkname, 'example.so')

    def test_source_ownership_and_modes_are_opt_in(self):
        """Source TARs can explicitly repair ownership/PAX fields and selected install modes."""
        original, _ = member('lib/example', uid=1000, gid=1001, mode=0o755)
        original.uname, original.gname = 'builder', 'builder'
        original.pax_headers = {'uid': '1000', 'gid': '1001', 'uname': 'builder', 'path': 'lib/example', 'SCHILY.xattr.user.note': 'keep'}
        result = normalized_member(original, root_owned=True, modes={'usr/lib/example': '0644'})
        self.assertEqual((result.uid, result.gid, result.uname, result.gname, result.mode), (0, 0, '', '', 0o644))
        self.assertEqual(result.pax_headers, {'SCHILY.xattr.user.note': 'keep'})
        self.assertEqual((original.uid, original.gid, original.name), (1000, 1001, 'lib/example'))

    def test_source_symlink_mode_is_normalized(self):
        """The source-owned path normalizes symlink modes while imports retain theirs."""
        original, _ = member('lib/example', tarfile.SYMTYPE, link='example.so', mode=0o755)
        self.assertEqual(normalized_member(original).mode, 0o755)
        self.assertEqual(normalized_member(original, root_owned=True).mode, 0o777)

    def test_unsupported_base_alias_fails(self):
        """Normalization cannot assume a directory layout that the selected base does not have."""
        entries = [(m, data) for m, data in aliases() if m.name != 'lib']
        entries.append(member('lib', tarfile.DIRTYPE, mode=0o755))
        with self.assertRaisesRegex(ValueError, 'unsupported directory alias: lib'):
            base_aliases(self.base(entries), expected_platform='linux/amd64')

    def test_whiteout_hides_base_alias(self):
        """A base-layer deletion invalidates the alias even when an earlier layer provided it."""
        with self.assertRaisesRegex(ValueError, 'unsupported directory alias: lib'):
            base_aliases(self.base(extra=[[member('.wh.lib')]]), expected_platform='linux/amd64')

    def test_opaque_whiteout_hides_target_directory(self):
        """Directory opacity must invalidate inherited alias destinations."""
        with self.assertRaisesRegex(ValueError, 'unsupported directory alias'):
            base_aliases(self.base(extra=[[member('usr/.wh..wh..opq')]]), expected_platform='linux/amd64')

    def test_foreign_base_platform_fails(self):
        """The normalizer enforces the consumer's declared OCI platform."""
        with self.assertRaises(ValueError):
            base_aliases(self.base(architecture='arm64'), expected_platform='linux/amd64')

    def test_source_cannot_replace_alias_or_its_metadata(self):
        """Payloads cannot turn inherited directory links into files or writable aliases."""
        for original, _ in [member('lib'), member('lib', tarfile.DIRTYPE, mode=0o777),
                            member('lib', tarfile.SYMTYPE, link='wrong', mode=0o777)]:
            with self.subTest(original=original), self.assertRaises(ValueError):
                normalized_member(original)

    def test_unsafe_paths_and_hardlinks_fail(self):
        """Host-absolute paths and traversal remain rejected after path rewriting."""
        for name in ('/usr/lib/x', '../x', 'lib/../../x'):
            with self.subTest(name=name), self.assertRaisesRegex(ValueError, 'unsafe'):
                normalized_path(name)
        with self.assertRaisesRegex(ValueError, 'unsafe'):
            normalized_member(member('lib/hard', tarfile.LNKTYPE, link='../outside')[0])
        with self.assertRaisesRegex(ValueError, 'hardlink targets a directory alias'):
            normalized_member(member('lib/hard', tarfile.LNKTYPE, link='lib')[0])

    def test_whiteouts_and_device_entries_fail(self):
        """Added payloads cannot hide base files or introduce unsupported device semantics."""
        for original, _ in [member('usr/.wh.lib'), member('device', tarfile.CHRTYPE)]:
            with self.subTest(original=original), self.assertRaises(ValueError):
                normalized_member(original)

    def test_custom_alias_policy(self):
        """Containers may declare different layouts instead of inheriting SONiC assumptions."""
        custom = {'opt/old': {'linkname': 'new', 'target': 'opt/new'}}
        self.assertEqual(normalized_path('opt/old/tool', directory_aliases=custom), 'opt/new/tool')
        self.assertEqual(normalized_path('lib/tool', directory_aliases={}), 'lib/tool')


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == '--fixture':
        base, payload = map(Path, sys.argv[2:])
        with tempfile.TemporaryDirectory() as directory:
            layer = Path(directory) / 'base.tar'
            archive(layer, aliases())
            layout(base, [layer])
        archive(payload, [member('lib/example', uid=101, gid=102, mode=0o640)])
    else:
        unittest.main()
