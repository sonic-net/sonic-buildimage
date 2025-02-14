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

    @pytest.mark.parametrize("state", ["disabled", "enabled"])
    def test_config_dhcp_server_feature_state_checking(self, mock_db, state):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        mock_db.set("CONFIG_DB", "FEATURE|dhcp_server", "state", state)
        result = runner.invoke(dhcp_server.dhcp_server, obj=db)
        if state == "disabled":
            assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
            assert "Feature dhcp_server is not enabled" in result.output
        elif state == "enabled":
            assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
            assert "Usage: dhcp_server [OPTIONS] COMMAND [ARGS]" in result.output
        else:
            assert False

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
            "gateway": "100.1.2.2",
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

    def test_config_dhcp_server_ipv4_range_add(self, mock_db):
        expected_value = {
            "range@": "10.10.10.10,10.10.10.11"
        }
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["range"].commands["add"], \
                ["range4", "10.10.10.10", "10.10.10.11"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert mock_db.get_all("CONFIG_DB", "DHCP_SERVER_IPV4_RANGE|range4") == expected_value

    def test_config_dhcp_server_ipv4_range_add_existing(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["range"].commands["add"], \
                ["range1", "10.10.10.10", "10.10.10.11"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_range_add_single_ip(self, mock_db):
        expected_value = {
            "range@": "10.10.10.10,10.10.10.10"
        }
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["range"].commands["add"], \
                ["range4", "10.10.10.10"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert mock_db.get_all("CONFIG_DB", "DHCP_SERVER_IPV4_RANGE|range4") == expected_value

    def test_config_dhcp_server_ipv4_range_add_wrong_ip(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["range"].commands["add"], \
                ["range4", "10.10.10"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_range_add_wrong_order(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["range"].commands["add"], \
                ["range4", "10.10.10.10", "10.10.10.9"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_range_update(self, mock_db):
        expected_value = {
            "range@": "10.10.10.10,10.10.10.11"
        }
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["range"].commands["update"], \
                ["range1", "10.10.10.10", "10.10.10.11"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert mock_db.get_all("CONFIG_DB", "DHCP_SERVER_IPV4_RANGE|range1") == expected_value

    def test_config_dhcp_server_ipv4_range_update_nonexisting(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["range"].commands["update"], \
                ["range4", "10.10.10.10", "10.10.10.11"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_range_update_single_ip(self, mock_db):
        expected_value = {
            "range@": "10.10.10.10,10.10.10.10"
        }
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["range"].commands["update"], \
                ["range1", "10.10.10.10"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert mock_db.get_all("CONFIG_DB", "DHCP_SERVER_IPV4_RANGE|range1") == expected_value

    def test_config_dhcp_server_ipv4_range_update_wrong_ip(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["range"].commands["update"], \
                ["range1", "10.10.10"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_range_add_wrong_order(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["range"].commands["update"], \
                ["range1", "10.10.10.10", "10.10.10.9"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_range_delete(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["range"].commands["del"], \
                ["range2"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert mock_db.exists("CONFIG_DB", "DHCP_SERVER_IPV4_RANGE|range2") == False

    def test_config_dhcp_server_ipv4_range_delete_nonexisting(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["range"].commands["del"], \
                ["range4"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_range_delete_referenced(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["range"].commands["del"], \
                ["range1"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_range_delete_referenced_force(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["range"].commands["del"], \
                ["range1", "--force"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert mock_db.exists("CONFIG_DB", "DHCP_SERVER_IPV4_RANGE|range1") == False

    def test_config_dhcp_server_ipv4_bind_range_nonexisting(self, mock_db):
        expected_value = "range2,range3"
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["bind"], \
                ["Vlan100", "Ethernet2", "--range", "range2,range3"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        result = mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4_PORT|Vlan100|Ethernet2", "ranges@")
        assert result and set(result.split(",")) == set(expected_value.split(","))

    def test_config_dhcp_server_ipv4_bind_ip_nonexisting(self, mock_db):
        expected_value = "100.1.1.1,100.1.1.2"
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["bind"], \
                ["Vlan100", "Ethernet2", "100.1.1.1,100.1.1.2"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        result = mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4_PORT|Vlan100|Ethernet2", "ips@")
        assert result and set(result.split(",")) == set(expected_value.split(","))

    def test_config_dhcp_server_ipv4_bind_range_existing_no_duplicate(self, mock_db):
        expected_value = "range1,range2,range3"
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["bind"], \
                ["Vlan100", "Ethernet7", "--range", "range2"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        result = mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4_PORT|Vlan100|Ethernet7", "ranges@")
        assert result and set(result.split(",")) == set(expected_value.split(","))

    def test_config_dhcp_server_ipv4_bind_range_existing_duplicate(self, mock_db):
        expected_value = "range1,range2,range3"
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["bind"], \
                ["Vlan100", "Ethernet7", "--range", "range2,range3"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        result = mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4_PORT|Vlan100|Ethernet7", "ranges@")
        assert result and set(result.split(",")) == set(expected_value.split(","))

    def test_config_dhcp_server_ipv4_bind_ip_existing_no_duplicate(self, mock_db):
        expected_value = "100.1.1.10,100.1.1.11,100.1.1.12,100.1.1.13"
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["bind"], \
                ["Vlan100", "Ethernet4", "100.1.1.12,100.1.1.13"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        result = mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4_PORT|Vlan100|Ethernet4", "ips@")
        assert result and set(result.split(",")) == set(expected_value.split(","))

    def test_config_dhcp_server_ipv4_bind_ip_existing_duplicate(self, mock_db):
        expected_value = "100.1.1.10,100.1.1.11,100.1.1.12"
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["bind"], \
                ["Vlan100", "Ethernet4", "100.1.1.11,100.1.1.12"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        result = mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4_PORT|Vlan100|Ethernet4", "ips@")
        assert result and set(result.split(",")) == set(expected_value.split(","))

    def test_config_dhcp_server_ipv4_bind_nonexisting_range(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["bind"], \
                ["Vlan100", "Ethernet7", "--range", "range4"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_bind_range_out_of_subnet(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["bind"], \
                ["Vlan100", "Ethernet7", "--range", "range5"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_bind_ip_out_of_subnet(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["bind"], \
                ["Vlan100", "Ethernet4", "100.1.2.10"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_bind_interface_not_in_vlan(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["bind"], \
                ["Vlan100", "Ethernet5", "100.1.1.10"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_bind_range_and_ip(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["bind"], \
                ["Vlan100", "Ethernet2", "100.1.1.13,100.1.1.14", "--range", "range3"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_bind_range_to_existing_ip(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["bind"], \
                ["Vlan100", "Ethernet4", "--range", "range3"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_bind_ip_to_existing_range(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["bind"], \
                ["Vlan100", "Ethernet7", "100.1.1.13,100.1.1.14"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_bind_nothing(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["bind"], \
                ["Vlan100", "Ethernet2"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_unbind_range_with_remain(self, mock_db):
        expected_value = "range1"
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["unbind"], \
                ["Vlan100", "Ethernet7", "--range", "range3"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        result = mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4_PORT|Vlan100|Ethernet7", "ranges@")
        assert result and set(result.split(",")) == set(expected_value.split(","))

    def test_config_dhcp_server_ipv4_unbind_range_with_no_remain(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["unbind"], \
                ["Vlan100", "Ethernet7", "--range", "range1,range3"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert mock_db.exists("CONFIG_DB", "DHCP_SERVER_IPV4_PORT|Vlan100|Ethernet7") == False

    def test_config_dhcp_server_ipv4_unbind_range_with_all(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["unbind"], \
                ["Vlan100", "Ethernet7", "all"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert mock_db.exists("CONFIG_DB", "DHCP_SERVER_IPV4_PORT|Vlan100|Ethernet7") == False

    def test_config_dhcp_server_ipv4_unbind_ip_with_remain(self, mock_db):
        expected_value = "100.1.1.10"
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["unbind"], \
                ["Vlan100", "Ethernet4", "100.1.1.11"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        result = mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4_PORT|Vlan100|Ethernet4", "ips@")
        assert result and set(result.split(",")) == set(expected_value.split(","))

    def test_config_dhcp_server_ipv4_unbind_ip_with_no_remain(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["unbind"], \
                ["Vlan100", "Ethernet4", "100.1.1.10,100.1.1.11"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert mock_db.exists("CONFIG_DB", "DHCP_SERVER_IPV4_PORT|Vlan100|Ethernet4") == False

    def test_config_dhcp_server_ipv4_unbind_ip_all(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["unbind"], \
                ["Vlan100", "Ethernet4", "all"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert mock_db.exists("CONFIG_DB", "DHCP_SERVER_IPV4_PORT|Vlan100|Ethernet4") == False

    def test_config_dhcp_server_ipv4_unbind_range_and_ip(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["unbind"], \
                ["Vlan100", "Ethernet2", "100.1.1.13,100.1.1.14", "--range", "range3"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_unbind_range_to_existing_ip(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["unbind"], \
                ["Vlan100", "Ethernet4", "--range", "range3"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_unbind_ip_to_existing_range(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["unbind"], \
                ["Vlan100", "Ethernet7", "100.1.1.13,100.1.1.14"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_unbind_nothing(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["unbind"], \
                ["Vlan100", "Ethernet4"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_unbind_unbind_range(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["unbind"], \
                ["Vlan100", "Ethernet7", "--range", "range2"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_unbind_unbind_ip(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["unbind"], \
                ["Vlan100", "Ethernet4", "100.1.1.13,100.1.1.14"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_option_add(self, mock_db):
        expected_value = {
            "id": "165",
            "type": "string",
            "value": "dummy_value"
        }
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["option"].commands["add"], \
                ["option62", "165", "string", "dummy_value"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert mock_db.get_all("CONFIG_DB", "DHCP_SERVER_IPV4_CUSTOMIZED_OPTIONS|option62") == expected_value

    def test_config_dhcp_server_ipv4_option_add_existing(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["option"].commands["add"], \
                ["option60", "163", "string", "dummy_value"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_option_add_illegal_option_id(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["option"].commands["add"], \
                ["option62", "10", "string", "dummy_value"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_option_add_illegal_type(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["option"].commands["add"], \
                ["option62", "165", "xx", "xx"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_option_add_illegal_value(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["option"].commands["add"], \
                ["option62", "165", "uint8", "1000000"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_option_del(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["option"].commands["del"], \
                ["option61"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert mock_db.exists("CONFIG_DB", "DHCP_SERVER_IPV4_CUSTOMIZED_OPTIONS|option61") == False

    def test_config_dhcp_server_ipv4_option_del_nonexisting(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["option"].commands["del"], \
                ["option62"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_option_del_referenced(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["option"].commands["del"], \
                ["option60"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_option_bind(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["option"].commands["bind"], \
                ["Vlan300", "option60"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan300", "customized_options@") == "option60"

    def test_config_dhcp_server_ipv4_option_bind_multiple_options(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["option"].commands["bind"], \
                ["Vlan300", "option60,option61"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        result = mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan300", "customized_options@")
        assert result and set(result.split(",")) == set("option60,option61".split(","))

    def test_config_dhcp_server_ipv4_option_bind_to_existing(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["option"].commands["bind"], \
                ["Vlan100", "option61"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        result = mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "customized_options@")
        assert result and set(result.split(",")) == set("option60,option61".split(","))

    def test_config_dhcp_server_ipv4_option_bind_same_option_to_existing(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["option"].commands["bind"], \
                ["Vlan100", "option60"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "customized_options@") == "option60"

    def test_config_dhcp_server_ipv4_option_bind_to_nonexisting_intf(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["option"].commands["bind"], \
                ["Vlan200", "option60"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_option_bind_nonexisting_option(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["option"].commands["bind"], \
                ["Vlan300", "option62"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_option_unbind(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["option"].commands["unbind"], \
                ["Vlan100", "option60"], obj=db)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        result = mock_db.get("CONFIG_DB", "DHCP_SERVER_IPV4|Vlan100", "customized_options@")
        assert result == None

    def test_config_dhcp_server_ipv4_option_unbind_nonexisting_intf(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["option"].commands["unbind"], \
                ["Vlan200", "option60"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_config_dhcp_server_ipv4_option_unbind_nonexisting_option(self, mock_db):
        runner = CliRunner()
        db = clicommon.Db()
        db.db = mock_db
        result = runner.invoke(dhcp_server.dhcp_server.commands["ipv4"].commands["option"].commands["unbind"], \
                ["Vlan100", "option61"], obj=db)
        assert result.exit_code == 2, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
