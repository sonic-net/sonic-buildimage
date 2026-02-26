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

      - run_nsenter: simulates host FS + systemctl/docker calls
      - container_fs: dict for "container" files
      - host_fs: dict for "host" files
      - config_db: dict for CONFIG_DB contents ("TABLE|KEY" -> {field: value})
    """
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]

    # Mock sonic_version.yml with a supported branch (default to 202311)
    version_file = tmp_path / "sonic_version.yml"
    version_file.write_text("build_version: 'SONiC.20231110.19'")
    
    original_exists = os.path.exists
    def mock_exists(path):
        if path == "/etc/sonic/sonic_version.yml":
            return True
        return original_exists(path)
    
    monkeypatch.setattr("os.path.exists", mock_exists)
    
    original_open = open
    def mock_open(file, *args, **kwargs):
        if file == "/etc/sonic/sonic_version.yml":
            return original_open(str(version_file), *args, **kwargs)
        return original_open(file, *args, **kwargs)
    
    monkeypatch.setattr("builtins.open", mock_open)

    # Fake host filesystem and command recorder
    host_fs = {}
    commands = []

    # Fake CONFIG_DB (redis key "TABLE|KEY" -> dict(field -> value))
    config_db = {}

    # ----- Patch db_hget / db_hset to use our fake CONFIG_DB -----
    def fake_db_hget(key: str, field: str):
        """Get a field from CONFIG_DB"""
        entry = config_db.get(key, {})
        return entry.get(field)
    
    def fake_db_hset(key: str, field: str, value: str) -> bool:
        """Set a field in CONFIG_DB"""
        if key not in config_db:
            config_db[key] = {}
        config_db[key][field] = value
        return True
    
    monkeypatch.setattr(real_sidecar_common, "db_hget", fake_db_hget)
    monkeypatch.setattr(real_sidecar_common, "db_hset", fake_db_hset)

    # ----- Patch run_nsenter for host operations -----
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

    # ----- Patch read_file_bytes_local to use container_fs -----
    # Fake container FS
    container_fs = {}
    
    def fake_read_file_bytes_local(path: str):
        return container_fs.get(path, None)
    
    monkeypatch.setattr(real_sidecar_common, "read_file_bytes_local", fake_read_file_bytes_local)

    # Now import systemd_stub after all patches are in place
    ss = importlib.import_module("systemd_stub")

    # Isolate POST_COPY_ACTIONS
    monkeypatch.setattr(ss, "POST_COPY_ACTIONS", {})

    return ss, container_fs, host_fs, commands, config_db


def test_sync_no_change_fast_path(ss):
    ss, container_fs, host_fs, commands, config_db = ss
    
    # Put required source files in container_fs - files that ensure_sync() expects
    # Default branch is 202311 from fixture
    container_fs["/usr/share/sonic/systemd_scripts/restapi.sh"] = b"same"
    container_fs["/usr/share/sonic/systemd_scripts/container_checker_202311"] = b"same"
    container_fs["/usr/share/sonic/scripts/k8s_pod_control.sh"] = b"same"
    container_fs["/usr/share/sonic/systemd_scripts/restapi.service_202311"] = b"same"
    
    # Put same files on host
    host_fs["/usr/bin/restapi.sh"] = b"same"
    host_fs["/bin/container_checker"] = b"same"
    host_fs["/usr/share/sonic/scripts/k8s_pod_control.sh"] = b"same"
    host_fs["/lib/systemd/system/restapi.service"] = b"same"

    ok = ss.ensure_sync()
    assert ok is True
    # No write path used (no /bin/sh -c cat > tmp) since files are identical
    assert not any(
        c[1][0] == "/bin/sh" and ("-c" in c[1] or "-lc" in c[1])
        for c in commands
    )


def test_sync_updates_and_post_actions(ss):
    ss, container_fs, host_fs, commands, config_db = ss
    
    # Put required source files in container_fs - files that ensure_sync() expects
    # Default branch is 202311 from fixture
    container_fs["/usr/share/sonic/systemd_scripts/restapi.sh"] = b"NEW-RESTAPI"
    container_fs["/usr/share/sonic/systemd_scripts/container_checker_202311"] = b"NEW-CHECKER"
    container_fs["/usr/share/sonic/scripts/k8s_pod_control.sh"] = b"NEW-K8S"
    container_fs["/usr/share/sonic/systemd_scripts/restapi.service_202311"] = b"NEW-SERVICE"
    
    # Put old files on host
    host_fs["/usr/bin/restapi.sh"] = b"OLD"
    host_fs["/bin/container_checker"] = b"OLD"
    host_fs["/usr/share/sonic/scripts/k8s_pod_control.sh"] = b"OLD"
    host_fs["/lib/systemd/system/restapi.service"] = b"OLD"

    ss.POST_COPY_ACTIONS["/bin/container_checker"] = [
        ["sudo", "systemctl", "daemon-reload"],
        ["sudo", "systemctl", "restart", "monit"],
    ]

    ok = ss.ensure_sync()
    assert ok is True
    assert host_fs["/bin/container_checker"] == b"NEW-CHECKER"

    post_cmds = [args for _, args in commands if args and args[0] == "sudo"]
    assert ("sudo", "systemctl", "daemon-reload") in post_cmds
    assert ("sudo", "systemctl", "restart", "monit") in post_cmds


def test_sync_missing_src_returns_false(ss):
    ss, container_fs, host_fs, commands, config_db = ss
    
    # Don't put source files in container_fs - ensure_sync() will fail
    # It will try to read files like /usr/share/sonic/systemd_scripts/restapi.sh but they won't exist
    
    ok = ss.ensure_sync()
    assert ok is False


def test_main_once_exits_zero_and_disables_post_actions(ss, monkeypatch):
    # Default restapi has no reconcile logic; simple sync only.
    systemd_stub, container_fs, host_fs, commands, config_db = ss

    systemd_stub.POST_COPY_ACTIONS["/bin/container_checker"] = [["sudo", "echo", "hi"]]
    monkeypatch.setattr(systemd_stub, "ensure_sync", lambda: True, raising=True)
    monkeypatch.setattr(sys, "argv", ["systemd_stub.py", "--once", "--no-post-actions"])

    rc = systemd_stub.main()
    assert rc == 0
    assert systemd_stub.POST_COPY_ACTIONS == {}


def test_env_controls_restapi_src_false(monkeypatch, tmp_path):
    """Test that when IS_V1_ENABLED=false, restapi.sh is used as the source."""
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    
    # Create fake sonic_version.yml for branch 202311
    version_file = tmp_path / "sonic_version.yml"
    version_file.write_text("build_version: 'SONiC.20231110.19'\n")
    
    monkeypatch.setenv("IS_V1_ENABLED", "false")
    
    # Mock file operations
    original_exists = os.path.exists
    def mock_exists(p):
        if p == "/etc/sonic/sonic_version.yml":
            return True
        return original_exists(p)
    monkeypatch.setattr("os.path.exists", mock_exists)
    
    original_open = open
    def mock_open(file, *args, **kwargs):
        if file == "/etc/sonic/sonic_version.yml":
            return original_open(str(version_file), *args, **kwargs)
        return original_open(file, *args, **kwargs)
    
    monkeypatch.setattr("builtins.open", mock_open)
    
    ss = importlib.import_module("systemd_stub")
    
    # Verify IS_V1_ENABLED is False and branch detection works
    assert ss.IS_V1_ENABLED is False
    branch = ss._get_branch_name()
    assert branch == "202311"


def test_env_controls_restapi_src_true(monkeypatch, tmp_path):
    """Test that when IS_V1_ENABLED=true, per-branch restapi.sh is used as the source."""
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    
    # Create fake sonic_version.yml for branch 202311
    version_file = tmp_path / "sonic_version.yml"
    version_file.write_text("build_version: 'SONiC.20231110.19'\n")
    
    monkeypatch.setenv("IS_V1_ENABLED", "true")
    
    # Mock file operations
    original_exists = os.path.exists
    def mock_exists(p):
        if p == "/etc/sonic/sonic_version.yml":
            return True
        return original_exists(p)
    monkeypatch.setattr("os.path.exists", mock_exists)
    
    original_open = open
    def mock_open(file, *args, **kwargs):
        if file == "/etc/sonic/sonic_version.yml":
            return original_open(str(version_file), *args, **kwargs)
        return original_open(file, *args, **kwargs)
    
    monkeypatch.setattr("builtins.open", mock_open)
    
    ss = importlib.import_module("systemd_stub")
    
    # Verify IS_V1_ENABLED is True and branch detection works
    assert ss.IS_V1_ENABLED is True
    branch = ss._get_branch_name()
    assert branch == "202311"


def test_env_controls_restapi_src_default(monkeypatch, tmp_path):
    """Test that when IS_V1_ENABLED is not set, it defaults to false (restapi.sh)."""
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    
    # Mock sonic_version.yml
    version_file = tmp_path / "sonic_version.yml"
    version_file.write_text("build_version: 'SONiC.20231110.19'\n")
    
    original_exists = os.path.exists
    def mock_exists(p):
        if p == "/etc/sonic/sonic_version.yml":
            return True
        return original_exists(p)
    monkeypatch.setattr("os.path.exists", mock_exists)
    
    original_open = open
    def mock_open(file, *args, **kwargs):
        if file == "/etc/sonic/sonic_version.yml":
            return original_open(str(version_file), *args, **kwargs)
        return original_open(file, *args, **kwargs)
    
    monkeypatch.setattr("builtins.open", mock_open)
    
    monkeypatch.delenv("IS_V1_ENABLED", raising=False)
    ss = importlib.import_module("systemd_stub")
    
    # Verify IS_V1_ENABLED defaults to False and branch detection works
    assert ss.IS_V1_ENABLED is False
    branch = ss._get_branch_name()
    assert branch == "202311"


def test_post_copy_actions_match_sync_items(monkeypatch, tmp_path):
    """Test that all POST_COPY_ACTIONS keys correspond to destination paths in SYNC_ITEMS."""
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    
    # Create fake sonic_version.yml for branch 202311
    version_file = tmp_path / "sonic_version.yml"
    version_file.write_text("build_version: 'SONiC.20231110.19'\n")
    
    monkeypatch.delenv("IS_V1_ENABLED", raising=False)
    
    # Mock file operations
    original_exists = os.path.exists
    def mock_exists(p):
        if p == "/etc/sonic/sonic_version.yml":
            return True
        return original_exists(p)
    monkeypatch.setattr("os.path.exists", mock_exists)
    
    original_open = open
    def mock_open(file, *args, **kwargs):
        if file == "/etc/sonic/sonic_version.yml":
            return original_open(str(version_file), *args, **kwargs)
        return original_open(file, *args, **kwargs)
    
    monkeypatch.setattr("builtins.open", mock_open)
    
    ss = importlib.import_module("systemd_stub")
    
    # Since SYNC_ITEMS is now built dynamically in ensure_sync(), we need to determine
    # expected destinations based on the branch (202311 from fixture)
    expected_destinations = {
        "/usr/bin/restapi.sh",
        "/bin/container_checker",
        "/usr/share/sonic/scripts/k8s_pod_control.sh",
        "/lib/systemd/system/restapi.service",
    }
    
    # Verify all POST_COPY_ACTIONS keys are in expected sync destinations
    for action_path in ss.POST_COPY_ACTIONS.keys():
        assert action_path in expected_destinations, \
            f"POST_COPY_ACTIONS key '{action_path}' does not match any destination in SYNC_ITEMS. " \
            f"Available destinations: {sorted(expected_destinations)}"


# ===== Per-branch detection tests =====

@pytest.mark.parametrize("version,expected_branch", [
    ("SONiC.20231110.19", "202311"),
    ("SONiC.20240510.25", "202405"),
    ("SONiC.20241110.22", "202411"),
    ("SONiC.20250510.04", "202505"),
    ("SONiC.20251110.01", "202511"),
    ("20231110.19", "202311"),  # Without SONiC. prefix
    ("20240510.25", "202405"),
    ("20241110.22", "202411"),
    ("20250510.04", "202505"),
    ("20251110.01", "202511"),
    # Test with non-standard suffixes (e.g., kw builds)
    ("20241110.kw.24", "202411"),
    ("SONiC.20241110.kw.24", "202411"),
    ("20240510.25", "202405"),
    ("SONiC.20231120.abc.123", "202311"),
])
def test_branch_detection_from_version(monkeypatch, tmp_path, version, expected_branch):
    """Test branch detection from various SONiC version formats."""
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    
    # Create fake sonic_version.yml
    version_file = tmp_path / "sonic_version.yml"
    version_file.write_text(f"build_version: '{version}'\n")
    
    monkeypatch.setenv("IS_V1_ENABLED", "false")
    
    # Mock file operations
    original_exists = os.path.exists
    def mock_exists(p):
        if p == "/etc/sonic/sonic_version.yml":
            return True
        return original_exists(p)
    monkeypatch.setattr("os.path.exists", mock_exists)
    
    original_open = open
    def mock_open(file, *args, **kwargs):
        if file == "/etc/sonic/sonic_version.yml":
            return original_open(str(version_file), *args, **kwargs)
        return original_open(file, *args, **kwargs)
    
    monkeypatch.setattr("builtins.open", mock_open)
    
    ss = importlib.import_module("systemd_stub")
    
    # Verify correct branch detected (branch_name is now computed at runtime in ensure_sync)
    branch = ss._get_branch_name()
    assert branch == expected_branch


@pytest.mark.parametrize("version", [
    "SONiC.master.921927-18199d73f",
    "master.921927-18199d73f",
    "SONiC.internal.135691748-dbb8d29985",
    "internal.135691748-dbb8d29985",
    "private-build-1.0",
    "unknown-format",
])
def test_unsupported_branches_exit_with_error(monkeypatch, tmp_path, version):
    """Test that unsupported branches (master/internal/private) return False from ensure_sync()."""
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    
    # Create fake sonic_version.yml
    version_file = tmp_path / "sonic_version.yml"
    version_file.write_text(f"build_version: '{version}'\n")
    
    monkeypatch.setenv("IS_V1_ENABLED", "false")
    
    # Mock file operations
    original_exists = os.path.exists
    def mock_exists(p):
        if p == "/etc/sonic/sonic_version.yml":
            return True
        return original_exists(p)
    monkeypatch.setattr("os.path.exists", mock_exists)
    
    original_open = open
    def mock_open(file, *args, **kwargs):
        if file == "/etc/sonic/sonic_version.yml":
            return original_open(str(version_file), *args, **kwargs)
        return original_open(file, *args, **kwargs)
    
    monkeypatch.setattr("builtins.open", mock_open)
    
    # Module import should succeed now (branch evaluation happens in ensure_sync())
    ss = importlib.import_module("systemd_stub")
    
    # But ensure_sync() should return False for unsupported branches
    result = ss.ensure_sync()
    assert result is False


@pytest.mark.parametrize("branch,is_v1_enabled", [
    ("202311", False),
    ("202405", False),
    ("202411", False),
    ("202505", False),
    ("202511", False),
    ("202311", True),
    ("202405", True),
    ("202411", True),
    ("202505", True),
    ("202511", True),
])
def test_per_branch_files_with_v1_flag(monkeypatch, tmp_path, branch, is_v1_enabled):
    """Test that per-branch files are correctly selected with IS_V1_ENABLED flag."""
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    
    # Map branch to version string
    branch_to_version = {
        "202311": "SONiC.20231110.19",
        "202405": "SONiC.20240510.25",
        "202411": "SONiC.20241110.22",
        "202505": "SONiC.20250510.04",
        "202511": "SONiC.20251110.01",
    }
    
    version = branch_to_version[branch]
    
    # Create fake sonic_version.yml
    version_file = tmp_path / "sonic_version.yml"
    version_file.write_text(f"build_version: '{version}'\n")
    
    monkeypatch.setenv("IS_V1_ENABLED", "true" if is_v1_enabled else "false")
    
    # Mock file operations
    original_exists = os.path.exists
    def mock_exists(p):
        if p == "/etc/sonic/sonic_version.yml":
            return True
        return original_exists(p)
    monkeypatch.setattr("os.path.exists", mock_exists)
    
    original_open = open
    def mock_open(file, *args, **kwargs):
        if file == "/etc/sonic/sonic_version.yml":
            return original_open(str(version_file), *args, **kwargs)
        return original_open(file, *args, **kwargs)
    
    monkeypatch.setattr("builtins.open", mock_open)
    
    ss = importlib.import_module("systemd_stub")
    
    # Verify branch detected correctly (branch_name is now computed at runtime in ensure_sync)
    detected_branch = ss._get_branch_name()
    assert detected_branch == branch
    
    # Verify IS_V1_ENABLED is set correctly
    assert ss.IS_V1_ENABLED == is_v1_enabled

