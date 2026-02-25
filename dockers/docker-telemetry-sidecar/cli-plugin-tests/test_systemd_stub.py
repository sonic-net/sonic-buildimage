# tests/test_systemd_stub.py
import sys
import os
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

    # Mock _get_branch_name to return "202412" by default (avoids real file/nsenter I/O)
    monkeypatch.setattr(ss, "_get_branch_name", lambda: "202412")

    # Provide a default container_checker in both filesystems so the auto-appended
    # SyncItem from ensure_sync() is always satisfied and is a no-op.
    container_fs["/usr/share/sonic/systemd_scripts/container_checker"] = b"default-checker"
    host_fs["/bin/container_checker"] = b"default-checker"

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


def test_telemetry_service_syncs_to_host_when_different(ss):
    ss, container_fs, host_fs, commands, config_db = ss

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


# ─────────────────────────── New tests for CONFIG_DB reconcile ───────────────────────────

def test_reconcile_enables_user_auth_and_cname(ss):
    ss, container_fs, host_fs, commands, config_db = ss
    # Set module-level flags directly (they're read inside reconcile)
    ss.GNMI_VERIFY_ENABLED = True
    ss.GNMI_CLIENT_CNAME = "AME Infra CA o6"

    # Precondition: empty DB
    assert config_db == {}

    ss.reconcile_config_db_once()

    # user_auth must be set to 'cert'
    assert config_db.get("GNMI|gnmi", {}).get("user_auth") == "cert"
    # CNAME hash must exist with role=gnmi_show_readonly (default GNMI_CLIENT_ROLE)
    cname_key = f"GNMI_CLIENT_CERT|{ss.GNMI_CLIENT_CNAME}"
    assert config_db.get(cname_key, {}).get("role") == "gnmi_show_readonly"


def test_reconcile_disabled_removes_cname(ss):
    ss, container_fs, host_fs, commands, config_db = ss
    ss.GNMI_VERIFY_ENABLED = False
    ss.GNMI_CLIENT_CNAME = "AME Infra CA o6"

    # Seed an existing entry to be removed
    config_db[f"GNMI_CLIENT_CERT|{ss.GNMI_CLIENT_CNAME}"] = {"role": "gnmi_show_readonly"}

    ss.reconcile_config_db_once()

    assert f"GNMI_CLIENT_CERT|{ss.GNMI_CLIENT_CNAME}" not in config_db


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

    # Clear SYNC_ITEMS to focus only on the container_checker logic
    ss_mod.SYNC_ITEMS[:] = []

    ok = ss_mod.ensure_sync()
    assert ok is True
    assert host_fs["/bin/container_checker"] == b"checker-202411"


def test_ensure_sync_uses_default_checker(ss):
    """When branch is not 202411, ensure_sync uses the default container_checker."""
    ss_mod, container_fs, host_fs, commands, config_db = ss

    # _get_branch_name already returns "202412" from fixture default

    # Provide the default checker in the container and a different one on host
    container_fs["/usr/share/sonic/systemd_scripts/container_checker"] = b"checker-default"
    host_fs["/bin/container_checker"] = b"old-checker"

    # Clear SYNC_ITEMS to focus only on the container_checker logic
    ss_mod.SYNC_ITEMS[:] = []

    ok = ss_mod.ensure_sync()
    assert ok is True
    assert host_fs["/bin/container_checker"] == b"checker-default"


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
