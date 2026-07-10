import pytest
import sys
import click
import os
sys.path.append('../cli/show/plugins/')
import show_dhcp_relay as show
import show.vlan as vlan
from swsscommon import swsscommon
from mock_config import COMMON_TEST_DATA, NEW_ADDED_TEST_DATA, MULTI_TEST_DATA
from parameterized import parameterized
from pyfakefs.fake_filesystem_unittest import patchfs
from unittest import mock

try:
    sys.path.insert(0, '../../../src/sonic-host-services/tests/common')
    from mock_configdb import MockConfigDb
    swsscommon.ConfigDBConnector = MockConfigDb
except KeyError:
    pass

expected_ipv6_table_with_header = """\
+-------------+----------------------+
|   Interface |   DHCP Relay Address |
+=============+======================+
|    Vlan1000 |         fc02:2000::1 |
|             |         fc02:2000::2 |
+-------------+----------------------+
"""

expected_ipv4_table_with_header = """\
+-------------+----------------------+
|   Interface |   DHCP Relay Address |
+=============+======================+
|    Vlan1000 |            192.0.0.1 |
|             |            192.0.0.2 |
+-------------+----------------------+
"""

expected_ipv4_table_with_enabled_dhcp_server_with_header = """\
+-------------+----------------------+
|   Interface |   DHCP Relay Address |
+=============+======================+
|    Vlan1000 |                  N/A |
+-------------+----------------------+
"""

expected_ipv6_table_without_header = """\
--------  ------------
Vlan1000  fc02:2000::1
          fc02:2000::2
--------  ------------
"""

expected_ipv6_table_multi_with_header = """\
+-------------+----------------------+
|   Interface |   DHCP Relay Address |
+=============+======================+
|    Vlan1000 |         fc02:2000::1 |
|             |         fc02:2000::2 |
+-------------+----------------------+
|    Vlan1001 |         fc02:2000::3 |
|             |         fc02:2000::4 |
+-------------+----------------------+
"""

expected_ipv4_table_multi_with_header = """\
+-------------+----------------------+
|   Interface |   DHCP Relay Address |
+=============+======================+
|    Vlan1000 |            192.0.0.1 |
|             |            192.0.0.2 |
+-------------+----------------------+
|    Vlan1001 |            192.0.0.3 |
|             |            192.0.0.4 |
+-------------+----------------------+
"""

expected_dhcpv4_table_multi_with_header = """\
+-------------+----------------------+--------------+--------------------+------------------+-----------------+----------------------+--------------------+-----------------+
|   Interface |   DHCP Relay Address |   Server Vrf |   Source Interface |   Link Selection |   VRF Selection |   Server ID Override |   Agent Relay Mode |   Max Hop Count |
+=============+======================+==============+====================+==================+=================+======================+====================+=================+
|    Vlan1000 |            192.0.0.1 |      default |        Ethernet112 |           enable |          enable |                  N/A |                N/A |             N/A |
|             |            192.0.0.2 |              |                    |                  |                 |                      |                    |                 |
+-------------+----------------------+--------------+--------------------+------------------+-----------------+----------------------+--------------------+-----------------+
|    Vlan1001 |            192.0.0.3 |          N/A |                N/A |              N/A |             N/A |                  N/A |            discard |               5 |
|             |            192.0.0.4 |              |                    |                  |                 |                      |                    |                 |
+-------------+----------------------+--------------+--------------------+------------------+-----------------+----------------------+--------------------+-----------------+
"""

DBCONFIG_PATH = '/var/run/redis/sonic-db/database_config.json'

IP_VER_TEST_PARAM_MAP = {
    "ipv4": {
        "entry": "dhcp_servers",
        "table": "VLAN"
    },
    "ipv4_dhcp": {
        "entry":  ["dhcpv4_servers", "server_vrf", "source_interface", "link_selection",
                   "vrf_selection", "server_id_override", "agent_relay_mode", "max_hop_count"],
        "table": "DHCPV4_RELAY"
    },
    "ipv6": {
        "entry": "dhcpv6_servers",
        "table": "DHCP_RELAY"
    }
}


def test_plugin_registration():
    cli = mock.MagicMock()
    show.register(cli)
    assert 'DHCP Helper Address' in dict(vlan.VlanBrief.COLUMNS)


@pytest.mark.parametrize("feature_table", [{}, {"dhcp_server": {"state": "disabled"}},
                                           {"dhcp_server": {"state": "enabled"}}, {"dhcp_server": {}}])
def test_dhcp_relay_column_output(feature_table):
    ctx = (
        ({'Vlan1001': {'dhcp_servers': ['192.0.0.1', '192.168.0.2']}}, {}, {}),
        (MockDb({"FEATURE": feature_table})),
    )
    if "dhcp_server" in feature_table and "state" in feature_table["dhcp_server"] and \
       feature_table["dhcp_server"]["state"] == "enabled":
        assert show.get_dhcp_helper_address(ctx, 'Vlan1001') == 'N/A'
    else:
        assert show.get_dhcp_helper_address(ctx, 'Vlan1001') == '192.0.0.1\n192.168.0.2'


@parameterized.expand(COMMON_TEST_DATA)
@patchfs
def test_show_dhcp_relay(test_name, test_data, fs):
    if not os.path.exists(DBCONFIG_PATH):
        fs.create_file(DBCONFIG_PATH)
    MockConfigDb.set_config_db(test_data["config_db"])
    config_db = MockConfigDb()
    ip_version = "ipv4" if "ipv4" in test_name else "ipv6"
    table = config_db.get_table(IP_VER_TEST_PARAM_MAP[ip_version]["table"])
    if test_name in ["ipv4_with_header", "ipv4_with_disabled_dhcp_server_with_header"]:
        result = show.get_dhcp_relay_data_with_header(table, IP_VER_TEST_PARAM_MAP[ip_version]["entry"])
        expected_output = expected_ipv4_table_with_header
    elif test_name == "ipv6_with_header":
        result = show.get_dhcp_relay_data_with_header(table, IP_VER_TEST_PARAM_MAP[ip_version]["entry"])
        expected_output = expected_ipv6_table_with_header
    elif test_name == "ipv6_without_header":
        result = show.get_data(table, "Vlan1000")
        expected_output = expected_ipv6_table_without_header
    elif test_name == "ipv4_with_enabled_dhcp_server_with_header":
        result = show.get_dhcp_relay_data_with_header(table, IP_VER_TEST_PARAM_MAP[ip_version]["entry"], True)
        expected_output = expected_ipv4_table_with_enabled_dhcp_server_with_header
    assert result == expected_output


@parameterized.expand(NEW_ADDED_TEST_DATA)
@patchfs
def test_show_new_added_dhcp_relay(test_name, test_data, fs):
    if not os.path.exists(DBCONFIG_PATH):
        fs.create_file(DBCONFIG_PATH)
    MockConfigDb.set_config_db(test_data["config_db"])
    config_db = MockConfigDb()
    ip_version = test_name
    table = config_db.get_table(IP_VER_TEST_PARAM_MAP[ip_version]["table"])
    if ip_version == "ipv4":
        result = show.get_dhcp_relay_data_with_header(table, IP_VER_TEST_PARAM_MAP[ip_version]["entry"])
        expected_output = expected_ipv4_table_with_header
        assert result == expected_output
    else:
        result = show.get_dhcp_relay_data_with_header(table, IP_VER_TEST_PARAM_MAP[ip_version]["entry"])
        expected_output = expected_ipv6_table_with_header
        assert result == expected_output

        result = show.get_data(table, "Vlan1001")
        expected_output = ""
        assert result == expected_output


@parameterized.expand(MULTI_TEST_DATA)
@patchfs
def test_show_multi_dhcp_relay(test_name, test_data, fs):
    if not os.path.exists(DBCONFIG_PATH):
        fs.create_file(DBCONFIG_PATH)
    MockConfigDb.set_config_db(test_data["config_db"])
    config_db = MockConfigDb()
    ip_version = test_name
    table = config_db.get_table(IP_VER_TEST_PARAM_MAP[ip_version]["table"])

    if ip_version == "ipv4_dhcp":
        config_db.set_entry("DEVICE_METADATA", "localhost", {"has_sonic_dhcpv4_relay" : "True"})
        result = show.get_dhcpv4_relay_data_with_header(table, IP_VER_TEST_PARAM_MAP[ip_version]["entry"])
    else:
        result = show.get_dhcp_relay_data_with_header(table, IP_VER_TEST_PARAM_MAP[ip_version]["entry"])
    if ip_version == "ipv4":
        expected_output = expected_ipv4_table_multi_with_header
    elif ip_version == "ipv4_dhcp":
        expected_output = expected_dhcpv4_table_multi_with_header
    else:
        expected_output = expected_ipv6_table_multi_with_header
    assert result == expected_output

def test_show_dhcp_relay_ipv4_counter_with_enabled_dhcp_server():
    with mock.patch.object(show, "is_dhcp_server_enabled", return_value=True), \
         mock.patch.object(swsscommon.ConfigDBConnector, "connect", return_value=None), \
         mock.patch.object(swsscommon.ConfigDBConnector, "get_table", return_value=None), \
         mock.patch.object(click, "echo", return_value=None) as mock_echo:
        show.ipv4_counters("TX", "Discover", "Vlan1000")
        expected_param = "Unsupport to check dhcp_relay ipv4 counter when dhcp_server feature is enabled"
        mock_echo.assert_called_once_with(expected_param)


@pytest.mark.parametrize("enable_dhcp_server", [True, False])
def test_is_dhcp_server_enabled(enable_dhcp_server):
    result = show.is_dhcp_server_enabled({"dhcp_server": {"state": "enabled" if enable_dhcp_server else "disabled"}})
    assert result == enable_dhcp_server


class MockDb(object):
    class MockCfgDb(object):
        def __init__(self, mock_cfgdb):
            self.mock_cfgdb = mock_cfgdb

        def get_table(self, table_name):
            return self.mock_cfgdb.get(table_name, {})

    def __init__(self, mock_cfgdb):
        self.cfgdb = self.MockCfgDb(mock_cfgdb)
