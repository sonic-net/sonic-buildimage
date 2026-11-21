"""Exercise the GitHub Actions producer checks on the optional VS kernel index."""

import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import mock_open, patch

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import vs_kernel_cache_snapshot as snapshot


class KernelCacheSnapshotTest(unittest.TestCase):
    def setUp(self):
        self.repo = "example/sonic-buildimage"
        self.workflow = {"id": 10, "path": snapshot.WORKFLOW}
        self.run = {
            "workflow_id": 10, "path": snapshot.WORKFLOW, "event": "push",
            "head_branch": "master", "repository": {"full_name": self.repo},
            "head_repository": {"full_name": self.repo},
        }
        self.runs = []
        self.artifacts = []
        self.calls = []

    def api(self, path):
        self.calls.append(path)
        return {
            "workflows/bazel-oci.yml": self.workflow,
            "workflows/10/runs?event=push&branch=master&per_page=100": {"workflow_runs": self.runs},
            "artifacts?name=vs-kernel-cache-index-amd64-v1&per_page=100": {"artifacts": self.artifacts},
        }[path]

    def add(self, run_id, *, changes=None, **artifact_changes):
        self.runs.append(dict(self.run, id=run_id, run_number=run_id, **(changes or {})))
        self.artifacts.append({
            "id": 1000 + run_id, "name": snapshot.ARTIFACT, "expired": False,
            "created_at": "2026-11-21T12:00:00Z", "workflow_run": {"id": run_id},
            **artifact_changes,
        })

    def test_name_collisions_and_stale_artifacts_cannot_override_a_master_index(self):
        self.add(10, created_at="2026-11-20T12:00:00Z")
        self.add(11)
        self.add(12, changes={"event": "pull_request"})
        self.add(13, changes={"event": "workflow_dispatch"})
        self.add(14, changes={"workflow_id": 11})
        self.add(15, changes={"path": ".github/workflows/other.yml"})
        self.add(16, changes={"head_branch": "dev"})
        self.add(17, changes={"head_repository": {"full_name": "fork/sonic-buildimage"}})
        self.add(18, changes={"repository": {"full_name": "fork/sonic-buildimage"}})
        self.add(19, changes={"head_repository": None})
        self.add(20, name="archive")
        self.add(21, expired=True)
        self.add(22, workflow_run=None)
        self.add(23, workflow_run={"id": 999})
        self.add(24, id="1004")
        self.add(100)
        self.add(101)
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "outputs"
            with patch.object(snapshot, "github_api", self.api), patch.dict(os.environ, {
                "GITHUB_RUN_NUMBER": "100", "GITHUB_REPOSITORY": self.repo, "GITHUB_OUTPUT": str(output),
            }):
                snapshot.main()
            self.assertEqual(output.read_text(), "run_id=11\nartifact_id=1011\n")
        self.assertEqual(len(self.calls), 3)

    def test_unexpected_workflow_and_absent_or_expired_indexes_are_cache_misses(self):
        self.assertIsNone(snapshot.select(self.api, self.repo, 100))
        self.add(10, expired=True)
        self.assertIsNone(snapshot.select(self.api, self.repo, 100))
        self.workflow["path"] = ".github/workflows/old.yml"
        self.calls.clear()
        self.assertIsNone(snapshot.select(self.api, self.repo, 100))
        self.assertEqual(self.calls, ["workflows/bazel-oci.yml"])

    def test_transient_api_error_keeps_the_normal_make_path(self):
        with patch.object(snapshot, "github_api", side_effect=TimeoutError("request timed out")), \
                patch.dict(os.environ, {"GITHUB_RUN_NUMBER": "100", "GITHUB_REPOSITORY": self.repo}), \
                patch.object(Path, "open", mock_open()) as output:
            snapshot.main()
        output.assert_not_called()


if __name__ == "__main__":
    unittest.main()
