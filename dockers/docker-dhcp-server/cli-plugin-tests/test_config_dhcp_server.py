import sys
from unittest import mock
import pytest

from click.testing import CliRunner

import utilities_common.cli as clicommon

sys.path.append('../cli/config/plugins/')
import dhcp_server


class TestConfigDHCPServer(object):
    def test_plugin_registration(self):
        cli = mock.MagicMock()
        dhcp_server.register(cli)

    str_type = [[12, "whatever", False],
                ["text", "whatever", False],
                ["string", "whatever", True],
                ["binary", "12abc", False],
                ["binary", "123abc45", True],
                ["boolean", "True", False],
                ["boolean", "true", True],
                ["ipv4-address", "10.10.1", False],
                ["ipv4-address", "10.10.1.0", True],
                ["uint8", "4500", False],
                ["uint8", "-45", False],
                ["uint8", "45", True],
                ]

    @pytest.mark.parametrize("type, value, result", str_type)
    def test_validate_str_type(self, type, value, result):
        assert dhcp_server.validate_str_type(type, value) == result

    def test_config_dhcp_server_ipv4_add(self, mock_db):
        expected_value = {
            "gateway": "10.10.10.10",
            "lease_time": "1000",
            "mode": "PORT",
            "netmask": "255.255.254.0",
            "state": "disabled"
        }
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["add"], \
                ["Vlan200", "--mode=PORT", "--lease_time=1000", "--gateway=10.10.10.10", "--netmask=255.255.254.0"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert mock_db.get_all("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan200") == expected_value

    def test_config_dhcp_server_ipv4_add_dup_gw_nm(self, mock_db):
        expected_value = {
            "gateway": "100.1.1.2",
            "lease_time": "1000",
            "mode": "PORT",
            "netmask": "255.255.255.0",
            "state": "disabled"
        }
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["add"], \
                ["Vlan200", "--mode=PORT", "--lease_time=1000", "--dup_gw_nm"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert mock_db.get_all("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan200") == expected_value

    def test_config_dhcp_server_ipv4_add_illegal_mode(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["add"], \
                ["Vlan200", "--mode=WHATEVER", "--lease_time=1000", "--gateway=10.10.10.10", "--netmask=255.255.254.0"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_add_illegal_lease_time(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["add"], \
                ["Vlan200", "--mode=PORT", "--lease_time=-1000", "--gateway=10.10.10.10", "--netmask=255.255.254.0"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_add_no_vlan(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["add"], \
                ["Vlan400", "--mode=PORT", "--lease_time=1000", "--gateway=10.10.10.10", "--netmask=255.255.254.0"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_add_no_vlan_ip(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["add"], \
                ["Vlan300", "--mode=PORT", "--lease_time=1000", "--dup_gw_nm"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_add_illegal_ip(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["add"], \
                ["Vlan200", "--mode=PORT", "--lease_time=1000", "--gateway=10000.10.10.10", "--netmask=255.255.254.0"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_add_already_exist(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["add"], \
                ["Vlan100", "--mode=PORT", "--lease_time=1000", "--gateway=10.10.10.10", "--netmask=255.255.254.0"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_del_already_exist(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["del"], ["Vlan100"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert mock_db.exists("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100") == False

    def test_config_dhcp_server_ipv4_del_does_not_exist(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["del"], ["Vlan200"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_enable_already_exist(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["enable"], ["Vlan300"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan300", "state") == "enabled"

    def test_config_dhcp_server_ipv4_enable_does_not_exist(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["enable"], ["Vlan200"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_disable_already_exist(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["disable"], ["Vlan100"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan300", "state") == "disabled"

    def test_config_dhcp_server_ipv4_disable_does_not_exist(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["disable"], ["Vlan200"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_update_does_not_exist(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["update"], ["Vlan200"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_update_already_exist(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["update"], ["Vlan100"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_update_wrong_mode(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["update"], ["Vlan100", "--mode=XX"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_update_lease_time(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["update"], ["Vlan100", "--lease_time=1800"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "lease_time") == "1800"

    def test_config_dhcp_server_ipv4_update_wrong_lease_time(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["update"], ["Vlan100", "--lease_time=-1800"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_update_dup_gw_nm(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["update"], ["Vlan100", "--dup_gw_nm"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "gateway") == "100.1.1.1"
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "netmask") == "255.255.255.0"

    def test_config_dhcp_server_ipv4_update_dup_gw_nm_no_data(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["update"], ["Vlan300", "--dup_gw_nm"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_update_gateway(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["update"], ["Vlan100", "--gateway=100.1.1.9"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "gateway") == "100.1.1.9"

    def test_config_dhcp_server_ipv4_update_wrong_gateway(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["update"], ["Vlan100", "--gateway=100.1.9"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_update_netmask(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["update"], ["Vlan100", "--netmask=255.255.254.0"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "netmask") == "255.255.254.0"

    def test_config_dhcp_server_ipv4_update_wrong_netmask(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["update"], ["Vlan100", "--netmask=255.255.254"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

