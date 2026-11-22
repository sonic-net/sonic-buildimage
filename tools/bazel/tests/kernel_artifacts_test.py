#!/usr/bin/env python3
"""Reject unsafe or mismatched dependency records before kernel publication."""

import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import unittest
from unittest.mock import Mock

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import kernel_artifacts
from tools.bazel.kernel import prepare as kernel
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

    def test_direct_root_publication_archives_without_private_consumer_records(self):
        self.fixture.make_fixture()
        summary = kernel_artifacts.collect(self.fixture.root, None, self.output,
            architecture=self.fixture.architecture, debs_directory=self.fixture.bundle)
        self.assertEqual(summary["artifact_count"], 5)
        self.assertEqual({path.name for path in self.output.iterdir()},
                         set(self.fixture.packages) | {"summary.json"})
        for item in summary["artifacts"]:
            self.assertEqual(hashlib.sha256((self.output / item["name"]).read_bytes()).hexdigest(), item["sha256"])

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

    def test_vs_handoff_survives_cached_make_reuse_and_reproduces_public_summary(self):
        # The VS checkout can advance while still consuming an unchanged kernel.
        baseline = self.collect()
        kernel.retain_resolution(self.state, self.fixture.bundle, self.fixture.provenance["resolution"])
        shutil.rmtree(self.state)
        (self.fixture.root / "unrelated").touch()
        subprocess.run(["git", "-C", str(self.fixture.root), "add", "unrelated"], check=True)
        self.fixture.commit(self.fixture.root, "Unrelated change to VS")
        self.assertTrue(kernel.ensure(self.fixture.root)["reused"])
        handoff = self.fixture.root / "vs"
        kernel_artifacts.collect(self.fixture.root, None, handoff, vs_handoff=True)
        self.assertEqual({p.name for p in handoff.iterdir()}, {
            *self.fixture.packages, kernel.MANIFEST, kernel.PROVENANCE, kernel_artifacts.VS_RECEIPT,
        })
        receipt = json.loads((handoff / kernel_artifacts.VS_RECEIPT).read_text())
        self.assertEqual(receipt["vs_revision"], kernel.git(self.fixture.root, "rev-parse", "HEAD"))
        self.assertNotEqual(receipt["vs_revision"], baseline["source_commit"])
        output = self.fixture.root / "public-from-vs"
        summary = kernel_artifacts.collect(self.fixture.root, None, output, debs_directory=handoff, from_vs_handoff=True)
        self.assertEqual(summary, baseline)
        self.assertEqual(summary["architecture"], self.fixture.architecture)
        self.assertEqual({p.name: p.read_bytes() for p in output.iterdir()},
                         {p.name: p.read_bytes() for p in self.output.iterdir()})

    def test_vs_handoff_refuses_missing_or_corrupt_resolution_evidence(self):
        for reason in ("missing", "corrupt"):
            with self.subTest(reason=reason):
                kernel.retain_resolution(self.state, self.fixture.bundle, self.fixture.provenance["resolution"])
                record = self.fixture.bundle / kernel.RESOLUTION_DIRECTORY / self.fixture.provenance["resolution"]["lock_sha256"]
                if reason == "missing":
                    record.unlink()
                    error = "expected nonempty regular kernel output"
                else:
                    record.write_bytes(b"wrong evidence")
                    error = "kernel dependency record differs"
                with self.assertRaisesRegex(ValueError, error):
                    kernel_artifacts.collect(self.fixture.root, None, self.output, vs_handoff=True)
                self.assertFalse(self.output.exists())

    def test_fresh_remote_restore_can_publish_without_bazel_build_state(self):
        """An authenticated master restore supplies the original provenance and all five VS packages."""
        self.fixture.remote_cache_fixture()
        shutil.rmtree(self.state)
        result = kernel.ensure(self.fixture.root, remote_cache="grpc://fixture:9092",
                               builder=Mock(side_effect=AssertionError("unexpected kernel build")))
        self.assertEqual(result["reuse_source"], "remote")
        self.assertFalse((self.fixture.bundle / kernel.RESOLUTION_DIRECTORY).exists())
        handoff = self.fixture.root / "vs"
        original = kernel_artifacts.collect(self.fixture.root, None, handoff, vs_handoff=True)
        summary = kernel_artifacts.collect(self.fixture.root, None, self.output, debs_directory=handoff,
                                           from_vs_handoff=True)
        self.assertEqual(summary, original)
        self.assertEqual({row["name"] for row in summary["artifacts"]}, set(self.fixture.packages))

        provenance = self.fixture.bundle / kernel.PROVENANCE
        original_bytes = provenance.read_bytes()
        provenance.write_bytes(original_bytes + b"\n")
        with self.assertRaisesRegex(ValueError, "verified master index"):
            kernel_artifacts.collect(self.fixture.root, None, self.fixture.root / "changed", vs_handoff=True)
        provenance.write_bytes(original_bytes)

        trusted = self.fixture.root / kernel.remote_bundle.INPUT_INDEX
        index = json.loads(trusted.read_text())
        index["metadata"][kernel.PROVENANCE]["sha256"] = "1" * 64
        trusted.write_text(json.dumps(index))
        with self.assertRaisesRegex(ValueError, "trusted master index"):
            kernel_artifacts.collect(self.fixture.root, None, self.fixture.root / "untrusted", vs_handoff=True)

    def test_vs_handoff_rejects_unreviewed_provenance_fields(self):
        self.fixture.provenance["private_path"] = "/home/runner/private"
        self.fixture.write_manifest()
        kernel.retain_resolution(self.state, self.fixture.bundle, self.fixture.provenance["resolution"])
        with self.assertRaisesRegex(ValueError, "unexpected kernel metadata"):
            kernel_artifacts.collect(self.fixture.root, None, self.output, vs_handoff=True)
        self.assertFalse(self.output.exists())

    def test_vs_consumer_rejects_changed_receipt_or_package(self):
        kernel.retain_resolution(self.state, self.fixture.bundle, self.fixture.provenance["resolution"])
        handoff = self.fixture.root / "vs"
        kernel_artifacts.collect(self.fixture.root, None, handoff, vs_handoff=True)
        path = handoff / kernel_artifacts.VS_RECEIPT
        original = path.read_bytes()
        for change in ("vs_revision", "resolution", "package"):
            with self.subTest(change=change):
                receipt = json.loads(original)
                if change == "vs_revision":
                    receipt["vs_revision"] = "1" * 40
                elif change == "resolution":
                    receipt["summary"]["resolution_sha256"]["MODULE.bazel.lock"] = "1" * 64
                else:
                    (handoff / next(iter(self.fixture.packages))).write_bytes(b"invalid DEB")
                path.write_text(json.dumps(receipt))
                error = "kernel artifact failed" if change == "package" else "kernel handoff differs"
                with self.assertRaisesRegex(ValueError, error):
                    kernel_artifacts.collect(self.fixture.root, None, self.output, debs_directory=handoff, from_vs_handoff=True)
                self.assertFalse(self.output.exists())

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
