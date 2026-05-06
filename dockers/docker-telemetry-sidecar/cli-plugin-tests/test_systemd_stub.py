# tests/test_systemd_stub.py
import sys
import os
import json
import types
import subprocess
import importlib

import pytest

# Add docker-telemetry-sidecar directory to path so we can import systemd_stub
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

# Add sonic-py-common to path so we can import the real sidecar_common
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "../../../src/sonic-py-common")))


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


def _fake_apply_patch(config_db, input_bytes, text):
    """Fake 'config apply-patch': apply RFC 6902 ops to the test config_db dict."""
    try:
        patch_ops = json.loads(input_bytes)
        for op in patch_ops:
            parts = op["path"].strip("/").split("/")
            if op["op"] == "add":
                if len(parts) == 1:
                    for k, v in op["value"].items():
                        config_db[f"{parts[0]}|{k}"] = dict(v)
                elif len(parts) == 2:
                    db_key = f"{parts[0]}|{parts[1]}"
                    if db_key in config_db:
                        config_db[db_key].update(op["value"])
                    else:
                        config_db[db_key] = dict(op["value"])
                elif len(parts) == 3:
                    db_key = f"{parts[0]}|{parts[1]}"
                    config_db.setdefault(db_key, {})[parts[2]] = op["value"]
            elif op["op"] == "replace":
                if len(parts) == 2:
                    config_db[f"{parts[0]}|{parts[1]}"] = dict(op["value"])
                elif len(parts) == 3:
                    db_key = f"{parts[0]}|{parts[1]}"
                    if db_key in config_db:
                        config_db[db_key][parts[2]] = op["value"]
            elif op["op"] == "remove":
                if len(parts) == 2:
                    config_db.pop(f"{parts[0]}|{parts[1]}", None)
                elif len(parts) == 3:
                    db_key = f"{parts[0]}|{parts[1]}"
                    e = config_db.get(db_key)
                    if e and parts[2] in e:
                        del e[parts[2]]
                        if not e:
                            del config_db[db_key]
        return 0, "" if text else b"", "" if text else b""
    except Exception as exc:
        return 1, "" if text else b"", str(exc) if text else str(exc).encode()


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

    def fake_db_hset(key: str, field: str, value) -> bool:
        """Set a field in a CONFIG_DB hash."""
        if key not in config_db:
            config_db[key] = {}
        config_db[key][field] = value
        return True

    def fake_db_del(key: str):
        """Delete a CONFIG_DB key entirely."""
        if key in config_db:
            del config_db[key]
            return True
        return False

    def fake_db_hdel(key: str, field: str):
        """Delete a single field from a CONFIG_DB hash."""
        entry = config_db.get(key)
        if entry is None or field not in entry:
            return True  # already absent
        del entry[field]
        if not entry:
            del config_db[key]
        return True

    def fake_db_get_table_keys(table: str):
        """Return all entry keys for a given table from config_db."""
        prefix = f"{table}|"
        return [k.split("|", 1)[1] for k in config_db if k.startswith(prefix)]

    monkeypatch.setattr(real_sidecar_common, "db_hget", fake_db_hget)
    monkeypatch.setattr(real_sidecar_common, "db_hgetall", fake_db_hgetall)
    monkeypatch.setattr(real_sidecar_common, "db_hset", fake_db_hset)
    monkeypatch.setattr(real_sidecar_common, "db_del", fake_db_del)
    monkeypatch.setattr(real_sidecar_common, "db_hdel", fake_db_hdel)
    monkeypatch.setattr(real_sidecar_common, "db_get_table_keys", fake_db_get_table_keys)

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

        # sudo -n config apply-patch /dev/stdin
        if (args[:4] == ["sudo", "-n", "config", "apply-patch"]
                and len(args) == 5 and args[4] == "/dev/stdin"
                and input_bytes is not None):
            return _fake_apply_patch(config_db, input_bytes, text)

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

    # Mock _get_branch_name to return "202412" by default (avoids real file/nsenter I/O)
    # Use "202412" because it is in the supported branch list.
    monkeypatch.setattr(ss, "_get_branch_name", lambda: "202412")

    # Reset the one-shot cleanup flag so each test starts fresh
    ss._stale_unit_cleaned = False
    monkeypatch.setattr(ss, "_STALE_UNIT_CLEANUP_ENABLED", True)

    # Provide the branch-specific container_checker in both filesystems so the auto-appended
    # SyncItem from ensure_sync() is always satisfied and is a no-op.
    container_fs["/usr/share/sonic/systemd_scripts/container_checker_202412"] = b"default-checker"
    host_fs["/bin/container_checker"] = b"default-checker"

    # Provide the branch-specific service_checker.py in both filesystems so the auto-appended
    # SyncItem from ensure_sync() is always satisfied and is a no-op.
    container_fs["/usr/share/sonic/systemd_scripts/service_checker.py_202412"] = b"default-service-checker"
    host_fs["/usr/local/lib/python3.11/dist-packages/health_checker/service_checker.py"] = b"default-service-checker"

    return ss, container_fs, host_fs, commands, config_db


def test_sync_no_change_fast_path(ss):
    ss, container_fs, host_fs, commands, config_db = ss
    item = ss.SyncItem("/container/telemetry.sh", "/host/telemetry.sh", 0o755)
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
    # Use telemetry.sh path (not /bin/container_checker) to avoid conflict
    # with the container_checker item that ensure_sync() appends automatically.
    item = ss.SyncItem("/container/telemetry.sh", "/usr/local/bin/telemetry.sh", 0o755)
    container_fs[item.src_in_container] = b"NEW"
    host_fs[item.dst_on_host] = b"OLD"
    ss.SYNC_ITEMS[:] = [item]

    ss.POST_COPY_ACTIONS[item.dst_on_host] = [
        ["sudo", "systemctl", "daemon-reload"],
        ["sudo", "systemctl", "restart", "telemetry"],
    ]

    ok = ss.ensure_sync()
    assert ok is True
    assert host_fs[item.dst_on_host] == b"NEW"

    post_cmds = [args for _, args in commands if args and args[0] == "sudo"]
    assert ("sudo", "systemctl", "daemon-reload") in post_cmds
    assert ("sudo", "systemctl", "restart", "telemetry") in post_cmds


def test_sync_missing_src_returns_false(ss):
    ss, container_fs, host_fs, commands, config_db = ss
    item = ss.SyncItem("/container/missing.sh", "/usr/local/bin/telemetry.sh", 0o755)
    ss.SYNC_ITEMS[:] = [item]
    ok = ss.ensure_sync()
    assert ok is False


def test_main_once_exits_zero_and_disables_post_actions(monkeypatch):
    # Default GNMI_VERIFY_ENABLED is False at import ⇒ reconcile is a no-op; no nsenter needed.
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



# ─────────────────────────── Tests for stale telemetry.service cleanup ───────────────────────────

STALE_UNIT = b"""[Unit]
Description=Telemetry container

[Service]
User=root
ExecStartPre=/usr/local/bin/telemetry.sh start
ExecStart=/usr/local/bin/telemetry.sh wait
"""

CLEAN_UNIT = b"""[Unit]
Description=Telemetry container

[Service]
User=admin
ExecStartPre=/usr/local/bin/telemetry.sh start
ExecStart=/usr/local/bin/telemetry.sh wait
"""


def test_cleanup_stale_unit_restores_from_packed_file(ss):
    """When host telemetry.service has User=root, cleanup overwrites it with the packed clean file."""
    ss_mod, container_fs, host_fs, commands, config_db = ss
    host_fs[ss_mod._HOST_TELEMETRY_SERVICE] = STALE_UNIT
    container_fs[ss_mod._CONTAINER_TELEMETRY_SERVICE] = CLEAN_UNIT

    ss_mod._cleanup_stale_service_unit()

    # Host file should now be the clean version
    assert host_fs[ss_mod._HOST_TELEMETRY_SERVICE] == CLEAN_UNIT
    # daemon-reload and restart should follow
    post_cmds = [args for _, args in commands if args and args[0] == "sudo"]
    assert ("sudo", "systemctl", "daemon-reload") in post_cmds
    assert ("sudo", "systemctl", "restart", "telemetry") in post_cmds


def test_cleanup_skips_when_user_admin(ss):
    """When host telemetry.service already has User=admin, cleanup is a no-op."""
    ss_mod, container_fs, host_fs, commands, config_db = ss
    host_fs[ss_mod._HOST_TELEMETRY_SERVICE] = CLEAN_UNIT

    ss_mod._cleanup_stale_service_unit()

    # No write should have occurred
    write_cmds = [args for _, args in commands if args and args[0] == "/bin/sh"]
    assert len(write_cmds) == 0


def test_cleanup_skips_when_file_missing(ss):
    """When host telemetry.service doesn't exist, cleanup is a no-op."""
    ss_mod, container_fs, host_fs, commands, config_db = ss
    # Don't put the file in host_fs

    ss_mod._cleanup_stale_service_unit()

    write_cmds = [args for _, args in commands if args and args[0] == "/bin/sh"]
    assert len(write_cmds) == 0


def test_cleanup_runs_only_once(ss):
    """The cleanup is a one-shot; second call should be a no-op."""
    ss_mod, container_fs, host_fs, commands, config_db = ss
    host_fs[ss_mod._HOST_TELEMETRY_SERVICE] = STALE_UNIT
    container_fs[ss_mod._CONTAINER_TELEMETRY_SERVICE] = CLEAN_UNIT

    ss_mod._cleanup_stale_service_unit()
    assert host_fs[ss_mod._HOST_TELEMETRY_SERVICE] == CLEAN_UNIT

    # Revert host to stale to prove second call is a no-op
    host_fs[ss_mod._HOST_TELEMETRY_SERVICE] = STALE_UNIT
    ss_mod._cleanup_stale_service_unit()
    # Should still be stale because the flag prevented re-run
    assert host_fs[ss_mod._HOST_TELEMETRY_SERVICE] == STALE_UNIT

def test_cleanup_retries_after_transient_read_failure(ss):
    """When host_read_bytes fails transiently, cleanup retries on the next call."""
    ss_mod, container_fs, host_fs, commands, config_db = ss
    container_fs[ss_mod._CONTAINER_TELEMETRY_SERVICE] = CLEAN_UNIT
    # First call: host file missing (transient failure)
    # Don't put the file in host_fs

    ss_mod._cleanup_stale_service_unit()
    assert ss_mod._stale_unit_cleaned is False  # flag NOT set; will retry

    # Second call: host file now present with stale content
    host_fs[ss_mod._HOST_TELEMETRY_SERVICE] = STALE_UNIT
    ss_mod._cleanup_stale_service_unit()
    assert host_fs[ss_mod._HOST_TELEMETRY_SERVICE] == CLEAN_UNIT
    assert ss_mod._stale_unit_cleaned is True


def test_cleanup_disabled_by_env(ss, monkeypatch):
    """When STALE_UNIT_CLEANUP_ENABLED=false, cleanup is skipped entirely."""
    ss_mod, container_fs, host_fs, commands, config_db = ss
    monkeypatch.setattr(ss_mod, "_STALE_UNIT_CLEANUP_ENABLED", False)
    host_fs[ss_mod._HOST_TELEMETRY_SERVICE] = STALE_UNIT
    container_fs[ss_mod._CONTAINER_TELEMETRY_SERVICE] = CLEAN_UNIT

    ss_mod._cleanup_stale_service_unit()
    # File should NOT be overwritten
    assert host_fs[ss_mod._HOST_TELEMETRY_SERVICE] == STALE_UNIT
    # Flag set so it won't retry
    assert ss_mod._stale_unit_cleaned is True


# ─────────────────────────── New tests for CONFIG_DB reconcile ───────────────────────────

def test_reconcile_enables_user_auth_and_cname(ss):
    ss, container_fs, host_fs, commands, config_db = ss
    # Set module-level flags directly (they're read inside reconcile)
    ss.GNMI_VERIFY_ENABLED = True
    ss.GNMI_CLIENT_CERTS = [{"cname": "fake-infra-ca.test.example.com", "role": ["gnmi_show_readonly"]}]

    # Precondition: empty DB
    assert config_db == {}

    ss.reconcile_config_db_once()

    assert config_db.get("TELEMETRY|gnmi", {}).get("user_auth") == "cert"
    # CNAME hash must exist with role=gnmi_show_readonly
    assert config_db.get("GNMI_CLIENT_CERT|fake-infra-ca.test.example.com", {}).get("role") == ["gnmi_show_readonly"]


def test_reconcile_disabled_removes_cname(ss):
    ss, container_fs, host_fs, commands, config_db = ss
    ss.GNMI_VERIFY_ENABLED = False

    # Seed an existing entry to be removed
    config_db["GNMI_CLIENT_CERT|fake-infra-ca.test.example.com"] = {"role": "gnmi_show_readonly"}

    ss.reconcile_config_db_once()

    assert "GNMI_CLIENT_CERT|fake-infra-ca.test.example.com" not in config_db


def test_reconcile_disabled_removes_multiple_cnames(ss):
    """When verify=false, all GNMI_CLIENT_CERT entries in the table are removed."""
    ss, container_fs, host_fs, commands, config_db = ss
    ss.GNMI_VERIFY_ENABLED = False

    config_db["GNMI_CLIENT_CERT|a.test.example.com"] = {"role": "admin"}
    config_db["GNMI_CLIENT_CERT|b.test.example.com"] = {"role": "gnmi_show_readonly"}

    ss.reconcile_config_db_once()

    assert "GNMI_CLIENT_CERT|a.test.example.com" not in config_db
    assert "GNMI_CLIENT_CERT|b.test.example.com" not in config_db


def test_reconcile_disabled_removes_entries_not_in_env(ss):
    """When verify=false, entries NOT in GNMI_CLIENT_CERTS env are also removed."""
    ss, container_fs, host_fs, commands, config_db = ss
    ss.GNMI_VERIFY_ENABLED = False
    ss.GNMI_CLIENT_CERTS = []  # env list is empty

    # These entries exist in DB but not in env
    config_db["GNMI_CLIENT_CERT|unknown.example.com"] = {"role": "admin"}
    config_db["GNMI_CLIENT_CERT|other.example.com"] = {"role": "gnmi_show_readonly"}

    ss.reconcile_config_db_once()

    assert "GNMI_CLIENT_CERT|unknown.example.com" not in config_db
    assert "GNMI_CLIENT_CERT|other.example.com" not in config_db


def test_reconcile_disabled_clears_user_auth(ss):
    """When verify=false and user_auth was 'cert', the field should be removed."""
    ss, container_fs, host_fs, commands, config_db = ss
    ss.GNMI_VERIFY_ENABLED = False

    # Seed user_auth=cert from a prior enabled state
    config_db["TELEMETRY|gnmi"] = {"user_auth": "cert"}

    ss.reconcile_config_db_once()

    assert "user_auth" not in config_db.get("TELEMETRY|gnmi", {})


def test_reconcile_disabled_removes_user_auth_any_value(ss):
    """When verify=false, user_auth should be removed regardless of its value."""
    ss, container_fs, host_fs, commands, config_db = ss
    ss.GNMI_VERIFY_ENABLED = False

    config_db["TELEMETRY|gnmi"] = {"user_auth": "password"}

    ss.reconcile_config_db_once()

    assert "user_auth" not in config_db.get("TELEMETRY|gnmi", {})


# ─────────────── Test that db_del passes None to truly delete keys ───────────────

def test_db_del_passes_none_to_set_entry(monkeypatch):
    """db_del must call set_entry(table, key, None) to truly remove the key.

    Passing {} would cause typed_to_raw to store {"NULL":"NULL"} as a placeholder.
    Passing None causes typed_to_raw to return {} which the C++ layer treats as
    a real deletion.
    """
    set_entry_calls = []

    class FakeConfigDB:
        def connect(self, *a, **kw):
            pass
        def get_entry(self, table, key):
            return {"role": "admin"}
        def set_entry(self, table, key, data):
            set_entry_calls.append((table, key, data))

    monkeypatch.setattr(real_sidecar_common, "_config_db", FakeConfigDB())

    ok = real_sidecar_common.db_del("GNMI_CLIENT_CERT|test.example.com")
    assert ok is True
    assert len(set_entry_calls) == 1
    table, key, data = set_entry_calls[0]
    assert table == "GNMI_CLIENT_CERT"
    assert key == "test.example.com"
    assert data is None, f"db_del must pass None to set_entry, got {data!r}"

def test_reconcile_multiple_cnames(ss):
    ss, container_fs, host_fs, commands, config_db = ss
    ss.GNMI_VERIFY_ENABLED = True
    ss.GNMI_CLIENT_CERTS = [
        {"cname": "fake-client.test.example.com", "role": ["admin"]},
        {"cname": "fake-server.test.example.com", "role": ["gnmi_show_readonly", "admin"]},
    ]
    assert config_db == {}
    ss.reconcile_config_db_once()

    assert config_db.get("TELEMETRY|gnmi", {}).get("user_auth") == "cert"
    assert config_db.get("GNMI_CLIENT_CERT|fake-client.test.example.com", {}).get("role") == ["admin"]
    assert config_db.get("GNMI_CLIENT_CERT|fake-server.test.example.com", {}).get("role") == ["gnmi_show_readonly", "admin"]

def test_reconcile_rewrites_stale_json_string_role(ss):
    """Reconcile must rewrite a stale JSON-string role into a proper list for YANG compliance."""
    ss, container_fs, host_fs, commands, config_db = ss
    ss.GNMI_VERIFY_ENABLED = True
    ss.GNMI_CLIENT_CERTS = [
        {"cname": "fake-client.test.example.com", "role": ["admin"]},
    ]

    # Seed a stale entry with old JSON-string format — stored as a string, not a list
    config_db["GNMI_CLIENT_CERT|fake-client.test.example.com"] = {"role": '["admin"]'}

    ss.reconcile_config_db_once()

    # Must be rewritten as a proper list so YANG leaf-list validation passes
    assert config_db.get("GNMI_CLIENT_CERT|fake-client.test.example.com", {}).get("role") == ["admin"]

def test_reconcile_rewrites_plain_string_role(ss):
    """Reconcile must rewrite a plain-string role into a list for YANG leaf-list compliance."""
    ss, container_fs, host_fs, commands, config_db = ss
    ss.GNMI_VERIFY_ENABLED = True
    ss.GNMI_CLIENT_CERTS = [
        {"cname": "fake-client.test.example.com", "role": ["gnmi_show_readonly"]},
    ]

    # Seed entry with old plain-string format (causes YANG 'Duplicated instance' errors)
    config_db["GNMI_CLIENT_CERT|fake-client.test.example.com"] = {"role": "gnmi_show_readonly"}

    ss.reconcile_config_db_once()

    # Must be rewritten as a proper list
    assert config_db.get("GNMI_CLIENT_CERT|fake-client.test.example.com", {}).get("role") == ["gnmi_show_readonly"]

def test_reconcile_overwrites_when_role_differs(ss):
    """Reconcile must overwrite when the stored role differs from the desired one."""
    ss, container_fs, host_fs, commands, config_db = ss
    ss.GNMI_VERIFY_ENABLED = True
    ss.GNMI_CLIENT_CERTS = [
        {"cname": "fake-client.test.example.com", "role": ["admin", "gnmi_show_readonly"]},
    ]

    # Seed an entry with a different role
    config_db["GNMI_CLIENT_CERT|fake-client.test.example.com"] = {"role": ["admin"]}

    ss.reconcile_config_db_once()

    # Must be overwritten with the new role list
    assert config_db.get("GNMI_CLIENT_CERT|fake-client.test.example.com", {}).get("role") == ["admin", "gnmi_show_readonly"]

def test_reconcile_skips_when_role_matches(ss):
    """Reconcile should not rewrite if the role already matches."""
    ss, container_fs, host_fs, commands, config_db = ss
    ss.GNMI_VERIFY_ENABLED = True
    ss.GNMI_CLIENT_CERTS = [
        {"cname": "fake-client.test.example.com", "role": ["admin"]},
    ]

    # Seed an entry that already matches
    config_db["GNMI_CLIENT_CERT|fake-client.test.example.com"] = {"role": ["admin"]}

    ss.reconcile_config_db_once()

    assert config_db.get("GNMI_CLIENT_CERT|fake-client.test.example.com", {}).get("role") == ["admin"]

# ─────────────────────────── Tests for _parse_client_certs ───────────────────────────

class TestParseClientCerts:
    """Tests for _parse_client_certs() env-var parsing."""

    @pytest.fixture(autouse=True)
    def _fresh_module(self, monkeypatch):
        if "systemd_stub" in sys.modules:
            del sys.modules["systemd_stub"]
        self.monkeypatch = monkeypatch

    def _import_with_env(self, env_vars):
        """Set env vars, re-import systemd_stub, and return the parsed GNMI_CLIENT_CERTS."""
        for k, v in env_vars.items():
            if v is None:
                self.monkeypatch.delenv(k, raising=False)
            else:
                self.monkeypatch.setenv(k, v)
        # Clear stale env vars not in the dict
        for k in ("GNMI_CLIENT_CERTS", "TELEMETRY_CLIENT_CNAME", "GNMI_CLIENT_ROLE"):
            if k not in env_vars:
                self.monkeypatch.delenv(k, raising=False)
        if "systemd_stub" in sys.modules:
            del sys.modules["systemd_stub"]
        ss = importlib.import_module("systemd_stub")
        return ss.GNMI_CLIENT_CERTS

    def test_valid_json_array(self):
        certs = self._import_with_env({
            "GNMI_CLIENT_CERTS": '[{"cname": "client.gbl", "role": "admin"}]'
        })
        assert certs == [{"cname": "client.gbl", "role": ["admin"]}]

    def test_valid_json_multiple_entries(self):
        certs = self._import_with_env({
            "GNMI_CLIENT_CERTS": '[{"cname": "a.gbl", "role": "admin"}, {"cname": "b.gbl", "role": "readonly"}]'
        })
        assert len(certs) == 2
        assert certs[0] == {"cname": "a.gbl", "role": ["admin"]}
        assert certs[1] == {"cname": "b.gbl", "role": ["readonly"]}

    def test_role_as_json_list(self):
        """role provided as a JSON array should be preserved as a list."""
        certs = self._import_with_env({
            "GNMI_CLIENT_CERTS": '[{"cname": "s.gbl", "role": ["gnmi_show_readonly", "admin"]}]'
        })
        assert certs == [{"cname": "s.gbl", "role": ["gnmi_show_readonly", "admin"]}]

    def test_non_array_json_falls_back_to_legacy(self):
        certs = self._import_with_env({
            "GNMI_CLIENT_CERTS": '{"cname": "c.gbl", "role": "admin"}',
            "TELEMETRY_CLIENT_CNAME": "legacy.gbl",
            "GNMI_CLIENT_ROLE": "readonly",
        })
        assert certs == [{"cname": "legacy.gbl", "role": ["readonly"]}]

    def test_invalid_json_falls_back_to_legacy(self):
        certs = self._import_with_env({
            "GNMI_CLIENT_CERTS": "not-json!",
            "TELEMETRY_CLIENT_CNAME": "fallback.gbl",
        })
        assert certs == [{"cname": "fallback.gbl", "role": ["gnmi_show_readonly"]}]

    def test_entry_not_dict_falls_back(self):
        certs = self._import_with_env({
            "GNMI_CLIENT_CERTS": '["not-a-dict"]',
            "TELEMETRY_CLIENT_CNAME": "fb.gbl",
        })
        assert certs == [{"cname": "fb.gbl", "role": ["gnmi_show_readonly"]}]

    def test_entry_missing_role_falls_back(self):
        certs = self._import_with_env({
            "GNMI_CLIENT_CERTS": '[{"cname": "x.gbl"}]',
            "TELEMETRY_CLIENT_CNAME": "fb.gbl",
        })
        assert certs == [{"cname": "fb.gbl", "role": ["gnmi_show_readonly"]}]

    def test_entry_empty_cname_falls_back(self):
        certs = self._import_with_env({
            "GNMI_CLIENT_CERTS": '[{"cname": "  ", "role": "admin"}]',
            "TELEMETRY_CLIENT_CNAME": "fb.gbl",
        })
        assert certs == [{"cname": "fb.gbl", "role": ["gnmi_show_readonly"]}]

    def test_legacy_single_entry(self):
        certs = self._import_with_env({
            "TELEMETRY_CLIENT_CNAME": "legacy.gbl",
            "GNMI_CLIENT_ROLE": "admin",
        })
        assert certs == [{"cname": "legacy.gbl", "role": ["admin"]}]

    def test_legacy_default_role(self):
        certs = self._import_with_env({
            "TELEMETRY_CLIENT_CNAME": "legacy.gbl",
        })
        assert certs == [{"cname": "legacy.gbl", "role": ["gnmi_show_readonly"]}]

    def test_no_env_returns_empty(self):
        certs = self._import_with_env({})
        assert certs == []

    def test_whitespace_stripped(self):
        certs = self._import_with_env({
            "GNMI_CLIENT_CERTS": '[{"cname": " client.gbl ", "role": " admin "}]'
        })
        assert certs == [{"cname": "client.gbl", "role": ["admin"]}]


# ─────────────────────────── Tests for _get_branch_name ───────────────────────────

class TestGetBranchName:
    """Tests for _get_branch_name() version-string parsing."""

    @pytest.fixture(autouse=True)
    def _fresh_module(self, monkeypatch):
        """Re-import systemd_stub fresh and expose the real _get_branch_name."""
        if "systemd_stub" in sys.modules:
            del sys.modules["systemd_stub"]
        self.ss = importlib.import_module("systemd_stub")
        self.monkeypatch = monkeypatch

    def _set_version_file(self, tmp_path, version_str):
        """Create a fake sonic_version.yml and patch the path."""
        vfile = tmp_path / "sonic_version.yml"
        vfile.write_text(f'build_version: "{version_str}"\n')
        self.monkeypatch.setattr(os.path, "exists", lambda p: p == str(vfile) or os.path.isfile(p))
        # Patch the version_file path inside _get_branch_name
        original_fn = self.ss._get_branch_name
        def patched():
            import types
            # Temporarily replace the hard-coded path
            src = original_fn.__code__
            # Simpler: just write to the expected path
            return original_fn()
        # Instead of complex patching, write to a temp file and patch open/exists
        return str(vfile)

    def _mock_version(self, version_str):
        """Mock _get_branch_name by patching the file read to return a specific version."""
        original_open = open
        version_file = "/etc/sonic/sonic_version.yml"

        def fake_exists(path):
            if path == version_file:
                return True
            return os.path.isfile(path)

        def fake_open(path, *args, **kwargs):
            if path == version_file:
                import io
                return io.StringIO(f'build_version: "{version_str}"\n')
            return original_open(path, *args, **kwargs)

        self.monkeypatch.setattr(os.path, "exists", fake_exists)
        self.monkeypatch.setattr("builtins.open", fake_open)

    def test_master_branch(self):
        self._mock_version("SONiC.master.921927-18199d73f")
        assert self.ss._get_branch_name() == "master"

    def test_master_branch_no_prefix(self):
        self._mock_version("master.100000-abcdef1234")
        assert self.ss._get_branch_name() == "master"

    def test_internal_branch(self):
        self._mock_version("SONiC.internal.135691748-dbb8d29985")
        assert self.ss._get_branch_name() == "internal"

    def test_internal_branch_no_prefix(self):
        self._mock_version("internal.999999-1234abcdef")
        assert self.ss._get_branch_name() == "internal"

    def test_feature_branch_202411(self):
        self._mock_version("SONiC.20241110.kw.24")
        assert self.ss._get_branch_name() == "202411"

    def test_feature_branch_202412(self):
        self._mock_version("SONiC.20241215.99")
        assert self.ss._get_branch_name() == "202412"

    def test_feature_branch_202505(self):
        self._mock_version("20250501.1")
        assert self.ss._get_branch_name() == "202505"

    def test_private_unmatched(self):
        self._mock_version("my-custom-build-v3")
        assert self.ss._get_branch_name() == "private"

    def test_no_version_file(self):
        """When no version file and nsenter also fails, returns 'private'."""
        self.monkeypatch.setattr(os.path, "exists", lambda p: False)
        self.monkeypatch.setattr(
            subprocess, "run",
            lambda *a, **kw: types.SimpleNamespace(returncode=1, stdout="", stderr="")
        )
        assert self.ss._get_branch_name() == "private"


# ─────────── Tests for branch-conditional container_checker in ensure_sync ───────────

def test_ensure_sync_uses_202411_checker(ss):
    """When branch is 202411, ensure_sync uses the branch-specific container_checker."""
    ss_mod, container_fs, host_fs, commands, config_db = ss

    # Override _get_branch_name to return 202411
    ss_mod._get_branch_name = lambda: "202411"

    # Provide the 202411-specific checker in the container and a different one on host
    container_fs["/usr/share/sonic/systemd_scripts/container_checker_202411"] = b"checker-202411"
    host_fs["/bin/container_checker"] = b"old-checker"

    # Also provide 202411 service_checker so it doesn't fail
    container_fs["/usr/share/sonic/systemd_scripts/service_checker.py_202411"] = b"service-checker-202411"
    host_fs[ss_mod.HOST_SERVICE_CHECKER] = b"service-checker-202411"

    # Clear SYNC_ITEMS to focus only on the container_checker logic
    ss_mod.SYNC_ITEMS[:] = []

    ok = ss_mod.ensure_sync()
    assert ok is True
    assert host_fs["/bin/container_checker"] == b"checker-202411"


def test_ensure_sync_aborts_for_unsupported_branch(ss):
    """When branch is not in the supported list, ensure_sync aborts and returns False."""
    ss_mod, container_fs, host_fs, commands, config_db = ss

    ss_mod._get_branch_name = lambda: "master"

    host_fs["/bin/container_checker"] = b"old-checker"

    ok = ss_mod.ensure_sync()
    assert ok is False
    # Nothing should be synced
    assert host_fs["/bin/container_checker"] == b"old-checker"


def test_ensure_sync_202411_missing_checker_fails(ss):
    """When branch is 202411 but the branch-specific checker is missing, sync fails."""
    ss_mod, container_fs, host_fs, commands, config_db = ss

    ss_mod._get_branch_name = lambda: "202411"

    # Don't provide the 202411 checker in container_fs
    # Remove default checker too
    container_fs.pop("/usr/share/sonic/systemd_scripts/container_checker_202411", None)
    container_fs.pop("/usr/share/sonic/systemd_scripts/container_checker", None)

    ss_mod.SYNC_ITEMS[:] = []

    ok = ss_mod.ensure_sync()
    assert ok is False


# ─────────── Tests for branch-conditional service_checker.py in ensure_sync ───────────

def test_ensure_sync_uses_202411_service_checker(ss):
    """When branch is 202411, ensure_sync uses the branch-specific service_checker.py."""
    ss_mod, container_fs, host_fs, commands, config_db = ss

    ss_mod._get_branch_name = lambda: "202411"

    # Provide the 202411-specific service_checker in the container and a different one on host
    container_fs["/usr/share/sonic/systemd_scripts/service_checker.py_202411"] = b"service-checker-202411"
    host_fs[ss_mod.HOST_SERVICE_CHECKER] = b"old-service-checker"

    # Also provide container_checker so it doesn't fail
    container_fs["/usr/share/sonic/systemd_scripts/container_checker_202411"] = b"checker-202411"
    host_fs["/bin/container_checker"] = b"checker-202411"

    ss_mod.SYNC_ITEMS[:] = []

    ok = ss_mod.ensure_sync()
    assert ok is True
    assert host_fs[ss_mod.HOST_SERVICE_CHECKER] == b"service-checker-202411"


def test_ensure_sync_aborts_service_checker_for_unsupported_branch(ss):
    """When branch is not in the supported list, ensure_sync aborts without syncing service_checker."""
    ss_mod, container_fs, host_fs, commands, config_db = ss

    ss_mod._get_branch_name = lambda: "master"

    host_fs[ss_mod.HOST_SERVICE_CHECKER] = b"old-service-checker"

    ok = ss_mod.ensure_sync()
    assert ok is False
    # service_checker should NOT be overwritten
    assert host_fs[ss_mod.HOST_SERVICE_CHECKER] == b"old-service-checker"


def test_ensure_sync_202411_missing_service_checker_fails(ss):
    """When branch is 202411 but the branch-specific service_checker is missing, sync fails."""
    ss_mod, container_fs, host_fs, commands, config_db = ss

    ss_mod._get_branch_name = lambda: "202411"

    # Provide container_checker so only service_checker causes the failure
    container_fs["/usr/share/sonic/systemd_scripts/container_checker_202411"] = b"checker-202411"
    host_fs["/bin/container_checker"] = b"checker-202411"

    # Remove service_checker sources
    container_fs.pop("/usr/share/sonic/systemd_scripts/service_checker.py_202411", None)
    container_fs.pop("/usr/share/sonic/systemd_scripts/service_checker.py", None)

    ss_mod.SYNC_ITEMS[:] = []

    ok = ss_mod.ensure_sync()
    assert ok is False


# ─────────── Tests for branch-conditional 202505 in ensure_sync ───────────

def test_ensure_sync_uses_202505_checker(ss):
    """When branch is 202505, ensure_sync uses the branch-specific container_checker."""
    ss_mod, container_fs, host_fs, commands, config_db = ss

    ss_mod._get_branch_name = lambda: "202505"

    container_fs["/usr/share/sonic/systemd_scripts/container_checker_202505"] = b"checker-202505"
    host_fs["/bin/container_checker"] = b"old-checker"

    # Also provide 202505 service_checker so it doesn't fail
    container_fs["/usr/share/sonic/systemd_scripts/service_checker.py_202505"] = b"service-checker-202505"
    host_fs[ss_mod.HOST_SERVICE_CHECKER] = b"service-checker-202505"

    ss_mod.SYNC_ITEMS[:] = []

    ok = ss_mod.ensure_sync()
    assert ok is True
    assert host_fs["/bin/container_checker"] == b"checker-202505"


def test_ensure_sync_uses_202505_service_checker(ss):
    """When branch is 202505, ensure_sync uses the branch-specific service_checker.py."""
    ss_mod, container_fs, host_fs, commands, config_db = ss

    ss_mod._get_branch_name = lambda: "202505"

    container_fs["/usr/share/sonic/systemd_scripts/service_checker.py_202505"] = b"service-checker-202505"
    host_fs[ss_mod.HOST_SERVICE_CHECKER] = b"old-service-checker"

    # Also provide container_checker so it doesn't fail
    container_fs["/usr/share/sonic/systemd_scripts/container_checker_202505"] = b"checker-202505"
    host_fs["/bin/container_checker"] = b"checker-202505"

    ss_mod.SYNC_ITEMS[:] = []

    ok = ss_mod.ensure_sync()
    assert ok is True
    assert host_fs[ss_mod.HOST_SERVICE_CHECKER] == b"service-checker-202505"


def test_ensure_sync_202505_missing_checker_fails(ss):
    """When branch is 202505 but the branch-specific checker is missing, sync fails."""
    ss_mod, container_fs, host_fs, commands, config_db = ss

    ss_mod._get_branch_name = lambda: "202505"

    container_fs.pop("/usr/share/sonic/systemd_scripts/container_checker_202505", None)
    container_fs.pop("/usr/share/sonic/systemd_scripts/container_checker", None)

    ss_mod.SYNC_ITEMS[:] = []

    ok = ss_mod.ensure_sync()
    assert ok is False


def test_ensure_sync_202505_missing_service_checker_fails(ss):
    """When branch is 202505 but the branch-specific service_checker is missing, sync fails."""
    ss_mod, container_fs, host_fs, commands, config_db = ss

    ss_mod._get_branch_name = lambda: "202505"

    # Provide container_checker so only service_checker causes the failure
    container_fs["/usr/share/sonic/systemd_scripts/container_checker_202505"] = b"checker-202505"
    host_fs["/bin/container_checker"] = b"checker-202505"

    container_fs.pop("/usr/share/sonic/systemd_scripts/service_checker.py_202505", None)
    container_fs.pop("/usr/share/sonic/systemd_scripts/service_checker.py", None)

    ss_mod.SYNC_ITEMS[:] = []

    ok = ss_mod.ensure_sync()
    assert ok is False


# ─────────── Tests for branch-conditional 202412 in ensure_sync ───────────

def test_ensure_sync_uses_202412_checker(ss):
    """When branch is 202412, ensure_sync uses the branch-specific container_checker."""
    ss_mod, container_fs, host_fs, commands, config_db = ss

    ss_mod._get_branch_name = lambda: "202412"

    container_fs["/usr/share/sonic/systemd_scripts/container_checker_202412"] = b"checker-202412"
    host_fs["/bin/container_checker"] = b"old-checker"

    # Also provide 202412 service_checker so it doesn't fail
    container_fs["/usr/share/sonic/systemd_scripts/service_checker.py_202412"] = b"service-checker-202412"
    host_fs[ss_mod.HOST_SERVICE_CHECKER] = b"service-checker-202412"

    ss_mod.SYNC_ITEMS[:] = []

    ok = ss_mod.ensure_sync()
    assert ok is True
    assert host_fs["/bin/container_checker"] == b"checker-202412"


def test_ensure_sync_uses_202412_service_checker(ss):
    """When branch is 202412, ensure_sync uses the branch-specific service_checker.py."""
    ss_mod, container_fs, host_fs, commands, config_db = ss

    ss_mod._get_branch_name = lambda: "202412"

    container_fs["/usr/share/sonic/systemd_scripts/service_checker.py_202412"] = b"service-checker-202412"
    host_fs[ss_mod.HOST_SERVICE_CHECKER] = b"old-service-checker"

    # Also provide container_checker so it doesn't fail
    container_fs["/usr/share/sonic/systemd_scripts/container_checker_202412"] = b"checker-202412"
    host_fs["/bin/container_checker"] = b"checker-202412"

    ss_mod.SYNC_ITEMS[:] = []

    ok = ss_mod.ensure_sync()
    assert ok is True
    assert host_fs[ss_mod.HOST_SERVICE_CHECKER] == b"service-checker-202412"


def test_ensure_sync_202412_missing_checker_fails(ss):
    """When branch is 202412 but the branch-specific checker is missing, sync fails."""
    ss_mod, container_fs, host_fs, commands, config_db = ss

    ss_mod._get_branch_name = lambda: "202412"

    container_fs.pop("/usr/share/sonic/systemd_scripts/container_checker_202412", None)
    container_fs.pop("/usr/share/sonic/systemd_scripts/container_checker", None)

    ss_mod.SYNC_ITEMS[:] = []

    ok = ss_mod.ensure_sync()
    assert ok is False


def test_ensure_sync_202412_missing_service_checker_fails(ss):
    """When branch is 202412 but the branch-specific service_checker is missing, sync fails."""
    ss_mod, container_fs, host_fs, commands, config_db = ss

    ss_mod._get_branch_name = lambda: "202412"

    # Provide container_checker so only service_checker causes the failure
    container_fs["/usr/share/sonic/systemd_scripts/container_checker_202412"] = b"checker-202412"
    host_fs["/bin/container_checker"] = b"checker-202412"

    container_fs.pop("/usr/share/sonic/systemd_scripts/service_checker.py_202412", None)
    container_fs.pop("/usr/share/sonic/systemd_scripts/service_checker.py", None)

    ss_mod.SYNC_ITEMS[:] = []

    ok = ss_mod.ensure_sync()
    assert ok is False


# ─────────────── Tests for _apply_config_patch / _build_enabled_patch ───────────────

def test_apply_config_patch_empty_is_noop(ss):
    """An empty patch list should return True without calling nsenter."""
    ss_mod, container_fs, host_fs, commands, config_db = ss
    before_count = len(commands)

    result = ss_mod._apply_config_patch([])
    assert result is True
    assert len(commands) == before_count  # no new nsenter calls


def test_apply_config_patch_calls_nsenter(ss):
    """A non-empty patch should invoke 'sudo -n config apply-patch /dev/stdin'."""
    ss_mod, container_fs, host_fs, commands, config_db = ss
    patch = [{
        "op": "add", "path": "/GNMI_CLIENT_CERT",
        "value": {"probe.example.com": {"role": ["admin"]}}
    }]

    result = ss_mod._apply_config_patch(patch)
    assert result is True

    apply_cmds = [
        args for _, args in commands
        if len(args) >= 4 and args[:4] == ("sudo", "-n", "config", "apply-patch")
    ]
    assert len(apply_cmds) >= 1


def test_build_enabled_patch_table_absent(ss):
    """When GNMI_CLIENT_CERT table is absent, patch uses a single 'add' at table level."""
    ss_mod, container_fs, host_fs, commands, config_db = ss
    ss_mod.GNMI_VERIFY_ENABLED = True
    ss_mod.GNMI_CLIENT_CERTS = [
        {"cname": "a.test.example.com", "role": ["admin"]},
        {"cname": "b.test.example.com", "role": ["readonly"]},
    ]
    config_db.clear()

    patch = ss_mod._build_enabled_patch()

    # Should have user_auth add + one table-level add
    assert len(patch) == 2
    table_op = [p for p in patch if p["path"] == "/GNMI_CLIENT_CERT"]
    assert len(table_op) == 1
    assert table_op[0]["op"] == "add"
    assert "a.test.example.com" in table_op[0]["value"]
    assert "b.test.example.com" in table_op[0]["value"]


def test_build_enabled_patch_mixed_new_and_existing(ss):
    """When table exists, new entries get 'add' and stale entries get 'replace'."""
    ss_mod, container_fs, host_fs, commands, config_db = ss
    ss_mod.GNMI_VERIFY_ENABLED = True
    ss_mod.GNMI_CLIENT_CERTS = [
        {"cname": "existing.test.example.com", "role": ["admin", "readonly"]},
        {"cname": "new.test.example.com", "role": ["admin"]},
    ]
    # Seed existing entry with a different (stale) role
    config_db.clear()
    config_db["GNMI_CLIENT_CERT|existing.test.example.com"] = {"role": ["admin"]}

    patch = ss_mod._build_enabled_patch()

    # user_auth add + replace for existing + add for new = 3 ops
    cert_ops = [p for p in patch if p["path"].startswith("/GNMI_CLIENT_CERT/")]
    assert len(cert_ops) == 2

    replace_op = [p for p in cert_ops if p["op"] == "replace"]
    assert len(replace_op) == 1
    assert replace_op[0]["path"] == "/GNMI_CLIENT_CERT/existing.test.example.com"

    add_op = [p for p in cert_ops if p["op"] == "add"]
    assert len(add_op) == 1
    assert add_op[0]["path"] == "/GNMI_CLIENT_CERT/new.test.example.com"


def test_build_enabled_patch_all_up_to_date(ss):
    """When everything matches, the patch should be empty."""
    ss_mod, container_fs, host_fs, commands, config_db = ss
    ss_mod.GNMI_VERIFY_ENABLED = True
    ss_mod.GNMI_CLIENT_CERTS = [
        {"cname": "ok.test.example.com", "role": ["admin"]},
    ]
    config_db.clear()
    config_db["TELEMETRY|gnmi"] = {"user_auth": "cert", "port": "8080"}
    config_db["GNMI_CLIENT_CERT|ok.test.example.com"] = {"role": ["admin"]}

    patch = ss_mod._build_enabled_patch()
    assert patch == []


def test_reconcile_enabled_uses_apply_patch_command(ss):
    """Reconcile in enabled mode should use 'config apply-patch' via nsenter."""
    ss_mod, container_fs, host_fs, commands, config_db = ss
    ss_mod.GNMI_VERIFY_ENABLED = True
    ss_mod.GNMI_CLIENT_CERTS = [{"cname": "test.example.com", "role": ["admin"]}]
    config_db.clear()

    ss_mod.reconcile_config_db_once()

    apply_cmds = [
        args for _, args in commands
        if len(args) >= 4 and args[:4] == ("sudo", "-n", "config", "apply-patch")
    ]
    assert len(apply_cmds) >= 1
    # Verify the end state
    assert config_db.get("TELEMETRY|gnmi", {}).get("user_auth") == "cert"
    assert config_db.get("GNMI_CLIENT_CERT|test.example.com", {}).get("role") == ["admin"]
