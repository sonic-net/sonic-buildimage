import os
import sys
import traceback
from unittest import mock

from click.testing import CliRunner

from utilities_common.db import Db

import pytest

sys.path.append('../cli/config/plugins/')
import dhcp_relay

config_vlan_add_dhcp_relay_output="""\
Added DHCP relay destination addresses ['192.0.0.100'] to Vlan1000
Restarting DHCP relay service...
"""

config_vlan_add_dhcpv6_relay_output="""\
Added DHCP relay destination addresses ['fc02:2000::1'] to Vlan1000
Restarting DHCP relay service...
"""

config_vlan_add_multiple_dhcpv6_relay_output="""\
Added DHCP relay destination addresses ['fc02:2000::1', 'fc02:2000::2', 'fc02:2000::3'] to Vlan1000
Restarting DHCP relay service...
"""

config_vlan_del_dhcp_relay_output="""\
Removed DHCP relay destination addresses ('192.0.0.100',) from Vlan1000
Restarting DHCP relay service...
"""

config_vlan_del_dhcpv6_relay_output="""\
Removed DHCP relay destination addresses ('fc02:2000::1',) from Vlan1000
Restarting DHCP relay service...
"""

config_vlan_del_multiple_dhcpv6_relay_output="""\
Removed DHCP relay destination addresses ('fc02:2000::1', 'fc02:2000::2', 'fc02:2000::3') from Vlan1000
Restarting DHCP relay service...
"""

@pytest.fixture(scope="module", params=["isc", "new"])
def mode(request):
    """
    Parametrize dhcp mode

    Args:
        request: pytest request object

    Returns:
        dhcp mode needed for test case
    """
    return request.param

class TestConfigVlanDhcpRelay(object):
    def test_plugin_registration(self):
        cli = mock.MagicMock()
        dhcp_relay.register(cli)
        cli.commands['vlan'].add_command.assert_called_once_with(dhcp_relay.vlan_dhcp_relay)

    def test_config_vlan_add_dhcp_relay_with_nonexist_vlanid(self):
        runner = CliRunner()

        with mock.patch('utilities_common.cli.run_command') as mock_run_command:
            result = runner.invoke(dhcp_relay.vlan_dhcp_relay.commands["add"],
                                   ["1001", "192.0.0.100"])
            print(result.exit_code)
            print(result.output)
            # traceback.print_tb(result.exc_info[2])
            assert result.exit_code != 0
            assert "Error: Vlan1001 doesn't exist" in result.output
            assert mock_run_command.call_count == 0

    def test_config_vlan_add_dhcp_relay_with_invalid_vlanid(self):
        runner = CliRunner()

        with mock.patch('utilities_common.cli.run_command') as mock_run_command:
            result = runner.invoke(dhcp_relay.vlan_dhcp_relay.commands["add"],
                                   ["4096", "192.0.0.100"])
            print(result.exit_code)
            print(result.output)
            # traceback.print_tb(result.exc_info[2])
            assert result.exit_code != 0
            assert "Error: Vlan4096 doesn't exist" in result.output
            assert mock_run_command.call_count == 0

    def test_config_vlan_add_dhcp_relay_with_invalid_ip(self, mock_cfgdb):
        runner = CliRunner()
        db = Db()
        db.cfgdb = mock_cfgdb

        with mock.patch('utilities_common.cli.run_command') as mock_run_command:
            result = runner.invoke(dhcp_relay.vlan_dhcp_relay.commands["add"],
                                   ["1000", "192.0.0.1000"], obj=db)
            print(result.exit_code)
            print(result.output)
            # traceback.print_tb(result.exc_info[2])
            assert result.exit_code != 0
            assert "Error: 192.0.0.1000 is invalid IP address" in result.output
            assert mock_run_command.call_count == 0

            result = runner.invoke(dhcp_relay.vlan_dhcp_relay.commands["add"],
                                   ["1000", "192.0.0."], obj=db)
            print(result.exit_code)
            print(result.output)
            assert result.exit_code != 0
            assert "Error: 192.0.0. is invalid IP address" in result.output
            assert mock_run_command.call_count == 0

    def test_config_vlan_add_dhcp_relay_with_exist_ip(self, mock_cfgdb):
        runner = CliRunner()
        db = Db()
        db.cfgdb = mock_cfgdb

        with mock.patch('utilities_common.cli.run_command') as mock_run_command:
            result = runner.invoke(dhcp_relay.vlan_dhcp_relay.commands["add"],
                                   ["1000", "192.0.0.1"], obj=db)
            print(result.exit_code)
            print(result.output)
            assert result.exit_code == 0
            assert "192.0.0.1 is already a DHCP relay destination for Vlan1000" in result.output
            assert mock_run_command.call_count == 0

    def test_config_vlan_add_del_dhcp_relay_dest(self, mock_cfgdb, mode):
        runner = CliRunner()
        db = Db()
        db.cfgdb = mock_cfgdb
        if mode == "isc":
           db.cfgdb.set_entry('DEVICE_METADATA', 'localhost', {'has_sonic_dhcpv4_relay': 'False'})
        elif mode == "new":
            db.cfgdb.set_entry('DEVICE_METADATA', 'localhost', {'has_sonic_dhcpv4_relay': 'True'})
        # add new relay dest
        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            result = runner.invoke(dhcp_relay.vlan_dhcp_relay.commands["add"],
                                   ["1000", "192.0.0.100"], obj=db)
            print(result.exit_code)
            print(result.output)
            assert result.exit_code == 0
            if mode == "isc":
                assert result.output == config_vlan_add_dhcp_relay_output
                assert mock_run_command.call_count == 3
                expected_calls = [
                    mock.call('DEVICE_METADATA', 'localhost', {'has_sonic_dhcpv4_relay': 'False'}),
                    mock.call('VLAN', 'Vlan1000', {'dhcp_servers': ['192.0.0.1', '192.0.0.100']})
                ]
                db.cfgdb.set_entry.assert_has_calls(expected_calls)
            elif mode == "new":
                assert "Added DHCP relay destination addresses ['192.0.0.100'] to Vlan1000" in result.output
                expected_calls = [
                    mock.call('DEVICE_METADATA', 'localhost', {'has_sonic_dhcpv4_relay': 'True'}),
                    #mock.call('VLAN', 'Vlan1000', {'dhcp_servers': ['192.0.0.1', '192.0.0.100']}),
                    mock.call('DHCPV4_RELAY', 'Vlan1000', {'dhcpv4_servers': ['192.0.0.1', '192.0.0.100']})
                ]
                db.cfgdb.set_entry.assert_has_calls(expected_calls)

        db.cfgdb.set_entry.reset_mock()
        if mode == "isc":
           db.cfgdb.set_entry('DEVICE_METADATA', 'localhost', {'has_sonic_dhcpv4_relay': 'False'})
        elif mode == "new":
            db.cfgdb.set_entry('DEVICE_METADATA', 'localhost', {'has_sonic_dhcpv4_relay': 'True'})

        # del relay dest
        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            result = runner.invoke(dhcp_relay.vlan_dhcp_relay.commands["del"],
                                   ["1000", "192.0.0.100"], obj=db)
            print(result.exit_code)
            print(result.output)
            assert result.exit_code == 0
            if mode == "isc":
                assert result.output == config_vlan_del_dhcp_relay_output
                assert mock_run_command.call_count == 3
                expected_calls = [
                    mock.call('DEVICE_METADATA', 'localhost', {'has_sonic_dhcpv4_relay': 'False'}),
                    mock.call('VLAN', 'Vlan1000', {'dhcp_servers': ['192.0.0.1']})
                ]
                db.cfgdb.set_entry.assert_has_calls(expected_calls)
            elif mode == "new":
                assert "Removed DHCP relay destination addresses ('192.0.0.100',) from Vlan1000" in result.output
                expected_calls = [
                    mock.call('DEVICE_METADATA', 'localhost', {'has_sonic_dhcpv4_relay': 'True'}),
                    #mock.call(('VLAN', 'Vlan1000', {'dhcp_servers': ['192.0.0.1']}),
                    mock.call('DHCPV4_RELAY', 'Vlan1000', {'dhcpv4_servers': ['192.0.0.1']})
                ]
                db.cfgdb.set_entry.assert_has_calls(expected_calls)

    def test_config_vlan_add_del_dhcpv6_relay_dest(self, mock_cfgdb):
        runner = CliRunner()
        db = Db()
        db.cfgdb = mock_cfgdb

        # add new relay dest
        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            result = runner.invoke(dhcp_relay.vlan_dhcp_relay.commands["add"],
                                   ["1000", "fc02:2000::1"], obj=db)
            print(result.exit_code)
            print(result.output)
            assert result.exit_code == 0
            assert result.output == config_vlan_add_dhcpv6_relay_output
            assert mock_run_command.call_count == 3
            db.cfgdb.set_entry.assert_called_once_with('VLAN', 'Vlan1000', {'dhcp_servers': ['192.0.0.1'], 'dhcpv6_servers': ['fc02:2000::1']})

        db.cfgdb.set_entry.reset_mock()

        # del relay dest
        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            result = runner.invoke(dhcp_relay.vlan_dhcp_relay.commands["del"],
                                   ["1000", "fc02:2000::1"], obj=db)
            print(result.exit_code)
            print(result.output)
            assert result.exit_code == 0
            assert result.output == config_vlan_del_dhcpv6_relay_output
            assert mock_run_command.call_count == 3
            db.cfgdb.set_entry.assert_called_once_with('VLAN', 'Vlan1000', {'dhcp_servers': ['192.0.0.1']})

    def test_config_vlan_add_del_multiple_dhcpv6_relay_dest(self, mock_cfgdb):
        runner = CliRunner()
        db = Db()
        db.cfgdb = mock_cfgdb

        # add new relay dest
        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            result = runner.invoke(dhcp_relay.vlan_dhcp_relay.commands["add"],
                                   ["1000", "fc02:2000::1", "fc02:2000::2", "fc02:2000::3"], obj=db)
            print(result.exit_code)
            print(result.output)
            assert result.exit_code == 0
            assert result.output == config_vlan_add_multiple_dhcpv6_relay_output
            assert mock_run_command.call_count == 3
            db.cfgdb.set_entry.assert_called_once_with('VLAN', 'Vlan1000', {'dhcp_servers': ['192.0.0.1'], 'dhcpv6_servers': ['fc02:2000::1', 'fc02:2000::2', 'fc02:2000::3']})

        db.cfgdb.set_entry.reset_mock()

        # del relay dest
        with mock.patch("utilities_common.cli.run_command") as mock_run_command:
            result = runner.invoke(dhcp_relay.vlan_dhcp_relay.commands["del"],
                                   ["1000", "fc02:2000::1", "fc02:2000::2", "fc02:2000::3"], obj=db)
            print(result.exit_code)
            print(result.output)
            assert result.exit_code == 0
            assert result.output == config_vlan_del_multiple_dhcpv6_relay_output
            assert mock_run_command.call_count == 3
            db.cfgdb.set_entry.assert_called_once_with('VLAN', 'Vlan1000', {'dhcp_servers': ['192.0.0.1']})

    def test_config_vlan_remove_nonexist_dhcp_relay_dest(self, mock_cfgdb):
        runner = CliRunner()
        db = Db()
        db.cfgdb = mock_cfgdb

        with mock.patch('utilities_common.cli.run_command') as mock_run_command:
            result = runner.invoke(dhcp_relay.vlan_dhcp_relay.commands["del"],
                                   ["1000", "192.0.0.100"], obj=db)
            print(result.exit_code)
            print(result.output)
            # traceback.print_tb(result.exc_info[2])
            assert result.exit_code != 0
            assert "Error: 192.0.0.100 is not a DHCP relay destination for Vlan1000" in result.output
            assert mock_run_command.call_count == 0

    def test_config_vlan_remove_dhcp_relay_dest_with_nonexist_vlanid(self, mock_cfgdb):
        runner = CliRunner()
        db = Db()
        db.cfgdb = mock_cfgdb

        with mock.patch('utilities_common.cli.run_command') as mock_run_command:
            result = runner.invoke(dhcp_relay.vlan_dhcp_relay.commands["del"],
                                   ["1001", "192.0.0.1"], obj=Db)
            print(result.exit_code)
            print(result.output)
            # traceback.print_tb(result.exc_info[2])
            assert result.exit_code != 0
            assert "Error: Vlan1001 doesn't exist" in result.output
            assert mock_run_command.call_count == 0
