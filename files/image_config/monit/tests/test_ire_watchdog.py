"""Unit tests for the monit ``ire_watchdog`` one-shot check.

The script under test (``files/image_config/monit/ire_watchdog``) has no ``.py``
extension, so it is loaded via :class:`importlib.machinery.SourceFileLoader`,
mirroring ``src/system-health/tests/test_system_health.py``.

The IRE_DATA_PATH_CRC_ERROR_COUNTER register is clear-on-read, so the check
alerts whenever any core reports a non-zero count (no baseline comparison).

Covered behaviour:
    * ``nonzero_cores()`` pure logic.
    * Any non-zero core -> a single ``LOG_ERR`` alert plus a timestamped entry
      appended to the error log (recording every per-core count).
    * All-zero reading -> no alert, no log written.
    * ``PlatformNotSupported`` / ``SyncdUnavailable`` / transient error ->
      exit 0, no false alert, error log not written.
    * ``log_counters()`` appends timestamped snapshots, caps history, and
      recovers from a corrupt log.
    * ``read_counters()`` bcmcmd output parsing (hex + decimal), the
      unknown-register (non-DNX) path and the syncd-down path.

The tests are self-contained: stdlib + pytest + ``unittest.mock`` only, with no
dependency on ``swsscommon`` or a live ASIC/syncd.
"""
import importlib.machinery
import importlib.util
import json
import os
import sys

from unittest.mock import patch

import pytest


def load_source(modname, filename):
    """Load an extension-less script as a module (mirrors system-health test)."""
    loader = importlib.machinery.SourceFileLoader(modname, filename)
    spec = importlib.util.spec_from_file_location(modname, filename, loader=loader)
    module = importlib.util.module_from_spec(spec)
    # The module is executed and cached in sys.modules under its own name.
    sys.modules[module.__name__] = module
    loader.exec_module(module)
    return module


mod = load_source(
    'ire_watchdog',
    os.path.join(os.path.dirname(__file__), '..', 'ire_watchdog'),
)


class _FakeProc:
    """Minimal stand-in for :class:`subprocess.CompletedProcess`."""

    def __init__(self, returncode=0, stdout='', stderr=''):
        self.returncode = returncode
        self.stdout = stdout
        self.stderr = stderr


def _read_json(path):
    with open(path) as f:
        return json.load(f)


def _assert_no_log_err(mock_syslog):
    """Fail if ``syslog.syslog`` was ever called at ``LOG_ERR`` level."""
    for c in mock_syslog.call_args_list:
        assert c.args[0] != mod.syslog.LOG_ERR


# --- pure logic -------------------------------------------------------------

def test_nonzero_cores():
    # Clear-on-read: any non-zero core is reported, sorted ascending.
    assert mod.nonzero_cores({0: 0, 1: 3, 2: 0}) == [1]
    assert mod.nonzero_cores({0: 5, 1: 2}) == [0, 1]
    # All-zero (or empty) reading -> nothing to report.
    assert mod.nonzero_cores({0: 0, 1: 0}) == []
    assert mod.nonzero_cores({}) == []


# --- high-level main() paths (read_counters patched) ------------------------

def test_nonzero_triggers_alert(tmp_path, monkeypatch):
    log = tmp_path / 'ire_crc_error_count.json'
    monkeypatch.setattr(mod, 'ERROR_LOG', str(log))
    with patch.object(mod, 'read_counters', return_value={0: 0, 1: 3}), \
            patch.object(mod.syslog, 'syslog') as mock_syslog:
        assert mod.main() == 1
        mock_syslog.assert_called_once_with(mod.syslog.LOG_ERR,
                                            mod.INTERRUPT_LOG_MSG)
    # One timestamped entry recording every per-core count (zeros included).
    entries = _read_json(str(log))
    assert len(entries) == 1
    assert 'timestamp' in entries[0]
    assert entries[0]['counters'] == {'0': 0, '1': 3}


def test_all_zero_no_alert(tmp_path, monkeypatch):
    log = tmp_path / 'ire_crc_error_count.json'
    monkeypatch.setattr(mod, 'ERROR_LOG', str(log))
    with patch.object(mod, 'read_counters', return_value={0: 0, 1: 0}), \
            patch.object(mod.syslog, 'syslog') as mock_syslog:
        assert mod.main() == 0
        mock_syslog.assert_not_called()
    # No errors -> nothing logged.
    assert not log.exists()


def test_platform_not_supported(tmp_path, monkeypatch):
    log = tmp_path / 'ire_crc_error_count.json'
    monkeypatch.setattr(mod, 'ERROR_LOG', str(log))
    with patch.object(mod, 'read_counters',
                      side_effect=mod.PlatformNotSupported('not dnx')), \
            patch.object(mod.syslog, 'syslog') as mock_syslog:
        assert mod.main() == 0
        _assert_no_log_err(mock_syslog)
    # Non-DNX no-op: nothing logged.
    assert not log.exists()


def test_syncd_unavailable(tmp_path, monkeypatch):
    log = tmp_path / 'ire_crc_error_count.json'
    monkeypatch.setattr(mod, 'ERROR_LOG', str(log))
    with patch.object(mod, 'read_counters',
                      side_effect=mod.SyncdUnavailable('syncd down')), \
            patch.object(mod.syslog, 'syslog') as mock_syslog:
        assert mod.main() == 0
        _assert_no_log_err(mock_syslog)
    assert not log.exists()


def test_transient_error_no_false_alert(tmp_path, monkeypatch):
    log = tmp_path / 'ire_crc_error_count.json'
    monkeypatch.setattr(mod, 'ERROR_LOG', str(log))
    with patch.object(mod, 'read_counters', side_effect=RuntimeError('boom')), \
            patch.object(mod.syslog, 'syslog') as mock_syslog:
        assert mod.main() == 0
        # A LOG_WARNING is acceptable here, but never a LOG_ERR false alert.
        _assert_no_log_err(mock_syslog)
    assert not log.exists()


# --- log_counters() ---------------------------------------------------------

def test_log_counters_appends_with_timestamp(tmp_path, monkeypatch):
    log = tmp_path / 'ire_crc_error_count.json'
    monkeypatch.setattr(mod, 'ERROR_LOG', str(log))
    mod.log_counters({0: 1, 1: 0})
    mod.log_counters({0: 0, 1: 7})
    entries = _read_json(str(log))
    assert len(entries) == 2
    assert all('timestamp' in e and 'counters' in e for e in entries)
    assert entries[0]['counters'] == {'0': 1, '1': 0}
    assert entries[1]['counters'] == {'0': 0, '1': 7}


def test_log_counters_caps_history(tmp_path, monkeypatch):
    log = tmp_path / 'ire_crc_error_count.json'
    monkeypatch.setattr(mod, 'ERROR_LOG', str(log))
    monkeypatch.setattr(mod, 'MAX_LOG_ENTRIES', 3)
    for i in range(1, 6):
        mod.log_counters({0: i})
    entries = _read_json(str(log))
    # Capped to the most recent MAX_LOG_ENTRIES entries (oldest dropped first).
    assert len(entries) == 3
    assert [e['counters']['0'] for e in entries] == [3, 4, 5]


def test_log_counters_resets_corrupt_log(tmp_path, monkeypatch):
    log = tmp_path / 'ire_crc_error_count.json'
    monkeypatch.setattr(mod, 'ERROR_LOG', str(log))
    log.write_text('not valid json{{{')
    mod.log_counters({0: 2})
    entries = _read_json(str(log))
    assert len(entries) == 1
    assert entries[0]['counters'] == {'0': 2}


# --- read_counters() parsing (subprocess patched) ---------------------------

def test_read_counters_parses_hex_and_dec():
    stdout = (
        'IRE_DATA_PATH_CRC_ERROR_COUNTER.IRE0[0x100]=10: '
        '<DATA_PATH_CRC_ERROR_COUNTER=10>\n'
        'IRE_DATA_PATH_CRC_ERROR_COUNTER.IRE1[0x101]=0x2a: '
        '<DATA_PATH_CRC_ERROR_COUNTER=0x2a>\n'
    )
    with patch.object(mod.subprocess, 'run',
                      return_value=_FakeProc(returncode=0, stdout=stdout)):
        # Decimal "10" and hex "0x2a" (== 42) are both accepted.
        assert mod.read_counters() == {0: 10, 1: 42}


def test_read_counters_unknown_register():
    with patch.object(mod.subprocess, 'run',
                      return_value=_FakeProc(returncode=0,
                                             stdout='Symbol not found')):
        with pytest.raises(mod.PlatformNotSupported):
            mod.read_counters()


def test_read_counters_syncd_down():
    stderr = 'Error response from daemon: Container syncd is not running'
    with patch.object(mod.subprocess, 'run',
                      return_value=_FakeProc(returncode=1, stderr=stderr)), \
            patch.object(mod, 'syncd_running', return_value=False):
        with pytest.raises(mod.SyncdUnavailable):
            mod.read_counters()


if __name__ == '__main__':
    sys.exit(pytest.main([__file__, '-v']))
