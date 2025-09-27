import sys
from unittest.mock import MagicMock, patch

from click.testing import CliRunner

sys.path.append('../cli/show/plugins/')
import show_macsec


class TestShowMACsec(object):
    def test_plugin_registration(self):
        cli = MagicMock()
        show_macsec.register(cli)
        cli.add_command.assert_called_once_with(show_macsec.macsec)

    def test_show_all(self):
        runner = CliRunner()
        result = runner.invoke(show_macsec.macsec,[])
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_show_one_port(self):
        runner = CliRunner()
        result = runner.invoke(show_macsec.macsec,["Ethernet1"])
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_show_profile(self):
        runner = CliRunner()
        result = runner.invoke(show_macsec.macsec,["--profile"])
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    @patch('show_macsec.SonicV2Connector')
    def test_post_status_success(self, mock_connector):
        """Test --post-status command with successful data"""
        # Mock the database connection and data
        mock_db = MagicMock()
        mock_connector.return_value = mock_db

        # Mock keys method to return test modules
        mock_db.keys.return_value = [
            "FIPS_MACSEC_POST_TABLE|crypto",
            "FIPS_MACSEC_POST_TABLE|sai"
        ]

        # Mock get_all method to return test data
        def mock_get_all(db_name, key):
            if key == "FIPS_MACSEC_POST_TABLE|crypto":
                return {
                    'status': 'pass',
                    'timestamp': '2025-09-15 10:30:00 UTC',
                }
            elif key == "FIPS_MACSEC_POST_TABLE|sai":
                return {
                    'status': 'pass',
                    'timestamp': '2025-09-15 10:30:00 UTC',
                }
            return {}

        mock_db.get_all.side_effect = mock_get_all

        runner = CliRunner()
        result = runner.invoke(show_macsec.macsec, ["--post-status"])

        assert result.exit_code == 0
        assert "Module      : crypto" in result.output
        assert "Status      : pass" in result.output
        assert "Timestamp   : 2025-09-15 10:30:00 UTC" in result.output
        assert "Module      : sai" in result.output

    @patch('show_macsec.SonicV2Connector')
    def test_post_status_no_entries(self, mock_connector):
        """Test --post-status command when no POST entries exist"""
        # Mock the database connection
        mock_db = MagicMock()
        mock_connector.return_value = mock_db

        # Mock keys method to return empty list
        mock_db.keys.return_value = []

        runner = CliRunner()
        result = runner.invoke(show_macsec.macsec, ["--post-status"])

        assert result.exit_code == 0
        assert "No entries found" in result.output

    def test_post_status_mutual_exclusivity(self):
        """Test that --post-status and other option/argument are mutually exclusive"""
        runner = CliRunner()
        result = runner.invoke(show_macsec.macsec, ["Ethernet0", "--post-status"])

        assert result.exit_code == 0
        assert "POST status is not valid with other options/arguments" in result.output

        result = runner.invoke(show_macsec.macsec, ["--profile", "--post-status"])

        assert result.exit_code == 0
        assert "POST status is not valid with other options/arguments" in result.output

        result = runner.invoke(show_macsec.macsec, ["--dump-file", "--post-status"])

        assert result.exit_code == 0
        assert "POST status is not valid with other options/arguments" in result.output

        result = runner.invoke(show_macsec.macsec, ["--profile", "--post-status"])

        assert result.exit_code == 0
        assert "POST status is not valid with other options/arguments" in result.output

        result = runner.invoke(show_macsec.macsec, ["Ethernet0", "--profile", "--post-status"])

        assert result.exit_code == 0
        assert "POST status is not valid with other options/arguments" in result.output
