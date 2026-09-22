"""
test_event_publisher.py — unit tests for the new-design eventd output path.

Runs WITHOUT swsscommon installed (Tier-1 prototype): swsscommon and
sonic_py_common are mocked, and a FakeEventApi records what would have been
published to eventd.  This proves EventPublisher's contract:

  * publishes params  type-id / action / resource / text  (and NOT severity)
  * RAISE is idempotent (the _active cache owns de-dup, since eventd does not)
  * CLEAR is published once and only when an alarm is active
  * sync_active_set() adopts eventd's ALARM table on startup
  * reconcile_active_set() repairs drift
  * purge_stale_alarms() clears alarms whose id disappeared after reload

Run:
    cd alarmd && python3 -m pytest tests/test_event_publisher.py -v
"""

import os
import sys
import unittest
from unittest.mock import MagicMock

TEST_DIR = os.path.dirname(os.path.abspath(__file__))
ALARMD_DIR = os.path.dirname(TEST_DIR)
sys.path.insert(0, ALARMD_DIR)

# --- Mock sonic_py_common before importing sonic_alarm.event_publisher -------
_mock_syslogger_cls = MagicMock()
_mock_syslogger_cls.return_value = MagicMock()
_mock_sonic_py_common = MagicMock()
_mock_sonic_py_common.syslogger.SysLogger = _mock_syslogger_cls
sys.modules['sonic_py_common'] = _mock_sonic_py_common
sys.modules['sonic_py_common.syslogger'] = _mock_sonic_py_common.syslogger

# The sonic_alarm package __init__ imports statedb_poller (which hard-imports
# swsscommon at module load) and event_subscriber.  Provide a minimal
# swsscommon mock so the package imports here (this mirrors tests/test_alarmd.py).
# EventPublisher itself never touches this mock — it uses the injected
# FakeEventApi below, so the publish path under test is real code, not a mock.
_mock_swsscommon_inner = MagicMock()
_mock_swsscommon_pkg = MagicMock()
_mock_swsscommon_pkg.swsscommon = _mock_swsscommon_inner
sys.modules['swsscommon'] = _mock_swsscommon_pkg
sys.modules['swsscommon.swsscommon'] = _mock_swsscommon_inner

from sonic_alarm.event_publisher import (   # noqa: E402
    EventPublisher, ACTION_RAISE, ACTION_CLEAR,
    PARAM_TYPE_ID, PARAM_ACTION, PARAM_RESOURCE, PARAM_TEXT,
    EVENT_TAG,
)


class FakeEventApi:
    """Records publishes instead of talking to eventd."""

    def __init__(self, rc=0):
        self.published = []      # list of (tag, params_dict)
        self.handle = object()
        self._rc = rc

    def init_publisher(self, source):
        self.source = source
        return self.handle

    def deinit_publisher(self, handle):
        self.handle = None

    def publish(self, handle, tag, params):
        self.published.append((tag, dict(params)))
        return self._rc


class FakeAlarmTable:
    """Minimal swsscommon.Table stand-in for the ALARM table."""

    def __init__(self, rows):
        # rows: dict of key -> dict(fields).  Keyed by seq-id like real eventd.
        self._rows = rows

    def getKeys(self):
        return list(self._rows.keys())

    def get(self, key):
        if key in self._rows:
            return True, list(self._rows[key].items())
        return False, []


DEFS = {
    'alarm_tables': [
        {
            'type': 'statedb',
            'table_name': 'PSU_INFO',
            'checks': [
                {'check_name': 'psu_presence', 'alarm_id': 'PSU_MISSING',
                 'severity': 'Critical', 'category': 'Hardware',
                 'description_template': '{object_name} PSU Missing'},
            ],
        },
    ],
}


class TestEventPublisher(unittest.TestCase):

    def _mk(self, rc=0, alarm_table=None):
        api = FakeEventApi(rc=rc)
        pub = EventPublisher(DEFS, event_api=api, alarm_table=alarm_table)
        return pub, api

    def test_raise_publishes_correct_params(self):
        pub, api = self._mk()
        pub.set_alarm('PSU_MISSING', 'PSU 1', is_fault=True)

        self.assertEqual(len(api.published), 1)
        tag, params = api.published[0]
        self.assertEqual(tag, EVENT_TAG)
        self.assertEqual(params[PARAM_TYPE_ID], 'PSU_MISSING')
        self.assertEqual(params[PARAM_ACTION], ACTION_RAISE)
        self.assertEqual(params[PARAM_RESOURCE], 'PSU 1')
        self.assertEqual(params[PARAM_TEXT], 'PSU 1 PSU Missing')

    def test_severity_is_not_published(self):
        # eventd assigns severity from the profile; publisher must not send it.
        pub, api = self._mk()
        pub.set_alarm('PSU_MISSING', 'PSU 1', is_fault=True)
        _, params = api.published[0]
        self.assertNotIn('severity', params)

    def test_raise_is_idempotent(self):
        pub, api = self._mk()
        pub.set_alarm('PSU_MISSING', 'PSU 1', is_fault=True)
        pub.set_alarm('PSU_MISSING', 'PSU 1', is_fault=True)  # repeat fault
        pub.set_alarm('PSU_MISSING', 'PSU 1', is_fault=True)
        self.assertEqual(len(api.published), 1)   # only one RAISE
        self.assertEqual(pub.active_count, 1)

    def test_clear_publishes_once(self):
        pub, api = self._mk()
        pub.set_alarm('PSU_MISSING', 'PSU 1', is_fault=True)
        pub.set_alarm('PSU_MISSING', 'PSU 1', is_fault=False)   # clears
        pub.set_alarm('PSU_MISSING', 'PSU 1', is_fault=False)   # no-op
        actions = [p[PARAM_ACTION] for _, p in api.published]
        self.assertEqual(actions, [ACTION_RAISE, ACTION_CLEAR])
        self.assertEqual(pub.active_count, 0)

    def test_per_object_independence(self):
        pub, api = self._mk()
        pub.set_alarm('PSU_MISSING', 'PSU 1', is_fault=True)
        pub.set_alarm('PSU_MISSING', 'PSU 2', is_fault=True)
        self.assertEqual(pub.active_count, 2)
        self.assertEqual(len(api.published), 2)

    def test_failed_publish_rolls_back_active(self):
        # non-zero, non -1 rc means a real publish failure -> not cached active
        pub, api = self._mk(rc=5)
        pub.set_alarm('PSU_MISSING', 'PSU 1', is_fault=True)
        self.assertEqual(pub.active_count, 0)

    def test_sync_active_set_adopts_alarm_table(self):
        table = FakeAlarmTable({
            '1001': {'type-id': 'PSU_MISSING', 'resource': 'PSU 1',
                     'action': 'RAISE', 'severity': 'CRITICAL'},
            '1002': {'type-id': 'OTHER_ID', 'resource': 'X',   # not ours
                     'action': 'RAISE', 'severity': 'MINOR'},
        })
        pub, api = self._mk(alarm_table=table)
        pub.sync_active_set()
        self.assertIn('PSU_MISSING|PSU 1', pub.active_tags)
        self.assertNotIn('OTHER_ID|X', pub.active_tags)   # filtered by known ids
        # Already active -> a fault evaluation must NOT republish RAISE.
        pub.set_alarm('PSU_MISSING', 'PSU 1', is_fault=True)
        self.assertEqual(len(api.published), 0)

    def test_reconcile_drops_orphan_and_adopts_missing(self):
        table = FakeAlarmTable({
            '1001': {'type-id': 'PSU_MISSING', 'resource': 'PSU 2',
                     'action': 'RAISE'},
        })
        pub, api = self._mk(alarm_table=table)
        # Local cache thinks PSU 1 active (eventd lost it) and PSU 2 not.
        pub.set_alarm('PSU_MISSING', 'PSU 1', is_fault=True)
        pub.reconcile_active_set()
        self.assertIn('PSU_MISSING|PSU 2', pub.active_tags)     # adopted
        self.assertNotIn('PSU_MISSING|PSU 1', pub.active_tags)  # dropped

    def test_purge_stale_alarms_clears_removed_ids(self):
        pub, api = self._mk()
        pub.set_alarm('PSU_MISSING', 'PSU 1', is_fault=True)
        purged = pub.purge_stale_alarms(valid_alarm_ids=set())  # id removed
        self.assertEqual(purged, ['PSU_MISSING|PSU 1'])
        self.assertEqual(pub.active_count, 0)
        self.assertEqual(api.published[-1][1][PARAM_ACTION], ACTION_CLEAR)


if __name__ == '__main__':
    unittest.main(verbosity=2)
