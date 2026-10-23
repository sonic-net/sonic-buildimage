from unittest.mock import MagicMock, patch

from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from . import swsscommon_test

with patch.dict("sys.modules", swsscommon=swsscommon_test):
    from swsscommon import swsscommon
    from bgpcfgd.managers_db import BGPDataBaseMgr

def test_set_del_handler():
    cfg_mgr = MagicMock()
    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   cfg_mgr,
        'tf':        TemplateFabric(),
        'constants': {},
    }
    m = BGPDataBaseMgr(common_objs, "CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME)
    assert m.constants == {}

    # test set_handler
    res = m.set_handler("test_key1", {"test_value1"})
    assert res, "Returns always True"
    assert "test_key1" in m.directory.get_slot(m.db_name, m.table_name)
    assert m.directory.get(m.db_name, m.table_name, "test_key1") == {"test_value1"}

    res = m.set_handler("test_key2", {})
    assert res, "Returns always True"
    assert "test_key2" in m.directory.get_slot(m.db_name, m.table_name)
    assert m.directory.get(m.db_name, m.table_name, "test_key2") == {}

    # test del_handler
    m.del_handler("test_key")
    assert "test_key" not in m.directory.get_slot(m.db_name, m.table_name)
    assert "test_key2" in m.directory.get_slot(m.db_name, m.table_name)
    assert m.directory.get(m.db_name, m.table_name, "test_key2") == {}

    m.del_handler("test_key2")
    assert "test_key2" not in m.directory.get_slot(m.db_name, m.table_name)


def test_device_metadata_bgp_asn_validation():
    common_objs = {
        'directory': Directory(),
        'cfg_mgr': MagicMock(),
        'tf': TemplateFabric(),
        'constants': {},
    }
    m = BGPDataBaseMgr(common_objs, "CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME)

    for bgp_asn in ("1", "65100", "4294967295"):
        assert m.set_handler("localhost", {"bgp_asn": bgp_asn})
        assert m.directory.get(m.db_name, m.table_name, "localhost") == {"bgp_asn": bgp_asn}

    for bgp_asn in (
        "0", "4294967296", "-1", "+1", " 65100", "65100 ", "65.100",
        "65100\ninvalid", "１２３", "0" * 10 + "1", "9" * 5000, 65100, None,
    ):
        assert m.set_handler("localhost", {"bgp_asn": bgp_asn})
        assert m.directory.get(m.db_name, m.table_name, "localhost") == {"bgp_asn": "4294967295"}


def test_bgp_asn_validation_scope():
    common_objs = {
        'directory': Directory(),
        'cfg_mgr': MagicMock(),
        'tf': TemplateFabric(),
        'constants': {},
    }
    metadata_mgr = BGPDataBaseMgr(
        common_objs, "CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME
    )
    neighbor_mgr = BGPDataBaseMgr(
        common_objs, "CONFIG_DB", "DEVICE_NEIGHBOR_METADATA"
    )

    assert metadata_mgr.set_handler("other-host", {"bgp_asn": "not-an-asn"})
    assert metadata_mgr.directory.get(
        metadata_mgr.db_name, metadata_mgr.table_name, "other-host"
    ) == {"bgp_asn": "not-an-asn"}

    assert neighbor_mgr.set_handler("localhost", {"bgp_asn": "not-an-asn"})
    assert neighbor_mgr.directory.get(
        neighbor_mgr.db_name, neighbor_mgr.table_name, "localhost"
    ) == {"bgp_asn": "not-an-asn"}
