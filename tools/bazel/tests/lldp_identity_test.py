#!/usr/bin/env python3
"""Exercise source provenance against a real shallow Git merge checkout."""

import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from tools.bazel.ci import lldp_image


class IdentityTest(unittest.TestCase):
    def test_shallow_merge_preserves_requested_parents_and_clean_source(self):
        with tempfile.TemporaryDirectory(prefix="lldp-identity-") as directory:
            root = Path(directory)
            source, clone = root / "source", root / "clone"
            source.mkdir()

            def git(workspace, *arguments):
                return subprocess.check_output(
                    ["git", "-c", "user.name=SONiC validation", "-c",
                     "user.email=securely1g@gmail.com", *arguments],
                    cwd=workspace, text=True, stderr=subprocess.PIPE).strip()

            git(source, "init", "-q", "--initial-branch=main")
            (source / "base").write_text("base\n")
            git(source, "add", "base")
            git(source, "commit", "-qm", "Base")
            git(source, "checkout", "-qb", "feature")
            (source / "feature").write_text("feature\n")
            git(source, "add", "feature")
            git(source, "commit", "-qm", "Feature")
            head = git(source, "rev-parse", "HEAD")
            git(source, "checkout", "-q", "main")
            (source / "base").write_text("updated base\n")
            git(source, "commit", "-qam", "Update base")
            base = git(source, "rev-parse", "HEAD")
            git(source, "merge", "--no-ff", "-qm", "Test merge", "feature")
            git(root, "clone", "-q", "--depth=1", source.as_uri(), str(clone))
            self.assertEqual(git(clone, "show", "-s", "--format=%P", "HEAD"), "")
            evidence = clone / "artifacts" / "lldp"
            evidence.mkdir(parents=True)
            (evidence / "receipt.json").write_text("{}\n")
            with patch.object(lldp_image, "ROOT", clone), patch.dict(
                    os.environ, {"CI_HEAD_SHA": head, "CI_BASE_SHA": base, "GITHUB_SHA": git(clone, "rev-parse", "HEAD")}):
                identity = lldp_image.source_identity()
                self.assertEqual(set(identity["tested_revision_parents"]), {base, head})
                lldp_image.clean_source(evidence)
                with patch.dict(os.environ, {"CI_HEAD_SHA": "0" * 40}):
                    with self.assertRaisesRegex(Exception, "requested PR head"):
                        lldp_image.source_identity()
                with patch.dict(os.environ, {"CI_BASE_SHA": "1" * 40}):
                    with self.assertRaisesRegex(Exception, "requested PR base"):
                        lldp_image.source_identity()
                (clone / "untracked.c").write_text("untracked source\n")
                with self.assertRaisesRegex(Exception, "untracked source"):
                    lldp_image.clean_source(evidence)
                (clone / "untracked.c").unlink()
                (clone / "feature").write_text("changed source\n")
                with self.assertRaises(subprocess.CalledProcessError):
                    lldp_image.clean_source(evidence)
                git(clone, "checkout", "--", "feature")
                with self.assertRaisesRegex(Exception, "cannot contain the source"):
                    lldp_image.clean_source(clone)


if __name__ == "__main__":
    unittest.main()
