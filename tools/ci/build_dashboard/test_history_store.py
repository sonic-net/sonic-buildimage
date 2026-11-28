import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch


SPEC = importlib.util.spec_from_file_location("history_store", Path(__file__).with_name("history_store.py"))
store = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(store)


class HistoryStoreTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.index = Path(directory.name, "data.json")
        self.data = {"schema_version": 1, "repository": "example/build", "generated_at": "2026-11-20T12:00:00Z",
                     "pull_requests": [{"number": 270}], "comparisons": [],
                     "runs": [{"run_id": number, "attempt": 1} for number in range(8, 0, -1)]}

    def test_split_round_trip_and_reuse_older_shards(self):
        self.index.write_text(json.dumps(self.data, indent=2))
        self.assertEqual(store.read_snapshot(self.index), self.data)
        with patch.object(store, "SHARD_TARGET_BYTES", 85):
            store.write_snapshot(self.index, self.data)
            self.assertEqual(store.read_snapshot(self.index), self.data)
            index = json.loads(self.index.read_bytes())
            self.assertEqual(index["schema_version"], 2)
            self.assertNotIn("runs", index)
            parts = index["run_shards"]
            self.assertGreater(len(parts), 1)
            for part in parts:
                content = (self.index.parent / part["path"]).read_bytes()
                self.assertLessEqual(len(content), 85)
                self.assertEqual(part["path"], f"history/{hashlib.sha256(content).hexdigest()}.json")
                self.assertEqual(part["count"], len(json.loads(content)))
            updated = dict(self.data, runs=[{"run_id": 9, "attempt": 1}, *self.data["runs"]])
            store.write_snapshot(self.index, updated)
            self.assertEqual(store.read_snapshot(self.index), updated)
            self.assertEqual(json.loads(self.index.read_bytes())["run_shards"][-1], parts[-1])

    def test_zero_runs_and_one_run_larger_than_target(self):
        empty = dict(self.data, runs=[])
        store.write_snapshot(self.index, empty)
        self.assertEqual(json.loads(self.index.read_bytes())["run_shards"], [])
        self.assertEqual(store.read_snapshot(self.index), empty)
        runs = [{"run_id": 3}, {"run_id": 2, "name": "X" * 100}, {"run_id": 1}]
        with patch.object(store, "SHARD_TARGET_BYTES", 64):
            store.write_snapshot(self.index, dict(self.data, runs=runs))
        parts = json.loads(self.index.read_bytes())["run_shards"]
        self.assertEqual([part["count"] for part in parts], [1, 1, 1])
        self.assertEqual(store.read_snapshot(self.index)["runs"], runs)

    def test_rejected_write_retains_readable_prior_snapshot(self):
        store.write_snapshot(self.index, self.data)
        huge = {"description": "X" * 1000}
        with patch.object(store, "MAX_FILE_BYTES", 400), patch.object(store, "SHARD_TARGET_BYTES", 40):
            with self.assertRaisesRegex(ValueError, "single dashboard run"):
                store.write_snapshot(self.index, dict(self.data, runs=[huge, *self.data["runs"]]))
            with self.assertRaisesRegex(ValueError, "index exceeds"):
                store.write_snapshot(self.index, dict(self.data, pull_requests=[huge]))
        self.assertEqual(store.read_snapshot(self.index), self.data)

    def test_reject_incomplete_corrupt_or_unexpected_shards(self):
        store.write_snapshot(self.index, self.data)
        original = self.index.read_bytes()
        part = json.loads(original)["run_shards"][0]
        for descriptors in ([part, part], [dict(part, count=1)], [dict(part, count=True)],
                            [dict(part, path="../data.json")], [dict(part, path="https://example.com/runs.json")],
                            [dict(part, unexpected="field")]):
            with self.subTest(descriptors=descriptors):
                self.index.write_text(json.dumps(dict(json.loads(original), run_shards=descriptors)))
                with self.assertRaises(ValueError):
                    store.read_snapshot(self.index)
        self.index.write_bytes(original)
        shard = self.index.parent / part["path"]
        shard.write_bytes(b"[]\n")
        with self.assertRaisesRegex(ValueError, "checksum mismatch"):
            store.read_snapshot(self.index)
        shard.unlink()
        with self.assertRaises(FileNotFoundError):
            store.read_snapshot(self.index)
        shard.symlink_to(self.index)
        with self.assertRaisesRegex(ValueError, "Invalid dashboard shard file"):
            store.read_snapshot(self.index)


if __name__ == "__main__":
    unittest.main()
