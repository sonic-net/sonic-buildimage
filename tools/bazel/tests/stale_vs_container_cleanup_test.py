"""Verify stale-container reclamation without touching a Docker daemon."""

import copy
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock
import urllib.error

import yaml

from tools.bazel.ci import cleanup_stale_vs_containers as subject


ROOT = Path(__file__).resolve().parents[3]


class StaleVsContainerCleanupTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.workspace = Path(temporary.name).resolve()
        self.cache = self.workspace / "vs-cache"
        self.cache.mkdir()
        self.container = "a" * 64
        self.record = {
            "Id": self.container,
            "State": {"Status": "exited", "Running": False},
            "Config": {"Labels": {subject.LABEL: "securely1g/sonic-buildimage:123:1:vs"}},
            "Mounts": [{"Type": "bind", "Source": str(self.cache / "bazel")}],
        }

    def cleanup(self):
        subject.cleanup(self.workspace, "securely1g/sonic-buildimage", "456", "1")

    def test_verified_stopped_container_is_rechecked_and_removed_without_force(self):
        with mock.patch.object(subject.subprocess, "check_output",
                               return_value=self.container + "\n"), \
                mock.patch.object(subject, "inspect", return_value=self.record) as inspect, \
                mock.patch.object(subject, "completed", return_value=True) as completed, \
                mock.patch.object(subject.subprocess, "run") as remove:
            self.cleanup()
        self.assertEqual(inspect.call_args_list, [mock.call(self.container)] * 2)
        completed.assert_called_once_with("securely1g/sonic-buildimage", "123", "1")
        remove.assert_called_once_with(["docker", "rm", self.container], check=True, timeout=30)
        self.assertTrue(self.cache.is_dir())

    def test_live_current_foreign_unlabelled_and_unrelated_containers_are_preserved(self):
        changes = [
            {"State": {"Status": "running", "Running": True}},
            {"State": {"Status": "paused", "Running": False}},
            {"State": {"Status": "created", "Running": False}},
            {"Config": {"Labels": {subject.LABEL: "securely1g/sonic-buildimage:456:1:vs"}}},
            {"Config": {"Labels": {subject.LABEL: "other/repo:123:1:vs"}}},
            {"Config": {"Labels": {}}},
            {"Config": {"Labels": {subject.LABEL: "securely1g/sonic-buildimage:123:1:other"}}},
            {"Mounts": [{"Type": "bind", "Source": str(self.workspace / "other")}]},
            {"Mounts": [{"Type": "volume", "Source": str(self.cache / "bazel")}]},
        ]
        for change in changes:
            record = dict(self.record, **change)
            with self.subTest(change=change), \
                    mock.patch.object(subject.subprocess, "check_output", return_value=self.container), \
                    mock.patch.object(subject, "inspect", return_value=record), \
                    mock.patch.object(subject, "completed") as completed, \
                    mock.patch.object(subject.subprocess, "run") as remove:
                self.cleanup()
                completed.assert_not_called()
                remove.assert_not_called()

    def test_in_progress_or_unverifiable_attempt_cannot_authorize_removal(self):
        for result in (False, urllib.error.HTTPError("https://api.github.com", 403, "denied", {}, None)):
            with self.subTest(result=result), \
                    mock.patch.object(subject.subprocess, "check_output", return_value=self.container), \
                    mock.patch.object(subject, "inspect", return_value=self.record), \
                    mock.patch.object(subject, "completed", **(
                        {"side_effect": result} if isinstance(result, Exception) else {"return_value": result})), \
                    mock.patch.object(subject.subprocess, "run") as remove:
                if isinstance(result, Exception):
                    with self.assertRaises(urllib.error.HTTPError):
                        self.cleanup()
                else:
                    self.cleanup()
                remove.assert_not_called()

    def test_restart_between_inspection_and_removal_fails_closed(self):
        running = copy.deepcopy(self.record)
        running["State"] = {"Status": "running", "Running": True}
        with mock.patch.object(subject.subprocess, "check_output", return_value=self.container), \
                mock.patch.object(subject, "inspect", side_effect=[self.record, running]), \
                mock.patch.object(subject, "completed", return_value=True), \
                mock.patch.object(subject.subprocess, "run") as remove:
            with self.assertRaisesRegex(ValueError, "Container changed"):
                self.cleanup()
            remove.assert_not_called()

    def test_docker_removal_error_is_not_ignored(self):
        with mock.patch.object(subject.subprocess, "check_output", return_value=self.container), \
                mock.patch.object(subject, "inspect", return_value=self.record), \
                mock.patch.object(subject, "completed", return_value=True), \
                mock.patch.object(subject.subprocess, "run",
                                  side_effect=subprocess.CalledProcessError(1, "docker")):
            with self.assertRaises(subprocess.CalledProcessError):
                self.cleanup()

    def test_redirected_cache_is_rejected_before_docker(self):
        self.cache.rmdir()
        self.cache.symlink_to(self.workspace, target_is_directory=True)
        with mock.patch.object(subject.subprocess, "check_output") as docker:
            with self.assertRaisesRegex(ValueError, "redirected"):
                self.cleanup()
            docker.assert_not_called()

    def test_attempt_api_requires_matching_identity_and_completed_status(self):
        for change, expected in (({}, True), ({"id": 999}, False),
                                 ({"run_attempt": 2}, False), ({"status": "in_progress"}, False)):
            record = dict(id=123, run_attempt=1, status="completed")
            record.update(change)
            response = mock.MagicMock()
            response.__enter__.return_value = response
            response.read.return_value = json.dumps(record).encode()
            with self.subTest(change=change), \
                    mock.patch.dict(os.environ, GITHUB_TOKEN="test-token"), \
                    mock.patch.object(subject.urllib.request, "build_opener") as opener:
                opener.return_value.open.return_value = response
                self.assertEqual(subject.completed("securely1g/sonic-buildimage", "123", "1"), expected)
                request = opener.return_value.open.call_args.args[0]
                self.assertEqual(request.full_url,
                                 "https://api.github.com/repos/securely1g/sonic-buildimage/actions/runs/123/attempts/1")
                self.assertIsNone(opener.call_args.args[0].redirect_request(
                    request, None, 302, "", {}, "https://other.example"))

    def test_workflow_verifies_stale_owners_before_cache_and_image_cleanup(self):
        workflow = yaml.safe_load((ROOT / ".github/workflows/bazel-oci.yml").read_text())
        job = workflow["jobs"]["vs_container"]
        self.assertEqual(job["permissions"], {"contents": "read", "actions": "read"})
        steps = job["steps"]
        index = next(i for i, step in enumerate(steps)
                     if "cleanup_stale_vs_containers" in step.get("run", ""))
        self.assertEqual(steps[index]["env"]["GITHUB_TOKEN"], "${{ github.token }}")
        for name in ("Reclaim unused images and build cache", "Reclaim local build caches"):
            self.assertLess(index, next(i for i, step in enumerate(steps) if step.get("name") == name))


if __name__ == "__main__":
    unittest.main()
