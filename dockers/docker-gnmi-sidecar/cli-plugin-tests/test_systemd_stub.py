# tests/test_systemd_stub.py
import sys
import os
import json
import types
import importlib

import pytest

# Add docker-gnmi-sidecar directory to path so we can import systemd_stub
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

# Add sonic-py-common to path so we can import the real sidecar_common
# Path: cli-plugin-tests -> docker-gnmi-sidecar -> dockers -> sonic-buildimage -> src/sonic-py-common
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


def _to_nested(config_db):
    nested = {}
    for db_key, fields in config_db.items():
        table, key = db_key.split("|", 1)
        nested.setdefault(table, {})[key] = json.loads(json.dumps(fields))
    return nested


def _from_nested(nested):
    return {f"{table}|{key}": fields
            for table, entries in nested.items() for key, fields in entries.items()}


def _strict_apply(doc, patch_ops):
    """Apply RFC 6902 add/replace ops; a missing parent (add) or target (replace) raises."""
    for op in patch_ops:
        parts = [p.replace("~1", "/").replace("~0", "~") for p in op["path"].lstrip("/").split("/")]
        parent = doc
        for part in parts[:-1]:
            if not isinstance(parent, dict) or part not in parent:
                raise KeyError(f"member {part!r} not found for path {op['path']}")
            parent = parent[part]
        leaf = parts[-1]
        if op["op"] == "replace" and leaf not in parent:
            raise KeyError(f"replace target {op['path']} does not exist")
        if op["op"] not in ("add", "replace"):
            raise ValueError(f"unsupported op {op['op']}")
        parent[leaf] = json.loads(json.dumps(op["value"]))
    return doc


def _fake_apply_patch(config_db, input_bytes, text):
    """Fake 'config apply-patch': strictly apply the patch to the test config_db dict."""
    try:
        nested = _strict_apply(_to_nested(config_db), json.loads(input_bytes))
    except Exception as exc:
        return 1, "" if text else b"", str(exc) if text else str(exc).encode()
    config_db.clear()
    config_db.update(_from_nested(nested))
    return 0, "" if text else b"", "" if text else b""


@pytest.fixture(scope="session", autouse=True)
def fake_logger_module():
    """
    Fakes were already set up at module level by _setup_fakes().
    This fixture just ensures they stay registered during test execution.
    """
    yield


@pytest.fixture
def config_db(monkeypatch):
    """Fake CONFIG_DB ("TABLE|KEY" -> {field: value}) behind the sidecar_common db_* helpers."""
    db = {}

    def fake_db_hget(key, field):
        return db.get(key, {}).get(field)

    def fake_db_hgetall(key):
        return dict(db.get(key, {}))

    def fake_db_hdel(key, field):
        entry = db.get(key)
        if entry is None or field not in entry:
            return True
        del entry[field]
        if not entry:
            del db[key]
        return True

    def fake_db_del(key):
        return db.pop(key, None) is not None

    def fake_db_get_table_keys(table):
        prefix = f"{table}|"
        return [k.split("|", 1)[1] for k in db if k.startswith(prefix)]

    monkeypatch.setattr(real_sidecar_common, "db_hget", fake_db_hget)
    monkeypatch.setattr(real_sidecar_common, "db_hgetall", fake_db_hgetall)
    monkeypatch.setattr(real_sidecar_common, "db_hdel", fake_db_hdel)
    monkeypatch.setattr(real_sidecar_common, "db_del", fake_db_del)
    monkeypatch.setattr(real_sidecar_common, "db_get_table_keys", fake_db_get_table_keys)
    return db


@pytest.fixture
def ss(tmp_path, monkeypatch, config_db):
    """
    Import systemd_stub fresh for every test, and provide fakes:

      - run_nsenter: simulates host FS + systemctl/docker calls (patched on sidecar_common)
      - container_fs: dict for "container" files
      - host_fs: dict for "host" files
      - config_db: see the config_db fixture; 'config apply-patch' writes to it
    """
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]

    # Enable container_checker sync so the shared sync tests exercise it
    # (the module default is off).
    monkeypatch.setenv("CONTAINER_CHECKER_SYNC_ENABLED", "true")

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

    # ----- Fake run_nsenter for host operations (patch on sidecar_common) -----
    mktemp_counter = {"n": 0}

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

        # /bin/mktemp <template-with-XXXXXX>
        if args[:1] == ["/bin/mktemp"] and len(args) == 2:
            template = args[1]
            mktemp_counter["n"] += 1
            unique = template.replace("XXXXXX", f"{mktemp_counter['n']:06d}")
            host_fs[unique] = b""
            out = unique + "\n"
            if text:
                return 0, out, ""
            return 0, out.encode(), b""

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

    # Reset the one-shot cleanup flag so each test starts fresh
    ss._stale_unit_cleaned = False
    monkeypatch.setattr(ss, "_STALE_UNIT_CLEANUP_ENABLED", True)

    # Isolate POST_COPY_ACTIONS
    monkeypatch.setattr(ss, "POST_COPY_ACTIONS", {}, raising=True)

    return ss, container_fs, host_fs, commands


def test_sync_no_change_fast_path(ss):
    ss, container_fs, host_fs, commands = ss
    item = ss.SyncItem("/container/gnmi.sh", "/host/gnmi.sh", 0o755)
    container_fs[item.src_in_container] = b"same"
    host_fs[item.dst_on_host] = b"same"
    ss.SYNC_ITEMS[:] = [item]
    # ensure_sync() also syncs the per-branch container_checker (202311 from fixture)
    container_fs["/usr/share/sonic/systemd_scripts/container_checker_202311"] = b"chk"
    host_fs["/bin/container_checker"] = b"chk"

    ok = ss.ensure_sync()
    assert ok is True
    # No write path used (no /bin/sh -c cat > tmp)
    assert not any(
        c[1][0] == "/bin/sh" and ("-c" in c[1] or "-lc" in c[1])
        for c in commands
    )


def test_sync_updates_and_post_actions(ss):
    ss, container_fs, host_fs, commands = ss
    # ensure_sync() appends the per-branch container_checker (202311 from fixture)
    container_checker_src = "/usr/share/sonic/systemd_scripts/container_checker_202311"
    container_fs[container_checker_src] = b"NEW"
    host_fs["/bin/container_checker"] = b"OLD"
    ss.SYNC_ITEMS[:] = []

    ss.POST_COPY_ACTIONS["/bin/container_checker"] = [
        ["sudo", "systemctl", "daemon-reload"],
        ["sudo", "systemctl", "restart", "monit"],
    ]

    ok = ss.ensure_sync()
    assert ok is True
    assert host_fs["/bin/container_checker"] == b"NEW"

    post_cmds = [args for _, args in commands if args and args[0] == "sudo"]
    assert ("sudo", "systemctl", "daemon-reload") in post_cmds
    assert ("sudo", "systemctl", "restart", "monit") in post_cmds


def test_sync_missing_src_returns_false(ss):
    ss, container_fs, host_fs, commands = ss
    # Provide a valid per-branch container_checker so the failure is solely the
    # missing gnmi.sh source, not the auto-appended checker.
    container_fs["/usr/share/sonic/systemd_scripts/container_checker_202311"] = b"chk"
    host_fs["/bin/container_checker"] = b"chk"
    item = ss.SyncItem("/container/missing.sh", "/usr/local/bin/gnmi.sh", 0o755)
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


def test_env_controls_gnmi_src_true(monkeypatch):
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    monkeypatch.setenv("IS_V1_ENABLED", "true")

    ss = importlib.import_module("systemd_stub")
    assert ss.IS_V1_ENABLED is True
    assert ss._GNMI_SRC.endswith("gnmi_v1.sh")


def test_env_controls_gnmi_src_false(monkeypatch):
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    monkeypatch.setenv("IS_V1_ENABLED", "false")

    ss = importlib.import_module("systemd_stub")
    assert ss.IS_V1_ENABLED is False
    assert ss._GNMI_SRC.endswith("gnmi.sh")


def test_env_controls_gnmi_src_default(monkeypatch):
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    monkeypatch.delenv("IS_V1_ENABLED", raising=False)

    ss = importlib.import_module("systemd_stub")
    assert ss.IS_V1_ENABLED is False
    assert ss._GNMI_SRC.endswith("gnmi.sh")


# ─────────────────────────── Tests for stale gnmi.service cleanup ───────────────────────────

STALE_UNIT = b"""[Unit]
Description=GNMI container

[Service]
User=root
ExecStartPre=/usr/local/bin/gnmi.sh start
ExecStart=/usr/local/bin/gnmi.sh wait
"""

CLEAN_UNIT = b"""[Unit]
Description=GNMI container

[Service]
User=admin
ExecStartPre=/usr/local/bin/gnmi.sh start
ExecStart=/usr/local/bin/gnmi.sh wait
"""


def test_cleanup_stale_unit_restores_from_packed_file(ss):
    """When host gnmi.service has User=root, cleanup overwrites it with the packed clean file."""
    ss_mod, container_fs, host_fs, commands = ss
    host_fs[ss_mod._HOST_GNMI_SERVICE] = STALE_UNIT
    container_fs[ss_mod._CONTAINER_GNMI_SERVICE] = CLEAN_UNIT

    ss_mod._cleanup_stale_service_unit()

    # Host file should now be the clean version
    assert host_fs[ss_mod._HOST_GNMI_SERVICE] == CLEAN_UNIT
    # daemon-reload and restart should follow
    post_cmds = [args for _, args in commands if args and args[0] == "sudo"]
    assert ("sudo", "systemctl", "daemon-reload") in post_cmds
    assert ("sudo", "systemctl", "restart", "gnmi") in post_cmds


def test_cleanup_skips_when_user_admin(ss):
    """When host gnmi.service already has User=admin, cleanup is a no-op."""
    ss_mod, container_fs, host_fs, commands = ss
    host_fs[ss_mod._HOST_GNMI_SERVICE] = CLEAN_UNIT

    ss_mod._cleanup_stale_service_unit()

    # No write should have occurred
    write_cmds = [args for _, args in commands if args and args[0] == "/bin/sh"]
    assert len(write_cmds) == 0


def test_cleanup_skips_when_file_missing(ss):
    """When host gnmi.service doesn't exist, cleanup is a no-op."""
    ss_mod, container_fs, host_fs, commands = ss
    # Don't put the file in host_fs

    ss_mod._cleanup_stale_service_unit()

    write_cmds = [args for _, args in commands if args and args[0] == "/bin/sh"]
    assert len(write_cmds) == 0


def test_cleanup_runs_only_once(ss):
    """The cleanup is a one-shot; second call should be a no-op."""
    ss_mod, container_fs, host_fs, commands = ss
    host_fs[ss_mod._HOST_GNMI_SERVICE] = STALE_UNIT
    container_fs[ss_mod._CONTAINER_GNMI_SERVICE] = CLEAN_UNIT

    ss_mod._cleanup_stale_service_unit()
    assert host_fs[ss_mod._HOST_GNMI_SERVICE] == CLEAN_UNIT

    # Revert host to stale to prove second call is a no-op
    host_fs[ss_mod._HOST_GNMI_SERVICE] = STALE_UNIT
    ss_mod._cleanup_stale_service_unit()
    # Should still be stale because the flag prevented re-run
    assert host_fs[ss_mod._HOST_GNMI_SERVICE] == STALE_UNIT


def test_cleanup_retries_after_transient_read_failure(ss):
    """When host_read_bytes fails transiently, cleanup retries on the next call."""
    ss_mod, container_fs, host_fs, commands = ss
    container_fs[ss_mod._CONTAINER_GNMI_SERVICE] = CLEAN_UNIT
    # First call: host file missing (transient failure)

    ss_mod._cleanup_stale_service_unit()
    assert ss_mod._stale_unit_cleaned is False  # flag NOT set; will retry

    # Second call: host file now present with stale content
    host_fs[ss_mod._HOST_GNMI_SERVICE] = STALE_UNIT
    ss_mod._cleanup_stale_service_unit()
    assert host_fs[ss_mod._HOST_GNMI_SERVICE] == CLEAN_UNIT
    assert ss_mod._stale_unit_cleaned is True


def test_cleanup_disabled_by_env(ss, monkeypatch):
    """When STALE_UNIT_CLEANUP_ENABLED=false, cleanup is skipped entirely."""
    ss_mod, container_fs, host_fs, commands = ss
    monkeypatch.setattr(ss_mod, "_STALE_UNIT_CLEANUP_ENABLED", False)
    host_fs[ss_mod._HOST_GNMI_SERVICE] = STALE_UNIT
    container_fs[ss_mod._CONTAINER_GNMI_SERVICE] = CLEAN_UNIT

    ss_mod._cleanup_stale_service_unit()
    # File should NOT be overwritten
    assert host_fs[ss_mod._HOST_GNMI_SERVICE] == STALE_UNIT
    # Flag set so it won't retry
    assert ss_mod._stale_unit_cleaned is True


# ─────────────────────────── Per-branch container_checker selection ───────────────────────────

def _mock_version(monkeypatch, tmp_path, version):
    """Make /etc/sonic/sonic_version.yml resolve to the given build_version."""
    version_file = tmp_path / "sonic_version.yml"
    version_file.write_text(f"build_version: '{version}'\n")

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


@pytest.mark.parametrize("version,expected_branch", [
    ("SONiC.20231110.19", "202311"),
    ("SONiC.20240510.25", "202405"),
    ("SONiC.20241110.22", "202411"),
    ("SONiC.20250510.04", "202505"),
    ("SONiC.20251110.01", "202511"),
    ("20231110.19", "202311"),          # Without SONiC. prefix
    ("20241110.kw.24", "202411"),       # Non-standard suffix
    ("SONiC.20231120.abc.123", "202311"),
])
def test_branch_detection_from_version(monkeypatch, tmp_path, version, expected_branch):
    """_get_branch_name() parses the SONiC version into a YYYYMM branch."""
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    _mock_version(monkeypatch, tmp_path, version)
    ss = importlib.import_module("systemd_stub")
    assert ss._get_branch_name() == expected_branch


@pytest.mark.parametrize("detected,resolved", [
    ("202311", "202311"),               # Exact match
    ("202505", "202505"),
    ("202511", "202511"),
    ("master", "202511"),               # Rolling branches -> latest supported
    ("internal", "202511"),
    ("private", "202511"),
    ("202406", "202405"),               # Between supported -> nearest lower
    ("202412", "202411"),
    ("202601", "202511"),               # Above newest -> newest
    ("202101", "202311"),               # Below oldest -> oldest
])
def test_resolve_branch(ss, detected, resolved):
    """_resolve_branch() maps any detected branch to a supported one."""
    ss_mod, _, _, _ = ss
    assert ss_mod._resolve_branch(detected) == resolved


@pytest.mark.parametrize("version,branch", [
    ("SONiC.20231110.19", "202311"),
    ("SONiC.20250510.04", "202505"),
    ("SONiC.20251110.01", "202511"),
])
def test_per_branch_container_checker_selected(monkeypatch, tmp_path, version, branch):
    """ensure_sync() syncs the container_checker matching the host OS branch."""
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    monkeypatch.setenv("CONTAINER_CHECKER_SYNC_ENABLED", "true")
    _mock_version(monkeypatch, tmp_path, version)

    host_fs = {}

    def fake_run_nsenter(args, *, text=True, input_bytes=None):
        if args[:1] == ["/bin/cat"] and len(args) == 2:
            path = args[1]
            if path in host_fs:
                out = host_fs[path]
                return (0, out.decode("utf-8", "ignore"), "") if text else (0, out, b"")
            return (1, "", "No such file") if text else (1, b"", b"No such file")
        if args[:1] == ["/bin/mkdir"]:
            return (0, "", "") if text else (0, b"", b"")
        if args[:1] == ["/bin/mktemp"] and len(args) == 2:
            tmp = args[1].replace("XXXXXX", "000001")
            host_fs[tmp] = b""
            return (0, tmp + "\n", "") if text else (0, (tmp + "\n").encode(), b"")
        if len(args) == 3 and args[0] == "/bin/sh" and args[1] in ("-c", "-lc") and args[2].strip().startswith("cat > "):
            tmp_path_arg = args[2].split("cat >", 1)[1].strip().strip("'\"")
            host_fs[tmp_path_arg] = input_bytes or b""
            return (0, "", "") if text else (0, b"", b"")
        if args[:1] == ["/bin/chmod"]:
            return (0, "", "") if text else (0, b"", b"")
        if args[:1] == ["/bin/mv"] and len(args) == 4:
            host_fs[args[3]] = host_fs.get(args[2], b"")
            host_fs.pop(args[2], None)
            return (0, "", "") if text else (0, b"", b"")
        if args[:1] == ["/bin/rm"]:
            host_fs.pop(args[-1], None)
            return (0, "", "") if text else (0, b"", b"")
        if args[:1] == ["sudo"]:
            return (0, "", "") if text else (0, b"", b"")
        return (1, "", "unsupported") if text else (1, b"", b"unsupported")

    monkeypatch.setattr(real_sidecar_common, "run_nsenter", fake_run_nsenter)

    container_fs = {
        f"/usr/share/sonic/systemd_scripts/container_checker_{branch}": b"CHECKER-" + branch.encode(),
    }
    monkeypatch.setattr(real_sidecar_common, "read_file_bytes_local",
                        lambda path: container_fs.get(path, None))

    ss = importlib.import_module("systemd_stub")
    ss._stale_unit_cleaned = True  # skip stale-unit cleanup
    monkeypatch.setattr(ss, "SYNC_ITEMS", [], raising=True)
    monkeypatch.setattr(ss, "POST_COPY_ACTIONS", {}, raising=True)

    ok = ss.ensure_sync()
    assert ok is True
    # The branch-specific checker landed on the host as /bin/container_checker
    assert host_fs["/bin/container_checker"] == b"CHECKER-" + branch.encode()


def test_ensure_sync_removes_native_container_when_not_v1(ss):
    """In V2 mode ensure_sync() removes any lingering native gnmi container each cycle."""
    ss_mod, container_fs, host_fs, commands = ss
    assert ss_mod.IS_V1_ENABLED is False
    # Provide an up-to-date per-branch checker so the sync itself is a no-op.
    container_fs["/usr/share/sonic/systemd_scripts/container_checker_202311"] = b"chk"
    host_fs["/bin/container_checker"] = b"chk"

    ss_mod.ensure_sync()

    docker_cmds = [args for _, args in commands if args[:2] == ("sudo", "docker")]
    assert ("sudo", "docker", "stop", "gnmi") in docker_cmds
    assert ("sudo", "docker", "rm", "--force", "gnmi") in docker_cmds


# ─────────────────────────── container_checker sync gate ───────────────────────────

def test_container_checker_sync_env_default_off(monkeypatch):
    """CONTAINER_CHECKER_SYNC_ENABLED defaults to False when unset."""
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    monkeypatch.delenv("CONTAINER_CHECKER_SYNC_ENABLED", raising=False)
    ss = importlib.import_module("systemd_stub")
    assert ss.CONTAINER_CHECKER_SYNC_ENABLED is False


def _run_ensure_sync_capture(monkeypatch, tmp_path, enabled):
    """Import systemd_stub with the gate set and return the host writes from one sync."""
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    monkeypatch.setenv("CONTAINER_CHECKER_SYNC_ENABLED", "true" if enabled else "false")
    _mock_version(monkeypatch, tmp_path, "SONiC.20231110.19")

    host_fs = {}

    def fake_run_nsenter(args, *, text=True, input_bytes=None):
        if args[:1] == ["/bin/cat"] and len(args) == 2:
            path = args[1]
            if path in host_fs:
                out = host_fs[path]
                return (0, out.decode("utf-8", "ignore"), "") if text else (0, out, b"")
            return (1, "", "No such file") if text else (1, b"", b"No such file")
        if args[:1] == ["/bin/mkdir"]:
            return (0, "", "") if text else (0, b"", b"")
        if args[:1] == ["/bin/mktemp"] and len(args) == 2:
            tmp = args[1].replace("XXXXXX", "000001")
            host_fs[tmp] = b""
            return (0, tmp + "\n", "") if text else (0, (tmp + "\n").encode(), b"")
        if len(args) == 3 and args[0] == "/bin/sh" and args[1] in ("-c", "-lc") and args[2].strip().startswith("cat > "):
            tmp_path_arg = args[2].split("cat >", 1)[1].strip().strip("'\"")
            host_fs[tmp_path_arg] = input_bytes or b""
            return (0, "", "") if text else (0, b"", b"")
        if args[:1] == ["/bin/chmod"]:
            return (0, "", "") if text else (0, b"", b"")
        if args[:1] == ["/bin/mv"] and len(args) == 4:
            host_fs[args[3]] = host_fs.get(args[2], b"")
            host_fs.pop(args[2], None)
            return (0, "", "") if text else (0, b"", b"")
        if args[:1] == ["/bin/rm"]:
            host_fs.pop(args[-1], None)
            return (0, "", "") if text else (0, b"", b"")
        if args[:1] == ["sudo"]:
            return (0, "", "") if text else (0, b"", b"")
        return (1, "", "unsupported") if text else (1, b"", b"unsupported")

    monkeypatch.setattr(real_sidecar_common, "run_nsenter", fake_run_nsenter)
    container_fs = {"/usr/share/sonic/systemd_scripts/container_checker_202311": b"CHECKER"}
    monkeypatch.setattr(real_sidecar_common, "read_file_bytes_local",
                        lambda path: container_fs.get(path, None))

    ss = importlib.import_module("systemd_stub")
    ss._stale_unit_cleaned = True
    monkeypatch.setattr(ss, "SYNC_ITEMS", [], raising=True)
    monkeypatch.setattr(ss, "POST_COPY_ACTIONS", {}, raising=True)

    ss.ensure_sync()
    return host_fs


def test_container_checker_synced_when_enabled(monkeypatch, tmp_path):
    """With the gate on, ensure_sync() writes /bin/container_checker."""
    host_fs = _run_ensure_sync_capture(monkeypatch, tmp_path, enabled=True)
    assert host_fs.get("/bin/container_checker") == b"CHECKER"


def test_container_checker_not_synced_when_disabled(monkeypatch, tmp_path):
    """With the gate off (default), ensure_sync() never touches /bin/container_checker."""
    host_fs = _run_ensure_sync_capture(monkeypatch, tmp_path, enabled=False)
    assert "/bin/container_checker" not in host_fs


# ─────────────────────────── CONFIG_DB client-auth reconcile ───────────────────────────

def test_reconcile_enables_user_auth_and_cname(ss, config_db):
    ss, container_fs, host_fs, commands = ss
    ss.GNMI_VERIFY_ENABLED = True
    ss.GNMI_CLIENT_CERTS = [{"cname": "fake-infra-ca.test.example.com", "role": ["gnmi_show_readonly"]}]
    assert config_db == {}

    ss.reconcile_config_db_once()

    assert config_db.get("GNMI|gnmi", {}).get("user_auth") == "cert"
    assert config_db.get("GNMI_CLIENT_CERT|fake-infra-ca.test.example.com", {}).get("role") == ["gnmi_show_readonly"]
    assert "TELEMETRY|gnmi" not in config_db


def test_reconcile_unset_leaves_config_db_untouched(ss, config_db):
    """Without a verify env var, existing user_auth and client certs are left alone."""
    ss, container_fs, host_fs, commands = ss
    ss.GNMI_VERIFY_ENABLED = None
    ss.GNMI_CLIENT_CERTS = [{"cname": "new.test.example.com", "role": ["admin"]}]
    config_db["GNMI|gnmi"] = {"user_auth": "cert"}
    config_db["GNMI_CLIENT_CERT|existing.test.example.com"] = {"role": ["admin"]}
    before = json.loads(json.dumps(config_db))

    ss.reconcile_config_db_once()

    assert config_db == before
    assert not any(args[:1] == ("sudo",) for _, args in commands)


def test_reconcile_disabled_removes_cname(ss, config_db):
    ss, container_fs, host_fs, commands = ss
    ss.GNMI_VERIFY_ENABLED = False
    config_db["GNMI_CLIENT_CERT|fake-infra-ca.test.example.com"] = {"role": "gnmi_show_readonly"}

    ss.reconcile_config_db_once()

    assert "GNMI_CLIENT_CERT|fake-infra-ca.test.example.com" not in config_db


def test_reconcile_disabled_removes_multiple_cnames(ss, config_db):
    """When verify=false, all GNMI_CLIENT_CERT entries in the table are removed."""
    ss, container_fs, host_fs, commands = ss
    ss.GNMI_VERIFY_ENABLED = False
    config_db["GNMI_CLIENT_CERT|a.test.example.com"] = {"role": "admin"}
    config_db["GNMI_CLIENT_CERT|b.test.example.com"] = {"role": "gnmi_show_readonly"}

    ss.reconcile_config_db_once()

    assert "GNMI_CLIENT_CERT|a.test.example.com" not in config_db
    assert "GNMI_CLIENT_CERT|b.test.example.com" not in config_db


def test_reconcile_disabled_removes_entries_not_in_env(ss, config_db):
    """When verify=false, entries NOT in GNMI_CLIENT_CERTS env are also removed."""
    ss, container_fs, host_fs, commands = ss
    ss.GNMI_VERIFY_ENABLED = False
    ss.GNMI_CLIENT_CERTS = []
    config_db["GNMI_CLIENT_CERT|unknown.example.com"] = {"role": "admin"}
    config_db["GNMI_CLIENT_CERT|other.example.com"] = {"role": "gnmi_show_readonly"}

    ss.reconcile_config_db_once()

    assert "GNMI_CLIENT_CERT|unknown.example.com" not in config_db
    assert "GNMI_CLIENT_CERT|other.example.com" not in config_db


def test_reconcile_disabled_clears_user_auth_and_restarts_gnmi(ss, config_db):
    """When verify=false and user_auth is set, the field is removed and gnmi restarted."""
    ss, container_fs, host_fs, commands = ss
    ss.GNMI_VERIFY_ENABLED = False
    config_db["GNMI|gnmi"] = {"user_auth": "cert", "port": "50052"}

    ss.reconcile_config_db_once()

    assert "user_auth" not in config_db.get("GNMI|gnmi", {})
    assert config_db["GNMI|gnmi"]["port"] == "50052"
    post_cmds = [args for _, args in commands if args and args[0] == "sudo"]
    assert ("sudo", "systemctl", "restart", "gnmi") in post_cmds


def test_reconcile_disabled_removes_user_auth_any_value(ss, config_db):
    """When verify=false, user_auth should be removed regardless of its value."""
    ss, container_fs, host_fs, commands = ss
    ss.GNMI_VERIFY_ENABLED = False
    config_db["GNMI|gnmi"] = {"user_auth": "password"}

    ss.reconcile_config_db_once()

    assert "user_auth" not in config_db.get("GNMI|gnmi", {})


def test_reconcile_disabled_no_restart_when_user_auth_absent(ss, config_db):
    ss, container_fs, host_fs, commands = ss
    ss.GNMI_VERIFY_ENABLED = False

    ss.reconcile_config_db_once()

    assert not any(args[:1] == ("sudo",) for _, args in commands)


def test_reconcile_multiple_cnames(ss, config_db):
    ss, container_fs, host_fs, commands = ss
    ss.GNMI_VERIFY_ENABLED = True
    ss.GNMI_CLIENT_CERTS = [
        {"cname": "fake-client.test.example.com", "role": ["admin"]},
        {"cname": "fake-server.test.example.com", "role": ["gnmi_show_readonly", "admin"]},
    ]

    ss.reconcile_config_db_once()

    assert config_db.get("GNMI|gnmi", {}).get("user_auth") == "cert"
    assert config_db.get("GNMI_CLIENT_CERT|fake-client.test.example.com", {}).get("role") == ["admin"]
    assert config_db.get("GNMI_CLIENT_CERT|fake-server.test.example.com", {}).get("role") == ["gnmi_show_readonly", "admin"]


def test_reconcile_rewrites_stale_json_string_role(ss, config_db):
    """Reconcile must rewrite a stale JSON-string role into a proper list for YANG compliance."""
    ss, container_fs, host_fs, commands = ss
    ss.GNMI_VERIFY_ENABLED = True
    ss.GNMI_CLIENT_CERTS = [{"cname": "fake-client.test.example.com", "role": ["admin"]}]
    config_db["GNMI_CLIENT_CERT|fake-client.test.example.com"] = {"role": '["admin"]'}

    ss.reconcile_config_db_once()

    assert config_db.get("GNMI_CLIENT_CERT|fake-client.test.example.com", {}).get("role") == ["admin"]


def test_reconcile_rewrites_plain_string_role(ss, config_db):
    """Reconcile must rewrite a plain-string role into a list for YANG leaf-list compliance."""
    ss, container_fs, host_fs, commands = ss
    ss.GNMI_VERIFY_ENABLED = True
    ss.GNMI_CLIENT_CERTS = [{"cname": "fake-client.test.example.com", "role": ["gnmi_show_readonly"]}]
    config_db["GNMI_CLIENT_CERT|fake-client.test.example.com"] = {"role": "gnmi_show_readonly"}

    ss.reconcile_config_db_once()

    assert config_db.get("GNMI_CLIENT_CERT|fake-client.test.example.com", {}).get("role") == ["gnmi_show_readonly"]


def test_reconcile_overwrites_when_role_differs(ss, config_db):
    ss, container_fs, host_fs, commands = ss
    ss.GNMI_VERIFY_ENABLED = True
    ss.GNMI_CLIENT_CERTS = [{"cname": "fake-client.test.example.com", "role": ["admin", "gnmi_show_readonly"]}]
    config_db["GNMI_CLIENT_CERT|fake-client.test.example.com"] = {"role": ["admin"]}

    ss.reconcile_config_db_once()

    assert config_db.get("GNMI_CLIENT_CERT|fake-client.test.example.com", {}).get("role") == ["admin", "gnmi_show_readonly"]


def test_reconcile_skips_when_role_matches(ss, config_db):
    ss, container_fs, host_fs, commands = ss
    ss.GNMI_VERIFY_ENABLED = True
    ss.GNMI_CLIENT_CERTS = [{"cname": "fake-client.test.example.com", "role": ["admin"]}]
    config_db["GNMI|gnmi"] = {"user_auth": "cert"}
    config_db["GNMI_CLIENT_CERT|fake-client.test.example.com"] = {"role": ["admin"]}

    ss.reconcile_config_db_once()

    assert config_db.get("GNMI_CLIENT_CERT|fake-client.test.example.com", {}).get("role") == ["admin"]
    assert not any(args[:4] == ("sudo", "-n", "config", "apply-patch") for _, args in commands)


def test_apply_config_patch_empty_is_noop(ss):
    ss_mod, container_fs, host_fs, commands = ss
    assert ss_mod._apply_config_patch([]) is True
    assert commands == []


def test_apply_config_patch_calls_nsenter(ss):
    ss_mod, container_fs, host_fs, commands = ss
    patch = [{"op": "add", "path": "/GNMI_CLIENT_CERT", "value": {"probe.example.com": {"role": ["admin"]}}}]

    assert ss_mod._apply_config_patch(patch) is True

    apply_cmds = [args for _, args in commands if args[:4] == ("sudo", "-n", "config", "apply-patch")]
    assert len(apply_cmds) == 1


def test_apply_config_patch_failure_returns_false(ss, monkeypatch):
    ss_mod, container_fs, host_fs, commands = ss
    monkeypatch.setattr(ss_mod, "run_nsenter", lambda *a, **kw: (1, b"", b"YANG validation failed"))

    assert ss_mod._apply_config_patch([{"op": "add", "path": "/GNMI/gnmi/user_auth", "value": "cert"}]) is False


def test_build_enabled_patch_table_absent(ss, config_db):
    """When GNMI_CLIENT_CERT table is absent, patch uses a single 'add' at table level."""
    ss_mod, container_fs, host_fs, commands = ss
    ss_mod.GNMI_VERIFY_ENABLED = True
    ss_mod.GNMI_CLIENT_CERTS = [
        {"cname": "a.test.example.com", "role": ["admin"]},
        {"cname": "b.test.example.com", "role": ["readonly"]},
    ]

    patch = ss_mod._build_enabled_patch()

    assert {"op": "add", "path": "/GNMI", "value": {"gnmi": {"user_auth": "cert"}}} in patch
    assert len(patch) == 2
    table_op = [p for p in patch if p["path"] == "/GNMI_CLIENT_CERT"]
    assert len(table_op) == 1
    assert table_op[0]["op"] == "add"
    assert set(table_op[0]["value"]) == {"a.test.example.com", "b.test.example.com"}


def test_build_enabled_patch_mixed_new_and_existing(ss, config_db):
    """When table exists, new entries get 'add' and stale entries get 'replace'."""
    ss_mod, container_fs, host_fs, commands = ss
    ss_mod.GNMI_VERIFY_ENABLED = True
    ss_mod.GNMI_CLIENT_CERTS = [
        {"cname": "existing.test.example.com", "role": ["admin", "readonly"]},
        {"cname": "new.test.example.com", "role": ["admin"]},
    ]
    config_db["GNMI_CLIENT_CERT|existing.test.example.com"] = {"role": ["admin"]}

    patch = ss_mod._build_enabled_patch()

    cert_ops = [p for p in patch if p["path"].startswith("/GNMI_CLIENT_CERT/")]
    assert len(cert_ops) == 2
    replace_op = [p for p in cert_ops if p["op"] == "replace"]
    assert [p["path"] for p in replace_op] == ["/GNMI_CLIENT_CERT/existing.test.example.com"]
    add_op = [p for p in cert_ops if p["op"] == "add"]
    assert [p["path"] for p in add_op] == ["/GNMI_CLIENT_CERT/new.test.example.com"]


def test_build_enabled_patch_all_up_to_date(ss, config_db):
    ss_mod, container_fs, host_fs, commands = ss
    ss_mod.GNMI_VERIFY_ENABLED = True
    ss_mod.GNMI_CLIENT_CERTS = [{"cname": "ok.test.example.com", "role": ["admin"]}]
    config_db["GNMI|gnmi"] = {"user_auth": "cert", "port": "50052"}
    config_db["GNMI_CLIENT_CERT|ok.test.example.com"] = {"role": ["admin"]}

    assert ss_mod._build_enabled_patch() == []


def test_build_enabled_patch_ignores_telemetry_table(ss, config_db):
    """user_auth in the TELEMETRY table does not satisfy the GNMI table."""
    ss_mod, container_fs, host_fs, commands = ss
    ss_mod.GNMI_VERIFY_ENABLED = True
    ss_mod.GNMI_CLIENT_CERTS = []
    config_db["TELEMETRY|gnmi"] = {"user_auth": "cert"}

    assert ss_mod._build_enabled_patch() == [{"op": "add", "path": "/GNMI", "value": {"gnmi": {"user_auth": "cert"}}}]


_CERTS = [
    {"cname": "a.test.example.com", "role": ["admin"]},
    {"cname": "b.test.example.com", "role": ["gnmi_show_readonly"]},
]

# (seed CONFIG_DB, expected user_auth op) for each GNMI|gnmi shape
_GNMI_SHAPES = {
    "gnmi_table_absent": (
        {},
        {"op": "add", "path": "/GNMI", "value": {"gnmi": {"user_auth": "cert"}}},
    ),
    "gnmi_key_absent": (
        {"GNMI|certs": {"server_crt": "/etc/sonic/telemetry/server.crt"}},
        {"op": "add", "path": "/GNMI/gnmi", "value": {"user_auth": "cert"}},
    ),
    "user_auth_absent": (
        {"GNMI|gnmi": {"port": "50052"}},
        {"op": "add", "path": "/GNMI/gnmi/user_auth", "value": "cert"},
    ),
    "user_auth_other": (
        {"GNMI|gnmi": {"port": "50052", "user_auth": "password"}},
        {"op": "replace", "path": "/GNMI/gnmi/user_auth", "value": "cert"},
    ),
}

# (seed CONFIG_DB, expected cert ops) for each GNMI_CLIENT_CERT shape
_CERT_SHAPES = {
    "cert_table_absent": (
        {},
        [{"op": "add", "path": "/GNMI_CLIENT_CERT",
          "value": {"a.test.example.com": {"role": ["admin"]},
                    "b.test.example.com": {"role": ["gnmi_show_readonly"]}}}],
    ),
    "cert_table_other_entry": (
        {"GNMI_CLIENT_CERT|other.test.example.com": {"role": ["admin"]}},
        [{"op": "add", "path": "/GNMI_CLIENT_CERT/a.test.example.com", "value": {"role": ["admin"]}},
         {"op": "add", "path": "/GNMI_CLIENT_CERT/b.test.example.com", "value": {"role": ["gnmi_show_readonly"]}}],
    ),
    "cert_entry_stale": (
        {"GNMI_CLIENT_CERT|a.test.example.com": {"role": ["readonly"]},
         "GNMI_CLIENT_CERT|b.test.example.com": {"role": ["gnmi_show_readonly"]}},
        [{"op": "replace", "path": "/GNMI_CLIENT_CERT/a.test.example.com", "value": {"role": ["admin"]}}],
    ),
}


def _shape_case(gnmi_shape, cert_shape):
    gnmi_seed, gnmi_op = _GNMI_SHAPES[gnmi_shape]
    cert_seed, cert_ops = _CERT_SHAPES[cert_shape]
    return {**gnmi_seed, **cert_seed}, [gnmi_op] + cert_ops


@pytest.mark.parametrize("gnmi_shape", sorted(_GNMI_SHAPES))
@pytest.mark.parametrize("cert_shape", sorted(_CERT_SHAPES))
def test_build_enabled_patch_shapes(ss, config_db, gnmi_shape, cert_shape):
    """The patch is minimal and valid for every combination of existing GNMI/GNMI_CLIENT_CERT state."""
    ss_mod, container_fs, host_fs, commands = ss
    ss_mod.GNMI_VERIFY_ENABLED = True
    ss_mod.GNMI_CLIENT_CERTS = _CERTS
    seed, expected = _shape_case(gnmi_shape, cert_shape)
    config_db.update(json.loads(json.dumps(seed)))

    patch = ss_mod._build_enabled_patch()
    assert patch == expected
    assert not any(op["value"] == {} for op in patch)

    ss_mod.reconcile_config_db_once()

    assert config_db["GNMI|gnmi"]["user_auth"] == "cert"
    for key, fields in seed.items():
        if key.startswith("GNMI|"):
            for field, value in fields.items():
                if field != "user_auth":
                    assert config_db[key][field] == value
    for entry in _CERTS:
        assert config_db[f"GNMI_CLIENT_CERT|{entry['cname']}"]["role"] == entry["role"]
    # Converged: a second pass has nothing to do.
    assert ss_mod._build_enabled_patch() == []


@pytest.mark.parametrize("gnmi_shape", sorted(_GNMI_SHAPES))
@pytest.mark.parametrize("cert_shape", sorted(_CERT_SHAPES))
def test_build_enabled_patch_shapes_apply_with_jsonpatch(ss, config_db, gnmi_shape, cert_shape):
    """Cross-check the patch with the jsonpatch library used by 'config apply-patch'."""
    jsonpatch = pytest.importorskip("jsonpatch")
    ss_mod, container_fs, host_fs, commands = ss
    ss_mod.GNMI_CLIENT_CERTS = _CERTS
    seed, _ = _shape_case(gnmi_shape, cert_shape)
    config_db.update(json.loads(json.dumps(seed)))

    result = jsonpatch.JsonPatch(ss_mod._build_enabled_patch()).apply(_to_nested(config_db))

    assert result["GNMI"]["gnmi"]["user_auth"] == "cert"
    for entry in _CERTS:
        assert result["GNMI_CLIENT_CERT"][entry["cname"]]["role"] == entry["role"]


def test_strict_apply_rejects_leaf_under_missing_parent():
    """Guard for the fake itself: the pre-fix patch shape must fail like the real apply-patch."""
    with pytest.raises(KeyError):
        _strict_apply({}, [{"op": "add", "path": "/GNMI/gnmi/user_auth", "value": "cert"}])


def test_main_once_reconciles_before_sync(monkeypatch):
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    ss = importlib.import_module("systemd_stub")
    calls = []
    monkeypatch.setattr(ss, "reconcile_config_db_once", lambda: calls.append("reconcile"), raising=True)
    monkeypatch.setattr(ss, "ensure_sync", lambda: calls.append("sync") or True, raising=True)
    monkeypatch.setattr(sys, "argv", ["systemd_stub.py", "--once"])

    assert ss.main() == 0
    assert calls == ["reconcile", "sync"]


# ─────────────────────────── Client-auth env parsing ───────────────────────────

_CLIENT_AUTH_ENV_VARS = (
    "GNMI_CLIENT_CERT_VERIFY_ENABLED", "TELEMETRY_CLIENT_CERT_VERIFY_ENABLED",
    "GNMI_CLIENT_CERTS", "GNMI_CLIENT_CNAME", "TELEMETRY_CLIENT_CNAME", "GNMI_CLIENT_ROLE",
)


def _import_with_env(monkeypatch, env_vars):
    for k in _CLIENT_AUTH_ENV_VARS:
        monkeypatch.delenv(k, raising=False)
    for k, v in env_vars.items():
        monkeypatch.setenv(k, v)
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    return importlib.import_module("systemd_stub")


@pytest.mark.parametrize("env,expected", [
    ({}, None),
    ({"GNMI_CLIENT_CERT_VERIFY_ENABLED": "true"}, True),
    ({"GNMI_CLIENT_CERT_VERIFY_ENABLED": "false"}, False),
    ({"TELEMETRY_CLIENT_CERT_VERIFY_ENABLED": "true"}, True),
    ({"TELEMETRY_CLIENT_CERT_VERIFY_ENABLED": "false"}, False),
    ({"GNMI_CLIENT_CERT_VERIFY_ENABLED": "false", "TELEMETRY_CLIENT_CERT_VERIFY_ENABLED": "true"}, False),
    ({"GNMI_CLIENT_CERT_VERIFY_ENABLED": "true", "TELEMETRY_CLIENT_CERT_VERIFY_ENABLED": "false"}, True),
])
def test_verify_enabled_env(monkeypatch, env, expected):
    assert _import_with_env(monkeypatch, env).GNMI_VERIFY_ENABLED is expected


class TestParseClientCerts:
    """Tests for _parse_client_certs() env-var parsing."""

    @pytest.fixture(autouse=True)
    def _env(self, monkeypatch):
        self.monkeypatch = monkeypatch

    def _certs(self, env_vars):
        return _import_with_env(self.monkeypatch, env_vars).GNMI_CLIENT_CERTS

    def test_valid_json_array(self):
        certs = self._certs({"GNMI_CLIENT_CERTS": '[{"cname": "client.gbl", "role": "admin"}]'})
        assert certs == [{"cname": "client.gbl", "role": ["admin"]}]

    def test_valid_json_multiple_entries(self):
        certs = self._certs({
            "GNMI_CLIENT_CERTS": '[{"cname": "a.gbl", "role": "admin"}, {"cname": "b.gbl", "role": "readonly"}]'
        })
        assert certs == [{"cname": "a.gbl", "role": ["admin"]}, {"cname": "b.gbl", "role": ["readonly"]}]

    def test_role_as_json_list(self):
        certs = self._certs({"GNMI_CLIENT_CERTS": '[{"cname": "s.gbl", "role": ["gnmi_show_readonly", "admin"]}]'})
        assert certs == [{"cname": "s.gbl", "role": ["gnmi_show_readonly", "admin"]}]

    def test_non_array_json_falls_back_to_legacy(self):
        certs = self._certs({
            "GNMI_CLIENT_CERTS": '{"cname": "c.gbl", "role": "admin"}',
            "GNMI_CLIENT_CNAME": "legacy.gbl",
            "GNMI_CLIENT_ROLE": "readonly",
        })
        assert certs == [{"cname": "legacy.gbl", "role": ["readonly"]}]

    def test_invalid_json_falls_back_to_legacy(self):
        certs = self._certs({"GNMI_CLIENT_CERTS": "not-json!", "GNMI_CLIENT_CNAME": "fallback.gbl"})
        assert certs == [{"cname": "fallback.gbl", "role": ["gnmi_show_readonly"]}]

    def test_entry_not_dict_falls_back(self):
        certs = self._certs({"GNMI_CLIENT_CERTS": '["not-a-dict"]', "GNMI_CLIENT_CNAME": "fb.gbl"})
        assert certs == [{"cname": "fb.gbl", "role": ["gnmi_show_readonly"]}]

    def test_entry_missing_role_falls_back(self):
        certs = self._certs({"GNMI_CLIENT_CERTS": '[{"cname": "x.gbl"}]', "GNMI_CLIENT_CNAME": "fb.gbl"})
        assert certs == [{"cname": "fb.gbl", "role": ["gnmi_show_readonly"]}]

    def test_entry_empty_cname_falls_back(self):
        certs = self._certs({"GNMI_CLIENT_CERTS": '[{"cname": "  ", "role": "admin"}]', "GNMI_CLIENT_CNAME": "fb.gbl"})
        assert certs == [{"cname": "fb.gbl", "role": ["gnmi_show_readonly"]}]

    def test_legacy_single_entry(self):
        certs = self._certs({"GNMI_CLIENT_CNAME": "legacy.gbl", "GNMI_CLIENT_ROLE": "admin"})
        assert certs == [{"cname": "legacy.gbl", "role": ["admin"]}]

    def test_legacy_default_role(self):
        assert self._certs({"GNMI_CLIENT_CNAME": "legacy.gbl"}) == [{"cname": "legacy.gbl", "role": ["gnmi_show_readonly"]}]

    def test_telemetry_cname_fallback(self):
        certs = self._certs({"TELEMETRY_CLIENT_CNAME": "tlm.gbl", "GNMI_CLIENT_ROLE": "admin"})
        assert certs == [{"cname": "tlm.gbl", "role": ["admin"]}]

    def test_gnmi_cname_preferred_over_telemetry_cname(self):
        certs = self._certs({"GNMI_CLIENT_CNAME": "gnmi.gbl", "TELEMETRY_CLIENT_CNAME": "tlm.gbl"})
        assert certs == [{"cname": "gnmi.gbl", "role": ["gnmi_show_readonly"]}]

    def test_no_env_returns_empty(self):
        assert self._certs({}) == []

    def test_whitespace_stripped(self):
        certs = self._certs({"GNMI_CLIENT_CERTS": '[{"cname": " client.gbl ", "role": " admin "}]'})
        assert certs == [{"cname": "client.gbl", "role": ["admin"]}]
