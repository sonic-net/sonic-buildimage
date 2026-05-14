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
# ───────────── CONFIG_DB helper tests ─────────────

class FakeConfigDB:
    """Minimal in-memory fake of ConfigDBConnector for unit testing."""

    def __init__(self, tables=None):
        # tables: {table_name: {entry_key: {field: value}}}
        self.tables = tables or {}
        self.set_calls = []  # list of (table, key, value) tuples

    def get_entry(self, table, key):
        return dict(self.tables.get(table, {}).get(key, {}))

    def get_table(self, table):
        return {k: dict(v) for k, v in self.tables.get(table, {}).items()}

    def set_entry(self, table, key, value):
        self.set_calls.append((table, key, value))
        if value is None:
            self.tables.get(table, {}).pop(key, None)
            return
        self.tables.setdefault(table, {})[key] = dict(value)


@pytest.fixture
def fake_db(fake_logger, monkeypatch):
    """Install a FakeConfigDB as the singleton used by sidecar_common."""
    db = FakeConfigDB()
    monkeypatch.setattr(sidecar_common, "_get_config_db", lambda: db)
    return db


def test_db_hdel_removes_field_and_keeps_entry(fake_db):
    fake_db.tables["TELEMETRY"] = {"gnmi": {"user_auth": "cert", "port": "50051"}}

    assert sidecar_common.db_hdel("TELEMETRY|gnmi", "user_auth") is True

    # set_entry called with remaining fields (not None — entry still has 'port')
    assert len(fake_db.set_calls) == 1
    table, key, value = fake_db.set_calls[0]
    assert (table, key) == ("TELEMETRY", "gnmi")
    assert value == {"port": "50051"}
    assert "user_auth" not in fake_db.tables["TELEMETRY"]["gnmi"]


def test_db_hdel_field_absent_is_noop_returns_true(fake_db):
    fake_db.tables["TELEMETRY"] = {"gnmi": {"port": "50051"}}

    assert sidecar_common.db_hdel("TELEMETRY|gnmi", "user_auth") is True
    # No mutation should have been written
    assert fake_db.set_calls == []


def test_db_hdel_last_field_writes_none_for_true_deletion(fake_db):
    """Removing the only field must call set_entry(table, key, None) so the
    entry is deleted, not left as an empty dict (regression coverage)."""
    fake_db.tables["TELEMETRY"] = {"gnmi": {"user_auth": "cert"}}

    assert sidecar_common.db_hdel("TELEMETRY|gnmi", "user_auth") is True

    assert len(fake_db.set_calls) == 1
    _, _, value = fake_db.set_calls[0]
    assert value is None


def test_db_hdel_handles_get_entry_failure(fake_db, monkeypatch):
    def raising_get(*_a, **_kw):
        raise RuntimeError("boom")

    monkeypatch.setattr(fake_db, "get_entry", raising_get)
    assert sidecar_common.db_hdel("TELEMETRY|gnmi", "user_auth") is False
    assert fake_db.set_calls == []


def test_db_hdel_returns_false_when_db_unavailable(fake_logger, monkeypatch):
    monkeypatch.setattr(sidecar_common, "_get_config_db", lambda: None)
    assert sidecar_common.db_hdel("TELEMETRY|gnmi", "user_auth") is False


def test_db_del_passes_none_not_empty_dict(fake_db):
    """db_del must pass None to set_entry — passing {} would not delete the
    entry (regression coverage for the fix in this commit)."""
    fake_db.tables["GNMI_CLIENT_CERT"] = {"server.example.com": {"role@": "admin"}}

    assert sidecar_common.db_del("GNMI_CLIENT_CERT|server.example.com") is True

    assert len(fake_db.set_calls) == 1
    table, key, value = fake_db.set_calls[0]
    assert (table, key) == ("GNMI_CLIENT_CERT", "server.example.com")
    assert value is None
    assert "server.example.com" not in fake_db.tables["GNMI_CLIENT_CERT"]


def test_db_del_returns_false_when_db_unavailable(fake_logger, monkeypatch):
    monkeypatch.setattr(sidecar_common, "_get_config_db", lambda: None)
    assert sidecar_common.db_del("GNMI_CLIENT_CERT|x") is False


def test_db_get_table_keys_returns_all_keys(fake_db):
    fake_db.tables["GNMI_CLIENT_CERT"] = {
        "client.a.example.com": {"role@": "admin"},
        "server.b.example.com": {"role@": "gnmi_show_readonly,admin"},
    }

    keys = sidecar_common.db_get_table_keys("GNMI_CLIENT_CERT")

    assert sorted(keys) == ["client.a.example.com", "server.b.example.com"]


def test_db_get_table_keys_empty_table(fake_db):
    assert sidecar_common.db_get_table_keys("GNMI_CLIENT_CERT") == []


def test_db_get_table_keys_returns_empty_when_db_unavailable(fake_logger, monkeypatch):
    monkeypatch.setattr(sidecar_common, "_get_config_db", lambda: None)
    assert sidecar_common.db_get_table_keys("GNMI_CLIENT_CERT") == []


def test_db_get_table_keys_handles_exception(fake_db, monkeypatch):
    def raising_get_table(*_a, **_kw):
        raise RuntimeError("boom")

    monkeypatch.setattr(fake_db, "get_table", raising_get_table)
    assert sidecar_common.db_get_table_keys("GNMI_CLIENT_CERT") == []


def test_db_hset_persists_leaf_list_value(fake_db):
    """db_hset must accept a List[str] value (YANG leaf-list) unchanged."""
    fake_db.tables["GNMI_CLIENT_CERT"] = {"server.example.com": {}}

    ok = sidecar_common.db_hset(
        "GNMI_CLIENT_CERT|server.example.com",
        "role",
        ["gnmi_show_readonly", "admin"],
    )
    assert ok is True

    assert len(fake_db.set_calls) == 1
    _, _, value = fake_db.set_calls[0]
    assert value["role"] == ["gnmi_show_readonly", "admin"]
