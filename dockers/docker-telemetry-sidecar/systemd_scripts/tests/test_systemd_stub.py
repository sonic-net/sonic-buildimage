# tests/test_systemd_stub.py
import sys
import types
import importlib
from pathlib import Path

import pytest


@pytest.fixture(scope="session", autouse=True)
def fake_logger_module():
    pkg = types.ModuleType("sonic_py_common")
    logger_mod = types.ModuleType("sonic_py_common.logger")

    class _Logger:
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

    logger_mod.Logger = _Logger
    pkg.logger = logger_mod
    sys.modules["sonic_py_common"] = pkg
    sys.modules["sonic_py_common.logger"] = logger_mod
    yield


@pytest.fixture
def ss(tmp_path, monkeypatch):
    """
    Import systemd_stub fresh for every test, and provide fakes:
      - run_nsenter: simulates a host FS + systemctl/docker calls
      - container_fs: dict for "container" files
      - host_fs: dict for "host" files
    """
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    ss = importlib.import_module("systemd_stub")

    # Fake host filesystem and command recorder
    host_fs = {}
    commands = []

    # Fake run_nsenter
    def fake_run_nsenter(args, *, text=True, input_bytes=None):
        commands.append(("nsenter", tuple(args)))
        # /bin/cat <path>
        if args[:1] == ["/bin/cat"] and len(args) == 2:
            path = args[1]
            if path in host_fs:
                out = host_fs[path]
                return 0, (out if not text else out.decode("utf-8", "ignore")), b"" if not text else ""
            return 1, b"" if not text else "", b"No such file" if text else b"No such file"
        # /bin/sh -c "cat > /tmp/xxx"
        if args[:2] == ["/bin/sh", "-c"] and len(args) == 3 and args[2].startswith("cat > "):
            tmp_path = args[2].split("cat > ", 1)[1].strip()
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
        # sudo … (allow anything)
        if args[:1] == ["sudo"]:
            return 0, "" if text else b"", "" if text else b""
        return 1, "" if text else b"", "unsupported" if text else b"unsupported"

    monkeypatch.setattr(ss, "run_nsenter", fake_run_nsenter, raising=True)

    # Fake container FS
    container_fs = {}

    def fake_read_file_bytes_local(path: str):
        return container_fs.get(path, None)

    monkeypatch.setattr(ss, "read_file_bytes_local", fake_read_file_bytes_local, raising=True)

    # Isolate POST_COPY_ACTIONS
    monkeypatch.setattr(ss, "POST_COPY_ACTIONS", {}, raising=True)

    return ss, container_fs, host_fs, commands


def test_sha256_bytes_basic():
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    ss = importlib.import_module("systemd_stub")
    assert ss.sha256_bytes(b"") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    assert ss.sha256_bytes(None) == ""
    assert ss.sha256_bytes(b"abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"


def test_host_write_atomic_and_read(ss):
    ss, container_fs, host_fs, commands = ss
    ok = ss.host_write_atomic("/etc/testfile", b"hello", 0o755)
    assert ok
    data = ss.host_read_bytes("/etc/testfile")
    assert data == b"hello"
    cmd_names = [c[1][0] for c in commands]
    assert "/bin/sh" in cmd_names
    assert "/bin/chmod" in cmd_names
    assert "/bin/mkdir" in cmd_names
    assert "/bin/mv" in cmd_names


def test_sync_no_change_fast_path(ss):
    ss, container_fs, host_fs, commands = ss
    item = ss.SyncItem("/container/telemetry.sh", "/host/telemetry.sh", 0o755)
    container_fs[item.src_in_container] = b"same"
    host_fs[item.dst_on_host] = b"same"
    ss.SYNC_ITEMS[:] = [item]

    ok = ss.ensure_sync()
    assert ok is True
    assert not any("/bin/sh" == c[1][0] and "-c" in c[1] for c in commands)


def test_sync_updates_and_post_actions(ss):
    ss, container_fs, host_fs, commands = ss
    item = ss.SyncItem("/container/container_checker", "/bin/container_checker", 0o755)
    container_fs[item.src_in_container] = b"NEW"
    host_fs[item.dst_on_host] = b"OLD"
    ss.SYNC_ITEMS[:] = [item]

    ss.POST_COPY_ACTIONS[item.dst_on_host] = [
        ["sudo", "systemctl", "daemon-reload"],
        ["sudo", "systemctl", "restart", "monit"],
    ]

    ok = ss.ensure_sync()
    assert ok is True
    assert host_fs[item.dst_on_host] == b"NEW"

    post_cmds = [args for _, args in commands if args and args[0] == "sudo"]
    assert ("sudo", "systemctl", "daemon-reload") in post_cmds
    assert ("sudo", "systemctl", "restart", "monit") in post_cmds


def test_sync_missing_src_returns_false(ss):
    ss, container_fs, host_fs, commands = ss
    item = ss.SyncItem("/container/missing.sh", "/usr/local/bin/telemetry.sh", 0o755)
    ss.SYNC_ITEMS[:] = [item]
    ok = ss.ensure_sync()
    assert ok is False


def test_main_once_exits_zero_and_disables_post_actions(monkeypatch):
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    ss = importlib.import_module("systemd_stub")

    ss.POST_COPY_ACTIONS["/bin/container_checker"] = [["sudo", "echo", "hi"]]
    monkeypatch.setattr(ss, "ensure_sync", lambda: True, raising=True)
    monkeypatch.setattr(sys, "argv", ["systemd_stub.py", "--once", "--no-post-actions"])

    rc = ss.main()
    assert rc == 0
    assert ss.POST_COPY_ACTIONS == {}


def test_main_once_exits_nonzero_when_sync_fails(monkeypatch):
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    ss = importlib.import_module("systemd_stub")
    monkeypatch.setattr(ss, "ensure_sync", lambda: False, raising=True)
    monkeypatch.setattr(sys, "argv", ["systemd_stub.py", "--once"])
    rc = ss.main()
    assert rc == 1


def test_env_controls_telemetry_src_true(monkeypatch):
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    monkeypatch.setenv("IS_V1_ENABLED", "true")

    ss = importlib.import_module("systemd_stub")
    assert ss.IS_V1_ENABLED is True
    assert ss._TELEMETRY_SRC.endswith("telemetry_v1.sh")


def test_env_controls_telemetry_src_false(monkeypatch):
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    monkeypatch.setenv("IS_V1_ENABLED", "false")

    ss = importlib.import_module("systemd_stub")
    assert ss.IS_V1_ENABLED is False
    assert ss._TELEMETRY_SRC.endswith("telemetry.sh")


def test_env_controls_telemetry_src_default(monkeypatch):
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    monkeypatch.delenv("IS_V1_ENABLED", raising=False)

    ss = importlib.import_module("systemd_stub")
    assert ss.IS_V1_ENABLED is False
    assert ss._TELEMETRY_SRC.endswith("telemetry.sh")


def test_telemetry_service_syncs_to_host_when_different(ss):
    ss, container_fs, host_fs, commands = ss

    # Prepare container unit content and host old content
    container_fs[ss.CONTAINER_TELEMETRY_SERVICE] = b"UNIT-NEW"
    host_fs[ss.HOST_TELEMETRY_SERVICE] = b"UNIT-OLD"

    # Only include the telemetry service item to make the assertion clear
    ss.SYNC_ITEMS[:] = [
        ss.SyncItem(ss.CONTAINER_TELEMETRY_SERVICE, ss.HOST_TELEMETRY_SERVICE, 0o644)
    ]

    # Add post actions for telemetry.service
    ss.POST_COPY_ACTIONS[ss.HOST_TELEMETRY_SERVICE] = [
        ["sudo", "systemctl", "daemon-reload"],
        ["sudo", "systemctl", "restart", "telemetry"],
    ]

    ok = ss.ensure_sync()
    assert ok is True
    assert host_fs[ss.HOST_TELEMETRY_SERVICE] == b"UNIT-NEW"

    # Verify systemctl actions were invoked
    post_cmds = [args for _, args in commands if args and args[0] == "sudo"]
    assert ("sudo", "systemctl", "daemon-reload") in post_cmds
    assert ("sudo", "systemctl", "restart", "telemetry") in post_cmds
