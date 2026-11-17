"""Check scoped input reuse without weakening archive or overlay validation."""

from contextlib import nullcontext
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from tools.bazel.ci import artifact_validation, validation_inputs
from tools.bazel.oci import oci_inventory, package_contract
from tools.bazel.tests.oci_base_fixture import tar_entries


class ValidationInputsTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="validation-inputs-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.layer = self.root / "layer.tar"
        self.layer.write_bytes(tar_entries([("usr/share/example", b"original", 0o644)]))

    def test_hash_each_path_before_sharing_content_inventory(self):
        other = self.root / "same-content.tar"
        other.write_bytes(self.layer.read_bytes())
        with patch.object(oci_inventory, "_layer_entries", wraps=oci_inventory._layer_entries) as read, \
                patch.object(validation_inputs, "_digest", wraps=validation_inputs._digest) as digest:
            with validation_inputs.validation_input_cache([self.root]):
                original, repeated = {}, {}
                oci_inventory.apply_layer(self.layer, original)
                oci_inventory.apply_layer(other, repeated)
                oci_inventory.apply_layer(self.layer, {})
                self.assertEqual(original, repeated)
                self.assertEqual(read.call_count, 1)
                self.assertEqual(digest.call_count, 2)
            oci_inventory.apply_layer(self.layer, {})
            self.assertEqual(read.call_count, 2)  # Nothing persists outside scope.

    def test_returns_independent_inventory_records(self):
        with validation_inputs.validation_input_cache([self.root]):
            files = {}
            oci_inventory.apply_layer(self.layer, files)
            expected = files["usr/share/example"].copy()
            files["usr/share/example"]["uid"] = 123
            oci_inventory.apply_layer(self.layer, files)
            self.assertEqual(files["usr/share/example"], expected)

    def test_statistics_measure_content_reuse_and_separate_final_integrity_reads(self):
        other = self.root / "same-content.tar"
        other.write_bytes(self.layer.read_bytes())
        with validation_inputs.validation_input_cache([self.root]) as cache:
            for path in (self.layer, other, self.layer):
                oci_inventory.apply_layer(path, {})
            stats = cache.statistics()
            self.assertEqual(stats["hash_reads"], 2)
            self.assertEqual(stats["hash_hits"], 1)
            self.assertEqual(stats["hashed_bytes"], self.layer.stat().st_size * 2)
            self.assertEqual(stats["inventory_misses"], 1)
            self.assertEqual(stats["inventory_hits"], 2)
            self.assertEqual(stats["verification_reads"], 0)
            stats["hash_reads"] = 999
            self.assertEqual(cache.statistics()["hash_reads"], 2)
        stats = cache.statistics()
        self.assertEqual(stats["verification_reads"], 2)
        self.assertEqual(stats["verification_bytes"], self.layer.stat().st_size * 2)
        self.assertGreaterEqual(stats["verification_seconds"], 0)

    def test_statistics_record_eviction_and_inventory_limits(self):
        value = {"entry": "metadata"}
        size = validation_inputs._size(value)
        with validation_inputs.validation_input_cache([self.layer], max_bytes=size) as cache:
            for kind in ("first", "second", "first"):
                validation_inputs.cached_inventory(self.layer, kind, lambda: value)
            stats = cache.statistics()
            self.assertEqual(stats["inventory_misses"], 3)
            self.assertEqual(stats["inventory_evictions"], 2)
            self.assertEqual(stats["retained_inventories"], 1)
            self.assertEqual(stats["retained_inventory_bytes"], size)
        for limits, counter in (({"max_bytes": 1}, "inventory_oversized"),
                                ({"max_files": 0}, "inventory_untracked")):
            with self.subTest(limits=limits):
                with validation_inputs.validation_input_cache([self.layer], **limits) as cache:
                    for _ in range(2):
                        validation_inputs.cached_inventory(self.layer, "large", lambda: value)
                    stats = cache.statistics()
                    self.assertEqual(stats[counter], 2)
                    self.assertEqual(stats["inventory_hits"], 0)
                    self.assertEqual(stats["retained_inventories"], 0)

    def test_rejects_mutation_even_when_size_and_mtime_are_restored(self):
        stamp = self.layer.stat()
        with self.assertRaisesRegex(ValueError, "validation input changed"):
            with validation_inputs.validation_input_cache([self.root]):
                artifact_validation.sha(self.layer)
                self.layer.write_bytes(tar_entries([("usr/share/example", b"modified", 0o644)]))
                os.utime(self.layer, ns=(stamp.st_atime_ns, stamp.st_mtime_ns))
                artifact_validation.sha(self.layer)

    def test_rejects_replacement_and_mutation_after_last_read(self):
        for replacement in (False, True):
            with self.subTest(replacement=replacement), self.assertRaisesRegex(ValueError, "validation input changed"):
                with validation_inputs.validation_input_cache([self.root]):
                    artifact_validation.sha(self.layer)
                    if replacement:
                        new = self.root / "new.tar"
                        new.write_bytes(self.layer.read_bytes())
                        new.replace(self.layer)
                    else:
                        self.layer.write_bytes(self.layer.read_bytes().replace(b"original", b"modified"))
                    # Exit checks a path even when no subsequent cache hit occurs.

    def test_rejects_mutation_during_inventory(self):
        def read():
            self.layer.write_bytes(b"changed")
            return {"entry": {"kind": "file"}}

        with self.assertRaisesRegex(ValueError, "validation input changed"):
            with validation_inputs.validation_input_cache([self.root]):
                validation_inputs.cached_inventory(self.layer, "test", read)

    def test_final_hash_rejects_changed_bytes_even_with_unchanged_stat_identity(self):
        stamp = validation_inputs._stamp(self.layer)
        with patch.object(validation_inputs, "_stamp", return_value=stamp):
            with self.assertRaisesRegex(ValueError, "validation input changed"):
                with validation_inputs.validation_input_cache([self.layer]):
                    artifact_validation.sha(self.layer)
                    self.layer.write_bytes(self.layer.read_bytes().replace(b"original", b"modified"))

    def test_temporary_inspection_files_are_never_retained(self):
        temporary = self.root / "extracted-elf"
        temporary.write_bytes(b"inspection")
        with validation_inputs.validation_input_cache([self.layer]):
            artifact_validation.sha(temporary)
            temporary.unlink()

    def test_bounds_reuse_and_does_not_retain_failures(self):
        for limits in ({"max_bytes": 1}, {"max_files": 0}):
            with self.subTest(limits=limits), patch.object(
                    oci_inventory, "_layer_entries", wraps=oci_inventory._layer_entries) as read:
                with validation_inputs.validation_input_cache([self.root], **limits):
                    oci_inventory.apply_layer(self.layer, {})
                    oci_inventory.apply_layer(self.layer, {})
                self.assertEqual(read.call_count, 2)
        with validation_inputs.validation_input_cache([self.root]):
            for _ in range(2):
                with self.assertRaisesRegex(ValueError, "bad inventory"):
                    validation_inputs.cached_inventory(self.layer, "failure", lambda: self.fail_read())

    @staticmethod
    def fail_read():
        raise ValueError("bad inventory")

    def test_layer_order_whiteouts_and_overlay_checks_on_cached_reads(self):
        whiteout = self.root / "whiteout.tar"
        whiteout.write_bytes(tar_entries([
            ("usr/share/.wh..wh..opq", b"", 0),
            ("usr/share/new", b"new", 0o644),
        ]))
        for context in (nullcontext(), validation_inputs.validation_input_cache([self.root])):
            with context:
                for _ in range(2):
                    files = {}
                    oci_inventory.apply_layer(self.layer, files)
                    oci_inventory.apply_layer(whiteout, files)
                    self.assertNotIn("usr/share/example", files)
                    self.assertIn("usr/share/new", files)
                    with self.assertRaisesRegex(ValueError, "contains a whiteout"):
                        oci_inventory.apply_layer(whiteout, files, checked_overlay=True)
                    with self.assertRaisesRegex(ValueError, "crosses a non-directory"):
                        oci_inventory.apply_layer(self.layer, {"usr": {"kind": "symlink", "linkname": "opt"}},
                                                  checked_overlay=True)
                    oci_inventory.apply_layer(self.layer, files)
                    self.assertIn("usr/share/example", files)

    def test_inventory_policy_and_normalizer_keys_remain_separate(self):
        def skip(_):
            return None

        with validation_inputs.validation_input_cache([self.root]):
            files = {}
            oci_inventory.apply_layer(self.layer, files, normalize_member=skip)
            self.assertEqual(files, {})
            oci_inventory.apply_layer(self.layer, files)
            self.assertIn("usr/share/example", files)
            self.assertEqual(package_contract.inventory(self.layer), files)
            self.assertEqual(artifact_validation.payload(self.layer), files)
        duplicate = self.root / "duplicate.tar"
        duplicate.write_bytes(tar_entries([("file", b"one", 0o644), ("file", b"two", 0o644)]))
        with validation_inputs.validation_input_cache([self.root]):
            oci_inventory.apply_layer(duplicate, {})  # OCI applies the last entry.
            with self.assertRaisesRegex(ValueError, "duplicate source package"):
                package_contract.inventory(duplicate)
            with self.assertRaisesRegex(ValueError, "duplicate package"):
                artifact_validation.payload(duplicate)


if __name__ == "__main__":
    unittest.main()
