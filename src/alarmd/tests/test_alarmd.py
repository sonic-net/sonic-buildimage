"""
test_alarmd.py — Comprehensive unit tests for alarmd daemon

Tests all major components:
  - evaluate_condition()      — field value comparison logic
  - EventPublisher            — eventd output path (tests/test_event_publisher.py)
  - StateDBPoller             — statedb polling with OR-logic for shared alarm_ids
  - ScriptRunner              — script execution, exit codes, timeouts
  - AlarmDaemon._load_defs()  — JSON loading + guardrails
  - alarm_defs.json           — structural validation

Run:
    cd alarmd && python3 -m pytest tests/ -v
"""

import json
import os
import fcntl
import signal
import sys
import unittest
from unittest.mock import MagicMock, patch, call, mock_open
from datetime import datetime
import tempfile
import shutil

import importlib.util
import importlib.machinery
import logging.handlers

TEST_DIR  = os.path.dirname(os.path.abspath(__file__))
ALARMD_DIR = os.path.dirname(TEST_DIR)
ALARMD_ENTRY = os.path.join(ALARMD_DIR, 'scripts', 'alarmd')
ALARM_DEFS_PATH = os.path.join(TEST_DIR, 'alarm_defs.json')  # Now in tests/

# Add alarmd's parent directory to sys.path so sonic_alarm package is importable
sys.path.insert(0, ALARMD_DIR)

# ---------------------------------------------------------------------------
# Mock sonic_py_common and swsscommon before importing any sonic_alarm modules.
# The refactored code uses module-level SysLogger instances and direct
# swsscommon imports — both must be mocked before first import.
# ---------------------------------------------------------------------------

# sonic_py_common mocks (SysLogger, DaemonBase, daemon_base)
_mock_syslogger_cls = MagicMock()
_mock_syslogger_instance = MagicMock()
_mock_syslogger_cls.return_value = _mock_syslogger_instance

_mock_sonic_py_common = MagicMock()
_mock_sonic_py_common.syslogger.SysLogger = _mock_syslogger_cls
_mock_sonic_py_common.daemon_base.DaemonBase = MagicMock
_mock_sonic_py_common.daemon_base.db_connect = MagicMock()

sys.modules['sonic_py_common'] = _mock_sonic_py_common
sys.modules['sonic_py_common.syslogger'] = _mock_sonic_py_common.syslogger
sys.modules['sonic_py_common.daemon_base'] = _mock_sonic_py_common.daemon_base

# swsscommon mocks
_mock_swsscommon_inner = MagicMock()
_mock_swsscommon_inner.DBConnector = MagicMock
_mock_swsscommon_inner.Table = MagicMock
_mock_swsscommon_inner.FieldValuePairs = MagicMock

_mock_swsscommon_pkg = MagicMock()
_mock_swsscommon_pkg.swsscommon = _mock_swsscommon_inner
_mock_swsscommon_pkg.FieldValuePairs = MagicMock

sys.modules['swsscommon'] = _mock_swsscommon_pkg
sys.modules['swsscommon.swsscommon'] = _mock_swsscommon_inner

# ---------------------------------------------------------------------------
# Import sonic_alarm package components
# ---------------------------------------------------------------------------
from sonic_alarm.statedb_poller import evaluate_condition, StateDBPoller
from sonic_alarm.script_runner import ScriptRunner
from sonic_alarm.event_publisher import EventPublisher
from sonic_alarm import config as alarm_config

# Import module references for patching module-level loggers in tests
import sonic_alarm.statedb_poller as _statedb_poller_mod
import sonic_alarm.script_runner as _script_runner_mod

# ---------------------------------------------------------------------------
# Load scripts/alarmd entry point as a module (for AlarmDaemon, main, etc.)
# Mock SysLogHandler before loading (no /dev/log in build env).
# ---------------------------------------------------------------------------
_orig_syslog = logging.handlers.SysLogHandler
logging.handlers.SysLogHandler = MagicMock

def _load_source(modname, filename):
    """Load a script without .py extension as a module."""
    loader = importlib.machinery.SourceFileLoader(modname, filename)
    spec = importlib.util.spec_from_file_location(modname, filename, loader=loader)
    module = importlib.util.module_from_spec(spec)
    sys.modules[module.__name__] = module
    loader.exec_module(module)
    return module

alarmd_script = _load_source('alarmd_script', ALARMD_ENTRY)

logging.handlers.SysLogHandler = _orig_syslog


class _FakeEventApi:
    """Minimal EventPublisher event-API stand-in (records publishes, rc=0)."""

    def __init__(self, rc=0):
        self.published = []
        self._rc = rc

    def init_publisher(self, source):
        return object()

    def deinit_publisher(self, handle):
        pass

    def publish(self, handle, tag, params):
        self.published.append((tag, dict(params)))
        return self._rc

# ═══════════════════════════════════════════════════════════════════════════
# 1. evaluate_condition() tests
# ═══════════════════════════════════════════════════════════════════════════
class TestEvaluateCondition(unittest.TestCase):
    """Tests for the condition evaluator used by statedb checks."""

    # --- equality ---
    def test_eq_true(self):
        self.assertTrue(evaluate_condition('false', '==', 'false'))

    def test_eq_case_sensitive(self):
        """Equality is case-sensitive per HLD §7.7."""
        self.assertFalse(evaluate_condition('False', '==', 'false'))
        self.assertFalse(evaluate_condition('TRUE', '==', 'true'))
        self.assertTrue(evaluate_condition('false', '==', 'false'))
        self.assertTrue(evaluate_condition('True', '==', 'True'))

    def test_eq_false(self):
        self.assertFalse(evaluate_condition('true', '==', 'false'))

    # --- inequality ---
    def test_neq_true(self):
        self.assertTrue(evaluate_condition('true', '!=', 'false'))

    def test_neq_false(self):
        self.assertFalse(evaluate_condition('false', '!=', 'false'))

    # --- numeric comparisons ---
    def test_less_than_true(self):
        self.assertTrue(evaluate_condition('30', '<', '50'))

    def test_less_than_false(self):
        self.assertFalse(evaluate_condition('70', '<', '50'))

    def test_less_than_float(self):
        self.assertTrue(evaluate_condition('11.5', '<', '11.6'))

    def test_greater_than_true(self):
        self.assertTrue(evaluate_condition('13.0', '>', '12.8'))

    def test_greater_than_false(self):
        self.assertFalse(evaluate_condition('12.2', '>', '12.8'))

    def test_lte(self):
        self.assertTrue(evaluate_condition('50', '<=', '50'))
        self.assertTrue(evaluate_condition('49', '<=', '50'))
        self.assertFalse(evaluate_condition('51', '<=', '50'))

    def test_gte(self):
        self.assertTrue(evaluate_condition('50', '>=', '50'))
        self.assertTrue(evaluate_condition('51', '>=', '50'))
        self.assertFalse(evaluate_condition('49', '>=', '50'))

    # --- edge cases ---
    def test_none_value(self):
        self.assertFalse(evaluate_condition(None, '==', 'false'))

    def test_non_numeric_comparison(self):
        """Non-numeric strings with < fall back to string comparison."""
        self.assertFalse(evaluate_condition('N/A', '<', '50'))
        # String fallback: 'abc' < 'def' is True lexicographically
        self.assertTrue(evaluate_condition('abc', '<', 'def'))

    def test_whitespace_handling(self):
        self.assertTrue(evaluate_condition(' false ', '==', 'false'))
        self.assertTrue(evaluate_condition('12.5 ', '>', ' 12.0'))

    def test_unknown_operator(self):
        self.assertFalse(evaluate_condition('foo', '~=', 'foo'))


# ═══════════════════════════════════════════════════════════════════════════
# 2. AlarmStore removed in eventd refactor — see tests/test_event_publisher.py
# ═══════════════════════════════════════════════════════════════════════════
# AlarmStore was removed in the eventd-integration refactor (HLD v0.2): the
# alarm output path is now EventPublisher (publishes RAISE/CLEAR through
# eventd; eventd owns the ALARM table).  Its behaviour is covered by
# tests/test_event_publisher.py.


# ═══════════════════════════════════════════════════════════════════════════
# 3. StateDBPoller tests
# ═══════════════════════════════════════════════════════════════════════════
class TestStateDBPoller(unittest.TestCase):
    """Tests for StateDB polling, including OR-logic for shared alarm_ids."""

    def _make_poller(self, tables):
        mock_db = MagicMock()
        store = MagicMock()
        poller = StateDBPoller(tables, store, mock_db)
        return poller, mock_db, store

    def _mock_table(self, mock_db, table_name, keys, data):
        """Configure mock_db so Table(mock_db, table_name) returns mock data.

        Parameters
        ----------
        keys : list of str
            Keys returned by getKeys().
        data : dict of str → dict
            key → {field: value} mapping for get() calls.
        """
        mock_tbl = MagicMock()
        mock_tbl.getKeys.return_value = keys
        def _get(key):
            if key in data:
                return True, list(data[key].items())
            return False, []
        mock_tbl.get.side_effect = _get

        # Patch the Table constructor so Table(mock_db, table_name) returns our mock
        return mock_tbl

    @patch('sonic_alarm.statedb_poller.Table')
    def test_simple_fault_detection(self, MockTable):
        """PSU presence=false should trigger PSU_MISSING alarm."""
        tables = [{
            'table_name': 'PSU_INFO',
            'object_key_pattern': 'PSU_INFO|*',
            'checks': [{
                'alarm_id': 'PSU_MISSING',
                'condition': {'field': 'presence', 'operator': '==', 'value': 'false'}
            }]
        }]
        poller, mock_db, store = self._make_poller(tables)
        mock_tbl = MagicMock()
        mock_tbl.getKeys.return_value = ['PSU1']
        mock_tbl.get.return_value = (True, [('presence', 'false'), ('status', 'true')])
        MockTable.return_value = mock_tbl

        poller.poll()

        store.set_alarm.assert_called_once_with('PSU_MISSING', 'PSU1', is_fault=True)

    @patch('sonic_alarm.statedb_poller.Table')
    def test_no_fault_clears(self, MockTable):
        """PSU presence=true should clear PSU_MISSING alarm."""
        tables = [{
            'table_name': 'PSU_INFO',
            'object_key_pattern': 'PSU_INFO|*',
            'checks': [{
                'alarm_id': 'PSU_MISSING',
                'condition': {'field': 'presence', 'operator': '==', 'value': 'false'}
            }]
        }]
        poller, mock_db, store = self._make_poller(tables)
        mock_tbl = MagicMock()
        mock_tbl.getKeys.return_value = ['PSU1']
        mock_tbl.get.return_value = (True, [('presence', 'true'), ('status', 'true')])
        MockTable.return_value = mock_tbl

        poller.poll()

        store.set_alarm.assert_called_once_with('PSU_MISSING', 'PSU1', is_fault=False)

    @patch('sonic_alarm.statedb_poller.Table')
    def test_multiple_keys(self, MockTable):
        """Should evaluate each key independently."""
        tables = [{
            'table_name': 'PSU_INFO',
            'object_key_pattern': 'PSU_INFO|*',
            'checks': [{
                'alarm_id': 'PSU_MISSING',
                'condition': {'field': 'presence', 'operator': '==', 'value': 'false'}
            }]
        }]
        poller, mock_db, store = self._make_poller(tables)
        mock_tbl = MagicMock()
        mock_tbl.getKeys.return_value = ['PSU1', 'PSU2']
        def _get(key):
            if key == 'PSU1':
                return True, [('presence', 'false')]
            return True, [('presence', 'true')]
        mock_tbl.get.side_effect = _get
        MockTable.return_value = mock_tbl

        poller.poll()

        calls = store.set_alarm.call_args_list
        self.assertEqual(len(calls), 2)
        # PSU1 → fault
        self.assertEqual(calls[0], call('PSU_MISSING', 'PSU1', is_fault=True))
        # PSU2 → no fault
        self.assertEqual(calls[1], call('PSU_MISSING', 'PSU2', is_fault=False))

    @patch('sonic_alarm.statedb_poller.Table')
    def test_or_logic_shared_alarm_id(self, MockTable):
        """Two checks with same alarm_id: fault if EITHER triggers (OR logic)."""
        tables = [{
            'table_name': 'PSU_INFO',
            'object_key_pattern': 'PSU_INFO|*',
            'checks': [
                {
                    'alarm_id': 'PSU_OUTPUT_VOLTAGE_FAULT',
                    'condition': {'field': 'voltage', 'operator': '<', 'value': '11.6'}
                },
                {
                    'alarm_id': 'PSU_OUTPUT_VOLTAGE_FAULT',
                    'condition': {'field': 'voltage', 'operator': '>', 'value': '12.8'}
                }
            ]
        }]
        poller, mock_db, store = self._make_poller(tables)

        # Test 1: voltage too low — should fault
        mock_tbl = MagicMock()
        mock_tbl.getKeys.return_value = ['PSU1']
        mock_tbl.get.return_value = (True, [('voltage', '11.0')])
        MockTable.return_value = mock_tbl

        poller.poll()
        store.set_alarm.assert_called_once_with('PSU_OUTPUT_VOLTAGE_FAULT', 'PSU1', is_fault=True)

    @patch('sonic_alarm.statedb_poller.Table')
    def test_or_logic_high_voltage(self, MockTable):
        """Shared alarm_id: high voltage should also fault."""
        tables = [{
            'table_name': 'PSU_INFO',
            'object_key_pattern': 'PSU_INFO|*',
            'checks': [
                {
                    'alarm_id': 'PSU_OUTPUT_VOLTAGE_FAULT',
                    'condition': {'field': 'voltage', 'operator': '<', 'value': '11.6'}
                },
                {
                    'alarm_id': 'PSU_OUTPUT_VOLTAGE_FAULT',
                    'condition': {'field': 'voltage', 'operator': '>', 'value': '12.8'}
                }
            ]
        }]
        poller, mock_db, store = self._make_poller(tables)

        mock_tbl = MagicMock()
        mock_tbl.getKeys.return_value = ['PSU1']
        mock_tbl.get.return_value = (True, [('voltage', '13.5')])
        MockTable.return_value = mock_tbl

        poller.poll()
        store.set_alarm.assert_called_once_with('PSU_OUTPUT_VOLTAGE_FAULT', 'PSU1', is_fault=True)

    @patch('sonic_alarm.statedb_poller.Table')
    def test_or_logic_normal_voltage_clears(self, MockTable):
        """Shared alarm_id: normal voltage (neither low nor high) should clear."""
        tables = [{
            'table_name': 'PSU_INFO',
            'object_key_pattern': 'PSU_INFO|*',
            'checks': [
                {
                    'alarm_id': 'PSU_OUTPUT_VOLTAGE_FAULT',
                    'condition': {'field': 'voltage', 'operator': '<', 'value': '11.6'}
                },
                {
                    'alarm_id': 'PSU_OUTPUT_VOLTAGE_FAULT',
                    'condition': {'field': 'voltage', 'operator': '>', 'value': '12.8'}
                }
            ]
        }]
        poller, mock_db, store = self._make_poller(tables)

        mock_tbl = MagicMock()
        mock_tbl.getKeys.return_value = ['PSU1']
        mock_tbl.get.return_value = (True, [('voltage', '12.2')])
        MockTable.return_value = mock_tbl

        poller.poll()
        store.set_alarm.assert_called_once_with('PSU_OUTPUT_VOLTAGE_FAULT', 'PSU1', is_fault=False)

    @patch('sonic_alarm.statedb_poller.Table')
    def test_exclude_keys(self, MockTable):
        """Keys in exclude_keys should be skipped."""
        tables = [{
            'table_name': 'STORAGE_INFO',
            'object_key_pattern': 'STORAGE_INFO|*',
            'exclude_keys': ['FSSTATS_SYNC', 'STORAGE_INFO|FSSTATS_SYNC'],
            'checks': [{
                'alarm_id': 'STORAGE_HEALTH_DEGRADED',
                'condition': {'field': 'health', 'operator': '<', 'value': '50'}
            }]
        }]
        poller, mock_db, store = self._make_poller(tables)
        mock_tbl = MagicMock()
        mock_tbl.getKeys.return_value = ['SSD0', 'FSSTATS_SYNC']
        mock_tbl.get.return_value = (True, [('health', '30')])
        MockTable.return_value = mock_tbl

        poller.poll()
        # Only SSD0 should trigger an alarm
        store.set_alarm.assert_called_once_with('STORAGE_HEALTH_DEGRADED', 'SSD0', is_fault=True)

    @patch('sonic_alarm.statedb_poller.Table')
    def test_missing_field_no_fault(self, MockTable):
        """If the checked field doesn't exist in DB, should not fault."""
        tables = [{
            'table_name': 'PSU_INFO',
            'object_key_pattern': 'PSU_INFO|*',
            'checks': [{
                'alarm_id': 'PSU_TEMP_FAULT',
                'condition': {'field': 'temp_fault', 'operator': '==', 'value': 'True'}
            }]
        }]
        poller, mock_db, store = self._make_poller(tables)
        mock_tbl = MagicMock()
        mock_tbl.getKeys.return_value = ['PSU1']
        # temp_fault field doesn't exist in this entry
        mock_tbl.get.return_value = (True, [('presence', 'true'), ('status', 'true')])
        MockTable.return_value = mock_tbl

        poller.poll()
        store.set_alarm.assert_called_once_with('PSU_TEMP_FAULT', 'PSU1', is_fault=False)

    @patch('sonic_alarm.statedb_poller.Table')
    def test_db_getkeys_failure(self, MockTable):
        """Table.getKeys() failure should not crash, should log error."""
        tables = [{
            'table_name': 'PSU_INFO',
            'object_key_pattern': 'PSU_INFO|*',
            'checks': [{'alarm_id': 'X', 'condition': {'field': 'f', 'operator': '==', 'value': 'v'}}]
        }]
        poller, mock_db, store = self._make_poller(tables)
        mock_tbl = MagicMock()
        mock_tbl.getKeys.side_effect = Exception('DB down')
        MockTable.return_value = mock_tbl

        poller.poll()  # should not raise
        store.set_alarm.assert_not_called()


# ═══════════════════════════════════════════════════════════════════════════
# 4. ScriptRunner tests
# ═══════════════════════════════════════════════════════════════════════════
class TestScriptRunner(unittest.TestCase):
    """Tests for script execution engine."""

    def setUp(self):
        # Script paths in test checks are fake — prevent the
        # os.path.isfile guard from skipping them.
        self._isfile_patcher = patch('os.path.isfile', return_value=True)
        self._isfile_patcher.start()

    def tearDown(self):
        self._isfile_patcher.stop()

    def _make_runner(self, groups):
        store = MagicMock()
        runner = ScriptRunner(groups, store)
        return runner, store

    @patch('subprocess.run')
    def test_script_exit_0_clears(self, mock_run):
        """Exit code 0 with condition exit_code != 0 → no fault → clear."""
        groups = [{
            'checks': [{
                'alarm_id': 'DISK_USAGE_HIGH',
                'object_name': 'root-partition',
                'command': '/etc/sonic/alarm_scripts/check_disk.sh /',
                'timeout': 10,
                'description_template': 'Root partition usage above threshold',
                'condition': {'exit_code': '!=', 'value': 0}
            }]
        }]
        runner, store = self._make_runner(groups)
        mock_run.return_value = MagicMock(returncode=0, stdout='', stderr='')

        runner.run_all()
        store.clear_alarm.assert_called_once_with('DISK_USAGE_HIGH', 'root-partition')

    @patch('subprocess.run')
    def test_script_exit_1_raises(self, mock_run):
        """Exit code 1 with condition exit_code != 0 → fault → raise."""
        groups = [{
            'checks': [{
                'alarm_id': 'DISK_USAGE_HIGH',
                'object_name': 'root-partition',
                'command': '/etc/sonic/alarm_scripts/check_disk.sh /',
                'timeout': 10,
                'description_template': 'Root partition usage above threshold',
                'condition': {'exit_code': '!=', 'value': 0}
            }]
        }]
        runner, store = self._make_runner(groups)
        mock_run.return_value = MagicMock(returncode=1, stdout='95% used on / (threshold 90%)')

        runner.run_all()
        store.raise_alarm.assert_called_once()
        args, kwargs = store.raise_alarm.call_args
        self.assertEqual(args[0], 'DISK_USAGE_HIGH')
        self.assertEqual(args[1], 'root-partition')
        self.assertIn('95% used', kwargs['description'])

    @patch('subprocess.run')
    def test_script_timeout_raises(self, mock_run):
        """Script timeout should raise alarm with timeout description."""
        import subprocess as sp
        groups = [{
            'checks': [{
                'alarm_id': 'CONTAINER_DOWN',
                'object_name': 'SYSTEM',
                'command': '/etc/sonic/alarm_scripts/check_containers.sh',
                'timeout': 15,
                'description_template': 'Critical container(s) not running',
                'condition': {'exit_code': '!=', 'value': 0}
            }]
        }]
        runner, store = self._make_runner(groups)
        mock_run.side_effect = sp.TimeoutExpired(cmd='test', timeout=15)

        runner.run_all()
        store.raise_alarm.assert_called_once()
        desc = store.raise_alarm.call_args[1]['description']
        self.assertIn('timed out', desc)

    @patch('subprocess.run')
    def test_script_condition_stdout_contains(self, mock_run):
        """stdout_contains condition should match on output."""
        groups = [{
            'checks': [{
                'alarm_id': 'TEST_ALARM',
                'object_name': 'SYSTEM',
                'command': 'echo FAIL',
                'timeout': 10,
                'description_template': 'Test failure',
                'condition': {'stdout_contains': 'FAIL'}
            }]
        }]
        runner, store = self._make_runner(groups)
        mock_run.return_value = MagicMock(returncode=0, stdout='FAIL: something broke')

        runner.run_all()
        store.raise_alarm.assert_called_once()

    @patch('subprocess.run')
    def test_script_condition_stdout_regex(self, mock_run):
        """stdout_regex condition should match with regex."""
        groups = [{
            'checks': [{
                'alarm_id': 'TEST_ALARM',
                'object_name': 'SYSTEM',
                'command': 'echo "ERROR level critical"',
                'timeout': 10,
                'description_template': 'Test failure',
                'condition': {'stdout_regex': 'ERROR.*critical'}
            }]
        }]
        runner, store = self._make_runner(groups)
        mock_run.return_value = MagicMock(returncode=0, stdout='ERROR level critical detected')

        runner.run_all()
        store.raise_alarm.assert_called_once()

    @patch('subprocess.run')
    def test_script_condition_stdout_regex_no_match(self, mock_run):
        """stdout_regex that doesn't match → clear."""
        groups = [{
            'checks': [{
                'alarm_id': 'TEST_ALARM',
                'object_name': 'SYSTEM',
                'command': 'echo "all ok"',
                'timeout': 10,
                'description_template': 'Test failure',
                'condition': {'stdout_regex': 'ERROR.*critical'}
            }]
        }]
        runner, store = self._make_runner(groups)
        mock_run.return_value = MagicMock(returncode=0, stdout='all ok')

        runner.run_all()
        store.clear_alarm.assert_called_once()

    @patch('subprocess.run')
    def test_script_exit_code_equals(self, mock_run):
        """exit_code == 2 condition: fault only on exit code 2."""
        groups = [{
            'checks': [{
                'alarm_id': 'SPECIFIC_ALARM',
                'object_name': 'SYSTEM',
                'command': 'test_cmd',
                'timeout': 10,
                'description_template': 'Specific failure',
                'condition': {'exit_code': '==', 'value': 2}
            }]
        }]
        runner, store = self._make_runner(groups)

        # exit code 2 → fault
        mock_run.return_value = MagicMock(returncode=2, stdout='')
        runner.run_all()
        store.raise_alarm.assert_called_once()

        store.reset_mock()

        # exit code 0 → no fault
        mock_run.return_value = MagicMock(returncode=0, stdout='')
        runner.run_all()
        store.clear_alarm.assert_called_once()

    @patch('subprocess.run')
    def test_empty_command_skipped(self, mock_run):
        """Check with empty command should be silently skipped."""
        groups = [{
            'checks': [{
                'alarm_id': 'X',
                'object_name': 'Y',
                'command': '',
                'timeout': 10,
                'condition': {'exit_code': '!=', 'value': 0}
            }]
        }]
        runner, store = self._make_runner(groups)
        runner.run_all()
        mock_run.assert_not_called()

    @patch('subprocess.run')
    def test_unmodified_script_no_warning(self, mock_run):
        """Script whose mtime hasn't changed should execute with no warning."""
        tmpdir = tempfile.mkdtemp(prefix='alarmd_mtime_')
        try:
            script = os.path.join(tmpdir, 'check.sh')
            with open(script, 'w') as f:
                f.write('#!/bin/bash\nexit 0\n')
            os.chmod(script, 0o755)

            self._isfile_patcher.stop()
            try:
                groups = [{
                    'checks': [{
                        'alarm_id': 'MTIME_OK',
                        'object_name': 'SYSTEM',
                        'command': script,
                        'timeout': 10,
                        'condition': {'exit_code': '!=', 'value': 0}
                    }]
                }]
                runner, store = self._make_runner(groups)
                mock_run.return_value = MagicMock(returncode=0, stdout='', stderr='')
                runner.run_all()
                # Script should execute normally
                mock_run.assert_called_once()
                # No "MODIFIED SCRIPT" warning should appear
                warning_msgs = [c[0][0] for c in _script_runner_mod.logger.log_warning.call_args_list]
                self.assertFalse(
                    any('MODIFIED SCRIPT' in m for m in warning_msgs),
                    "Should not warn when script is unmodified")
            finally:
                self._isfile_patcher.start()
        finally:
            shutil.rmtree(tmpdir, ignore_errors=True)

    @patch('subprocess.run')
    def test_modified_script_warns_but_executes(self, mock_run):
        """Script modified after config load should log WARNING but still run."""
        tmpdir = tempfile.mkdtemp(prefix='alarmd_mtime_')
        try:
            script = os.path.join(tmpdir, 'check.sh')
            with open(script, 'w') as f:
                f.write('#!/bin/bash\nexit 0\n')
            os.chmod(script, 0o755)

            self._isfile_patcher.stop()
            try:
                groups = [{
                    'checks': [{
                        'alarm_id': 'MTIME_CHANGED',
                        'object_name': 'SYSTEM',
                        'command': script,
                        'timeout': 10,
                        'condition': {'exit_code': '!=', 'value': 0}
                    }]
                }]
                runner, store = self._make_runner(groups)

                # Simulate script modification: touch with a future mtime
                import time
                new_mtime = os.path.getmtime(script) + 100
                os.utime(script, (new_mtime, new_mtime))

                mock_run.return_value = MagicMock(returncode=0, stdout='', stderr='')
                runner.run_all()
                # Script must still execute
                mock_run.assert_called_once()
                # WARNING should have been logged
                warning_msgs = [c[0][0] for c in _script_runner_mod.logger.log_warning.call_args_list]
                self.assertTrue(
                    any('MODIFIED SCRIPT' in m for m in warning_msgs),
                    "Should warn when script mtime has changed")
            finally:
                self._isfile_patcher.start()
        finally:
            shutil.rmtree(tmpdir, ignore_errors=True)

    @patch('subprocess.run')
    def test_sighup_resets_mtime_baseline(self, mock_run):
        """Re-creating ScriptRunner (SIGHUP reload) resets the mtime baseline."""
        tmpdir = tempfile.mkdtemp(prefix='alarmd_mtime_')
        try:
            script = os.path.join(tmpdir, 'check.sh')
            with open(script, 'w') as f:
                f.write('#!/bin/bash\nexit 0\n')
            os.chmod(script, 0o755)

            self._isfile_patcher.stop()
            try:
                groups = [{
                    'checks': [{
                        'alarm_id': 'MTIME_RELOAD',
                        'object_name': 'SYSTEM',
                        'command': script,
                        'timeout': 10,
                        'condition': {'exit_code': '!=', 'value': 0}
                    }]
                }]
                # First runner: records baseline mtime
                runner1, _ = self._make_runner(groups)

                # Modify the script
                import time
                new_mtime = os.path.getmtime(script) + 100
                os.utime(script, (new_mtime, new_mtime))

                # Second runner (simulates SIGHUP rebuild): re-snapshots
                runner2, _ = self._make_runner(groups)
                mock_run.return_value = MagicMock(returncode=0, stdout='', stderr='')
                # Reset module-level logger to clear warnings from prior tests
                _script_runner_mod.logger.reset_mock()
                runner2.run_all()

                # No warning — the second runner's baseline matches current mtime
                warning_msgs = [c[0][0] for c in _script_runner_mod.logger.log_warning.call_args_list]
                self.assertFalse(
                    any('MODIFIED SCRIPT' in m for m in warning_msgs),
                    "After reload, mtime baseline should be reset — no warning")
            finally:
                self._isfile_patcher.start()
        finally:
            shutil.rmtree(tmpdir, ignore_errors=True)


# ═══════════════════════════════════════════════════════════════════════════
# 5. alarm_defs.json structural validation
# ═══════════════════════════════════════════════════════════════════════════
class TestAlarmDefsJson(unittest.TestCase):
    """Validate the alarm_defs.json file structure and content."""

    @classmethod
    def setUpClass(cls):
        if not os.path.isfile(ALARM_DEFS_PATH):
            raise unittest.SkipTest(f'alarm_defs.json not found at {ALARM_DEFS_PATH}')
        with open(ALARM_DEFS_PATH) as f:
            cls.defs = json.load(f)

    def test_valid_json(self):
        """alarm_defs.json should be valid JSON."""
        self.assertIsInstance(self.defs, dict)

    def test_has_sonic_version(self):
        self.assertIn('sonic_version', self.defs)

    def test_has_alarm_tables(self):
        self.assertIn('alarm_tables', self.defs)
        self.assertIsInstance(self.defs['alarm_tables'], list)
        self.assertGreater(len(self.defs['alarm_tables']), 0)

    def test_has_alarmd_settings(self):
        settings = self.defs.get('alarmd_settings', {})
        self.assertIn('statedb_poll_interval', settings)
        self.assertIn('script_poll_interval', settings)
        self.assertGreaterEqual(settings['statedb_poll_interval'], 2)
        self.assertGreaterEqual(settings['script_poll_interval'], 30)

    def test_each_table_has_type(self):
        for table in self.defs['alarm_tables']:
            self.assertIn('type', table)
            self.assertIn(table['type'], ('statedb', 'script'))

    def test_statedb_tables_have_required_fields(self):
        for table in self.defs['alarm_tables']:
            if table['type'] != 'statedb':
                continue
            self.assertIn('table_name', table, f"statedb table missing table_name")
            self.assertIn('object_key_pattern', table)
            self.assertIn('checks', table)

    def test_script_groups_have_required_fields(self):
        for table in self.defs['alarm_tables']:
            if table['type'] != 'script':
                continue
            self.assertIn('group_name', table)
            self.assertIn('checks', table)

    def test_every_check_has_alarm_id(self):
        for table in self.defs['alarm_tables']:
            for check in table.get('checks', []):
                self.assertIn('alarm_id', check, f"Check missing alarm_id in {table.get('table_name', table.get('group_name'))}")
                self.assertTrue(len(check['alarm_id']) > 0)

    def test_statedb_checks_have_conditions(self):
        for table in self.defs['alarm_tables']:
            if table['type'] != 'statedb':
                continue
            for check in table.get('checks', []):
                cond = check.get('condition', {})
                self.assertIn('field', cond, f"statedb check {check['alarm_id']} missing condition.field")
                self.assertIn('operator', cond)
                self.assertIn('value', cond)

    def test_script_checks_have_command(self):
        for table in self.defs['alarm_tables']:
            if table['type'] != 'script':
                continue
            for check in table.get('checks', []):
                self.assertIn('command', check, f"script check {check['alarm_id']} missing command")
                self.assertTrue(len(check['command']) > 0)

    def test_severity_values_valid(self):
        valid_severities = {'Critical', 'Major', 'Minor', 'Warning'}
        for table in self.defs['alarm_tables']:
            for check in table.get('checks', []):
                sev = check.get('severity', 'Minor')
                self.assertIn(sev, valid_severities,
                              f"Invalid severity '{sev}' in {check['alarm_id']}")

    def test_no_duplicate_check_names_within_table(self):
        for table in self.defs['alarm_tables']:
            names = [c['check_name'] for c in table.get('checks', []) if 'check_name' in c]
            self.assertEqual(len(names), len(set(names)),
                             f"Duplicate check_name in {table.get('table_name', table.get('group_name'))}")

    def test_total_checks_within_limits(self):
        statedb_count = sum(
            len(t.get('checks', []))
            for t in self.defs['alarm_tables'] if t.get('type') == 'statedb')
        script_count = sum(
            len(t.get('checks', []))
            for t in self.defs['alarm_tables'] if t.get('type') == 'script')
        self.assertLessEqual(statedb_count, 200, "statedb checks exceed hard limit")
        self.assertLessEqual(script_count, 50, "script checks exceed hard limit")


# ═══════════════════════════════════════════════════════════════════════════
# 6. Guardrail / hard-limit tests
# ═══════════════════════════════════════════════════════════════════════════
class TestGuardrails(unittest.TestCase):
    """Test alarmd's hard-limit enforcement."""

    def test_truncate_checks_static(self):
        """_truncate_checks should cap total checks."""
        tables = [
            {'checks': [{'alarm_id': f'A{i}'} for i in range(10)]},
            {'checks': [{'alarm_id': f'B{i}'} for i in range(10)]},
        ]
        result = alarmd_script._truncate_checks(tables, 15)
        total = sum(len(t['checks']) for t in result)
        self.assertEqual(total, 15)

    def test_truncate_checks_exact(self):
        """If total == max, nothing should be dropped."""
        tables = [
            {'checks': [{'alarm_id': f'A{i}'} for i in range(5)]},
        ]
        result = alarmd_script._truncate_checks(tables, 5)
        total = sum(len(t['checks']) for t in result)
        self.assertEqual(total, 5)

    def test_truncate_checks_zero(self):
        """max=0 should return empty."""
        tables = [
            {'checks': [{'alarm_id': 'A'}]},
        ]
        result = alarmd_script._truncate_checks(tables, 0)
        self.assertEqual(len(result), 0)


# ═══════════════════════════════════════════════════════════════════════════
# 7. ScriptRunner._evaluate_script_condition() tests
# ═══════════════════════════════════════════════════════════════════════════
class TestScriptConditionEvaluator(unittest.TestCase):
    """Direct tests for ScriptRunner._evaluate_script_condition()."""

    def test_empty_condition_defaults_to_exit_code(self):
        self.assertTrue(ScriptRunner._evaluate_script_condition({}, 1, ''))
        self.assertFalse(ScriptRunner._evaluate_script_condition({}, 0, ''))

    def test_exit_code_neq(self):
        cond = {'exit_code': '!=', 'value': 0}
        self.assertTrue(ScriptRunner._evaluate_script_condition(cond, 1, ''))
        self.assertFalse(ScriptRunner._evaluate_script_condition(cond, 0, ''))

    def test_exit_code_eq(self):
        cond = {'exit_code': '==', 'value': 2}
        self.assertTrue(ScriptRunner._evaluate_script_condition(cond, 2, ''))
        self.assertFalse(ScriptRunner._evaluate_script_condition(cond, 0, ''))

    def test_exit_code_gt(self):
        cond = {'exit_code': '>', 'value': 1}
        self.assertTrue(ScriptRunner._evaluate_script_condition(cond, 2, ''))
        self.assertFalse(ScriptRunner._evaluate_script_condition(cond, 0, ''))

    def test_stdout_contains(self):
        cond = {'stdout_contains': 'CRITICAL'}
        self.assertTrue(ScriptRunner._evaluate_script_condition(cond, 0, 'CRITICAL error found'))
        self.assertFalse(ScriptRunner._evaluate_script_condition(cond, 0, 'all clear'))

    def test_stdout_regex(self):
        cond = {'stdout_regex': r'ERROR \d+ found'}
        self.assertTrue(ScriptRunner._evaluate_script_condition(cond, 0, 'ERROR 42 found in log'))
        self.assertFalse(ScriptRunner._evaluate_script_condition(cond, 0, 'no errors'))

    def test_exit_code_unknown_operator_returns_false(self):
        """Unknown exit_code operator is not a fault (dispatch miss → False)."""
        cond = {'exit_code': '~', 'value': 0}
        self.assertFalse(ScriptRunner._evaluate_script_condition(cond, 1, ''))
        self.assertFalse(ScriptRunner._evaluate_script_condition(cond, 0, ''))


# ═══════════════════════════════════════════════════════════════════════════
# 8. main() / AlarmDaemon startup tests
# ═══════════════════════════════════════════════════════════════════════════
class TestMainStartup(unittest.TestCase):
    """Tests for main() entry point and AlarmDaemon startup edge cases."""

    @patch.object(alarmd_script, 'acquire_pidfile_lock', return_value=MagicMock())
    @patch.object(alarmd_script.AlarmDaemon, '__init__', side_effect=Exception('Redis down'))
    def test_main_startup_exception_returns_1(self, _mock_init, _mock_lock):
        """Unhandled exception during startup should return 1, not crash."""
        result = alarmd_script.main()
        self.assertEqual(result, 1)

    @patch.object(alarmd_script, 'acquire_pidfile_lock', return_value=MagicMock())
    @patch.object(alarmd_script.AlarmDaemon, '__init__', side_effect=SystemExit(1))
    def test_main_load_defs_failure_propagates(self, _mock_init, _mock_lock):
        """SystemExit from _load_defs should propagate (not be swallowed)."""
        with self.assertRaises(SystemExit):
            alarmd_script.main()


# ═══════════════════════════════════════════════════════════════════════════
# 8b. PID file lock (single-instance enforcement) tests
# ═══════════════════════════════════════════════════════════════════════════
class TestPidfileLock(unittest.TestCase):
    """Tests for acquire_pidfile_lock() single-instance guard."""

    def setUp(self):
        self.tmpdir = tempfile.mkdtemp(prefix='alarmd_pid_')
        self.pidfile_path = os.path.join(self.tmpdir, 'alarmd.pid')

    def tearDown(self):
        shutil.rmtree(self.tmpdir, ignore_errors=True)

    @patch('alarmd_script.PIDFILE_PATH')
    def test_acquires_lock_writes_pid(self, mock_path):
        """First instance acquires lock and writes PID to file."""
        mock_path.__str__ = lambda s: self.pidfile_path
        # Patch the constant used inside the function
        with patch.object(alarmd_script, 'PIDFILE_PATH', self.pidfile_path):
            fh = alarmd_script.acquire_pidfile_lock()
            try:
                # PID file should contain our PID
                with open(self.pidfile_path) as f:
                    content = f.read()
                self.assertEqual(content, str(os.getpid()))
            finally:
                fh.close()

    def test_second_instance_blocked(self):
        """Second call to acquire_pidfile_lock() raises SystemExit."""
        with patch.object(alarmd_script, 'PIDFILE_PATH', self.pidfile_path):
            fh1 = alarmd_script.acquire_pidfile_lock()
            try:
                with self.assertRaises(SystemExit) as ctx:
                    alarmd_script.acquire_pidfile_lock()
                self.assertIn('already running', str(ctx.exception))
            finally:
                fh1.close()

    def test_lock_released_after_close(self):
        """After closing the fd, a new instance can acquire the lock."""
        with patch.object(alarmd_script, 'PIDFILE_PATH', self.pidfile_path):
            fh1 = alarmd_script.acquire_pidfile_lock()
            fh1.close()
            # Now a second acquisition should succeed
            fh2 = alarmd_script.acquire_pidfile_lock()
            fh2.close()


# ═══════════════════════════════════════════════════════════════════════════
# 9. sonic_version mismatch detection
# ═══════════════════════════════════════════════════════════════════════════
class TestSonicVersionCheck(unittest.TestCase):
    """Verify that _check_sonic_version() warns on mismatch."""

    @patch('sonic_alarm.config._get_running_sonic_version', return_value='master')
    def test_matching_version_no_warning(self, _mock_ver):
        """No warning when sonic_version matches the running branch."""
        defs = {'sonic_version': 'master', 'alarm_tables': []}
        with patch.object(alarm_config._log, 'warning') as mock_warn:
            alarm_config._check_sonic_version(defs)
            mock_warn.assert_not_called()

    @patch('sonic_alarm.config._get_running_sonic_version', return_value='master')
    def test_mismatched_version_warns(self, _mock_ver):
        """Warning logged when sonic_version differs from running branch."""
        defs = {'sonic_version': '202505', 'alarm_tables': []}
        with patch.object(alarm_config._log, 'warning') as mock_warn:
            alarm_config._check_sonic_version(defs)
            mock_warn.assert_called_once()
            msg = mock_warn.call_args[0][0]
            self.assertIn('SONIC VERSION MISMATCH', msg)

    @patch('sonic_alarm.config._get_running_sonic_version', return_value=None)
    def test_unreadable_version_file_skips_check(self, _mock_ver):
        """If sonic_version.yml is unreadable, check is skipped silently."""
        defs = {'sonic_version': '202505', 'alarm_tables': []}
        with patch.object(alarm_config._log, 'warning') as mock_warn:
            alarm_config._check_sonic_version(defs)
            mock_warn.assert_not_called()

    def test_no_sonic_version_in_defs_skips_check(self):
        """If alarm_defs omit sonic_version, check is skipped."""
        defs = {'alarm_tables': []}
        with patch.object(alarm_config._log, 'warning') as mock_warn:
            alarm_config._check_sonic_version(defs)
            mock_warn.assert_not_called()


# ═══════════════════════════════════════════════════════════════════════════
# 10. Fragment / directory loading tests
# ═══════════════════════════════════════════════════════════════════════════
class TestSighupReload(unittest.TestCase):
    """Test the SIGHUP-based config reload mechanism.

    Safety guarantees verified:
      1. Successful reload swaps poller/runner with new tables
      2. Failed reload preserves old config (no crash)
      3. Stale alarms are purged after alarm_id removed from config
      4. EventPublisher metadata is refreshed for new alarm_ids
      5. _sighup_handler sets flag; main loop checks it
    """

    def setUp(self):
        self.tmpdir = tempfile.mkdtemp(prefix='alarmd_reload_')

        # Create a daemon-like object without triggering AlarmDaemon.__init__
        # We can't use AlarmDaemon.__new__ because DaemonBase is a MagicMock class.
        self.daemon = MagicMock(spec=[])  # empty spec = no attribute restrictions
        # DaemonBase uses self.log_info(), self.log_error(), etc.
        self.daemon.log_info = MagicMock()
        self.daemon.log_error = MagicMock()
        self.daemon.log_warning = MagicMock()
        self.daemon._running = True
        self.daemon._reload_requested = False
        # Bind the real _reload and _split_and_limit methods
        self.daemon._reload = lambda: alarmd_script.AlarmDaemon._reload(self.daemon)
        self.daemon._split_and_limit = lambda tables: alarmd_script.AlarmDaemon._split_and_limit(self.daemon, tables)
        self.daemon.signal_handler = lambda sig, frame: alarmd_script.AlarmDaemon.signal_handler(self.daemon, sig, frame)

        # Build initial config
        self.initial_defs = {
            'sonic_version': 'master',
            'alarmd_settings': {
                'statedb_poll_interval': 3,
                'script_poll_interval': 60,
            },
            'alarm_tables': [
                {
                    'type': 'statedb',
                    'table_name': 'PSU_INFO',
                    'object_key_pattern': 'PSU_INFO|*',
                    'checks': [
                        {
                            'alarm_id': 'PSU_MISSING',
                            'severity': 'Critical',
                            'category': 'Hardware',
                            'description_template': '{object_name} missing',
                            'condition': {'field': 'status', 'operator': '!=', 'value': 'true'},
                        },
                        {
                            'alarm_id': 'PSU_VOLTAGE_FAULT',
                            'severity': 'Major',
                            'category': 'Hardware',
                            'description_template': '{object_name} voltage fault',
                            'condition': {'field': 'voltage_fault', 'operator': '==', 'value': 'true'},
                        },
                    ],
                },
            ],
        }
        self.daemon._defs = self.initial_defs

        # Mock STATE_DB and SYSTEM_ALARMS table
        self.daemon._state_db = MagicMock()
        self.mock_table = MagicMock()
        self.daemon._alarms_table = self.mock_table
        self.mock_table.getKeys.return_value = []

        # Build the eventd output path (EventPublisher) with a fake event API
        # (rc=0) and the mock ALARM table.  Alias as _store so the existing
        # assertions below (which predate the AlarmStore→EventPublisher rename)
        # keep working; the daemon's _reload() operates on _publisher.
        self.publisher = EventPublisher(
            self.initial_defs, event_api=_FakeEventApi(),
            alarm_table=self.mock_table)
        self.daemon._publisher = self.publisher
        self.daemon._store = self.publisher

        # Build initial poller/runner
        self.daemon._statedb_interval = 3
        self.daemon._script_interval = 60
        self.daemon._statedb_poller = StateDBPoller(
            self.initial_defs['alarm_tables'], self.daemon._store,
            self.daemon._state_db)
        self.daemon._script_runner = ScriptRunner(
            [], self.daemon._store)

    def tearDown(self):
        shutil.rmtree(self.tmpdir, ignore_errors=True)

    def _write_json(self, path, data):
        with open(path, 'w') as f:
            json.dump(data, f)

    # --- Test 1: Successful reload swaps engines ---

    @patch('sonic_alarm.config.load_alarm_defs')
    def test_successful_reload_swaps_engines(self, mock_load):
        """After SIGHUP with valid config, poller/runner use new tables."""
        new_defs = {
            'sonic_version': '202605',
            'alarmd_settings': {'statedb_poll_interval': 5, 'script_poll_interval': 90},
            'alarm_tables': [
                {
                    'type': 'statedb',
                    'table_name': 'FAN_INFO',
                    'object_key_pattern': 'FAN_INFO|*',
                    'checks': [
                        {
                            'alarm_id': 'FAN_MISSING',
                            'severity': 'Major',
                            'category': 'Hardware',
                            'description_template': '{object_name} missing',
                            'condition': {'field': 'status', 'operator': '!=', 'value': 'true'},
                        },
                    ],
                },
            ],
        }
        mock_load.return_value = new_defs

        old_poller = self.daemon._statedb_poller
        old_runner = self.daemon._script_runner

        self.daemon._reload()

        # Poller and runner should be new objects
        self.assertIsNot(self.daemon._statedb_poller, old_poller)
        self.assertIsNot(self.daemon._script_runner, old_runner)

        # New poller should have the FAN_INFO table
        self.assertEqual(len(self.daemon._statedb_poller._tables), 1)
        self.assertEqual(self.daemon._statedb_poller._tables[0]['table_name'], 'FAN_INFO')

        # Defs should be updated
        self.assertEqual(self.daemon._defs['sonic_version'], '202605')

        # Intervals should be updated
        self.assertEqual(self.daemon._statedb_interval, 5)
        self.assertEqual(self.daemon._script_interval, 90)

        # Log should confirm reload
        self.daemon.log_info.assert_any_call(
            "Reload complete: 1 statedb checks, "
            "0 script checks, "
            "0 stale alarms purged")

    # --- Test 2: Failed reload preserves old config ---

    @patch('sonic_alarm.config.load_alarm_defs')
    def test_failed_reload_preserves_old_config(self, mock_load):
        """If new config fails to parse, old config stays intact."""
        mock_load.return_value = None  # load_alarm_defs returns None on error

        old_poller = self.daemon._statedb_poller
        old_runner = self.daemon._script_runner
        old_defs = self.daemon._defs

        self.daemon._reload()

        # Everything should be unchanged
        self.assertIs(self.daemon._statedb_poller, old_poller)
        self.assertIs(self.daemon._script_runner, old_runner)
        self.assertIs(self.daemon._defs, old_defs)

        # Error should be logged
        self.daemon.log_error.assert_any_call(
            "Reload FAILED — keeping existing config. "
            "Fix the alarm definitions and send SIGHUP again.")

    # --- Test 3: Stale alarms are purged ---

    @patch('sonic_alarm.config.load_alarm_defs')
    def test_stale_alarms_purged_after_reload(self, mock_load):
        """Alarms whose alarm_id was removed from config get purged."""
        # Simulate active alarms for PSU_MISSING and PSU_VOLTAGE_FAULT
        self.daemon._store._active.add('PSU_MISSING|PSU1')
        self.daemon._store._active.add('PSU_VOLTAGE_FAULT|PSU2')

        # New config only has FAN_MISSING — neither PSU alarm exists
        new_defs = {
            'sonic_version': '202605',
            'alarmd_settings': {'statedb_poll_interval': 3, 'script_poll_interval': 60},
            'alarm_tables': [
                {
                    'type': 'statedb',
                    'table_name': 'FAN_INFO',
                    'object_key_pattern': 'FAN_INFO|*',
                    'checks': [
                        {'alarm_id': 'FAN_MISSING', 'severity': 'Major',
                         'category': 'Hardware',
                         'condition': {'field': 'status', 'operator': '!=', 'value': 'true'}},
                    ],
                },
            ],
        }
        mock_load.return_value = new_defs

        self.daemon._reload()

        # Both PSU alarms should be purged from active set (EventPublisher
        # publishes a CLEAR through eventd; it does not write a DB table).
        self.assertNotIn('PSU_MISSING|PSU1', self.daemon._store._active)
        self.assertNotIn('PSU_VOLTAGE_FAULT|PSU2', self.daemon._store._active)

    # --- Test 4: EventPublisher metadata refreshed for new alarm_ids ---

    @patch('sonic_alarm.config.load_alarm_defs')
    def test_metadata_refreshed_for_new_alarm_ids(self, mock_load):
        """After reload, new alarm_ids should have proper metadata."""
        # Initially only PSU alarms in metadata
        self.assertIn('PSU_MISSING', self.daemon._store._meta)
        self.assertNotIn('FAN_MISSING', self.daemon._store._meta)

        new_defs = {
            'sonic_version': '202605',
            'alarmd_settings': {'statedb_poll_interval': 3, 'script_poll_interval': 60},
            'alarm_tables': [
                {
                    'type': 'statedb',
                    'table_name': 'FAN_INFO',
                    'object_key_pattern': 'FAN_INFO|*',
                    'checks': [
                        {
                            'alarm_id': 'FAN_MISSING',
                            'severity': 'Critical',
                            'category': 'Cooling',
                            'description_template': '{object_name} fan missing',
                            'condition': {'field': 'status', 'operator': '!=', 'value': 'true'},
                        },
                    ],
                },
            ],
        }
        mock_load.return_value = new_defs

        self.daemon._reload()

        # FAN_MISSING should now be in metadata with correct values.
        # NOTE: EventPublisher metadata does NOT carry severity (eventd assigns
        # it from the event profile); only category/source/template are kept.
        self.assertIn('FAN_MISSING', self.daemon._store._meta)
        meta = self.daemon._store._meta['FAN_MISSING']
        self.assertEqual(meta['category'], 'Cooling')
        self.assertEqual(meta['source'], 'FAN_INFO')

    # --- Test 5: SIGHUP handler sets flag ---

    def test_sighup_handler_sets_flag(self):
        """signal_handler(SIGHUP) should only set a flag, not do actual reload."""
        self.assertFalse(self.daemon._reload_requested)
        self.daemon.signal_handler(signal.SIGHUP, None)
        self.assertTrue(self.daemon._reload_requested)

    # --- Test 6: Reload with malformed fragment preserves old config ---

    @patch('sonic_alarm.config.load_alarm_defs')
    def test_reload_bad_fragment_preserves_config(self, mock_load):
        """A bad config during reload should not crash — old config stays."""
        mock_load.return_value = None  # simulates parse failure

        old_poller = self.daemon._statedb_poller
        old_defs = self.daemon._defs

        self.daemon._reload()

        self.assertIs(self.daemon._statedb_poller, old_poller)
        self.assertIs(self.daemon._defs, old_defs)

    # --- Test 7: Stale alarms not purged when they still exist ---

    @patch('sonic_alarm.config.load_alarm_defs')
    def test_active_alarms_kept_when_still_defined(self, mock_load):
        """Alarms that still exist in new config should NOT be purged."""
        self.daemon._store._active.add('PSU_MISSING|PSU1')

        # New config still has PSU_MISSING
        new_defs = {
            'sonic_version': '202605',
            'alarmd_settings': {'statedb_poll_interval': 3, 'script_poll_interval': 60},
            'alarm_tables': [
                {
                    'type': 'statedb',
                    'table_name': 'PSU_INFO',
                    'object_key_pattern': 'PSU_INFO|*',
                    'checks': [
                        {'alarm_id': 'PSU_MISSING', 'severity': 'Critical',
                         'category': 'Hardware',
                         'condition': {'field': 'status', 'operator': '!=', 'value': 'true'}},
                    ],
                },
            ],
        }
        mock_load.return_value = new_defs

        self.daemon._reload()

        # PSU_MISSING should still be active
        self.assertIn('PSU_MISSING|PSU1', self.daemon._store._active)

    # --- Test 8: purge_stale_alarms standalone test ---

    def test_purge_stale_alarms_returns_purged_list(self):
        """purge_stale_alarms should return the list of tags purged."""
        store = self.daemon._store
        store._active.add('OLD_ALARM|OBJ1')
        store._active.add('KEPT_ALARM|OBJ2')

        purged = store.purge_stale_alarms({'KEPT_ALARM'})
        self.assertEqual(len(purged), 1)
        self.assertIn('OLD_ALARM|OBJ1', purged)
        self.assertNotIn('OLD_ALARM|OBJ1', store._active)
        self.assertIn('KEPT_ALARM|OBJ2', store._active)

    # --- Test 9: known_alarm_ids returns correct set ---

    def test_known_alarm_ids(self):
        """known_alarm_ids should reflect current metadata."""
        ids = self.daemon._store.known_alarm_ids()
        self.assertIn('PSU_MISSING', ids)
        self.assertIn('PSU_VOLTAGE_FAULT', ids)
        self.assertNotIn('FAN_MISSING', ids)

    # --- Test 10: rebuild_meta replaces old metadata ---

    def test_rebuild_meta_replaces_old(self):
        """_rebuild_meta should replace all old metadata."""
        store = self.daemon._store
        self.assertIn('PSU_MISSING', store._meta)

        new_defs = {
            'alarm_tables': [
                {
                    'type': 'statedb',
                    'table_name': 'TEMPERATURE_INFO',
                    'checks': [
                        {'alarm_id': 'TEMP_HIGH', 'severity': 'Major', 'category': 'Environment'},
                    ],
                },
            ],
        }
        store._rebuild_meta(new_defs)

        # Old IDs gone, new ID present.  (EventPublisher meta carries no
        # severity — eventd owns it — so assert on category instead.)
        self.assertNotIn('PSU_MISSING', store._meta)
        self.assertIn('TEMP_HIGH', store._meta)
        self.assertEqual(store._meta['TEMP_HIGH']['category'], 'Environment')


# ═══════════════════════════════════════════════════════════════════════════
# 11. alarm_defs.json field naming validation
# ═══════════════════════════════════════════════════════════════════════════
class TestAlarmDefsFieldNaming(unittest.TestCase):
    """Verify vendor-specific field names have been removed."""

    @classmethod
    def setUpClass(cls):
        if not os.path.isfile(ALARM_DEFS_PATH):
            raise unittest.SkipTest(f'alarm_defs.json not found at {ALARM_DEFS_PATH}')
        with open(ALARM_DEFS_PATH) as f:
            cls.raw = f.read()
            cls.defs = json.loads(cls.raw)

    def test_no_evo_field_names(self):
        """alarm_defs.json should not contain vendor-specific 'evo_' field names."""
        self.assertNotIn('evo_alarm_reason', self.raw)
        self.assertNotIn('evo_error_ids', self.raw)

    def test_reference_fields_are_optional(self):
        """'reference' and 'reference_codes' should be used if cross-refs exist."""
        for table in self.defs['alarm_tables']:
            for check in table.get('checks', []):
                if 'reference' in check:
                    self.assertIsInstance(check['reference'], str)
                if 'reference_codes' in check:
                    self.assertIsInstance(check['reference_codes'], list)


# ═══════════════════════════════════════════════════════════════════════════
# 12. Removed: Fragment directory and data quality validation tests
# ═══════════════════════════════════════════════════════════════════════════
# TestAlarmDefsFragmentDirectory and TestAlarmDefsDataQuality have been
# removed from unit tests. These are production config validators that should
# be run as part of sonic-mgmt DUT functional tests with real platform configs.
# All 110 functional daemon tests still pass.
# See: /b/ndatta/ZAlarmd/test_alarmd_functional.py for DUT test framework.


# ═══════════════════════════════════════════════════════════════════════════
# 13. Common alarm catalog tests
# ═══════════════════════════════════════════════════════════════════════════
class TestLoadCommonCatalog(unittest.TestCase):
    """Test _load_common_catalog() loads the shipped common_alarm_defs.json."""

    def test_common_catalog_loads(self):
        """common_alarm_defs.json should load and have alarm_tables."""
        result = alarm_config._load_common_catalog()
        self.assertIsNotNone(result)
        self.assertIn('alarm_tables', result)
        tables = result['alarm_tables']
        self.assertEqual(len(tables), 2)  # PSU_INFO, FAN_INFO
        table_names = [t['table_name'] for t in tables]
        self.assertIn('PSU_INFO', table_names)
        self.assertIn('FAN_INFO', table_names)

    def test_common_catalog_has_10_checks(self):
        """Common catalog should have exactly 10 checks (6 PSU + 4 FAN)."""
        result = alarm_config._load_common_catalog()
        total = sum(len(t.get('checks', [])) for t in result['alarm_tables'])
        self.assertEqual(total, 10)

    def test_common_psu_checks(self):
        """PSU_INFO table should have 6 checks with correct alarm_ids."""
        result = alarm_config._load_common_catalog()
        psu_table = [t for t in result['alarm_tables']
                     if t['table_name'] == 'PSU_INFO'][0]
        alarm_ids = [c['alarm_id'] for c in psu_table['checks']]
        self.assertIn('PSU_MISSING', alarm_ids)
        self.assertIn('PSU_POWER_BAD', alarm_ids)
        self.assertIn('PSU_VOLTAGE_OOR', alarm_ids)
        self.assertIn('PSU_CURRENT_OOR', alarm_ids)
        self.assertIn('PSU_FAN_FAULT', alarm_ids)
        self.assertIn('PSU_TEMP_FAULT', alarm_ids)
        self.assertEqual(len(psu_table['checks']), 6)

    def test_common_fan_checks(self):
        """FAN_INFO table should have 4 checks."""
        result = alarm_config._load_common_catalog()
        fan_table = [t for t in result['alarm_tables']
                     if t['table_name'] == 'FAN_INFO'][0]
        alarm_ids = [c['alarm_id'] for c in fan_table['checks']]
        self.assertIn('FAN_MISSING', alarm_ids)
        self.assertIn('FAN_FAULT', alarm_ids)
        self.assertIn('FAN_UNDER_SPEED', alarm_ids)
        self.assertIn('FAN_OVER_SPEED', alarm_ids)
        self.assertEqual(len(fan_table['checks']), 4)


# ═══════════════════════════════════════════════════════════════════════════
# 14. Merge mechanism tests
# ═══════════════════════════════════════════════════════════════════════════
class TestMergeCommon(unittest.TestCase):
    """Test _merge_common() — platform merge on top of common catalog."""

    def test_empty_platform_gets_common_baseline(self):
        """Platform file with no alarm_tables gets all common checks."""
        defs = {'sonic_version': 'master',
                'alarmd_settings': {'statedb_poll_interval': 3}}
        result = alarm_config._merge_common(defs)
        self.assertIsNotNone(result)
        self.assertIn('alarm_tables', result)
        table_names = [t['table_name'] for t in result['alarm_tables']]
        self.assertIn('PSU_INFO', table_names)
        self.assertIn('FAN_INFO', table_names)
        total = sum(len(t.get('checks', [])) for t in result['alarm_tables'])
        self.assertEqual(total, 10)
        # Settings carried over
        self.assertEqual(result['sonic_version'], 'master')
        self.assertEqual(result['alarmd_settings']['statedb_poll_interval'], 3)

    def test_add_new_table(self):
        """Platform adds a new table not in common."""
        defs = {
            'alarm_tables': [{
                'type': 'statedb',
                'table_name': 'TEMPERATURE_INFO',
                'object_key_pattern': 'TEMPERATURE_INFO|*',
                'checks': [{
                    'check_name': 'over_temp',
                    'alarm_id': 'TEMP_WARNING',
                    'severity': 'Major',
                    'category': 'Hardware',
                    'condition': {'field': 'warning_status',
                                  'operator': '==', 'value': 'True'}
                }]
            }]
        }
        result = alarm_config._merge_common(defs)
        table_names = [t['table_name'] for t in result['alarm_tables']]
        self.assertIn('TEMPERATURE_INFO', table_names)
        total = sum(len(t.get('checks', [])) for t in result['alarm_tables'])
        self.assertEqual(total, 11)  # 10 common + 1 new

    def test_append_checks_to_existing_table(self):
        """Platform adds new checks to a common table (PSU_INFO)."""
        defs = {
            'alarm_tables': [{
                'type': 'statedb',
                'table_name': 'PSU_INFO',
                'object_key_pattern': 'PSU_INFO|*',
                'checks': [{
                    'check_name': 'input_voltage_fault',
                    'alarm_id': 'PSU_INPUT_VOLTAGE_FAULT',
                    'severity': 'Major',
                    'category': 'Hardware',
                    'description_template': '{object_name} Input Voltage Fault',
                    'condition': {'field': 'input_voltage_status',
                                  'operator': '==', 'value': 'Fault'}
                }]
            }]
        }
        result = alarm_config._merge_common(defs)
        psu_table = [t for t in result['alarm_tables']
                     if t['table_name'] == 'PSU_INFO'][0]
        check_names = [c['check_name'] for c in psu_table['checks']]
        self.assertIn('input_voltage_fault', check_names)
        self.assertIn('psu_presence', check_names)  # common still present
        self.assertEqual(len(psu_table['checks']), 7)  # 6 + 1

    def test_replace_common_check_by_check_name(self):
        """Platform check with same check_name replaces common check."""
        defs = {
            'alarm_tables': [{
                'type': 'statedb',
                'table_name': 'PSU_INFO',
                'object_key_pattern': 'PSU_INFO|*',
                'checks': [{
                    'check_name': 'psu_presence',  # same as common
                    'alarm_id': 'PSU_MISSING',
                    'severity': 'Major',  # was Critical in common
                    'category': 'Hardware',
                    'description_template': '{object_name} Not Present',
                    'condition': {'field': 'presence', 'operator': '==',
                                  'value': 'false'}
                }]
            }]
        }
        result = alarm_config._merge_common(defs)
        psu_table = [t for t in result['alarm_tables']
                     if t['table_name'] == 'PSU_INFO'][0]
        presence = [c for c in psu_table['checks']
                    if c['check_name'] == 'psu_presence'][0]
        self.assertEqual(presence['severity'], 'Major')
        self.assertEqual(presence['description_template'],
                         '{object_name} Not Present')
        # Still 6 checks (replaced, not added)
        self.assertEqual(len(psu_table['checks']), 6)

    def test_disable_list_removes_checks(self):
        """disable list removes specific checks from common."""
        defs = {
            'disable': [
                'PSU_INFO.voltage_out_of_range',
                'PSU_INFO.current_out_of_range'
            ]
        }
        result = alarm_config._merge_common(defs)
        psu_table = [t for t in result['alarm_tables']
                     if t['table_name'] == 'PSU_INFO'][0]
        check_names = [c['check_name'] for c in psu_table['checks']]
        self.assertNotIn('voltage_out_of_range', check_names)
        self.assertNotIn('current_out_of_range', check_names)
        self.assertIn('psu_presence', check_names)
        self.assertEqual(len(psu_table['checks']), 4)  # 6 - 2

    def test_combined_add_replace_disable(self):
        """All merge operations work together."""
        defs = {
            'sonic_version': 'master',
            'alarmd_settings': {'statedb_poll_interval': 5,
                                'script_poll_interval': 90},
            'disable': [
                'PSU_INFO.voltage_out_of_range',
                'PSU_INFO.current_out_of_range'
            ],
            'alarm_tables': [
                {
                    'type': 'statedb',
                    'table_name': 'PSU_INFO',
                    'object_key_pattern': 'PSU_INFO|*',
                    'checks': [
                        {
                            'check_name': 'psu_presence',  # replace severity
                            'alarm_id': 'PSU_MISSING',
                            'severity': 'Major',
                            'category': 'Hardware',
                            'description_template': '{object_name} Absent',
                            'condition': {'field': 'presence',
                                          'operator': '==', 'value': 'false'}
                        },
                        {
                            'check_name': 'pmbus_fault',  # new check
                            'alarm_id': 'PSU_PMBUS_FAULT',
                            'severity': 'Major',
                            'category': 'Hardware',
                            'condition': {'field': 'input_voltage_status',
                                          'operator': '==', 'value': 'Fault'}
                        }
                    ]
                },
                {
                    'type': 'statedb',
                    'table_name': 'TEMPERATURE_INFO',
                    'object_key_pattern': 'TEMPERATURE_INFO|*',
                    'checks': [{
                        'check_name': 'temp_warn',
                        'alarm_id': 'TEMP_WARNING',
                        'severity': 'Major',
                        'category': 'Hardware',
                        'condition': {'field': 'warning_status',
                                      'operator': '==', 'value': 'True'}
                    }]
                }
            ]
        }
        result = alarm_config._merge_common(defs)
        self.assertIsNotNone(result)

        # Settings carried over
        self.assertEqual(result['sonic_version'], 'master')
        self.assertEqual(result['alarmd_settings']['statedb_poll_interval'], 5)

        # PSU_INFO: 6 common - 2 disabled + 1 added (psu_presence replaced) = 5
        psu = [t for t in result['alarm_tables']
               if t['table_name'] == 'PSU_INFO'][0]
        self.assertEqual(len(psu['checks']), 5)
        check_names = [c['check_name'] for c in psu['checks']]
        self.assertNotIn('voltage_out_of_range', check_names)
        self.assertNotIn('current_out_of_range', check_names)
        self.assertIn('pmbus_fault', check_names)
        presence = [c for c in psu['checks']
                    if c['check_name'] == 'psu_presence'][0]
        self.assertEqual(presence['severity'], 'Major')

        # FAN_INFO: untouched (4 checks)
        fan = [t for t in result['alarm_tables']
               if t['table_name'] == 'FAN_INFO'][0]
        self.assertEqual(len(fan['checks']), 4)

        # TEMPERATURE_INFO: added (1 check)
        temp = [t for t in result['alarm_tables']
                if t['table_name'] == 'TEMPERATURE_INFO'][0]
        self.assertEqual(len(temp['checks']), 1)

        # Total: 5 + 4 + 1 = 10
        total = sum(len(t.get('checks', [])) for t in result['alarm_tables'])
        self.assertEqual(total, 10)

    def test_common_catalog_load_failure(self):
        """If common catalog fails to load, returns None."""
        with patch.object(alarm_config, '_load_common_catalog',
                          return_value=None):
            result = alarm_config._merge_common({})
            self.assertIsNone(result)


# ═══════════════════════════════════════════════════════════════════════════
# 15. Shorthand expansion tests
# ═══════════════════════════════════════════════════════════════════════════
class TestShorthandExpansion(unittest.TestCase):
    """Test _expand_shorthand() and _parse_condition_expr()."""

    def test_parse_eq(self):
        result = alarm_config._parse_condition_expr('presence == false')
        self.assertEqual(result, ('presence', '==', 'false'))

    def test_parse_neq(self):
        result = alarm_config._parse_condition_expr('status != true')
        self.assertEqual(result, ('status', '!=', 'true'))

    def test_parse_lt(self):
        result = alarm_config._parse_condition_expr('voltage < 11.6')
        self.assertEqual(result, ('voltage', '<', '11.6'))

    def test_parse_lte(self):
        result = alarm_config._parse_condition_expr('temp <= 85')
        self.assertEqual(result, ('temp', '<=', '85'))

    def test_parse_gt(self):
        result = alarm_config._parse_condition_expr('current > 230')
        self.assertEqual(result, ('current', '>', '230'))

    def test_parse_gte(self):
        result = alarm_config._parse_condition_expr('speed >= 100')
        self.assertEqual(result, ('speed', '>=', '100'))

    def test_parse_bad_expr(self):
        result = alarm_config._parse_condition_expr('no_operator_here')
        self.assertIsNone(result)

    def test_expand_shorthand_basic(self):
        """checks_short expands to full check dicts."""
        tables = [{
            'type': 'statedb',
            'table_name': 'FAN_INFO',
            'checks_short': [
                ['FAN_MISSING', 'presence == false', 'Major'],
                ['FAN_FAULT', 'status == false', 'Major'],
            ]
        }]
        alarm_config._expand_shorthand(tables)

        self.assertNotIn('checks_short', tables[0])
        checks = tables[0]['checks']
        self.assertEqual(len(checks), 2)
        self.assertEqual(checks[0]['alarm_id'], 'FAN_MISSING')
        self.assertEqual(checks[0]['condition']['field'], 'presence')
        self.assertEqual(checks[0]['condition']['operator'], '==')
        self.assertEqual(checks[0]['condition']['value'], 'false')
        self.assertEqual(checks[0]['severity'], 'Major')

    def test_expand_shorthand_merges_with_existing(self):
        """checks_short entries are appended to existing checks."""
        tables = [{
            'type': 'statedb',
            'table_name': 'PSU_INFO',
            'checks': [
                {'check_name': 'existing', 'alarm_id': 'EXISTING', 'severity': 'Minor',
                 'condition': {'field': 'x', 'operator': '==', 'value': 'y'}},
            ],
            'checks_short': [
                ['NEW_CHECK', 'field1 == val1', 'Major'],
            ]
        }]
        alarm_config._expand_shorthand(tables)
        self.assertEqual(len(tables[0]['checks']), 2)
        self.assertEqual(tables[0]['checks'][0]['alarm_id'], 'EXISTING')
        self.assertEqual(tables[0]['checks'][1]['alarm_id'], 'NEW_CHECK')

    def test_expand_shorthand_malformed_skipped(self):
        """Malformed shorthand entries are skipped."""
        tables = [{
            'type': 'statedb',
            'table_name': 'X',
            'checks_short': [
                ['ONLY_TWO_FIELDS', 'bad'],  # too few elements
                'not_a_list',
            ]
        }]
        alarm_config._expand_shorthand(tables)
        self.assertEqual(len(tables[0].get('checks', [])), 0)


# ═══════════════════════════════════════════════════════════════════════════
# 16. (Reserved — runtime overrides removed in v1 simplification)
# ═══════════════════════════════════════════════════════════════════════════


# ═══════════════════════════════════════════════════════════════════════════
# 17. load_alarm_defs integration (merge in pipeline)
# ═══════════════════════════════════════════════════════════════════════════
class TestLoadAlarmDefsWithMerge(unittest.TestCase):
    """Test that load_alarm_defs() integrates common merge correctly."""

    @patch('sonic_alarm.config._check_sonic_version')
    @patch('sonic_alarm.config.validate', return_value=(True, []))
    @patch('sonic_alarm.config._load_json_file')
    @patch('sonic_alarm.config.resolve_paths')
    def test_merge_resolved_before_validate(self, mock_resolve, mock_load,
                                             mock_val, mock_ver):
        """load_alarm_defs should merge with common, then validate."""
        mock_resolve.return_value = ('/fake/alarm_defs.json', '/fake/dir')
        mock_load.return_value = {
            'sonic_version': 'master',
            'alarmd_settings': {'statedb_poll_interval': 3,
                                'script_poll_interval': 60},
            'alarm_tables': []}

        # Patch _merge_common to return a known result
        with patch('sonic_alarm.config._merge_common') as mock_merge:
            mock_merge.return_value = {
                'alarm_tables': [], 'sonic_version': 'master'}
            result = alarm_config.load_alarm_defs()
            mock_merge.assert_called_once()

    @patch('sonic_alarm.config._check_sonic_version')
    @patch('sonic_alarm.config.validate', return_value=(True, []))
    @patch('sonic_alarm.config._merge_common', return_value=None)
    @patch('sonic_alarm.config._load_json_file')
    @patch('sonic_alarm.config.resolve_paths')
    def test_merge_failure_returns_none(self, mock_resolve, mock_load,
                                        mock_merge, mock_val, mock_ver):
        """If _merge_common returns None, load_alarm_defs returns None."""
        mock_resolve.return_value = ('/fake/alarm_defs.json', '/fake/dir')
        mock_load.return_value = {'alarm_tables': []}
        result = alarm_config.load_alarm_defs()
        self.assertIsNone(result)

    @patch('sonic_alarm.config._check_sonic_version')
    @patch('sonic_alarm.config.validate', return_value=(True, []))
    @patch('sonic_alarm.config.resolve_paths')
    def test_fallback_to_common_catalog(self, mock_resolve, mock_val,
                                         mock_ver):
        """When no platform files exist, fall back to common catalog."""
        mock_resolve.return_value = (None, None)
        result = alarm_config.load_alarm_defs()
        self.assertIsNotNone(result)
        self.assertIn('alarm_tables', result)
        table_names = [t['table_name'] for t in result['alarm_tables']]
        self.assertIn('PSU_INFO', table_names)
        self.assertIn('FAN_INFO', table_names)


# ═══════════════════════════════════════════════════════════════════════════
# 18. config.py — resolve_paths, _load_json_file tests
# ═══════════════════════════════════════════════════════════════════════════
class TestResolvePaths(unittest.TestCase):
    """Test config.resolve_paths() — platform path resolution."""

    def setUp(self):
        self.tmpdir = tempfile.mkdtemp(prefix='alarmd_resolve_')

    def tearDown(self):
        shutil.rmtree(self.tmpdir, ignore_errors=True)

    @patch('sonic_alarm.config.MACHINE_CONF')
    @patch('sonic_alarm.config.DEVICE_BASE')
    def test_resolve_with_base_file(self, mock_base, mock_conf):
        """resolve_paths finds alarm_defs.json when it exists."""
        # Create machine.conf
        conf_path = os.path.join(self.tmpdir, 'machine.conf')
        with open(conf_path, 'w') as f:
            f.write('onie_platform=x86_64-test_platform-r0\n')
        alarm_config.MACHINE_CONF = conf_path

        # Create device dir with alarm_defs.json
        plat_dir = os.path.join(self.tmpdir, 'device', 'x86_64-test_platform-r0')
        os.makedirs(plat_dir)
        defs_file = os.path.join(plat_dir, 'alarm_defs.json')
        with open(defs_file, 'w') as f:
            json.dump({'alarm_tables': []}, f)
        alarm_config.DEVICE_BASE = os.path.join(self.tmpdir, 'device')

        fp, pdir = alarm_config.resolve_paths()
        self.assertEqual(fp, defs_file)
        self.assertEqual(pdir, plat_dir)

    @patch('sonic_alarm.config.MACHINE_CONF')
    @patch('sonic_alarm.config.DEVICE_BASE')
    def test_resolve_no_platform_files(self, mock_base, mock_conf):
        """resolve_paths returns (None, platform_dir) and warns when no file."""
        conf_path = os.path.join(self.tmpdir, 'machine.conf')
        with open(conf_path, 'w') as f:
            f.write('onie_platform=x86_64-test_platform-r0\n')
        alarm_config.MACHINE_CONF = conf_path

        plat_dir = os.path.join(self.tmpdir, 'device', 'x86_64-test_platform-r0')
        os.makedirs(plat_dir)  # exists but no alarm_defs inside
        alarm_config.DEVICE_BASE = os.path.join(self.tmpdir, 'device')

        fp, pdir = alarm_config.resolve_paths()
        self.assertIsNone(fp)
        self.assertEqual(pdir, plat_dir)

    def test_resolve_missing_machine_conf(self):
        """resolve_paths handles missing machine.conf."""
        alarm_config.MACHINE_CONF = '/nonexistent/machine.conf'
        fp, pdir = alarm_config.resolve_paths()
        self.assertIsNone(fp)
        self.assertIsNone(pdir)

    def test_resolve_no_platform_in_machine_conf(self):
        """resolve_paths handles machine.conf without platform line."""
        conf_path = os.path.join(self.tmpdir, 'machine.conf')
        with open(conf_path, 'w') as f:
            f.write('some_other_key=value\n')
        alarm_config.MACHINE_CONF = conf_path
        fp, pdir = alarm_config.resolve_paths()
        self.assertIsNone(fp)
        self.assertIsNone(pdir)

    @patch('sonic_alarm.config.MACHINE_CONF')
    @patch('sonic_alarm.config.DEVICE_BASE')
    def test_resolve_aboot_platform(self, mock_base, mock_conf):
        """resolve_paths supports aboot_platform= prefix."""
        conf_path = os.path.join(self.tmpdir, 'machine.conf')
        with open(conf_path, 'w') as f:
            f.write('aboot_platform=aboot-test-platform\n')
        alarm_config.MACHINE_CONF = conf_path

        plat_dir = os.path.join(self.tmpdir, 'device', 'aboot-test-platform')
        os.makedirs(plat_dir)
        defs_file = os.path.join(plat_dir, 'alarm_defs.json')
        with open(defs_file, 'w') as f:
            json.dump({'alarm_tables': []}, f)
        alarm_config.DEVICE_BASE = os.path.join(self.tmpdir, 'device')

        fp, pdir = alarm_config.resolve_paths()
        self.assertEqual(fp, defs_file)


class TestLoadJsonFile(unittest.TestCase):
    """Test config._load_json_file()."""

    def setUp(self):
        self.tmpdir = tempfile.mkdtemp(prefix='alarmd_json_')

    def tearDown(self):
        shutil.rmtree(self.tmpdir, ignore_errors=True)

    def test_valid_json(self):
        path = os.path.join(self.tmpdir, 'good.json')
        with open(path, 'w') as f:
            json.dump({'alarm_tables': [{'x': 1}]}, f)
        result = alarm_config._load_json_file(path)
        self.assertIsNotNone(result)
        self.assertEqual(result['alarm_tables'][0]['x'], 1)

    def test_invalid_json(self):
        path = os.path.join(self.tmpdir, 'bad.json')
        with open(path, 'w') as f:
            f.write('{not valid json')
        result = alarm_config._load_json_file(path)
        self.assertIsNone(result)

    def test_missing_file(self):
        result = alarm_config._load_json_file('/nonexistent/file.json')
        self.assertIsNone(result)


# ═══════════════════════════════════════════════════════════════════════════
# 19. config.py — validate() comprehensive tests
# ═══════════════════════════════════════════════════════════════════════════
class TestValidate(unittest.TestCase):
    """Test config.validate() — semantic validation of alarm definitions."""

    def test_valid_statedb_config(self):
        """A well-formed statedb config passes validation."""
        defs = {
            'alarm_tables': [{
                'type': 'statedb',
                'table_name': 'PSU_INFO',
                'checks': [{
                    'check_name': 'psu_presence',
                    'alarm_id': 'PSU_MISSING',
                    'severity': 'Critical',
                    'condition': {'field': 'presence', 'operator': '==', 'value': 'false'}
                }]
            }]
        }
        is_valid, errors = alarm_config.validate(defs)
        self.assertTrue(is_valid)
        self.assertEqual(len(errors), 0)

    def test_missing_alarm_tables_key(self):
        is_valid, errors = alarm_config.validate({})
        self.assertFalse(is_valid)
        self.assertIn("Config missing 'alarm_tables' key", errors[0])

    def test_none_defs(self):
        is_valid, errors = alarm_config.validate(None)
        self.assertFalse(is_valid)

    def test_invalid_type(self):
        defs = {
            'alarm_tables': [{'type': 'invalid', 'table_name': 'X', 'checks': []}]
        }
        is_valid, errors = alarm_config.validate(defs)
        self.assertFalse(is_valid)
        self.assertTrue(any('Invalid type' in e for e in errors))

    def test_missing_table_name_statedb(self):
        defs = {
            'alarm_tables': [{'type': 'statedb', 'checks': [
                {'check_name': 'x', 'alarm_id': 'X', 'severity': 'Minor',
                 'condition': {'field': 'f', 'operator': '==', 'value': 'v'}}
            ]}]
        }
        is_valid, errors = alarm_config.validate(defs)
        self.assertFalse(is_valid)
        self.assertTrue(any("Missing 'table_name'" in e for e in errors))

    def test_missing_alarm_id(self):
        defs = {
            'alarm_tables': [{
                'type': 'statedb', 'table_name': 'X',
                'checks': [{'check_name': 'c1', 'severity': 'Minor',
                             'condition': {'field': 'f', 'operator': '==', 'value': 'v'}}]
            }]
        }
        is_valid, errors = alarm_config.validate(defs)
        self.assertFalse(is_valid)
        self.assertTrue(any("Missing required field 'alarm_id'" in e for e in errors))

    def test_missing_condition_block(self):
        defs = {
            'alarm_tables': [{
                'type': 'statedb', 'table_name': 'X',
                'checks': [{'check_name': 'c1', 'alarm_id': 'A1', 'severity': 'Minor'}]
            }]
        }
        is_valid, errors = alarm_config.validate(defs)
        self.assertFalse(is_valid)
        self.assertTrue(any("Missing 'condition'" in e for e in errors))

    def test_missing_condition_field(self):
        defs = {
            'alarm_tables': [{
                'type': 'statedb', 'table_name': 'X',
                'checks': [{'check_name': 'c1', 'alarm_id': 'A1', 'severity': 'Minor',
                             'condition': {'operator': '==', 'value': 'v'}}]
            }]
        }
        is_valid, errors = alarm_config.validate(defs)
        self.assertFalse(is_valid)
        self.assertTrue(any('condition.field' in e for e in errors))

    def test_invalid_operator(self):
        defs = {
            'alarm_tables': [{
                'type': 'statedb', 'table_name': 'X',
                'checks': [{'check_name': 'c1', 'alarm_id': 'A1', 'severity': 'Minor',
                             'condition': {'field': 'f', 'operator': '~=', 'value': 'v'}}]
            }]
        }
        is_valid, errors = alarm_config.validate(defs)
        self.assertFalse(is_valid)
        self.assertTrue(any("Invalid operator" in e for e in errors))

    def test_empty_checks_warns_but_passes(self):
        defs = {
            'alarm_tables': [{
                'type': 'statedb', 'table_name': 'EMPTY', 'checks': []
            }]
        }
        is_valid, errors = alarm_config.validate(defs)
        self.assertTrue(is_valid)  # Empty checks is a warning, not error

    def test_script_missing_command(self):
        defs = {
            'alarm_tables': [{
                'type': 'script', 'group_name': 'scripts',
                'checks': [{'check_name': 'c1', 'alarm_id': 'A1', 'severity': 'Minor'}]
            }]
        }
        is_valid, errors = alarm_config.validate(defs)
        self.assertFalse(is_valid)
        self.assertTrue(any("Missing 'command'" in e for e in errors))

    def test_script_nonexistent_path(self):
        defs = {
            'alarm_tables': [{
                'type': 'script', 'group_name': 'scripts',
                'checks': [{
                    'check_name': 'c1', 'alarm_id': 'A1', 'severity': 'Minor',
                    'command': '/nonexistent/check.sh', 'timeout': 10,
                    'condition': {'exit_code': '!=', 'value': 0}
                }]
            }]
        }
        is_valid, errors = alarm_config.validate(defs)
        self.assertFalse(is_valid)
        self.assertTrue(any('Script not found' in e for e in errors))

    def test_script_invalid_exit_code_operator(self):
        """Invalid exit_code operator in script condition → error."""
        tmpdir = tempfile.mkdtemp()
        try:
            script = os.path.join(tmpdir, 'check.sh')
            with open(script, 'w') as f:
                f.write('#!/bin/bash\nexit 0\n')
            os.chmod(script, 0o755)

            defs = {
                'alarm_tables': [{
                    'type': 'script', 'group_name': 'scripts',
                    'checks': [{
                        'check_name': 'c1', 'alarm_id': 'A1', 'severity': 'Minor',
                        'command': script, 'timeout': 10,
                        'condition': {'exit_code': '~', 'value': 0}
                    }]
                }]
            }
            is_valid, errors = alarm_config.validate(defs)
            self.assertFalse(is_valid)
            self.assertTrue(any('exit_code' in e for e in errors))
        finally:
            shutil.rmtree(tmpdir, ignore_errors=True)

    def test_script_invalid_regex(self):
        """Invalid regex in stdout_regex condition → error."""
        tmpdir = tempfile.mkdtemp()
        try:
            script = os.path.join(tmpdir, 'check.sh')
            with open(script, 'w') as f:
                f.write('#!/bin/bash\nexit 0\n')
            os.chmod(script, 0o755)

            defs = {
                'alarm_tables': [{
                    'type': 'script', 'group_name': 'scripts',
                    'checks': [{
                        'check_name': 'c1', 'alarm_id': 'A1', 'severity': 'Minor',
                        'command': script, 'timeout': 10,
                        'condition': {'stdout_regex': '[invalid(regex'}
                    }]
                }]
            }
            is_valid, errors = alarm_config.validate(defs)
            self.assertFalse(is_valid)
            self.assertTrue(any('regex' in e.lower() for e in errors))
        finally:
            shutil.rmtree(tmpdir, ignore_errors=True)

    def test_script_timeout_warnings(self):
        """Script with invalid or excessive timeout → warnings but still passes."""
        tmpdir = tempfile.mkdtemp()
        try:
            script = os.path.join(tmpdir, 'check.sh')
            with open(script, 'w') as f:
                f.write('#!/bin/bash\nexit 0\n')
            os.chmod(script, 0o755)

            defs = {
                'alarm_tables': [{
                    'type': 'script', 'group_name': 'scripts',
                    'checks': [{
                        'check_name': 'c1', 'alarm_id': 'A1', 'severity': 'Minor',
                        'command': script, 'timeout': 999,
                        'condition': {'exit_code': '!=', 'value': 0}
                    }]
                }]
            }
            is_valid, errors = alarm_config.validate(defs)
            self.assertTrue(is_valid)  # excessive timeout is a warning
        finally:
            shutil.rmtree(tmpdir, ignore_errors=True)

    def test_duplicate_check_name_warns(self):
        """Duplicate check_name is a warning, not an error."""
        defs = {
            'alarm_tables': [{
                'type': 'statedb', 'table_name': 'X',
                'checks': [
                    {'check_name': 'dup', 'alarm_id': 'A1', 'severity': 'Minor',
                     'condition': {'field': 'f', 'operator': '==', 'value': 'v'}},
                    {'check_name': 'dup', 'alarm_id': 'A2', 'severity': 'Minor',
                     'condition': {'field': 'f2', 'operator': '==', 'value': 'v2'}},
                ]
            }]
        }
        is_valid, errors = alarm_config.validate(defs)
        self.assertTrue(is_valid)  # Duplicate is a warning

    def test_cross_table_conflict_warns(self):
        """Same alarm_id from different tables → warning."""
        defs = {
            'alarm_tables': [
                {'type': 'statedb', 'table_name': 'TABLE_A',
                 'checks': [{'check_name': 'c1', 'alarm_id': 'SHARED_ALARM',
                              'severity': 'Minor',
                              'condition': {'field': 'f', 'operator': '==', 'value': 'v'}}]},
                {'type': 'statedb', 'table_name': 'TABLE_B',
                 'checks': [{'check_name': 'c2', 'alarm_id': 'SHARED_ALARM',
                              'severity': 'Minor',
                              'condition': {'field': 'f2', 'operator': '==', 'value': 'v2'}}]},
            ]
        }
        is_valid, errors = alarm_config.validate(defs)
        self.assertTrue(is_valid)  # Cross-table is a warning

    def test_empty_condition_value_warns(self):
        """condition.value='' → warning but passes."""
        defs = {
            'alarm_tables': [{
                'type': 'statedb', 'table_name': 'X',
                'checks': [{'check_name': 'c1', 'alarm_id': 'A1', 'severity': 'Minor',
                             'condition': {'field': 'f', 'operator': '==', 'value': ''}}]
            }]
        }
        is_valid, _ = alarm_config.validate(defs)
        self.assertTrue(is_valid)


# ═══════════════════════════════════════════════════════════════════════════
# 20. config.py — load_alarm_defs() pipeline integration tests
# ═══════════════════════════════════════════════════════════════════════════
class TestLoadAlarmDefsPipeline(unittest.TestCase):
    """Test load_alarm_defs() end-to-end through various pipeline branches."""

    def setUp(self):
        self.tmpdir = tempfile.mkdtemp(prefix='alarmd_pipeline_')

    def tearDown(self):
        shutil.rmtree(self.tmpdir, ignore_errors=True)

    @patch('sonic_alarm.config._check_sonic_version')
    @patch('sonic_alarm.config._load_json_file')
    @patch('sonic_alarm.config.resolve_paths')
    def test_base_file_load_failure_returns_none(self, mock_resolve, mock_load,
                                                   mock_ver):
        """If base file fails to parse, return None."""
        mock_resolve.return_value = ('/fake/alarm_defs.json', '/fake/dir')
        mock_load.return_value = None  # parse failure
        result = alarm_config.load_alarm_defs()
        self.assertIsNone(result)

    @patch('sonic_alarm.config._check_sonic_version')
    @patch('sonic_alarm.config.validate', return_value=(False, ['bad config']))
    @patch('sonic_alarm.config._load_json_file')
    @patch('sonic_alarm.config.resolve_paths')
    def test_validation_failure_returns_none(self, mock_resolve, mock_load,
                                              mock_val, mock_ver):
        """Validation failure returns None."""
        mock_resolve.return_value = ('/fake/alarm_defs.json', '/fake/dir')
        mock_load.return_value = {'alarm_tables': []}
        # _merge_common will be called — mock it to return valid structure
        with patch('sonic_alarm.config._merge_common') as mock_merge:
            mock_merge.return_value = {'alarm_tables': []}
            result = alarm_config.load_alarm_defs()
            self.assertIsNone(result)

    @patch('sonic_alarm.config._check_sonic_version')
    @patch('sonic_alarm.config.validate', return_value=(True, []))
    @patch('sonic_alarm.config.resolve_paths')
    def test_common_fallback_load_failure_returns_none(self, mock_resolve,
                                                        mock_val, mock_ver):
        """If no platform file and common catalog fails, return None."""
        mock_resolve.return_value = (None, None)
        with patch('sonic_alarm.config._load_common_catalog',
                   return_value=None):
            result = alarm_config.load_alarm_defs()
            self.assertIsNone(result)


# ═══════════════════════════════════════════════════════════════════════════
# 21. config.py — _get_running_sonic_version tests
# ═══════════════════════════════════════════════════════════════════════════
class TestGetRunningSonicVersion(unittest.TestCase):
    """Test _get_running_sonic_version() file parsing."""

    def setUp(self):
        self.tmpdir = tempfile.mkdtemp(prefix='alarmd_ver_')
        self.orig_path = alarm_config.SONIC_VERSION_FILE

    def tearDown(self):
        alarm_config.SONIC_VERSION_FILE = self.orig_path
        shutil.rmtree(self.tmpdir, ignore_errors=True)

    def _write_version_file(self, content):
        path = os.path.join(self.tmpdir, 'sonic_version.yml')
        with open(path, 'w') as f:
            f.write(content)
        # Temporarily patch the constant
        from sonic_alarm import config as cfg
        cfg.SONIC_VERSION_FILE = path
        return path

    def test_reads_branch_unquoted(self):
        self._write_version_file('build_version: master.123\nbranch: master\n')
        result = alarm_config._get_running_sonic_version()
        self.assertEqual(result, 'master')

    def test_reads_branch_quoted(self):
        self._write_version_file("branch: '202505'\n")
        result = alarm_config._get_running_sonic_version()
        self.assertEqual(result, '202505')

    def test_missing_file_returns_none(self):
        from sonic_alarm import config as cfg
        cfg.SONIC_VERSION_FILE = '/nonexistent/sonic_version.yml'
        result = alarm_config._get_running_sonic_version()
        self.assertIsNone(result)

    def test_no_branch_field_returns_none(self):
        self._write_version_file('build_version: master.123\n')
        result = alarm_config._get_running_sonic_version()
        self.assertIsNone(result)


# ═══════════════════════════════════════════════════════════════════════════
# 22. (Removed) utils.py tests — utils.py no longer exists in refactored code.
#     Logging is handled by module-level SysLogger and DaemonBase.
# ═══════════════════════════════════════════════════════════════════════════


# ═══════════════════════════════════════════════════════════════════════════
# 23. alarm_store.py — additional edge case tests
# ═══════════════════════════════════════════════════════════════════════════
# TestAlarmStoreEdgeCases was removed with alarm_store.py (HLD v0.2).  The
# surviving behaviour (sync_active_set / reconcile / purge_stale_alarms /
# active_tags) now lives on EventPublisher and is covered by
# tests/test_event_publisher.py.  clear_on_startup was dropped entirely:
# alarmd no longer wipes an alarm table at startup (eventd owns ALARM).


# ═══════════════════════════════════════════════════════════════════════════
# 24. statedb_poller.py — additional branch coverage
# ═══════════════════════════════════════════════════════════════════════════
class TestStateDBPollerEdgeCases(unittest.TestCase):
    """Additional coverage for statedb_poller.py edge paths."""

    def _make_poller(self, tables):
        mock_db = MagicMock()
        store = MagicMock()
        poller = StateDBPoller(tables, store, mock_db)
        return poller, mock_db, store

    @patch('sonic_alarm.statedb_poller.Table')
    def test_empty_table_name_skipped(self, MockTable):
        """Table with empty table_name is skipped."""
        tables = [{'table_name': '', 'checks': [{'alarm_id': 'X',
                   'condition': {'field': 'f', 'operator': '==', 'value': 'v'}}]}]
        poller, _, store = self._make_poller(tables)
        poller.poll()
        store.set_alarm.assert_not_called()
        MockTable.assert_not_called()

    @patch('sonic_alarm.statedb_poller.Table')
    def test_empty_checks_skipped(self, MockTable):
        """Table with no checks is skipped."""
        tables = [{'table_name': 'X', 'checks': []}]
        poller, _, store = self._make_poller(tables)
        poller.poll()
        store.set_alarm.assert_not_called()
        MockTable.assert_not_called()

    @patch('sonic_alarm.statedb_poller.Table')
    def test_get_key_failure_continues(self, MockTable):
        """tbl.get(key) failure is logged and skipped."""
        tables = [{'table_name': 'X', 'checks': [
            {'alarm_id': 'A1', 'condition': {'field': 'f', 'operator': '==', 'value': 'v'}}
        ]}]
        poller, _, store = self._make_poller(tables)
        mock_tbl = MagicMock()
        mock_tbl.getKeys.return_value = ['k1']
        mock_tbl.get.side_effect = Exception('Redis error')
        MockTable.return_value = mock_tbl

        poller.poll()
        _statedb_poller_mod.logger.log_error.assert_called()
        store.set_alarm.assert_not_called()

    @patch('sonic_alarm.statedb_poller.Table')
    def test_get_returns_false_status(self, MockTable):
        """tbl.get returns False status → key skipped."""
        tables = [{'table_name': 'X', 'checks': [
            {'alarm_id': 'A1', 'condition': {'field': 'f', 'operator': '==', 'value': 'v'}}
        ]}]
        poller, _, store = self._make_poller(tables)
        mock_tbl = MagicMock()
        mock_tbl.getKeys.return_value = ['k1']
        mock_tbl.get.return_value = (False, [])
        MockTable.return_value = mock_tbl

        poller.poll()
        store.set_alarm.assert_not_called()

    @patch('sonic_alarm.statedb_poller.Table')
    def test_check_missing_alarm_id_skipped(self, MockTable):
        """Check with no alarm_id is skipped with warning."""
        tables = [{'table_name': 'X', 'checks': [
            {'check_name': 'bad', 'condition': {'field': 'f', 'operator': '==', 'value': 'v'}}
        ]}]
        poller, _, store = self._make_poller(tables)
        mock_tbl = MagicMock()
        mock_tbl.getKeys.return_value = ['k1']
        mock_tbl.get.return_value = (True, [('f', 'v')])
        MockTable.return_value = mock_tbl

        poller.poll()
        _statedb_poller_mod.logger.log_warning.assert_called()
        store.set_alarm.assert_not_called()

    @patch('sonic_alarm.statedb_poller.Table')
    def test_check_missing_field_name_skipped(self, MockTable):
        """Check with empty condition.field is skipped."""
        tables = [{'table_name': 'X', 'checks': [
            {'alarm_id': 'A1', 'check_name': 'nofield',
             'condition': {'field': '', 'operator': '==', 'value': 'v'}}
        ]}]
        poller, _, store = self._make_poller(tables)
        mock_tbl = MagicMock()
        mock_tbl.getKeys.return_value = ['k1']
        mock_tbl.get.return_value = (True, [('x', 'y')])
        MockTable.return_value = mock_tbl

        poller.poll()
        _statedb_poller_mod.logger.log_warning.assert_called()
        store.set_alarm.assert_not_called()

    @patch('sonic_alarm.statedb_poller.Table')
    def test_set_alarm_exception_logged(self, MockTable):
        """store.set_alarm exception is caught and logged."""
        tables = [{'table_name': 'X', 'checks': [
            {'alarm_id': 'A1', 'condition': {'field': 'f', 'operator': '==', 'value': 'v'}}
        ]}]
        poller, _, store = self._make_poller(tables)
        mock_tbl = MagicMock()
        mock_tbl.getKeys.return_value = ['k1']
        mock_tbl.get.return_value = (True, [('f', 'v')])
        MockTable.return_value = mock_tbl
        store.set_alarm.side_effect = Exception('Store error')

        poller.poll()
        _statedb_poller_mod.logger.log_error.assert_called()

    @patch('sonic_alarm.statedb_poller.Table')
    def test_filtered_keys_empty_skips(self, MockTable):
        """All keys excluded → no alarms evaluated."""
        tables = [{'table_name': 'X', 'exclude_keys': ['k1'],
                   'checks': [{'alarm_id': 'A1',
                                'condition': {'field': 'f', 'operator': '==', 'value': 'v'}}]}]
        poller, _, store = self._make_poller(tables)
        mock_tbl = MagicMock()
        mock_tbl.getKeys.return_value = ['k1']
        MockTable.return_value = mock_tbl

        poller.poll()
        store.set_alarm.assert_not_called()


# ═══════════════════════════════════════════════════════════════════════════
# 25. script_runner.py — additional branch coverage
# ═══════════════════════════════════════════════════════════════════════════
class TestScriptRunnerEdgeCases(unittest.TestCase):
    """Additional coverage for script_runner.py edge paths."""

    def setUp(self):
        self._isfile_patcher = patch('os.path.isfile', return_value=True)
        self._isfile_patcher.start()

    def tearDown(self):
        self._isfile_patcher.stop()

    def _make_runner(self, groups):
        store = MagicMock()
        runner = ScriptRunner(groups, store)
        return runner, store

    def test_empty_groups_noop(self):
        """No script groups → run_all does nothing."""
        runner, store = self._make_runner([])
        runner.run_all()
        store.raise_alarm.assert_not_called()
        store.clear_alarm.assert_not_called()

    def test_empty_checks_noop(self):
        """Script group with no checks → nothing runs."""
        runner, store = self._make_runner([{'checks': []}])
        runner.run_all()
        store.raise_alarm.assert_not_called()

    @patch('subprocess.run')
    def test_missing_alarm_id_skipped(self, mock_run):
        """Check with no alarm_id is skipped."""
        groups = [{'checks': [{'command': '/bin/true', 'timeout': 10,
                                'condition': {'exit_code': '!=', 'value': 0}}]}]
        runner, store = self._make_runner(groups)
        runner.run_all()
        mock_run.assert_not_called()

    @patch('subprocess.run')
    def test_invalid_timeout_clamped(self, mock_run):
        """Negative timeout defaults to SCRIPT_TIMEOUT."""
        groups = [{'checks': [{
            'alarm_id': 'A1', 'object_name': 'OBJ', 'command': '/bin/true',
            'timeout': -5, 'condition': {'exit_code': '!=', 'value': 0}
        }]}]
        runner, store = self._make_runner(groups)
        mock_run.return_value = MagicMock(returncode=0, stdout='')
        runner.run_all()
        # Should have warned about invalid timeout
        warned_msgs = [str(c) for c in _script_runner_mod.logger.log_warning.call_args_list]
        self.assertTrue(any('Invalid timeout' in m for m in warned_msgs))

    @patch('subprocess.run')
    def test_non_numeric_timeout_clamped(self, mock_run):
        """Non-numeric timeout defaults to SCRIPT_TIMEOUT."""
        groups = [{'checks': [{
            'alarm_id': 'A1', 'object_name': 'OBJ', 'command': '/bin/true',
            'timeout': 'abc', 'condition': {'exit_code': '!=', 'value': 0}
        }]}]
        runner, store = self._make_runner(groups)
        mock_run.return_value = MagicMock(returncode=0, stdout='')
        runner.run_all()
        warned_msgs = [str(c) for c in _script_runner_mod.logger.log_warning.call_args_list]
        self.assertTrue(any('Non-numeric timeout' in m for m in warned_msgs))

    @patch('subprocess.run')
    def test_script_not_found_skipped(self, mock_run):
        """Script that doesn't exist on disk is skipped."""
        self._isfile_patcher.stop()
        try:
            groups = [{'checks': [{
                'alarm_id': 'A1', 'object_name': 'OBJ',
                'command': '/nonexistent/script.sh',
                'timeout': 10, 'condition': {'exit_code': '!=', 'value': 0}
            }]}]
            runner, store = self._make_runner(groups)
            runner.run_all()
            mock_run.assert_not_called()
            # Second call: should not warn again (cached)
            runner.run_all()
        finally:
            self._isfile_patcher.start()

    @patch('subprocess.run')
    def test_subprocess_generic_exception(self, mock_run):
        """Generic subprocess exception is caught."""
        groups = [{'checks': [{
            'alarm_id': 'A1', 'object_name': 'OBJ', 'command': '/bin/true',
            'timeout': 10, 'condition': {'exit_code': '!=', 'value': 0}
        }]}]
        runner, store = self._make_runner(groups)
        mock_run.side_effect = OSError('Permission denied')
        runner.run_all()
        _script_runner_mod.logger.log_error.assert_called()
        store.raise_alarm.assert_not_called()

    @patch('subprocess.run')
    def test_fault_description_no_stdout(self, mock_run):
        """Fault alarm with no stdout uses only the template."""
        groups = [{'checks': [{
            'alarm_id': 'DISK_HIGH', 'object_name': 'root',
            'command': '/bin/true', 'timeout': 10,
            'description_template': '{object_name} disk high',
            'condition': {'exit_code': '!=', 'value': 0}
        }]}]
        runner, store = self._make_runner(groups)
        mock_run.return_value = MagicMock(returncode=1, stdout='')
        runner.run_all()
        store.raise_alarm.assert_called_once()
        desc = store.raise_alarm.call_args[1]['description']
        self.assertEqual(desc, 'root disk high')

    def test_default_condition_no_condition_key(self):
        """No condition key → defaults to exit_code != 0."""
        self.assertTrue(ScriptRunner._evaluate_script_condition(None, 1, ''))
        self.assertFalse(ScriptRunner._evaluate_script_condition(None, 0, ''))

    @patch('subprocess.run')
    def test_timeout_raise_alarm_fails(self, mock_run):
        """Timeout alarm raise failure is caught."""
        import subprocess as sp
        groups = [{'checks': [{
            'alarm_id': 'A1', 'object_name': 'OBJ',
            'command': '/bin/sleep 100', 'timeout': 1,
            'condition': {'exit_code': '!=', 'value': 0}
        }]}]
        runner, store = self._make_runner(groups)
        mock_run.side_effect = sp.TimeoutExpired(cmd='test', timeout=1)
        store.raise_alarm.side_effect = Exception('Store down')
        runner.run_all()
        _script_runner_mod.logger.log_error.assert_called()


##############################################################################
# 26. COVERAGE PUSH – script_runner deep error paths
##############################################################################

class TestScriptRunnerDeepPaths(unittest.TestCase):
    """Cover remaining script_runner lines: timeout clamping, parse errors,
    condition eval errors, set_alarm errors, future exception."""

    def _make_runner(self, groups):
        store = MagicMock()
        runner = ScriptRunner(groups, store, max_workers=2)
        return runner, store

    @patch('subprocess.run')
    @patch('os.path.isfile', return_value=True)
    def test_timeout_exceeds_max_clamped(self, mock_isfile, mock_run):
        """timeout > MAX_SCRIPT_TIMEOUT is clamped."""
        from sonic_alarm.constants import MAX_SCRIPT_TIMEOUT
        groups = [{'checks': [{
            'alarm_id': 'A1', 'object_name': 'OBJ',
            'command': '/bin/test_script', 'timeout': 99999,
            'condition': {'exit_code': '!=', 'value': 0}
        }]}]
        runner, store = self._make_runner(groups)
        mock_run.return_value = MagicMock(returncode=0, stdout='')
        runner.run_all()
        # Should have called subprocess.run with clamped timeout
        self.assertTrue(mock_run.called)
        call_kwargs = mock_run.call_args
        self.assertLessEqual(
            call_kwargs[1].get('timeout', 99999), MAX_SCRIPT_TIMEOUT)

    @patch('os.path.isfile', return_value=True)
    def test_command_parse_exception(self, mock_isfile):
        """command.split() raises → caught (lines 111-115)."""
        groups = [{'checks': [{
            'alarm_id': 'A1', 'object_name': 'OBJ',
            'command': MagicMock(split=MagicMock(side_effect=AttributeError('boom'))),
            'timeout': 5,
            'condition': {'exit_code': '!=', 'value': 0}
        }]}]
        runner, store = self._make_runner(groups)
        runner.run_all()
        _script_runner_mod.logger.log_error.assert_called()

    @patch('subprocess.run')
    @patch('os.path.isfile', return_value=True)
    def test_condition_eval_exception(self, mock_isfile, mock_run):
        """_evaluate_script_condition raises → caught (lines 151-155)."""
        groups = [{'checks': [{
            'alarm_id': 'A1', 'object_name': 'OBJ',
            'command': '/bin/test_cmd', 'timeout': 5,
            'condition': {'exit_code': '!=', 'value': 0}
        }]}]
        runner, store = self._make_runner(groups)
        mock_run.return_value = MagicMock(returncode=1, stdout='out')
        # Monkey-patch the static method to raise
        with patch.object(ScriptRunner, '_evaluate_script_condition',
                          side_effect=RuntimeError('eval fail')):
            runner.run_all()
        _script_runner_mod.logger.log_error.assert_called()

    @patch('subprocess.run')
    @patch('os.path.isfile', return_value=True)
    def test_set_alarm_exception_on_fault(self, mock_isfile, mock_run):
        """store.raise_alarm raises during fault → caught (lines 168-169)."""
        groups = [{'checks': [{
            'alarm_id': 'A1', 'object_name': 'OBJ',
            'command': '/bin/test_cmd', 'timeout': 5,
            'condition': {'exit_code': '!=', 'value': 0}
        }]}]
        runner, store = self._make_runner(groups)
        mock_run.return_value = MagicMock(returncode=1, stdout='out')
        store.raise_alarm.side_effect = Exception('db down')
        runner.run_all()
        _script_runner_mod.logger.log_error.assert_called()

    @patch('subprocess.run')
    @patch('os.path.isfile', return_value=True)
    def test_set_alarm_exception_on_clear(self, mock_isfile, mock_run):
        """store.clear_alarm raises during clear → caught (line 169)."""
        groups = [{'checks': [{
            'alarm_id': 'A1', 'object_name': 'OBJ',
            'command': '/bin/test_cmd', 'timeout': 5,
            'condition': {'exit_code': '!=', 'value': 0}
        }]}]
        runner, store = self._make_runner(groups)
        mock_run.return_value = MagicMock(returncode=0, stdout='')
        store.clear_alarm.side_effect = Exception('db down')
        runner.run_all()
        _script_runner_mod.logger.log_error.assert_called()

    @patch('subprocess.run')
    @patch('os.path.isfile', return_value=True)
    def test_fault_description_with_stdout(self, mock_isfile, mock_run):
        """Fault description includes stdout snippet (lines 161-163)."""
        groups = [{'checks': [{
            'alarm_id': 'A1', 'object_name': 'OBJ',
            'command': '/bin/test_cmd', 'timeout': 5,
            'description_template': '{object_name} script failed',
            'condition': {'exit_code': '!=', 'value': 0}
        }]}]
        runner, store = self._make_runner(groups)
        mock_run.return_value = MagicMock(returncode=1, stdout='Something bad happened')
        runner.run_all()
        store.raise_alarm.assert_called_once()
        desc = store.raise_alarm.call_args[1]['description']
        self.assertIn('OBJ script failed', desc)
        self.assertIn('Something bad happened', desc)

    @patch('subprocess.run')
    @patch('os.path.isfile', return_value=True)
    def test_future_exception_in_run_all(self, mock_isfile, mock_run):
        """Future raises non-caught exception → logged (lines 68-69)."""
        groups = [{'checks': [{
            'alarm_id': 'A1', 'object_name': 'OBJ',
            'command': '/bin/test_cmd', 'timeout': 5,
            'condition': {'exit_code': '!=', 'value': 0}
        }]}]
        runner, store = self._make_runner(groups)
        # Make _run_check raise to trigger the future exception handler
        with patch.object(runner, '_run_check', side_effect=RuntimeError('boom')):
            runner.run_all()
        _script_runner_mod.logger.log_error.assert_called()

    @patch('subprocess.run')
    @patch('os.path.isfile', return_value=True)
    def test_script_execution_generic_exception(self, mock_isfile, mock_run):
        """subprocess.run raises non-timeout exception (lines 145-148)."""
        groups = [{'checks': [{
            'alarm_id': 'A1', 'object_name': 'OBJ',
            'command': '/bin/test_cmd', 'timeout': 5,
            'condition': {'exit_code': '!=', 'value': 0}
        }]}]
        runner, store = self._make_runner(groups)
        mock_run.side_effect = OSError('Permission denied')
        runner.run_all()
        _script_runner_mod.logger.log_error.assert_called()


##############################################################################
# 27. COVERAGE PUSH – config.py deep error paths
##############################################################################

class TestConfigDeepPaths(unittest.TestCase):
    """Cover remaining config.py error paths."""

    @patch('sonic_alarm.config._load_common_catalog', return_value=None)
    def test_merge_common_catalog_fails(self, mock_common):
        """_merge_common when common catalog load returns None."""
        platform_defs = {'alarm_tables': []}
        result = alarm_config._merge_common(platform_defs)
        self.assertIsNone(result)
        mock_common.assert_called_once()

    def test_shorthand_unparseable_condition_skipped(self):
        """Shorthand entry with no valid operator → parsed=None → skip (line 341)."""
        tables = [{
            'table_name': 'T1',
            'checks': [],
            'checks_short': [
                ['alarm_id_ok', 'field == value', 'Major'],
                ['alarm_bad', 'noop', 'Minor'],  # no operator → returns None
            ]
        }]
        alarm_config._expand_shorthand(tables)
        # Only the first shorthand should expand; the bad one is skipped
        self.assertEqual(len(tables[0].get('checks', [])), 1)
        self.assertEqual(tables[0]['checks'][0]['alarm_id'], 'alarm_id_ok')

    @patch('sonic_alarm.config._load_common_catalog', return_value=None)
    @patch('sonic_alarm.config._load_json_file', return_value=None)
    @patch('sonic_alarm.config.resolve_paths')
    def test_load_alarm_defs_fallback_common_fails(self, mock_paths, mock_json,
                                                    mock_common):
        """No platform files + common catalog fallback fails."""
        mock_paths.return_value = (None, None)
        result = alarm_config.load_alarm_defs()
        self.assertIsNone(result)


##############################################################################
# 28. COVERAGE PUSH – config.py validate() script type paths
##############################################################################

class TestValidateScriptPaths(unittest.TestCase):
    """Cover script-type validation in validate() (lines 611+)."""

    def test_validate_script_missing_command(self):
        """Script check missing 'command' → error (line 686)."""
        defs = {'alarm_tables': [{
            'type': 'script',
            'group_name': 'scripts',
            'checks': [{'alarm_id': 'A1', 'object_name': 'OBJ'}]
        }]}
        ok, errors = alarm_config.validate(defs)
        self.assertFalse(ok)
        self.assertTrue(any('command' in e for e in errors))

    @patch('os.access', return_value=False)
    @patch('os.path.isfile', return_value=True)
    def test_validate_script_not_executable(self, mock_isfile, mock_access):
        """Script exists but not executable → error (line 691)."""
        defs = {'alarm_tables': [{
            'type': 'script',
            'group_name': 'scripts',
            'checks': [{
                'alarm_id': 'A1', 'object_name': 'OBJ',
                'command': '/usr/bin/some_script',
                'timeout': 5,
                'condition': {'exit_code': '!=', 'value': 0}
            }]
        }]}
        ok, errors = alarm_config.validate(defs)
        self.assertFalse(ok)
        self.assertTrue(any('executable' in e for e in errors))

    @patch('os.access', return_value=True)
    @patch('os.path.isfile', return_value=True)
    def test_validate_script_valid(self, mock_isfile, mock_access):
        """Valid script check passes validation."""
        defs = {'alarm_tables': [{
            'type': 'script',
            'group_name': 'scripts',
            'checks': [{
                'alarm_id': 'A1', 'object_name': 'OBJ',
                'command': '/usr/bin/some_script',
                'timeout': 5,
                'condition': {'exit_code': '!=', 'value': 0}
            }]
        }]}
        ok, errors = alarm_config.validate(defs)
        self.assertTrue(ok)
        self.assertEqual(errors, [])

    @patch('os.access', return_value=True)
    @patch('os.path.isfile', return_value=True)
    def test_validate_script_invalid_exit_code_operator(self, mock_isfile, mock_access):
        """Script check with invalid exit_code operator → error."""
        defs = {'alarm_tables': [{
            'type': 'script',
            'group_name': 'scripts',
            'checks': [{
                'alarm_id': 'A1', 'object_name': 'OBJ',
                'command': '/usr/bin/some_script',
                'timeout': 5,
                'condition': {'exit_code': 'BADOP'}
            }]
        }]}
        ok, errors = alarm_config.validate(defs)
        self.assertFalse(ok)
        self.assertTrue(any('exit_code' in e for e in errors))

    @patch('os.access', return_value=True)
    @patch('os.path.isfile', return_value=True)
    def test_validate_script_invalid_regex(self, mock_isfile, mock_access):
        """Script check with invalid stdout_regex → error."""
        defs = {'alarm_tables': [{
            'type': 'script',
            'group_name': 'scripts',
            'checks': [{
                'alarm_id': 'A1', 'object_name': 'OBJ',
                'command': '/usr/bin/some_script',
                'timeout': 5,
                'condition': {'stdout_regex': '[invalid'}
            }]
        }]}
        ok, errors = alarm_config.validate(defs)
        self.assertFalse(ok)
        self.assertTrue(any('regex' in e.lower() for e in errors))

    @patch('os.access', return_value=True)
    @patch('os.path.isfile', return_value=True)
    def test_validate_script_timeout_exceeds_max(self, mock_isfile, mock_access):
        """Script check with timeout > max → warning but still valid."""
        defs = {'alarm_tables': [{
            'type': 'script',
            'group_name': 'scripts',
            'checks': [{
                'alarm_id': 'A1', 'object_name': 'OBJ',
                'command': '/usr/bin/some_script',
                'timeout': 9999,
                'condition': {'exit_code': '!=', 'value': 0}
            }]
        }]}
        ok, errors = alarm_config.validate(defs)
        self.assertTrue(ok)  # warnings don't fail validation

    @patch('os.access', return_value=True)
    @patch('os.path.isfile', return_value=True)
    def test_validate_script_invalid_timeout(self, mock_isfile, mock_access):
        """Script check with non-numeric timeout → warning."""
        defs = {'alarm_tables': [{
            'type': 'script',
            'group_name': 'scripts',
            'checks': [{
                'alarm_id': 'A1', 'object_name': 'OBJ',
                'command': '/usr/bin/some_script',
                'timeout': 'bad',
                'condition': {'exit_code': '!=', 'value': 0}
            }]
        }]}
        ok, errors = alarm_config.validate(defs)
        self.assertTrue(ok)  # warnings don't fail validation

    @patch('os.path.isfile', return_value=False)
    def test_validate_script_not_found(self, mock_isfile):
        """Script file doesn't exist → error (line 688)."""
        defs = {'alarm_tables': [{
            'type': 'script',
            'group_name': 'scripts',
            'checks': [{
                'alarm_id': 'A1', 'object_name': 'OBJ',
                'command': '/nonexistent/script',
                'timeout': 5,
                'condition': {}
            }]
        }]}
        ok, errors = alarm_config.validate(defs)
        self.assertFalse(ok)
        self.assertTrue(any('not found' in e.lower() for e in errors))

    def test_validate_script_missing_group_name(self):
        """Script type without group_name → warning."""
        defs = {'alarm_tables': [{
            'type': 'script',
            'checks': [{
                'alarm_id': 'A1', 'object_name': 'OBJ',
                'command': '/nonexistent/script',
                'timeout': 5,
                'condition': {}
            }]
        }]}
        # Should not crash; group_name missing generates a warning
        ok, errors = alarm_config.validate(defs)
        # The script won't exist, so there will be errors, but no crash
        self.assertFalse(ok)


##############################################################################
# 29. COVERAGE PUSH – statedb_poller set_alarm error in poll loop
##############################################################################

class TestStateDBPollerSetAlarmError(unittest.TestCase):
    """Cover statedb_poller lines 182-186: set_alarm raises in poll()."""

    @patch('sonic_alarm.statedb_poller.Table')
    def test_set_alarm_exception_in_poll(self, MockTable):
        """set_alarm raises during poll → error is logged, no crash."""
        store = MagicMock()
        store.set_alarm.side_effect = Exception('DB write fail')
        state_db = MagicMock()
        table_defs = [{
            'table_name': 'TEST_TABLE',
            'checks': [{
                'check_name': 'test_check',
                'alarm_id': 'ALARM_1',
                'severity': 'Major',
                'condition': {
                    'field': 'status',
                    'operator': '!=',
                    'value': 'ok'
                }
            }]
        }]

        mock_tbl = MagicMock()
        mock_tbl.getKeys.return_value = ['key1']
        mock_tbl.get.return_value = (True, [('status', 'bad')])
        MockTable.return_value = mock_tbl

        poller = StateDBPoller(table_defs, store, state_db)

        poller.poll()
        store.set_alarm.assert_called()
        _statedb_poller_mod.logger.log_error.assert_called()


##############################################################################
# 30. (Removed) utils.py SysLogHandler tests — utils.py no longer exists.
##############################################################################


if __name__ == '__main__':
    unittest.main()
