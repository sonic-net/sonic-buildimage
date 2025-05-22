import sys
from unittest.mock import patch, Mock, call, MagicMock
import pytest
import os
import importlib.util

from swsscommon.swsscommon import SonicV2Connector

original_import_module = importlib.import_module


@pytest.fixture(scope="module")
def patch_import_module():
    # We need to mock import module because clear_dhcp_relay.py has below import
    # dhcp6_relay = importlib.import_module('show.plugins.dhcp-relay')
    # When install current container, sonic-application-extension would move below file to destination in switch
    # Src: dockers/docker-dhcp-relay/cli/show/plugins/show_dhcp_relay.py
    # Dst: python-package-patch/show/plugins/dhcp-relay.py
    # The dst path doesn't exist in UT env, hence we need to mock it
    fake_dhcp6_relay = MagicMock()

    with patch('importlib.import_module') as mock_import:
        def side_effect(name):
            if name == 'show.plugins.dhcp-relay':
                return fake_dhcp6_relay
            return original_import_module(name)  # fallback

        mock_import.side_effect = side_effect

        clear_dhcp_relay_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "cli", "clear", "plugins",
                                                             "clear_dhcp_relay.py"))
        spec = importlib.util.spec_from_file_location("clear_dhcp_relay", clear_dhcp_relay_path)
        clear_dhcp_relay = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(clear_dhcp_relay)
        yield clear_dhcp_relay


def test_plugin_registration(patch_import_module):
    cli = MagicMock()
    clear_dhcp_relay = patch_import_module
    clear_dhcp_relay.register(cli)


@pytest.mark.parametrize("interface", [None, "Vlan1000"])
def test_clear_dhcp_relay_ipv6_counter(interface, patch_import_module):
    clear_dhcp_relay = patch_import_module
    gotten_interfaces = ["Ethernet0, Ethernet1"]

    mock_counter = MagicMock()
    clear_dhcp_relay.dhcp6_relay.DHCPv6_Counter.return_value = mock_counter
    mock_counter.get_interface.return_value = gotten_interfaces
    clear_dhcp_relay.clear_dhcp_relay_ipv6_counter(interface)
    if interface:
        mock_counter.clear_table.assert_called_once_with(interface)
    else:
        calls = [call(intf) for intf in gotten_interfaces]
        mock_counter.clear_table.assert_has_calls(calls)


def test_get_db_connector(patch_import_module):
    clear_dhcp_relay = patch_import_module
    with patch.object(SonicV2Connector, "connect") as mock_connect:
        clear_dhcp_relay.get_db_connector()
        mock_connect.assert_has_calls([
            call("COUNTERS_DB"),
            call("CONFIG_DB")
        ])


def test_clear_dhcpv4_db_counters(patch_import_module):
    clear_dhcp_relay = patch_import_module
    mock_db = MagicMock()
    with patch.object(clear_dhcp_relay, "get_db_connector", return_value=mock_db)
