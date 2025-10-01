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
      - run_nsenter: simulates a host FS + systemctl/docker/stat/chown/chmod calls
      - container_fs: dict for "container" files (path -> bytes)
      - host_fs: dict for "host" file contents (path -> bytes)
      - host_meta: dict for "host" file metadata (path -> {"owner","group","mode"})
      - commands: list of all nsenter command tuples for assertion
    """
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    ss = importlib.import_module("systemd_stub")

    # Fake host filesystem and metadata and command recorder
    host_fs = {}
    host_meta = {}
    commands = []

    # Seed kubeconfig + client paths so ownership enforcement can work
    kubeconf = "/etc/kubernetes/kubelet.conf"
    client = "/var/lib/kubelet/pki/kubelet-client-current.pem"
    host_fs[kubeconf] = (
        b"apiVersion: v1\n"
        b"clusters: []\n"
        b"users:\n"
        b"- name: kubelet\n"
        b"  user:\n"
        b"    client-certificate: " + client.encode() + b"\n"
        b"    client-key: " + client.encode() + b"\n"
    )
    host_meta[kubeconf] = {"owner": "root", "group": "root", "mode": 0o600}
    host_fs[client] = b"FAKE-KEY"
    host_meta[client] = {"owner": "root", "group": "root", "mode": 0o600}

    # Fake run_nsenter
    def fake_run_nsenter(args, *, text=True, input_bytes=None):
        commands.append(("nsenter", tuple(args)))

        def _ok(out=b"", err=b""):
            if text:
                return 0, (out.decode("utf-8", "ignore") if isinstance(out, (bytes, bytearray)) else out), (err.decode("utf-8", "ignore") if isinstance(err, (bytes, bytearray)) else err)
            return 0, out, err

        def _fail(msg="unsupported"):
            return 1, "" if text else b"", msg if text else msg.encode()

        # /bin/cat <path>
        if args[:1] == ["/bin/cat"] and len(args) == 2:
            path = args[1]
            if path in host_fs:
                out = host_fs[path]
                return _ok(out)
            return _fail("No such file")

        # /usr/bin/stat -c "%U %G %a" <path>
        if args[:2] == ["/usr/bin/stat", "-c"] and len(args) == 4:
            fmt, path = args[2], args[3]
            if path not in host_meta:
                # Treat as missing -> fail
                return _fail("stat: No such file")
            meta = host_meta[path]
            if fmt == "%U %G %a":
                out = f"{meta['owner']} {meta['group']} {meta['mode']:o}\n"
                return _ok(out)
            return _fail("unsupported stat format")

        # /bin/chown owner:group <path>
        if args[:1] == ["/bin/chown"] and len(args) == 3:
            owngrp, path = args[1], args[2]
            if path not in host_meta:
                host_meta[path] = {"owner": "root", "group": "root", "mode": 0o600}
            owner, group = owngrp.split(":", 1)
            host_meta[path]["owner"] = owner
            host_meta[path]["group"] = group
            return _ok()

        # /bin/chmod <mode> <path>
        if args[:1] == ["/bin/chmod"] and len(args) == 3:
            mode_str, path = args[1], args[2]
            if path not in host_meta:
                host_meta[path] = {"owner": "root", "group": "root", "mode": 0o600}
            host_meta[path]["mode"] = int(mode_str, 8)
            return _ok()

        # /bin/mkdir -p <dir>
        if args[:1] == ["/bin/mkdir"]:
            return _ok()

        # /bin/sh -lc "cat > /tmp/xxx"
        if args[:2] == ["/bin/sh", "-lc"] and len(args) == 3 and args[2].startswith("cat > "):
            tmp_path = args[2].split("cat > ", 1)[1].strip()
            host_fs[tmp_path] = input_bytes or (b"" if text else b"")
            # track meta for tmp file
            host_meta.setdefault(tmp_path, {"owner": "root", "group": "root", "mode": 0o600})
            return _ok()

        # /bin/mv -f <src> <dst>
        if args[:1] == ["/bin/mv"] and len(args) == 4:
            src, dst = args[2], args[3]
            host_fs[dst] = host_fs.get(src, b"")
            # move meta if present
            if src in host_meta:
                host_meta[dst] = dict(host_meta[src])
            host_fs.pop(src, None)
            host_meta.pop(src, None)
            return _ok()

        # /bin/rm -f <path>
        if args[:1] == ["/bin/rm"]:
            target = args[-1]
            host_fs.pop(target, None)
            host_meta.pop(target, None)
            return _ok()

        # /usr/bin/docker ... (stop/rm)
        if args[:1] == ["/usr/bin/docker"]:
            return _ok()

        # /bin/systemctl ... (daemon-reload/restart)
        if args[:1] == ["/bin/systemctl"]:
            return _ok()

        # tolerate "sudo ..." if any test still injects it
        if args[:1] == ["sudo"]:
            return _ok()

        return _fail()

    monkeypatch.setattr(ss, "run_nsenter", fake_run_nsenter, raising=True)

    # Fake container FS
    container_fs = {}
    def fake_read_file_bytes_local(path: str):
        return container_fs.get(path, None)
    monkeypatch.setattr(ss, "read_file_bytes_local", fake_read_file_bytes_local, raising=True)

    # Isolate POST_COPY_ACTIONS
    monkeypatch.setattr(ss, "POST_COPY_ACTIONS", {}, raising=True)

    # Expose handy internals to tests
    return ss, container_fs, host_fs, host_meta, commands


def test_sha256_bytes_basic():
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    ss = importlib.import_module("systemd_stub")
    assert ss.sha256_bytes(b"") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    assert ss.sha256_bytes(None) == ""
    assert ss.sha256_bytes(b"abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"


def test_host_write_atomic_and_read(ss):
    ss, container_fs, host_fs, host_meta, commands = ss
    ok = ss.host_write_atomic("/etc/testfile", b"hello", 0o755)
    assert ok
    data = ss.host_read_bytes("/etc/testfile")
    assert data == b"hello"
    cmd_names = [c[1][0] for c in commands]
    assert "/bin/sh" in cmd_names
    assert "/bin/chmod" in cmd_names
    assert "/bin/mkdir" in cmd_names
    assert "/bin/mv" in cmd_names


def test_sync_no_change_fast_path(ss, monkeypatch):
    ss, container_fs, host_fs, host_meta, commands = ss
    # Avoid enforcement side-effects in this "fast path" test
    monkeypatch.setattr(ss, "enforce_kube_credentials_owner_periodic", lambda: True, raising=True)

    item = ss.SyncItem("/container/telemetry.sh", "/host/telemetry.sh", 0o755)
    container_fs[item.src_in_container] = b"same"
    host_fs[item.dst_on_host] = b"same"
    ss.SYNC_ITEMS[:] = [item]

    ok = ss.ensure_sync()
    assert ok is True

    # No copy path (no /bin/sh -lc "cat > ...")
    assert not any(c[1][:2] == ("/bin/sh", "-lc") and "cat > " in c[1][2] for c in commands)


def test_sync_updates_and_post_actions(ss, monkeypatch):
    ss, container_fs, host_fs, host_meta, commands = ss
    # Avoid ownership enforcement noise in this test
    monkeypatch.setattr(ss, "enforce_kube_credentials_owner_periodic", lambda: True, raising=True)

    item = ss.SyncItem("/container/container_checker", "/bin/container_checker", 0o755)
    container_fs[item.src_in_container] = b"NEW"
    host_fs[item.dst_on_host] = b"OLD"
    ss.SYNC_ITEMS[:] = [item]

    ss.POST_COPY_ACTIONS[item.dst_on_host] = [
        ["/bin/systemctl", "daemon-reload"],
        ["/bin/systemctl", "restart", "monit"],
    ]

    ok = ss.ensure_sync()
    assert ok is True
    assert host_fs[item.dst_on_host] == b"NEW"

    post_cmds = [args for _, args in commands if args and args[0] == "/bin/systemctl"]
    assert ("/bin/systemctl", "daemon-reload") in post_cmds
    assert ("/bin/systemctl", "restart", "monit") in post_cmds


def test_sync_missing_src_returns_false(ss, monkeypatch):
    ss, container_fs, host_fs, host_meta, commands = ss
    # Avoid ownership enforcement noise in this test
    monkeypatch.setattr(ss, "enforce_kube_credentials_owner_periodic", lambda: True, raising=True)

    item = ss.SyncItem("/container/missing.sh", "/usr/local/bin/telemetry.sh", 0o755)
    ss.SYNC_ITEMS[:] = [item]
    ok = ss.ensure_sync()
    assert ok is False


def test_main_once_exits_zero_and_disables_post_actions(monkeypatch):
    if "systemd_stub" in sys.modules:
        del sys.modules["systemd_stub"]
    ss = importlib.import_module("systemd_stub")

    ss.POST_COPY_ACTIONS["/bin/container_checker"] = [["/bin/echo", "hi"]]
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


def test_enforce_kube_owner_mode_drift(ss):
    """
    New test: ensure periodic enforcement detects drift and fixes owner/mode
    for kubeconfig + client credential.
    """
    ss, container_fs, host_fs, host_meta, commands = ss
    kc = "/etc/kubernetes/kubelet.conf"
    client = "/var/lib/kubelet/pki/kubelet-client-current.pem"

    # Drift: reset both to root:root 600
    host_meta[kc] = {"owner": "root", "group": "root", "mode": 0o600}
    host_meta[client] = {"owner": "root", "group": "root", "mode": 0o600}

    # Call the periodic enforcement directly
    ok = ss.enforce_kube_credentials_owner_periodic()
    assert ok is True

    # Should now be admin:admin 600
    assert host_meta[kc]["owner"] == "admin"
    assert host_meta[kc]["group"] == "admin"
    assert host_meta[kc]["mode"] == 0o600

    assert host_meta[client]["owner"] == "admin"
    assert host_meta[client]["group"] == "admin"
    assert host_meta[client]["mode"] == 0o600

    # And we should have seen chown/chmod/stat calls
    called = [" ".join(a) for _, a in commands]
    assert any(cmd.startswith("/usr/bin/stat -c") and cmd.endswith(kc) for cmd in called)
    assert any(cmd.startswith("/bin/chown admin:admin ") and cmd.endswith(kc) for cmd in called)
    assert any(cmd.startswith("/bin/chmod 600 ") and cmd.endswith(kc) for cmd in called)
