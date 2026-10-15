import sys
from unittest.mock import MagicMock, patch

# runner.py imports swsscommon at module load; tests do not require the C extension.
sys.modules.setdefault("swsscommon", MagicMock())
sys.modules.setdefault("swsscommon.swsscommon", sys.modules["swsscommon"])

from bgpcfgd.runner import Runner  # noqa: E402


class _Mgr(object):
    def __init__(self, db, table):
        self._db = db
        self._table = table
        self.handler = MagicMock()

    def get_database(self):
        return self._db

    def get_table_name(self):
        return self._table


def _patch_swsscommon(swss):
    swss.SonicDBConfig.getDbId.side_effect = lambda name: {
        "CONFIG_DB": 4, "STATE_DB": 6, "APPL_DB": 0
    }[name]

    def db_ctor(name, *args, **kwargs):
        conn = MagicMock()
        conn.getDbId.return_value = swss.SonicDBConfig.getDbId(name)
        return conn

    swss.DBConnector.side_effect = db_ctor

    def sst_ctor(conn, table_name):
        sst = MagicMock()
        sst.getTableName.return_value = table_name
        sst.getDbConnector.return_value = conn
        sst.pop.return_value = ("", "", [])
        return sst

    swss.SubscriberStateTable.side_effect = sst_ctor
    swss.Select.OBJECT = 0
    swss.Select.ERROR = 1
    swss.Select.TIMEOUT = 2
    selector = MagicMock()
    selector.TIMEOUT = swss.Select.TIMEOUT
    selector.ERROR = swss.Select.ERROR
    selector.select.return_value = (swss.Select.TIMEOUT, None)
    swss.Select.return_value = selector
    return swss


@patch("bgpcfgd.runner.swsscommon")
def test_subscribers_drain_in_add_manager_order(swss):
    """Reload KEYS dumps must apply INTERFACE tables before BGP_NEIGHBOR."""
    _patch_swsscommon(swss)
    runner = Runner(MagicMock())
    for table in (
        "INTERFACE",
        "LOOPBACK_INTERFACE",
        "VLAN_SUB_INTERFACE",
        "BGP_NEIGHBOR",
    ):
        runner.add_manager(_Mgr("CONFIG_DB", table))

    assert [s.getTableName() for s in runner.subscribers.values()] == [
        "INTERFACE",
        "LOOPBACK_INTERFACE",
        "VLAN_SUB_INTERFACE",
        "BGP_NEIGHBOR",
    ]


@patch("bgpcfgd.runner.swsscommon")
def test_one_subscriber_per_db_table(swss):
    _patch_swsscommon(swss)
    runner = Runner(MagicMock())
    runner.add_manager(_Mgr("CONFIG_DB", "DEVICE_METADATA"))
    runner.add_manager(_Mgr("CONFIG_DB", "DEVICE_METADATA"))

    assert list(runner.subscribers) == [(4, "DEVICE_METADATA")]
    assert len(runner.callbacks[4]["DEVICE_METADATA"]) == 2


@patch("bgpcfgd.runner.swsscommon")
def test_same_table_name_different_db_gets_two_subscribers(swss):
    _patch_swsscommon(swss)
    runner = Runner(MagicMock())
    runner.add_manager(_Mgr("CONFIG_DB", "STATIC_ROUTE"))
    runner.add_manager(_Mgr("APPL_DB", "STATIC_ROUTE"))

    assert list(runner.subscribers) == [(4, "STATIC_ROUTE"), (0, "STATIC_ROUTE")]
    assert swss.SubscriberStateTable.call_count == 2

