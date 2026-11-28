"""Preserve each checked image layer while sharing only identical movable files."""

import gzip
import io
import json
from pathlib import Path
import shlex
import subprocess
import sys
import tarfile
import tempfile
import unittest

from tools.bazel.oci import shared_runtime_layers as layers


ACTION_FILES = {}
for argument in sys.argv[1:]:
    if argument.startswith("--action-"):
        key, value = argument[len("--action-"):].split("=", 1)
        ACTION_FILES[key] = Path(value)
        sys.argv.remove(argument)


class SharedRuntimeLayers(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.pigz = self.root / "pigz"
        self.pigz_code = self.root / "pigz.py"
        self.pigz_code.write_text("""
import gzip, shutil, sys
assert sys.argv[1:] == ["-n", "-6", "-p", "2", "-c"]
with gzip.GzipFile(filename="", fileobj=sys.stdout.buffer, mode="wb", mtime=0) as out:
    shutil.copyfileobj(sys.stdin.buffer, out)
""")
        self.pigz.write_text("#!/bin/sh\nexec " + shlex.quote(sys.executable) + " " +
                             shlex.quote(str(self.pigz_code)) + ' "$@"\n')
        self.pigz.chmod(0o755)

    def member(self, name, data=b"payload", **options):
        info = tarfile.TarInfo(name)
        info.size = len(data)
        info.mode = 0o755
        info.uid, info.gid = 42, 43
        info.mtime = 123
        for key, value in options.items():
            setattr(info, key, value)
        if not info.isfile():
            info.size = 0
            data = b""
        return info, data

    def archive(self, name, members, compressed=False, format=tarfile.PAX_FORMAT):
        path = self.root / name
        with tarfile.open(path, "w:gz" if compressed else "w", format=format) as tar:
            for info, data in members:
                tar.addfile(info, io.BytesIO(data) if info.isfile() else None)
        return path

    def run_split(self, first, second, base_non_dirs=None):
        outputs = [self.root / name for name in ("common.gz", "delta-first.gz", "delta-second.gz", "receipt.json")]
        layers.split(first, second, *outputs, str(self.pigz), base_non_dirs=base_non_dirs)
        return outputs[:3], json.loads(outputs[3].read_text())

    def installed_tree(self, *archives):
        with tempfile.TemporaryDirectory(dir=self.root) as destination:
            for archive in archives:
                subprocess.run(["tar", "--extract", "--file", archive, "--directory", destination,
                                "--no-same-owner"], check=True)
            root = Path(destination)
            tree = {}
            for path in root.rglob("*"):
                name = path.relative_to(root).as_posix()
                if path.is_symlink():
                    tree[name] = ("symlink", str(path.readlink()))
                elif path.is_dir():
                    tree[name] = ("directory",)
                else:
                    tree[name] = ("file", path.read_bytes())
            return tree

    def test_shared_content_and_per_image_differences_preserve_exact_file_metadata(self):
        common = self.member("usr/lib/shared.so", b"shared" * 1000,
                             pax_headers={"SCHILY.xattr.user.probe": "retained"})
        directory = self.member("usr/lib", type=tarfile.DIRTYPE, mode=0o750)
        parent = self.member("usr", type=tarfile.DIRTYPE)
        first = self.archive("first.tar", [parent, directory, common, self.member("usr/bin/tool", b"first")])
        second = self.archive("second.gz", [parent, directory, common, self.member("usr/bin/tool", b"second")], True)
        (shared, left, right), receipt = self.run_split(first, second)
        self.assertEqual(receipt["common_files"], 1)
        self.assertEqual(receipt["common_file_bytes"], 6000)
        self.assertIsNone(receipt["fallback"])
        for original, delta in ((first, left), (second, right)):
            self.assertEqual(layers.inventory(shared)[0] | layers.inventory(delta)[0],
                             layers.inventory(original)[0])

    def test_different_owners_modes_and_xattrs_are_not_shared(self):
        first = self.archive("first.tar", [
            self.member("owner"), self.member("mode"), self.member("xattr")])
        second = self.archive("second.tar", [
            self.member("owner", uid=99), self.member("mode", mode=0o644),
            self.member("xattr", pax_headers={"SCHILY.xattr.user.test": "different"})])
        (_, left, right), receipt = self.run_split(first, second)
        self.assertEqual(receipt["common_files"], 0)
        for original, delta in ((first, left), (second, right)):
            self.assertEqual(gzip.decompress(delta.read_bytes()), original.read_bytes())

    def test_hardlinks_and_their_targets_stay_in_original_layer(self):
        members = [
            self.member("usr", type=tarfile.DIRTYPE),
            self.member("usr/lib", type=tarfile.DIRTYPE),
            self.member("usr/lib/target"),
            self.member("usr/lib/link", type=tarfile.LNKTYPE, linkname="usr/lib/target"),
            self.member("usr/lib/absolute-link", type=tarfile.LNKTYPE, linkname="/usr/lib/target"),
            self.member("usr/lib/movable"),
        ]
        first = self.archive("first.tar", members)
        second = self.archive("second.tar", members)
        (shared, left, right), receipt = self.run_split(first, second)
        self.assertEqual(set(layers.inventory(shared)[0]), {"usr", "usr/lib", "usr/lib/movable"})
        self.assertEqual(receipt["common_files"], 1)
        for original, delta in ((first, left), (second, right)):
            self.assertEqual(layers.inventory(shared)[0] | layers.inventory(delta)[0],
                             layers.inventory(original)[0])

    def test_whiteouts_and_duplicate_paths_use_lossless_fallback(self):
        for members in (
            [self.member("shared"), self.member(".wh.obsolete", b"")],
            [self.member("shared", b"old"), self.member("shared", b"new")],
        ):
            with self.subTest(members=members):
                first = self.archive("first.gz", members, True)
                second = self.archive("second.tar", members)
                (shared, left, right), receipt = self.run_split(first, second)
                self.assertEqual(receipt["common_files"], 0)
                self.assertEqual(layers.inventory(shared)[0], {})
                self.assertEqual(left.read_bytes(), first.read_bytes())
                self.assertEqual(gzip.decompress(right.read_bytes()), second.read_bytes())

    def test_unsafe_paths_fail_before_emitting_layers(self):
        for name in ("/etc/escape", "usr/../escape"):
            with self.subTest(name=name), self.assertRaisesRegex(ValueError, "unsafe"):
                source = self.archive("unsafe.tar", [self.member(name)])
                self.run_split(source, source)

    def test_compression_failure_stops_the_action(self):
        self.pigz_code.write_text("raise SystemExit(3)\n")
        source = self.archive("empty.tar", [])
        with self.assertRaisesRegex(ValueError, "compress"):
            self.run_split(source, source)

    def test_outputs_cannot_overwrite_an_input(self):
        source = self.archive("source.tar", [self.member("keep")])
        original = source.read_bytes()
        with self.assertRaisesRegex(ValueError, "distinct"):
            layers.split(source, source, source, self.root / "a.gz", self.root / "b.gz",
                         self.root / "receipt.json", str(self.pigz))
        self.assertEqual(source.read_bytes(), original)

    def test_long_gnu_paths_preserve_installed_metadata_when_reencoded_as_pax(self):
        member = self.member("usr/include/" + "a" * 110 + "/header.h")
        parents = [self.member(str(parent), type=tarfile.DIRTYPE)
                   for parent in reversed(Path(member[0].name).parents) if str(parent) != "."]
        first = self.archive("gnu.tar", [*parents, member], format=tarfile.GNU_FORMAT)
        second = self.archive("pax.tar", [*parents, member])
        (shared, left, right), receipt = self.run_split(first, second)
        self.assertEqual(receipt["common_files"], 1)
        for original, delta in ((first, left), (second, right)):
            self.assertEqual(layers.inventory(shared)[0] | layers.inventory(delta)[0],
                             layers.inventory(original)[0])

    def test_write_through_symlink_keeps_original_order(self):
        directory = self.member("lib", type=tarfile.DIRTYPE)
        alias = self.member("alias", type=tarfile.SYMTYPE, linkname="lib")
        last = self.member("lib/x", b"last")
        plain = self.archive("plain.tar", [directory, last])
        for inherited_alias in (False, True):
            base = self.archive("base.tar", [directory, alias] if inherited_alias else [])
            prefix = [directory] if inherited_alias else [directory, alias]
            aliased = self.archive("aliased.tar", [*prefix, self.member("alias/x", b"first"), last])
            for first, second in ((aliased, plain), (plain, aliased)):
                with self.subTest(inherited_alias=inherited_alias, first=first.name):
                    base_non_dirs = {"alias"} if inherited_alias else set()
                    (shared, left, right), _ = self.run_split(first, second, base_non_dirs)
                    for original, delta in ((first, left), (second, right)):
                        expected = self.installed_tree(base, original)
                        self.assertEqual(expected["lib/x"], ("file", b"last"))
                        self.assertEqual(self.installed_tree(base, shared, delta), expected)

    def test_late_directory_keeps_writes_through_base_symlink_in_place(self):
        base = self.archive("base.tar", [
            self.member("usr", type=tarfile.DIRTYPE),
            self.member("usr/lib", type=tarfile.DIRTYPE),
            self.member("lib", type=tarfile.SYMTYPE, linkname="usr/lib")])
        directory = self.member("lib", type=tarfile.DIRTYPE)
        payload = self.member("lib/x")
        late = self.archive("late.tar", [payload, directory])
        early = self.archive("early.tar", [directory, payload])
        for first, second in ((late, early), (early, late)):
            with self.subTest(first=first.name):
                (shared, left, right), _ = self.run_split(first, second, {"lib"})
                for original, delta in ((first, left), (second, right)):
                    expected = self.installed_tree(base, original)
                    location = "usr/lib/x" if original == late else "lib/x"
                    self.assertEqual(expected[location], ("file", b"payload"))
                    self.assertEqual(self.installed_tree(base, shared, delta), expected)

    def test_files_can_still_be_shared_before_late_real_directory_headers(self):
        safe = self.member("usr/lib/safe")
        members = [safe, self.member("usr/lib", type=tarfile.DIRTYPE),
                   self.member("usr", type=tarfile.DIRTYPE)]
        source = self.archive("source.tar", members)
        (shared, delta, _), receipt = self.run_split(source, source, set())
        self.assertEqual(receipt["common_files"], 1)
        self.assertEqual(set(layers.inventory(shared)[0]), {"usr/lib/safe"})
        self.assertEqual(self.installed_tree(source), self.installed_tree(shared, delta))

    def test_hardlink_reads_base_contents_before_overwrite_through_alias(self):
        directory = self.member("lib", type=tarfile.DIRTYPE)
        base = self.archive("base.tar", [
            directory, self.member("alias", type=tarfile.SYMTYPE, linkname="lib"),
            self.member("lib/x", b"inherited")])
        source = self.archive("source.tar", [
            directory, self.member("safe"), self.member("keep", type=tarfile.LNKTYPE, linkname="alias/x"),
            self.member("lib/x", b"overwritten")])
        (shared, delta, _), receipt = self.run_split(source, source, {"alias", "lib/x"})
        self.assertEqual(receipt["common_files"], 1)
        expected = self.installed_tree(base, source)
        self.assertEqual(expected["keep"], ("file", b"inherited"))
        self.assertEqual(self.installed_tree(base, shared, delta), expected)

    def test_missing_or_different_parent_directories_keep_files_in_original_layers(self):
        parent = self.member("usr", type=tarfile.DIRTYPE)
        for second_members in ([self.member("usr/file")],
                               [self.member("usr", type=tarfile.DIRTYPE, uid=99),
                                self.member("usr/file")]):
            with self.subTest(second_members=second_members):
                first = self.archive("first.tar", [parent, self.member("usr/file")])
                second = self.archive("second.tar", second_members)
                (_, left, right), receipt = self.run_split(first, second)
                self.assertEqual(receipt["common_files"], 0)
                for original, delta in ((first, left), (second, right)):
                    self.assertEqual(gzip.decompress(delta.read_bytes()), original.read_bytes())

    @unittest.skipUnless(ACTION_FILES, "Bazel action runfiles are not supplied")
    def test_real_bazel_action_preserves_both_declared_inputs(self):
        shared = layers.inventory(ACTION_FILES["common"])[0]
        self.assertTrue(shared)
        for original, delta in (("first", "left"), ("second", "right")):
            self.assertEqual(shared | layers.inventory(ACTION_FILES[delta])[0],
                             layers.inventory(ACTION_FILES[original])[0])

    def test_rejects_unsupported_compression_before_writing_outputs(self):
        plain = self.archive("plain.tar", [self.member("shared")])
        compressed = self.root / "other.tar"
        outputs = ("common.gz", "delta-first.gz", "delta-second.gz", "receipt.json")
        for codec in ("bz2", "xz"):
            for name in ("shared", "different"):
                with self.subTest(codec=codec, name=name):
                    info, data = self.member(name)
                    with tarfile.open(compressed, "w:" + codec) as archive:
                        archive.addfile(info, io.BytesIO(data))
                    for inputs in ((compressed, plain), (plain, compressed)):
                        with self.assertRaisesRegex(ValueError, "expected a plain tar or gzip-compressed tar"):
                            self.run_split(*inputs)
                        self.assertFalse(any((self.root / path).exists() for path in outputs))


if __name__ == "__main__":
    unittest.main()
