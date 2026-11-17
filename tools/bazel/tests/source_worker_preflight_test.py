#!/usr/bin/env python3
"""Reject unsupported worker limits and untrusted image markers."""

import importlib.util
import json
from pathlib import Path
import stat
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location("source_worker_preflight", ROOT / "tools/bazel/ci/source_worker_preflight.py")
worker = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(worker)


class WorkerPreflightTest(unittest.TestCase):
    def test_real_cgroup_cpu_format(self):
        self.assertEqual(worker.cpu_limit("600000 100000\n"), 6)
        self.assertEqual(worker.cpu_limit("250000 100000\n"), 2.5)
        for value in ("max 100000", "0 100000", "600000 0", "invalid", "-1 100000"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                worker.cpu_limit(value)

    def test_marker_must_be_immutable_and_identify_the_selected_slave(self):
        value = {"uid": 1000, "gid": 1000, "runner_root": "/opt/actions-runner", "work_root": "/actions/_work",
                 "base_image": "sha256:" + "a" * 64}
        path = mock.Mock()
        path.read_text.return_value = json.dumps(value)
        path.lstat.return_value = SimpleNamespace(st_mode=stat.S_IFREG | 0o444, st_uid=0)
        self.assertEqual(worker.marker_identity(path), {"base_image": value["base_image"]})
        for mode, uid in ((stat.S_IFREG | 0o644, 0), (stat.S_IFLNK | 0o444, 0), (stat.S_IFREG | 0o444, 1000)):
            path.lstat.return_value = SimpleNamespace(st_mode=mode, st_uid=uid)
            with self.subTest(mode=mode, uid=uid), self.assertRaises(ValueError):
                worker.marker_identity(path)
        path.lstat.return_value = SimpleNamespace(st_mode=stat.S_IFREG | 0o444, st_uid=0)
        value["base_image"] = "latest"
        path.read_text.return_value = json.dumps(value)
        with self.assertRaises(ValueError):
            worker.marker_identity(path)

    def test_measured_image_memory_is_required_by_real_preflight(self):
        # The old 10 GiB workers passed isolation, but the cold Sysmgr image
        # consumed 14.69 GiB. Exercise the actual cgroup gate at its boundary.
        for gib, expected in ((10, False), (16, False), (20, True), (24, True)):
            values = {
                "/sys/fs/cgroup/memory.max": str(gib * 1024**3),
                "/sys/fs/cgroup/memory.current": "1024",
                "/sys/fs/cgroup/cpu.max": "400000 100000",
                "/proc/self/cgroup": "0::/",
                "/sys/fs/cgroup/cgroup.type": "domain",
                "/sys/fs/cgroup/cgroup.controllers": "cpu memory pids",
            }
            with self.subTest(gib=gib), \
                    mock.patch.object(worker.Path, "read_text", autospec=True,
                                      side_effect=lambda path: values[str(path)]), \
                    mock.patch.object(worker, "marker_identity", return_value={"base_image": "sha256:" + "a" * 64}), \
                    mock.patch.object(worker.subprocess, "run", return_value=SimpleNamespace(returncode=0)), \
                    mock.patch.object(worker.subprocess, "check_output", return_value="ext4"), \
                    mock.patch.object(worker.shutil, "disk_usage", return_value=SimpleNamespace(free=100 * 1024**3)):
                receipt = worker.measure(worker.WORKERS[0])
            self.assertEqual(receipt["checks"]["bounded_build_resources"], expected)
            self.assertEqual(receipt["facts"]["memory_limit_bytes"], gib * 1024**3)
            if expected:
                self.assertEqual(receipt["build_profile"]["name"], "two_actions")
                self.assertEqual(receipt["build_profile"]["measured_memory_limit_bytes"], gib * 1024**3)
                self.assertEqual(receipt["build_profile"]["measured_cpu_limit"], 4)
                self.assertEqual(receipt["build_profile"]["minimum_memory_bytes"], 20 * 1024**3)
            else:
                self.assertIsNone(receipt["build_profile"])

    def test_cpu_boundary_selects_two_actions_only_on_four_cpu_workers(self):
        for cpus, jobs, memory in ((2, 1, 4000), (3.999, 1, 4000), (4, 2, 8000), (6, 2, 8000)):
            with self.subTest(cpus=cpus):
                profile = worker.select_build_profile(20 * 1024**3, cpus)
                self.assertEqual((profile["jobs"], profile["cpu"], profile["scheduler_memory_mb"]), (jobs, jobs, memory))
                self.assertEqual(profile["measured_cpu_limit"], cpus)
                self.assertEqual(profile["jvm_heap_gib"], 3)
        for memory, cpus in ((20 * 1024**3 - 1, 4), (20 * 1024**3, 1.999),
                             (20 * 1024**3, 6.001), (20 * 1024**3, float("nan"))):
            with self.subTest(memory=memory, cpus=cpus), self.assertRaises(ValueError):
                worker.select_build_profile(memory, cpus)

    def test_failed_preflight_retains_public_receipt(self):
        receipt = {"schema": 1, "status": "failed", "checks": {"worker_affinity": False}, "facts": {"worker": "unexpected"}}
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory) / "receipt.json"
            with mock.patch.object(worker, "measure", return_value=receipt), \
                    mock.patch.object(worker.subprocess, "check_output", return_value="a" * 40), \
                    mock.patch("sys.argv", ["preflight", "--worker", worker.WORKERS[0], "--output", str(target)]), \
                    mock.patch("builtins.print"):
                self.assertEqual(worker.main(), 1)
            self.assertEqual(json.loads(target.read_text())["status"], "failed")


if __name__ == "__main__":
    unittest.main()
