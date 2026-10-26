import json
import os
from importlib.machinery import SourceFileLoader
from io import StringIO
from unittest import TestCase, mock


cfggen_path = os.path.join(os.path.dirname(__file__), "..", "sonic-cfggen")
cfggen = SourceFileLoader("gnmi_cfggen", cfggen_path).load_module()


class TestGnmiCfggen(TestCase):
    def setUp(self):
        self.data = {
            "FEATURE": {"gnmi": {"state": "enabled"}},
            "DEVICE_METADATA": {"localhost": {"type": "ToRRouter"}},
            "TELEMETRY": {"gnmi": {"port": "50051", "client_auth": "true"}},
        }
        self.configdb = mock.MagicMock()
        self.configdb.get_table.return_value = {}
        self.configdb.get_entry.return_value = {}

    def run_cfggen(self, *arguments):
        output = StringIO()
        with mock.patch("sys.argv", ["sonic-cfggen", "-a", json.dumps(self.data)] + list(arguments)), \
                mock.patch("sys.stdout", output), \
                mock.patch("sys.stderr", StringIO()), \
                mock.patch.object(cfggen.device_info, "get_platform", return_value=None), \
                mock.patch.object(cfggen, "get_asic_id_from_name", return_value=0), \
                mock.patch.object(cfggen, "load_namespace_config"), \
                mock.patch.object(cfggen, "ConfigDBPipeConnector", return_value=self.configdb):
            cfggen.main()
        return output.getvalue()

    def test_configuration_loading_writes_the_explicit_profile(self):
        self.run_cfggen("--write-to-db")
        written = self.configdb.mod_config.call_args[0][0]
        self.assertEqual(written["GNMI"], {
            "gnmi": {"port": "8080", "user_auth": "password,jwt"},
        })
        self.assertEqual(written["TELEMETRY"], self.data["TELEMETRY"])

    def test_read_only_configuration_queries_do_not_create_a_profile(self):
        output = self.run_cfggen("--print-data")
        self.assertNotIn("GNMI", json.loads(output))
        self.configdb.mod_config.assert_not_called()
        self.configdb.get_table.assert_not_called()

    def test_existing_invalid_native_configuration_is_not_repaired(self):
        self.data["GNMI"] = {"gnmi": {"port": "8080"}}
        self.run_cfggen("--write-to-db")
        written = self.configdb.mod_config.call_args[0][0]
        self.assertEqual(written["GNMI"], self.data["GNMI"])
        self.assertNotIn("user_auth", written["GNMI"]["gnmi"])

    def test_asic_namespace_does_not_get_host_gnmi_defaults(self):
        self.run_cfggen("-n", "asic0", "--write-to-db")
        self.assertNotIn("GNMI", self.configdb.mod_config.call_args[0][0])
        self.configdb.get_table.assert_not_called()

    def test_failed_database_write_is_reported(self):
        self.configdb.mod_config.side_effect = RuntimeError("CONFIG_DB write failed")
        with self.assertRaisesRegex(RuntimeError, "CONFIG_DB write failed"):
            self.run_cfggen("--write-to-db")
