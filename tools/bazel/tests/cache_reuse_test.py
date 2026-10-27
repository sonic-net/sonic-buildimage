"""Check cache evidence independently of private command and environment data."""

import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci import cache_reuse, public_artifacts


class CacheReuseTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.log = self.root / "execution.json"
        self.events = self.root / "events.jsonl"
        self.target = "//dockers/docker-orchagent:ci_artifacts"
        self.records = [
            {"targetLabel": "@@sonic_swss+//orchagent:orchagent", "mnemonic": "CppCompile",
             "runner": "disk cache hit", "cacheHit": True},
            {"targetLabel": "@@sonic_swss+//orchagent:orchagent", "mnemonic": "CppCompile",
             "runner": "disk cache hit", "cacheHit": True},
            {"targetLabel": "//dockers/docker-orchagent:ci_swss", "mnemonic": "CopyFile",
             "runner": "processwrapper-sandbox", "cacheHit": False},
            {"mnemonic": "WorkspaceStatus", "runner": "local"},
        ]
        self.build_events = [
            {"id": {"started": {}}, "started": {"command": "build", "buildToolVersion": "8.5.1"}},
            {"id": {"targetCompleted": {"label": self.target}}, "completed": {"success": True}},
            {"id": {"buildFinished": {}}, "finished": {"overallSuccess": True, "exitCode": {"code": 0}}},
            {"id": {"buildMetrics": {}}, "lastMessage": True},
        ]
        self.write()

    def write(self):
        self.log.write_text("\n".join(json.dumps(record, indent=2) for record in self.records) + "\n")
        self.events.write_text("".join(json.dumps(event) + "\n" for event in self.build_events))

    def collect(self):
        return cache_reuse.collect(self.log, self.events, self.target)

    def test_counts_cover_cached_and_executed_production_actions(self):
        """Aggregate repeats without confusing a copy or bookkeeping spawn with compilation."""
        result = self.collect()
        self.assertEqual(result["phase"], "before_tests")
        self.assertEqual(result["artifact_target"], self.target)
        self.assertEqual((result["action_count"], result["cache_hit_count"], result["executed_count"]), (4, 2, 2))
        self.assertEqual(len(result["actions"]), 3)
        self.assertEqual(result["actions"][0]["target"], None)
        cached = next(action for action in result["actions"] if action["cache_hit"])
        self.assertEqual(cached["count"], 2)
        # The producer and publication allowlist must agree on the exact schema.
        self.assertEqual(public_artifacts.cache_reuse_summary(result), result)

    def test_private_fields_and_chunk_boundaries_do_not_affect_public_counts(self):
        """Large multiline records cross stream chunks without leaking their private fields."""
        self.records[0].update(commandArgs=["private-command-" + "x" * 150000],
                               environmentVariables=[{"name": "PRIVATE", "value": "private-environment"}],
                               inputs=[{"path": "/home/private/input"}],
                               actualOutputs=[{"path": "/home/private/output"}])
        self.write()
        result = self.collect()
        self.assertEqual(result["action_count"], 4)
        self.assertNotIn("private", json.dumps(result))

    def test_large_input_lists_do_not_redecode_every_chunk(self):
        """Bound decoder work for the large input lists in real SpawnExec records."""
        record = {**self.records[0], "inputs": [
            {"path": "external/source/include/header.h", "digest": {"hash": "a" * 64}}
            for _ in range(10000)
        ]}
        self.log.write_text(json.dumps(record))
        attempts = []
        decoder_type = json.JSONDecoder

        class MeasuredDecoder(decoder_type):
            def raw_decode(self, text, idx=0):
                attempts.append(len(text) - idx)
                return super().raw_decode(text, idx)

        with mock.patch.object(cache_reuse.json, "JSONDecoder", MeasuredDecoder):
            self.assertEqual(list(cache_reuse.json_records(self.log)), [record])
        # A fixed 64 KiB retry parses the prefix quadratically. Geometric
        # retries visit less than four times the final record's text.
        self.assertLess(sum(attempts), 4 * self.log.stat().st_size)

    def test_mixed_records_keep_order_without_copying_the_remaining_buffer(self):
        """A large read may contain many small records and part of the next large one."""
        large = {**self.records[0], "inputs": [{"path": "x" * 1000}] * 1200}
        records = [large, *self.records * 500, large, self.records[-1]]
        self.log.write_text("\n \t".join(json.dumps(record) for record in records) + "\n")
        offsets = []
        decoder_type = json.JSONDecoder

        class MeasuredDecoder(decoder_type):
            def raw_decode(self, text, idx=0):
                offsets.append(idx)
                return super().raw_decode(text, idx)

        with mock.patch.object(cache_reuse.json, "JSONDecoder", MeasuredDecoder):
            self.assertEqual(list(cache_reuse.json_records(self.log)), records)
        self.assertGreater(sum(offset > 0 for offset in offsets), 1000)

    def test_size_limit_applies_to_each_record_and_includes_the_exact_limit(self):
        """Reject an oversized record while allowing a larger stream of valid records."""
        limit = 8192
        overhead = len(json.dumps({"inputs": ""}))
        exact = json.dumps({"inputs": "x" * (limit - overhead)})
        with mock.patch.object(cache_reuse, "_CHUNK_SIZE", 1024), \
             mock.patch.object(cache_reuse, "_MAX_RECORD_SIZE", limit):
            self.log.write_text(" \n" + exact + "\n{}\n" + exact + "\n")
            self.assertEqual(list(cache_reuse.json_records(self.log)),
                             [json.loads(exact), {}, json.loads(exact)])
            self.log.write_text(json.dumps({"inputs": "x" * (limit + 1 - overhead)}))
            with self.assertRaisesRegex(ValueError, "record exceeds 64 MiB"):
                list(cache_reuse.json_records(self.log))

    def test_empty_missing_malformed_or_truncated_logs_fail(self):
        """No records and incomplete JSON cannot be interpreted as full cache reuse."""
        for text in (None, "", " \n", "{", '{"runner":', "[]", '{"mnemonic":"CppCompile"}',
                     '{"mnemonic":"CppCompile","mnemonic":"CppLink"}',
                     json.dumps(self.records[0]) + '\n{"runner":'):
            with self.subTest(text=text):
                if text is None:
                    self.log.unlink(missing_ok=True)
                else:
                    self.log.write_text(text)
                with self.assertRaises(ValueError):
                    self.collect()

    def test_invalid_execution_fields_fail_without_echoing_private_values(self):
        """Reject unsafe labels, false cache claims and failed spawns."""
        changes = (
            {"targetLabel": "/home/private/input"},
            {"targetLabel": False},
            {"mnemonic": "private/command"}, {"runner": "private\ncommand"},
            {"cacheHit": "true"}, {"cacheHit": False},
            {"exitCode": 1}, {"exitCode": False}, {"status": "private-error-detail"},
        )
        original = self.records[0]
        for fields in changes:
            with self.subTest(fields=fields):
                self.records = [{**original, **fields}]
                self.write()
                with self.assertRaises(ValueError) as raised:
                    self.collect()
                self.assertNotIn("private", str(raised.exception))

    def test_incomplete_failed_wrong_target_or_test_bep_is_rejected(self):
        """A spawn log needs the completed successful production build that produced it."""
        original = json.loads(json.dumps(self.build_events))
        for mode in ("no-final-marker", "failed", "wrong-target", "test-command", "missing-finish"):
            with self.subTest(mode=mode):
                self.build_events = json.loads(json.dumps(original))
                if mode == "no-final-marker": self.build_events[-1].pop("lastMessage")
                elif mode == "failed": self.build_events[2]["finished"] = {"overallSuccess": False, "exitCode": {"code": 1}}
                elif mode == "wrong-target": self.build_events[1]["id"]["targetCompleted"]["label"] = "//other:ci_artifacts"
                elif mode == "test-command": self.build_events[0]["started"]["command"] = "test"
                else: del self.build_events[2]
                self.write()
                with self.assertRaises(ValueError):
                    self.collect()


if __name__ == "__main__":
    unittest.main()
