import copy
import ipaddress
import json
import pytest
from common_utils import MockConfigDb
from dhcp_server.common.utils import DhcpDbConnector
from dhcp_server.dhcpservd.dhcp_cfggen import DhcpServCfgGenerator

expected_dhcp_config = {
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
                "subnet": "192.168.0.0/21",
                "pools": [
                    {
                        "pool": "192.168.0.2 - 192.168.0.6",
                        "client-class": "sonic-host:etp8"
                    },
                    {
                        "pool": "192.168.0.10 - 192.168.0.10",
                        "client-class": "sonic-host:etp8"
                    },
                    {
                        "pool": "192.168.0.7 - 192.168.0.7",
                        "client-class": "sonic-host:etp7"
                    }
                ],
                "option-data": [
                    {
                        "name": "routers",
                        "data": "192.168.0.1"
                    },
                    {
                        "name": "dhcp-server-identifier",
                        "data": "192.168.0.1"
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
                "name": "sonic-host:etp8",
                "test": "substring(relay4[1].hex, -15, 15) == 'sonic-host:etp8'"
            },
            {
                "name": "sonic-host:etp7",
                "test": "substring(relay4[1].hex, -15, 15) == 'sonic-host:etp7'"
            }
        ]
    }
}
expected_dhcp_config_without_port_config = {
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
        ]
    }
}
expected_parsed_range = {
    "range2": [ipaddress.IPv4Address("192.168.0.3"), ipaddress.IPv4Address("192.168.0.6")],
    "range3": [ipaddress.IPv4Address("192.168.0.10"), ipaddress.IPv4Address("192.168.0.10")],
    "range1": [ipaddress.IPv4Address("192.168.0.2"), ipaddress.IPv4Address("192.168.0.5")],
    "range0": [ipaddress.IPv4Address("192.168.8.2"), ipaddress.IPv4Address("192.168.8.3")]
}
expected_vlan_ipv4_interface = {
    "Vlan1000": [{
        "ip": "192.168.0.1/21",
        "network": ipaddress.ip_network("192.168.0.1/21", strict=False)
    }],
    "Vlan2000": [
        {
            "ip": "192.168.1.1/21",
            "network": ipaddress.ip_network("192.168.1.1/21", strict=False)
        },
        {
            "ip": "192.168.2.1/21",
            "network": ipaddress.ip_network("192.168.2.1/21", strict=False)
        }
    ]
}
expected_parsed_port = {
    "Vlan1000": {
        "192.168.0.1/21": {
            "etp8": [["192.168.0.2", "192.168.0.6"], ["192.168.0.10", "192.168.0.10"]],
            "etp7": [["192.168.0.7", "192.168.0.7"]]
        }
    }
}
tested_parsed_port = {
    "Vlan1000": {
        "192.168.0.1/21": {
            "etp8": [["192.168.0.2", "192.168.0.6"], ["192.168.0.10", "192.168.0.10"]],
            "etp7": [["192.168.0.7", "192.168.0.7"]],
            "etp9": []
        }
    }
}
expected_render_obj = {
    "subnets": [{
            "subnet": "192.168.0.0/21",
            "pools": [{"range": "192.168.0.2 - 192.168.0.6", "client_class": "sonic-host:etp8"},
                      {"range": "192.168.0.10 - 192.168.0.10", "client_class": "sonic-host:etp8"},
                      {"range": "192.168.0.7 - 192.168.0.7", "client_class": "sonic-host:etp7"}],
            "gateway": "192.168.0.1", "server_id": "192.168.0.1", "lease_time": "900"
    }],
    "client_classes": [
        {"name": "sonic-host:etp8", "condition": "substring(relay4[1].hex, -15, 15) == 'sonic-host:etp8'"},
        {"name": "sonic-host:etp7", "condition": "substring(relay4[1].hex, -15, 15) == 'sonic-host:etp7'"}
    ],
    "lease_update_script_path": "/etc/kea/lease_update.sh",
    "lease_path": "/tmp/kea-lease.csv"
}


def test_parse_port_alias(mock_swsscommon_dbconnector_init, mock_get_render_template):
    dhcp_db_connector = DhcpDbConnector()
    dhcp_cfg_generator = DhcpServCfgGenerator(dhcp_db_connector,
                                              port_map_path="tests/test_data/port-name-alias-map.txt")
    assert dhcp_cfg_generator.port_alias_map == {'Ethernet24': 'etp7', 'Ethernet28': 'etp8'}


@pytest.mark.parametrize("is_success", [True, False])
def test_parse_hostname(is_success, mock_swsscommon_dbconnector_init, mock_parse_port_map_alias,
                        mock_get_render_template):
    mock_config_db = MockConfigDb(config_db_path="tests/test_data/mock_config_db.json")
    dhcp_db_connector = DhcpDbConnector()
    dhcp_cfg_generator = DhcpServCfgGenerator(dhcp_db_connector)
    device_metadata = mock_config_db.config_db.get("DEVICE_METADATA") if is_success else {}
    try:
        hostname = dhcp_cfg_generator._parse_hostname(device_metadata)
        assert hostname == "sonic-host"
    except Exception as err:
        assert str(err) == "Cannot get hostname"


def test_parse_range(mock_swsscommon_dbconnector_init, mock_parse_port_map_alias, mock_get_render_template):
    mock_config_db = MockConfigDb(config_db_path="tests/test_data/mock_config_db.json")
    dhcp_db_connector = DhcpDbConnector()
    dhcp_cfg_generator = DhcpServCfgGenerator(dhcp_db_connector)
    parse_result = dhcp_cfg_generator._parse_range(mock_config_db.config_db.get("DHCP_SERVER_IPV4_RANGE"))
    assert parse_result == expected_parsed_range


def test_parse_vlan(mock_swsscommon_dbconnector_init, mock_parse_port_map_alias, mock_get_render_template):
    mock_config_db = MockConfigDb(config_db_path="tests/test_data/mock_config_db.json")
    dhcp_db_connector = DhcpDbConnector()
    dhcp_cfg_generator = DhcpServCfgGenerator(dhcp_db_connector)
    vlan_interfaces, vlan_members = dhcp_cfg_generator._parse_vlan(mock_config_db.config_db.get("VLAN_INTERFACE"),
                                                                   mock_config_db.config_db.get("VLAN_MEMBER"))
    assert vlan_interfaces == expected_vlan_ipv4_interface
    assert list(vlan_members) == ["Vlan1000|Ethernet24", "Vlan1000|Ethernet28", "Vlan1000|Ethernet40"]


@pytest.mark.parametrize("test_config_db", ["mock_config_db.json", "mock_config_db_without_port_config.json"])
def test_parse_port(test_config_db, mock_swsscommon_dbconnector_init, mock_get_render_template,
                    mock_parse_port_map_alias):
    mock_config_db = MockConfigDb(config_db_path="tests/test_data/{}".format(test_config_db))
    dhcp_db_connector = DhcpDbConnector()
    dhcp_cfg_generator = DhcpServCfgGenerator(dhcp_db_connector)
    tested_vlan_interfaces = expected_vlan_ipv4_interface
    tested_ranges = expected_parsed_range
    ipv4_port = mock_config_db.config_db.get("DHCP_SERVER_IPV4_PORT")
    vlan_members = mock_config_db.config_db.get("VLAN_MEMBER").keys()
    parse_result = dhcp_cfg_generator._parse_port(ipv4_port, tested_vlan_interfaces, vlan_members, tested_ranges)
    assert parse_result == (expected_parsed_port if test_config_db == "mock_config_db.json" else {})


def test_construct_obj_for_template(mock_swsscommon_dbconnector_init, mock_parse_port_map_alias,
                                    mock_get_render_template):
    mock_config_db = MockConfigDb(config_db_path="tests/test_data/mock_config_db.json")
    dhcp_db_connector = DhcpDbConnector()
    dhcp_cfg_generator = DhcpServCfgGenerator(dhcp_db_connector)
    tested_hostname = "sonic-host"
    render_obj = dhcp_cfg_generator._construct_obj_for_template(mock_config_db.config_db.get("DHCP_SERVER_IPV4"),
                                                                tested_parsed_port, tested_hostname)
    assert render_obj == expected_render_obj


@pytest.mark.parametrize("with_port_config", [True, False])
def test_render_config(mock_swsscommon_dbconnector_init, mock_parse_port_map_alias, with_port_config):
    dhcp_db_connector = DhcpDbConnector()
    dhcp_cfg_generator = DhcpServCfgGenerator(dhcp_db_connector,
                                              kea_conf_template_path="tests/test_data/kea-dhcp4.conf.j2")
    render_obj = copy.deepcopy(expected_render_obj)
    if not with_port_config:
        render_obj["client_classes"] = []
        render_obj["subnets"] = []
    config = dhcp_cfg_generator._render_config(render_obj)
    assert json.loads(config) == expected_dhcp_config if with_port_config else expected_dhcp_config_without_port_config
