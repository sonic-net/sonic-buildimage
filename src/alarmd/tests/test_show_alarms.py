"""
test_show_alarms.py — Unit tests for the 'show alarms' CLI command.

The CLI reads eventd's ALARM table from EVENT_DB (HLD 9.1).  These tests mock
SonicV2Connector so they run without Redis, exercising all output modes:
table, JSON, summary, grouped, filtered, and the "framework not enabled" path.

Run:
    python3 -m pytest tests/test_show_alarms.py -v
"""

import json
import os
import sys
import unittest
from unittest.mock import patch, MagicMock
from click.testing import CliRunner

TEST_DIR = os.path.dirname(os.path.abspath(__file__))
ALARMD_DIR = os.path.dirname(TEST_DIR)
# show/alarms.py lives in sonic-utilities — walk up from src/alarmd
# to the sonic-buildimage root (2 levels up):
#   alarmd/ -> src/ -> sonic-buildimage/
def walk_up(path, levels):
    for _ in range(levels):
        path = os.path.dirname(path)
    return path
REPO_ROOT = walk_up(ALARMD_DIR, 2)  # .../sonic-buildimage
SHOW_DIR = os.path.join(REPO_ROOT, 'src', 'sonic-utilities', 'show')

if not os.path.isdir(SHOW_DIR):
    raise unittest.SkipTest(
        f"sonic-utilities show/ not found at {SHOW_DIR} — "
        "test_show_alarms requires the full sonic-buildimage tree")

# Add show/ to path so we can import alarms.py
sys.path.insert(0, SHOW_DIR)
sys.path.insert(0, ALARMD_DIR)

# ---------------------------------------------------------------------------
# Mock data — eventd ALARM rows (EVENT_DB schema).  Severities are uppercase;
# time-created is a UTC-nanosecond string; acknowledged is a string boolean.
# ---------------------------------------------------------------------------
MOCK_ALARMS = [
    {
        'sequence-id': '1001',
        'type-id': 'PSU_MISSING',
        'resource': 'PSU 2',
        'severity': 'CRITICAL',
        'text': 'PSU 2 Absent',
        'time-created': '1740477600123000000',
        'acknowledged': 'false',
    },
    {
        'sequence-id': '1002',
        'type-id': 'FAN_FAULT',
        'resource': 'FAN 3',
        'severity': 'MAJOR',
        'text': 'FAN 3 Fault',
        'time-created': '1740477660456000000',
        'acknowledged': 'false',
    },
    {
        'sequence-id': '1003',
        'type-id': 'DISK_USAGE_HIGH',
        'resource': 'root-partition',
        'severity': 'MAJOR',
        'text': 'Root partition usage above threshold',
        'time-created': '1740477720789000000',
        'acknowledged': 'true',
    },
    {
        'sequence-id': '1004',
        'type-id': 'CPU_USAGE_HIGH',
        'resource': 'SYSTEM',
        'severity': 'MINOR',
        'text': 'CPU usage above threshold',
        'time-created': '1740477780000000000',
        'acknowledged': 'false',
    },
]

NO_ALARMS = []


# ---------------------------------------------------------------------------
# Import the CLI module with mocked swsscommon
# ---------------------------------------------------------------------------
# We need to mock swsscommon before importing alarms.py
mock_swsscommon = MagicMock()
sys.modules['swsscommon'] = mock_swsscommon
sys.modules['swsscommon.swsscommon'] = mock_swsscommon

import alarms as alarms_module


class TestShowAlarms(unittest.TestCase):
    """Tests for the 'show alarms' CLI command."""

    def setUp(self):
        self.runner = CliRunner()

    @patch.object(alarms_module, '_read_alarms', return_value=MOCK_ALARMS)
    def test_default_table_output(self, mock_read):
        result = self.runner.invoke(alarms_module.alarms, [])
        self.assertEqual(result.exit_code, 0)
        self.assertIn('PSU_MISSING', result.output)
        self.assertIn('FAN_FAULT', result.output)
        self.assertIn('DISK_USAGE_HIGH', result.output)
        self.assertIn('Total: 4 active alarm(s)', result.output)

    @patch.object(alarms_module, '_read_alarms', return_value=MOCK_ALARMS)
    def test_severity_ordering(self, mock_read):
        """CRITICAL should appear before MAJOR, MAJOR before MINOR."""
        result = self.runner.invoke(alarms_module.alarms, [])
        crit_pos = result.output.index('CRITICAL')
        major_pos = result.output.index('MAJOR')
        minor_pos = result.output.index('MINOR')
        self.assertLess(crit_pos, major_pos)
        self.assertLess(major_pos, minor_pos)

    @patch.object(alarms_module, '_read_alarms', return_value=MOCK_ALARMS)
    def test_filter_by_severity(self, mock_read):
        result = self.runner.invoke(alarms_module.alarms, ['-s', 'Critical'])
        self.assertEqual(result.exit_code, 0)
        self.assertIn('PSU_MISSING', result.output)
        self.assertNotIn('FAN_FAULT', result.output)
        self.assertIn('Total: 1 active alarm(s)', result.output)

    @patch.object(alarms_module, '_read_alarms', return_value=MOCK_ALARMS)
    def test_filter_by_id(self, mock_read):
        result = self.runner.invoke(alarms_module.alarms, ['-i', 'PSU_MISSING'])
        self.assertEqual(result.exit_code, 0)
        self.assertIn('PSU_MISSING', result.output)
        self.assertNotIn('FAN_FAULT', result.output)
        self.assertIn('Total: 1 active alarm(s)', result.output)

    @patch.object(alarms_module, '_read_alarms', return_value=MOCK_ALARMS)
    def test_filter_by_resource(self, mock_read):
        result = self.runner.invoke(alarms_module.alarms, ['-r', 'PSU 2'])
        self.assertEqual(result.exit_code, 0)
        self.assertIn('PSU_MISSING', result.output)
        self.assertNotIn('FAN_FAULT', result.output)
        self.assertIn('Total: 1 active alarm(s)', result.output)

    @patch.object(alarms_module, '_read_alarms', return_value=MOCK_ALARMS)
    def test_filter_by_acknowledged(self, mock_read):
        # Only DISK_USAGE_HIGH is acknowledged=true.
        result = self.runner.invoke(alarms_module.alarms, ['-a', 'true'])
        self.assertEqual(result.exit_code, 0)
        self.assertIn('DISK_USAGE_HIGH', result.output)
        self.assertNotIn('PSU_MISSING', result.output)
        self.assertIn('Total: 1 active alarm(s)', result.output)

    @patch.object(alarms_module, '_read_alarms', return_value=MOCK_ALARMS)
    def test_filter_by_unacknowledged(self, mock_read):
        result = self.runner.invoke(alarms_module.alarms, ['-a', 'false'])
        self.assertEqual(result.exit_code, 0)
        self.assertNotIn('DISK_USAGE_HIGH', result.output)
        self.assertIn('PSU_MISSING', result.output)
        self.assertIn('Total: 3 active alarm(s)', result.output)

    @patch.object(alarms_module, '_read_alarms', return_value=MOCK_ALARMS)
    def test_filter_case_insensitive(self, mock_read):
        result = self.runner.invoke(alarms_module.alarms, ['-s', 'critical'])
        self.assertIn('PSU_MISSING', result.output)

    @patch.object(alarms_module, '_read_alarms', return_value=MOCK_ALARMS)
    def test_json_output(self, mock_read):
        result = self.runner.invoke(alarms_module.alarms, ['--json'])
        self.assertEqual(result.exit_code, 0)
        data = json.loads(result.output)
        self.assertEqual(len(data), 4)
        alarm_ids = [a['type-id'] for a in data]
        self.assertIn('PSU_MISSING', alarm_ids)

    @patch.object(alarms_module, '_read_alarms', return_value=MOCK_ALARMS)
    def test_json_with_filter(self, mock_read):
        result = self.runner.invoke(alarms_module.alarms,
                                    ['--json', '-s', 'Critical'])
        data = json.loads(result.output)
        self.assertEqual(len(data), 1)
        self.assertEqual(data[0]['type-id'], 'PSU_MISSING')

    @patch.object(alarms_module, '_read_alarms', return_value=MOCK_ALARMS)
    def test_summary_output(self, mock_read):
        result = self.runner.invoke(alarms_module.alarms, ['--summary'])
        self.assertEqual(result.exit_code, 0)
        self.assertIn('Critical', result.output)
        self.assertIn('Major', result.output)
        self.assertIn('Minor', result.output)
        self.assertIn('Total', result.output)

    @patch.object(alarms_module, '_read_alarms', return_value=MOCK_ALARMS)
    def test_group_by_severity(self, mock_read):
        result = self.runner.invoke(alarms_module.alarms, ['-g', 'severity'])
        self.assertEqual(result.exit_code, 0)
        self.assertIn('severity: CRITICAL', result.output)
        self.assertIn('severity: MAJOR', result.output)
        self.assertIn('severity: MINOR', result.output)

    @patch.object(alarms_module, '_read_alarms', return_value=MOCK_ALARMS)
    def test_group_by_resource(self, mock_read):
        result = self.runner.invoke(alarms_module.alarms, ['-g', 'resource'])
        self.assertEqual(result.exit_code, 0)
        self.assertIn('resource: PSU 2', result.output)
        self.assertIn('resource: SYSTEM', result.output)

    @patch.object(alarms_module, '_read_alarms', return_value=NO_ALARMS)
    def test_no_alarms(self, mock_read):
        result = self.runner.invoke(alarms_module.alarms, [])
        self.assertEqual(result.exit_code, 0)
        self.assertIn('No active alarms', result.output)

    @patch.object(alarms_module, '_read_alarms', return_value=NO_ALARMS)
    def test_no_alarms_summary(self, mock_read):
        result = self.runner.invoke(alarms_module.alarms, ['--summary'])
        self.assertEqual(result.exit_code, 0)
        self.assertIn('Total', result.output)

    @patch.object(alarms_module, '_read_alarms', return_value=None)
    def test_framework_not_enabled(self, mock_read):
        """When EVENT_DB is absent, _read_alarms returns None and the command
        reports that the Event & Alarm Framework is not enabled."""
        result = self.runner.invoke(alarms_module.alarms, [])
        self.assertEqual(result.exit_code, 0)
        self.assertIn('not enabled', result.output)

    @patch.object(alarms_module, '_read_alarms', return_value=MOCK_ALARMS)
    def test_combined_filters(self, mock_read):
        """Multiple filters should AND together."""
        result = self.runner.invoke(alarms_module.alarms,
                                    ['-r', 'SYSTEM', '-s', 'Minor'])
        self.assertIn('CPU_USAGE_HIGH', result.output)
        self.assertNotIn('DISK_USAGE_HIGH', result.output)
        self.assertIn('Total: 1 active alarm(s)', result.output)

    @patch.object(alarms_module, '_read_alarms', return_value=MOCK_ALARMS)
    def test_filter_no_match(self, mock_read):
        result = self.runner.invoke(alarms_module.alarms, ['-r', 'NONEXISTENT'])
        self.assertIn('No active alarms', result.output)


class TestReadAlarms(unittest.TestCase):
    """Exercise the real _read_alarms() body with a mocked SonicV2Connector."""

    def _make_mock_db(self, keys, entries):
        """Build a mock SonicV2Connector that returns the given keys/entries.

        The mock exposes an EVENT_DB attribute so _get_event_db() treats the
        framework as present.
        """
        mock_db = MagicMock()
        mock_db.EVENT_DB = 'EVENT_DB'
        mock_db.keys.return_value = keys
        mock_db.get_all.side_effect = lambda db, key: entries.get(key)
        return mock_db

    @staticmethod
    def _call_read_alarms(MockConnector):
        """Call the real _read_alarms() body with a mocked SonicV2Connector.

        Temporarily patches swsscommon so the lazy import inside
        _get_event_db() picks up our mock, then reloads the module.
        """
        mock_swss = MagicMock()
        mock_swss.SonicV2Connector = MockConnector
        with patch.dict('sys.modules', {
            'swsscommon': MagicMock(swsscommon=mock_swss),
            'swsscommon.swsscommon': mock_swss,
        }):
            import importlib
            import alarms as _mod
            importlib.reload(_mod)
            return _mod._read_alarms()

    def test_read_alarms_happy_path(self):
        """_read_alarms() returns alarm dicts from the ALARM table keys."""
        mock_db = self._make_mock_db(
            keys=['ALARM|1002', 'ALARM|1001'],
            entries={
                'ALARM|1001': {
                    'type-id': 'PSU_MISSING',
                    'resource': 'PSU 2',
                    'severity': 'CRITICAL',
                },
                'ALARM|1002': {
                    'type-id': 'FAN_FAULT',
                    'resource': 'FAN 3',
                    'severity': 'MAJOR',
                },
            },
        )
        MockConnector = MagicMock(return_value=mock_db)
        result = self._call_read_alarms(MockConnector)
        self.assertEqual(len(result), 2)
        # keys are sorted -> ALARM|1001 (PSU_MISSING) first
        self.assertEqual(result[0]['type-id'], 'PSU_MISSING')
        self.assertEqual(result[0]['sequence-id'], '1001')
        self.assertEqual(result[1]['type-id'], 'FAN_FAULT')

    def test_read_alarms_no_keys(self):
        """_read_alarms() returns [] when EVENT_DB has no ALARM keys."""
        mock_db = self._make_mock_db(keys=None, entries={})
        MockConnector = MagicMock(return_value=mock_db)
        result = self._call_read_alarms(MockConnector)
        self.assertEqual(result, [])

    def test_read_alarms_get_all_returns_none(self):
        """_read_alarms() skips keys where get_all returns None."""
        mock_db = self._make_mock_db(
            keys=['ALARM|9'],
            entries={'ALARM|9': None},
        )
        MockConnector = MagicMock(return_value=mock_db)
        result = self._call_read_alarms(MockConnector)
        self.assertEqual(result, [])

    def test_read_alarms_framework_disabled(self):
        """When SonicV2Connector has no EVENT_DB attribute, _read_alarms()
        returns None (Event & Alarm Framework not enabled)."""
        mock_db = MagicMock(spec=[])   # no EVENT_DB attribute
        MockConnector = MagicMock(return_value=mock_db)
        result = self._call_read_alarms(MockConnector)
        self.assertIsNone(result)


if __name__ == '__main__':
    unittest.main()
