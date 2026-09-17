import sys
import subprocess
import datetime
from unittest.mock import MagicMock, patch

from click.testing import CliRunner

sys.path.append('../cli/show/plugins/')
import show_macsec


primary_ckn = "01234567890123456789012345678912"
fallback_ckn = "11234567890123456789012345678912"
secret_cak = "2363647040534355560e000802065d574d400e000e030307075f0e5050000e5541"


def mka_record(query_status="ok", config_status="in-sync", age_seconds=2):
    last_updated = (
        datetime.datetime.now(datetime.timezone.utc) -
        datetime.timedelta(seconds=age_seconds)
    ).isoformat().replace("+00:00", "Z")
    return {
        "namespace": "",
        "interface": "Ethernet0",
        "session": {
            "profile": "rotation",
            "kay_status": "active",
            "authenticated": "false",
            "secured": "true",
            "failed": "false",
            "actor_sci": "0011223344550001",
            "key_server_sci": "0011223344550001",
            "actor_priority": "16",
            "key_server_priority": "16",
            "is_key_server": "true",
            "keys_distributed": "7",
            "keys_received": "0",
            "mka_hello_time_ms": "2000",
            "query_status": query_status,
            "last_updated": last_updated,
            "config_status": config_status,
        },
        "participants": [
            {
                "ckn": primary_ckn,
                "mi": "102030405060708090a0b0c0",
                "mn": "482",
                "active": "true",
                "is_principal": "true",
                "is_primary": "true",
                "live_peers": "1",
                "potential_peers": "0",
                "is_key_server": "true",
                "is_elected": "true",
            },
            {
                "ckn": fallback_ckn,
                "mi": "c0b0a0908070605040302010",
                "mn": "324",
                "active": "true",
                "is_principal": "false",
                "is_primary": "false",
                "live_peers": "1",
                "potential_peers": "0",
                "is_key_server": "false",
                "is_elected": "false",
            },
        ],
        "secrets": [secret_cak],
    }


def populate_mka_records(records):
    def collect(context, interface_name):
        context.mka_records.extend(records)
    return collect


def state_db_for_interface(interface_name):
    db = MagicMock()
    db.STATE_DB = "STATE_DB"
    db.get_db_separator.return_value = "|"
    session_key = "MACSEC_MKA_SESSION_TABLE|{}".format(interface_name)
    participant_key = "MACSEC_MKA_PARTICIPANT_TABLE|{}|{}".format(
        interface_name, primary_ckn
    )

    def keys(db_name, pattern):
        if pattern == "MACSEC_MKA_SESSION_TABLE|*":
            return [session_key]
        if pattern == "MACSEC_MKA_PARTICIPANT_TABLE|{}|*".format(interface_name):
            return [participant_key]
        return []

    def get_all(db_name, key):
        if key == session_key:
            return mka_record()["session"]
        if key == participant_key:
            return mka_record()["participants"][0]
        return {}

    db.keys.side_effect = keys
    db.get_all.side_effect = get_all
    return db



class TestShowMACsec(object):
    def test_plugin_registration(self):
        cli = MagicMock()
        show_macsec.register(cli)
        cli.add_command.assert_called_once_with(show_macsec.macsec)

    def test_show_all(self):
        runner = CliRunner()
        result = runner.invoke(show_macsec.macsec,[])
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert "MACsec port(Ethernet1)" in result.output
        assert "MACSEC_PORT_TABLE_KEY_SET" not in result.output
        assert "MACSEC_PORT_TABLE_DEL_SET" not in result.output

    def test_show_one_port(self):
        runner = CliRunner()
        result = runner.invoke(show_macsec.macsec,["Ethernet1"])
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_show_profile(self):
        runner = CliRunner()
        result = runner.invoke(show_macsec.macsec,["--profile"])
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    @patch.object(show_macsec.MacsecContext, "collect_mka", autospec=True)
    def test_show_mka_compact_health_status_and_age(self, collect_mka):
        records = []
        for interface, query_status, config_status, age_seconds in (
            ("Ethernet0", "ok", "in-sync", 3),
            ("Ethernet8", "error", "in-sync", 3),
            ("Ethernet16", "ok", "degraded", 3),
            ("Ethernet24", "ok", "in-sync", 61),
            ("Ethernet32", "error", "degraded", 61),
            ("Ethernet40", "error", "in-sync", None),
        ):
            record = mka_record(
                query_status=query_status,
                config_status=config_status,
                age_seconds=age_seconds or 0,
            )
            record["interface"] = interface
            if age_seconds is None:
                del record["session"]["last_updated"]
            records.append(record)
        collect_mka.side_effect = populate_mka_records(records)
        runner = CliRunner()
        result = runner.invoke(show_macsec.macsec, ["--mka"])

        assert result.exit_code == 0, result.output
        header = result.output.splitlines()[0].split()
        assert header == [
            "Interface",
            "KaY",
            "Secured",
            "Principal",
            "CKN",
            "Role",
            "Live",
            "Key-server",
            "SCI",
            "Local-KS",
            "Status",
            "Age",
        ]
        rows = {
            columns[0]: (columns[-2], columns[-1])
            for columns in (
                line.split()
                for line in result.output.splitlines()
                if line.startswith("Ethernet")
            )
        }
        assert rows == {
            "Ethernet0": ("ok", "3s"),
            "Ethernet8": ("query-error", "3s"),
            "Ethernet16": ("config-degraded", "3s"),
            "Ethernet24": ("stale", "61s"),
            "Ethernet32": (
                "query-error,stale,config-degraded",
                "61s",
            ),
            "Ethernet40": ("query-error,age-unknown", "never"),
        }

    def test_mka_freshness_matches_full_sweep_budget(self):
        now = datetime.datetime.now(datetime.timezone.utc)
        twenty_seconds_ago = (
            now - datetime.timedelta(seconds=20)
        ).isoformat().replace("+00:00", "Z")
        sixty_one_seconds_ago = (
            now - datetime.timedelta(seconds=61)
        ).isoformat().replace("+00:00", "Z")

        assert show_macsec._freshness(
            twenty_seconds_ago, "ok", now
        ) == ("20s", "{} (20s ago)".format(twenty_seconds_ago))
        assert "stale" in show_macsec._freshness(
            sixty_one_seconds_ago, "ok", now
        )[0]
        assert "query-unknown" in show_macsec._freshness(
            twenty_seconds_ago, "unexpected", now
        )[0]
        assert show_macsec._compact_status(
            {"query_status": "ok", "config_status": "in-sync"}, 60
        ) == "ok"
        assert show_macsec._compact_status(
            {"query_status": "ok", "config_status": "in-sync"}, 60.001
        ) == "stale"

    def test_mka_safe_formatters_reject_malformed_values(self):
        assert show_macsec._safe_hex("+011223344550001", (16,)) == "-"
        assert show_macsec._safe_hex(" 011223344550001", (16,)) == "-"
        assert show_macsec._format_milliseconds(None) == "-"
        assert show_macsec._format_milliseconds("invalid") == "-"
        assert show_macsec._format_milliseconds("2000") == "2000 ms"

    @patch.object(show_macsec.MacsecContext, "collect_mka", autospec=True)
    def test_show_mka_compact_naturally_sorts_interfaces_and_namespaces(self, collect_mka):
        records = []
        for interface, namespace in (
            ("Ethernet104", "asic0"),
            ("Ethernet8", "asic1"),
            ("Ethernet16", "asic0"),
            ("Ethernet0", "asic0"),
            ("Ethernet8", "asic0"),
        ):
            record = mka_record()
            record["interface"] = interface
            record["namespace"] = namespace
            records.append(record)
        collect_mka.side_effect = populate_mka_records(records)
        runner = CliRunner()
        result = runner.invoke(show_macsec.macsec, ["--mka"])

        assert result.exit_code == 0, result.output
        assert "Namespace" in result.output
        output_rows = [
            line.split()[:2]
            for line in result.output.splitlines()
            if "Ethernet" in line
        ]
        assert output_rows == [
            ["asic0", "Ethernet0"],
            ["asic0", "Ethernet8"],
            ["asic1", "Ethernet8"],
            ["asic0", "Ethernet16"],
            ["asic0", "Ethernet104"],
        ]

    @patch.object(show_macsec.MacsecContext, "collect_mka", autospec=True)
    def test_show_mka_detail_uses_secret_safe_allowlist(self, collect_mka):
        record = mka_record(config_status="degraded")
        record["session"]["config_error"] = (
            "apply failed while processing {}".format(secret_cak)
        )
        record["session"]["primary_cak"] = secret_cak
        record["participants"][0]["sak"] = secret_cak
        record["participants"].append({
            "ckn": secret_cak[2:],
            "active": "true",
            "is_principal": "false",
            "is_primary": "false",
            "live_peers": "1",
            "potential_peers": "0",
            "is_key_server": "false",
            "is_elected": "false",
        })
        collect_mka.side_effect = populate_mka_records([record])
        runner = CliRunner()
        result = runner.invoke(show_macsec.macsec, ["--mka", "Ethernet0"])

        assert result.exit_code == 0, result.output
        assert "Interface:             Ethernet0" in result.output
        assert "Authenticated-only CP: false" in result.output
        assert "Secured:               true" in result.output
        assert primary_ckn in result.output
        assert fallback_ckn in result.output
        assert "CONFIG ERROR:" in result.output
        assert "[redacted]" in result.output
        assert secret_cak not in result.output
        assert secret_cak[2:] not in result.output
        assert "primary_cak" not in result.output
        assert "sak" not in result.output.lower()

    @patch.object(show_macsec.MacsecContext, "collect_mka", autospec=True)
    def test_show_mka_missing_session_is_visible(self, collect_mka):
        collect_mka.side_effect = populate_mka_records([])
        runner = CliRunner()
        result = runner.invoke(show_macsec.macsec, ["--mka", "Ethernet0"])

        assert result.exit_code == 0
        assert "state for Ethernet0 is missing" in result.output

    def test_mka_mutual_exclusivity(self):
        runner = CliRunner()
        result = runner.invoke(show_macsec.macsec, ["--mka", "--profile"])
        assert result.exit_code == 0
        assert "mka is not valid with profile or dump-file" in result.output

        result = runner.invoke(show_macsec.macsec, ["--mka", "--post-status"])
        assert result.exit_code == 0
        assert "POST status is not valid with other options/arguments" in result.output

    def test_mka_collection_aggregates_namespace_local_state(self):
        context = show_macsec.MacsecContext.__new__(show_macsec.MacsecContext)
        context.mka_records = []
        context.config_db = MagicMock()
        context.config_db.get_entry.return_value = {
            "primary_cak": secret_cak,
        }
        context.multi_asic = MagicMock()

        context.multi_asic.current_namespace = "asic0"
        context.db = state_db_for_interface("Ethernet0")
        show_macsec.MacsecContext.collect_mka.__wrapped__(context, None)

        context.multi_asic.current_namespace = "asic1"
        context.db = state_db_for_interface("Ethernet4")
        show_macsec.MacsecContext.collect_mka.__wrapped__(context, None)

        assert [
            (record["namespace"], record["interface"])
            for record in context.mka_records
        ] == [
            ("asic0", "Ethernet0"),
            ("asic1", "Ethernet4"),
        ]
        assert secret_cak in context.mka_records[0]["secrets"]
        assert secret_cak[2:] in context.mka_records[0]["secrets"]

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

    @patch('sonic_py_common.device_info.get_path_to_platform_dir', MagicMock(return_value='.'))
    def test_show_fips_module(self):
        runner = CliRunner()
        result = runner.invoke(show_macsec.macsec,["--fips-module"])
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert "SONiC-MACsec-Version 1.0" in result.output

        # remove fips-module from platform.json. Re-test. This is to test scenario that some vendors
        # do not have fips_module defined in platform file yet.
        cmd = ['sed', '-i.bak', '/fips_module/d', './platform.json']
        subprocess.run(cmd, capture_output=True)
        result = runner.invoke(show_macsec.macsec,["--fips-module"])
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert result.output == ""
        # put original platform.json back
        cmd = ['mv', './platform.json.bak', './platform.json']
        subprocess.run(cmd, stdout=subprocess.DEVNULL)

    def test_fips_module_mutual_exclusivity(self):
        """Test that --fips-module and other option/argument are mutually exclusive"""
        runner = CliRunner()
        
        result = runner.invoke(show_macsec.macsec, ["--fips-module", "--post-status"])
        assert result.exit_code == 0
        assert "POST status is not valid with other options/arguments" in result.output

        result = runner.invoke(show_macsec.macsec, ["--fips-module", "--profile"])
        assert result.exit_code == 0
        assert "fips-module is not valid with other options/arguments" in result.output

        result = runner.invoke(show_macsec.macsec, ["--fips-module", "Ethernet1"])
        assert result.exit_code == 0
        assert "fips-module is not valid with other options/arguments" in result.output

        result = runner.invoke(show_macsec.macsec, ["--fips-module", "--dump-file"])
        assert result.exit_code == 0
        assert "fips-module is not valid with other options/arguments" in result.output