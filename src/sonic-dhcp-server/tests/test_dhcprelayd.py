import psutil
import pytest
import subprocess
import sys
import time
from common_utils import mock_get_config_db_table, MockProc, MockPopen
from dhcp_server.common.utils import DhcpDbConnector
from dhcp_server.dhcprelayd.dhcprelayd import DhcpRelayd, KILLED_OLD, NOT_KILLED, NOT_FOUND_PROC
from dhcp_server.common.dhcp_db_monitor import DhcpRelaydDbMonitor
from swsscommon import swsscommon
from unittest.mock import patch, call


def test_start(mock_swsscommon_dbconnector_init):
    with patch.object(DhcpRelayd, "refresh_dhcrelay", return_value=None) as mock_refresh, \
         patch.object(DhcpRelaydDbMonitor, "subscribe_table", return_value=None) as mock_subscribe:
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector)
        dhcprelayd.start()
        mock_refresh.assert_called_once_with()
        mock_subscribe.assert_called_once_with()


def test_refresh_dhcrelay(mock_swsscommon_dbconnector_init):
    with patch.object(DhcpRelayd, "_get_dhcp_server_ip", return_value="240.127.1.2"), \
         patch.object(DhcpDbConnector, "get_config_db_table", side_effect=mock_get_config_db_table), \
         patch.object(DhcpRelayd, "_start_dhcrelay_process", return_value=None), \
         patch.object(DhcpRelayd, "_start_dhcpmon_process", return_value=None):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector)
        dhcprelayd.refresh_dhcrelay()


@pytest.mark.parametrize("new_dhcp_interfaces", [[], ["Vlan1000"], ["Vlan1000", "Vlan2000"]])
@pytest.mark.parametrize("kill_res", [KILLED_OLD, NOT_KILLED, NOT_FOUND_PROC])
@pytest.mark.parametrize("proc_status", [psutil.STATUS_ZOMBIE, psutil.STATUS_RUNNING])
def test_start_dhcrelay_process(mock_swsscommon_dbconnector_init, new_dhcp_interfaces, kill_res, proc_status):
    with patch.object(DhcpRelayd, "_kill_exist_relay_releated_process", return_value=kill_res), \
         patch.object(subprocess, "Popen", return_value=MockPopen(999)) as mock_popen, \
         patch.object(time, "sleep"), \
         patch("dhcp_server.dhcprelayd.dhcprelayd.terminate_proc", return_value=None) as mock_terminate, \
         patch.object(psutil.Process, "__init__", return_value=None), \
         patch.object(psutil.Process, "status", return_value=proc_status), \
         patch.object(sys, "exit") as mock_exit:
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector)
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
         patch("dhcp_server.dhcprelayd.dhcprelayd.terminate_proc", return_value=None) as mock_terminate, \
         patch.object(psutil.Process, "__init__", return_value=None), \
         patch.object(psutil.Process, "status", return_value=proc_status):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector)
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
    with patch.object(psutil, "process_iter", return_value=process_iter_ret):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector)
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
         patch.object(sys, "exit") as mock_exit:
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector)
        ret = dhcprelayd._get_dhcp_server_ip()
        if get_res[0] == 1:
            assert ret == get_res[1]
        else:
            mock_exit.assert_called_once_with(1)
            mock_sleep.assert_has_calls([call(10) for _ in range(10)])
