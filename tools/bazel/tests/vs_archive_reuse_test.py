"""Protect VS archive reuse against stale outputs and mismatched debug images."""

import copy
import gzip
import hashlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import vs_archive_reuse as reuse


def encoded(value):
    return json.dumps(value, sort_keys=True).encode()


def tar_bytes(entries):
    output = io.BytesIO()
    with tarfile.open(fileobj=output, mode="w") as archive:
        for name, content in entries:
            member = tarfile.TarInfo(name)
            member.size = len(content)
            archive.addfile(member, io.BytesIO(content))
    return output.getvalue()


class ArchiveReuseTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.workspace = Path(temporary.name)
        self.target = self.workspace / "target"
        (self.target / ".container-build-method").mkdir(parents=True)
        (self.workspace / ".arch").write_text("amd64\n")
        (self.workspace / ".platform").write_text("vs\n")
        self.runtime_manifest = {"version": "1.0.0", "package": {"name": "swss", "version": "1.0.0"},
                                 "service": {"name": "swss"}}
        self.layers = [tar_bytes([("usr/bin/orchagent", b"runtime payload")]),
                       tar_bytes([("etc/swss/config", b"configuration")])]
        self.debug_layers = self.layers + [tar_bytes([("usr/lib/debug/orchagent.debug", b"symbols")])]
        self.write_archive()
        self.write_archive(debug=True)

    def write_archive(self, *, debug=False, change_config=None, layers=None, tag=None,
                      extra_entries=(), manifest=None, config_digest=None, compressed_layers=(),
                      change_stored_layers=None):
        name = reuse.DEBUG if debug else reuse.RUNTIME
        manifest = copy.deepcopy(manifest or self.runtime_manifest)
        if debug:
            manifest["package"]["version"] += ".dbg" if "+" in manifest["package"]["version"] else "+dbg"
        directory = self.target / "bazel-manifests" / name
        directory.mkdir(parents=True, exist_ok=True)
        (directory / "manifest.json").write_bytes(encoded(manifest))
        (self.target / ".container-build-method" / (name + ".gz")).write_text("bazel\n")
        layers = layers if layers is not None else self.debug_layers if debug else self.layers
        diff_ids = [hashlib.sha256(layer).hexdigest() for layer in layers]
        config = {"os": "linux", "architecture": "amd64", "config": {
            "Entrypoint": ["/usr/bin/docker-init.sh"],
            "Env": ["DEBIAN_FRONTEND=noninteractive", "PRIVATE_TEST_VALUE=never-publish-me"],
            "Labels": {reuse.LABEL: json.dumps(manifest), "Tag": "same-build"}, "WorkingDir": "/"},
            "rootfs": {"type": "layers", "diff_ids": ["sha256:" + digest for digest in diff_ids]}}
        if change_config:
            change_config(config)
        config_bytes = encoded(config)
        config_name = "blobs/sha256/" + (config_digest or hashlib.sha256(config_bytes).hexdigest())
        stored_layers = [gzip.compress(layer, mtime=0) if index in compressed_layers else layer
                         for index, layer in enumerate(layers)]
        if change_stored_layers:
            change_stored_layers(stored_layers)
        layer_names = ["blobs/sha256/" + hashlib.sha256(layer).hexdigest() + ".tar.gz"
                       for layer in stored_layers]
        entries = [(config_name, config_bytes), *zip(layer_names, stored_layers), ("manifest.json", encoded([
            {"Config": config_name, "Layers": layer_names, "RepoTags": [tag or name + ":latest"]}]))]
        path = self.target / (name + ".gz")
        path.write_bytes(gzip.compress(tar_bytes([*entries, *extra_entries]), mtime=0))
        return path

    def test_capture_and_verify_complete_contents_without_publishing_settings(self):
        receipt = reuse.capture(self.workspace)
        result = reuse.verify(self.workspace, receipt)
        self.assertEqual(receipt["runtime"]["sha256"], hashlib.sha256(
            (self.target / "docker-orchagent.gz").read_bytes()).hexdigest())
        self.assertEqual(result["debug"]["layers"][:2], result["runtime"]["layers"])
        self.assertEqual(set(result["validation_seconds"]), {"capture", "verify"})
        self.assertNotIn("never-publish-me", json.dumps(result))
        self.assertNotIn(str(self.workspace), json.dumps(result))

    def test_compressed_base_layer_and_raw_source_layer_match_uncompressed_rootfs(self):
        self.write_archive(compressed_layers={0})
        self.write_archive(debug=True, compressed_layers={0, 2})
        receipt = reuse.capture(self.workspace)
        result = reuse.verify(self.workspace, receipt)
        self.assertEqual(receipt["runtime"]["rootfs"], [
            "sha256:" + hashlib.sha256(layer).hexdigest() for layer in self.layers])
        self.assertEqual(receipt["runtime"]["layers"][0]["sha256"],
                         hashlib.sha256(gzip.compress(self.layers[0], mtime=0)).hexdigest())
        self.assertEqual(result["debug"]["layers"][:2], receipt["runtime"]["layers"])

    def test_rejects_corrupted_or_truncated_inner_gzip_layer(self):
        def corrupt_crc(layers):
            changed = bytearray(layers[0])
            changed[-8] ^= 1
            layers[0] = bytes(changed)

        def truncate(layers):
            layers[0] = layers[0][:-4]

        for change in (corrupt_crc, truncate):
            with self.subTest(change=change.__name__):
                self.write_archive(compressed_layers={0}, change_stored_layers=change)
                with self.assertRaises((gzip.BadGzipFile, EOFError)):
                    reuse.capture(self.workspace)

    def test_compressed_layers_still_reject_wrong_rootfs_and_changed_debug_base(self):
        self.write_archive(compressed_layers={0},
                           change_config=lambda value: value["rootfs"]["diff_ids"].reverse())
        with self.assertRaisesRegex(ValueError, "layers differ from its rootfs"):
            reuse.capture(self.workspace)
        self.write_archive(compressed_layers={0})
        receipt = reuse.capture(self.workspace)
        self.write_archive(debug=True)  # Same uncompressed content, different stored bytes.
        with self.assertRaisesRegex(ValueError, "exact runtime layers"):
            reuse.verify(self.workspace, receipt)

    def test_runtime_only_does_not_require_debug_image(self):
        receipt = reuse.capture(self.workspace)
        (self.target / "docker-orchagent-dbg.gz").unlink()
        result = reuse.verify(self.workspace, receipt, runtime_only=True)
        self.assertIn("verify_runtime", result["validation_seconds"])
        self.assertNotIn("debug", result)

    def test_rejects_changed_runtime_even_with_valid_image_metadata(self):
        receipt = reuse.capture(self.workspace)
        self.write_archive(layers=[tar_bytes([("usr/bin/orchagent", b"changed payload")])])
        with self.assertRaisesRegex(ValueError, "runtime archive changed"):
            reuse.verify(self.workspace, receipt)

    def test_rejects_corruption_after_tar_end_marker(self):
        path = self.target / "docker-orchagent.gz"
        data = bytearray(path.read_bytes())
        data[-8] ^= 1  # gzip CRC is beyond tarfile's final payload member.
        path.write_bytes(data)
        with self.assertRaises((gzip.BadGzipFile, tarfile.ReadError)):
            reuse.capture(self.workspace)

    def test_rejects_wrong_tags_architecture_entrypoint_command_and_environment(self):
        changes = [lambda value: value.update(architecture="arm64"),
                   lambda value: value.update(os="windows"),
                   lambda value: value["config"].update(Entrypoint=["/bin/sh"]),
                   lambda value: value["config"].update(Cmd=["unexpected"]),
                   lambda value: value["config"].update(Env=["DEBIAN_FRONTEND=interactive"])]
        for change in changes:
            with self.subTest(change=change):
                self.write_archive(change_config=change)
                with self.assertRaises(ValueError):
                    reuse.capture(self.workspace)
        self.write_archive(tag="docker-other:latest")
        with self.assertRaisesRegex(ValueError, "tag differs"):
            reuse.capture(self.workspace)

    def test_rejects_layer_bytes_that_do_not_match_rootfs(self):
        self.write_archive(change_config=lambda value: value["rootfs"]["diff_ids"].reverse())
        with self.assertRaisesRegex(ValueError, "layers differ from its rootfs"):
            reuse.capture(self.workspace)

    def test_rejects_config_filename_that_does_not_match_contents(self):
        self.write_archive(config_digest="0" * 64)
        with self.assertRaisesRegex(ValueError, "config filename differs"):
            reuse.capture(self.workspace)

    def test_rejects_truncated_archive(self):
        path = self.target / "docker-orchagent.gz"
        path.write_bytes(path.read_bytes()[:-12])
        with self.assertRaises((EOFError, tarfile.ReadError)):
            reuse.capture(self.workspace)

    def test_rejects_manifest_label_that_does_not_match_make(self):
        self.write_archive(change_config=lambda value: value["config"]["Labels"].update({reuse.LABEL: "{}"}))
        with self.assertRaisesRegex(ValueError, "manifest label differs"):
            reuse.capture(self.workspace)

    def test_rejects_changed_profile_or_make_manifest(self):
        receipt = reuse.capture(self.workspace)
        for path, replacement in ((".arch", "arm64"), (".platform", "broadcom"),
                                  ("target/.container-build-method/docker-orchagent.gz", "make"),
                                  ("target/bazel-manifests/docker-orchagent/manifest.json", "{}")):
            with self.subTest(path=path):
                file = self.workspace / path
                previous = file.read_bytes()
                file.write_text(replacement)
                with self.assertRaisesRegex(ValueError, "profile"):
                    reuse.verify(self.workspace, receipt, runtime_only=True)
                file.write_bytes(previous)

    def test_rejects_debug_layer_reordering_and_replacement(self):
        receipt = reuse.capture(self.workspace)
        for layers in (list(reversed(self.layers)) + self.debug_layers[2:],
                       [tar_bytes([("usr/bin/orchagent", b"different runtime")])] + self.debug_layers[1:]):
            with self.subTest(layers=layers):
                self.write_archive(debug=True, layers=layers)
                with self.assertRaisesRegex(ValueError, "exact runtime layers"):
                    reuse.verify(self.workspace, receipt)

    def test_rejects_debug_runtime_settings_changes(self):
        receipt = reuse.capture(self.workspace)
        self.write_archive(debug=True, change_config=lambda value: value["config"].update(WorkingDir="/other"))
        with self.assertRaisesRegex(ValueError, "changes runtime settings"):
            reuse.verify(self.workspace, receipt)

    def test_rejects_debug_manifest_change_beyond_version_suffix(self):
        receipt = reuse.capture(self.workspace)
        changed = copy.deepcopy(self.runtime_manifest)
        changed["service"]["name"] = "other"
        self.write_archive(debug=True, manifest=changed)
        with self.assertRaisesRegex(ValueError, "beyond the version suffix"):
            reuse.verify(self.workspace, receipt)

    def test_supports_existing_build_metadata_in_version(self):
        self.runtime_manifest["package"]["version"] = "1.0.0+custom"
        self.write_archive()
        self.write_archive(debug=True)
        reuse.verify(self.workspace, reuse.capture(self.workspace))

    def test_rejects_duplicate_members(self):
        self.write_archive(extra_entries=[("manifest.json", b"[]")])
        with self.assertRaisesRegex(ValueError, "duplicate Docker archive"):
            reuse.capture(self.workspace)

    def test_cli_writes_and_updates_receipt(self):
        receipt = self.workspace / "artifacts/receipt.json"
        for action in ("capture", "verify-runtime", "verify"):
            subprocess.run([sys.executable, "-B", str(ROOT / "tools/bazel/ci/vs_archive_reuse.py"),
                            action, "--workspace", str(self.workspace), "--receipt", str(receipt)],
                           check=True, capture_output=True, text=True)
        self.assertEqual(set(json.loads(receipt.read_text())["validation_seconds"]),
                         {"capture", "verify_runtime", "verify"})


if __name__ == "__main__":
    unittest.main()
