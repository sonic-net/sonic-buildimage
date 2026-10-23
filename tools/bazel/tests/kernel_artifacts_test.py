#!/usr/bin/env python3
"""Reject unsafe or mismatched dependency records before kernel publication."""

import hashlib
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import kernel_artifacts
from tools.bazel.tests import kernel_test


class KernelArtifactsTest(unittest.TestCase):
    def setUp(self):
        self.fixture = kernel_test.KernelBundleTest()
        self.fixture.setUp()
        self.addCleanup(self.fixture.doCleanups)
        self.state = self.fixture.root / "state"
        self.state.mkdir()
        self.output = self.fixture.root / "public"
        self.records = {
            "consumer/MODULE.bazel.lock": {"lockFileVersion": 25, "registryFileHashes": {}, "moduleExtensions": {}},
            "build/module-graph.json": {"key": "<root>", "name": "sonic-kernel-cache-consumer", "version": "", "root": True, "dependencies": []},
            "build/kernel-source.json": {"url": "https://github.com/securely1g/sonic-linux-kernel/archive/" + "b" * 40 + ".tar.gz"},
            "build/infra-source.json": {"url": "https://github.com/securely1g/sonic-build-infra/archive/" + "a" * 40 + ".tar.gz"},
        }
        self.write_records()

    def write_records(self):
        fields = ("lock_sha256", "graph_sha256", "kernel_source_json_sha256", "infra_source_json_sha256")
        for (name, value), field in zip(self.records.items(), fields):
            path = self.state / name
            path.parent.mkdir(parents=True, exist_ok=True)
            data = (json.dumps(value) + "\n").encode()
            path.write_bytes(data)
            self.fixture.provenance["resolution"][field] = hashlib.sha256(data).hexdigest()
        self.fixture.write_manifest()

    def collect(self):
        return kernel_artifacts.collect(self.fixture.root, self.state, self.output,
                                        architecture=self.fixture.state["launcher"]["architecture"],
                                        debs_directory=self.fixture.bundle)

    def test_retains_only_verified_artifacts_and_fixed_summary(self):
        (self.state / "launcher.log").write_text("private diagnostic sentinel")
        self.collect()
        self.assertEqual({p.name for p in self.output.iterdir()},
                         {*self.fixture.packages, "summary.json"})
        summary = json.loads((self.output / "summary.json").read_text())
        self.assertEqual(summary["artifact_count"], 5)
        for item in summary["artifacts"]:
            self.assertEqual(hashlib.sha256((self.output / item["name"]).read_bytes()).hexdigest(), item["sha256"])

    def test_omits_recorded_environment_and_arbitrary_dependency_fields(self):
        self.records["consumer/MODULE.bazel.lock"]["moduleExtensions"] = {"fixture": {"envVariables": {"PRIVATE": "sentinel"}}}
        self.records["consumer/MODULE.bazel.lock"]["facts"] = {"note": "private internal sentinel"}
        self.records["build/module-graph.json"]["dependencies"] = [{"private_path": "/tmp/private-evidence"}]
        self.write_records()
        self.collect()
        data = (self.output / "summary.json").read_text()
        self.assertNotIn("sentinel", data)
        self.assertNotIn("private-evidence", data)
        self.assertEqual(len(list(self.output.iterdir())), 6)

    def test_rejects_changed_lock(self):
        (self.state / "consumer/MODULE.bazel.lock").write_text("{}\n")
        with self.assertRaises(ValueError):
            self.collect()
        self.assertFalse(self.output.exists())

    def test_omits_unselected_provenance_fields(self):
        self.fixture.provenance["private_path"] = "/home/runner/private"
        self.fixture.write_manifest()
        self.collect()
        self.assertNotIn("/home/runner/private", (self.output / "summary.json").read_text())

    def test_omits_unrelated_files_in_shared_deb_directory(self):
        (self.fixture.bundle / "event.json").write_text("{}")
        (self.fixture.bundle / "unrelated.deb").write_bytes(b"unrelated package")
        self.collect()
        self.assertEqual({p.name for p in self.output.iterdir()},
                         {*self.fixture.packages, "summary.json"})
        self.assertEqual((self.fixture.bundle / "unrelated.deb").read_bytes(), b"unrelated package")


if __name__ == "__main__":
    unittest.main()
