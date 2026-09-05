import click
import pytest
import sys
sys.path.append("../cli/show/plugins/")
import show_dhcp_relay

from unittest.mock import patch, call, MagicMock, ANY
from click.testing import CliRunner

# Constants matching show_dhcp_relay.py
DHCPMON_V4_PREFIX = "DHCPV4_COUNTER_TABLE"
DHCPMON_V6_PREFIX = "DHCPV6_COUNTER_TABLE"
DHCPMON_SUPPORTED_DIR = ["TX", "RX"]
DHCPMON_V4_DISPLAY_TYPES = [
    "Unknown", "Discover", "Offer", "Request", "Decline",
    "Ack", "Nak", "Release", "Inform", "Bootp"
]
DHCPMON_V6_DISPLAY_TYPES = [
    "Unknown", "Solicit", "Advertise", "Request", "Confirm",
    "Renew", "Rebind", "Reply", "Release", "Decline",
    "Reconfigure", "Information-Request",
    "Relay-Forward", "Relay-Reply"
]


# ======================================================================
# Shared dhcpmon helper function tests
# These are prefix-independent — logic is the same for v4 and v6.
# ======================================================================

def test_plugin_registration():
    cli = MagicMock()
    show_dhcp_relay.register(cli)


def test_get_vlan_members_from_config_db():
    mock_db = MagicMock()
    mock_db.keys.return_value = [
        "VLAN_MEMBER|Vlan1000|Ethernet1",
        "VLAN_MEMBER|Vlan1000|Ethernet2",
        "VLAN_MEMBER|Vlan2000|Ethernet3"
    ]
    result = show_dhcp_relay.get_vlan_members_from_config_db(
        mock_db, "Vlan1000"
    )
    assert result == {
        "Vlan1000": set(["Ethernet1", "Ethernet2"])
    }


@pytest.mark.parametrize("get, lookup, expected_res", [
    (None, "TypeA", 0),
    ("{'TypeA':'0','TypeB':'1','TypeC':'0'}", "TypeB", 1),
    ("{'TypeA':'0','TypeB':'1','TypeC':'0'}", "TypeD", 0)
])
def test_get_count_from_db(get, lookup, expected_res):
    mock_db = MagicMock()
    mock_db.get.return_value = get
    res = show_dhcp_relay.get_count_from_db(
        mock_db, "", "", lookup
    )
    assert res == expected_res


@pytest.mark.parametrize(
    "interface_name, vlan_interface, expected_result",
    [
        ("Ethernet1", "Vlan1000", "Downlink"),
        ("Ethernet4", "Vlan1000", "Uplink"),
        ("Ethernet5", "Vlan3000", "Unknown"),
        ("eth0", "Vlan3000", "MGMT")
    ]
)
def test_get_interfaces_type(
    interface_name, vlan_interface, expected_result
):
    vlan_members = {
        "Vlan1000": set(["Ethernet1", "Ethernet2"]),
        "Vlan2000": set(["Ethernet3"])
    }
    mgmt_intfs = ["eth0"]
    result = show_dhcp_relay.get_interfaces_type(
        interface_name, mgmt_intfs,
        vlan_interface, vlan_members
    )
    assert result == expected_result


def test_natural_sort_key():
    assert show_dhcp_relay.natural_sort_key("Ethernet12") == [
        "ethernet", 12, ""
    ]


def test_get_mgmt_intfs():
    mock_db = MagicMock()
    mock_db.keys.return_value = [
        "MGMT_PORT|eth0",
        "MGMT_PORT|eth1"
    ]
    result = show_dhcp_relay.get_mgmt_intfs_from_config_db(
        mock_db
    )
    assert result == ["eth0", "eth1"]


# ======================================================================
# Prefix-parameterized tests
# These test the shared dhcpmon functions with both v4 and v6
# prefixes. Fake type names ("TypeA", "TypeB") are used
# intentionally — we are testing function mechanics, not
# protocol-specific behavior.
# ======================================================================

@pytest.mark.parametrize("prefix", [
    DHCPMON_V4_PREFIX, DHCPMON_V6_PREFIX
])
@pytest.mark.parametrize("valid", [True, False])
@pytest.mark.parametrize("vlan_interface", ["Vlan1000", ""])
def test_is_dhcpmon_vlan_valid(valid, vlan_interface, prefix):
    mock_db = MagicMock()
    mock_db.exists.return_value = valid
    result = show_dhcp_relay.is_dhcpmon_vlan_valid(
        vlan_interface, mock_db, prefix
    )
    if vlan_interface:
        assert result == valid
    else:
        assert result


@pytest.mark.parametrize("prefix", [
    DHCPMON_V4_PREFIX, DHCPMON_V6_PREFIX
])
@pytest.mark.parametrize("vlan, expected_result", [
    ("", set(["Vlan1000", "Vlan2000", "Vlan100"])),
    ("Vlan1000", set(["Vlan1000"])),
    ("Vlan100", set(["Vlan100"]))
])
def test_get_dhcpmon_vlans(vlan, expected_result, prefix):
    mock_db = MagicMock()
    mock_db.keys.return_value = [
        prefix + ":Vlan1000",
        prefix + ":Vlan100",
        prefix + ":Vlan1000:Ethernet0",
        prefix + ":Vlan2000"
    ]
    result = show_dhcp_relay.get_dhcpmon_vlans(
        vlan, mock_db, prefix
    )
    assert result == expected_result


@pytest.mark.parametrize("prefix", [
    DHCPMON_V4_PREFIX, DHCPMON_V6_PREFIX
])
def test_generate_dhcpmon_output_with_type(prefix):
    expected_output = """\
+------------------+-----------+----+----+
| Vlan1000 (TypeA) | Intf Type | RX | TX |
+------------------+-----------+----+----+
| Vlan1000         | VLAN      | 5  | 5  |
| Ethernet0        | Downlink  | 10 | 10 |
| Ethernet1        | Uplink    | 20 | 20 |
+------------------+-----------+----+----+
"""
    vlan_members = {
        "Vlan1000": set(["Ethernet0"])
    }
    mgmt_intfs = []

    def mock_get_count(db, key, current_dir, current_type):
        if key == prefix + ":Vlan1000":
            return 5
        elif key == prefix + ":Vlan1000:Ethernet0":
            return 10
        elif key == prefix + ":Vlan1000:Ethernet1":
            return 20
        return 0

    mock_db = MagicMock()
    mock_db.keys.return_value = [
        prefix + ":Vlan1000:Ethernet0",
        prefix + ":Vlan1000:Ethernet1"
    ]
    with patch.object(
        show_dhcp_relay, "get_count_from_db",
        side_effect=mock_get_count
    ), patch.object(
        show_dhcp_relay, "get_interfaces_type",
        side_effect=["Downlink", "Uplink"]
    ):
        output = show_dhcp_relay.generate_dhcpmon_output_with_type(
            mock_db, vlan_members,
            ["TypeA"], ["RX", "TX"],
            "Vlan1000", mgmt_intfs, prefix
        )
        assert output == expected_output


@pytest.mark.parametrize("prefix", [
    DHCPMON_V4_PREFIX, DHCPMON_V6_PREFIX
])
def test_generate_dhcpmon_output_without_type(prefix):
    expected_output = """\
+---------------+-----------+-------+-------+
| Vlan1000 (RX) | Intf Type | TypeA | TypeB |
+---------------+-----------+-------+-------+
| Vlan1000      | VLAN      | 10    | 10    |
| Ethernet0     | Downlink  | 20    | 20    |
| Ethernet1     | Uplink    | 30    | 30    |
+---------------+-----------+-------+-------+
"""
    vlan_members = {
        "Vlan1000": set(["Ethernet0"])
    }
    mgmt_intfs = []

    def mock_get_count(db, key, current_dir, current_type):
        if key == prefix + ":Vlan1000":
            return 10
        elif key == prefix + ":Vlan1000:Ethernet0":
            return 20
        elif key == prefix + ":Vlan1000:Ethernet1":
            return 30
        return 0

    mock_db = MagicMock()
    mock_db.keys.return_value = [
        prefix + ":Vlan1000:Ethernet0",
        prefix + ":Vlan1000:Ethernet1"
    ]
    with patch.object(
        show_dhcp_relay, "get_count_from_db",
        side_effect=mock_get_count
    ), patch.object(
        show_dhcp_relay, "get_interfaces_type",
        side_effect=["Downlink", "Uplink"]
    ):
        output = show_dhcp_relay.generate_dhcpmon_output_without_type(
            mock_db, vlan_members,
            ["TypeA", "TypeB"], "RX",
            "Vlan1000", mgmt_intfs, prefix
        )
        assert expected_output == output


@pytest.mark.parametrize("prefix", [
    DHCPMON_V4_PREFIX, DHCPMON_V6_PREFIX
])
def test_print_dhcpmon_counter_data_with_type(prefix):
    mock_vlans = ["Vlan1000", "Vlan2000"]
    mock_vlan_members = {
        "Vlan1000": set(["Ethernet0"]),
        "Vlan2000": set(["Ethernet1"])
    }
    mock_mgmt_intfs = ["eth0"]
    types = ["TypeA"]
    dirs = ["RX", "TX"]
    with patch.object(
        show_dhcp_relay, "get_dhcpmon_vlans",
        return_value=mock_vlans
    ), patch.object(
        show_dhcp_relay, "get_vlan_members_from_config_db",
        return_value=mock_vlan_members
    ), patch.object(
        show_dhcp_relay, "get_mgmt_intfs_from_config_db",
        return_value=mock_mgmt_intfs
    ), patch.object(
        show_dhcp_relay,
        "generate_dhcpmon_output_with_type",
        return_value=""
    ) as mock_gen_with_type, patch.object(
        show_dhcp_relay,
        "generate_dhcpmon_output_without_type",
        return_value=""
    ) as mock_gen_without_type:
        show_dhcp_relay.print_dhcpmon_counter_data(
            "", dirs, types, MagicMock(), prefix
        )

        expected_calls = [
            call(
                ANY, mock_vlan_members, types, dirs,
                vlan, mock_mgmt_intfs, prefix
            )
            for vlan in mock_vlans
        ]
        mock_gen_with_type.assert_has_calls(expected_calls)
        mock_gen_without_type.assert_not_called()


@pytest.mark.parametrize("prefix", [
    DHCPMON_V4_PREFIX, DHCPMON_V6_PREFIX
])
def test_print_dhcpmon_counter_data_without_type(prefix):
    mock_vlans = ["Vlan1000", "Vlan2000"]
    mock_vlan_members = {
        "Vlan1000": set(["Ethernet0"]),
        "Vlan2000": set(["Ethernet1"])
    }
    mock_mgmt_intfs = ["eth0"]
    types = ["TypeA", "TypeB", "TypeC", "TypeD"]
    dirs = ["RX", "TX"]
    with patch.object(
        show_dhcp_relay, "get_dhcpmon_vlans",
        return_value=mock_vlans
    ), patch.object(
        show_dhcp_relay, "get_vlan_members_from_config_db",
        return_value=mock_vlan_members
    ), patch.object(
        show_dhcp_relay, "get_mgmt_intfs_from_config_db",
        return_value=mock_mgmt_intfs
    ), patch.object(
        show_dhcp_relay,
        "generate_dhcpmon_output_with_type",
        return_value=""
    ) as mock_gen_with_type, patch.object(
        show_dhcp_relay,
        "generate_dhcpmon_output_without_type",
        return_value=""
    ) as mock_gen_without_type:
        show_dhcp_relay.print_dhcpmon_counter_data(
            "", dirs, types, MagicMock(), prefix
        )

        mock_gen_with_type.assert_not_called()

        calls = []
        for vlan in mock_vlans:
            for dir in dirs:
                calls.append(
                    call(
                        ANY, mock_vlan_members, types, dir,
                        vlan, mock_mgmt_intfs, prefix
                    )
                )
        mock_gen_without_type.assert_has_calls(calls)


# ======================================================================
# Command wiring tests — show dhcp_relay ipv4/ipv6 counters
# These test the actual click commands with real v4/v6 types.
# ======================================================================

def test_show_dhcpmon_v4_counters_vlan_invalid():
    runner = CliRunner()
    with patch.object(
        show_dhcp_relay, "is_dhcpmon_vlan_valid",
        return_value=False
    ) as mock_check, patch.object(
        click, "echo"
    ) as mock_echo, patch.object(
        show_dhcp_relay, "print_dhcpmon_counter_data"
    ) as mock_print:
        vlan = "Vlan1000"
        result = runner.invoke(
            show_dhcp_relay.dhcp_relay.commands["ipv4"]
            .commands["counters"],
            vlan
        )
        assert result.exit_code == 0
        mock_check.assert_called_once_with(vlan, ANY, ANY)
        mock_echo.assert_called_once_with(
            "Vlan interface {} doesn't have any count data"
            .format(vlan)
        )
        mock_print.assert_not_called()


@pytest.mark.parametrize(
    "dir, type, expected_dirs, expected_types",
    [
        (None, None, DHCPMON_SUPPORTED_DIR,
         DHCPMON_V4_DISPLAY_TYPES),
        (None, "Discover", DHCPMON_SUPPORTED_DIR,
         ["Discover"]),
        ("RX", None, ["RX"], DHCPMON_V4_DISPLAY_TYPES),
        ("RX", "Offer", ["RX"], ["Offer"])
    ]
)
def test_show_dhcpmon_v4_counters(
    dir, type, expected_dirs, expected_types
):
    runner = CliRunner()
    with patch.object(
        show_dhcp_relay, "is_dhcpmon_vlan_valid",
        return_value=True
    ), patch.object(
        show_dhcp_relay, "print_dhcpmon_counter_data"
    ) as mock_print:
        args = []
        if dir:
            args.append("--dir={}".format(dir))
        if type:
            args.append("--type={}".format(type))
        result = runner.invoke(
            show_dhcp_relay.dhcp_relay.commands["ipv4"]
            .commands["counters"],
            args
        )
        assert result.exit_code == 0
        mock_print.assert_called_once_with(
            "", expected_dirs, expected_types, ANY, ANY
        )


@pytest.mark.parametrize(
    "args", ["--dir=fail", "--type=fail"]
)
def test_show_dhcpmon_v4_counters_incorrect_args(args):
    runner = CliRunner()
    result = runner.invoke(
        show_dhcp_relay.dhcp_relay.commands["ipv4"]
        .commands["counters"],
        args
    )
    assert result.exit_code != 0


def test_show_dhcpmon_v6_counters_vlan_invalid():
    runner = CliRunner()
    with patch.object(
        show_dhcp_relay, "is_dhcpmon_vlan_valid",
        return_value=False
    ) as mock_check, patch.object(
        click, "echo"
    ) as mock_echo, patch.object(
        show_dhcp_relay, "print_dhcpmon_counter_data"
    ) as mock_print:
        vlan = "Vlan1000"
        result = runner.invoke(
            show_dhcp_relay.dhcp_relay.commands["ipv6"]
            .commands["counters"],
            vlan
        )
        assert result.exit_code == 0
        mock_check.assert_called_once_with(vlan, ANY, ANY)
        mock_echo.assert_called_once_with(
            "Vlan interface {} doesn't have any count data"
            .format(vlan)
        )
        mock_print.assert_not_called()


@pytest.mark.parametrize(
    "dir, type, expected_dirs, expected_types",
    [
        (None, None, DHCPMON_SUPPORTED_DIR,
         DHCPMON_V6_DISPLAY_TYPES),
        (None, "Solicit", DHCPMON_SUPPORTED_DIR,
         ["Solicit"]),
        ("RX", None, ["RX"], DHCPMON_V6_DISPLAY_TYPES),
        ("TX", "Reply", ["TX"], ["Reply"])
    ]
)
def test_show_dhcpmon_v6_counters(
    dir, type, expected_dirs, expected_types
):
    runner = CliRunner()
    with patch.object(
        show_dhcp_relay, "is_dhcpmon_vlan_valid",
        return_value=True
    ), patch.object(
        show_dhcp_relay, "print_dhcpmon_counter_data"
    ) as mock_print:
        args = []
        if dir:
            args.append("--dir={}".format(dir))
        if type:
            args.append("--type={}".format(type))
        result = runner.invoke(
            show_dhcp_relay.dhcp_relay.commands["ipv6"]
            .commands["counters"],
            args
        )
        assert result.exit_code == 0
        mock_print.assert_called_once_with(
            "", expected_dirs, expected_types, ANY, ANY
        )


@pytest.mark.parametrize(
    "args", ["--dir=fail", "--type=fail"]
)
def test_show_dhcpmon_v6_counters_incorrect_args(args):
    runner = CliRunner()
    result = runner.invoke(
        show_dhcp_relay.dhcp_relay.commands["ipv6"]
        .commands["counters"],
        args
    )
    assert result.exit_code != 0
