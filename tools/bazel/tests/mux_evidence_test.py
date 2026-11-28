#!/usr/bin/env python3
"""Reject stale MUX receipts and incomplete runtime/debug handoffs."""

import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock

from tools.bazel.ci import mux_image, mux_loaded


class MUXEvidenceTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="mux-evidence-")
        self.addCleanup(self.temporary.cleanup)
        root = Path(self.temporary.name)
        self.checkout = root / "source"
        self.checkout.mkdir()
        self.directory = root / "artifacts"
        self.directory.mkdir()
        self.git("init", "--initial-branch=main")
        self.git("config", "user.name", "MUX evidence test")
        self.git("config", "user.email", "test@example.invalid")
        self.git("config", "commit.gpgsign", "false")
        (self.checkout / "original").write_text("original\n")
        self.git("add", ".")
        self.git("commit", "-m", "initial")
        self.git("checkout", "-b", "feature")
        (self.checkout / "feature").write_text("feature\n")
        self.git("add", ".")
        self.git("commit", "-m", "feature")
        self.head = self.git("rev-parse", "HEAD")
        self.git("checkout", "main")
        (self.checkout / "base").write_text("base\n")
        self.git("add", ".")
        self.git("commit", "-m", "base")
        self.base = self.git("rev-parse", "HEAD")
        self.git("merge", "--no-ff", "feature", "-m", "tested merge")
        self.revision = self.git("rev-parse", "HEAD")
        self.addCleanup(mock.patch.stopall)
        mock.patch.object(mux_image, "ROOT", self.checkout).start()
        mock.patch.dict(os.environ, {"GITHUB_SHA": self.revision,
                                    "CI_HEAD_SHA": self.head, "CI_BASE_SHA": self.base}).start()
        artifacts = {}
        for name in mux_image.IMAGES:
            archive = self.directory / (name + ".gz")
            archive.write_bytes(name.encode())
            artifacts[archive.name] = {"sha256": hashlib.sha256(archive.read_bytes()).hexdigest()}
        self.report = {"status": "passed", "architecture": "amd64", "revision": self.revision,
                       "tested_revision_parents": [self.base, self.head],
                       "images": {name: {} for name in mux_image.IMAGES}, "artifacts": artifacts}
        self.publish()

    def git(self, *args):
        return subprocess.check_output(["git", *args], cwd=self.checkout,
                                       stderr=subprocess.DEVNULL, text=True).strip()

    def publish(self):
        (self.directory / "validation.json").write_text(json.dumps(self.report))

    def test_matching_merge_and_complete_archives_pass(self):
        report, source = mux_loaded.verify_inputs(self.directory)
        self.assertEqual(source["tested_revision_parents"], [self.base, self.head])
        self.assertEqual(set(report["images"]), set(mux_image.IMAGES))

    def test_requested_head_cannot_match_merge_base(self):
        with mock.patch.dict(os.environ, {"CI_HEAD_SHA": self.base}):
            with self.assertRaisesRegex(ValueError, "requested PR head"):
                mux_loaded.verify_inputs(self.directory)

    def test_requested_base_cannot_match_feature_head(self):
        with mock.patch.dict(os.environ, {"CI_BASE_SHA": self.head}):
            with self.assertRaisesRegex(ValueError, "requested PR base"):
                mux_loaded.verify_inputs(self.directory)

    def test_wrong_github_revision_fails(self):
        with mock.patch.dict(os.environ, {"GITHUB_SHA": self.head}):
            with self.assertRaisesRegex(ValueError, "requested GitHub revision"):
                mux_loaded.verify_inputs(self.directory)

    def test_wrong_receipt_source_or_parents_fail(self):
        for field, value in (("revision", self.head), ("tested_revision_parents", [self.head, self.base])):
            with self.subTest(field=field):
                altered = dict(self.report, **{field: value})
                (self.directory / "validation.json").write_text(json.dumps(altered))
                with self.assertRaisesRegex(ValueError, "differs from built source"):
                    mux_loaded.verify_inputs(self.directory)

    def test_missing_or_extra_variants_fail(self):
        for names in ([], [mux_image.IMAGES[0]], [*mux_image.IMAGES, "unexpected"]):
            with self.subTest(names=names):
                self.report["images"] = {name: {} for name in names}
                self.publish()
                with self.assertRaisesRegex(ValueError, "both runtime and debug"):
                    mux_loaded.verify_inputs(self.directory)

    def test_each_archive_digest_is_checked(self):
        for name in mux_image.IMAGES:
            with self.subTest(image=name):
                archive = self.directory / (name + ".gz")
                original = archive.read_bytes()
                archive.write_bytes(b"different build")
                with self.assertRaisesRegex(ValueError, "archive changed"):
                    mux_loaded.verify_inputs(self.directory)
                archive.write_bytes(original)

    def test_untracked_source_is_rejected(self):
        (self.checkout / "new_source.py").write_text("uncommitted")
        with self.assertRaisesRegex(ValueError, "untracked source inputs"):
            mux_loaded.verify_inputs(self.directory)


if __name__ == "__main__":
    unittest.main()
