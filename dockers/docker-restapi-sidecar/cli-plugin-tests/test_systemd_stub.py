# tests/test_systemd_stub.py
import sys
import os
import types
import importlib

import pytest

# Add docker-restapi-sidecar directory to path so we can import systemd_stub
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

# Add sonic-py-common to path so we can import the real sidecar_common
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "../../../../../src/sonic-py-common")))


# ===== Create fakes BEFORE importing sidecar_common =====
def _setup_fakes():
    """Create fake modules before any imports that need them."""
    # ----- fake swsscommon.swsscommon.ConfigDBConnector -----
    swss_pkg = types.ModuleType("swsscommon")
    swss_common_mod = types.ModuleType("swsscommon.swsscommon")

    class _DummyConfigDBConnector:
        def __init__(self, *_, **__):
            pass

        def connect(self, *_, **__):
            pass

        def get_entry(self, *_, **__):
            return {}

        def set_entry(self, *_, **__):
            pass

    swss_common_mod.ConfigDBConnector = _DummyConfigDBConnector
    swss_pkg.swsscommon = swss_common_mod
    sys.modules["swsscommon"] = swss_pkg
    sys.modules["swsscommon.swsscommon"] = swss_common_mod
    
    # ----- fake sonic_py_common.logger ONLY (let real sonic_py_common load) -----
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
    sys.modules["sonic_py_common.logger"] = logger_mod

# Create fakes before any imports
_setup_fakes()

# Now safe to import sidecar_common (it will use the fake swsscommon)
from sonic_py_common import sidecar_common as real_sidecar_common


@pytest.fixture(scope="session", autouse=True)
def fake_logger_module():
    """
    Fakes were already set up at module level by _setup_fakes().
    This fixture just ensures they stay registered during test execution.
    """
    yield


@pytest.fixture
def ss(tmp_path, monkeypatch):
    """
    Import systemd_stub fresh for every test, and provide fakes:

      - run_nsenter: simulates host FS + systemctl/docker calls (patched on sidecar_common)
      - container_fs: dict for "container" files
      - host_fs: dict for "host" files
      - config_db: dict for CONFIG_DB contents ("TABLE|KEY" -> {field: value})
      - ConfigDBConnector: replaced with a fake that reads/writes config_db (patched on sidecar_common)
    """
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]

    # Fake host filesystem and command recorder
    host_fs = {}
    commands = []

    # Fake CONFIG_DB (redis key "TABLE|KEY" -> dict(field -> value))
    config_db = {}

    # ----- Patch db_hget, db_hgetall, db_hset, db_del on sidecar_common -----
    def fake_db_hget(key: str, field: str):
        """Get a single field from a CONFIG_DB hash."""
        entry = config_db.get(key, {})
        return entry.get(field)

    def fake_db_hgetall(key: str):
        """Get all fields from a CONFIG_DB hash."""
        return dict(config_db.get(key, {}))

    def fake_db_hset(key: str, field: str, value):
        """Set a field in a CONFIG_DB hash."""
        if key not in config_db:
            config_db[key] = {}
        config_db[key][field] = value

    def fake_db_del(key: str):
        """Delete a CONFIG_DB key entirely."""
        if key in config_db:
            del config_db[key]
            return True
        return False

    monkeypatch.setattr(real_sidecar_common, "db_hget", fake_db_hget)
    monkeypatch.setattr(real_sidecar_common, "db_hgetall", fake_db_hgetall)
    monkeypatch.setattr(real_sidecar_common, "db_hset", fake_db_hset)
    monkeypatch.setattr(real_sidecar_common, "db_del", fake_db_del)

    # ----- Fake run_nsenter for host operations (patch on sidecar_common) -----
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
            and args[1] in ("-c", "-lc")  # accept both forms
            and args[2].strip().startswith("cat > ")
        ):
            tmp_path = args[2].split("cat >", 1)[1].strip()
            # strip quotes if shlex.quote added them
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

        # sudo … (post actions)
        if args[:1] == ["sudo"]:
            return 0, "" if text else b"", "" if text else b""

        return 1, "" if text else b"", "unsupported" if text else b"unsupported"

    monkeypatch.setattr(real_sidecar_common, "run_nsenter", fake_run_nsenter)

    # Fake container FS - patch read_file_bytes_local on sidecar_common
    container_fs = {}

    def fake_read_file_bytes_local(path: str):
        return container_fs.get(path, None)

    monkeypatch.setattr(real_sidecar_common, "read_file_bytes_local", fake_read_file_bytes_local)

    # Now import systemd_stub (it will use patched sidecar_common)
    ss = importlib.import_module("systemd_stub")

    # Isolate POST_COPY_ACTIONS
    monkeypatch.setattr(ss, "POST_COPY_ACTIONS", {}, raising=True)

    return ss, container_fs, host_fs, commands, config_db


def test_sync_no_change_fast_path(ss):
    ss, container_fs, host_fs, commands, config_db = ss
    item = ss.SyncItem("/container/restapi.sh", "/host/restapi.sh", 0o755)
    container_fs[item.src_in_container] = b"same"
    host_fs[item.dst_on_host] = b"same"
    ss.SYNC_ITEMS[:] = [item]

    ok = ss.ensure_sync()
    assert ok is True
    # No write path used (no /bin/sh -c cat > tmp)
    assert not any(
        c[1][0] == "/bin/sh" and ("-c" in c[1] or "-lc" in c[1])
        for c in commands
    )


def test_sync_updates_and_post_actions(ss):
    ss, container_fs, host_fs, commands, config_db = ss
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
    ss, container_fs, host_fs, commands, config_db = ss
    item = ss.SyncItem("/container/missing.sh", "/usr/bin/restapi.sh", 0o755)
    ss.SYNC_ITEMS[:] = [item]
    ok = ss.ensure_sync()
    assert ok is False


def test_main_once_exits_zero_and_disables_post_actions(monkeypatch):
    # Default restapi has no reconcile logic; simple sync only.
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    ss = importlib.import_module("systemd_stub")

    ss.POST_COPY_ACTIONS["/bin/container_checker"] = [["sudo", "echo", "hi"]]
    monkeypatch.setattr(ss, "ensure_sync", lambda: True, raising=True)
    monkeypatch.setattr(sys, "argv", ["systemd_stub.py", "--once", "--no-post-actions"])

    rc = ss.main()
    assert rc == 0
    # Post-actions should be cleared (no-op check)
    assert not ss.POST_COPY_ACTIONS


def test_is_v1_enabled_false_uses_restapi_sh(monkeypatch):
    """Test that when IS_V1_ENABLED=false, restapi.sh is used as the source."""
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    
    monkeypatch.setenv("IS_V1_ENABLED", "false")
    ss = importlib.import_module("systemd_stub")
    
    # Verify the source is restapi.sh
    assert ss._RESTAPI_SRC == "/usr/share/sonic/systemd_scripts/restapi.sh"
    
    # Verify SYNC_ITEMS contains the correct source
    restapi_sync_item = next((item for item in ss.SYNC_ITEMS if item.dst_on_host == "/usr/bin/restapi.sh"), None)
    assert restapi_sync_item is not None
    assert restapi_sync_item.src_in_container == "/usr/share/sonic/systemd_scripts/restapi.sh"


def test_is_v1_enabled_true_uses_restapi_v1_sh(monkeypatch):
    """Test that when IS_V1_ENABLED=true, restapi_v1.sh is used as the source."""
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    
    monkeypatch.setenv("IS_V1_ENABLED", "true")
    ss = importlib.import_module("systemd_stub")
    
    # Verify the source is restapi_v1.sh
    assert ss._RESTAPI_SRC == "/usr/share/sonic/systemd_scripts/restapi_v1.sh"
    
    # Verify SYNC_ITEMS contains the correct source
    restapi_sync_item = next((item for item in ss.SYNC_ITEMS if item.dst_on_host == "/usr/bin/restapi.sh"), None)
    assert restapi_sync_item is not None
    assert restapi_sync_item.src_in_container == "/usr/share/sonic/systemd_scripts/restapi_v1.sh"


def test_is_v1_enabled_various_truthy_values(monkeypatch):
    """Test that IS_V1_ENABLED recognizes various truthy string values."""
    truthy_values = ["1", "true", "True", "TRUE", "yes", "Yes", "YES"]
    
    for value in truthy_values:
        if "systemd_stub" in sys.modules:
            del sys.modules["systemd_stub"]
        
        monkeypatch.setenv("IS_V1_ENABLED", value)
        ss = importlib.import_module("systemd_stub")
        
        assert ss._RESTAPI_SRC == "/usr/share/sonic/systemd_scripts/restapi_v1.sh", \
            f"Failed for IS_V1_ENABLED={value}"


def test_is_v1_enabled_default_when_not_set(monkeypatch):
    """Test that when IS_V1_ENABLED is not set, it defaults to false (restapi.sh)."""
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    
    monkeypatch.delenv("IS_V1_ENABLED", raising=False)
    ss = importlib.import_module("systemd_stub")
    
    # Verify the default is restapi.sh (not v1)
    assert ss._RESTAPI_SRC == "/usr/share/sonic/systemd_scripts/restapi.sh"


def test_post_copy_actions_match_sync_items():
    """Test that all POST_COPY_ACTIONS keys correspond to destination paths in SYNC_ITEMS."""
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    
    ss = importlib.import_module("systemd_stub")
    
    # Get all destination paths from SYNC_ITEMS
    sync_destinations = {item.dst_on_host for item in ss.SYNC_ITEMS}
    
    # Verify all POST_COPY_ACTIONS keys are in SYNC_ITEMS destinations
    for action_path in ss.POST_COPY_ACTIONS.keys():
        assert action_path in sync_destinations, \
            f"POST_COPY_ACTIONS key '{action_path}' does not match any destination in SYNC_ITEMS. " \
            f"Available destinations: {sorted(sync_destinations)}"
