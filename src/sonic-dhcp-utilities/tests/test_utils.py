import dhcp_utilities.common.utils as utils
import ipaddress
import psutil
import pytest
from swsscommon import swsscommon
from common_utils import MockProc
from unittest.mock import patch, call, PropertyMock

interval_test_data = {
    "ordered_with_overlap": {
        "intervals": [["192.168.0.2", "192.168.0.5"], ["192.168.0.3", "192.168.0.6"], ["192.168.0.10", "192.168.0.10"]],
        "expected_res": [["192.168.0.2", "192.168.0.6"], ["192.168.0.10", "192.168.0.10"]]
    },
    "not_order_with_overlap": {
        "intervals": [["192.168.0.3", "192.168.0.6"], ["192.168.0.2", "192.168.0.5"], ["192.168.0.10", "192.168.0.10"]],
        "expected_res": [["192.168.0.2", "192.168.0.6"], ["192.168.0.10", "192.168.0.10"]]
    },
    "ordered_without_overlap": {
        "intervals": [["192.168.0.2", "192.168.0.5"], ["192.168.0.10", "192.168.0.10"]],
        "expected_res": [["192.168.0.2", "192.168.0.5"], ["192.168.0.10", "192.168.0.10"]]
    },
    "not_ordered_without_overlap": {
        "intervals": [["192.168.0.10", "192.168.0.10"], ["192.168.0.2", "192.168.0.5"]],
        "expected_res": [["192.168.0.2", "192.168.0.5"], ["192.168.0.10", "192.168.0.10"]]
    },
    "single_interval": {
        "intervals": [["192.168.0.10", "192.168.0.10"]],
        "expected_res": [["192.168.0.10", "192.168.0.10"]]
    }
}
validate_str_type_data = [
    # type, value, expected_res
    ["string", 123, False],
    ["string", "123", True],
    ["binary", "01020304ef", True],
    # False because we only support octet-based binary
    ["binary", "01020304e", False],
    ["binary", "0102ab0304ef", True],
    ["binary", "we", False],
    ["boolean", "true", True],
    ["boolean", "false", True],
    ["boolean", "0", False],
    ["boolean", "1", False],
    ["boolean", True, False],
    ["boolean", "213", False],
    ["ipv4-address", "192.168.0.1", True],
    ["ipv4-address", "300.168.0.1", False],
    ["ipv4-address", 123, False],
    ["ipv4-address", "123", False],
    ["ipv4-address", "192.168.0.1/24", False],
    ["uint8", "e123", False],
    ["uint8", 123, False],
    ["uint8", "300", False],
    ["uint8", "128", True],
    ["uint16", "1000", True],
    ["uint16", "65536", False],
    ["uint32", "4294967296", False],
    ["uint32", "65536", True],
    # False because we don't support uint64
    ["uint64", "65536", False]
]


def test_construct_without_sock(mock_swsscommon_dbconnector_init):
    utils.DhcpDbConnector()
    mock_swsscommon_dbconnector_init.assert_has_calls([
        call(swsscommon.CONFIG_DB, "127.0.0.1", 6379, 0),
        call(swsscommon.STATE_DB, "127.0.0.1", 6379, 0)
    ])


def test_construct_sock(mock_swsscommon_dbconnector_init):
    redis_sock = "/var/run/redis/redis.sock"
    dhcp_db_connector = utils.DhcpDbConnector(redis_sock=redis_sock)
    assert dhcp_db_connector.redis_sock == redis_sock

    mock_swsscommon_dbconnector_init.assert_has_calls([
        call(swsscommon.CONFIG_DB, redis_sock, 0),
        call(swsscommon.STATE_DB, redis_sock, 0)
    ])


def test_get_db_table(mock_swsscommon_dbconnector_init, mock_swsscommon_table_init):
    dhcp_db_connector = utils.DhcpDbConnector()
    with patch.object(swsscommon.Table, "getKeys", return_value=["key1", "key2"]) as mock_get_keys, \
         patch.object(utils, "get_entry", return_value={"list": "1,2", "value": "3,4"}), \
         patch.object(swsscommon.Table, "hget", side_effect=mock_hget):
        ret = dhcp_db_connector.get_config_db_table("VLAN")
        assert ret == {
            "key1": {"list": ["1", "2"], "value": "3,4"},
            "key2": {"list": ["1", "2"], "value": "3,4"}
        }
        ret = dhcp_db_connector.get_state_db_table("VLAN")
        mock_swsscommon_table_init.assert_has_calls([
            call(dhcp_db_connector.config_db, "VLAN"),
            call(dhcp_db_connector.state_db, "VLAN")
        ])
        mock_get_keys.assert_has_calls([
            call(),
            call()
        ])
        assert ret == {
            "key1": {"list": ["1", "2"], "value": "3,4"},
            "key2": {"list": ["1", "2"], "value": "3,4"}
        }


def test_get_entry(mock_swsscommon_dbconnector_init, mock_swsscommon_table_init):
    tested_entry = {"key": "value"}
    dhcp_db_connector = utils.DhcpDbConnector()
    with patch.object(swsscommon.Table, "get", return_value=(None, tested_entry)) as mock_get:
        res = utils.get_entry(swsscommon.Table(dhcp_db_connector.config_db, "VLAN"), "dummy_entry")
        assert res == tested_entry
        mock_get.assert_called_once_with("dummy_entry")


@pytest.mark.parametrize("test_type", interval_test_data.keys())
def test_merge_intervals(test_type):
    intervals = convert_ip_address_intervals(interval_test_data[test_type]["intervals"])
    expected_res = convert_ip_address_intervals(interval_test_data[test_type]["expected_res"])
    assert utils.merge_intervals(intervals) == expected_res


def mock_hget(_, field):
    if field == "list":
        return False, ""
    else:
        return True, ""


def convert_ip_address_intervals(intervals):
    ret = []
    for interval in intervals:
        ret.append([ipaddress.ip_address(interval[0]), ipaddress.ip_address(interval[1])])
    return ret


@pytest.mark.parametrize("test_data", validate_str_type_data)
def test_validate_ttr_type(test_data):
    res = utils.validate_str_type(test_data[0], test_data[1])
    assert res == test_data[2]


@pytest.mark.parametrize("is_smart_switch", [True, False])
def test_is_smart_switch(is_smart_switch):
    device_metadata = {"localhost": {"subtype": "SmartSwitch"}} if is_smart_switch else {"localhost": {}}
    assert utils.is_smart_switch(device_metadata) == is_smart_switch
