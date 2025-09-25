import copy
import ipaddress
import json
import pytest
from common_utils import MockConfigDb, mock_get_config_db_table, PORT_MODE_CHECKER
from dhcp_utilities.common.utils import DhcpDbConnector
from dhcp_utilities.dhcpservd.dhcp_cfggen import DhcpServCfgGenerator
from unittest.mock import patch

expected_dhcp_config = {
    "Dhcp4": {
        "option-def": [
            {
                "name": "option223",
                "code": 223,
                "type": "string"
            }
        ],
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
                "id": 1000,
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
                        "data": "10.0.0.1",
                        "always-send": False
                    }
                ],
                "valid-lifetime": 900,
                "reservations": []
            },
            {
                "id": 4000,
                "subnet": "192.168.3.0/24",
                "pools": [
                    {
                        "pool": "192.168.3.2 - 192.168.3.3",
                        "client-class": "sonic-host:etp11"
                    }
                ],
                "option-data": [
                    {
                        "name": "dhcp-server-identifier",
                        "data": "192.168.3.1",
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
                "name": "sonic-host:etp8",
                "test": "substring(relay4[1].hex, -15, 15) == 'sonic-host:etp8'"
            },
            {
                "name": "sonic-host:etp7",
                "test": "substring(relay4[1].hex, -15, 15) == 'sonic-host:etp7'"
            },
            {
                "name": "sonic-host:etp11",
                "test": "substring(relay4[1].hex, -16, 16) == 'sonic-host:etp11'"
            }
        ]
    }
}
expected_dhcp_config_without_port_config = {
    "Dhcp4": {
        "option-def": [
            {
                "name": "option223",
                "code": 223,
                "type": "string"
            }
        ],
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
    "range4": [ipaddress.IPv4Address("192.168.0.1"), ipaddress.IPv4Address("192.168.0.1")],
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
            "etp8": [["192.168.0.2", "192.168.0.5"], ["192.168.0.10", "192.168.0.10"]],
            "etp7": [["192.168.0.7", "192.168.0.7"]],
            "Ethernet40": [["192.168.0.10", "192.168.0.10"]]
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
    "subnets": [
        {
            "subnet": "192.168.0.0/21", 'id': '1000',
            "pools": [{"range": "192.168.0.2 - 192.168.0.6", "client_class": "sonic-host:etp8"},
                      {"range": "192.168.0.10 - 192.168.0.10", "client_class": "sonic-host:etp8"},
                      {"range": "192.168.0.7 - 192.168.0.7", "client_class": "sonic-host:etp7"}],
            "gateway": "192.168.0.1", "lease_time": "900",
            "customized_options": {
                "option223": {
                    "always_send": "true",
                    "value": "dummy_value",
                    "option_type": "customized",
                    "id": "223"
                },
                "option60": {
                    "always_send": "false",
                    "value": "dummy_value",
                    "option_type": "standard",
                    "id": "60"
                }
            },
            "dhcp_server_id_option": {
                "always_send": "false",
                "value": "10.0.0.1",
                "option_type": "standard",
                "id": "54"
            }
        },
        {
            "subnet": "192.168.3.0/24", 'id': '4000',
            "pools": [{"range": "192.168.3.2 - 192.168.3.3", "client_class": "sonic-host:etp11"}],
            "lease_time": "900",
            "customized_options": {
                "option223": {
                    "always_send": "true",
                    "value": "dummy_value",
                    "option_type": "customized",
                    "id": "223"
                },
                "option60": {
                    "always_send": "false",
                    "value": "dummy_value",
                    "option_type": "standard",
                    "id": "60"
                }
            },
            "dhcp_server_id_option": {
                "value": "192.168.3.1",
                "always_send": "true"
            }
        }
    ],
    "client_classes": [
        {"name": "sonic-host:etp8", "condition": "substring(relay4[1].hex, -15, 15) == 'sonic-host:etp8'"},
        {"name": "sonic-host:etp7", "condition": "substring(relay4[1].hex, -15, 15) == 'sonic-host:etp7'"},
        {"name": "sonic-host:etp11", "condition": "substring(relay4[1].hex, -16, 16) == 'sonic-host:etp11'"}
    ],
    "lease_update_script_path": "/etc/kea/lease_update.sh",
    "lease_path": "/tmp/kea-lease.csv",
    "customized_options": {
        "option223": {
            "id": "223",
            "value": "dummy_value",
            "type": "string",
            "always_send": "true",
            "option_type": "customized"
        }
    },
    "hook_lib_path": "/usr/local/lib/kea/hooks/libdhcp_run_script.so"
}
tested_options_data = {
    "data": {
        "option83": {
            "id": "83",
            "type": "string",
            "value": "dummy_value"
        },
        "option223": {
            "id": "223",
            "type": "string",
            "value": "dummy_value"
        },
        "option32": {
            "id": "32",
            "type": "string",
            "value": "192.168.0.1"
        },
        "option60": {
            "id": "60",
            "type": "string",
            "value": "dummy_value"
        },
        "option222": {
            "id": "222",
            "type": "text",
            "value": "dummy_value"
        },
        "option219": {
            "id": "219",
            "type": "uint8",
            "value": "259"
        },
        "option218": {
            "id": "218",
            "type": "string",
            "value": "long_valuelong_valuelong_valuelong_valuelong_valuelong_valuelong_valuelong_valuelong_value" +
                     "long_valuelong_valuelong_valuelong_valuelong_valuelong_valuelong_valuelong_valuelong_value" +
                     "long_valuelong_valuelong_valuelong_valuelong_valuelong_valuelong_valuelong_valuelong_value" +
                     "long_valuelong_valuelong_valuelong_valuelong_value"
        },
        "option217": {
            "id": "217",
            "type": "string",
            "value": "dummy_value,dummy_value"
        },
        "option216": {
            "id": "216",
            "type": "uint8",
            "value": "8"
        }
    },
    "res": {
        "option223": {"value": "dummy_value", "option_type": "customized"},
        "option217": {"value": "dummy_value\\\\,dummy_value", "option_type": "customized"},
        "option216": {"value": "8", "option_type": "customized"},
        "option60": {"value": "dummy_value", "option_type": "standard"},
        "option32": {"value": "192.168.0.1", "type": "ipv4-address", "option_type": "standard"},
    }
}


def test_parse_port_alias(mock_swsscommon_dbconnector_init, mock_get_render_template):
    with patch.object(DhcpDbConnector, "get_config_db_table", side_effect=mock_get_config_db_table):
        dhcp_db_connector = DhcpDbConnector()
        dhcp_cfg_generator = DhcpServCfgGenerator(dhcp_db_connector, "/usr/local/lib/kea/hooks/libdhcp_run_script.so")
        assert dhcp_cfg_generator.port_alias_map == {"Ethernet0": "etp1", "Ethernet1": "etp2",
                                                     "PortChannel101": "PortChannel101"}


@pytest.mark.parametrize("is_success", [True, False])
def test_parse_hostname(is_success, mock_swsscommon_dbconnector_init, mock_parse_port_map_alias,
                        mock_get_render_template):
    mock_config_db = MockConfigDb(config_db_path="tests/test_data/mock_config_db.json")
    dhcp_db_connector = DhcpDbConnector()
    dhcp_cfg_generator = DhcpServCfgGenerator(dhcp_db_connector, "/usr/local/lib/kea/hooks/libdhcp_run_script.so")
    device_metadata = mock_config_db.config_db.get("DEVICE_METADATA") if is_success else {}
    try:
        hostname = dhcp_cfg_generator._parse_hostname(device_metadata)
        assert hostname == "sonic-host"
    except Exception as err:
        assert str(err) == "Cannot get hostname"


def test_parse_range(mock_swsscommon_dbconnector_init, mock_parse_port_map_alias, mock_get_render_template):
    mock_config_db = MockConfigDb(config_db_path="tests/test_data/mock_config_db.json")
    dhcp_db_connector = DhcpDbConnector()
    dhcp_cfg_generator = DhcpServCfgGenerator(dhcp_db_connector, "/usr/local/lib/kea/hooks/libdhcp_run_script.so")
    parse_result = dhcp_cfg_generator._parse_range(mock_config_db.config_db.get("DHCP_SERVER_IPV4_RANGE"))
    assert parse_result == expected_parsed_range


def test_parse_vlan(mock_swsscommon_dbconnector_init, mock_parse_port_map_alias, mock_get_render_template):
    mock_config_db = MockConfigDb(config_db_path="tests/test_data/mock_config_db.json")
    dhcp_db_connector = DhcpDbConnector()
    dhcp_cfg_generator = DhcpServCfgGenerator(dhcp_db_connector, "/usr/local/lib/kea/hooks/libdhcp_run_script.so")
    vlan_interfaces, vlan_members = dhcp_cfg_generator._parse_vlan(mock_config_db.config_db.get("VLAN_INTERFACE"),
                                                                   mock_config_db.config_db.get("VLAN_MEMBER"))
    assert vlan_interfaces == expected_vlan_ipv4_interface
    assert vlan_members == set(["Vlan1000|Ethernet24", "Vlan1000|Ethernet28", "Vlan1000|Ethernet40",
                                "Vlan3000|Ethernet44"])


@pytest.mark.parametrize("test_config_db", ["mock_config_db.json", "mock_config_db_without_port_config.json"])
def test_parse_port(test_config_db, mock_swsscommon_dbconnector_init, mock_get_render_template,
                    mock_parse_port_map_alias):
    mock_config_db = MockConfigDb(config_db_path="tests/test_data/{}".format(test_config_db))
    dhcp_db_connector = DhcpDbConnector()
    dhcp_cfg_generator = DhcpServCfgGenerator(dhcp_db_connector, "/usr/local/lib/kea/hooks/libdhcp_run_script.so")
    tested_vlan_interfaces = expected_vlan_ipv4_interface
    tested_ranges = expected_parsed_range
    ipv4_port = mock_config_db.config_db.get("DHCP_SERVER_IPV4_PORT")
    vlan_members = mock_config_db.config_db.get("VLAN_MEMBER").keys()
    parsed_port, used_ranges = dhcp_cfg_generator._parse_port(ipv4_port, tested_vlan_interfaces, vlan_members,
                                                              tested_ranges)
    assert parsed_port == (expected_parsed_port if test_config_db == "mock_config_db.json" else {})
    assert used_ranges == ({"range1", "range0", "range3", "range2", "range6"}
                           if test_config_db == "mock_config_db.json" else set())


def test_generate(mock_swsscommon_dbconnector_init, mock_parse_port_map_alias, mock_get_render_template):
    with patch.object(DhcpServCfgGenerator, "_parse_hostname"), \
         patch.object(DhcpServCfgGenerator, "_parse_vlan", return_value=({}, set(["Ethernet0"]))), \
         patch.object(DhcpServCfgGenerator, "_get_dhcp_ipv4_tables_from_db", return_value=(None, None, None, None)), \
         patch.object(DhcpServCfgGenerator, "_parse_range"), \
         patch.object(DhcpServCfgGenerator, "_parse_port", return_value=(None, set(["range1"]))), \
         patch.object(DhcpServCfgGenerator, "_parse_customized_options"), \
         patch.object(DhcpServCfgGenerator, "_parse_dpu", return_value=(set(), set())), \
         patch.object(DhcpServCfgGenerator, "_construct_obj_for_template",
                      return_value=(None, set(["Vlan1000"]), set(["option1"]), set(["dummy"]))), \
         patch.object(DhcpServCfgGenerator, "_render_config", return_value="dummy_config"), \
         patch.object(DhcpDbConnector, "get_config_db_table", side_effect=mock_get_config_db_table), \
         patch("dhcp_utilities.dhcpservd.dhcp_cfggen.is_smart_switch", return_value=False):
        dhcp_db_connector = DhcpDbConnector()
        dhcp_cfg_generator = DhcpServCfgGenerator(dhcp_db_connector, "/usr/local/lib/kea/hooks/libdhcp_run_script.so")
        kea_dhcp4_config, used_ranges, enabled_dhcp_interfaces, used_options, subscribe_table = \
            dhcp_cfg_generator.generate()
        assert kea_dhcp4_config == "dummy_config"
        assert used_ranges == set(["range1"])
        assert enabled_dhcp_interfaces == set(["Vlan1000"])
        assert used_options == set(["option1"])
        expected_tables = set(["dummy"])
        assert subscribe_table == expected_tables


def test_construct_obj_for_template(mock_swsscommon_dbconnector_init, mock_parse_port_map_alias,
                                    mock_get_render_template):
    mock_config_db = MockConfigDb(config_db_path="tests/test_data/mock_config_db.json")
    dhcp_db_connector = DhcpDbConnector()
    customized_options = {
        "option223": {
            "id": "223",
            "value": "dummy_value",
            "type": "string",
            "always_send": "true",
            "option_type": "customized"
        },
        "option60": {
            "id": "60",
            "value": "dummy_value",
            "type": "string",
            "always_send": "false",
            "option_type": "standard"
        },
        "option54": {
            "id": "54",
            "value": "10.0.0.1",
            "type": "ipv4-address",
            "always_send": "false",
            "option_type": "standard"
        }
    }
    dhcp_cfg_generator = DhcpServCfgGenerator(dhcp_db_connector, "/usr/local/lib/kea/hooks/libdhcp_run_script.so")
    tested_hostname = "sonic-host"
    port_ips = {
        "Vlan1000": {
            "192.168.0.1/21": {
                "etp8": [["192.168.0.2", "192.168.0.6"], ["192.168.0.10", "192.168.0.10"]],
                "etp7": [["192.168.0.7", "192.168.0.7"]],
                "etp9": []
            }
        },
        "Vlan4000": {
            "192.168.3.1/24": {
                "etp11": [["192.168.3.2", "192.168.3.3"]]
            }
        },
        "Vlan6000": {
        }
    }
    render_obj, enabled_dhcp_interfaces, used_options, subscribe_table = \
        dhcp_cfg_generator._construct_obj_for_template(mock_config_db.config_db.get("DHCP_SERVER_IPV4"),
                                                       port_ips, tested_hostname, customized_options)
    assert render_obj == expected_render_obj
    assert enabled_dhcp_interfaces == {"Vlan1000", "Vlan4000", "Vlan3000", "Vlan6000"}
    assert used_options == set(["option223", "option60", "option54"])
    assert subscribe_table == set(PORT_MODE_CHECKER)


@pytest.mark.parametrize("with_port_config", [True, False])
@pytest.mark.parametrize("with_option_config", [True, False])
def test_render_config(mock_swsscommon_dbconnector_init, mock_parse_port_map_alias, with_port_config,
                       with_option_config, request):
    dhcp_db_connector = DhcpDbConnector()
    dhcp_cfg_generator = DhcpServCfgGenerator(dhcp_db_connector, "/usr/local/lib/kea/hooks/libdhcp_run_script.so",
                                              kea_conf_template_path="tests/test_data/kea-dhcp4.conf.j2")
    render_obj = copy.deepcopy(expected_render_obj)
    if not with_port_config:
        render_obj["client_classes"] = []
        render_obj["subnets"] = []
        expected_config = copy.deepcopy(expected_dhcp_config_without_port_config)
    else:
        expected_config = copy.deepcopy(expected_dhcp_config)
        if not with_option_config:
            for i in range(len(expected_config["Dhcp4"]["subnet4"])):
                render_obj["subnets"][i]["customized_options"] = {}
        else:
            for i in range(len(expected_config["Dhcp4"]["subnet4"])):
                expected_config["Dhcp4"]["subnet4"][i]["option-data"].insert(0, {
                                "code": 60,
                                "data": "dummy_value",
                                "always-send": False,
                                "csv-format": True
                        })
                expected_config["Dhcp4"]["subnet4"][i]["option-data"].insert(0, {
                                "name": "option223",
                                "data": "dummy_value",
                                "always-send": True
                        })
    config = dhcp_cfg_generator._render_config(render_obj)
    assert json.loads(config) == expected_config


def test_parse_customized_options(mock_swsscommon_dbconnector_init, mock_get_render_template,
                                  mock_parse_port_map_alias):
    dhcp_db_connector = DhcpDbConnector()
    dhcp_cfg_generator = DhcpServCfgGenerator(dhcp_db_connector, "/usr/local/lib/kea/hooks/libdhcp_run_script.so")
    customized_options_ipv4 = tested_options_data["data"]
    customized_options = dhcp_cfg_generator._parse_customized_options(customized_options_ipv4)
    expected_res = {}
    for key, value in tested_options_data["res"].items():
        expected_res[key] = {
            "id": customized_options_ipv4[key]["id"],
            "value": value["value"],
            "type": value.get("type", customized_options_ipv4[key]["type"]),
            "always_send": "true",
            "option_type": value["option_type"]
        }
    assert customized_options == expected_res


def test_parse_dpus(mock_swsscommon_dbconnector_init, mock_get_render_template, mock_parse_port_map_alias):
    dhcp_db_connector = DhcpDbConnector()
    dhcp_cfg_generator = DhcpServCfgGenerator(dhcp_db_connector, "/usr/local/lib/kea/hooks/libdhcp_run_script.so")
    dpus_table = {"dpu0": {"midplane_interface": "dpu0"}}
    mid_plane_table = {"GLOBAL": {"bridge": "bridge-midplane", "ip_prefix": "169.254.200.254/24"}}
    mid_plane, dpus = dhcp_cfg_generator._parse_dpu(dpus_table, mid_plane_table)
    assert mid_plane == {"bridge": "bridge-midplane", "ip_prefix": "169.254.200.254/24"}
    assert dpus == set(["dpu0"])
