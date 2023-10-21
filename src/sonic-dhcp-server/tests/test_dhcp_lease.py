from dhcp_server.dhcp_server_utils import DhcpDbConnector
from dhcp_server.dhcp_lease import KeaDhcp4LeaseHandler, LeaseHanlder
from swsscommon import swsscommon
from unittest.mock import patch, call, MagicMock

expected_lease = {
    "Vlan1000|10:70:fd:b6:13:00": {
        "lease_start": "1693997305",
        "lease_end": "1693997305",
        "ip": "192.168.0.2"
    },
    "Vlan1000|10:70:fd:b6:13:17": {
        "lease_start": "1693997315",
        "lease_end": "1694000915",
        "ip": "192.168.0.131"
    },
    "Vlan2000|10:70:fd:b6:13:15": {
        "lease_start": "1693995705",
        "lease_end": "1693999305",
        "ip": "193.168.2.2"
    }
}
expected_fdb_info = {
    "10:70:fd:b6:13:00": "Vlan1000",
    "10:70:fd:b6:13:15": "Vlan2000",
    "10:70:fd:b6:13:17": "Vlan1000",
}


def test_read_kea_lease_with_file_not_found(mock_swsscommon_dbconnector_init):
    db_connector = DhcpDbConnector()
    kea_lease_handler = KeaDhcp4LeaseHandler(db_connector)
    try:
        kea_lease_handler._read()
    except FileNotFoundError:
        pass


def test_read_kea_lease(mock_swsscommon_dbconnector_init):
    tested_fdb_info = expected_fdb_info
    with patch.object(KeaDhcp4LeaseHandler, "_get_fdb_info", return_value=tested_fdb_info):
        db_connector = DhcpDbConnector()
        kea_lease_handler = KeaDhcp4LeaseHandler(db_connector, lease_file="tests/test_data/kea-lease.csv")
        # Verify whether lease information read is as expected
        lease = kea_lease_handler._read()
        print(lease)
        print(expected_lease)
        assert lease == expected_lease


def test_get_fdb_info(mock_swsscommon_dbconnector_init):
    mock_fdb_table = {
        "Vlan2000:10:70:fd:b6:13:15": {"port": "Ethernet31", "type": "dynamic"},
        "Vlan1000:10:70:fd:b6:13:00": {"port": "Ethernet32", "type": "dynamic"},
        "Vlan1000:10:70:fd:b6:13:17": {"port": "Ethernet32", "type": "dynamic"}
    }
    with patch("dhcp_server.dhcp_server_utils.DhcpDbConnector.get_state_db_table", return_value=mock_fdb_table):
        db_connector = DhcpDbConnector()
        kea_lease_handler = KeaDhcp4LeaseHandler(db_connector, lease_file="tests/test_data/kea-lease.csv")
        # Verify whether lease information read is as expected
        fdb_info = kea_lease_handler._get_fdb_info()
        assert fdb_info == expected_fdb_info


def test_update_kea_lease(mock_swsscommon_dbconnector_init, mock_swsscommon_table_init):
    tested_lease = expected_lease
    with patch.object(swsscommon.Table, "getKeys"), \
         patch.object(swsscommon.DBConnector, "hset") as mock_hset, \
         patch.object(KeaDhcp4LeaseHandler, "_read", MagicMock(return_value=tested_lease)), \
         patch.object(DhcpDbConnector, "get_state_db_table",
                      return_value={"Vlan1000|aa:bb:cc:dd:ee:ff": {}, "Vlan1000|10:70:fd:b6:13:00": {}}), \
         patch.object(swsscommon.DBConnector, "delete") as mock_delete, \
         patch("time.sleep", return_value=None) as mock_sleep:
        db_connector = DhcpDbConnector()
        kea_lease_handler = KeaDhcp4LeaseHandler(db_connector)
        kea_lease_handler.update_lease()
        # Verify that old key was deleted
        mock_delete.assert_has_calls([
            call("DHCP_SERVER_IPV4_LEASE|Vlan1000|10:70:fd:b6:13:00"),
            call("DHCP_SERVER_IPV4_LEASE|Vlan1000|aa:bb:cc:dd:ee:ff")
        ])
        # Verify that lease has been updated, to be noted that lease for "192.168.0.2" didn't been updated because
        # lease_start equals to lease_end
        mock_hset.assert_has_calls([
            call('DHCP_SERVER_IPV4_LEASE|Vlan1000|10:70:fd:b6:13:17', 'lease_start', '1693997315'),
            call('DHCP_SERVER_IPV4_LEASE|Vlan1000|10:70:fd:b6:13:17', 'lease_end', '1694000915'),
            call('DHCP_SERVER_IPV4_LEASE|Vlan1000|10:70:fd:b6:13:17', 'ip', '192.168.0.131')
        ])
        kea_lease_handler.update_lease()
        mock_sleep.assert_called_once_with(2)


def test_no_implement(mock_swsscommon_dbconnector_init):
    db_connector = DhcpDbConnector()
    lease_handler = LeaseHanlder(db_connector)
    try:
        lease_handler._read()
    except NotImplementedError:
        pass
    try:
        lease_handler.register()
    except NotImplementedError:
        pass
