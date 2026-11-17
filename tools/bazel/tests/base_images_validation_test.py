"""Reject assembled images that differ from their source package receipts."""

from contextlib import ExitStack
import io
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


def write_source_receipt(path, base_digest, **changes):
    """Use the source receipt schema that carries the selector's owner identity."""
    document = {
        "schema": 2, "kind": "bazel_source", "image": "docker-orchagent",
        "architecture": "amd64", "distribution": "trixie", "contract_sha256": "b" * 64,
        "base_manifest_digest": base_digest, "module_file_sha256": {"driver": "c" * 64},
        "packages": [{"package": "local-driver", "version": "2.0", "architecture": "amd64",
            "control_fields": {"Package": "local-driver", "Version": "2.0", "Architecture": "amd64",
                               "Depends": "", "Pre-Depends": "", "Provides": "", "Multi-Arch": "no"},
            "input_tar_sha256": "a" * 64,
            "source": {"module": "driver", "version": "1.0-" + "d" * 40,
                       "commit": "d" * 40, "target": "@driver//:runtime"},
            "files": {"usr/lib/driver.so": {"kind": "file"}}}],
    }
    document.update(changes)
    path.write_text(json.dumps(document))
    return document


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
                source_receipt = directory / "docker-orchagent.source-packages.receipt.json"
                if architecture == "amd64":
                    index = json.loads(oci_files(*image_fixture(architecture))["index.json"])
                    write_source_receipt(source_receipt, index["manifests"][0]["digest"])
                for name in targets:
                    paths[name] = directory / name
                    document = {"source_packages": []}
                    if name.endswith(".installed-checks.json"):
                        document = {"architecture": architecture, "images": [{"name": name.split(".")[0]}]}
                    if name == "docker-orchagent.runtime-apt-selection.json":
                        index = json.loads(oci_files(*image_fixture(architecture))["index.json"])
                        document = {
                            "schema": 1, "variant": "runtime",
                            "source_receipt_sha256": base_images.validation.sha(source_receipt),
                            "base_manifest_digest": index["manifests"][0]["digest"],
                            "dependency_check": {"status": "satisfied"},
                            "selected": [{"package": "runtime-addition"}],
                            "skipped_base": [{"package": "base-package"}],
                        }
                    if name == source_receipt.name:
                        document = json.loads(source_receipt.read_bytes())
                    paths[name].write_text(json.dumps(document))
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
                mock.patch.object(base_images, "installed_inventory", return_value={
                    "run/redis/sonic-db/database_config.json": {"kind": "file", "uid": 0, "gid": 0, "mode": 0o644},
                    "etc/libnl-3/classid": {"kind": "file"},
                    "etc/libnl-3/pktloc": {"kind": "file"},
                }),
                mock.patch.object(base_images, "verify_directory_aliases"),
                mock.patch.object(base_images, "verify_editor_commands"),
                mock.patch.object(base_images, "verify_source_files", return_value={}),
                mock.patch.object(base_images, "verify_symbols", return_value=[]),
                mock.patch.object(base_images.image_validation, "installed_images_policy",
                                  return_value={"architecture": architecture, "images": []}),
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
                self.assertEqual(queries, list(base_images.IMAGES))
                receipt = json.loads((artifacts / "receipt.json").read_text())
                self.assertEqual(receipt["status"], "passed")
                policy_path = artifacts / "installed-images-policy.json"
                self.assertEqual(receipt["installed_images_policy_sha256"], base_images.validation.sha(policy_path))
                self.assertIn("//dockers/docker-base-trixie:docker-base-trixie", audit.call_args.args[2])
                selection_target = "//dockers/docker-orchagent:runtime_apt_selection"
                if architecture == "amd64":
                    self.assertIn(selection_target, audit.call_args.args[2])
                    self.assertIn("//dockers/docker-orchagent:libteam_source_receipt", audit.call_args.args[2])
                    self.assertTrue((artifacts / "docker-orchagent.source-packages.receipt.json").is_file())
                    self.assertEqual(receipt["orchagent_base_selection"], {
                        "base_manifest_digest": receipt["images"]["docker-config-engine-trixie"]["manifest_digest"],
                        "selected_package_count": 1, "skipped_base_package_count": 1,
                    })
                else:
                    self.assertNotIn(selection_target, audit.call_args.args[2])
                    self.assertNotIn("orchagent_base_selection", receipt)
                    self.assertFalse((artifacts / "docker-orchagent.runtime-apt-selection.json").exists())
                    self.assertFalse((artifacts / "docker-orchagent.source-packages.receipt.json").exists())

    def test_cached_archives_materialize_layout_for_each_native_architecture(self):
        for architecture in ("amd64", "arm64"):
            with self.subTest(architecture=architecture):
                self.run_cached_build(architecture)

    def test_failed_layout_build_stops_before_inspection(self):
        self.run_cached_build("amd64", fail_layout_build=True)


class OrchagentBaseSelectionTest(unittest.TestCase):
    """Require the actual source schema and reject unrelated owner/base receipts."""

    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.source_receipt = Path(temporary.name) / "source.json"
        self.digest = "sha256:" + "a" * 64
        write_source_receipt(self.source_receipt, self.digest)
        self.selection = {
            "schema": 1, "variant": "runtime",
            "source_receipt_sha256": base_images.validation.sha(self.source_receipt),
            "base_manifest_digest": "sha256:" + "a" * 64,
            "dependency_check": {"status": "satisfied"},
            "selected": [{"package": "arping"}], "skipped_base": [{"package": "libnl-3-200"}],
        }

    def test_requires_runtime_selection_for_the_exact_source_base(self):
        self.assertNotIn("image", self.selection)
        self.assertNotIn("group", self.selection)
        self.assertEqual(base_images.verify_orchagent_selection(
            self.selection, self.digest, self.source_receipt), {
            "base_manifest_digest": self.digest, "selected_package_count": 1, "skipped_base_package_count": 1,
        })
        for changed in (
                {"base_manifest_digest": "sha256:" + "b" * 64},
                {"schema": 2}, {"variant": "debug"}, {"variant": None, "group": "runtime"},
                {"source_receipt_sha256": "f" * 64}, {"source_receipt_sha256": None},
                {"dependency_check": {"status": "failed"}}, {"dependency_check": {}}):
            selection = {**self.selection, **changed}
            for key, value in changed.items():
                if value is None:
                    selection.pop(key)
            with self.subTest(changed=changed), self.assertRaises(ValueError):
                base_images.verify_orchagent_selection(selection, self.digest, self.source_receipt)
        with self.assertRaisesRegex(ValueError, "different config-engine image"):
            base_images.verify_orchagent_selection(self.selection, "sha256:" + "f" * 64, self.source_receipt)

    def test_matching_hash_does_not_accept_another_owner_or_base(self):
        """A correct digest alone cannot substitute a different image's source receipt."""
        for changed in ({"image": "docker-telemetry"}, {"architecture": "arm64"},
                        {"distribution": "bookworm"}, {"schema": 1},
                        {"base_manifest_digest": "sha256:" + "b" * 64}):
            write_source_receipt(self.source_receipt, self.digest, **changed)
            selection = {**self.selection, "source_receipt_sha256": base_images.validation.sha(self.source_receipt)}
            with self.subTest(changed=changed), self.assertRaises(ValueError):
                base_images.verify_orchagent_selection(selection, self.digest, self.source_receipt)


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
        self.files = {self.path: {**self.item, "kind": "file", "elf_machine": 62}}

    def test_installed_file_matches_source_identity(self):
        native = verify_source_files(self.files, [self.report], 62)
        self.assertEqual(native[self.path]["source_package"], "source-component")

    def test_installed_python_and_startup_bytes_modes_and_owners_are_preserved(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "runtime.tar"
            with tarfile.open(path, "w") as archive:
                member = tarfile.TarInfo("usr/local/lib/python3.13/dist-packages/sonic_grpc/gnoi/system_pb2.py")
                member.size, member.mode = 5, 0o644
                archive.addfile(member, io.BytesIO(b"proto"))
                link = tarfile.TarInfo("usr/bin/owner-alias")
                link.type, link.linkname, link.mode = tarfile.SYMTYPE, "owner-command", 0
                archive.addfile(link)
            files = base_images.validation.payload(path)
            files[link.name]["mode"] = 0o777
            self.assertEqual(len(base_images.verify_installed_layer(files, path)), 2)
            for key, value in (("sha256", "wrong"), ("mode", 0o755), ("uid", 1000), ("gid", 1000),
                               ("kind", "symlink")):
                changed = {**files, member.name: {**files[member.name], key: value}}
                with self.subTest(key=key), self.assertRaisesRegex(ValueError, "changed config-engine"):
                    base_images.verify_installed_layer(changed, path)
            for changed in ({link.name: files[link.name]},
                            {**files, link.name: {**files[link.name], "linkname": "different"}}):
                with self.assertRaisesRegex(ValueError, "changed config-engine"):
                    base_images.verify_installed_layer(changed, path)

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
