import ipaddress
import json
from unittest.mock import patch

import pytest

from dhcp_utilities.common.utils import DhcpDbConnector
from dhcp_utilities.dhcpservd.dhcp_cfggen import DhcpServCfgGenerator, MATCH_MODE_CHECKER, PORT_MODE_CHECKER


DHCP_SERVERS = {
    "Vlan1000": {
        "gateway": "192.168.0.1",
        "lease_time": "900",
        "mode": "MATCH",
        "state": "enabled"
    }
}
DHCP_INTERFACES = {
    "Vlan1000": [{
        "network": ipaddress.ip_network("192.168.0.0/24"),
        "ip": "192.168.0.1/24"
    }]
}
VLAN_MEMBERS = {"Vlan1000|Ethernet24"}
RANGES = {
    "fallback-range": [
        ipaddress.ip_address("192.168.0.30"),
        ipaddress.ip_address("192.168.0.30")
    ]
}
MATCHES = {
    "port-etp7": {"type": "circuit_id", "value": "etp7"},
    "vendor-a": {"type": "option60", "value": "MAIA-A"},
    "vendor-b": {"type": "option60", "value": "MAIA-B"}
}


class DictConnector:
    def __init__(self, tables):
        self.tables = tables

    def get_config_db_table(self, table_name):
        return self.tables.get(table_name, {})


def create_generator(mock_swsscommon_dbconnector_init, mock_get_render_template):
    with patch.object(DhcpServCfgGenerator, "_parse_port_map_alias"):
        generator = DhcpServCfgGenerator(
            DhcpDbConnector(),
            "/usr/local/lib/kea/hooks/libdhcp_run_script.so"
        )
    generator.port_alias_map = {
        "Ethernet24": "etp7",
        "Ethernet28": "etp8"
    }
    return generator


def parse_bindings(generator, bindings, matches=None, servers=None, interfaces=None, members=None, ranges=None):
    return generator._parse_match_bindings(
        servers if servers is not None else DHCP_SERVERS,
        matches if matches is not None else MATCHES,
        bindings,
        interfaces if interfaces is not None else DHCP_INTERFACES,
        members if members is not None else VLAN_MEMBERS,
        ranges if ranges is not None else RANGES,
        "sonic-host"
    )


def get_class_for_pool(pools, classes, pool_range):
    class_name = next(pool["client_class"] for pool in pools if pool["range"] == pool_range)
    return next(client_class for client_class in classes if client_class["name"] == class_name)


def test_parse_match_bindings_specificity_fallback(mock_swsscommon_dbconnector_init, mock_get_render_template):
    generator = create_generator(mock_swsscommon_dbconnector_init, mock_get_render_template)
    bindings = {
        "Vlan1000|specific": {
            "matches": ["port-etp7", "vendor-a"],
            "ips": ["192.168.0.20"]
        },
        "Vlan1000|fallback": {
            "matches": ["port-etp7"],
            "ranges": ["fallback-range"]
        }
    }

    pools, classes, used_ranges, used_matches = parse_bindings(generator, bindings)

    subnet_pools = pools["Vlan1000"]["192.168.0.1/24"]
    specific_class = get_class_for_pool(subnet_pools, classes, "192.168.0.20 - 192.168.0.20")
    fallback_class = get_class_for_pool(subnet_pools, classes, "192.168.0.30 - 192.168.0.30")
    assert "relay4[1].exists" in specific_class["condition"]
    assert "option[60].exists" in specific_class["condition"]
    assert "0x736f6e69632d686f73743a65747037" in specific_class["condition"]
    assert "0x4d4149412d41" in specific_class["condition"]
    assert "not" not in specific_class["condition"]
    assert "not" in fallback_class["condition"]
    assert used_ranges == {"fallback-range"}
    assert used_matches == {"port-etp7", "vendor-a"}


def test_identical_pool_bindings_are_grouped_with_or(mock_swsscommon_dbconnector_init, mock_get_render_template):
    generator = create_generator(mock_swsscommon_dbconnector_init, mock_get_render_template)
    bindings = {
        "Vlan1000|vendor-a": {
            "matches": ["vendor-a"],
            "ips": ["192.168.0.20"]
        },
        "Vlan1000|vendor-b": {
            "matches": ["vendor-b"],
            "ips": ["192.168.0.20"]
        }
    }

    pools, classes, _, _ = parse_bindings(generator, bindings)

    assert len(pools["Vlan1000"]["192.168.0.1/24"]) == 1
    assert len(classes) == 1
    assert " or " in classes[0]["condition"]
    assert "0x4d4149412d41" in classes[0]["condition"]
    assert "0x4d4149412d42" in classes[0]["condition"]


def test_adjacent_match_ips_are_normalized(mock_swsscommon_dbconnector_init, mock_get_render_template):
    generator = create_generator(mock_swsscommon_dbconnector_init, mock_get_render_template)
    bindings = {
        "Vlan1000|vendor-a": {
            "matches": ["vendor-a"],
            "ips": ["192.168.0.21", "192.168.0.20"]
        }
    }

    pools, _, _, _ = parse_bindings(generator, bindings)

    assert pools["Vlan1000"]["192.168.0.1/24"][0]["range"] == "192.168.0.20 - 192.168.0.21"


def test_four_bindings_on_one_port(mock_swsscommon_dbconnector_init, mock_get_render_template):
    generator = create_generator(mock_swsscommon_dbconnector_init, mock_get_render_template)
    matches = {"port-etp7": MATCHES["port-etp7"]}
    bindings = {}
    for index in range(4):
        match_name = "vendor-{}".format(index)
        matches[match_name] = {"type": "option60", "value": "VENDOR-{}".format(index)}
        bindings["Vlan1000|binding-{}".format(index)] = {
            "matches": ["port-etp7", match_name],
            "ips": ["192.168.0.{}".format(20 + index)]
        }

    pools, classes, _, used_matches = parse_bindings(generator, bindings, matches=matches)

    assert len(pools["Vlan1000"]["192.168.0.1/24"]) == 4
    assert len(classes) == 4
    assert len(used_matches) == 5


def test_match_compilation_is_deterministic(mock_swsscommon_dbconnector_init, mock_get_render_template):
    generator = create_generator(mock_swsscommon_dbconnector_init, mock_get_render_template)
    bindings = {
        "Vlan1000|vendor-b": {
            "matches": ["vendor-b", "port-etp7"],
            "ips": ["192.168.0.21"]
        },
        "Vlan1000|vendor-a": {
            "matches": ["port-etp7", "vendor-a"],
            "ips": ["192.168.0.20"]
        }
    }

    first = parse_bindings(generator, bindings)
    second = parse_bindings(generator, dict(reversed(list(bindings.items()))))

    assert first == second


@pytest.mark.parametrize("bindings,error", [
    ({}, "requires at least one binding"),
    ({
        "Vlan1000|missing": {
            "matches": ["missing-match"],
            "ips": ["192.168.0.20"]
        }
    }, "does not exist"),
    ({
        "Vlan1000|duplicate-type": {
            "matches": ["vendor-a", "vendor-b"],
            "ips": ["192.168.0.20"]
        }
    }, "multiple matches of type option60"),
    ({
        "Vlan1000|outside": {
            "matches": ["vendor-a"],
            "ips": ["192.168.1.20"]
        }
    }, "is not in an IPv4 subnet")
])
def test_invalid_match_bindings_are_rejected(mock_swsscommon_dbconnector_init, mock_get_render_template,
                                             bindings, error):
    generator = create_generator(mock_swsscommon_dbconnector_init, mock_get_render_template)

    with pytest.raises(ValueError, match=error):
        parse_bindings(generator, bindings)


def test_equal_specificity_overlap_is_rejected(mock_swsscommon_dbconnector_init, mock_get_render_template):
    generator = create_generator(mock_swsscommon_dbconnector_init, mock_get_render_template)
    bindings = {
        "Vlan1000|port-only": {
            "matches": ["port-etp7"],
            "ips": ["192.168.0.20"]
        },
        "Vlan1000|vendor-only": {
            "matches": ["vendor-a"],
            "ips": ["192.168.0.21"]
        }
    }

    with pytest.raises(ValueError, match="overlap with equal specificity"):
        parse_bindings(generator, bindings)


def test_partial_pool_overlap_is_rejected(mock_swsscommon_dbconnector_init, mock_get_render_template):
    generator = create_generator(mock_swsscommon_dbconnector_init, mock_get_render_template)
    ranges = {
        "range-a": [ipaddress.ip_address("192.168.0.20"), ipaddress.ip_address("192.168.0.30")],
        "range-b": [ipaddress.ip_address("192.168.0.25"), ipaddress.ip_address("192.168.0.35")]
    }
    bindings = {
        "Vlan1000|vendor-a": {
            "matches": ["vendor-a"],
            "ranges": ["range-a"]
        },
        "Vlan1000|vendor-b": {
            "matches": ["vendor-b"],
            "ranges": ["range-b"]
        }
    }

    with pytest.raises(ValueError, match="partially overlapping pools"):
        parse_bindings(generator, bindings, ranges=ranges)


def test_circuit_id_must_be_vlan_member(mock_swsscommon_dbconnector_init, mock_get_render_template):
    generator = create_generator(mock_swsscommon_dbconnector_init, mock_get_render_template)
    bindings = {
        "Vlan1000|port-only": {
            "matches": ["port-etp7"],
            "ips": ["192.168.0.20"]
        }
    }

    with pytest.raises(ValueError, match="is not a member"):
        parse_bindings(generator, bindings, members={"Vlan1000|Ethernet28"})


def test_match_value_byte_limit_is_enforced(mock_swsscommon_dbconnector_init, mock_get_render_template):
    generator = create_generator(mock_swsscommon_dbconnector_init, mock_get_render_template)
    matches = {
        "oversized": {"type": "option60", "value": "é" * 128}
    }
    bindings = {
        "Vlan1000|oversized": {
            "matches": ["oversized"],
            "ips": ["192.168.0.20"]
        }
    }

    with pytest.raises(ValueError, match="payload limit"):
        parse_bindings(generator, bindings, matches=matches)


def test_mixed_port_and_match_rendering(mock_swsscommon_dbconnector_init):
    with patch.object(DhcpServCfgGenerator, "_parse_port_map_alias"):
        generator = DhcpServCfgGenerator(
            DhcpDbConnector(),
            "/usr/local/lib/kea/hooks/libdhcp_run_script.so",
            kea_conf_template_path="tests/test_data/kea-dhcp4.conf.j2"
        )
    generator.port_alias_map = {"Ethernet24": "etp7"}
    servers = {
        "Vlan1000": {"mode": "PORT", "state": "enabled", "lease_time": "900"},
        "Vlan2000": {"mode": "MATCH", "state": "enabled", "lease_time": "900"}
    }
    interfaces = {
        "Vlan2000": [{
            "network": ipaddress.ip_network("192.168.1.0/24"),
            "ip": "192.168.1.1/24"
        }]
    }
    bindings = {
        "Vlan2000|vendor-a": {
            "matches": ["vendor-a"],
            "ips": ["192.168.1.20"]
        }
    }
    match_pools, match_classes, _, _ = parse_bindings(
        generator,
        bindings,
        servers=servers,
        interfaces=interfaces,
        members=set()
    )
    port_pools = {
        "Vlan1000": {
            "192.168.0.1/24": {
                "etp7": [["192.168.0.20", "192.168.0.20"]]
            }
        }
    }

    render_obj, enabled_interfaces, _, checkers = generator._construct_obj_for_template(
        servers,
        port_pools,
        "sonic-host",
        {},
        match_pools=match_pools,
        match_client_classes=match_classes
    )
    rendered = json.loads(generator._render_config(render_obj))

    assert enabled_interfaces == {"Vlan1000", "Vlan2000"}
    assert checkers == set(PORT_MODE_CHECKER) | set(MATCH_MODE_CHECKER)
    assert {subnet["subnet"] for subnet in rendered["Dhcp4"]["subnet4"]} == {
        "192.168.0.0/24", "192.168.1.0/24"
    }
    assert len(rendered["Dhcp4"]["client-classes"]) == 2


def test_match_mode_uses_midplane_subnet_id_on_smart_switch(
        mock_swsscommon_dbconnector_init, mock_get_render_template):
    generator = create_generator(mock_swsscommon_dbconnector_init, mock_get_render_template)
    match_pools, match_client_classes, _, _ = parse_bindings(
        generator,
        {
            "Vlan1000|binding-a": {
                "matches": ["port-etp7"],
                "ranges": ["range-a"]
            }
        },
        ranges={
            "range-a": [
                ipaddress.ip_address("192.168.0.20"),
                ipaddress.ip_address("192.168.0.20")
            ]
        }
    )
    render_obj, _, _, _ = generator._construct_obj_for_template(
        DHCP_SERVERS,
        {},
        "sonic-host",
        {},
        smart_switch=True,
        match_pools=match_pools,
        match_client_classes=match_client_classes
    )

    assert render_obj["subnets"][0]["id"] == 10000


def test_generate_match_config_and_snapshots():
    tables = {
        "DEVICE_METADATA": {"localhost": {"hostname": "sonic-host"}},
        "VLAN_INTERFACE": {
            "Vlan1000": {},
            "Vlan1000|192.168.0.1/24": {}
        },
        "VLAN_MEMBER": {
            "Vlan1000|Ethernet24": {}
        },
        "PORT": {
            "Ethernet24": {"alias": "etp7"}
        },
        "PORTCHANNEL": {},
        "DPUS": {},
        "MID_PLANE_BRIDGE": {},
        "DHCP_SERVER_IPV4": {
            "Vlan1000": {
                "gateway": "192.168.0.1",
                "lease_time": "900",
                "mode": "MATCH",
                "state": "enabled"
            },
            "Vlan2000": {
                "lease_time": "900",
                "mode": "MATCH",
                "state": "disabled"
            }
        },
        "DHCP_SERVER_IPV4_CUSTOMIZED_OPTIONS": {},
        "DHCP_SERVER_IPV4_RANGE": {
            "fallback-range": {"range": ["192.168.0.30"]}
        },
        "DHCP_SERVER_IPV4_PORT": {},
        "DHCP_SERVER_IPV4_MATCH": {
            **MATCHES,
            "unused": {"type": "unsupported", "value": ""}
        },
        "DHCP_SERVER_IPV4_BINDING": {
            "Vlan1000|specific": {
                "matches": ["port-etp7", "vendor-a"],
                "ips": ["192.168.0.20"]
            },
            "Vlan1000|fallback": {
                "matches": ["port-etp7"],
                "ranges": ["fallback-range"]
            },
            "Vlan2000|inactive-invalid": {
                "matches": ["missing"],
                "ranges": ["missing"]
            }
        }
    }
    generator = DhcpServCfgGenerator(
        DictConnector(tables),
        "/usr/local/lib/kea/hooks/libdhcp_run_script.so",
        kea_conf_template_path="tests/test_data/kea-dhcp4.conf.j2"
    )

    config, used_ranges, enabled_interfaces, used_options, checkers, enabled_port_interfaces, \
        enabled_match_interfaces, used_matches = generator.generate()
    parsed_config = json.loads(config)

    assert len(parsed_config["Dhcp4"]["subnet4"]) == 1
    assert len(parsed_config["Dhcp4"]["subnet4"][0]["pools"]) == 2
    assert len(parsed_config["Dhcp4"]["client-classes"]) == 2
    assert used_ranges == {"fallback-range"}
    assert enabled_interfaces == {"Vlan1000"}
    assert used_options == set()
    assert checkers == set(MATCH_MODE_CHECKER)
    assert enabled_port_interfaces == set()
    assert enabled_match_interfaces == {"Vlan1000"}
    assert used_matches == {"port-etp7", "vendor-a"}
