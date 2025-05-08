import os
import sys
import traceback
from unittest import mock

from click.testing import CliRunner

from utilities_common.db import Db

import pytest
#sys.path.append('../cli/config/plugins/')
sys.path.append('/usr/local/lib/python3.11/dist-packages/config/plugins')
import dhcp_relay

config_dhcpv4_relay_add_output = """\
Added DHCPv4 relay configuration for Vlan200
"""

config_dhcpv4_relay_update_output = """\
Updated DHCPv4 Relay Configuration to Vlan200
"""

config_dhcpv4_relay_del_output = """\
Removed DHCPv4 relay configuration from Vlan200
"""

config_dhcpv4_relay_add_source_interface_output = """\
Added Source Interface to Vlan200
"""

config_dhcpv4_relay_update_source_interface_output = """\
Updated Source Interface Configuration to Vlan200
"""

config_dhcpv4_relay_add_server_vrf_output = """\
Added Server VRF VrfRED to Vlan200
"""

config_dhcpv4_relay_update_server_vrf_output = """\
Updated Server VRF VrfBLUE to Vlan200
"""

config_dhcpv4_relay_en_selection_flag_output = """\
Added {flag} flag configuration as {value} to {vlan}
"""

config_dhcpv4_relay_upd_selection_flag_output = """\
Updated {flag} flag configuration to {value} to {vlan}
"""

config_dhcpv4_relay_add_agent_relay_mode_output = """\
Added Agent Relay Mode to Vlan200
"""

config_dhcpv4_relay_update_agent_relay_mode_output = """\
Updated Agent Relay Mode to Vlan200
"""

config_dhcpv4_relay_add_max_hop_count_output = """\
Added Max Hop Count as 1 to Vlan200
"""

config_dhcpv4_relay_update_max_hop_count_output = """\
Updated Max Hop Count to Vlan200
"""

config_dhcpv4_relay_add_all_option_output = """\
Added Source Interface, Server VRF VrfRED, agent_relay_mode and max_hop_count to Vlan200
"""

@pytest.fixture
def mock_cfgdb():
    """Mock the Config DB for testing."""
    mock_db = mock.MagicMock()
    return mock_db


class TestConfigDhcpv4Relay(object):
    def test_plugin_registration(self):
        cli = mock.MagicMock()
        dhcp_relay.register(cli)
        cli.commands['dhcpv4_relay'].add_command(dhcp_relay.dhcpv4_relay)

    def test_config_dhcpv4_relay_del_nonexistent_relay(self, mock_cfgdb):
        """Deleting Non Existent Vlan from DHCPv4 Relay Config"""
        runner = CliRunner()
        db = Db()
        db.cfgdb = mock_cfgdb
        mock_cfgdb.get_entry.return_value = {}

        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["del"], ["Vlan300"], obj=db)
            assert result.exit_code != 0
            assert "Error: Vlan Vlan300 does not exist in the configDB" in result.output
            assert mock_run_command.call_count == 0

        db.cfgdb.set_entry.reset_mock()

    def test_config_dhcpv4_relay_update_nonexistent_vlan(self, mock_cfgdb):
        """Updating DHCPv4 Relay config for a non-existent VLAN"""
        runner = CliRunner()
        db = Db()
        db.cfgdb = mock_cfgdb
        mock_cfgdb.get_entry.return_value = {}

        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            result = runner.invoke(
                dhcp_relay.dhcpv4_relay.commands["update"],
                ["--dhcpv4-servers", "1.1.1.1", "--source-interface", "Ethernet4", "Vlan786"],
                obj=db
            )
            assert result.exit_code != 0
            assert "Error: Vlan Vlan786 does not exist in the configDB" in result.output
            assert mock_run_command.call_count == 0

        db.cfgdb.set_entry.reset_mock()


    def test_config_dhcpv4_relay_invalid_source_interface(self, mock_cfgdb):
        """Validating error when source interface is not a valid Ethernet, PortChannel, or Loopback interface"""
        runner = CliRunner()
        db = Db()
        db.cfgdb = mock_cfgdb

        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            result = runner.invoke(
                dhcp_relay.dhcpv4_relay.commands["add"],
                [
                    "--dhcpv4-servers", "3.3.3.3",
                    "--source-interface", "InvalidIntf123",
                    "Vlan200"
                ],
                obj=db
            )
            assert result.exit_code != 0
            assert "Error: InvalidIntf123 is not a valid Ethernet, PortChannel, or Loopback interface." in result.output
            assert mock_run_command.call_count == 0

        db.cfgdb.set_entry.reset_mock()

    def test_config_dhcpv4_relay_invalid_server_vrf(self, mock_cfgdb):
        """Adding a Nonexistent VRF to DHCPv4 Relay Config"""
        runner = CliRunner()
        db = Db()
        db.cfgdb = mock_cfgdb

        # Ensure the invalid VRF does not exist in the VRF table
        db.cfgdb.get_entry.return_value = {}

        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            result = runner.invoke(
                dhcp_relay.dhcpv4_relay.commands["add"],
                [
                    "--dhcpv4-servers", "3.3.3.3",
                    "--link-selection", "enable",
                    "--vrf-selection", "enable",
                    "--server-id-override", "enable",
                    "--server-vrf", "Vrf99",
                    "Vlan200"
                ],
                obj=db
            )
            assert result.exit_code != 0  
            assert "Error: VRF Vrf99 does not exist in the VRF table." in result.output
            assert mock_run_command.call_count == 0

        db.cfgdb.set_entry.reset_mock()

    def test_config_dhcpv4_basic_relay(self, mock_cfgdb):
        """Validating dhcpv4-servers in DHCPv4 Relay Config"""
        runner = CliRunner()
        db = Db()
        db.cfgdb = mock_cfgdb

        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["add"],
                                   ["--dhcpv4-servers", "3.3.3.3", "Vlan200"], obj=db)
            assert result.exit_code == 0
            assert result.output == config_dhcpv4_relay_add_output
            assert mock_run_command.call_count == 0

        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["update"],
                                   ["--dhcpv4-servers", "4.4.4.4", "Vlan200"], obj=db)
            assert result.output == config_dhcpv4_relay_update_output
            assert result.exit_code == 0
            assert mock_run_command.call_count == 0

        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["del"], ["Vlan200"], obj=db)
            assert result.exit_code == 0
            assert result.output == config_dhcpv4_relay_del_output
            assert mock_run_command.call_count == 0

        db.cfgdb.set_entry.reset_mock()

    def test_config_dhcpv4_multiple_servers_relay(self, mock_cfgdb):
        """Validating multiple dhcpv4-servers in DHCPv4 Relay Config (Add, Update, Delete)"""
        runner = CliRunner()
        db = Db()
        db.cfgdb = mock_cfgdb

        initial_servers = "1.1.1.1,2.2.2.2,3.3.3.3,4.4.4.4,5.5.5.5"
        updated_servers = "6.6.6.6,7.7.7.7,8.8.8.8,9.9.9.9,10.10.10.10"

        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            # Adding multiple dhcpv4-servers to dhcpv4 relay via 'initial_servers'
            result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["add"],
                                   ["--dhcpv4-servers", initial_servers, "Vlan200"], obj=db)
            assert result.exit_code == 0
            assert result.output == config_dhcpv4_relay_add_output
            assert mock_run_command.call_count == 0

        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            # Update the dhcpv4-servers with another set of IPs via 'updated_servers'
            result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["update"],
                                   ["--dhcpv4-servers", updated_servers, "Vlan200"], obj=db)
            assert result.exit_code == 0
            assert result.output == config_dhcpv4_relay_update_output
            assert mock_run_command.call_count == 0

        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            # Delete
            result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["del"], ["Vlan200"], obj=db)
            assert result.exit_code == 0
            assert result.output == config_dhcpv4_relay_del_output
            assert mock_run_command.call_count == 0

        db.cfgdb.set_entry.reset_mock()

    def test_config_dhcpv4_relay_source_interface(self, mock_cfgdb):
        """Validating source interfaces in DHCPv4 Relay Config"""
        runner = CliRunner()
        db = Db()
        db.cfgdb = mock_cfgdb
        
        interfaces = [
            ("Loopback2", "LOOPBACK_INTERFACE", "Loopback3"),
            ("Ethernet8", "INTERFACE", "Ethernet12"),
            ("PortChannel2", "PORTCHANNEL", "PortChannel3")
        ]

        for interface, table, updated_interface in interfaces:
            # Adding "interface" and "updated_interface" to global "table" for add and update case
            runner.invoke(dhcp_relay.dhcpv4_relay.commands["add"], ["config", table.lower(), "add", interface], obj=db)
            runner.invoke(dhcp_relay.dhcpv4_relay.commands["add"], ["config", table.lower(), "add", updated_interface], obj=db)

            #Adding "interface" as source interface
            with mock.patch("utilities_common.cli.run_command") as mock_run_command:
                result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["add"],
                                       ["--dhcpv4-servers", "3.3.3.3", "--source-interface", interface, "Vlan200"], obj=db)
                assert result.exit_code == 0
                assert result.output == config_dhcpv4_relay_add_source_interface_output
                assert mock_run_command.call_count == 0

            # Updating source interface from "interface" to "updated_interface
            with mock.patch("utilities_common.cli.run_command") as mock_run_command:
                result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["update"],
                                       ["--dhcpv4-servers", "3.3.3.3", "--source-interface", updated_interface, "Vlan200"], obj=db)
                assert result.exit_code == 0
                assert result.output == config_dhcpv4_relay_update_source_interface_output
                assert mock_run_command.call_count == 0

            with mock.patch("utilities_common.cli.run_command") as mock_run_command:
                result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["del"], ["Vlan200"], obj=db)
                assert result.exit_code == 0
                assert result.output == config_dhcpv4_relay_del_output
                assert mock_run_command.call_count == 0

        db.cfgdb.set_entry.reset_mock()

    def test_config_dhcpv4_relay_server_vrf(self, mock_cfgdb):
        """Validating server vrf in DHCPv4 Relay Config"""
        runner = CliRunner()
        db = Db()
        db.cfgdb = mock_cfgdb

        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            result = runner.invoke(
                dhcp_relay.dhcpv4_relay.commands["add"],
                [
                    "--dhcpv4-servers", "3.3.3.3",
                    "--link-selection", "enable",
                    "--vrf-selection", "enable",
                    "--server-id-override", "enable",
                    "--server-vrf", "VrfRED",
                    "Vlan200"
                ],
                obj=db
            )
            assert result.exit_code == 0
            assert result.output == config_dhcpv4_relay_add_server_vrf_output
            assert mock_run_command.call_count == 0

        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            result = runner.invoke(
                dhcp_relay.dhcpv4_relay.commands["update"],
                [
                    "--dhcpv4-servers", "3.3.3.3",
                    "--link-selection", "enable",
                    "--vrf-selection", "enable",
                    "--server-id-override", "enable",
                    "--server-vrf", "VrfBLUE",
                    "Vlan200"
                ],
                obj=db
            )
            assert result.exit_code == 0
            assert result.output == config_dhcpv4_relay_update_server_vrf_output
            assert mock_run_command.call_count == 0

        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["del"], ["Vlan200"], obj=db)
            assert result.exit_code == 0
            assert result.output == config_dhcpv4_relay_del_output
            assert mock_run_command.call_count == 0

        db.cfgdb.set_entry.reset_mock()

    def test_config_dhcpv4_relay_server_vrf_link_selection_disabled(self, mock_cfgdb):
        """Test error when --link-selection is disabled but --server-vrf is passed"""
        runner = CliRunner()
        db = Db()
        db.cfgdb = mock_cfgdb

        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            result = runner.invoke(
                dhcp_relay.dhcpv4_relay.commands["add"],
                [
                    "--dhcpv4-servers", "3.3.3.3",
                    "--link-selection", "disable",
                    "--vrf-selection", "enable",
                    "--server-id-override", "enable",
                    "--server-vrf", "VrfRED",
                    "Vlan200"
                ],
                obj=db
            )
            assert result.exit_code != 0
            assert "Error: server-vrf requires link-selection, vrf-selection and server-id-override flags to be enabled." in result.output
            assert mock_run_command.call_count == 0

        db.cfgdb.ser_entry.reset_mock()

    def test_config_dhcpv4_relay_selection_flags(self, mock_cfgdb):
        """Validating selection flags in DHCPv4 relay configs"""
        runner = CliRunner()
        db = Db()
        db.cfgdb = mock_cfgdb

        flags = ["link-selection", "vrf-selection", "server-id-override"]
        test_cases = [
            ("Vlan200", "enable", "disable"),
            ("Vlan200", "enable", "disable"),
            ("Vlan200", "enable", "disable")
        ]
        for i, flag in enumerate(flags):
            vlan, add_val, update_val = test_cases[i]

            # For add case, we will loop through the three flags to enable them
            with mock.patch("utilities_common.cli.run_command") as mock_run_command:
                result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["add"],
                                       ["--dhcpv4-servers", "3.3.3.3", f"--{flag}", add_val, vlan],
                                       obj=db)
                assert result.exit_code == 0
                assert result.output == config_dhcpv4_relay_en_selection_flag_output.format(flag=flag, value=add_val, vlan=vlan)
                assert mock_run_command.call_count == 0

            #For update case, we will loop through the three flags to disable them
            with mock.patch("utilities_common.cli.run_command") as mock_run_command:
                result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["update"],
                                       [f"--{flag}", update_val, vlan],
                                       obj=db)
                assert result.exit_code == 0
                assert result.output == config_dhcpv4_relay_upd_selection_flag_output.format(flag=flag, value=update_val, vlan=vlan)
                assert mock_run_command.call_count == 0

            with mock.patch("utilities_common.cli.run_command") as mock_run_command:
                result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["del"], [vlan], obj=db)
                assert result.exit_code == 0
                assert result.output == config_dhcpv4_relay_del_output
                assert mock_run_command.call_count == 0

            db.cfgdb.set_entry.reset_mock()

    def test_config_dhcpv4_relay_agent_relay_mode(self, mock_cfgdb):
        """Validating Agent Relay Modes in DHCPv4 Relay Config"""
        runner = CliRunner()
        db = Db()
        db.cfgdb = mock_cfgdb

        agent_relay_modes = [
            "discard",
            "forward_and_append",
            "forward_and_replace",
            "forward_untouched"
        ]

        for add_mode in agent_relay_modes:
            with mock.patch("utilities_common.cli.run_command") as mock_run_command:
                result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["add"],
                                       ["--dhcpv4-servers", "3.3.3.3", "--agent-relay-mode", add_mode, "Vlan200"],
                                       obj=db)
                assert result.exit_code == 0
                assert result.output == config_dhcpv4_relay_add_agent_relay_mode_output
                assert mock_run_command.call_count == 0

            update_modes = [mode for mode in agent_relay_modes if mode != add_mode]

            for update_mode in update_modes:
                with mock.patch("utilities_common.cli.run_command") as mock_run_command:
                    result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["update"],
                                           ["--agent-relay-mode", update_mode, "Vlan200"],
                                           obj=db)
                    assert result.exit_code == 0
                    assert result.output == config_dhcpv4_relay_update_agent_relay_mode_output
                    assert mock_run_command.call_count == 0

            with mock.patch("utilities_common.cli.run_command") as mock_run_command:
                result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["del"], ["Vlan200"], obj=db)
                assert result.exit_code == 0
                assert result.output == config_dhcpv4_relay_del_output
                assert mock_run_command.call_count == 0

        db.cfgdb.set_entry.reset_mock()

    def test_config_dhcpv4_relay_max_hop_count(self, mock_cfgdb):
        """Validating Max Hop Count in DHCPv4 Relay Config"""
        runner = CliRunner()
        db = Db()
        db.cfgdb = mock_cfgdb

        #For add case, testing the min value
        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["add"],
                                   ["--dhcpv4-servers", "3.3.3.3", "--max-hop-count", '1', "Vlan200"],
                                   obj=db)
            assert result.exit_code == 0
            assert result.output == config_dhcpv4_relay_add_max_hop_count_output
            assert mock_run_command.call_count == 0

        # For update case, testing default, mid and max values
        max_hop_counts = ["4", "8", "16"]
        for count in max_hop_counts:
            with mock.patch("utilities_common.cli.run_command") as mock_run_command:
                result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["update"],
                                       ["--max-hop-count", count, "Vlan200"],
                                       obj=db)
                assert result.exit_code == 0
                assert result.output == config_dhcpv4_relay_update_max_hop_count_output
                assert mock_run_command.call_count == 0

        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["del"], ["Vlan200"], obj=db)
            assert result.exit_code == 0
            assert result.output == config_dhcpv4_relay_del_output
            assert mock_run_command.call_count == 0

        db.cfgdb.set_entry.reset_mock()

    def test_config_dhcpv4_relay_add_and_delete(self, mock_cfgdb):
        runner = CliRunner()
        db = Db()
        db.cfgdb = mock_cfgdb

        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["add"],
                                   ["--dhcpv4-servers", "3.3.3.3",
                                    "--vrf-selection", "enable",
                                    "--server-id-override", "enable",
                                    "--source-interface", "Ethernet8",
                                    "--link-selection", "enable",
                                    "--agent-relay-mode", "discard",
                                    "--max-hop-count", "8",
                                    "--server-vrf", "VrfRED",
                                    "Vlan200"],
                                   obj=db)
            assert result.exit_code == 0
            assert result.output == config_dhcpv4_relay_add_all_option_output
            assert mock_run_command.call_count == 0

        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["del"], ["Vlan200"], obj=db)
            assert result.exit_code == 0
            assert result.output == config_dhcpv4_relay_del_output
            assert mock_run_command.call_count == 0

        db.cfgdb.set_entry.reset_mock()
