import sys

from click.testing import CliRunner

import utilities_common.cli as clicommon

sys.path.append('../cli/show/plugins/')
import show_dhcp_server


IPV4_COMMAND = show_dhcp_server.dhcp_server.commands["ipv4"]


def invoke(mock_db, command, args):
    db = clicommon.Db()
    db.db = mock_db
    return CliRunner().invoke(command, args, obj=db)


class TestShowDHCPServerMatch:
    def test_show_all_matches_sorted_by_name(self, mock_db):
        result = invoke(mock_db, IPV4_COMMAND.commands["match"], [])

        assert result.exit_code == 0, result.output
        assert "Match" in result.output
        assert "Type" in result.output
        assert "Value" in result.output
        names = ["port-etp2", "port-etp5", "unused-match", "vendor-a", "vendor-b"]
        positions = [result.output.index(name) for name in names]
        assert positions == sorted(positions)

    def test_show_one_match(self, mock_db):
        result = invoke(mock_db, IPV4_COMMAND.commands["match"], ["vendor-a"])

        assert result.exit_code == 0, result.output
        assert "vendor-a" in result.output
        assert "option60" in result.output
        assert "MAIA-A" in result.output
        assert "vendor-b" not in result.output

    def test_show_bindings_with_ips_and_ranges(self, mock_db):
        result = invoke(mock_db, IPV4_COMMAND.commands["binding"], [])

        assert result.exit_code == 0, result.output
        assert "Interface" in result.output
        assert "Binding" in result.output
        assert "Matches" in result.output
        assert "IPs" in result.output
        assert "Ranges" in result.output
        assert result.output.index("fallback") < result.output.index("maia-a")
        assert "range3" in result.output
        assert "100.1.1.20" in result.output

    def test_show_bindings_filtered_by_vlan(self, mock_db):
        mock_db.hmset(
            "CONFIG_DB",
            "DHCP_SERVER_IPV4_BINDING|Vlan300|staged",
            {"matches@": "vendor-b", "ips@": "100.1.3.20"},
        )

        result = invoke(mock_db, IPV4_COMMAND.commands["binding"], ["Vlan100"])

        assert result.exit_code == 0, result.output
        assert "Vlan100" in result.output
        assert "Vlan300" not in result.output
        assert "staged" not in result.output
