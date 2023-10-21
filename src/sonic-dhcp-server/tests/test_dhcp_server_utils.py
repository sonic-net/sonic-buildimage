import dhcp_server.dhcp_server_utils as dhcp_server_utils
import ipaddress
import pytest
from swsscommon import swsscommon
from unittest.mock import patch, call

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


def test_construct_without_sock(mock_swsscommon_dbconnector_init):
    dhcp_server_utils.DhcpDbConnector()
    mock_swsscommon_dbconnector_init.assert_has_calls([
        call(swsscommon.CONFIG_DB, "127.0.0.1", 6379, 0),
        call(swsscommon.STATE_DB, "127.0.0.1", 6379, 0)
    ])


def test_construct_sock(mock_swsscommon_dbconnector_init):
    redis_sock = "/var/run/redis/redis.sock"
    dhcp_db_connector = dhcp_server_utils.DhcpDbConnector(redis_sock=redis_sock)
    assert dhcp_db_connector.redis_sock == redis_sock

    mock_swsscommon_dbconnector_init.assert_has_calls([
        call(swsscommon.CONFIG_DB, redis_sock, 0),
        call(swsscommon.STATE_DB, redis_sock, 0)
    ])


def test_get_config_db_table(mock_swsscommon_dbconnector_init, mock_swsscommon_table_init):
    dhcp_db_connector = dhcp_server_utils.DhcpDbConnector()
    with patch.object(swsscommon.Table, "getKeys", return_value=["key1", "key2"]) as mock_get_keys, \
         patch.object(dhcp_server_utils, "get_entry", return_value={"list": "1,2", "value": "3,4"}), \
         patch.object(swsscommon.Table, "hget", side_effect=mock_hget):
        ret = dhcp_db_connector.get_config_db_table("VLAN")
        mock_swsscommon_table_init.assert_called_once_with(dhcp_db_connector.config_db, "VLAN")
        print(ret)
        mock_get_keys.assert_called_once_with()
        print(ret)
        assert ret == {
            "key1": {"list": ["1", "2"], "value": "3,4"},
            "key2": {"list": ["1", "2"], "value": "3,4"}
        }


@pytest.mark.parametrize("test_type", interval_test_data.keys())
def test_merge_intervals(test_type):
    intervals = convert_ip_address_intervals(interval_test_data[test_type]["intervals"])
    expected_res = convert_ip_address_intervals(interval_test_data[test_type]["expected_res"])
    assert dhcp_server_utils.merge_intervals(intervals) == expected_res


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
