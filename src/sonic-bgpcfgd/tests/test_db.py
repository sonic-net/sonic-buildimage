from unittest.mock import MagicMock, patch

from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from . import swsscommon_test
from swsscommon import swsscommon

with patch.dict("sys.modules", swsscommon=swsscommon_test):
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

    res = m.set_handler("localhost", {'bgp_asn':'65100','suppress-fib-pending':'enabled'})
    assert res, "Returns always True"
    assert m.directory.get(m.db_name, m.table_name, "localhost") == {'bgp_asn':'65100','suppress-fib-pending':'enabled'}

    res = m.set_handler("localhost", {'bgp_asn':'65100','suppress-fib-pending':'disabled'})
    assert res, "Returns always True"
    assert m.directory.get(m.db_name, m.table_name, "localhost") == {'bgp_asn':'65100','suppress-fib-pending':'disabled'}

    # test del_handler
    m.del_handler("test_key")
    assert "test_key" not in m.directory.get_slot(m.db_name, m.table_name)
    assert "test_key2" in m.directory.get_slot(m.db_name, m.table_name)
    assert m.directory.get(m.db_name, m.table_name, "test_key2") == {}

    m.del_handler("test_key2")
    assert "test_key2" not in m.directory.get_slot(m.db_name, m.table_name)
