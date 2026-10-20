from copy import deepcopy
from unittest import TestCase
from unittest.mock import MagicMock

from gnmi_config import migrate_gnmi_defaults, set_gnmi_defaults


class TestGnmiConfig(TestCase):
    def setUp(self):
        self.data = {
            "FEATURE": {"gnmi": {"state": "enabled"}},
            "DEVICE_METADATA": {"localhost": {"type": "ToRRouter"}},
            "TELEMETRY": {
                "gnmi": {"port": "50051", "client_auth": "true"},
                "certs": {"server_crt": "/legacy/server.crt", "server_key": "/legacy/server.key"},
            },
        }
        self.configdb = MagicMock()
        self.configdb.get_table.return_value = {}
        self.configdb.get_entry.return_value = {}

    def test_provision_native_defaults_without_changing_legacy_configuration(self):
        original = deepcopy(self.data)
        self.assertTrue(set_gnmi_defaults(self.data, self.configdb))
        self.assertEqual(self.data.pop("GNMI"), {
            "gnmi": {"port": "8080", "user_auth": "password,jwt"},
        })
        self.assertEqual(self.data, original)

    def test_provisioning_is_idempotent(self):
        self.assertTrue(set_gnmi_defaults(self.data, self.configdb))
        original = deepcopy(self.data)
        self.assertFalse(set_gnmi_defaults(self.data, self.configdb))
        self.assertEqual(self.data, original)

    def test_always_enabled_feature_gets_explicit_defaults(self):
        self.data["FEATURE"]["gnmi"]["state"] = "always_enabled"
        self.assertTrue(set_gnmi_defaults(self.data, self.configdb))

    def test_missing_or_disabled_feature_is_not_provisioned(self):
        for feature in ({}, {"gnmi": {}}, {"gnmi": {"state": "disabled"}}):
            with self.subTest(feature=feature):
                self.data["FEATURE"] = feature
                original = deepcopy(self.data)
                self.assertFalse(set_gnmi_defaults(self.data, self.configdb))
                self.assertEqual(self.data, original)
        self.configdb.get_table.assert_not_called()

    def test_explicit_native_configuration_is_never_repaired_or_overwritten(self):
        for gnmi in (
            {},
            {"gnmi": {"port": "8080"}},
            {"gnmi": {"port": "8080", "user_auth": ""}},
            {"gnmi": {"port": "8080", "user_auth": None}},
            {"gnmi": {"port": "8080", "user_auth": "password"}},
            {"gnmi": {"port": "0"}},
            {"certs": {"server_crt": "/server.crt", "server_key": "/server.key"}},
        ):
            with self.subTest(gnmi=gnmi):
                self.data["GNMI"] = gnmi
                original = deepcopy(self.data)
                self.assertFalse(set_gnmi_defaults(self.data, self.configdb))
                self.assertEqual(self.data, original)
        self.configdb.get_table.assert_not_called()

    def test_partial_config_write_preserves_existing_native_configuration(self):
        for gnmi in (
            {"gnmi": {"port": "8080"}},
            {"gnmi": {"port": "8080", "user_auth": "jwt"}},
            {"certs": {"server_crt": "/server.crt"}},
        ):
            with self.subTest(gnmi=gnmi):
                self.configdb.get_table.return_value = gnmi
                original = deepcopy(self.data)
                self.assertFalse(set_gnmi_defaults(self.data, self.configdb))
                self.assertEqual(self.data, original)

    def test_input_tls_and_dpu_metadata_keep_existing_launcher_defaults(self):
        for metadata in (
            {"x509": {"server_crt": "/server.crt", "server_key": "/server.key"}},
            {"localhost": {"type": "SmartSwitchDPU"}},
            {"localhost": {"switch_type": "dpu"}},
        ):
            with self.subTest(metadata=metadata):
                self.data["DEVICE_METADATA"] = metadata
                original = deepcopy(self.data)
                self.assertFalse(set_gnmi_defaults(self.data, self.configdb))
                self.assertEqual(self.data, original)

    def test_existing_tls_and_dpu_metadata_are_respected_by_partial_writes(self):
        for entries in (
            {"x509": {"server_crt": "/server.crt", "server_key": "/server.key"}},
            {"localhost": {"type": "SmartSwitchDPU"}},
            {"localhost": {"switch_type": "dpu"}},
        ):
            with self.subTest(entries=entries):
                self.data["DEVICE_METADATA"] = {"localhost": {"hostname": "switch"}}
                self.configdb.get_entry.side_effect = lambda table, key: entries.get(key, {})
                original = deepcopy(self.data)
                self.assertFalse(set_gnmi_defaults(self.data, self.configdb))
                self.assertEqual(self.data, original)

    def test_database_read_failure_is_not_hidden(self):
        self.configdb.get_table.side_effect = RuntimeError("CONFIG_DB unavailable")
        with self.assertRaisesRegex(RuntimeError, "CONFIG_DB unavailable"):
            set_gnmi_defaults(self.data, self.configdb)
        self.assertNotIn("GNMI", self.data)

    def test_upgrade_migration_writes_only_the_new_native_profile(self):
        self.configdb.get_entry.side_effect = lambda table, key: (
            {"state": "enabled"} if table == "FEATURE" else {}
        )
        self.assertTrue(migrate_gnmi_defaults(self.configdb))
        self.configdb.mod_config.assert_called_once_with({
            "GNMI": {"gnmi": {"port": "8080", "user_auth": "password,jwt"}},
        })

    def test_upgrade_migration_does_not_enable_a_disabled_feature(self):
        self.configdb.get_entry.return_value = {"state": "disabled"}
        self.assertFalse(migrate_gnmi_defaults(self.configdb))
        self.configdb.mod_config.assert_not_called()

    def test_upgrade_migration_preserves_existing_native_configuration(self):
        self.configdb.get_entry.return_value = {"state": "enabled"}
        self.configdb.get_table.return_value = {"gnmi": {"port": "8080"}}
        self.assertFalse(migrate_gnmi_defaults(self.configdb))
        self.configdb.mod_config.assert_not_called()

    def test_failed_upgrade_migration_is_not_hidden(self):
        self.configdb.get_entry.side_effect = lambda table, key: (
            {"state": "enabled"} if table == "FEATURE" else {}
        )
        self.configdb.mod_config.side_effect = RuntimeError("CONFIG_DB write failed")
        with self.assertRaisesRegex(RuntimeError, "CONFIG_DB write failed"):
            migrate_gnmi_defaults(self.configdb)
