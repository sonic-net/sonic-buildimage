import pytest
from common_utils import MockSubscribeTable
from dhcp_server.common.dhcp_db_monitor import DhcpDbMonitor, DhcpRelaydDbMonitor
from dhcp_server.common.utils import DhcpDbConnector
from swsscommon import swsscommon
from unittest.mock import patch, call, ANY, PropertyMock

tested_subscribe_dhcp_server_table = [
    {
        "table": [
            ("Vlan1000", "SET", (("customized_options", "option1"), ("state", "enabled"),))
        ],
        "exp_res": False
    },
    {
        "table": [
            ("Vlan2000", "SET", (("state", "enabled"),))
        ],
        "exp_res": True
    },
    {
        "table": [
            ("Vlan1000", "DEL", ())
        ],
        "exp_res": True
    },
    {
        "table": [
            ("Vlan2000", "DEL", ())
        ],
        "exp_res": False
    },
    {
        "table": [
            ("Vlan2000", "DEL", ()),
            ("Vlan1000", "DEL", ())
        ],
        "exp_res": True
    },
    {
        "table": [
            ("Vlan3000", "SET", (("state", "enabled"),))
        ],
        "exp_res": True
    }
]
tested_subscribe_vlan_table = [
    {
        "table": [
            ("Vlan1000", "SET", (("vlanid", "1000"),))
        ],
        "exp_res": True
    },
    {
        "table": [
            ("Vlan1001", "SET", (("vlanid", "1001"),))
        ],
        "exp_res": False
    },
    {
        "table": [
            ("Vlan1000", "SET", (("vlanid", "1000"),)),
            ("Vlan1002", "SET", (("vlanid", "1002"),))
        ],
        "exp_res": True
    },
    {
        "table": [
            ("Vlan1001", "DEL", ())
        ],
        "exp_res": False
    },
    {
        "table": [
            ("Vlan1000", "DEL", ())
        ],
        "exp_res": True
    },
    {
        "table": [
            ("Vlan1000", "SET", (("vlanid", "1000"),)),
            ("Vlan1001", "DEL", ())
        ],
        "exp_res": True
    },
    {
        "table": [
            ("Vlan1003", "SET", (("vlanid", "1003"),))
        ],
        "exp_res": False
    }
]
tested_subscribe_vlan_intf_table = [
    {
        "table": [
            ("Vlan1000", "SET", ())
        ],
        "exp_res": False
    },
    {
        "table": [
            ("Vlan1000|192.168.0.1/24", "SET", ())
        ],
        "exp_res": True
    },
    {
        "table": [
            ("Vlan1000|fc02::8/64", "SET", ())
        ],
        "exp_res": False
    },
    {
        "table": [
            ("Vlan2000|192.168.0.1/24", "SET", ())
        ],
        "exp_res": False
    },
    {
        "table": [
            ("Vlan1001|192.168.0.1/24", "SET", ())
        ],
        "exp_res": False
    }
]


@pytest.mark.parametrize("select_result", [swsscommon.Select.TIMEOUT, swsscommon.Select.OBJECT])
def test_dhcp_db_monitor(mock_swsscommon_dbconnector_init, select_result):
    db_connector = DhcpDbConnector()
    dhcp_db_monitor = DhcpDbMonitor(db_connector)
    try:
        dhcp_db_monitor.subscribe_table()
    except NotImplementedError:
        pass
    try:
        dhcp_db_monitor._do_check()
    except NotImplementedError:
        pass
    with patch.object(DhcpDbMonitor, "_do_check", return_value=None) as mock_do_check, \
         patch.object(swsscommon.Select, "select", return_value=(select_result, None)):
        dhcp_db_monitor.check_db_update("mock_param")
        if select_result == swsscommon.Select.TIMEOUT:
            mock_do_check.assert_not_called()
        elif select_result == swsscommon.Select.OBJECT:
            mock_do_check.assert_called_once_with("mock_param")


def test_dhcp_relayd_monitor_subscribe_table(mock_swsscommon_dbconnector_init):
    with patch.object(swsscommon, "SubscriberStateTable", side_effect=mock_subscriber_state_table) as mock_subscribe, \
         patch.object(swsscommon.Select, "addSelectable", return_value=None) as mock_add_select:
        db_connector = DhcpDbConnector()
        dhcp_relayd_db_monitor = DhcpRelaydDbMonitor(db_connector)
        dhcp_relayd_db_monitor.subscribe_table()
        mock_subscribe.assert_has_calls([
            call(ANY, "DHCP_SERVER_IPV4"),
            call(ANY, "VLAN"),
            call(ANY, "VLAN_INTERFACE")
        ])
        mock_add_select.assert_has_calls([
            call("DHCP_SERVER_IPV4"),
            call("VLAN"),
            call("VLAN_INTERFACE")
        ])


@pytest.mark.parametrize("check_param", [{}, {"enabled_dhcp_interfaces": "dummy"}])
def test_dhcp_relayd_monitor_do_check(mock_swsscommon_dbconnector_init, check_param):
    with patch.object(DhcpRelaydDbMonitor, "_check_dhcp_server_update") as mock_check_dhcp_server_update, \
         patch.object(DhcpRelaydDbMonitor, "_check_vlan_update") as mock_check_vlan_update, \
         patch.object(DhcpRelaydDbMonitor, "_check_vlan_intf_update") as mock_check_vlan_intf_update:
        db_connector = DhcpDbConnector()
        dhcp_relayd_db_monitor = DhcpRelaydDbMonitor(db_connector)
        dhcp_relayd_db_monitor._do_check(check_param)
        if "enabled_dhcp_interfaces" in check_param:
            mock_check_dhcp_server_update.assert_called_once_with("dummy")
            mock_check_vlan_update.assert_called_once_with("dummy")
            mock_check_vlan_intf_update.assert_called_once_with("dummy")
        else:
            mock_check_dhcp_server_update.assert_not_called()
            mock_check_vlan_update.assert_not_called()
            mock_check_vlan_intf_update.assert_not_called()


@pytest.mark.parametrize("dhcp_server_table_update", tested_subscribe_dhcp_server_table)
def test_dhcp_relayd_monitor_check_dhcp_server_update(mock_swsscommon_dbconnector_init, dhcp_server_table_update):
    tested_table = dhcp_server_table_update["table"]
    with patch.object(DhcpRelaydDbMonitor, "subscribe_dhcp_server_table",
                      return_value=MockSubscribeTable(tested_table),
                      new_callable=PropertyMock):
        db_connector = DhcpDbConnector()
        dhcp_relayd_db_monitor = DhcpRelaydDbMonitor(db_connector)
        check_res = dhcp_relayd_db_monitor._check_dhcp_server_update(set(["Vlan1000"]))
        assert check_res == dhcp_server_table_update["exp_res"]


@pytest.mark.parametrize("vlan_table_update", tested_subscribe_vlan_table)
def test_dhcp_relayd_monitor_check_vlan_update(mock_swsscommon_dbconnector_init, vlan_table_update):
    tested_table = vlan_table_update["table"]
    with patch.object(DhcpRelaydDbMonitor, "subscribe_vlan_table", return_value=MockSubscribeTable(tested_table),
                      new_callable=PropertyMock):
        db_connector = DhcpDbConnector()
        dhcp_relayd_db_monitor = DhcpRelaydDbMonitor(db_connector)
        check_res = dhcp_relayd_db_monitor._check_vlan_update(set(["Vlan1000"]))
        assert check_res == vlan_table_update["exp_res"]


@pytest.mark.parametrize("vlan_intf_table_update", tested_subscribe_vlan_intf_table)
def test_dhcp_relayd_monitor_check_vlan_intf_update(mock_swsscommon_dbconnector_init, vlan_intf_table_update):
    tested_table = vlan_intf_table_update["table"]
    with patch.object(DhcpRelaydDbMonitor, "subscribe_vlan_intf_table", return_value=MockSubscribeTable(tested_table),
                      new_callable=PropertyMock):
        db_connector = DhcpDbConnector()
        dhcp_relayd_db_monitor = DhcpRelaydDbMonitor(db_connector)
        check_res = dhcp_relayd_db_monitor._check_vlan_intf_update(set(["Vlan1000"]))
        assert check_res == vlan_intf_table_update["exp_res"]


def mock_subscriber_state_table(db, table_name):
    return table_name
