"""Exercise GitHub checkout provenance with real Git objects and shallow clones.

Contract: GitHub documents GITHUB_SHA as the tested pull-request merge commit.
Receipts identify its actual base and requested PR head, even when the event's
base snapshot is older. A different checkout or head must publish no identity.
"""

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

from tools.bazel.ci import sysmgr_image

ROOT = Path(__file__).resolve().parents[3]
SCRIPT = ROOT / "tools/bazel/ci/checkout_identity.py"


class CheckoutIdentityTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="checkout identity ")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.repo = self.root / "source"
        self.repo.mkdir()
        self.git("init", "-q")
        self.git("config", "user.name", "Provenance test")
        self.git("config", "user.email", "provenance@example.invalid")
        self.git("config", "commit.gpgsign", "false")
        self.tree = self.git("mktree", input="")
        self.old_base = self.commit("old base")
        self.head = self.commit("PR change", self.old_base)
        self.base = self.commit("base advanced", self.old_base)
        self.merge = self.commit("tested merge", self.base, self.head)
        self.git("update-ref", "refs/heads/main", self.merge)
        self.git("symbolic-ref", "HEAD", "refs/heads/main")

    def git(self, *args, input=None):
        return subprocess.check_output(["git", *args], cwd=self.repo, input=input,
                                       text=True, stderr=subprocess.PIPE).strip()

    def commit(self, message, *parents):
        args = ["commit-tree", self.tree]
        for parent in parents:
            args.extend(["-p", parent])
        return self.git(*args, input=message + "\n")

    def run_identity(self, *, repo=None, revision=None, head=None, base=None, event="pull_request"):
        output = self.root / "github-output"
        output.write_text("retained=value\n")
        result = subprocess.run([
            sys.executable, str(SCRIPT), "--expected-revision", revision or self.merge,
            "--event-name", event, "--head-sha", self.head if head is None else head,
            "--event-base-sha", self.old_base if base is None else base,
            "--github-output", str(output),
        ], cwd=repo or self.repo, text=True, capture_output=True)
        return result, output.read_text()

    def test_advanced_base_reaches_unchanged_strict_sysmgr_check(self):
        # This is the actual failing #51 condition, before normalizing the CI
        # boundary: the old event base is an ancestor, not a direct parent.
        with patch.object(sysmgr_image, "ROOT", self.repo), patch.dict(os.environ, {
            "CI_HEAD_SHA": self.head, "CI_BASE_SHA": self.old_base,
        }):
            with self.assertRaisesRegex(ValueError, "requested base"):
                sysmgr_image.source_identity()
        result, output = self.run_identity()
        self.assertEqual(result.returncode, 0, result.stderr)
        identity = json.loads(result.stdout)
        self.assertEqual(identity["revision"], self.merge)
        self.assertEqual(identity["head_sha"], self.head)
        self.assertEqual(identity["base_sha"], self.base)
        self.assertEqual(identity["event_base_sha"], self.old_base)
        self.assertEqual(identity["parents"], [self.base, self.head])
        self.assertTrue(output.startswith("retained=value\n"))
        values = dict(line.split("=", 1) for line in output.splitlines())
        with patch.object(sysmgr_image, "ROOT", self.repo), patch.dict(os.environ, {
            "CI_HEAD_SHA": values["head_sha"], "CI_BASE_SHA": values["base_sha"],
        }):
            receipt = sysmgr_image.source_identity()
        self.assertEqual(receipt, {"revision": self.merge,
                                   "tested_revision_parents": [self.base, self.head]})

    def test_matching_event_base_preserves_the_same_identity(self):
        result, output = self.run_identity(base=self.base)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)["base_sha"], self.base)
        self.assertIn("head_sha=" + self.head + "\n", output)

    def test_shallow_actions_checkout_uses_immutable_parent_headers(self):
        shallow = self.root / "shallow"
        subprocess.run(["git", "clone", "-q", "--depth=1", self.repo.as_uri(), str(shallow)],
                       check=True, capture_output=True)
        self.assertTrue((shallow / ".git/shallow").is_file())
        result, output = self.run_identity(repo=shallow)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)["parents"], [self.base, self.head])
        self.assertIn("base_sha=" + self.base + "\n", output)

    def test_wrong_revision_or_pr_head_publishes_no_identity(self):
        for change in ({"revision": self.old_base}, {"head": self.base}):
            with self.subTest(change=change):
                result, output = self.run_identity(**change)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, "")
                self.assertEqual(output, "retained=value\n")

    def test_pr_head_checkout_and_reversed_merge_parents_are_rejected(self):
        reversed_merge = self.commit("reversed parents", self.head, self.base)
        for revision in (self.head, reversed_merge):
            self.git("update-ref", "refs/heads/main", revision)
            result, output = self.run_identity(revision=revision)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(output, "retained=value\n")

    def test_incomplete_pr_identity_is_rejected(self):
        for change in ({"head": ""}, {"base": ""}):
            with self.subTest(change=change):
                result, output = self.run_identity(**change)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(output, "retained=value\n")

    def test_non_pr_events_keep_optional_pr_fields_empty(self):
        for event in ("push", "workflow_dispatch"):
            with self.subTest(event=event):
                result, output = self.run_identity(event=event, head="", base="")
                self.assertEqual(result.returncode, 0, result.stderr)
                value = json.loads(result.stdout)
                self.assertEqual(value["revision"], self.merge)
                self.assertEqual(value["head_sha"], "")
                self.assertEqual(value["base_sha"], "")
                self.assertIn("base_sha=\n", output)

    def test_non_pr_event_rejects_pr_metadata(self):
        result, output = self.run_identity(event="push")
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(output, "retained=value\n")


if __name__ == "__main__":
    unittest.main()
