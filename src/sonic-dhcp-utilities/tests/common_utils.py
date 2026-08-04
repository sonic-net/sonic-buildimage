import heapq
import json
import psutil
from dhcp_utilities.common.dhcp_db_monitor import DhcpRelaydDbMonitor
from dhcp_utilities.common.utils import DhcpDbConnector
from dhcp_utilities.dhcprelayd.dhcprelayd import DhcpRelayd, FEATURE_CHECKER, DHCP_SERVER_CHECKER, VLAN_INTF_CHECKER
from unittest.mock import patch, PropertyMock, call

MOCK_CONFIG_DB_PATH = "tests/test_data/mock_config_db.json"
TEST_DATA_PATH = "tests/test_data/dhcp_db_monitor_test_data.json"
DHCP_SERVER_IPV4 = "DHCP_SERVER_IPV4"
DHCP_SERVER_IPV4_CUSTOMIZED_OPTIONS = "DHCP_SERVER_IPV4_CUSTOMIZED_OPTIONS"
DHCP_SERVER_IPV4_RANGE = "DHCP_SERVER_IPV4_RANGE"
DHCP_SERVER_IPV4_PORT = "DHCP_SERVER_IPV4_PORT"
VLAN = "VLAN"
VLAN_INTERFACE = "VLAN_INTERFACE"
VLAN_MEMBER = "VLAN_MEMBER"
PORT_MODE_CHECKER = ["DhcpServerTableCfgChangeEventChecker", "DhcpPortTableEventChecker", "DhcpRangeTableEventChecker",
                     "DhcpOptionTableEventChecker", "VlanTableEventChecker", "VlanIntfTableEventChecker",
                     "VlanMemberTableEventChecker"]
SMART_SWITCH_CHECKER = ["DpusTableEventChecker", "MidPlaneTableEventChecker"]


class MockConfigDb(object):
    def __init__(self, config_db_path=MOCK_CONFIG_DB_PATH):
        with open(config_db_path, "r", encoding="utf8") as file:
            self.config_db = json.load(file)

    def get_config_db_table(self, table_name):
        return self.config_db.get(table_name, {})


class MockSelect(object):
    def __init__(self):
        pass

    def select(self, timeout):
        return None, None


class MockSubscribeTable(object):
    def __init__(self, tables):
        """
        Args:
            tables: table update event, sample: [
                ("Vlan1000", "SET", (("state", "enabled"),)),
                ("Vlan1000", "SET", (("customized_options", "option1"), ("state", "enabled"),))
            ]
        """
        self.stack = []
        for item in tables:
            heapq.heappush(self.stack, item)

    def pop(self):
        res = heapq.heappop(self.stack)
        return res

    def hasData(self):
        return len(self.stack) != 0


def mock_get_config_db_table(table_name):
    mock_config_db = MockConfigDb()
    return mock_config_db.get_config_db_table(table_name)


class MockProc(object):
    def __init__(self, name, pid=1, exited=False, ppid=1):
        self.proc_name = name
        self.pid = pid
        self.exited = exited
        self.parent_id = ppid

    def name(self):
        if self.exited:
            raise psutil.NoSuchProcess(self.pid)
        return self.proc_name

    def send_signal(self, sig_num):
        pass

    def cmdline(self):
        if self.exited:
            raise psutil.NoSuchProcess(self.pid)
        if self.proc_name == "dhcrelay":
            return ["/usr/sbin/dhcrelay", "-d", "-m", "discard", "-a", "%h:%p", "%P", "--name-alias-map-file",
                    "/tmp/port-name-alias-map.txt", "-id", "Vlan1000", "-iu", "docker0", "240.127.1.2"]
        if self.proc_name == "dhcpmon":
            return ["/usr/sbin/dhcpmon", "-id", "Vlan1000", "-iu", "docker0", "-im", "eth0"]

    def terminate(self):
        pass

    def wait(self):
        pass

    def status(self):
        return self.status

    def ppid(self):
        return self.parent_id


class MockPopen(object):
    def __init__(self, pid):
        self.pid = pid


def mock_exit_func(status):
    raise SystemExit(status)


def get_subscribe_table_tested_data(test_name):
    test_obj = {}
    with open(TEST_DATA_PATH, "r") as file:
        test_obj = json.loads(file.read())
    tested_data = test_obj[test_name]
    for data in tested_data:
        for i in range(len(data["table"])):
            for j in range(len(data["table"][i][2])):
                data["table"][i][2][j] = tuple(data["table"][i][2][j])
            data["table"][i][2] = tuple(data["table"][i][2])
            data["table"][i] = tuple(data["table"][i])
    return tested_data


class MockSubprocessRes(object):
    def __init__(self, returncode):
        self.returncode = returncode


def dhcprelayd_refresh_dhcrelay_test(expected_checkers, is_smart_switch, mock_get_config_db_table):
    with patch.object(DhcpRelayd, "_get_dhcp_server_ip", return_value="240.127.1.2"), \
         patch.object(DhcpDbConnector, "get_config_db_table", side_effect=mock_get_config_db_table), \
         patch.object(DhcpRelayd, "_start_dhcrelay_process", return_value=None), \
         patch.object(DhcpRelayd, "_start_dhcpmon_process", return_value=None), \
         patch.object(DhcpRelayd, "_enable_checkers") as mock_enable_checkers, \
         patch.object(DhcpRelayd, "_disable_checkers") as mock_disable_checkers, \
         patch.object(DhcpRelayd, "smart_switch", return_value=is_smart_switch, new_callable=PropertyMock):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, None)
        dhcprelayd.refresh_dhcrelay()
        mock_enable_checkers.assert_called_once_with(expected_checkers)
        mock_disable_checkers.assert_called_once_with(set())


def dhcprelayd_proceed_with_check_res_test(enabled_checkers, feature_enabled, feature_res, dhcp_server_res,
                                           vlan_intf_res, is_smart_switch, expected_checkers):
    with patch.object(DhcpRelayd, "_enable_checkers") as mock_enable_checkers, \
         patch.object(DhcpRelayd, "_execute_supervisor_dhcp_relay_process") as mock_execute_process, \
         patch.object(DhcpRelayd, "refresh_dhcrelay") as mock_refresh_dhcrelay, \
         patch.object(DhcpRelayd, "_disable_checkers") as mock_disable_checkers, \
         patch.object(DhcpRelayd, "_kill_exist_relay_releated_process") as mock_kill_process, \
         patch.object(DhcpRelayd, "_check_dhcp_relay_processes") as mock_check_process, \
         patch.object(DhcpRelayd, "smart_switch", return_value=is_smart_switch,
                      new_callable=PropertyMock), \
         patch.object(DhcpRelayd, "enabled_checkers", return_value=enabled_checkers, new_callable=PropertyMock):
        dhcp_db_connector = DhcpDbConnector()
        dhcprelayd = DhcpRelayd(dhcp_db_connector, DhcpRelaydDbMonitor(None, None, []))
        dhcprelayd.dhcp_server_feature_enabled = True if feature_enabled else False
        check_res = {}
        if feature_res is not None:
            check_res[FEATURE_CHECKER] = feature_res
        if dhcp_server_res is not None:
            check_res[DHCP_SERVER_CHECKER] = dhcp_server_res
        if vlan_intf_res is not None:
            check_res[VLAN_INTF_CHECKER] = vlan_intf_res
        dhcprelayd._proceed_with_check_res(check_res, feature_enabled)
        if feature_res is None or not feature_res:
            # Feature status didn't change

            # disabled -> disabled
            if not feature_enabled:
                mock_check_process.assert_called_once_with()
            # enabled-> enabled
            else:
                if vlan_intf_res:
                    mock_refresh_dhcrelay.assert_called_once_with(True)
                elif dhcp_server_res:
                    mock_refresh_dhcrelay.assert_called_once_with(False)
                else:
                    mock_refresh_dhcrelay.assert_not_called()
        else:
            # Feature status changed

            # enabled -> disabled
            if feature_enabled:
                mock_disable_checkers.assert_called_once_with(expected_checkers)
                mock_kill_process.assert_has_calls([
                    call([], "dhcpmon", True),
                    call([], "dhcrelay", True)
                ])
            # disabled-> enabled
            else:
                mock_enable_checkers.assert_called_once_with([DHCP_SERVER_CHECKER])
                mock_execute_process.assert_called_once_with("stop")
                mock_refresh_dhcrelay.assert_called_once_with()
