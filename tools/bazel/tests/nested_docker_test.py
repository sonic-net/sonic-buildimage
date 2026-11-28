"""Check the private cgroup boundary used by installed-image validation."""

import errno
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools.bazel.ci import prepare_nested_docker


class NestedDockerTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        for name, contents in {
            ".dockerenv": "",
            "proc/self/cgroup": "0::/\n",
            "sys/fs/cgroup/cgroup.type": "domain\n",
            "sys/fs/cgroup/cgroup.controllers": "cpu memory pids io\n",
            "sys/fs/cgroup/cgroup.procs": "1\n23\n",
        }.items():
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(contents)
        patch = mock.patch.object(
            prepare_nested_docker, "Path", side_effect=lambda name: self.root / name.lstrip("/"))
        patch.start()
        self.addCleanup(patch.stop)
        patch = mock.patch.object(prepare_nested_docker.os, "geteuid", return_value=0)
        patch.start()
        self.addCleanup(patch.stop)

    def test_processes_move_before_domain_controllers_are_enabled(self):
        calls = []
        original = Path.write_text

        def write(path, content, *args, **kwargs):
            calls.append((path.relative_to(self.root).as_posix(), content))
            return original(path, content, *args, **kwargs)

        with mock.patch.object(Path, "write_text", write):
            result = prepare_nested_docker.prepare()
        self.assertEqual(result["version"], 2)
        self.assertEqual(calls, [
            ("sys/fs/cgroup/sonic-docker-init/cgroup.procs", "1"),
            ("sys/fs/cgroup/sonic-docker-init/cgroup.procs", "23"),
            ("sys/fs/cgroup/cgroup.subtree_control", "+cpu +memory +pids +io"),
        ])

    def test_shared_or_non_domain_view_is_rejected_before_writes(self):
        for name, value in (
            ("proc/self/cgroup", "0::/system.slice/docker.service\n"),
            ("sys/fs/cgroup/cgroup.type", "domain invalid\n"),
        ):
            path = self.root / name
            previous = path.read_text()
            path.write_text(value)
            with self.subTest(name=name), self.assertRaisesRegex(ValueError, "fresh private domain"):
                prepare_nested_docker.prepare()
            self.assertFalse((self.root / "sys/fs/cgroup/sonic-docker-init").exists())
            path.write_text(previous)

    def test_missing_delegation_and_host_invocation_are_rejected(self):
        (self.root / "sys/fs/cgroup/cgroup.controllers").write_text("cpu pids\n")
        with self.assertRaisesRegex(ValueError, "delegated CPU, memory, or PID"):
            prepare_nested_docker.prepare()
        (self.root / ".dockerenv").unlink()
        with self.assertRaisesRegex(ValueError, "inside its slave"):
            prepare_nested_docker.prepare()

    def test_busy_controller_enable_retries(self):
        original = Path.write_text
        attempts = []

        def write(path, content, *args, **kwargs):
            if path.name == "cgroup.subtree_control":
                attempts.append(content)
                if len(attempts) == 1:
                    raise OSError(errno.EBUSY, "process raced with delegation")
            return original(path, content, *args, **kwargs)

        with mock.patch.object(Path, "write_text", write), mock.patch.object(prepare_nested_docker.time, "sleep") as sleep:
            prepare_nested_docker.prepare()
        self.assertEqual(len(attempts), 2)
        sleep.assert_called_once_with(0.1)

    def test_controller_permission_error_is_not_ignored(self):
        original = Path.write_text

        def write(path, content, *args, **kwargs):
            if path.name == "cgroup.subtree_control":
                raise PermissionError(errno.EPERM, "delegation denied")
            return original(path, content, *args, **kwargs)

        with mock.patch.object(Path, "write_text", write), mock.patch.object(prepare_nested_docker.time, "sleep") as sleep:
            with self.assertRaises(PermissionError):
                prepare_nested_docker.prepare()
        sleep.assert_not_called()

    def test_v1_does_not_modify_cgroups(self):
        (self.root / "sys/fs/cgroup/cgroup.controllers").unlink()
        self.assertEqual(prepare_nested_docker.prepare(), {"version": 1})
        self.assertFalse((self.root / "sys/fs/cgroup/sonic-docker-init").exists())

    def test_unprivileged_container_is_rejected_before_cgroup_writes(self):
        with mock.patch.object(prepare_nested_docker.os, "geteuid", return_value=1000):
            with self.assertRaisesRegex(ValueError, "as root inside its slave"):
                prepare_nested_docker.prepare()
        self.assertFalse((self.root / "sys/fs/cgroup/sonic-docker-init").exists())
        self.assertFalse((self.root / "sys/fs/cgroup/cgroup.subtree_control").exists())


if __name__ == "__main__":
    unittest.main()
