import os
import sys
import traceback
from unittest import mock

from click.testing import CliRunner

from utilities_common.db import Db
import pytest
sys.path.append('/usr/local/lib/python3.11/dist-packages/config/plugins')
import dhcp_relay

config_dhcpv4_relay_add_output = """\
Added DHCPv4 Servers as 3.3.3.3 to Vlan200
"""

config_dhcpv4_relay_update_output = """\
Updated DHCPv4 Servers as 4.4.4.4 to Vlan200
"""

config_dhcpv4_relay_del_output = """\
Removed DHCPv4 relay configuration for Vlan200
"""

config_dhcpv4_relay_add_multiple_ips_output = """\
Added DHCPv4 Servers as {initial_servers} to Vlan200
"""

config_dhcpv4_relay_update_multiple_ips_output = """\
Updated DHCPv4 Servers as {updated_servers} to Vlan200
"""

config_dhcpv4_relay_partial_del_output = """\
Removed DHCPv4 relay configuration from Vlan200 for Servers [{partial_servers}]
"""

config_dhcpv4_relay_add_source_interface_output = """\
Added DHCPv4 Servers as 3.3.3.3, Source Interface as {interface} to Vlan200
"""

config_dhcpv4_relay_update_source_interface_output = """\
Updated Source Interface as {updated_interface} to Vlan200
"""

config_dhcpv4_relay_add_server_vrf_output = """\
Added DHCPv4 Servers as 3.3.3.3, Server VRF as VrfRED, link_selection as enable, vrf_selection as enable, server_id_override as enable to Vlan200
"""

config_dhcpv4_relay_update_server_vrf_output = """\
Updated DHCPv4 Servers as 4.4.4.4, Server VRF as VrfBLUE to Vlan200
"""

config_dhcpv4_relay_add_server_vrf_default_output = """\
Added DHCPv4 Servers as 3.3.3.3, Server VRF as default, link_selection as enable, vrf_selection as enable, server_id_override as enable to Vlan200
"""

config_dhcpv4_relay_update_server_vrf_default_output = """\
Updated Server VRF as {server_vrf} to Vlan200
"""

config_dhcpv4_relay_en_selection_flag_output = """\
Added DHCPv4 Servers as 3.3.3.3, {flag} as {value} to {vlan}
"""

config_dhcpv4_relay_upd_selection_flag_output = """\
Updated {flag} as {value} to {vlan}
"""

config_dhcpv4_relay_add_agent_relay_mode_output = """\
Added DHCPv4 Servers as 3.3.3.3, Agent Relay Mode as {mode} to Vlan200
"""

config_dhcpv4_relay_update_agent_relay_mode_output = """\
Updated Agent Relay Mode as {mode} to Vlan200
"""

config_dhcpv4_relay_add_max_hop_count_output = """\
Added DHCPv4 Servers as 3.3.3.3, Max Hop Count as 1 to Vlan200
"""

config_dhcpv4_relay_update_max_hop_count_output = """\
Updated Max Hop Count as {count} to Vlan200
"""

config_dhcpv4_relay_add_all_option_output = """\
Added DHCPv4 Servers as 3.3.3.3, Source Interface as Ethernet8, Server VRF as VrfRED, \
link_selection as enable, vrf_selection as enable, server_id_override as enable, \
Agent Relay Mode as discard, Max Hop Count as 8 to Vlan200
"""

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
            assert "Error: DHCPv4 relay configuration not found for Vlan Vlan300" in result.output
            assert mock_run_command.call_count == 0

        db.cfgdb.set_entry.reset_mock()

    def test_config_dhcpv4_relay_update_nonexistent_vlan(self, mock_cfgdb):
        """Updating DHCPv4 Relay config for a non-existent VLAN"""
        runner = CliRunner()
        db = Db()
        db.cfgdb = mock_cfgdb

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
            assert result.output == config_dhcpv4_relay_add_multiple_ips_output.format(initial_servers=initial_servers)
            assert mock_run_command.call_count == 0

        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            # Update the dhcpv4-servers with another set of IPs via 'updated_servers'
            result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["update"],
                                   ["--dhcpv4-servers", updated_servers, "Vlan200"], obj=db)
            assert result.exit_code == 0
            assert result.output == config_dhcpv4_relay_update_multiple_ips_output.format(updated_servers=updated_servers)
            assert mock_run_command.call_count == 0

        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["del"], ["Vlan200"], obj=db)
            assert result.exit_code == 0
            assert result.output == config_dhcpv4_relay_del_output
            assert mock_run_command.call_count == 0

        db.cfgdb.set_entry.reset_mock()

    def test_config_dhcpv4_multiple_servers_delete(self, mock_cfgdb):
        """Validate partial deletion and complete cleanup of multiple DHCPv4 servers"""
        runner = CliRunner()
        db = Db()
        db.cfgdb = mock_cfgdb

        initial_servers = "100.100.100.1,100.100.100.2,100.100.100.3,100.100.100.4,100.100.100.5"
        delete_1 = "100.100.100.4,100.100.100.5"
        delete_2 = "100.100.100.1,100.100.100.2,100.100.100.3"

        # Add multiple DHCPv4 servers
        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["add"],
                               ["--dhcpv4-servers", initial_servers, "Vlan200"], obj=db)
            print(result.output)
            assert result.exit_code == 0
            assert result.output == config_dhcpv4_relay_add_multiple_ips_output.format(initial_servers=initial_servers)
            assert mock_run_command.call_count == 0
        # Partially delete some of the servers
        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            mock_cfgdb.get_entry.return_value = {"dhcpv4_servers": initial_servers}
            result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["del"],
                               ["--dhcpv4-servers", delete_1, "Vlan200"], obj=db)
            print(result.output)
            assert result.exit_code == 0
            assert result.output == config_dhcpv4_relay_partial_del_output.format(partial_servers=delete_1)
            assert mock_run_command.call_count == 0
        # Delete the remaining servers, should clean up the full entry
        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["del"],
                                   ["--dhcpv4-servers", delete_2, "Vlan200"], obj=db)
            print(result.output)
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
            ("Loopback2", "Loopback3"),
            ("Ethernet8", "Ethernet12"),
            ("PortChannel5", "PortChannel6")
        ]
        
        for interface, updated_interface in interfaces:

            with mock.patch("utilities_common.cli.run_command") as mock_run_command:
                result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["add"],
                                       ["--dhcpv4-servers", "3.3.3.3", "--source-interface", interface, "Vlan200"], obj=db)
                assert result.exit_code == 0
                assert result.output == config_dhcpv4_relay_add_source_interface_output.format(interface=interface)
                assert mock_run_command.call_count == 0

            with mock.patch("utilities_common.cli.run_command") as mock_run_command:
                result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["update"],
                                       ["--source-interface", updated_interface, "Vlan200"], obj=db)
                assert result.exit_code == 0
                assert result.output == config_dhcpv4_relay_update_source_interface_output.format(updated_interface=updated_interface)
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
            result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["update"],
                [
                    "--dhcpv4-servers", "4.4.4.4",
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

    def test_config_dhcpv4_relay_default_server_vrf(self, mock_cfgdb):
        """Validating default server VRF addition, update to VrfRED, update back to default, then delete"""
        runner = CliRunner()
        db = Db()
        db.cfgdb = mock_cfgdb

        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            # Add with default VRF
            result = runner.invoke(
                dhcp_relay.dhcpv4_relay.commands["add"],
                [
                    "--dhcpv4-servers", "3.3.3.3",
                    "--link-selection", "enable",
                    "--vrf-selection", "enable",
                    "--server-id-override", "enable",
                    "--server-vrf", "default",
                    "Vlan200"
                ],
                obj=db
            )
            assert result.exit_code == 0
            assert result.output == config_dhcpv4_relay_add_server_vrf_default_output
            assert mock_run_command.call_count == 0

        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            # Update to VrfRED
            result = runner.invoke(
                dhcp_relay.dhcpv4_relay.commands["update"],
                [
                    "--server-vrf", "VrfRED",
                    "Vlan200"
                ],
                obj=db
            )
            assert result.exit_code == 0
            assert result.output == config_dhcpv4_relay_update_server_vrf_default_output.format(server_vrf="VrfRED")
            assert mock_run_command.call_count == 0

        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            # Update back to default
            result = runner.invoke(
                dhcp_relay.dhcpv4_relay.commands["update"],
                [
                    "--server-vrf", "default",
                    "Vlan200"
                ],
                obj=db
            )
            assert result.exit_code == 0
            assert result.output == config_dhcpv4_relay_update_server_vrf_default_output.format(server_vrf="default")
            assert mock_run_command.call_count == 0

        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            # Final delete
            result = runner.invoke(
                dhcp_relay.dhcpv4_relay.commands["del"],
                ["Vlan200"],
                obj=db
            )
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

        flag_map = {
            "link-selection": "link_selection",
            "vrf-selection": "vrf_selection",
            "server-id-override": "server_id_override"
        }
        for flag, config_key in flag_map.items():
            vlan, add_val, update_val = ("Vlan200", "enable", "disable")

            with mock.patch("utilities_common.cli.run_command") as mock_run_command:
                result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["add"],
                                       ["--dhcpv4-servers", "3.3.3.3", f"--{flag}", add_val, vlan],
                                       obj=db)
                assert result.exit_code == 0
                assert result.output == config_dhcpv4_relay_en_selection_flag_output.format(flag=config_key, value=add_val, vlan=vlan)
                assert mock_run_command.call_count == 0

            with mock.patch("utilities_common.cli.run_command") as mock_run_command:
                result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["update"],
                                       [f"--{flag}", update_val, vlan],
                                       obj=db)
                assert result.exit_code == 0
                assert result.output == config_dhcpv4_relay_upd_selection_flag_output.format(flag=config_key, value=update_val, vlan=vlan)
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
            "append",
            "replace"
        ]

        for add_mode in agent_relay_modes:
            with mock.patch("utilities_common.cli.run_command") as mock_run_command:
                result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["add"],
                                       ["--dhcpv4-servers", "3.3.3.3", "--agent-relay-mode", add_mode, "Vlan200"],
                                       obj=db)
                assert result.exit_code == 0
                assert result.output == config_dhcpv4_relay_add_agent_relay_mode_output.format(mode=add_mode)
                assert mock_run_command.call_count == 0

            update_modes = [mode for mode in agent_relay_modes if mode != add_mode]

            for update_mode in update_modes:
                with mock.patch("utilities_common.cli.run_command") as mock_run_command:
                    result = runner.invoke(dhcp_relay.dhcpv4_relay.commands["update"],
                                           ["--agent-relay-mode", update_mode, "Vlan200"],
                                           obj=db)
                    assert result.exit_code == 0
                    assert result.output == config_dhcpv4_relay_update_agent_relay_mode_output.format(mode=update_mode)
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
                assert result.output == config_dhcpv4_relay_update_max_hop_count_output.format(count=count)
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
