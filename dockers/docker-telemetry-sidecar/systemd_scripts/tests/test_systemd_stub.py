# tests/test_systemd_stub.py
import sys
import types
import importlib

import pytest


@pytest.fixture(scope="session", autouse=True)
def fake_logger_module():
    """
    Provide fakes for:
      - sonic_py_common.logger.Logger
      - swsscommon.swsscommon.ConfigDBConnector

    so the module under test can import and use them without real SONiC deps.
    """
    # ----- fake sonic_py_common.logger -----
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

    # ----- fake swsscommon.swsscommon.ConfigDBConnector (import-time only) -----
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

    yield


@pytest.fixture
def ss(tmp_path, monkeypatch):
    """
    Import systemd_stub fresh for every test, and provide fakes:

      - run_nsenter: simulates host FS + systemctl/docker calls
      - container_fs: dict for "container" files
      - host_fs: dict for "host" files
      - config_db: dict for CONFIG_DB contents ("TABLE|KEY" -> {field: value})
      - ConfigDBConnector: replaced with a fake that reads/writes config_db
    """
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    ss = importlib.import_module("systemd_stub")

    # Fake host filesystem and command recorder
    host_fs = {}
    commands = []

    # Fake CONFIG_DB (redis key "TABLE|KEY" -> dict(field -> value))
    config_db = {}

    # ----- Fake ConfigDBConnector used by systemd_stub -----
    class FakeConfigDBConnector:
        def __init__(self, *_, **__):
            pass

        def connect(self, *_, **__):
            # nothing to do
            pass

        def get_entry(self, table, key):
            redis_key = f"{table}|{key}"
            # Return a copy to mimic real behavior (avoid accidental in-place mutation)
            entry = config_db.get(redis_key, {})
            return dict(entry)

        def set_entry(self, table, key, entry):
            redis_key = f"{table}|{key}"
            # In ConfigDBConnector semantics, empty dict deletes the entry
            if not entry:
                config_db.pop(redis_key, None)
            else:
                config_db[redis_key] = dict(entry)

    monkeypatch.setattr(ss, "ConfigDBConnector", FakeConfigDBConnector, raising=True)

    # ----- Fake run_nsenter for host operations -----
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

    monkeypatch.setattr(ss, "run_nsenter", fake_run_nsenter, raising=True)

    # Fake container FS
    container_fs = {}

    def fake_read_file_bytes_local(path: str):
        return container_fs.get(path, None)

    monkeypatch.setattr(ss, "read_file_bytes_local", fake_read_file_bytes_local, raising=True)

    # Isolate POST_COPY_ACTIONS
    monkeypatch.setattr(ss, "POST_COPY_ACTIONS", {}, raising=True)

    return ss, container_fs, host_fs, commands, config_db


def test_sha256_bytes_basic():
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    ss = importlib.import_module("systemd_stub")
    assert ss.sha256_bytes(b"") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    assert ss.sha256_bytes(None) == ""
    assert ss.sha256_bytes(b"abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"


def test_host_write_atomic_and_read(ss):
    ss, container_fs, host_fs, commands, config_db = ss
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
