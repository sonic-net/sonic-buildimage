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
    DHCP_SERVER_CHECKER, VLAN_CHECKERS
from swsscommon import swsscommon
from unittest.mock import patch, call, PropertyMock


@pytest.mark.parametrize("dhcp_server_enabled", [True, False])
def test_start(mock_swsscommon_dbconnector_init, dhcp_server_enabled):
    with patch.object(DhcpRelayd, "_get_dhcp_relay_config") as mock_get_config, \
         patch.object(DhcpRelayd, "_is_dhcp_server_enabled", return_value=dhcp_server_enabled) as mock_enabled, \
         patch.object(DhcpRelayd, "_execute_supervisor_dhcp_relay_process") as mock_execute, \
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
            enabled_checkers.add("DhcpServerTableIntfEnablementEventChecker")
        else:
            mock_execute.assert_not_called()
        mock_enabled_checkers.assert_called_once_with(enabled_checkers)


def test_refresh_dhcrelay(mock_swsscommon_dbconnector_init):
    expected_checkers = set(["VlanIntfTableEventChecker", "VlanTableEventChecker"])
    dhcprelayd_refresh_dhcrelay_test(expected_checkers, False, mock_get_config_db_table)


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
            ]
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
