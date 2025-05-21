import sys
from utilities_common.db import Db
from unittest.mock import patch, Mock, call, MagicMock
from click.testing import CliRunner

import pytest
sys.path.append("../cli/clear/plugins/")
import dhcp_relay as clear_dhcp_relay


def test_plugin_registration():
    cli = MagicMock()
    clear_dhcp_relay.register(cli)
    

@pytest.mark.parametrize("interface", [None, "Vlan1000"])
def test_clear_dhcp_relay_ipv6_counter(interface):
    gotten_interfaces = ["Ethernet0, Ethernet1"]
    with patch("clear_dhcp_relay.dhcp6_relay.DHCPv6_Counter.__init__", return_value=Mock()), \
         patch("clear_dhcp_relay.dhcp6_relay.DHCPv6_Counter.get_interface",
                    return_value=gotten_interfaces), \
             patch("clear_dhcp_relay.dhcp6_relay.DHCPv6_Counter.clear_table") as mock_clear_table:
        clear_dhcp_relay.clear_dhcp_relay_ipv6_counter(interface)
        if interface:
            mock_clear_table.assert_called_once_with(interface)
        else:
            calls = [call(intf) for intf in gotten_interfaces]
            mock_clear_table.assert_has_calls(calls)


class MockDb(object):
    class MockCfgDb(object):
        def __init__(self, mock_cfgdb):
            self.mock_cfgdb = mock_cfgdb

        def get_table(self, table_name):
            return self.mock_cfgdb.get(table_name, {})

    def __init__(self, mock_cfgdb):
        self.cfgdb = self.MockCfgDb(mock_cfgdb)
