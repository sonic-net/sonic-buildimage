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
        with patch('show_macsec.multi_asic_util.MultiAsic') as mock_multi_asic_class:
            # Setup MultiAsic mock
            mock_multi_asic_instance = MagicMock()
            mock_multi_asic_instance.is_multi_asic.return_value = False
            mock_multi_asic_instance.get_ns_list_based_on_options.return_value = ['']
            mock_multi_asic_class.return_value = mock_multi_asic_instance

            # Setup database mock
            def mock_connector_side_effect(use_unix_socket_path=True, namespace=None):
                mock_db = MagicMock()

                if not namespace or namespace == '':
                    mock_db.keys.return_value = ['FIPS_MACSEC_POST_TABLE|crypto', 'FIPS_MACSEC_POST_TABLE|sai']
                    # Mock get_all for different modules
                    def mock_get_all(db_name, key):
                        if key == "FIPS_MACSEC_POST_TABLE|crypto":
                            return {
                                'status': 'pass',
                                'timestamp': '2025-09-15 10:30:00 UTC',
                            }
                        elif key == "FIPS_MACSEC_POST_TABLE|sai":
                            return {
                                'status': 'fail',
                                'timestamp': '2025-09-15 10:31:30 UTC',
                            }
                        return {}
                    mock_db.get_all.side_effect = mock_get_all
                else:
                    mock_db.keys.return_value = []
                    mock_db.get_all.return_value = {}

                return mock_db
            mock_connector.side_effect = mock_connector_side_effect

            # Test the CLI
            runner = CliRunner()
            result = runner.invoke(show_macsec.macsec, ["--post-status"])

            # Assertions
            assert result.exit_code == 0
            assert "Module      : crypto" in result.output
            assert "Module      : sai" in result.output
            assert "Status      : pass" in result.output
            assert "Status      : fail" in result.output

    @patch('show_macsec.SonicV2Connector')
    def test_post_status_no_entries(self, mock_connector):
        """Test --post-status command when no POST entries exist"""
        with patch('show_macsec.multi_asic_util.MultiAsic') as mock_multi_asic_class:
            # Mock the database connection
            # Setup MultiAsic mock
            mock_multi_asic_instance = MagicMock()
            mock_multi_asic_instance.is_multi_asic.return_value = False
            mock_multi_asic_instance.get_ns_list_based_on_options.return_value = ['']
            mock_multi_asic_class.return_value = mock_multi_asic_instance

            # Create separate mock instances for different database connections
            def mock_connector_side_effect(use_unix_socket_path=True, namespace=None):
                mock_db = MagicMock()
                mock_db.keys.return_value = []
                mock_connector.return_value = mock_db
                # Return empty dict for any get_all call (no POST data)
                mock_db.get_all.return_value = {}
                return mock_db

            mock_connector.side_effect = mock_connector_side_effect

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

    @patch('show_macsec.SonicV2Connector')
    def test_post_status_multi_asic_success(self, mock_connector):
        """Test --post-status command in multi-ASIC environment with successful data"""
        with patch('show_macsec.multi_asic_util.MultiAsic') as mock_multi_asic_class:
            mock_multi_asic_instance = MagicMock()
            mock_multi_asic_instance.is_multi_asic.return_value = True
            mock_multi_asic_instance.get_ns_list_based_on_options.return_value = ['', 'asic0', 'asic1']
            mock_multi_asic_class.return_value = mock_multi_asic_instance

            # Setup database mock
            def mock_connector_side_effect(use_unix_socket_path=True, namespace=None):
                mock_db = MagicMock()

                if namespace == 'asic0':
                    mock_db.keys.return_value = ['FIPS_MACSEC_POST_TABLE|sai']
                    mock_db.get_all.return_value = {'status': 'fail', 'timestamp': '2025-09-15 10:31:00 UTC'}
                elif namespace == 'asic1':
                    mock_db.keys.return_value = ['FIPS_MACSEC_POST_TABLE|sai']
                    mock_db.get_all.return_value = {'status': 'inprogress', 'timestamp': '2025-09-15 10:31:30 UTC'}
                elif not namespace or namespace == '':
                    mock_db.keys.return_value = ['FIPS_MACSEC_POST_TABLE|crypto', 'FIPS_MACSEC_POST_TABLE|sai']
                    # Mock get_all for different modules
                    def mock_get_all(db_name, key):
                        if key == "FIPS_MACSEC_POST_TABLE|crypto":
                            return {
                                'status': 'pass',
                                'timestamp': '2025-09-15 10:30:00 UTC',
                            }
                        elif key == "FIPS_MACSEC_POST_TABLE|sai":
                            return {
                                'status': 'pass',
                                'timestamp': '2025-09-15 10:31:30 UTC',
                            }
                            return {}
                    mock_db.get_all.side_effect = mock_get_all
                else:
                    mock_db.keys.return_value = []
                    mock_db.get_all.return_value = {}

                return mock_db

            mock_connector.side_effect = mock_connector_side_effect

            # Test the CLI
            runner = CliRunner()
            result = runner.invoke(show_macsec.macsec, ["--post-status"])

            # Assertions
            assert result.exit_code == 0
            assert "Module      : crypto" in result.output
            assert "Module      : sai" in result.output
            assert "Namespace (asic0)" in result.output
            assert "Namespace (asic1)" in result.output
            assert "Status      : pass" in result.output
            assert "Status      : fail" in result.output
            assert "Status      : inprogress" in result.output

    @patch('show_macsec.SonicV2Connector')
    def test_post_status_multi_asic_specific_namespace(self, mock_connector):
        """Test --post-status command with specific namespace filter"""
        with patch('show_macsec.multi_asic_util.MultiAsic') as mock_multi_asic_class:
            mock_multi_asic_instance = MagicMock()
            mock_multi_asic_instance.is_multi_asic.return_value = True
            mock_multi_asic_instance.get_ns_list_based_on_options.return_value = ['asic1']
            mock_multi_asic_class.return_value = mock_multi_asic_instance

            # Mock database to return keys and data only for asic1
            def mock_connector_side_effect(use_unix_socket_path=True, namespace=None):
                mock_db = MagicMock()

                if namespace == 'asic1':
                    # Only asic1 should have data when filtered
                    mock_db.keys.return_value = ['FIPS_MACSEC_POST_TABLE|sai']
                    # Mock get_all for different modules
                    def mock_get_all(db_name, key):
                        if key == "FIPS_MACSEC_POST_TABLE|sai":
                            return {
                                'status': 'pass',
                                'timestamp': '2025-09-15 10:31:00 UTC',
                            }
                        return {}
                    mock_db.get_all.side_effect = mock_get_all
                else:
                    # No keys for other namespaces (they shouldn't be called with filtering)
                    mock_db.keys.return_value = []
                    mock_db.get_all.return_value = {}

                return mock_db

            mock_connector.side_effect = mock_connector_side_effect

            # Test the new approach with namespace filtering
            runner = CliRunner()
            result = runner.invoke(show_macsec.macsec, ["--post-status"])

            assert result.exit_code == 0
            # Check that modules are displayed for asic1 only
            assert "Module      : sai" in result.output
            assert "Status      : pass" in result.output
            assert "Namespace (asic1)" in result.output

    @patch('show_macsec.SonicV2Connector')
    def test_post_status_multi_asic_partial_entries(self, mock_connector):
        """Test --post-status command when no POST data is available"""
        # Setup MultiAsic mock
        with patch('show_macsec.multi_asic_util.MultiAsic') as mock_multi_asic_class:
            mock_multi_asic_instance = MagicMock()
            mock_multi_asic_instance.is_multi_asic.return_value = True
            mock_multi_asic_instance.get_ns_list_based_on_options.return_value = ['', 'asic0', 'asic1']
            mock_multi_asic_class.return_value = mock_multi_asic_instance
            def mock_connector_side_effect(use_unix_socket_path=True, namespace=None):
                mock_db = MagicMock()
                if namespace == 'asic0':
                    # Only asic1 should have data when filtered
                    mock_db.keys.return_value = ['FIPS_MACSEC_POST_TABLE|sai']
                    # Mock get_all for different modules
                    def mock_get_all(db_name, key):
                        if key == "FIPS_MACSEC_POST_TABLE|sai":
                            return {
                                'status': 'pass',
                                'timestamp': '2025-09-15 10:31:00 UTC',
                            }
                        return {}
                    mock_db.get_all.side_effect = mock_get_all
                else:
                    # No keys for other namespaces (they shouldn't be called with filtering)
                    mock_db.keys.return_value = []
                    mock_db.get_all.return_value = {}

                return mock_db

            mock_connector.side_effect = mock_connector_side_effect

            # Test the CLI command
            runner = CliRunner()
            result = runner.invoke(show_macsec.macsec, ["--post-status"])

            assert result.exit_code == 0
            assert "No entries found" in result.output
            assert "Module      : sai" in result.output
            assert "Status      : pass" in result.output
            assert "Namespace (asic1)" in result.output
