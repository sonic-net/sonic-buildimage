import sys
from unittest import mock

from click.testing import CliRunner

import utilities_common.cli as clicommon

sys.path.append('../cli/show/plugins/')
import show_dhcp_server


class TestShowDHCPServer(object):
    def test_plugin_registration(self):
        cli = mock.MagicMock()
        show_dhcp_server.register(cli)

    def test_show_dhcp_server_ipv4_lease_without_dhcpintf(self, mock_db):
        expected_stdout = """\
Interface            MAC Address        IP           Lease Start          Lease End
-------------------  -----------------  -----------  -------------------  -------------------
Vlan1000|Ethernet10  10:70:fd:b6:13:00  192.168.0.1  2023-03-01 03:16:21  2023-03-01 03:31:21
Vlan1000|Ethernet11  10:70:fd:b6:13:01  192.168.0.2  2023-03-01 03:16:21  2023-03-01 03:31:21
Vlan1001|<Unknown>   10:70:fd:b6:13:02  192.168.0.3  2023-03-01 03:16:21  2023-03-01 03:31:21
"""
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(show_dhcp_server.dhcp_server.commands["ipv4"].commands["lease"], [], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert result.stdout == expected_stdout

    def test_show_dhcp_server_ipv4_lease_with_dhcpintf(self, mock_db):
        expected_stdout = """\
Interface            MAC Address        IP           Lease Start          Lease End
-------------------  -----------------  -----------  -------------------  -------------------
Vlan1000|Ethernet10  10:70:fd:b6:13:00  192.168.0.1  2023-03-01 03:16:21  2023-03-01 03:31:21
Vlan1000|Ethernet11  10:70:fd:b6:13:01  192.168.0.2  2023-03-01 03:16:21  2023-03-01 03:31:21
"""
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(show_dhcp_server.dhcp_server.commands["ipv4"].commands["lease"], ["Vlan1000"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert result.stdout == expected_stdout

    def test_show_dhcp_server_ipv4_lease_client_not_in_fdb(self, mock_db):
        expected_stdout = """\
Interface           MAC Address        IP           Lease Start          Lease End
------------------  -----------------  -----------  -------------------  -------------------
Vlan1001|<Unknown>  10:70:fd:b6:13:02  192.168.0.3  2023-03-01 03:16:21  2023-03-01 03:31:21
"""
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(show_dhcp_server.dhcp_server.commands["ipv4"].commands["lease"], ["Vlan1001"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert result.stdout == expected_stdout
