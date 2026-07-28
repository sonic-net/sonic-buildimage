"""
Unit tests for the BMC watchdog IPC layer.

These exercise the hw-watchdog-mgrd daemon's IPC server (with the hardware
device ioctls mocked) driven through the real sonic_platform Watchdog IPC
client, validating arm/disarm/is_armed/get_remaining_time end to end as well as
the JSON intent-file persistence and startup-adoption logic.
"""

import importlib.util
import json
import os
import select
import sys
import threading
import types

import pytest

TEST_DIR = os.path.dirname(os.path.abspath(__file__))
NEXTHOP_DIR = os.path.dirname(TEST_DIR)
ASPEED_DIR = os.path.dirname(NEXTHOP_DIR)
CLIENT_PATH = os.path.join(
    NEXTHOP_DIR, "common", "sonic_platform", "watchdog.py")
DAEMON_PATH = os.path.join(
    ASPEED_DIR, "aspeed-platform-services", "scripts", "hw-watchdog-mgrd.py")

# sonic_platform_base is not available in the unit-test environment; stub the
# only piece the watchdog client needs.
if "sonic_platform_base" not in sys.modules:
    base = types.ModuleType("sonic_platform_base")
    wb = types.ModuleType("sonic_platform_base.watchdog_base")

    class WatchdogBase:
        pass

    wb.WatchdogBase = WatchdogBase
    base.watchdog_base = wb
    sys.modules["sonic_platform_base"] = base
    sys.modules["sonic_platform_base.watchdog_base"] = wb


def _load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


wdtd = _load("hw_watchdog_mgrd", DAEMON_PATH)
wdtc = _load("watchdog_client", CLIENT_PATH)


@pytest.fixture
def ipc(tmp_path):
    """Start the daemon IPC server with mocked device ioctls, yield a client."""
    sock_path = str(tmp_path / "wdt.sock")
    wdtd.SOCKET_PATH = sock_path
    # Keep intent-file writes out of /run during tests.
    wdtd.INTENT_FILE = str(tmp_path / "intent.json")

    # In-memory model of the hardware watchdog state.
    hw = {"armed": False, "timeout": 0}

    def fake_ioctl(fd, op, arg=None, mutate=True):
        if op == wdtd.WDIOC_SETTIMEOUT:
            hw["timeout"] = int(arg[0])
        elif op == wdtd.WDIOC_SETOPTIONS:
            opt = int(arg[0])
            if opt == wdtd.WDIOS_ENABLECARD:
                hw["armed"] = True
            elif opt == wdtd.WDIOS_DISABLECARD:
                hw["armed"] = False
        elif op == wdtd.WDIOC_KEEPALIVE:
            pass
        return 0

    daemon = wdtd.WatchdogManager()
    daemon.fd = 999
    daemon._open_device = lambda: True
    daemon._read_sysfs_str = lambda f: "active" if hw["armed"] else "inactive"
    orig_ioctl = wdtd.fcntl.ioctl
    wdtd.fcntl.ioctl = fake_ioctl

    server = daemon._create_socket()
    stop = threading.Event()

    def serve():
        while not stop.is_set():
            readable, _, _ = select.select([server], [], [], 0.1)
            for s in readable:
                daemon._handle_connection(s)

    thread = threading.Thread(target=serve, daemon=True)
    thread.start()

    client = wdtc.Watchdog()
    client.socket_path = sock_path

    yield client, hw

    stop.set()
    thread.join(timeout=2)
    server.close()
    wdtd.fcntl.ioctl = orig_ioctl


def test_arm_disarm_status(ipc):
    client, hw = ipc
    assert client.is_armed() is False
    assert client.arm(120) == 120
    assert hw["armed"] is True
    assert hw["timeout"] == 120
    assert client.is_armed() is True
    remaining = client.get_remaining_time()
    assert 0 <= remaining <= 120
    assert client.disarm() is True
    assert hw["armed"] is False
    assert client.is_armed() is False
    assert client.get_remaining_time() == -1


def test_arm_rejects_out_of_range(ipc):
    client, hw = ipc
    assert client.arm(-1) == -1
    # 0 means "unknown" to the kernel (not "disable") and tiny timeouts risk an
    # almost-immediate reset, so anything below MIN_TIMEOUT is rejected.
    assert client.arm(0) == -1
    assert client.arm(wdtd.MIN_TIMEOUT - 1) == -1
    assert client.arm(wdtd.MAX_TIMEOUT + 1) == -1
    assert hw["armed"] is False


def test_arm_accepts_minimum_timeout(ipc):
    client, hw = ipc
    assert client.arm(wdtd.MIN_TIMEOUT) == wdtd.MIN_TIMEOUT
    assert hw["armed"] is True
    assert hw["timeout"] == wdtd.MIN_TIMEOUT


def test_rearm_keeps_armed(ipc):
    client, hw = ipc
    assert client.arm(120) == 120
    # Re-arming at the same timeout should ping, leaving it armed.
    assert client.arm(120) == 120
    assert client.is_armed() is True


def test_unknown_command(ipc):
    client, _ = ipc
    resp = client._request("bogus")
    assert resp is not None and "error" in resp


def test_client_handles_daemon_unavailable(tmp_path):
    client = wdtc.Watchdog()
    client.socket_path = str(tmp_path / "nonexistent.sock")
    # Point the sysfs fallback at a path that does not exist so the test is
    # hermetic regardless of any real watchdog device on the host.
    wdtc.WATCHDOG_SYSFS_PATH = str(tmp_path / "no-sysfs") + "/"
    assert client.arm(60) == -1
    assert client.disarm() is False
    assert client.is_armed() is False
    assert client.get_remaining_time() == -1


def test_is_armed_falls_back_to_sysfs_when_daemon_down(tmp_path):
    # Simulates a daemon crash: the socket is gone but the hardware watchdog is
    # still armed (counting down).  is_armed() must report the true hw state.
    client = wdtc.Watchdog()
    client.socket_path = str(tmp_path / "nonexistent.sock")
    sysfs = tmp_path / "sysfs"
    sysfs.mkdir()
    wdtc.WATCHDOG_SYSFS_PATH = str(sysfs) + "/"

    (sysfs / "state").write_text("active\n")
    assert client.is_armed() is True

    (sysfs / "state").write_text("inactive\n")
    assert client.is_armed() is False


def test_arm_writes_intent_file(ipc):
    client, _ = ipc
    client.arm(120)
    with open(wdtd.INTENT_FILE) as f:
        assert json.load(f) == {"armed": True, "timeout": 120}
    client.disarm()
    with open(wdtd.INTENT_FILE) as f:
        # Disarm clears the armed flag but preserves the timeout for reference.
        assert json.load(f) == {"armed": False, "timeout": 120}


def _build_daemon(tmp_path, monkeypatch, hw):
    """Build a daemon with mocked device/sysfs and an isolated intent file."""
    monkeypatch.setattr(wdtd, "INTENT_FILE", str(tmp_path / "intent.json"))

    def fake_ioctl(fd, op, arg=None, mutate=True):
        if op == wdtd.WDIOC_SETTIMEOUT:
            hw["timeout"] = int(arg[0])
        elif op == wdtd.WDIOC_SETOPTIONS:
            opt = int(arg[0])
            if opt == wdtd.WDIOS_ENABLECARD:
                hw["armed"] = True
            elif opt == wdtd.WDIOS_DISABLECARD:
                hw["armed"] = False
        return 0

    monkeypatch.setattr(wdtd.fcntl, "ioctl", fake_ioctl)
    daemon = wdtd.WatchdogManager()
    daemon.fd = 999
    daemon._open_device = lambda: True
    daemon._read_sysfs_str = lambda f: (
        str(hw["timeout"]) if f == "timeout"
        else ("active" if hw["armed"] else "inactive"))
    return daemon


def test_startup_arms_from_intent(tmp_path, monkeypatch):
    hw = {"armed": False, "timeout": 0}
    daemon = _build_daemon(tmp_path, monkeypatch, hw)
    daemon._write_intent(True, 150)
    daemon._startup_arm()
    assert daemon.armed is True
    assert hw["armed"] is True
    assert hw["timeout"] == 150


def test_startup_no_intent_boot_arm_disabled(tmp_path, monkeypatch):
    # Fresh boot (no tmpfs intent) with boot_arm policy off: the platform.json
    # value is the source of truth, so the daemon actively disarms to ensure the
    # hardware watchdog is off (even if the bootloader had armed it).
    hw = {"armed": False, "timeout": 0}
    daemon = _build_daemon(tmp_path, monkeypatch, hw)
    daemon.boot_arm = False
    daemon._startup_arm()
    assert daemon.armed is False
    assert hw["armed"] is False


def test_startup_no_intent_boot_arm_enabled(tmp_path, monkeypatch):
    # Fresh boot (no tmpfs intent) with boot_arm policy on: the daemon arms the
    # watchdog at the default timeout per platform policy.
    hw = {"armed": False, "timeout": 0}
    daemon = _build_daemon(tmp_path, monkeypatch, hw)
    daemon.boot_arm = True
    daemon._startup_arm()
    assert daemon.armed is True
    assert hw["armed"] is True
    assert hw["timeout"] == wdtd.DEFAULT_TIMEOUT


def test_pet_interval_scales_with_timeout():
    # Long timeouts are capped at the nominal keepalive interval; short timeouts
    # pet at half the window so they cannot expire before the next pet.
    daemon = wdtd.WatchdogManager()
    assert daemon._pet_interval_for(wdtd.DEFAULT_TIMEOUT) == \
        wdtd.KEEPALIVE_INTERVAL
    assert daemon._pet_interval_for(wdtd.KEEPALIVE_INTERVAL * 2) == \
        wdtd.KEEPALIVE_INTERVAL
    assert daemon._pet_interval_for(40) == 20
    # At the minimum armable timeout the interval is half of it, and never drops
    # below that floor even for smaller (non-armable) values.
    assert daemon._pet_interval_for(wdtd.MIN_TIMEOUT) == wdtd.MIN_TIMEOUT // 2
    assert daemon._pet_interval_for(1) == wdtd.MIN_TIMEOUT // 2
    assert daemon._pet_interval_for(0) == wdtd.MIN_TIMEOUT // 2


def test_arm_short_timeout_pets_within_window(tmp_path, monkeypatch):
    # A timeout below the nominal 60s keepalive interval must shrink the pet
    # cadence so the hardware is refreshed before it can expire and reset the box.
    hw = {"armed": False, "timeout": 0}
    daemon = _build_daemon(tmp_path, monkeypatch, hw)
    assert daemon.arm(40) == 40
    assert daemon.pet_interval == 20
    assert daemon.pet_interval < daemon.timeout
    # The next pet is scheduled one interval after the arming pet.
    assert daemon.next_ping == daemon.last_ping + daemon.pet_interval


def test_arm_long_timeout_caps_pet_interval(tmp_path, monkeypatch):
    # A long timeout keeps the nominal 60s cadence rather than petting needlessly
    # often, while still staying inside the hardware window.
    hw = {"armed": False, "timeout": 0}
    daemon = _build_daemon(tmp_path, monkeypatch, hw)
    assert daemon.arm(wdtd.DEFAULT_TIMEOUT) == wdtd.DEFAULT_TIMEOUT
    assert daemon.pet_interval == wdtd.KEEPALIVE_INTERVAL
    assert daemon.pet_interval < daemon.timeout


def test_default_policy_enabled():
    # Both protections default on so the box is protected out of the box, even
    # when platform.json omits the watchdog section entirely.
    daemon = wdtd.WatchdogManager()
    assert daemon.boot_arm is True
    assert daemon.shutdown_protect is True


def test_load_platform_config_defaults_enabled(monkeypatch):
    # platform.json without a "watchdog" section leaves both protections on.
    daemon = wdtd.WatchdogManager()
    daemon.boot_arm = False
    daemon.shutdown_protect = False
    monkeypatch.setattr(wdtd, "get_platform_json_data", lambda: {})
    daemon._load_platform_config()
    assert daemon.boot_arm is True
    assert daemon.shutdown_protect is True


def test_load_platform_config_explicit_opt_out(monkeypatch):
    # platform.json can still explicitly disable each protection.
    daemon = wdtd.WatchdogManager()
    monkeypatch.setattr(
        wdtd, "get_platform_json_data",
        lambda: {"watchdog": {"boot_arm": False, "shutdown_protect": False}})
    daemon._load_platform_config()
    assert daemon.boot_arm is False
    assert daemon.shutdown_protect is False


def test_startup_intent_disarmed_overrides_boot_arm(tmp_path, monkeypatch):
    # A runtime disarm earlier in this boot session (intent present, armed false)
    # must win over boot_arm on a daemon restart.
    hw = {"armed": False, "timeout": 0}
    daemon = _build_daemon(tmp_path, monkeypatch, hw)
    daemon.boot_arm = True
    daemon._write_intent(False, wdtd.DEFAULT_TIMEOUT)
    daemon._startup_arm()
    assert daemon.armed is False
    assert hw["armed"] is False


def test_startup_intent_disarmed_stops_active_hw(tmp_path, monkeypatch):
    # Crash mid-disarm: the intent was recorded disarmed but the hardware is
    # still ACTIVE.  Startup must complete the disarm (honour the intent), not
    # re-adopt the watchdog.
    hw = {"armed": True, "timeout": 90}
    daemon = _build_daemon(tmp_path, monkeypatch, hw)
    daemon._write_intent(False, 90)
    daemon._startup_arm()
    assert daemon.armed is False
    assert hw["armed"] is False


def test_startup_adopts_active_hardware(tmp_path, monkeypatch):
    # Safety net: no intent recorded (e.g. an intent-write failure), but the
    # hardware watchdog is ACTIVE and would not be petted by the kernel once its
    # opener is gone.  The new instance must adopt it instead of leaving it
    # unpetted.
    hw = {"armed": True, "timeout": 90}
    daemon = _build_daemon(tmp_path, monkeypatch, hw)
    daemon._startup_arm()
    assert daemon.armed is True
    assert hw["armed"] is True
    assert daemon.timeout == 90


def _cleanup_capture(daemon, monkeypatch):
    """Run cleanup() with os write/close/unlink and keepalive captured."""
    writes = []
    closed = []
    keepalives = []
    monkeypatch.setattr(wdtd.os, "write", lambda fd, b: writes.append(b))
    monkeypatch.setattr(wdtd.os, "close", lambda fd: closed.append(fd))
    monkeypatch.setattr(wdtd.os, "unlink", lambda p: None)
    monkeypatch.setattr(daemon, "_keepalive", lambda: keepalives.append(True))
    with pytest.raises(SystemExit):
        daemon.cleanup()
    return writes, closed, keepalives


def test_cleanup_shutdown_protect_skips_magic_close(tmp_path, monkeypatch):
    # During a system shutdown with shutdown_protect on, the daemon pets once and
    # closes WITHOUT the magic 'V', leaving the watchdog armed for reboot
    # protection.
    hw = {"armed": False, "timeout": 0}
    daemon = _build_daemon(tmp_path, monkeypatch, hw)
    daemon.armed = True
    daemon.timeout = 180
    daemon.shutdown_protect = True
    monkeypatch.setattr(daemon, "_system_is_stopping", lambda: True)
    writes, closed, keepalives = _cleanup_capture(daemon, monkeypatch)
    assert keepalives == [True]
    assert b"V" not in writes
    assert closed == [999]


def test_cleanup_normal_stop_magic_closes(tmp_path, monkeypatch):
    # A normal daemon stop/restart (not a system shutdown) disarms via
    # magic-close even when shutdown_protect is enabled.
    hw = {"armed": False, "timeout": 0}
    daemon = _build_daemon(tmp_path, monkeypatch, hw)
    daemon.armed = True
    daemon.timeout = 180
    daemon.shutdown_protect = True
    monkeypatch.setattr(daemon, "_system_is_stopping", lambda: False)
    writes, closed, _ = _cleanup_capture(daemon, monkeypatch)
    assert writes == [b"V"]
    assert closed == [999]


def test_cleanup_no_shutdown_protect_magic_closes(tmp_path, monkeypatch):
    # With shutdown_protect off, the daemon disarms via magic-close even during a
    # system shutdown.
    hw = {"armed": False, "timeout": 0}
    daemon = _build_daemon(tmp_path, monkeypatch, hw)
    daemon.armed = True
    daemon.timeout = 180
    daemon.shutdown_protect = False
    monkeypatch.setattr(daemon, "_system_is_stopping", lambda: True)
    writes, closed, _ = _cleanup_capture(daemon, monkeypatch)
    assert writes == [b"V"]
    assert closed == [999]
