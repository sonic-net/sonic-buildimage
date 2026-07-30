import psutil
import pytest
import subprocess
import sys
import time
from common_utils import mock_get_config_db_table, MockProc, MockPopen, MockSubprocessRes, mock_exit_func, \
    dhcprelayd_refresh_dhcrelay_test, dhcprelayd_proceed_with_check_res_test
from dhcp_utilities.common.utils import DhcpDbConnector
from dhcp_utilities.common.dhcp_db_monitor import ConfigDbEventChecker, DhcpRelaydDbMonitor
from dhcp_utilities.dhcprelayd.dhcprelayd import DhcpRelayd, KILLED_OLD, NOT_KILLED, NOT_FOUND_PROC, \
    FEATURE_CHECKER, DHCP_SERVER_CHECKER, DHCPV4_RELAY_CHECKER, VLAN_CHECKERS
from swsscommon import swsscommon
from unittest.mock import patch, call, PropertyMock


@pytest.mark.parametrize("dhcp_server_enabled", [True, False])
def test_start(mock_swsscommon_dbconnector_init, dhcp_server_enabled):
    with patch.object(DhcpRelayd, "_get_dhcp_relay_config") as mock_get_config, \
         patch.object(DhcpRelayd, "_is_dhcp_server_enabled", return_value=dhcp_server_enabled) as mock_enabled, \
         patch.object(DhcpRelayd, "_execute_supervisor_dhcp_relay_process") as mock_execute, \
         patch.object(DhcpRelayd, "refresh_dhcrelay") as mock_refresh, \
         patch.object(time, "sleep"), \
         patch.object(DhcpRelaydDbMonitor, "enable_checkers") as mock_enabled_checkers, \
         patch.object(DhcpDbConnector, "get_config_db_table", side_effect=mock_get_config_db_table):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, DhcpRelaydDbMonitor(None, None, []))
        dhcprelayd.start()
        mock_get_config.assert_called_once_with()
        mock_enabled.assert_called_once_with()
        enabled_checkers = set(["DhcpServerFeatureStateChecker"])
        if dhcp_server_enabled:
            mock_execute.assert_called_once_with("stop")
            mock_refresh.assert_called_once_with()
            enabled_checkers.add("DhcpServerTableIntfEnablementEventChecker")
        else:
            mock_execute.assert_not_called()
            mock_refresh.assert_not_called()
        mock_enabled_checkers.assert_called_once_with(enabled_checkers)


def test_start_enables_sonic_relay_checker(mock_swsscommon_dbconnector_init):
    def get_config_db_table(table_name):
        table = mock_get_config_db_table(table_name)
        if table_name == "DEVICE_METADATA":
            table["localhost"]["has_sonic_dhcpv4_relay"] = "True"
        return table

    with patch.object(DhcpRelayd, "_get_dhcp_relay_config"), \
         patch.object(DhcpRelayd, "_is_dhcp_server_enabled", return_value=False), \
         patch.object(time, "sleep"), \
         patch.object(DhcpRelaydDbMonitor, "enable_checkers") as mock_enabled_checkers, \
         patch.object(DhcpDbConnector, "get_config_db_table", side_effect=get_config_db_table):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, DhcpRelaydDbMonitor(None, None, []))
        dhcprelayd.start()
        mock_enabled_checkers.assert_called_once_with(
            {"DhcpServerFeatureStateChecker", DHCPV4_RELAY_CHECKER}
        )


def test_start_sonic_dhcp_server_defers_relay_ownership_until_refresh(
        mock_swsscommon_dbconnector_init):
    def get_config_db_table(table_name):
        table = mock_get_config_db_table(table_name)
        if table_name == "DEVICE_METADATA":
            table["localhost"]["has_sonic_dhcpv4_relay"] = "True"
        return table

    with patch.object(DhcpRelayd, "_get_dhcp_relay_config"), \
         patch.object(DhcpRelayd, "_is_dhcp_server_enabled", return_value=True), \
         patch.object(DhcpRelayd, "_execute_supervisor_dhcp_relay_process") as mock_execute, \
         patch.object(DhcpRelayd, "refresh_dhcrelay") as mock_refresh, \
         patch.object(time, "sleep"), \
         patch.object(DhcpRelaydDbMonitor, "enable_checkers"), \
         patch.object(DhcpDbConnector, "get_config_db_table", side_effect=get_config_db_table):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, DhcpRelaydDbMonitor(None, None, []))
        dhcprelayd.start()

        mock_execute.assert_not_called()
        mock_refresh.assert_called_once_with()


def test_refresh_dhcrelay(mock_swsscommon_dbconnector_init):
    expected_checkers = set(["VlanIntfTableEventChecker", "VlanTableEventChecker"])
    dhcprelayd_refresh_dhcrelay_test(expected_checkers, False, mock_get_config_db_table)


@pytest.mark.parametrize("has_sonic_dhcpv4_relay, subtype, supervisor_configured, expected_sonic_cmds", [
    (True, None, True, ["/usr/sbin/dhcp4relay"]),
    (True, None, False, ["/usr/sbin/dhcp4relay"]),
    (True, "DualToR", True, ["/usr/sbin/dhcp4relay", "-u", "Loopback0"]),
    (False, None, False, None)
])
def test_refresh_dhcrelay_selects_relay_implementation(mock_swsscommon_dbconnector_init,
                                                      has_sonic_dhcpv4_relay, subtype, supervisor_configured,
                                                      expected_sonic_cmds):
    def get_config_db_table(table_name):
        table = mock_get_config_db_table(table_name)
        if table_name == "DEVICE_METADATA":
            table["localhost"]["has_sonic_dhcpv4_relay"] = str(has_sonic_dhcpv4_relay)
            if subtype:
                table["localhost"]["subtype"] = subtype
        return table

    with patch.object(DhcpDbConnector, "get_config_db_table", side_effect=get_config_db_table), \
         patch.object(DhcpRelayd, "_get_dhcp_server_ip", return_value="240.127.1.2") as mock_get_server_ip, \
         patch.object(DhcpRelayd, "_start_dhcrelay_process") as mock_start_isc, \
         patch.object(DhcpRelayd, "_start_sonic_dhcp_relay_process") as mock_start_sonic, \
         patch.object(DhcpRelayd, "_enable_checkers"), \
         patch.object(DhcpRelayd, "_disable_checkers"), \
         patch.object(DhcpRelayd, "_execute_supervisor_dhcp_relay_process") as mock_execute, \
         patch.object(DhcpRelayd, "dhcp_relay_supervisor_config",
                      return_value={"dhcp4relay": ["/usr/sbin/dhcp4relay"]}
                      if supervisor_configured else {},
                      new_callable=PropertyMock), \
         patch.object(DhcpRelayd, "smart_switch", return_value=False, new_callable=PropertyMock):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, None)
        dhcprelayd.refresh_dhcrelay()

        expected_interfaces = {"Vlan1000", "Vlan3000"}
        if has_sonic_dhcpv4_relay:
            mock_start_sonic.assert_called_once_with(expected_interfaces, expected_sonic_cmds, False)
            assert dhcprelayd.sonic_dhcp_server_relay_active
            if supervisor_configured:
                mock_execute.assert_called_once_with("stop", ["dhcp4relay"])
            else:
                mock_execute.assert_not_called()
            mock_start_isc.assert_not_called()
            mock_get_server_ip.assert_not_called()
        else:
            mock_start_isc.assert_called_once_with(expected_interfaces, "240.127.1.2", False)
            mock_start_sonic.assert_not_called()
            mock_get_server_ip.assert_called_once_with()
            assert not dhcprelayd.sonic_dhcp_server_relay_active
            mock_execute.assert_not_called()


def test_sonic_external_relay_remains_supervisor_owned_without_local_interfaces(
        mock_swsscommon_dbconnector_init):
    def get_config_db_table(table_name):
        table = mock_get_config_db_table(table_name)
        if table_name == "DEVICE_METADATA":
            table["localhost"]["has_sonic_dhcpv4_relay"] = "True"
        elif table_name == "DHCP_SERVER_IPV4":
            return {}
        elif table_name == "DHCPV4_RELAY":
            return {"Vlan1000": {"dhcpv4_servers": ["192.0.0.1"]}}
        return table

    with patch.object(DhcpDbConnector, "get_config_db_table", side_effect=get_config_db_table), \
         patch.object(DhcpRelayd, "_enable_checkers"), \
         patch.object(DhcpRelayd, "_disable_checkers"), \
         patch.object(DhcpRelayd, "dhcp_relay_supervisor_config",
                      return_value={"dhcp4relay": ["/usr/sbin/dhcp4relay"]},
                      new_callable=PropertyMock), \
         patch.object(DhcpRelayd, "_execute_supervisor_dhcp_relay_process") as mock_execute, \
         patch.object(DhcpRelayd, "_start_sonic_dhcp_relay_process") as mock_start, \
         patch.object(DhcpRelayd, "_kill_exist_relay_releated_process") as mock_kill, \
         patch.object(DhcpRelayd, "smart_switch", return_value=False, new_callable=PropertyMock):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, None)
        dhcprelayd.refresh_dhcrelay()

        assert not dhcprelayd.sonic_dhcp_server_relay_active
        mock_execute.assert_not_called()
        mock_start.assert_not_called()
        mock_kill.assert_not_called()


@pytest.mark.parametrize("supervisor_configured", [True, False])
def test_sonic_local_interface_removal_restores_external_relay(
        mock_swsscommon_dbconnector_init, supervisor_configured):
    def get_config_db_table(table_name):
        table = mock_get_config_db_table(table_name)
        if table_name == "DEVICE_METADATA":
            table["localhost"]["has_sonic_dhcpv4_relay"] = "True"
        elif table_name == "DHCP_SERVER_IPV4":
            return {}
        return table

    supervisor_config = {"dhcp4relay": ["/usr/sbin/dhcp4relay"]} if supervisor_configured else {}
    with patch.object(DhcpDbConnector, "get_config_db_table", side_effect=get_config_db_table), \
         patch.object(DhcpRelayd, "_enable_checkers"), \
         patch.object(DhcpRelayd, "_disable_checkers"), \
         patch.object(DhcpRelayd, "dhcp_relay_supervisor_config",
                      return_value=supervisor_config, new_callable=PropertyMock), \
         patch.object(DhcpRelayd, "_execute_supervisor_dhcp_relay_process") as mock_execute, \
         patch.object(DhcpRelayd, "_start_sonic_dhcp_relay_process") as mock_start, \
         patch.object(DhcpRelayd, "_kill_exist_relay_releated_process") as mock_kill, \
         patch.object(DhcpRelayd, "_check_sonic_dhcpv4_relay_config_transition") as mock_transition, \
         patch.object(DhcpRelayd, "smart_switch", return_value=False, new_callable=PropertyMock):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, None)
        dhcprelayd.sonic_dhcp_server_relay_active = True
        dhcprelayd.refresh_dhcrelay()

        assert not dhcprelayd.sonic_dhcp_server_relay_active
        mock_kill.assert_called_once_with([], "dhcp4relay", True)
        mock_transition.assert_called_once_with()
        mock_start.assert_not_called()
        if supervisor_configured:
            mock_execute.assert_called_once_with("start", ["dhcp4relay"])
        else:
            mock_execute.assert_not_called()


def test_sonic_relay_checker_survives_local_server_transition(mock_swsscommon_dbconnector_init):
    def get_config_db_table(table_name):
        table = mock_get_config_db_table(table_name)
        if table_name == "DEVICE_METADATA":
            table["localhost"]["has_sonic_dhcpv4_relay"] = "True"
        return table

    with patch.object(DhcpDbConnector, "get_config_db_table", side_effect=get_config_db_table), \
         patch.object(DhcpRelaydDbMonitor, "enable_checkers"), \
         patch.object(DhcpRelaydDbMonitor, "disable_checkers"), \
         patch.object(DhcpRelayd, "_start_sonic_dhcp_relay_process"), \
         patch.object(DhcpRelayd, "_kill_exist_relay_releated_process"), \
         patch.object(DhcpRelayd, "_execute_supervisor_dhcp_relay_process") as mock_execute, \
         patch.object(DhcpRelayd, "dhcp_relay_supervisor_config",
                      return_value={"dhcp4relay": ["/usr/sbin/dhcp4relay"]},
                      new_callable=PropertyMock), \
         patch.object(DhcpRelayd, "_check_sonic_dhcpv4_relay_config_transition") as mock_transition, \
         patch.object(DhcpRelayd, "_check_dhcp_relay_processes"), \
         patch.object(DhcpRelayd, "smart_switch", return_value=False, new_callable=PropertyMock):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(
            dhcp_db_connector,
            DhcpRelaydDbMonitor(None, None, []),
            enabled_checkers=[FEATURE_CHECKER, DHCP_SERVER_CHECKER, DHCPV4_RELAY_CHECKER]
        )
        dhcprelayd.has_sonic_dhcpv4_relay = True
        dhcprelayd.dhcp_server_feature_enabled = True

        dhcprelayd.refresh_dhcrelay()
        assert DHCPV4_RELAY_CHECKER in dhcprelayd.enabled_checkers
        assert dhcprelayd.sonic_dhcp_server_relay_active

        dhcprelayd._proceed_with_check_res({FEATURE_CHECKER: True}, True)
        assert not dhcprelayd.dhcp_server_feature_enabled
        assert DHCPV4_RELAY_CHECKER in dhcprelayd.enabled_checkers
        assert not dhcprelayd.sonic_dhcp_server_relay_active

        # Later external zero/nonzero transitions must still reach the checker.
        dhcprelayd._proceed_with_check_res({DHCPV4_RELAY_CHECKER: True}, False)
        dhcprelayd._proceed_with_check_res({DHCPV4_RELAY_CHECKER: True}, False)
        assert mock_transition.call_count == 3
        mock_execute.assert_has_calls([
            call("stop", ["dhcp4relay"]),
            call("start", ["dhcp4relay"])
        ])


@pytest.mark.parametrize("new_dhcp_interfaces", [[], ["Vlan1000"], ["Vlan1000", "Vlan2000"]])
@pytest.mark.parametrize("kill_res", [KILLED_OLD, NOT_KILLED, NOT_FOUND_PROC])
@pytest.mark.parametrize("proc_status", [psutil.STATUS_ZOMBIE, psutil.STATUS_RUNNING])
def test_start_dhcrelay_process(mock_swsscommon_dbconnector_init, new_dhcp_interfaces, kill_res, proc_status,):
    with patch.object(DhcpRelayd, "_kill_exist_relay_releated_process", return_value=kill_res), \
         patch.object(subprocess, "Popen", return_value=MockPopen(999)) as mock_popen, \
         patch.object(time, "sleep"), \
         patch("dhcp_utilities.dhcprelayd.dhcprelayd.terminate_proc", return_value=None) as mock_terminate, \
         patch.object(psutil.Process, "__init__", return_value=None), \
         patch.object(psutil.Process, "status", return_value=proc_status), \
         patch.object(sys, "exit") as mock_exit, \
         patch.object(ConfigDbEventChecker, "enable"):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, None)
        dhcprelayd._start_dhcrelay_process(new_dhcp_interfaces, "240.127.1.2", False)
        if len(new_dhcp_interfaces) == 0 or kill_res == NOT_KILLED:
            mock_popen.assert_not_called()
        else:
            call_param = ["/usr/sbin/dhcrelay", "-d", "-m", "discard", "-a", "%h:%p", "%P", "--name-alias-map-file",
                          "/tmp/port-name-alias-map.txt"]
            for interface in new_dhcp_interfaces:
                call_param += ["-id", interface]
            call_param += ["-iu", "docker0", "240.127.1.2"]
            mock_popen.assert_called_once_with(call_param)
        if len(new_dhcp_interfaces) != 0 and kill_res != NOT_KILLED and proc_status == psutil.STATUS_ZOMBIE:
            mock_terminate.assert_called_once()
            mock_exit.assert_called_once_with(1)
        else:
            mock_terminate.assert_not_called()
            mock_exit.assert_not_called()


@pytest.mark.parametrize("new_dhcp_interfaces", [[], ["Vlan1000"]])
@pytest.mark.parametrize("kill_res", [KILLED_OLD, NOT_KILLED, NOT_FOUND_PROC])
@pytest.mark.parametrize("proc_status", [psutil.STATUS_ZOMBIE, psutil.STATUS_RUNNING])
def test_start_sonic_dhcp_relay_process(mock_swsscommon_dbconnector_init, new_dhcp_interfaces, kill_res,
                                        proc_status):
    with patch.object(DhcpRelayd, "_kill_exist_relay_releated_process", return_value=kill_res), \
         patch.object(subprocess, "Popen", return_value=MockPopen(999)) as mock_popen, \
         patch.object(time, "sleep"), \
         patch("dhcp_utilities.dhcprelayd.dhcprelayd.terminate_proc", return_value=None) as mock_terminate, \
         patch.object(psutil.Process, "__init__", return_value=None), \
         patch.object(psutil.Process, "status", return_value=proc_status), \
         patch.object(sys, "exit") as mock_exit, \
         patch.object(ConfigDbEventChecker, "enable"):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, None)
        dhcprelayd._start_sonic_dhcp_relay_process(
            set(new_dhcp_interfaces), ["/usr/sbin/dhcp4relay"], False
        )

        if len(new_dhcp_interfaces) == 0 or kill_res == NOT_KILLED:
            mock_popen.assert_not_called()
        else:
            mock_popen.assert_called_once_with(["/usr/sbin/dhcp4relay"])

        if len(new_dhcp_interfaces) != 0 and kill_res != NOT_KILLED and proc_status == psutil.STATUS_ZOMBIE:
            mock_terminate.assert_called_once()
            mock_exit.assert_called_once_with(1)
        else:
            mock_terminate.assert_not_called()
            mock_exit.assert_not_called()


@pytest.mark.parametrize("new_dhcp_interfaces_list", [[], ["Vlan1000"], ["Vlan1000", "Vlan2000"]])
@pytest.mark.parametrize("kill_res", [KILLED_OLD, NOT_KILLED, NOT_FOUND_PROC])
@pytest.mark.parametrize("proc_status", [psutil.STATUS_ZOMBIE, psutil.STATUS_RUNNING])
def test_start_dhcpmon_process(mock_swsscommon_dbconnector_init, new_dhcp_interfaces_list, kill_res, proc_status):
    new_dhcp_interfaces = set(new_dhcp_interfaces_list)
    with patch.object(DhcpRelayd, "_kill_exist_relay_releated_process", return_value=kill_res), \
         patch.object(subprocess, "Popen", return_value=MockPopen(999)) as mock_popen, \
         patch.object(time, "sleep"), \
         patch("dhcp_utilities.dhcprelayd.dhcprelayd.terminate_proc", return_value=None) as mock_terminate, \
         patch.object(psutil.Process, "__init__", return_value=None), \
         patch.object(psutil.Process, "status", return_value=proc_status), \
         patch.object(ConfigDbEventChecker, "enable"):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, None)
        dhcprelayd._start_dhcpmon_process(new_dhcp_interfaces, False)
        if len(new_dhcp_interfaces) == 0 or kill_res == NOT_KILLED:
            mock_popen.assert_not_called()
        else:
            calls = []
            for interface in new_dhcp_interfaces:
                call_param = ["/usr/sbin/dhcpmon", "-id", interface, "-iu", "docker0", "-im", "eth0"]
                calls.append(call(call_param))
            mock_popen.assert_has_calls(calls)
        if len(new_dhcp_interfaces) != 0 and kill_res != NOT_KILLED and proc_status == psutil.STATUS_ZOMBIE:
            mock_terminate.assert_called_once()
        else:
            mock_terminate.assert_not_called()


@pytest.mark.parametrize("new_dhcp_interfaces_list", [[], ["Vlan1000"], ["Vlan1000", "Vlan2000"]])
@pytest.mark.parametrize("process_name", ["dhcrelay", "dhcpmon"])
@pytest.mark.parametrize("running_procs", [[], ["dhcrelay"], ["dhcpmon"], ["dhcrelay", "dhcpmon"]])
@pytest.mark.parametrize("force_kill", [True, False])
def test_kill_exist_relay_releated_process(mock_swsscommon_dbconnector_init, new_dhcp_interfaces_list, process_name,
                                           running_procs, force_kill):
    new_dhcp_interfaces = set(new_dhcp_interfaces_list)
    process_iter_ret = []
    for running_proc in running_procs:
        process_iter_ret.append(MockProc(running_proc))
    process_iter_ret.append(MockProc("exited_proc", exited=True))
    with patch.object(psutil, "process_iter", return_value=process_iter_ret), \
         patch.object(ConfigDbEventChecker, "enable"):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, None)
        res = dhcprelayd._kill_exist_relay_releated_process(new_dhcp_interfaces, process_name, force_kill)
        if force_kill and process_name in running_procs:
            assert res == KILLED_OLD
        elif new_dhcp_interfaces_list == ["Vlan1000"] and process_name in running_procs:
            assert res == NOT_KILLED
        elif process_name not in running_procs:
            assert res == NOT_FOUND_PROC
        elif new_dhcp_interfaces_list != ["Vlan1000"]:
            assert res == KILLED_OLD


@pytest.mark.parametrize("new_dhcp_interfaces", [set(), {"Vlan1000"}])
@pytest.mark.parametrize("force_kill", [True, False])
def test_kill_existing_sonic_dhcp_relay_process(mock_swsscommon_dbconnector_init, new_dhcp_interfaces, force_kill):
    process = MockProc("dhcp4relay")
    with patch.object(psutil, "process_iter", return_value=[process]), \
         patch.object(ConfigDbEventChecker, "enable"):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, None)
        res = dhcprelayd._kill_exist_relay_releated_process(
            new_dhcp_interfaces, "dhcp4relay", force_kill
        )

        if new_dhcp_interfaces and not force_kill:
            assert res == NOT_KILLED
        else:
            assert res == KILLED_OLD


@pytest.mark.parametrize("get_res", [(1, "240.127.1.2"), (0, None)])
def test_get_dhcp_server_ip(mock_swsscommon_dbconnector_init, mock_swsscommon_table_init, get_res):
    with patch.object(swsscommon.Table, "hget", return_value=get_res), \
         patch.object(time, "sleep") as mock_sleep, \
         patch.object(sys, "exit") as mock_exit, \
         patch.object(ConfigDbEventChecker, "enable"):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, None)
        ret = dhcprelayd._get_dhcp_server_ip()
        if get_res[0] == 1:
            assert ret == get_res[1]
        else:
            mock_exit.assert_called_once_with(1)
            mock_sleep.assert_has_calls([call(10) for _ in range(10)])


tested_feature_table = [
    {
        "dhcp_server": {
            "delayed": "True"
        }
    },
    {
        "dhcp_server": {
            "state": "enabled"
        }
    },
    {
        "dhcp_server": {
            "state": "disabled"
        }
    },
    {}
]


@pytest.mark.parametrize("feature_table", tested_feature_table)
def test_is_dhcp_server_enabled(mock_swsscommon_dbconnector_init, mock_swsscommon_table_init, feature_table):
    with patch.object(DhcpDbConnector, "get_config_db_table", return_value=feature_table):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, None)
        res = dhcprelayd._is_dhcp_server_enabled()
        if "dhcp_server" in feature_table and "state" in feature_table["dhcp_server"] and \
           feature_table["dhcp_server"]["state"] == "enabled":
            assert res
        else:
            assert not res


@pytest.mark.parametrize("relay_table, expected", [
    ({}, False),
    ({"Vlan1000": {"dhcpv4_servers": []}}, False),
    ({"Vlan1000": {"dhcpv4_servers": [""]}}, False),
    ({"Vlan1000": {"dhcpv4_servers": ["  "]}}, False),
    ({"Vlan1000": {"dhcpv4_servers": ["192.0.0.1"]}}, True)
])
def test_is_sonic_dhcpv4_relay_configured(mock_swsscommon_dbconnector_init, relay_table, expected):
    with patch.object(DhcpDbConnector, "get_config_db_table", return_value=relay_table):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, None)
        assert dhcprelayd._is_sonic_dhcpv4_relay_configured() == expected


@pytest.mark.parametrize("relay_configured, supervisor_configured, should_exit", [
    (False, False, False),
    (True, True, False),
    (True, False, True),
    (False, True, True)
])
def test_sonic_dhcpv4_relay_config_transition(mock_swsscommon_dbconnector_init, relay_configured,
                                              supervisor_configured, should_exit):
    relay_table = {"Vlan1000": {"dhcpv4_servers": ["192.0.0.1"]}} if relay_configured else {}
    supervisor_config = {"dhcp4relay": ["/usr/sbin/dhcp4relay"]} if supervisor_configured else {}
    with patch.object(DhcpDbConnector, "get_config_db_table", return_value=relay_table), \
         patch.object(DhcpRelayd, "dhcp_relay_supervisor_config", return_value=supervisor_config,
                      new_callable=PropertyMock), \
         patch.object(sys, "exit", side_effect=mock_exit_func) as mock_exit:
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, None)
        dhcprelayd.has_sonic_dhcpv4_relay = True
        try:
            dhcprelayd._check_sonic_dhcpv4_relay_config_transition()
        except SystemExit:
            assert should_exit
        else:
            assert not should_exit
        if should_exit:
            mock_exit.assert_called_once_with(1)
        else:
            mock_exit.assert_not_called()


@pytest.mark.parametrize("relay_config_changed", [True, False])
@pytest.mark.parametrize("local_relay_active", [True, False])
def test_dhcpv4_relay_config_change_is_checked(mock_swsscommon_dbconnector_init, local_relay_active,
                                               relay_config_changed):
    with patch.object(DhcpRelayd, "_check_sonic_dhcpv4_relay_config_transition") as mock_transition, \
         patch.object(DhcpRelayd, "_check_dhcp_relay_processes") as mock_check:
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, None)
        dhcprelayd.has_sonic_dhcpv4_relay = True
        dhcprelayd.dhcp_server_feature_enabled = True
        dhcprelayd.sonic_dhcp_server_relay_active = local_relay_active
        check_res = {DHCPV4_RELAY_CHECKER: True} if relay_config_changed else {}
        dhcprelayd._proceed_with_check_res(check_res, True)
        if relay_config_changed:
            mock_transition.assert_called_once_with()
        else:
            mock_transition.assert_not_called()
        if local_relay_active:
            mock_check.assert_not_called()
        else:
            mock_check.assert_called_once_with()


@pytest.mark.parametrize("op", ["stop", "start", "starts"])
@pytest.mark.parametrize("return_code", [0, -1])
def test_execute_supervisor_dhcp_relay_process(mock_swsscommon_dbconnector_init, mock_swsscommon_table_init, op,
                                               return_code):
    with patch.object(sys, "exit", side_effect=mock_exit_func) as mock_exit, \
         patch.object(subprocess, "run", return_value=MockSubprocessRes(return_code)) as mock_run, \
         patch.object(DhcpRelayd, "dhcp_relay_supervisor_config", return_value={"dhcpmon-Vlan1000": ""},
                      new_callable=PropertyMock):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, None)
        try:
            dhcprelayd._execute_supervisor_dhcp_relay_process(op)
        except SystemExit:
            mock_exit.assert_called_once_with(1)
            assert op == "starts" or return_code != 0
        else:
            mock_run.assert_called_once_with(["supervisorctl", op, "dhcpmon-Vlan1000"], check=True)


@pytest.mark.parametrize("op", ["stop", "start"])
def test_execute_grouped_sonic_dhcp_relay_process(mock_swsscommon_dbconnector_init, op):
    with patch.object(subprocess, "run", return_value=MockSubprocessRes(0)) as mock_run, \
         patch.object(DhcpRelayd, "dhcp_relay_supervisor_config",
                      return_value={"dhcp4relay": ["/usr/sbin/dhcp4relay"]},
                      new_callable=PropertyMock):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, None)
        dhcprelayd._execute_supervisor_dhcp_relay_process(op, ["dhcp4relay"])
        mock_run.assert_called_once_with(
            ["supervisorctl", op, "dhcp-relay:dhcp4relay"], check=True
        )


@pytest.mark.parametrize("op", ["stop", "start"])
def test_execute_standalone_sonic_dhcp_relay_process_fallback(mock_swsscommon_dbconnector_init, op):
    grouped_error = subprocess.CalledProcessError(1, ["supervisorctl", op, "dhcp-relay:dhcp4relay"])
    with patch.object(subprocess, "run",
                      side_effect=[grouped_error, MockSubprocessRes(0)]) as mock_run, \
         patch.object(DhcpRelayd, "dhcp_relay_supervisor_config",
                      return_value={"dhcp4relay": ["/usr/sbin/dhcp4relay"]},
                      new_callable=PropertyMock):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, None)
        dhcprelayd._execute_supervisor_dhcp_relay_process(op, ["dhcp4relay"])
        mock_run.assert_has_calls([
            call(["supervisorctl", op, "dhcp-relay:dhcp4relay"], check=True),
            call(["supervisorctl", op, "dhcp4relay"], check=True)
        ])


@pytest.mark.parametrize("iter_process", [
    [
        ["dhcrelay", 2, False, 1], ["dhcrelay", 3, False, 2], ["dhcpmon", 4, False, 1], ["dhcrelay", 5, True, 1]
    ],
    [
        ["dhcpmon", 4, False, 1]
    ]])
def test_check_dhcp_relay_process(mock_swsscommon_dbconnector_init, mock_swsscommon_table_init, iter_process):
    exp_config = {
        "isc-dhcpv4-relay-Vlan1000": ["/usr/sbin/dhcrelay", "-d", "-m", "discard", "-a", "%h:%p", "%P",
                                      "--name-alias-map-file", "/tmp/port-name-alias-map.txt", "-id", "Vlan1000",
                                      "-iu", "docker0", "240.127.1.2"],
        "dhcpmon-Vlan1000": ["/usr/sbin/dhcpmon", "-id", "Vlan1000", "-iu", "docker0", "-im", "eth0"]
    }
    process_iter_ret = [MockProc(name=item[0], pid=item[1], exited=item[2], ppid=item[3]) for item in iter_process]
    with patch.object(DhcpRelayd, "dhcp_relay_supervisor_config",
                      return_value=exp_config, new_callable=PropertyMock), \
         patch.object(sys, "exit", mock_exit_func), \
         patch.object(psutil, "process_iter", return_value=process_iter_ret):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, None)
        try:
            dhcprelayd._check_dhcp_relay_processes()
        except SystemExit:
            assert all(process[0] != "dhcrelay" for process in iter_process)
        else:
            assert any(process[0] == "dhcrelay" for process in iter_process)


@pytest.mark.parametrize("running", [True, False])
def test_check_sonic_dhcp_relay_process(mock_swsscommon_dbconnector_init, mock_swsscommon_table_init, running):
    process_iter_ret = [MockProc("dhcp4relay")] if running else []
    with patch.object(DhcpRelayd, "dhcp_relay_supervisor_config",
                      return_value={"dhcp4relay": ["/usr/sbin/dhcp4relay"]},
                      new_callable=PropertyMock), \
         patch.object(sys, "exit", mock_exit_func), \
         patch.object(psutil, "process_iter", return_value=process_iter_ret):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, None)
        try:
            dhcprelayd._check_dhcp_relay_processes()
        except SystemExit:
            assert not running
        else:
            assert running


def test_get_dhcp_relay_config(mock_swsscommon_dbconnector_init, mock_swsscommon_table_init):
    with patch.object(DhcpRelayd, "supervisord_conf_path", return_value="tests/test_data/supervisor.conf",
                      new_callable=PropertyMock):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, None)
        res = dhcprelayd._get_dhcp_relay_config()
        assert res == {
            "isc-dhcpv4-relay-Vlan1000": [
                "/usr/sbin/dhcrelay", "-d", "-m", "discard", "-a", "%h:%p", "%P", "--name-alias-map-file",
                "/tmp/port-name-alias-map.txt", "-id", "Vlan1000", "-iu", "PortChannel101", "-iu", "PortChannel102",
                "-iu", "PortChannel103", "-iu", "PortChannel104", "192.0.0.1", "192.0.0.2", "192.0.0.3", "192.0.0.4"
            ],
            "dhcpmon:dhcpmon-Vlan1000": [
                "/usr/sbin/dhcpmon", "-id", "Vlan1000", "-iu", "PortChannel101", "-iu", "PortChannel102", "-iu",
                "PortChannel103", "-iu", "PortChannel104", "-im", "eth0"
            ],
            "dhcp4relay": ["/usr/sbin/dhcp4relay"]
        }


@pytest.mark.parametrize("enabled_checkers", [set(["dummy"]), set()])
def test_enable_checkers(mock_swsscommon_dbconnector_init, mock_swsscommon_table_init, enabled_checkers):
    with patch.object(DhcpRelayd, "enabled_checkers", return_value=enabled_checkers, new_callable=PropertyMock), \
         patch.object(DhcpRelaydDbMonitor, "enable_checkers") as mock_enable_checkers:
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, DhcpRelaydDbMonitor(None, None, []))
        dhcprelayd._enable_checkers(["dummy"])
        mock_enable_checkers.assert_called_once_with(set() if "dummy" in enabled_checkers else set(["dummy"]))


@pytest.mark.parametrize("enabled_checkers", [set(["dummy"]), set()])
def test_disable_checkers(mock_swsscommon_dbconnector_init, mock_swsscommon_table_init, enabled_checkers):
    with patch.object(DhcpRelayd, "enabled_checkers", return_value=enabled_checkers, new_callable=PropertyMock), \
         patch.object(DhcpRelaydDbMonitor, "disable_checkers") as mock_disable_checkers:
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, DhcpRelaydDbMonitor(None, None, []))
        dhcprelayd._disable_checkers(["dummy"])
        mock_disable_checkers.assert_called_once_with(set() if "dummy" not in enabled_checkers else set(["dummy"]))


@pytest.mark.parametrize("feature_enabled", [True, False])
@pytest.mark.parametrize("feature_res", [True, False, None])
@pytest.mark.parametrize("dhcp_server_res", [True, False, None])
@pytest.mark.parametrize("vlan_intf_res", [True, False, None])
def test_proceed_with_check_res(mock_swsscommon_dbconnector_init, mock_swsscommon_table_init, feature_enabled,
                                feature_res, dhcp_server_res, vlan_intf_res):
    enabled_checkers = set([DHCP_SERVER_CHECKER] + VLAN_CHECKERS)
    expected_checkers = set([DHCP_SERVER_CHECKER] + VLAN_CHECKERS)
    dhcprelayd_proceed_with_check_res_test(enabled_checkers, feature_enabled, feature_res, dhcp_server_res,
                                           vlan_intf_res, False, expected_checkers)
