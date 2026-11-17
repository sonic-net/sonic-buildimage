"""Small synthetic archives verify layer containment without running any tools."""

import io
import json
from pathlib import Path
import subprocess
import tarfile
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from diagnostics import (
    ArtifactIdentifier, ArtifactIndex, ArtifactType, Codes, ComparableArtifact,
    DiagnosticSink, Modifier,
)
from extractor import _unpack_docker_archive, extract_all


class LayerContainmentTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.base = Path(temporary.name)
        self.outside = self.base / "outside"
        self.outside.mkdir()
        self.victim = self.outside / "victim"
        self.victim.write_bytes(b"unchanged")
        self.sequence = 0
        for target in ("subprocess.run", "subprocess.Popen", "socket.socket"):
            blocker = patch(target, side_effect=AssertionError("No process/network allowed"))
            blocker.start()
            self.addCleanup(blocker.stop)

    def archive(self, layers, *, manifest_layers=None):
        self.sequence += 1
        path = self.base / f"image-{self.sequence}.gz"
        names = [f"layer-{n}.tar" for n in range(len(layers))]
        with tarfile.open(path, "w:gz") as outer:
            files = {"manifest.json": json.dumps([{
                "Layers": names if manifest_layers is None else manifest_layers
            }]).encode()}
            for name, members in zip(names, layers):
                stream = io.BytesIO()
                with tarfile.open(fileobj=stream, mode="w") as layer:
                    for member_name, kind, value in members:
                        entry = tarfile.TarInfo(member_name)
                        if kind == "file":
                            entry.size = len(value)
                            layer.addfile(entry, io.BytesIO(value))
                        else:
                            entry.type = {"symlink": tarfile.SYMTYPE,
                                          "hardlink": tarfile.LNKTYPE,
                                          "fifo": tarfile.FIFOTYPE,
                                          "char": tarfile.CHRTYPE,
                                          "block": tarfile.BLKTYPE}[kind]
                            if kind in ("symlink", "hardlink"):
                                entry.linkname = value
                            layer.addfile(entry)
                files[name] = stream.getvalue()
            for name, data in files.items():
                entry = tarfile.TarInfo(name)
                entry.size = len(data)
                outer.addfile(entry, io.BytesIO(data))
        return path, self.base / f"unpacked-{self.sequence}" / "root"

    def context(self):
        self.sequence += 1
        return SimpleNamespace(workdir=self.base / f"work-{self.sequence}",
                               sink=DiagnosticSink(), index=ArtifactIndex(),
                               tools=SimpleNamespace(readelf=object()))

    def source(self, name, make, bazel):
        modifier = Modifier.DEBUG if name.endswith("-dbg.gz") else Modifier.RUNTIME
        return ComparableArtifact(ArtifactIdentifier(name, None, frozenset({modifier})),
                                  bazel, make, ArtifactType.OCI_IMAGE)

    def rejected(self, layers):
        archive, root = self.archive(layers)
        with self.assertRaises(tarfile.FilterError):
            _unpack_docker_archive(archive, root)
        self.assertEqual(self.victim.read_bytes(), b"unchanged")

    def test_member_and_hardlink_traversal_are_rejected(self):
        for name in ("../outside/victim", str(self.victim)):
            with self.subTest(kind="file", name=name):
                self.rejected([[(name, "file", b"overwrite")]])
            with self.subTest(kind="hardlink", name=name):
                self.rejected([[('inside', 'hardlink', name)]])

    def test_manifest_layer_references_stay_inside_outer_archive(self):
        for name in ('../outside/victim', str(self.victim)):
            with self.subTest(name=name):
                archive, root = self.archive([], manifest_layers=[name])
                with self.assertRaises(tarfile.FilterError):
                    _unpack_docker_archive(archive, root)
                self.assertEqual(self.victim.read_bytes(), b'unchanged')

    def test_whiteout_traversal_and_invalid_targets_are_rejected(self):
        for name in ("../outside/.wh.victim", str(self.outside / '.wh.victim'),
                     '.wh.', '.wh..', '.wh...'):
            with self.subTest(name=name):
                self.rejected([[(name, "file", b"")]])

    def test_lower_layer_symlink_cannot_redirect_write_link_or_whiteout(self):
        lower = [("escape", "symlink", str(self.outside))]
        for upper in ([('escape/victim', 'file', b'overwrite')],
                      [('inside', 'hardlink', 'escape/victim')],
                      [('escape/.wh.victim', 'file', b'')],
                      [('escape/.wh..wh..opq', 'file', b'')]):
            with self.subTest(upper=upper):
                self.rejected([lower, upper])

    def test_special_inner_members_are_rejected_before_creation(self):
        with patch("os.mknod", side_effect=AssertionError("device creation forbidden")), \
                patch("os.mkfifo", side_effect=AssertionError("FIFO creation forbidden")):
            for kind in ("fifo", "char", "block"):
                for name in ("dev/unsafe", ".wh.unsafe"):
                    with self.subTest(kind=kind, name=name):
                        self.rejected([[(name, kind, "")]])

    def test_same_layer_names_do_not_skip_unequal_files(self):
        make, _ = self.archive([[("usr/share/value", "file", b"make")]])
        bazel, _ = self.archive([[("usr/share/value", "file", b"bazel")]])
        entries = extract_all(self.context(), [self.source("docker-required.gz", make, bazel)],
                              scoped_oci=True)
        self.assertEqual([entry.identifier.name for entry in entries], ["/usr/share/value"])
        self.assertEqual(entries[0].makeVersion.read_bytes(), b"make")
        self.assertEqual(entries[0].bazelVersion.read_bytes(), b"bazel")

    def test_scoped_bad_archive_is_reported_and_other_sources_continue(self):
        broken_make = self.base / "broken-make.gz"
        broken_bazel = self.base / "broken-bazel.gz"
        broken_make.write_bytes(b"not a gzip archive")
        broken_bazel.write_bytes(b"not a gzip archive")
        make, _ = self.archive([[("usr/share/value", "file", b"value")]])
        bazel, _ = self.archive([[("usr/share/value", "file", b"value")]])
        sources = [self.source("docker-diagnostic.gz", broken_make, broken_bazel),
                   self.source("docker-required.gz", make, bazel)]
        ctx = self.context()
        entries = extract_all(ctx, sources, scoped_oci=True)
        self.assertEqual([entry.identifier.name for entry in entries], ["/usr/share/value"])
        self.assertEqual([(d.artifact.name, d.code) for d in ctx.sink.diagnostics],
                         [("docker-diagnostic.gz", Codes.EXTRACTION_UNREADABLE)])
        with self.assertRaises(OSError):
            extract_all(self.context(), sources)

    def test_scoped_dangling_hardlink_is_reported_and_other_sources_continue(self):
        dangling = [[("inside", "hardlink", "missing")]]
        broken_make, _ = self.archive(dangling)
        broken_bazel, _ = self.archive(dangling)
        make, _ = self.archive([[("usr/share/value", "file", b"value")]])
        bazel, _ = self.archive([[("usr/share/value", "file", b"value")]])
        sources = [self.source("docker-diagnostic.gz", broken_make, broken_bazel),
                   self.source("docker-required.gz", make, bazel)]
        ctx = self.context()
        entries = extract_all(ctx, sources, scoped_oci=True)
        self.assertEqual([entry.identifier.name for entry in entries], ["/usr/share/value"])
        self.assertEqual([(d.artifact.name, d.code) for d in ctx.sink.diagnostics],
                         [("docker-diagnostic.gz", Codes.EXTRACTION_UNREADABLE)])
        with self.assertRaises(KeyError):
            extract_all(self.context(), sources)

    def test_scoped_readelf_errors_belong_to_the_affected_source(self):
        sources = []
        for name in ("diagnostic", "required"):
            members = [(f"usr/bin/{name}", "file", b"\x7fELFruntime"),
                       ("usr/lib/debug/.build-id/aa/bb.debug", "file", b"\x7fELFsymbols")]
            make, _ = self.archive([members])
            bazel, _ = self.archive([members])
            sources.append(self.source(f"docker-{name}.gz", make, bazel))

        def root_name(identifier):
            while identifier.source is not None:
                identifier = identifier.source
            return identifier.name

        for failing in ("diagnostic", "required"):
            with self.subTest(failing=failing):
                def build_id(binary, _readelf):
                    if binary.name == failing:
                        raise subprocess.CalledProcessError(1, ["readelf", "-n", str(binary)])
                    return "aabb"

                with patch("extractor._build_id_of_binary", side_effect=build_id):
                    ctx = self.context()
                    entries = extract_all(ctx, sources, scoped_oci=True)
                    with self.assertRaises(subprocess.CalledProcessError):
                        extract_all(self.context(), sources)
                self.assertEqual([(root_name(d.artifact), d.code) for d in ctx.sink.diagnostics],
                                 [(f"docker-{failing}.gz", Codes.EXTRACTION_UNREADABLE)])
                other = "required" if failing == "diagnostic" else "diagnostic"
                self.assertTrue(any(entry.type is ArtifactType.ELF_DEBUG_INFO
                                    and entry.identifier.name == f"/usr/bin/{other}"
                                    for entry in entries))
                self.assertTrue(any(entry.type is ArtifactType.ELF_EXECUTABLE
                                    and entry.identifier.name == f"/usr/bin/{other}"
                                    for entry in entries))
                self.assertFalse(any(entry.type is ArtifactType.ELF_EXECUTABLE
                                     and entry.identifier.name == f"/usr/bin/{failing}"
                                     for entry in entries))

    def test_scoped_empty_debug_maps_do_not_run_readelf(self):
        runtime = [[("usr/bin/app", "file", b"\x7fELFruntime")]]
        make, _ = self.archive(runtime)
        bazel, _ = self.archive(runtime)
        ctx = self.context()
        with patch("extractor._build_id_of_binary", side_effect=AssertionError("needless readelf")):
            entries = extract_all(ctx, [self.source("docker-required.gz", make, bazel)],
                                  scoped_oci=True)
        self.assertEqual([entry.type for entry in entries], [ArtifactType.ELF_EXECUTABLE])
        self.assertEqual(ctx.sink.diagnostics, [])

    def test_scoped_debug_files_cannot_come_from_another_image_family(self):
        runtime = [("usr/bin/app", "file", b"\x7fELFruntime")]
        symbols = [("usr/lib/debug/.build-id/aa/bb.debug", "file", b"\x7fELFsymbols")]
        make_runtime, _ = self.archive([runtime])
        bazel_runtime, _ = self.archive([runtime])
        make_debug, _ = self.archive([[]])
        bazel_debug, _ = self.archive([symbols])
        diagnostic_make, _ = self.archive([symbols])
        diagnostic_bazel, _ = self.archive([[]])
        sources = [self.source("docker-required.gz", make_runtime, bazel_runtime),
                   self.source("docker-required-dbg.gz", make_debug, bazel_debug),
                   self.source("docker-diagnostic-dbg.gz", diagnostic_make, diagnostic_bazel)]
        with patch("extractor._build_id_of_binary", return_value="aabb"):
            scoped = self.context()
            scoped_entries = extract_all(scoped, sources, scoped_oci=True)
            legacy = self.context()
            legacy_entries = extract_all(legacy, sources)
        self.assertFalse(any(entry.type is ArtifactType.ELF_DEBUG_INFO for entry in scoped_entries))
        self.assertEqual([(d.artifact.source.name, d.code) for d in scoped.sink.diagnostics],
                         [("docker-required.gz", Codes.EXTRACTION_NO_DEBUG)])
        self.assertTrue(any(entry.type is ArtifactType.ELF_DEBUG_INFO for entry in legacy_entries))
        self.assertEqual(legacy.sink.diagnostics, [])

    def test_valid_layers_keep_literal_links_and_apply_whiteouts(self):
        archive, root = self.archive([
            [('usr/bin/tool', 'file', b'program'),
             ('usr/bin/relative', 'symlink', 'tool'),
             ('var/run', 'symlink', '/run'),
             ('old', 'file', b'remove'),
             ('remove-link', 'symlink', str(self.victim)),
             ('cache/old', 'file', b'remove')],
            [('usr/bin/hard', 'hardlink', 'usr/bin/tool'),
             ('.wh.old', 'file', b''),
             ('.wh.remove-link', 'file', b''),
             ('cache/.wh..wh..opq', 'file', b''),
             ('cache/new', 'file', b'keep')],
        ])
        _unpack_docker_archive(archive, root)
        self.assertEqual((root / 'usr/bin/tool').read_bytes(), b'program')
        self.assertTrue((root / 'usr/bin/hard').samefile(root / 'usr/bin/tool'))
        self.assertEqual((root / 'usr/bin/relative').readlink(), Path('tool'))
        self.assertEqual((root / 'var/run').readlink(), Path('/run'))
        self.assertFalse((root / 'old').exists())
        self.assertFalse((root / 'remove-link').is_symlink())
        self.assertFalse((root / 'cache/old').exists())
        self.assertEqual((root / 'cache/new').read_bytes(), b'keep')
        self.assertEqual(self.victim.read_bytes(), b'unchanged')


if __name__ == '__main__':
    unittest.main()
