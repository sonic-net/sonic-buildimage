"""
Tests for sonic_py_common.sidecar_common module.

This module contains shared utilities for SONiC sidecar containers.
"""
import sys
import os
import types
import pytest

# Add sonic-py-common to path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from sonic_py_common import sidecar_common


@pytest.fixture
def fake_logger():
    """Provide fake logger to avoid dependency on SONiC logger."""
    logger_mod = types.ModuleType("sonic_py_common.logger")

    class FakeLogger:
        def __init__(self):
            self.messages = []

        def _log(self, level, msg):
            self.messages.append((level, msg))

        def log_debug(self, msg):     self._log("DEBUG", msg)
        def log_info(self, msg):      self._log("INFO", msg)
        def log_error(self, msg):     self._log("ERROR", msg)
        def log_notice(self, msg):    self._log("NOTICE", msg)
        def log_warning(self, msg):   self._log("WARNING", msg)
        def log_critical(self, msg):  self._log("CRITICAL", msg)

    logger_mod.Logger = FakeLogger
    sys.modules["sonic_py_common.logger"] = logger_mod

    yield


@pytest.fixture
def mock_nsenter(monkeypatch):
    """Mock run_nsenter for file operations testing."""
    host_fs = {}
    commands = []

    def fake_run_nsenter(args, *, text=True, input_bytes=None):
        commands.append(("nsenter", tuple(args)))

        # /bin/cat <path>
        if args[:1] == ["/bin/cat"] and len(args) == 2:
            path = args[1]
            if path in host_fs:
                out = host_fs[path]
                if text:
                    return 0, out.decode("utf-8", "ignore"), ""
                return 0, out, b""
            return 1, "" if text else b"", "No such file" if text else b"No such file"

        # /bin/sh -c "cat > /tmp/xxx"
        if (
            len(args) == 3
            and args[0] == "/bin/sh"
            and args[1] in ("-c", "-lc")
            and args[2].strip().startswith("cat > ")
        ):
            tmp_path = args[2].split("cat >", 1)[1].strip()
            if tmp_path and tmp_path[0] == tmp_path[-1] and tmp_path[0] in ("'", '"'):
                tmp_path = tmp_path[1:-1]
            host_fs[tmp_path] = input_bytes or (b"" if text else b"")
            return 0, "" if text else b"", "" if text else b""

        # chmod / mkdir / mv / rm
        if args[:1] == ["/bin/chmod"]:
            return 0, "" if text else b"", "" if text else b""
        if args[:1] == ["/bin/mkdir"]:
            return 0, "" if text else b"", "" if text else b""
        if args[:1] == ["/bin/mv"] and len(args) == 4:
            src, dst = args[2], args[3]
            host_fs[dst] = host_fs.get(src, b"")
            host_fs.pop(src, None)
            return 0, "" if text else b"", "" if text else b""
        if args[:1] == ["/bin/rm"]:
            target = args[-1]
            host_fs.pop(target, None)
            return 0, "" if text else b"", "" if text else b""

        return 1, "" if text else b"", "unsupported" if text else b"unsupported"

    monkeypatch.setattr(sidecar_common, "run_nsenter", fake_run_nsenter)

    return host_fs, commands


def test_sha256_bytes_basic():
    """Test SHA256 hash calculation."""
    assert sidecar_common.sha256_bytes(b"") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    assert sidecar_common.sha256_bytes(None) == ""
    assert sidecar_common.sha256_bytes(b"abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"


def test_get_bool_env_var(monkeypatch):
    """Test boolean environment variable parsing."""
    # Test default value when not set
    monkeypatch.delenv("TEST_VAR", raising=False)
    assert sidecar_common.get_bool_env_var("TEST_VAR", default=False) is False
    assert sidecar_common.get_bool_env_var("TEST_VAR", default=True) is True

    # Test true values
    for val in ["1", "true", "True", "TRUE", "yes", "YES", "y", "Y", "on", "ON"]:
        monkeypatch.setenv("TEST_VAR", val)
        assert sidecar_common.get_bool_env_var("TEST_VAR") is True

    # Test false values
    for val in ["0", "false", "False", "no", "off", "other"]:
        monkeypatch.setenv("TEST_VAR", val)
        assert sidecar_common.get_bool_env_var("TEST_VAR", default=True) is False


def test_host_write_atomic_and_read(fake_logger, mock_nsenter):
    """Test atomic file write and read on host."""
    host_fs, commands = mock_nsenter

    ok = sidecar_common.host_write_atomic("/etc/testfile", b"hello", 0o755)
    assert ok
    data = sidecar_common.host_read_bytes("/etc/testfile")
    assert data == b"hello"
    cmd_names = [c[1][0] for c in commands]
    assert "/bin/sh" in cmd_names
    assert "/bin/chmod" in cmd_names
    assert "/bin/mkdir" in cmd_names
    assert "/bin/mv" in cmd_names


def test_sync_items_success(fake_logger, mock_nsenter, monkeypatch):
    """Test successful file synchronization."""
    host_fs, commands = mock_nsenter
    container_fs = {}

    def fake_read_file_bytes_local(path):
        return container_fs.get(path, None)

    monkeypatch.setattr(sidecar_common, "read_file_bytes_local", fake_read_file_bytes_local)

    item = sidecar_common.SyncItem("/container/test.sh", "/host/test.sh", 0o755)
    container_fs[item.src_in_container] = b"#!/bin/bash\necho test"

    ok = sidecar_common.sync_items([item], {})
    assert ok
    assert host_fs["/host/test.sh"] == b"#!/bin/bash\necho test"


def test_sync_items_missing_source(fake_logger, monkeypatch):
    """Test sync_items returns False when source file is missing."""
    container_fs = {}

    def fake_read_file_bytes_local(path):
        return container_fs.get(path, None)

    monkeypatch.setattr(sidecar_common, "read_file_bytes_local", fake_read_file_bytes_local)

    item = sidecar_common.SyncItem("/container/missing.sh", "/host/test.sh", 0o755)

    ok = sidecar_common.sync_items([item], {})
    assert ok is False


def test_sync_items_with_post_actions(fake_logger, mock_nsenter, monkeypatch):
    """Test sync_items executes post-copy actions."""
    host_fs, commands = mock_nsenter
    container_fs = {}

    def fake_read_file_bytes_local(path):
        return container_fs.get(path, None)

    monkeypatch.setattr(sidecar_common, "read_file_bytes_local", fake_read_file_bytes_local)

    item = sidecar_common.SyncItem("/container/script.sh", "/host/script.sh", 0o755)
    container_fs[item.src_in_container] = b"#!/bin/bash\necho hello"

    post_actions = {
        "/host/script.sh": [
            ["sudo", "systemctl", "daemon-reload"],
            ["sudo", "systemctl", "restart", "myservice"],
        ]
    }

    ok = sidecar_common.sync_items([item], post_actions)
    assert ok

    # Verify post-actions were called
    sudo_cmds = [args for _, args in commands if args and args[0] == "sudo"]
    assert ("sudo", "systemctl", "daemon-reload") in sudo_cmds
    assert ("sudo", "systemctl", "restart", "myservice") in sudo_cmds


# ─────────────────────────── Tests for cleanup_native_container ───────────────────────────

@pytest.fixture
def docker_nsenter(monkeypatch):
    """Mock run_nsenter with configurable docker inspect response."""
    commands = []
    inspect_result = {"rc": 1, "out": "", "err": ""}

    def fake_run_nsenter(args, *, text=True, input_bytes=None):
        commands.append(("nsenter", tuple(args)))
        if args[:2] == ["sudo", "docker"] and "inspect" in args:
            return inspect_result["rc"], inspect_result["out"], inspect_result["err"]
        if args[:1] == ["sudo"]:
            return 0, "" if text else b"", "" if text else b""
        return 1, "" if text else b"", "unsupported" if text else b"unsupported"

    monkeypatch.setattr(sidecar_common, "run_nsenter", fake_run_nsenter)
    return commands, inspect_result


def test_cleanup_native_container_removes_running(fake_logger, docker_nsenter):
    """When a native container is running, it is stopped and force-removed."""
    commands, inspect_result = docker_nsenter
    inspect_result.update(rc=0, out="running\n", err="")

    sidecar_common.cleanup_native_container("mycontainer", False)

    sudo_cmds = [args for _, args in commands if args and args[0] == "sudo"]
    assert ("sudo", "docker", "inspect", "--format", "{{.State.Status}}", "mycontainer") in sudo_cmds
    assert ("sudo", "docker", "stop", "mycontainer") in sudo_cmds
    assert ("sudo", "docker", "rm", "--force", "mycontainer") in sudo_cmds


def test_cleanup_native_container_removes_exited(fake_logger, docker_nsenter):
    """When a native container exists but is exited, it is still removed."""
    commands, inspect_result = docker_nsenter
    inspect_result.update(rc=0, out="exited\n", err="")

    sidecar_common.cleanup_native_container("mycontainer", False)

    sudo_cmds = [args for _, args in commands if args and args[0] == "sudo"]
    assert ("sudo", "docker", "stop", "mycontainer") in sudo_cmds
    assert ("sudo", "docker", "rm", "--force", "mycontainer") in sudo_cmds


def test_cleanup_native_container_noop_when_absent(fake_logger, docker_nsenter):
    """When no native container exists, cleanup is a no-op."""
    commands, inspect_result = docker_nsenter
    inspect_result.update(rc=1, out="", err="No such object: mycontainer")

    sidecar_common.cleanup_native_container("mycontainer", False)

    sudo_cmds = [args for _, args in commands if args and args[0] == "sudo"]
    assert ("sudo", "docker", "stop", "mycontainer") not in sudo_cmds
    assert ("sudo", "docker", "rm", "--force", "mycontainer") not in sudo_cmds


def test_cleanup_native_container_skipped_when_v1(fake_logger, docker_nsenter):
    """When is_v1_enabled=True, native container cleanup is skipped entirely."""
    commands, _ = docker_nsenter

    sidecar_common.cleanup_native_container("mycontainer", True)

    assert len(commands) == 0


def test_cleanup_native_container_uses_container_name(fake_logger, docker_nsenter):
    """Container name parameter is correctly passed to all docker commands."""
    commands, inspect_result = docker_nsenter
    inspect_result.update(rc=0, out="running\n", err="")

    sidecar_common.cleanup_native_container("restapi", False)

    sudo_cmds = [args for _, args in commands if args and args[0] == "sudo"]
    assert ("sudo", "docker", "stop", "restapi") in sudo_cmds
    assert ("sudo", "docker", "rm", "--force", "restapi") in sudo_cmds

    commands.clear()
    inspect_result.update(rc=0, out="running\n", err="")

    sidecar_common.cleanup_native_container("acms", False)

    sudo_cmds = [args for _, args in commands if args and args[0] == "sudo"]
    assert ("sudo", "docker", "stop", "acms") in sudo_cmds
    assert ("sudo", "docker", "rm", "--force", "acms") in sudo_cmds
