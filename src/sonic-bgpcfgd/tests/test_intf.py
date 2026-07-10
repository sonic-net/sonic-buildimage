from unittest.mock import MagicMock, patch

from bgpcfgd.directory import Directory
from bgpcfgd.template import TemplateFabric
from swsscommon import swsscommon
from bgpcfgd.managers_intf import InterfaceMgr

def set_handler_test(manager, key, value):
    res = manager.set_handler(key, value)
    assert res, "Returns always True"
    assert manager.directory.get(manager.db_name, manager.table_name, key) == value

def del_handler_test(manager, key):
    manager.del_handler(key)
    assert manager.directory.get_path(manager.db_name, manager.table_name, key) == None

@patch('bgpcfgd.managers_intf.log_warn')
def test_intf(mocked_log_warn):
    cfg_mgr = MagicMock()
    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   cfg_mgr,
        'tf':        TemplateFabric(),
        'constants': {},
    }
    m = InterfaceMgr(common_objs, "CONFIG_DB", swsscommon.CFG_VLAN_INTF_TABLE_NAME)

    set_handler_test(m, "Vlan1000", {})
    set_handler_test(m, "Vlan1000|192.168.0.1/21", {})

    # test set handler with invalid ip network
    res = m.set_handler("Vlan1000|invalid_netowrk", {})
    assert res, "Returns always True"
    mocked_log_warn.assert_called_with("Subnet 'invalid_netowrk' format is wrong for interface 'Vlan1000'")

    del_handler_test(m, "Vlan1000")
    del_handler_test(m, "Vlan1000|192.168.0.1/21")
    del_handler_test(m, "Vlan1000|invalid_netowrk")
    mocked_log_warn.assert_called_with("Subnet 'invalid_netowrk' format is wrong for interface 'Vlan1000'")

def test_intf_ipv6():
    cfg_mgr = MagicMock()
    common_objs = {
        'directory': Directory(),
        'cfg_mgr':   cfg_mgr,
        'tf':        TemplateFabric(),
        'constants': {},
    }
    m = InterfaceMgr(common_objs, "CONFIG_DB", swsscommon.CFG_VLAN_INTF_TABLE_NAME)

    set_handler_test(m, "Vlan1000|fc02:1000::1/64", {})
    del_handler_test(m, "Vlan1000|fc02:1000::1/64")
