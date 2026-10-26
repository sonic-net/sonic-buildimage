"""Reject assembled images that differ from their source package receipts."""

from contextlib import ExitStack
import json
from pathlib import Path
import subprocess
import sys
import tarfile
from types import SimpleNamespace
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci.base_images import verify_directory_aliases, verify_editor_commands, verify_source_files
from tools.bazel.ci import base_images
from tools.bazel.tests.oci_base_fixture import image_fixture, oci_files, write_layout


class CachedImageLayoutTest(unittest.TestCase):
    """An archive cache hit does not promise its intermediate layout exists."""

    def run_cached_build(self, architecture, fail_layout_build=False):
        with tempfile.TemporaryDirectory() as temporary, ExitStack() as stack:
            root = Path(temporary)
            artifacts = root / "artifacts"
            queries = []

            def execute(command, directory, receipt, name, **kwargs):
                if command[0] == "git":
                    return "a" * 40
                if command[0] == "make" or command[1] == "test":
                    return ""
                if command[1] == "build":
                    if fail_layout_build:
                        raise subprocess.CalledProcessError(1, command)
                    for image in base_images.IMAGES:
                        if "//dockers/" + image + ":" + image in command:
                            config, layers = image_fixture(architecture)
                            write_layout(root / "bazel-bin" / image, oci_files(config, layers))
                    return ""
                if command[1] == "cquery":
                    image = command[-1].split(":")[-1]
                    queries.append(image)
                    return "bazel-bin/" + image + "\n"
                self.fail("unexpected command: " + repr(command))

            def cached_archives(workspace, directory, receipt, targets, **kwargs):
                paths = {}
                for name in targets:
                    paths[name] = directory / name
                    paths[name].write_text(json.dumps({"source_packages": []}))
                return paths

            patches = [
                mock.patch.object(base_images, "ROOT", root),
                mock.patch.object(sys, "argv", ["base_images.py", "--artifacts", str(artifacts),
                                               "--architecture", architecture]),
                mock.patch.object(base_images.platform, "machine", return_value={"amd64": "x86_64", "arm64": "aarch64"}[architecture]),
                mock.patch.object(base_images.platform, "freedesktop_os_release", return_value={"VERSION_CODENAME": "trixie"}),
                mock.patch.object(base_images.command_log, "execute", side_effect=execute),
                mock.patch.object(base_images.build, "collect_archives", side_effect=cached_archives),
                mock.patch.object(base_images.resolution, "collect", return_value={}),
                mock.patch.object(base_images, "IMAGES", ("docker-base-trixie",)),
                mock.patch.object(base_images, "verify_directory_aliases"),
                mock.patch.object(base_images, "verify_editor_commands"),
                mock.patch.object(base_images, "verify_source_files", return_value={}),
                mock.patch.object(base_images, "verify_symbols", return_value=[]),
            ]
            for patch in patches:
                stack.enter_context(patch)
            audit = stack.enter_context(mock.patch.object(base_images.bazel_commands, "audit_targets", return_value={}))
            if fail_layout_build:
                with self.assertRaises(subprocess.CalledProcessError):
                    base_images.main()
                self.assertEqual(queries, [])
                self.assertEqual(json.loads((artifacts / "receipt.json").read_text())["status"], "failed")
            else:
                base_images.main()
                self.assertEqual(queries, ["docker-base-trixie"])
                self.assertEqual(json.loads((artifacts / "receipt.json").read_text())["status"], "passed")
                self.assertIn("//dockers/docker-base-trixie:docker-base-trixie", audit.call_args.args[2])

    def test_cached_archives_materialize_layout_for_each_native_architecture(self):
        for architecture in ("amd64", "arm64"):
            with self.subTest(architecture=architecture):
                self.run_cached_build(architecture)

    def test_failed_layout_build_stops_before_inspection(self):
        self.run_cached_build("amd64", fail_layout_build=True)


class DuplicateLayerPathsTest(unittest.TestCase):
    def test_rejects_duplicate_directories_with_equivalent_spelling(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "layer.tar"
            with tarfile.open(path, "w") as archive:
                for name in ("./usr/", "usr"):
                    member = tarfile.TarInfo(name)
                    member.type = tarfile.DIRTYPE
                    archive.addfile(member)
            with self.assertRaisesRegex(ValueError, "repeats a path"):
                base_images.verify_unique_layer_paths(SimpleNamespace(layers=[path]))

    def test_later_layers_may_replace_the_same_path(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "layer.tar"
            with tarfile.open(path, "w") as archive:
                archive.addfile(tarfile.TarInfo("etc/config"))
            base_images.verify_unique_layer_paths(SimpleNamespace(layers=[path, path]))


class SourceImageValidationTest(unittest.TestCase):
    def setUp(self):
        self.path = "usr/bin/source-program"
        self.item = {"path": self.path, "sha256": "a" * 64, "mode": 0o755, "uid": 0, "gid": 0}
        self.report = {"source_packages": [{"package": "source-component", "native_files": [self.item]}]}
        self.files = {self.path: {**self.item, "elf_machine": 62}}

    def test_installed_file_matches_source_identity(self):
        native = verify_source_files(self.files, [self.report], 62)
        self.assertEqual(native[self.path]["source_package"], "source-component")

    def test_later_layer_cannot_replace_source_elf_or_metadata(self):
        for field, value in (("sha256", "b" * 64), ("mode", 0o644), ("uid", 1000), ("gid", 1000)):
            with self.subTest(field=field), self.assertRaisesRegex(ValueError, "changed source"):
                verify_source_files({self.path: {**self.files[self.path], field: value}}, [self.report], 62)

    def test_native_image_cannot_contain_foreign_architecture(self):
        with self.assertRaisesRegex(ValueError, "architecture"):
            verify_source_files(self.files, [self.report], 183)

    def test_distinct_owners_cannot_silently_overwrite_each_other(self):
        with self.assertRaisesRegex(ValueError, "two source owners"):
            verify_source_files(self.files, [self.report, self.report], 62)

    def test_data_paths_must_not_cross_a_preserved_directory_link(self):
        files = {name: {"kind": "symlink", "linkname": target} for name, target in {
            "bin": "usr/bin", "sbin": "usr/sbin", "lib": "usr/lib", "var/run": "/run",
        }.items()}
        verify_directory_aliases(files)  # ARM64 need not have a lib64 alias.
        files["var/run/redis/sonic-db/database_config.json"] = {"kind": "file"}
        with self.assertRaisesRegex(ValueError, "through a directory alias"):
            verify_directory_aliases(files)
        del files["var/run/redis/sonic-db/database_config.json"]
        files["var/run"] = {"kind": "directory"}
        with self.assertRaisesRegex(ValueError, "changed a Debian directory alias"):
            verify_directory_aliases(files)

    def test_base_editor_alternatives_select_the_installed_tiny_binary(self):
        files = {}
        for name in ("editor", "ex", "rview", "vi", "view"):
            files["usr/bin/" + name] = {"kind": "symlink", "linkname": "/etc/alternatives/" + name}
            files["etc/alternatives/" + name] = {"kind": "symlink", "linkname": "/usr/bin/vim.tiny"}
        verify_editor_commands(files)
        files["etc/alternatives/editor"]["linkname"] = "/usr/bin/vim.basic"
        with self.assertRaisesRegex(ValueError, "vim-tiny command"):
            verify_editor_commands(files)
        del files["etc/alternatives/editor"]
        with self.assertRaisesRegex(ValueError, "vim-tiny command"):
            verify_editor_commands(files)


if __name__ == "__main__":
    unittest.main()
