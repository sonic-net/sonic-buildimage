import pytest
import sys
from common_utils import MockSubscribeTable, get_subscribe_table_tested_data, \
    PORT_MODE_CHECKER, mock_exit_func
from dhcp_utilities.common.dhcp_db_monitor import DhcpRelaydDbMonitor, DhcpServdDbMonitor, ConfigDbEventChecker, \
    DhcpServerTableIntfEnablementEventChecker, DhcpServerTableCfgChangeEventChecker, \
    DhcpPortTableEventChecker, DhcpRangeTableEventChecker, DhcpOptionTableEventChecker, \
    VlanTableEventChecker, VlanMemberTableEventChecker, VlanIntfTableEventChecker, DhcpServerFeatureStateChecker, \
    MidPlaneTableEventChecker, DpusTableEventChecker
from dhcp_utilities.common.utils import DhcpDbConnector
from swsscommon import swsscommon
from unittest.mock import patch, ANY, PropertyMock, MagicMock


@pytest.mark.parametrize("checker_enabled", [True, False])
@pytest.mark.parametrize("select_result", [swsscommon.Select.TIMEOUT, swsscommon.Select.OBJECT])
def test_dhcp_relayd_monitor_check_db_update(mock_swsscommon_dbconnector_init, select_result, checker_enabled):
    with patch.object(DhcpServerTableIntfEnablementEventChecker, "check_update_event") \
        as mock_check_update_event, \
         patch.object(ConfigDbEventChecker, "is_enabled", return_value=checker_enabled), \
         patch.object(VlanTableEventChecker, "check_update_event") as mock_check_vlan_update, \
         patch.object(VlanIntfTableEventChecker, "check_update_event") as mock_check_vlan_intf_update, \
         patch.object(swsscommon.Select, "select", return_value=(select_result, None)), \
         patch.object(ConfigDbEventChecker, "enable"):
        db_connector = DhcpDbConnector()
        checkers = [VlanTableEventChecker(None, None), VlanIntfTableEventChecker(None, None),
                    DhcpServerTableIntfEnablementEventChecker(None, None)]
        dhcp_relayd_db_monitor = DhcpRelaydDbMonitor(db_connector, swsscommon.Select(), checkers)
        tested_db_snapshot = {"enabled_dhcp_interfaces": "dummy"}
        dhcp_relayd_db_monitor.check_db_update(tested_db_snapshot)
        if select_result == swsscommon.Select.OBJECT and checker_enabled:
            mock_check_vlan_update.assert_called_once_with(tested_db_snapshot)
            mock_check_update_event.assert_called_once_with(tested_db_snapshot)
            mock_check_vlan_intf_update.assert_called_once_with(tested_db_snapshot)
        else:
            mock_check_vlan_update.assert_not_called()
            mock_check_update_event.assert_not_called()
            mock_check_vlan_intf_update.assert_not_called()


@pytest.mark.parametrize("tables", [["VlanTableEventChecker"], ["dummy"]])
def test_dhcp_relayd_enable_checker(tables, mock_swsscommon_dbconnector_init):
    with patch.object(ConfigDbEventChecker, "enable") as mock_enable:
        db_connector = DhcpDbConnector()
        dhcp_relayd_db_monitor = DhcpRelaydDbMonitor(db_connector, None, [VlanTableEventChecker(None, None)])
        dhcp_relayd_db_monitor.enable_checkers(set(tables))
        if "VlanTableEventChecker" in tables:
            mock_enable.assert_called_once()
        else:
            mock_enable.assert_not_called()


@pytest.mark.parametrize("tables", [["VlanTableEventChecker"], ["dummy"]])
def test_dhcp_relayd_disable_checker(tables, mock_swsscommon_dbconnector_init):
    with patch.object(ConfigDbEventChecker, "disable") as mock_disable:
        db_connector = DhcpDbConnector()
        checker = VlanTableEventChecker(None, None)
        checker.is_enabled = True
        dhcp_relayd_db_monitor = DhcpRelaydDbMonitor(db_connector, None, [checker])
        dhcp_relayd_db_monitor.disable_checkers(set(tables))
        if "VlanTableEventChecker" in tables:
            mock_disable.assert_called_once()
        else:
            mock_disable.assert_not_called()


@pytest.mark.parametrize("select_result", [swsscommon.Select.TIMEOUT, swsscommon.Select.OBJECT])
@pytest.mark.parametrize("is_checker_enabled", [True, False])
def test_dhcp_servd_monitor_check_db_update(mock_swsscommon_dbconnector_init, select_result,
                                            is_checker_enabled):
    with patch.object(DhcpServerTableCfgChangeEventChecker, "check_update_event") \
        as mock_check_dhcp_server_update_event, \
         patch.object(DhcpPortTableEventChecker, "check_update_event") as mock_check_dhcp_server_port_update, \
         patch.object(DhcpRangeTableEventChecker, "check_update_event") as mock_check_dhcp_server_range_update, \
         patch.object(DhcpOptionTableEventChecker, "check_update_event") as mock_check_dhcp_server_option_update, \
         patch.object(VlanMemberTableEventChecker, "check_update_event") as mock_check_vlan_member_update, \
         patch.object(VlanTableEventChecker, "check_update_event") as mock_check_vlan_update, \
         patch.object(VlanIntfTableEventChecker, "check_update_event") as mock_check_vlan_intf_update, \
         patch.object(ConfigDbEventChecker, "is_enabled", return_value=is_checker_enabled), \
         patch.object(swsscommon.Select, "select", return_value=(select_result, None)), \
         patch.object(ConfigDbEventChecker, "clear_event") as mock_clear:
        db_connector = DhcpDbConnector()
        dhcp_checker = DhcpServerTableCfgChangeEventChecker(None, None)
        dhcp_checker.enabled = True
        vlan_checker = VlanIntfTableEventChecker(None, None)
        db_monitor = DhcpServdDbMonitor(db_connector, swsscommon.Select(), [dhcp_checker, vlan_checker])
        tested_db_snapshot = {"enabled_dhcp_interfaces": "dummy1", "used_range": "dummy2",
                              "used_options": "dummy3"}
        db_monitor.check_db_update(tested_db_snapshot)
        if select_result == swsscommon.Select.OBJECT and is_checker_enabled:
            mock_check_dhcp_server_update_event.assert_called_once_with(tested_db_snapshot)
            mock_check_vlan_update.assert_not_called()
            mock_clear.assert_called_once_with()
        else:
            mock_check_dhcp_server_update_event.assert_not_called()
            mock_check_dhcp_server_port_update.assert_not_called()
            mock_check_dhcp_server_range_update.assert_not_called()
            mock_check_vlan_member_update.assert_not_called()
            mock_check_dhcp_server_option_update.assert_not_called()
            mock_check_vlan_update.assert_not_called()
            mock_check_vlan_intf_update.assert_not_called()
            mock_clear.assert_not_called()


@pytest.mark.parametrize("tables", [set(["VlanIntfTableEventChecker"]), set(["dummy1"])])
def test_dhcp_servd_monitor_enable_checkers(mock_swsscommon_dbconnector_init, tables):
    with patch.object(ConfigDbEventChecker, "enable") as mock_enable:
        db_connector = DhcpDbConnector()
        checker = VlanIntfTableEventChecker(None, None)
        db_monitor = DhcpServdDbMonitor(db_connector, None, [checker])
        db_monitor.enable_checkers(tables)
        if tables == set(["VlanIntfTableEventChecker"]):
            mock_enable.assert_called_once()
        else:
            mock_enable.assert_not_called()


@pytest.mark.parametrize("tables", [set(PORT_MODE_CHECKER), set(["dummy"])])
def test_dhcp_servd_monitor_disble_checkers(mock_swsscommon_dbconnector_init, tables):
    with patch.object(ConfigDbEventChecker, "disable") as mock_disable:
        db_connector = DhcpDbConnector()
        checker = DhcpPortTableEventChecker(None, None)
        db_monitor = DhcpServdDbMonitor(db_connector, None, [checker])
        db_monitor.disable_checkers(tables)
        if tables == set(PORT_MODE_CHECKER):
            mock_disable.assert_called_once_with()
        else:
            mock_disable.assert_not_called()


def test_db_event_checker_init(mock_swsscommon_dbconnector_init):
    sel = swsscommon.Select()
    db_event_checker = ConfigDbEventChecker(sel, MagicMock())
    try:
        db_event_checker._get_parameter(None)
    except NotImplementedError:
        pass
    else:
        pytest.fail("Run _get_parameter didn't get error")
    try:
        db_event_checker._process_check(None, None, None, None)
    except NotImplementedError:
        pass
    else:
        pytest.fail("Run _process_check didn't get error")


@pytest.mark.parametrize("tested_data", get_subscribe_table_tested_data("test_table_clear"))
def test_db_event_checker_clear_event(mock_swsscommon_dbconnector_init, tested_data):
    with patch.object(ConfigDbEventChecker, "enable"), \
         patch.object(ConfigDbEventChecker, "subscriber_state_table",
                      return_value=MockSubscribeTable(tested_data["table"]), new_callable=PropertyMock):
        sel = swsscommon.Select()
        db_event_checker = ConfigDbEventChecker(sel, MagicMock())
        db_event_checker.enabled = True
        assert db_event_checker.subscriber_state_table.hasData()
        db_event_checker.clear_event()
        assert not db_event_checker.subscriber_state_table.hasData()


@pytest.mark.parametrize("is_enabled", [True, False])
def test_db_event_checker_is_enabled(is_enabled):
    sel = swsscommon.Select()
    db_event_checker = ConfigDbEventChecker(sel, MagicMock())
    db_event_checker.enabled = is_enabled
    assert db_event_checker.is_enabled() == is_enabled


@pytest.mark.parametrize("param_name", ["param1", "param2"])
def test_db_event_checker_check_db_snapshot(mock_swsscommon_dbconnector_init, param_name):
    sel = swsscommon.Select()
    db_event_checker = ConfigDbEventChecker(sel, MagicMock())
    tested_db_snapshot = {"param1": "value1"}
    check_res = db_event_checker._check_db_snapshot(tested_db_snapshot, param_name)
    assert check_res == (param_name in tested_db_snapshot)


@pytest.mark.parametrize("enabled", [True, False])
def test_db_event_checker_disable(mock_swsscommon_dbconnector_init, enabled):
    with patch.object(swsscommon.Select, "removeSelectable") as mock_remove, \
         patch.object(ConfigDbEventChecker, "enabled", return_value=enabled, new_callable=PropertyMock), \
         patch.object(sys, "exit", side_effect=mock_exit_func) as mock_exit:
        sel = swsscommon.Select()
        db_event_checker = ConfigDbEventChecker(sel, MagicMock())
        try:
            db_event_checker.disable()
        except SystemExit:
            mock_remove.assert_not_called()
            mock_exit.assert_called_once_with(1)
        else:
            mock_remove.assert_called_once_with(None)
            mock_exit.assert_not_called()


@pytest.mark.parametrize("enabled", [True, False])
def test_db_event_checker_subscribe_table(mock_swsscommon_dbconnector_init, enabled):
    with patch.object(ConfigDbEventChecker, "enabled", return_value=enabled, new_callable=PropertyMock), \
         patch.object(sys, "exit", side_effect=mock_exit_func) as mock_exit, \
         patch.object(swsscommon, "SubscriberStateTable") as mock_sub, \
         patch.object(swsscommon.Select, "addSelectable") as mock_add_sel_tbl:
        sel = swsscommon.Select()
        db_event_checker = ConfigDbEventChecker(sel, MagicMock())
        try:
            db_event_checker.enable()
        except SystemExit:
            mock_exit.assert_called_once_with(1)
            mock_add_sel_tbl.assert_not_called()
            mock_sub.assert_not_called()
        else:
            mock_exit.assert_not_called()
            mock_add_sel_tbl.assert_called_once_with(ANY)
            mock_sub.assert_called_once_with(ANY, "")


@pytest.mark.parametrize("tested_db_snapshot", [{"enabled_dhcp_interfaces": {"Vlan1000"}}, {}])
@pytest.mark.parametrize("tested_data", get_subscribe_table_tested_data("test_dhcp_server_update"))
def test_dhcp_server_table_cfg_change_checker(mock_swsscommon_dbconnector_init, tested_data, tested_db_snapshot):
    with patch.object(ConfigDbEventChecker, "enable"), \
         patch.object(ConfigDbEventChecker, "subscriber_state_table",
                      return_value=MockSubscribeTable(tested_data["table"]), new_callable=PropertyMock), \
         patch.object(sys, "exit"):
        sel = swsscommon.Select()
        db_event_checker = DhcpServerTableCfgChangeEventChecker(sel, MagicMock())
        expected_res = tested_data["exp_res"] if isinstance(tested_data["exp_res"], bool) else \
            tested_data["exp_res"]["cfg_change"]
        check_res = db_event_checker.check_update_event(tested_db_snapshot)
        if "enabled_dhcp_interfaces" not in tested_db_snapshot:
            assert check_res
        else:
            assert expected_res == check_res


@pytest.mark.parametrize("tested_db_snapshot", [{"enabled_dhcp_interfaces": {"Vlan1000"}}, {}])
@pytest.mark.parametrize("tested_data", get_subscribe_table_tested_data("test_dhcp_server_update"))
def test_dhcp_server_table_enablement_change_checker(mock_swsscommon_dbconnector_init, tested_data, tested_db_snapshot):
    with patch.object(ConfigDbEventChecker, "enable"), \
         patch.object(ConfigDbEventChecker, "subscriber_state_table",
                      return_value=MockSubscribeTable(tested_data["table"]), new_callable=PropertyMock), \
         patch.object(sys, "exit"):
        sel = swsscommon.Select()
        db_event_checker = DhcpServerTableIntfEnablementEventChecker(sel, MagicMock())
        expected_res = tested_data["exp_res"] if isinstance(tested_data["exp_res"], bool) else \
            tested_data["exp_res"]["enablement"]
        check_res = db_event_checker.check_update_event(tested_db_snapshot)
        if "enabled_dhcp_interfaces" not in tested_db_snapshot:
            assert check_res
        else:
            assert expected_res == check_res


@pytest.mark.parametrize("tested_db_snapshot", [{"enabled_dhcp_interfaces": {"Vlan1000"}}, {}])
@pytest.mark.parametrize("tested_data", get_subscribe_table_tested_data("test_port_update"))
def test_dhcp_port_table_checker(mock_swsscommon_dbconnector_init, tested_data, tested_db_snapshot):
    with patch.object(ConfigDbEventChecker, "enable"), \
         patch.object(ConfigDbEventChecker, "subscriber_state_table",
                      return_value=MockSubscribeTable(tested_data["table"]), new_callable=PropertyMock), \
         patch.object(sys, "exit"):
        sel = swsscommon.Select()
        db_event_checker = DhcpPortTableEventChecker(sel, MagicMock())
        expected_res = tested_data["exp_res"]
        check_res = db_event_checker.check_update_event(tested_db_snapshot)
        if "enabled_dhcp_interfaces" not in tested_db_snapshot:
            assert check_res
        else:
            assert expected_res == check_res


@pytest.mark.parametrize("tested_db_snapshot", [{"used_range": {"range1"}}, {}])
@pytest.mark.parametrize("tested_data", get_subscribe_table_tested_data("test_range_update"))
def test_dhcp_range_table_checker(mock_swsscommon_dbconnector_init, tested_data, tested_db_snapshot):
    with patch.object(ConfigDbEventChecker, "enable"), \
         patch.object(ConfigDbEventChecker, "subscriber_state_table",
                      return_value=MockSubscribeTable(tested_data["table"]), new_callable=PropertyMock), \
         patch.object(sys, "exit"):
        sel = swsscommon.Select()
        db_event_checker = DhcpRangeTableEventChecker(sel, MagicMock())
        expected_res = tested_data["exp_res"]
        check_res = db_event_checker.check_update_event(tested_db_snapshot)
        if "used_range" not in tested_db_snapshot:
            assert check_res
        else:
            assert expected_res == check_res


@pytest.mark.parametrize("tested_db_snapshot", [{"used_options": {"option223"}}, {}])
@pytest.mark.parametrize("tested_data", get_subscribe_table_tested_data("test_option_update"))
def test_dhcp_option_table_checker(mock_swsscommon_dbconnector_init, tested_data, tested_db_snapshot):
    with patch.object(ConfigDbEventChecker, "enable"), \
         patch.object(ConfigDbEventChecker, "subscriber_state_table",
                      return_value=MockSubscribeTable(tested_data["table"]), new_callable=PropertyMock), \
         patch.object(sys, "exit"):
        sel = swsscommon.Select()
        db_event_checker = DhcpOptionTableEventChecker(sel, MagicMock())
        expected_res = tested_data["exp_res"]
        check_res = db_event_checker.check_update_event(tested_db_snapshot)
        if "used_options" not in tested_db_snapshot:
            assert check_res
        else:
            assert expected_res == check_res


@pytest.mark.parametrize("tested_db_snapshot", [{"enabled_dhcp_interfaces": {"Vlan1000"}}, {}])
@pytest.mark.parametrize("tested_data", get_subscribe_table_tested_data("test_vlan_update"))
def test_vlan_table_checker(mock_swsscommon_dbconnector_init, tested_data, tested_db_snapshot):
    with patch.object(ConfigDbEventChecker, "enable"), \
         patch.object(ConfigDbEventChecker, "subscriber_state_table",
                      return_value=MockSubscribeTable(tested_data["table"]), new_callable=PropertyMock), \
         patch.object(sys, "exit"):
        sel = swsscommon.Select()
        db_event_checker = VlanTableEventChecker(sel, MagicMock())
        expected_res = tested_data["exp_res"]
        check_res = db_event_checker.check_update_event(tested_db_snapshot)
        if "enabled_dhcp_interfaces" not in tested_db_snapshot:
            assert check_res
        else:
            assert expected_res == check_res


@pytest.mark.parametrize("tested_db_snapshot", [{"enabled_dhcp_interfaces": {"Vlan1000"}}, {}])
@pytest.mark.parametrize("tested_data", get_subscribe_table_tested_data("test_vlan_intf_update"))
def test_vlan_intf_table_checker(mock_swsscommon_dbconnector_init, tested_data, tested_db_snapshot):
    with patch.object(ConfigDbEventChecker, "enable"), \
         patch.object(ConfigDbEventChecker, "subscriber_state_table",
                      return_value=MockSubscribeTable(tested_data["table"]), new_callable=PropertyMock), \
         patch.object(sys, "exit"):
        sel = swsscommon.Select()
        db_event_checker = VlanIntfTableEventChecker(sel, MagicMock())
        expected_res = tested_data["exp_res"]
        check_res = db_event_checker.check_update_event(tested_db_snapshot)
        if "enabled_dhcp_interfaces" not in tested_db_snapshot:
            assert check_res
        else:
            assert expected_res == check_res


@pytest.mark.parametrize("tested_db_snapshot", [{"enabled_dhcp_interfaces": {"Vlan1000"}}, {}])
@pytest.mark.parametrize("tested_data", get_subscribe_table_tested_data("test_vlan_member_update"))
def test_vlan_member_table_checker(mock_swsscommon_dbconnector_init, tested_data, tested_db_snapshot):
    with patch.object(ConfigDbEventChecker, "enable"), \
         patch.object(ConfigDbEventChecker, "subscriber_state_table",
                      return_value=MockSubscribeTable(tested_data["table"]), new_callable=PropertyMock), \
         patch.object(sys, "exit"):
        sel = swsscommon.Select()
        db_event_checker = VlanMemberTableEventChecker(sel, MagicMock())
        expected_res = tested_data["exp_res"]
        check_res = db_event_checker.check_update_event(tested_db_snapshot)
        if "enabled_dhcp_interfaces" not in tested_db_snapshot:
            assert check_res
        else:
            assert expected_res == check_res


@pytest.mark.parametrize("tested_db_snapshot", [{"dhcp_server_feature_enabled": True},
                                                {"dhcp_server_feature_enabled": False}, {}])
@pytest.mark.parametrize("tested_data", get_subscribe_table_tested_data("test_feature_update"))
def test_feature_table_checker(mock_swsscommon_dbconnector_init, tested_data, tested_db_snapshot):
    with patch.object(ConfigDbEventChecker, "enable"), \
         patch.object(ConfigDbEventChecker, "subscriber_state_table",
                      return_value=MockSubscribeTable(tested_data["table"]), new_callable=PropertyMock), \
         patch.object(sys, "exit"):
        sel = swsscommon.Select()
        db_event_checker = DhcpServerFeatureStateChecker(sel, MagicMock())
        check_res = db_event_checker.check_update_event(tested_db_snapshot)
        if "dhcp_server_feature_enabled" not in tested_db_snapshot:
            assert check_res
        else:
            expected_res = tested_data["exp_res"]["pre_enabled"] if tested_db_snapshot["dhcp_server_feature_enabled"] \
                else tested_data["exp_res"]["pre_disabled"]
            assert expected_res == check_res


@pytest.mark.parametrize("tested_db_snapshot", [{"enabled_dhcp_interfaces": {"bridge-midplane"}}, {}])
@pytest.mark.parametrize("tested_data", get_subscribe_table_tested_data("test_mid_plane_update"))
def test_mid_plane_table_checker(mock_swsscommon_dbconnector_init, tested_data, tested_db_snapshot):
    with patch.object(ConfigDbEventChecker, "enable"), \
         patch.object(ConfigDbEventChecker, "subscriber_state_table",
                      return_value=MockSubscribeTable(tested_data["table"]), new_callable=PropertyMock), \
         patch.object(sys, "exit"):
        sel = swsscommon.Select()
        db_event_checker = MidPlaneTableEventChecker(sel, MagicMock())
        expected_res = tested_data["exp_res"]
        check_res = db_event_checker.check_update_event(tested_db_snapshot)
        if "enabled_dhcp_interfaces" not in tested_db_snapshot:
            assert check_res
        else:
            assert expected_res == check_res


@pytest.mark.parametrize("tested_data", get_subscribe_table_tested_data("test_dpus_update"))
def test_dpus_table_checker(mock_swsscommon_dbconnector_init, tested_data):
    with patch.object(ConfigDbEventChecker, "enable"), \
         patch.object(ConfigDbEventChecker, "subscriber_state_table",
                      return_value=MockSubscribeTable(tested_data["table"]), new_callable=PropertyMock), \
         patch.object(sys, "exit"):
        sel = swsscommon.Select()
        db_event_checker = DpusTableEventChecker(sel, MagicMock())
        expected_res = tested_data["exp_res"]
        check_res = db_event_checker.check_update_event({})
        assert expected_res == check_res
