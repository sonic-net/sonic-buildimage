import json
import pytest
from common_utils import MockConfigDb, dhcprelayd_refresh_dhcrelay_test, dhcprelayd_proceed_with_check_res_test
from dhcp_utilities.dhcpservd.dhcp_lease import KeaDhcp4LeaseHandler
from dhcp_utilities.dhcprelayd.dhcprelayd import DHCP_SERVER_CHECKER, MID_PLANE_CHECKER
from dhcp_utilities.dhcpservd.dhcp_cfggen import DhcpServCfgGenerator
from dhcp_utilities.common.utils import DhcpDbConnector
from unittest.mock import patch

MOCK_CONFIG_DB_PATH_SMART_SWITCH = "tests/test_data/mock_config_db_smart_switch.json"
expected_kea_config = {
    "Dhcp4": {
        "hooks-libraries": [
            {
                "library": "/usr/local/lib/kea/hooks/libdhcp_run_script.so",
                "parameters": {
                    "name": "/etc/kea/lease_update.sh",
                    "sync": False
                }
            }
        ],
        "interfaces-config": {
            "interfaces": [
                "eth0"
            ]
        },
        "control-socket": {
            "socket-type": "unix",
            "socket-name": "/run/kea/kea4-ctrl-socket"
        },
        "lease-database": {
            "type": "memfile",
            "persist": True,
            "name": "/tmp/kea-lease.csv",
            "lfc-interval": 3600
        },
        "subnet4": [
            {
                "id": 10000,
                "subnet": "169.254.200.0/24",
                "pools": [
                    {
                        "pool": "169.254.200.1 - 169.254.200.1",
                        "client-class": "sonic-host:dpu0"
                    },
                    {
                        "pool": "169.254.200.2 - 169.254.200.2",
                        "client-class": "sonic-host:dpu1"
                    },
                    {
                        "pool": "169.254.200.3 - 169.254.200.3",
                        "client-class": "sonic-host:dpu2"
                    },
                    {
                        "pool": "169.254.200.4 - 169.254.200.4",
                        "client-class": "sonic-host:dpu3"
                    }
                ],
                "option-data": [
                    {
                        "name": "routers",
                        "data": "169.254.200.254"
                    },
                    {
                        "name": "dhcp-server-identifier",
                        "data": "169.254.200.254",
                        "always-send": True
                    }
                ],
                "valid-lifetime": 900,
                "reservations": []
            }
        ],
        "loggers": [
            {
                "name": "kea-dhcp4",
                "output_options": [
                    {
                        "output": "/var/log/kea-dhcp.log",
                        "pattern": "%-5p %m\n"
                    }
                ],
                "severity": "INFO",
                "debuglevel": 0
            }
        ],
        "client-classes": [
            {
                "name": "sonic-host:dpu0",
                "test": "substring(relay4[1].hex, -15, 15) == 'sonic-host:dpu0'"
            },
            {
                "name": "sonic-host:dpu1",
                "test": "substring(relay4[1].hex, -15, 15) == 'sonic-host:dpu1'"
            },
            {
                "name": "sonic-host:dpu2",
                "test": "substring(relay4[1].hex, -15, 15) == 'sonic-host:dpu2'"
            },
            {
                "name": "sonic-host:dpu3",
                "test": "substring(relay4[1].hex, -15, 15) == 'sonic-host:dpu3'"
            }
        ]
    }
}


expected_lease = {
    'bridge-midplane|aa:bb:cc:dd:ff:01': {
        'ip': '169.254.200.1',
        'lease_end': '1718053209',
        'lease_start': '1718052309'
    },
    'bridge-midplane|aa:bb:cc:dd:ff:02': {
        'ip': '169.254.200.2',
        'lease_end': '1718053210',
        'lease_start': '1718052310'
    },
    'bridge-midplane|aa:bb:cc:dd:ff:03': {
        'ip': '169.254.200.3',
        'lease_end': '1718053210',
        'lease_start': '1718052310'
    },
   'bridge-midplane|aa:bb:cc:dd:ff:04': {
       'ip': '169.254.200.4',
       'lease_end': '1718053209',
       'lease_start': '1718052309'
    }
}


def test_dhcprelayd_refresh_dhcrelay(mock_swsscommon_dbconnector_init):
    expected_checkers = set(["MidPlaneTableEventChecker"])
    dhcprelayd_refresh_dhcrelay_test(expected_checkers, True, mock_get_config_db_table)


@pytest.mark.parametrize("feature_enabled", [True, False])
@pytest.mark.parametrize("feature_res", [True, False, None])
@pytest.mark.parametrize("dhcp_server_res", [True, False, None])
def test_dhcprelayd_proceed_with_check_res(mock_swsscommon_dbconnector_init, mock_swsscommon_table_init,
                                           feature_enabled, feature_res, dhcp_server_res):
    enabled_checkers = set([DHCP_SERVER_CHECKER, MID_PLANE_CHECKER])
    expected_checkers = set([DHCP_SERVER_CHECKER, MID_PLANE_CHECKER])
    dhcprelayd_proceed_with_check_res_test(enabled_checkers, feature_enabled, feature_res, dhcp_server_res,
                                           None, True, expected_checkers)


def test_dhcp_dhcp_cfggen_generate(mock_swsscommon_dbconnector_init, mock_parse_port_map_alias):
    with patch.object(DhcpDbConnector, "get_config_db_table", side_effect=mock_get_config_db_table):
        dhcp_db_connector = DhcpDbConnector()
        dhcp_cfg_generator = DhcpServCfgGenerator(dhcp_db_connector, "/usr/local/lib/kea/hooks/libdhcp_run_script.so",
                                                  kea_conf_template_path="tests/test_data/kea-dhcp4.conf.j2")
        kea_dhcp4_config, used_ranges, enabled_dhcp_interfaces, used_options, subscribe_table = \
            dhcp_cfg_generator.generate()
        print(json.loads(kea_dhcp4_config))
        print(expected_kea_config)
        assert json.loads(kea_dhcp4_config) == expected_kea_config
        assert used_ranges == set()
        assert enabled_dhcp_interfaces == set(["bridge-midplane"])
        assert used_options == set(["option60", "option223"])
        expected_tables = set(["DpusTableEventChecker", "MidPlaneTableEventChecker", "VlanTableEventChecker",
                               "VlanIntfTableEventChecker", "DhcpRangeTableEventChecker", "VlanMemberTableEventChecker",
                               "DhcpOptionTableEventChecker", "DhcpPortTableEventChecker",
                               "DhcpServerTableCfgChangeEventChecker"])
        assert subscribe_table == expected_tables


def mock_get_config_db_table(table_name):
    mock_config_db = MockConfigDb(MOCK_CONFIG_DB_PATH_SMART_SWITCH)
    return mock_config_db.get_config_db_table(table_name)


def test_read_kea_lease(mock_swsscommon_dbconnector_init):
    with patch.object(DhcpDbConnector, "get_config_db_table", side_effect=mock_get_config_db_table):
        db_connector = DhcpDbConnector()
        kea_lease_handler = KeaDhcp4LeaseHandler(db_connector, lease_file="tests/test_data/kea-lease_smart_switch.csv")
        # Verify whether lease information read is as expected
        lease = kea_lease_handler._read()
        assert lease == expected_lease
