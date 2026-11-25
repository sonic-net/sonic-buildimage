#
# test_leak_verifier.py
#
"""Unit tests for the consumer preflight in scripts/test_leak_injection_sim.py.

On the platform marker directory the verifier must confirm that thermalctld
marks injected leaks as test leaks before it writes any marker, so that an
older thermalctld/bmcctld never sees an unmarked injected leak. These tests
drive main() with fake platform objects and a fake STATE_DB and record the
order of the preflight, the checks and every marker write.
"""

import importlib.util
import os
import sys
import types

import pytest

SCRIPTS_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "scripts")
NAMES = ["coldplate_leak", "driptray_leak"]

_spec = importlib.util.spec_from_file_location(
    "leak_verifier", os.path.join(SCRIPTS_DIR, "test_leak_injection_sim.py"))
verifier = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(verifier)


class FakeSensor:
    def __init__(self, name):
        self.name = name

    def get_name(self):
        return self.name


class FakeLeakTest:
    def __init__(self, events):
        self.events = events

    def is_leak_test_supported(self):
        return True

    def is_test_leak_enabled(self, name):
        return False

    def set_test_leak(self, name, enable, *, severity=None):
        self.events.append(("set_test_leak", name, enable))
        return True

    def clear_test_leaks(self):
        self.events.append(("clear_test_leaks",))
        return True


class FakeLiquidCooling:
    def __init__(self, events, leak_test_dir):
        self.leak_test_dir = leak_test_dir
        self.leak_test = FakeLeakTest(events)

    def get_all_leak_sensors(self):
        return [FakeSensor(n) for n in NAMES]

    def get_leak_sensor_test(self):
        return self.leak_test


class FakeDb:
    STATE_DB = "STATE_DB"

    def __init__(self, rows):
        self.rows = rows

    def connect(self, db):
        pass

    def get_all(self, db, key):
        return dict(self.rows.get(key, {}))


COMPATIBLE_ROWS = {
    "LIQUID_COOLING_INFO|coldplate_leak": {"leaking": "No", "test_leak": "Disabled",
                                           "leak_source": "none"},
    "LIQUID_COOLING_INFO|driptray_leak": {"leaking": "No", "test_leak": "Disabled",
                                          "leak_source": "none"},
    "SYSTEM_LEAK_STATUS|system": {"device_leak_status": "None", "test_leak": "Disabled"},
}


def _strip(rows, key, field):
    rows = {k: dict(v) for k, v in rows.items()}
    rows[key].pop(field)
    return rows


@pytest.fixture
def fake_swsscommon(monkeypatch):
    """Install a fake swsscommon whose STATE_DB holds the rows set on it."""
    state = {"rows": {}}

    def connector(use_unix_socket_path=True):
        return FakeDb(state["rows"])

    pkg = types.ModuleType("swsscommon")
    mod = types.ModuleType("swsscommon.swsscommon")
    mod.SonicV2Connector = connector
    pkg.swsscommon = mod
    monkeypatch.setitem(sys.modules, "swsscommon", pkg)
    monkeypatch.setitem(sys.modules, "swsscommon.swsscommon", mod)
    return state


@pytest.fixture
def harness(monkeypatch, tmp_path):
    """Fake the platform and record the preflight, every check and every write."""
    events = []
    shared_dir = str(tmp_path / "shared")
    module = types.SimpleNamespace(
        LEAK_TEST_DIR=shared_dir,
        LiquidCooling=lambda leak_test_dir: FakeLiquidCooling(events, leak_test_dir))

    monkeypatch.setattr(verifier.sim, "load_liquid_cooling_module", lambda: module)
    monkeypatch.setattr(verifier.sim, "get_liquid_cooling",
                        lambda d=None: FakeLiquidCooling(events, d or shared_dir))

    real_connect = verifier.connect_compatible_consumer

    def connect(names):
        db, reason = real_connect(names)
        events.append(("preflight", db is not None))
        return db, reason

    monkeypatch.setattr(verifier, "connect_compatible_consumer", connect)

    def recorder(label):
        def check(report, *args, **kwargs):
            events.append((label,))
            if label == "check_state_db":
                events.append(("state_db_handle", args[0]))
        return check

    for label in ("check_baseline", "check_contract", "check_sensor_cycle",
                  "check_clear_all", "check_cli", "check_state_db"):
        monkeypatch.setattr(verifier, label, recorder(label))
    monkeypatch.setattr(verifier.subprocess, "run",
                        lambda *a, **k: events.append(("subprocess",)))
    return types.SimpleNamespace(events=events, shared_dir=shared_dir,
                                 private_dir=str(tmp_path / "private"))


def _writes(events):
    return [e for e in events if e[0] in ("set_test_leak", "subprocess")
            or e[0].startswith("check_")]


class TestSharedDirPreflight:
    @pytest.mark.parametrize("verify_db", [False, True])
    def test_no_swsscommon_refuses_before_any_write(self, harness, monkeypatch, verify_db):
        monkeypatch.setitem(sys.modules, "swsscommon", None)
        monkeypatch.setitem(sys.modules, "swsscommon.swsscommon", None)
        argv = ["--verify-db"] if verify_db else []
        assert verifier.main(argv) == 2
        assert harness.events == [("preflight", False)]

    @pytest.mark.parametrize("rows", [
        {},
        _strip(COMPATIBLE_ROWS, "LIQUID_COOLING_INFO|coldplate_leak", "test_leak"),
        _strip(COMPATIBLE_ROWS, "LIQUID_COOLING_INFO|driptray_leak", "leak_source"),
        _strip(COMPATIBLE_ROWS, "SYSTEM_LEAK_STATUS|system", "test_leak"),
    ], ids=["no_rows", "sensor_without_test_leak", "sensor_without_leak_source",
            "system_without_test_leak"])
    @pytest.mark.parametrize("verify_db", [False, True])
    def test_incompatible_consumer_refuses_before_any_write(
            self, harness, fake_swsscommon, rows, verify_db):
        fake_swsscommon["rows"] = rows
        argv = ["--verify-db"] if verify_db else []
        assert verifier.main(argv) == 2
        assert harness.events == [("preflight", False)]

    def test_compatible_consumer_preflight_runs_first(self, harness, fake_swsscommon):
        fake_swsscommon["rows"] = COMPATIBLE_ROWS
        assert verifier.main(["--verify-db"]) == 0
        assert harness.events[0] == ("preflight", True)
        labels = [e[0] for e in _writes(harness.events)]
        assert labels[0] == "check_baseline"
        assert "check_state_db" in labels
        handle = next(e[1] for e in harness.events if e[0] == "state_db_handle")
        assert isinstance(handle, FakeDb)

    def test_explicit_shared_dir_is_still_guarded(self, harness, fake_swsscommon):
        fake_swsscommon["rows"] = {}
        assert verifier.main(["--leak-test-dir", harness.shared_dir + "/"]) == 2
        assert harness.events == [("preflight", False)]


class TestPrivateDir:
    def test_private_dir_skips_preflight_and_state_db(self, harness, fake_swsscommon, capsys):
        fake_swsscommon["rows"] = {}
        assert verifier.main(["--leak-test-dir", harness.private_dir, "--verify-db"]) == 0
        labels = [e[0] for e in harness.events]
        assert "preflight" not in labels
        assert "check_state_db" not in labels
        assert "check_sensor_cycle" in labels
        assert "[SKIP] STATE_DB verification: thermalctld does not watch" in capsys.readouterr().out
