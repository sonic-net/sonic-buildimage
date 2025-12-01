import click
import copy
import pytest
import sys
sys.path.append("../cli/show/plugins/")
import show_dhcp_relay

from unittest.mock import patch, call, MagicMock, ANY
from click.testing import CliRunner

SUPPORTED_DHCPV4_TYPE = [
    "Unknown", "Discover", "Offer", "Request", "Decline", "Ack", "Nak", "Release", "Inform", "Bootp"
]
SUPPORTED_DIR = ["TX", "RX"]
expected_counts_v4 = """\
Packet type Abbr: Un - Unknown, Dis - Discover, Off - Offer, Req - Request,
                  Ack - Acknowledge, Nack - NegativeAcknowledge, Rel - Release,
                  Inf - Inform, Dec - Decline, Mal - Malformed, Drp - Dropped

+-------------+------+-------+-------+-------+-------+--------+-------+-------+-------+-------+-------+
| Vlan (TX)   | Un   | Dis   | Off   | Req   | Ack   | Nack   | Rel   | Inf   | Dec   | Mal   | Drp   |
+=============+======+=======+=======+=======+=======+========+=======+=======+=======+=======+=======+
+-------------+------+-------+-------+-------+-------+--------+-------+-------+-------+-------+-------+
"""
expected_counts_v4_rx = """\
Packet type Abbr: Un - Unknown, Dis - Discover, Off - Offer, Req - Request,
                  Ack - Acknowledge, Nack - NegativeAcknowledge, Rel - Release,
                  Inf - Inform, Dec - Decline, Mal - Malformed, Drp - Dropped

+-------------+------+-------+-------+-------+-------+--------+-------+-------+-------+-------+-------+
| Vlan (RX)   | Un   | Dis   | Off   | Req   | Ack   | Nack   | Rel   | Inf   | Dec   | Mal   | Drp   |
+=============+======+=======+=======+=======+=======+========+=======+=======+=======+=======+=======+
+-------------+------+-------+-------+-------+-------+--------+-------+-------+-------+-------+-------+
"""

expected_counts_v4_type = """\
+-------------------+------+------+
| Vlan (Discover)   | TX   | RX   |
+===================+======+======+
+-------------------+------+------+
"""



def test_plugin_registration():
    cli = MagicMock()
    show_dhcp_relay.register(cli)


@pytest.mark.parametrize("valid", [True, False])
@pytest.mark.parametrize("vlan_interface", ["Vlan1000", ""])
def test_is_vlan_interface_valid(valid, vlan_interface):
    mock_db = MagicMock()
    mock_db.exists.return_value = valid
    result = show_dhcp_relay.is_vlan_interface_valid(vlan_interface, mock_db)
    if vlan_interface:
        assert result == valid
    else:
        assert result


def test_get_vlan_members_from_config_db():
    mock_db = MagicMock()
    mock_db.keys.return_value = [
        "VLAN_MEMBER|Vlan1000|Ethernet1",
        "VLAN_MEMBER|Vlan1000|Ethernet2",
        "VLAN_MEMBER|Vlan2000|Ethernet3"
    ]
    result = show_dhcp_relay.get_vlan_members_from_config_db(mock_db, "Vlan1000")
    assert result == {
        "Vlan1000": set(["Ethernet1", "Ethernet2"])
    }


@pytest.mark.parametrize("get, expected_res", [
    (None, 0),
    ("{'Unknown':'0','Discover':'1','Offer':'0'}", "1"),
    ("{'Unknown':'0','Request':'1','Offer':'0'}", "0")
])
def test_get_count_from_db(get, expected_res):
    mock_db = MagicMock()
    mock_db.get.return_value = get
    res = show_dhcp_relay.get_count_from_db(mock_db, "", "", "Discover")
    assert res == expected_res


@pytest.mark.parametrize("vlan, dirs_count", [
    ("Vlan1000", {"RX": "1", "TX": "5"}),
    ("Vlan1000", {"RX": "3"})
])
def test_append_vlan_count_with_type_specified(vlan, dirs_count):
    def mock_get_count_from_db(db, key, current_dir, current_type):
        return dirs_count[current_dir]

    with patch.object(show_dhcp_relay, "get_count_from_db", side_effect=mock_get_count_from_db):
        dummy_summary = "dummy"
        data = {
            dummy_summary: [],
            "Intf Type": []
        }
        for dir in dirs_count.keys():
            data[dir] = []
        expected_res = copy.deepcopy(data)
        show_dhcp_relay.append_vlan_count_with_type_specified(data, MagicMock, list(dirs_count.keys()), vlan, "",
                                                              dummy_summary)
        expected_res["Intf Type"].append("VLAN")
        expected_res[dummy_summary].append(vlan)
        for dir, count in dirs_count.items():
            expected_res[dir].append(count)
        assert expected_res == data


@pytest.mark.parametrize("interface_name, vlan_interface, expected_result", [
    ("Ethernet1", "Vlan1000", "Downlink"),
    ("Ethernet4", "Vlan1000", "Uplink"),
    ("Ethernet5", "Vlan3000", "Unknown"),
    ("eth0", "Vlan3000", "MGMT")
])
def test_get_interfaces_type(interface_name, vlan_interface, expected_result):
    vlan_members = {
        "Vlan1000": set(["Ethernet1", "Ethernet2"]),
        "Vlan2000": set(["Ethernet3"])
    }
    mgmt_intfs = ["eth0"]
    result = show_dhcp_relay.get_interfaces_type(interface_name, mgmt_intfs, vlan_interface, vlan_members)
    assert result == expected_result


@pytest.mark.parametrize("interfaces, dirs_count, vlan_interface", [
    (["Ethernet1", "Ethernet2", "Ethernet3", "Ethernet4"], {"RX": "1", "TX": "5"}, "Vlan1000"),
    (["eth0", "Ethernet3", "Ethernet4", "Ethernet5"], {"RX": "3 ", "TX": "5"}, "Vlan3000"),
])
def test_append_interfaces_count_with_type_specified(interfaces, dirs_count, vlan_interface):
    def mock_get_count_from_db(db, key, current_dir, current_type):
        return dirs_count[current_dir]

    vlan_members = {
        "Vlan1000": set(["Ethernet1", "Ethernet2"]),
        "Vlan2000": set(["Ethernet3"])
    }
    mgmt_intfs = ["eth0"]
    with patch.object(show_dhcp_relay, "get_count_from_db", side_effect=mock_get_count_from_db):
        dummy_summary = "dummy"
        data = {
            dummy_summary: [],
            "Intf Type": []
        }
        for dir in dirs_count.keys():
            data[dir] = []
        expected_data = copy.deepcopy(data)
        mock_db = MagicMock()
        mock_db.keys.return_value = ["DHCPV4_COUNTER_TABLE:" + vlan_interface + ":" + intf for intf in interfaces]
        show_dhcp_relay.append_interfaces_count_with_type_specified(data, mock_db, list(dirs_count.keys()),
                                                                    vlan_interface, "", dummy_summary, vlan_members,
                                                                    mgmt_intfs)
        expected_data[dummy_summary] = interfaces
        for intf in interfaces:
            if intf in mgmt_intfs:
                expected_data["Intf Type"].append("MGMT")
            elif vlan_interface not in vlan_members:
                expected_data["Intf Type"].append("Unknown")
            elif intf in vlan_members[vlan_interface]:
                expected_data["Intf Type"].append("Downlink")
            else:
                expected_data["Intf Type"].append("Uplink")
            for dir in dirs_count.keys():
                expected_data[dir].append(dirs_count[dir])
        assert expected_data == data


def test_generate_output_with_type_specified():
    expected_output = """\
+------------------+-----------+----+----+
| Vlan1000 (Offer) | Intf Type | RX | TX |
+------------------+-----------+----+----+
| Vlan1000         | VLAN      | 5  | 5  |
| Ethernet0        | Downlink  | 10 | 10 |
| Ethernet1        | Uplink    | 20 | 20 |
+------------------+-----------+----+----+
"""

    def mock_append_vlan(data, db, dirs, vlan_interface, current_type, summary):
        data[summary].append("Vlan1000")
        data["Intf Type"].append("VLAN")
        for dir in dirs:
            data[dir].append("5")

    def mock_append_interfaces(data, db, dirs, vlan_interface, current_type, summary, vlan_members, mgmt_intfs):
        data[summary].extend(["Ethernet0", "Ethernet1"])
        data["Intf Type"].extend(["Downlink", "Uplink"])
        for dir in dirs:
            data[dir].extend(["10", "20"])

    with patch.object(show_dhcp_relay, "append_vlan_count_with_type_specified", side_effect=mock_append_vlan), \
         patch.object(show_dhcp_relay, "append_interfaces_count_with_type_specified",
                      side_effect=mock_append_interfaces):
        output = show_dhcp_relay.generate_output_with_type_specified(MagicMock(), [], ["Offer"], ["RX", "TX"],
                                                                     "Vlan1000", [])
        assert output == expected_output


@pytest.mark.parametrize("vlan, types_count", [
    ("Vlan1000", {"Discover": "1", "Offer": "5"}),
    ("Vlan1000", {"Ack": "3"})
])
def test_append_vlan_count_without_type_specified(vlan, types_count):
    def mock_get_count_from_db(db, key, current_dir, current_type):
        return types_count[current_type]

    with patch.object(show_dhcp_relay, "get_count_from_db", side_effect=mock_get_count_from_db):
        dummy_summary = "dummy"
        data = {
            dummy_summary: [],
            "Intf Type": []
        }
        for type in types_count.keys():
            data[type] = []
        expected_res = copy.deepcopy(data)
        show_dhcp_relay.append_vlan_count_without_type_specified(data, MagicMock(), "", vlan, list(types_count.keys()),
                                                                 dummy_summary)
        expected_res["Intf Type"].append("VLAN")
        expected_res[dummy_summary].append(vlan)
        for type, count in types_count.items():
            expected_res[type].append(count)
        assert expected_res == data


def test_natural_sort_key():
    assert show_dhcp_relay.natural_sort_key("Ethernet12") == ["ethernet", 12, ""]


@pytest.mark.parametrize("types_count", [{"Discover": "5", "Offer": "10"}, {"Request": "3", "Ack": "5"}])
def test_append_interfaces_count_without_type_specified(types_count):
    def mock_get_count_from_db(db, key, current_dir, current_type):
        return types_count[current_type]

    mock_db = MagicMock()
    interfaces = ["Ethernet0", "Ethernet1"]
    mock_db.keys.return_value = ["DHCPV4_COUNTER_TABLE:Vlan1000:{}".format(intf) for intf in interfaces]
    dummy_summary = "dummy"
    data = {
        dummy_summary: [],
        "Intf Type": []
    }
    for type in types_count.keys():
        data[type] = []
    with patch.object(show_dhcp_relay, "get_interfaces_type", return_value="Downlink"), \
         patch.object(show_dhcp_relay, "get_count_from_db", side_effect=mock_get_count_from_db):
        expected_data = copy.deepcopy(data)
        show_dhcp_relay.append_interfaces_count_without_type_specified(data, mock_db, "", "", list(types_count.keys()),
                                                                       dummy_summary, {}, [])
        expected_data[dummy_summary].extend(interfaces)
        expected_data["Intf Type"].extend(["Downlink", "Downlink"])
        for type, count in types_count.items():
            expected_data[type].extend([count, count])
        assert expected_data == data


def test_generate_output_without_type_specified():
    expected_output = """\
+---------------+-----------+----------+-------+
| Vlan1000 (RX) | Intf Type | Discover | Offer |
+---------------+-----------+----------+-------+
| Vlan1000      | VLAN      | 10       | 10    |
| Ethernet0     | Downlink  | 20       | 20    |
| Ethernet1     | Uplink    | 30       | 30    |
+---------------+-----------+----------+-------+
"""

    def mock_append_vlan(data, db, dir, vlan_interface, types, summary):
        data[summary].append("Vlan1000")
        data["Intf Type"].append("VLAN")
        for type in types:
            data[type].append("10")

    def mock_append_interfaces(data, db, dir, vlan_interface, types, summary, vlan_members,
                               mgmt_intfs):
        data[summary].extend(["Ethernet0", "Ethernet1"])
        data["Intf Type"].extend(["Downlink", "Uplink"])
        for type in types:
            data[type].extend(["20", "30"])
    with patch.object(show_dhcp_relay, "append_vlan_count_without_type_specified", side_effect=mock_append_vlan), \
            patch.object(show_dhcp_relay, "append_interfaces_count_without_type_specified",
                         side_effect=mock_append_interfaces):
        output = show_dhcp_relay.generate_output_without_type_specified(MagicMock(), {}, ["Discover", "Offer"], "RX",
                                                                        "Vlan1000", [])
        assert expected_output == output


def test_get_mgmt_intfs():
    mock_db = MagicMock()
    mock_db.keys.return_value = [
        "MGMT_PORT|eth0",
        "MGMT_PORT|eth1"
    ]
    result = show_dhcp_relay.get_mgmt_intfs_from_config_db(mock_db)
    assert result == ["eth0", "eth1"]


@pytest.mark.parametrize("vlan, expected_result", [
    ("", set(["Vlan1000", "Vlan2000", "Vlan100"])),
    ("Vlan1000", set(["Vlan1000"])),
    ("Vlan100", set(["Vlan100"]))
])
def test_get_vlans_from_counters_db(vlan, expected_result):
    mock_db = MagicMock()
    mock_db.keys.return_value = [
        "DHCPV4_COUNTER_TABLE:Vlan1000",
        "DHCPV4_COUNTER_TABLE:Vlan100",
        "DHCPV4_COUNTER_TABLE:Vlan1000:Ethernet0",
        "DHCPV4_COUNTER_TABLE:Vlan2000"
    ]
    result = show_dhcp_relay.get_vlans_from_counters_db(vlan, mock_db)
    assert result == expected_result


def test_print_dhcpv4_relay_counter_data_with_type_specified():
    mock_vlans = ["Vlan1000", "Vlan2000"]
    mock_vlan_members = {"Vlan1000": set(["Ethernet0"]), "Vlan2000": set(["Ethernet1"])}
    mock_mgmt_intfs = ["eth0"]
    types = ["Discover"]
    dirs = ["RX", "TX"]
    with patch.object(show_dhcp_relay, "get_vlans_from_counters_db", return_value=mock_vlans), \
         patch.object(show_dhcp_relay, "get_vlan_members_from_config_db", return_value=mock_vlan_members), \
         patch.object(show_dhcp_relay, "get_mgmt_intfs_from_config_db", return_value=mock_mgmt_intfs), \
         patch.object(show_dhcp_relay, "generate_output_with_type_specified",
                      return_value="") as mock_generate_with_type_specified, \
         patch.object(show_dhcp_relay,
                      "generate_output_without_type_specified",
                      return_value="") as mock_generate_withtout_type_specified:
        show_dhcp_relay.print_dhcpv4_relay_counter_data("", dirs, types, MagicMock())

        # Verify calls for generate_output_with_type_specified
        expected_calls = [
            call(ANY, mock_vlan_members, types, dirs, vlan, mock_mgmt_intfs)
            for vlan in mock_vlans
        ]
        mock_generate_with_type_specified.assert_has_calls(expected_calls)

        # Verify calls for generate_output_without_type_specified
        mock_generate_withtout_type_specified.assert_not_called()


def test_print_dhcpv4_relay_counter_data_without_type_specified():
    mock_vlans = ["Vlan1000", "Vlan2000"]
    mock_vlan_members = {"Vlan1000": set(["Ethernet0"]), "Vlan2000": set(["Ethernet1"])}
    mock_mgmt_intfs = ["eth0"]
    types = ["Discover", "Offer", "Request", "ack"]
    dirs = ["RX", "TX"]
    with patch.object(show_dhcp_relay, "get_vlans_from_counters_db", return_value=mock_vlans), \
         patch.object(show_dhcp_relay, "get_vlan_members_from_config_db", return_value=mock_vlan_members), \
         patch.object(show_dhcp_relay, "get_mgmt_intfs_from_config_db", return_value=mock_mgmt_intfs), \
         patch.object(show_dhcp_relay, "generate_output_with_type_specified",
                      return_value="") as mock_generate_with_type_specified, \
         patch.object(show_dhcp_relay,
                      "generate_output_without_type_specified",
                      return_value="") as mock_generate_withtout_type_specified:
        show_dhcp_relay.print_dhcpv4_relay_counter_data("", dirs, types, MagicMock())

        # Verify calls for generate_output_with_type_specified
        mock_generate_with_type_specified.assert_not_called()

        # Verify calls for generate_output_without_type_specified
        calls = []
        for vlan in mock_vlans:
            for dir in dirs:
                calls.append(call(ANY, mock_vlan_members, types, dir, vlan, mock_mgmt_intfs))
        mock_generate_withtout_type_specified.assert_has_calls(calls)


def test_dhcp_relay_ip4counters_vlan_invalid():
    runner = CliRunner()
    with patch.object(show_dhcp_relay, "is_vlan_interface_valid", return_value=False) as mock_check, \
         patch.object(click, "echo") as mock_echo, \
         patch.object(show_dhcp_relay, "print_dhcpv4_relay_counter_data") as mock_print:
        vlan = "Vlan1000"
        result = runner.invoke(show_dhcp_relay.dhcp_relay.commands["ipv4"].commands["counters"], vlan)
        assert result.exit_code == 0
        mock_check.assert_called_once_with(vlan, ANY)
        mock_echo.assert_called_once_with("Vlan interface {} doesn't have any count data".format(vlan))
        mock_print.assert_not_called()


@pytest.mark.parametrize("dir, type, expected_dirs, expected_types", [
    (None, None, SUPPORTED_DIR, SUPPORTED_DHCPV4_TYPE),
    (None, "Discover", SUPPORTED_DIR, ["Discover"]),
    ("RX", None, ["RX"], SUPPORTED_DHCPV4_TYPE),
    ("RX", "Offer", ["RX"], ["Offer"])
])
def test_dhcp_relay_ip4counters(dir, type, expected_dirs, expected_types):
    runner = CliRunner()
    with patch.object(show_dhcp_relay, "is_vlan_interface_valid", return_value=True), \
         patch.object(show_dhcp_relay, "print_dhcpv4_relay_counter_data") as mock_print:
        args = []
        if dir:
            args.append("--dir={}".format(dir))
        if type:
            args.append("--type={}".format(type))
        result = runner.invoke(show_dhcp_relay.dhcp_relay.commands["ipv4"].commands["counters"], args)
        assert result.exit_code == 0
        mock_print.assert_called_once_with("", expected_dirs, expected_types, ANY)


@pytest.mark.parametrize("args", ["--dir=fail", "--type=fail"])
def test_dhcp_relay_ip4counters_incorrect_dir(args):
    runner = CliRunner()
    result = runner.invoke(show_dhcp_relay.dhcp_relay.commands["ipv4"].commands["counters"], args)
    assert result.exit_code != 0

class TestDhcpRelayCounters(object):

    def test_show_vlan_counts(self):
        runner = CliRunner()
        result = runner.invoke(show_dhcp_relay.dhcp4relay_counters.commands["vlan-counts"], ["Vlan1000"])
        print(result.output)
        assert result.output == expected_counts_v4

    def test_show_vlan_counts_dir(self):
        runner = CliRunner()
        result = runner.invoke(show_dhcp_relay.dhcp4relay_counters.commands["vlan-counts"], ["Vlan1000", "-d", "RX"])
        print(result.output)
        assert result.output == expected_counts_v4_rx

    def test_show_vlan_counts_type(self):
        runner = CliRunner()
        result = runner.invoke(show_dhcp_relay.dhcp4relay_counters.commands["vlan-counts"], ["Vlan1000", "-t", "Discover"])
        print(result.output)
        assert result.output == expected_counts_v4_type

    def test_show_ipv4_vlan_counters(self):
        runner = CliRunner()
        result = runner.invoke(show_dhcp_relay.dhcp_relay_ipv4.commands["vlan-counters"], ["Vlan1000"])
        print(result.output)
        assert result.output == expected_counts_v4

    def test_show_ipv4_vlan_counters_dir(self):
        runner = CliRunner()
        result = runner.invoke(show_dhcp_relay.dhcp_relay_ipv4.commands["vlan-counters"], ["Vlan1000", "-d", "RX"])
        print(result.output)
        assert result.output == expected_counts_v4_rx

    def test_show_vlan_counts_type(self):
        runner = CliRunner()
        result = runner.invoke(show_dhcp_relay.dhcp_relay_ipv4.commands["vlan-counters"], ["Vlan1000", "-t", "Discover"])
        print(result.output)
        assert result.output == expected_counts_v4_type
