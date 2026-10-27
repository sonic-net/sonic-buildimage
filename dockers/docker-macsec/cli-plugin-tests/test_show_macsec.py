import json
import subprocess
import sys
from unittest.mock import MagicMock, patch

from click.testing import CliRunner

sys.path.append('../cli/show/plugins/')
import show_macsec


primary_ckn = "01234567890123456789012345678912"
fallback_ckn = "11234567890123456789012345678912"
secret_cak = "2363647040534355560e000802065d574d400e000e030307075f0e5050000e5541"


def mka_record():
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
            "config_status": "in-sync",
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
    return state_db_for_snapshots([interface_name], [interface_name])


def state_db_for_snapshots(session_interfaces, participant_interfaces,
                           empty_participant_interfaces=(), session_overrides=None,
                           participant_overrides=None):
    db = MagicMock()
    db.STATE_DB = "STATE_DB"
    db.get_db_separator.return_value = "|"
    session_keys = [
        "MACSEC_MKA_SESSION_TABLE|{}".format(interface_name)
        for interface_name in session_interfaces
    ]
    participant_keys = {
        interface_name: "MACSEC_MKA_PARTICIPANT_TABLE|{}|{}".format(
            interface_name, primary_ckn
        )
        for interface_name in participant_interfaces
    }
    empty_participant_keys = {
        participant_keys[interface_name]
        for interface_name in empty_participant_interfaces
    }

    def keys(db_name, pattern):
        if pattern == "MACSEC_MKA_SESSION_TABLE|*":
            return session_keys
        for interface_name, participant_key in participant_keys.items():
            if pattern == "MACSEC_MKA_PARTICIPANT_TABLE|{}|*".format(
                    interface_name):
                return [participant_key]
        return []

    def get_all(db_name, key):
        if key in session_keys:
            if session_overrides and key in session_overrides:
                return session_overrides[key]
            return mka_record()["session"]
        if participant_overrides and key in participant_overrides:
            return participant_overrides[key]
        if key in empty_participant_keys:
            return {}
        if key in participant_keys.values():
            return mka_record()["participants"][0]
        return {}

    db.keys.side_effect = keys
    db.get_all.side_effect = get_all
    return db



class TestShowMACsec(object):
    def test_cache_json_round_trip_preserves_sa_counter_baseline(self, tmp_path):
        cached_sa = show_macsec.MACsecIngressSA.__new__(show_macsec.MACsecIngressSA)
        cached_sa.key = "MACSEC_INGRESS_SA_TABLE:Ethernet0:sci:0"
        cached_sa.sak = "sak-a"
        cached_sa.counters = {"SAI_MACSEC_SA_STAT_OCTETS_ENCRYPTED": "120"}

        target_sa = show_macsec.MACsecIngressSA.__new__(show_macsec.MACsecIngressSA)
        target_sa.key = cached_sa.key
        target_sa.sak = cached_sa.sak
        target_sa.an = "0"
        target_sa.meta = {"state": "active"}
        target_sa.counters = {"SAI_MACSEC_SA_STAT_OCTETS_ENCRYPTED": "125"}

        path = tmp_path / "macsecstats.json"
        cache = {
            "version": 1,
            "time": "2026-10-05 12:00:00",
            "objs": [show_macsec._cache_entry(cached_sa)],
        }
        show_macsec._save_cache(str(path), cache)

        loaded_cache = show_macsec._load_cache(str(path))
        cached_entry = show_macsec.cache_find(loaded_cache, target_sa)
        rotated_sa = show_macsec.MACsecIngressSA.__new__(show_macsec.MACsecIngressSA)
        rotated_sa.key = target_sa.key
        rotated_sa.sak = "sak-b"

        assert "5" in target_sa.dump_str(cached_entry)
        assert show_macsec.cache_find(loaded_cache, rotated_sa) is None

    def test_load_cache_ignores_malformed_cache_data(self, tmp_path):
        malformed_values = [
            "[]",
            '{"version": 1, "time": "now", "objs": [{}]}',
            '{"version": 1, "time": "now", "objs": [{"type": "MACsecIngressSA", "key": "sa"}]}',
            '{"version": 1, "time": "now", "objs": [{"type": "MACsecIngressSA", "key": "sa", "sak_hash": "' +
            "a" * 64 + '", "counters": {"octets": "invalid"}}]}',
            '{"version": 1, "objs": []}',
            '{"version": 1, "time": "now", "objs": [',
        ]

        for index, contents in enumerate(malformed_values):
            path = tmp_path / "cache-{}.json".format(index)
            path.write_text(contents, encoding="utf-8")
            assert show_macsec._load_cache(str(path)) == {}

    def test_show_ignores_malformed_cache(self, tmp_path, monkeypatch):
        cache_path = tmp_path / "macsecstats.json"
        cache_path.write_text("[]", encoding="utf-8")
        monkeypatch.setattr(show_macsec, "CACHE_FILE", str(tmp_path / "macsecstats{}.json"))

        result = CliRunner().invoke(show_macsec.macsec, [])

        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(
            result.exit_code, result.exception, result.exc_info)
        assert "MACsec port(Ethernet1)" in result.output

    def test_dump_file_replaces_cache_without_loading_it(self, tmp_path, monkeypatch):
        cache_path = tmp_path / "macsecstats.json"
        cache_path.write_text("[]", encoding="utf-8")
        monkeypatch.setattr(show_macsec, "CACHE_FILE", str(tmp_path / "macsecstats{}.json"))

        def unexpected_cache_load(path):
            raise AssertionError("cache should not be loaded")

        monkeypatch.setattr(show_macsec, "_load_cache", unexpected_cache_load)

        result = CliRunner().invoke(show_macsec.macsec, ["--dump-file"])

        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(
            result.exit_code, result.exception, result.exc_info)
        with open(cache_path, encoding="utf-8") as cache_file:
            assert json.load(cache_file)["version"] == 1

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

    def test_show_port_deleted_from_config(self):
        """A port whose 'macsec' field was removed from CONFIG_DB but whose
        APPL_DB entry has not been cleaned up yet (mid-teardown of
        'config macsec port del') is still shown, with an empty profile.
        Ethernet5 is in this state in the mock DBs."""
        runner = CliRunner()
        result = runner.invoke(show_macsec.macsec,[])
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        assert "MACsec port(Ethernet5)" in result.output

    def test_show_profile(self):
        runner = CliRunner()
        result = runner.invoke(show_macsec.macsec,["--profile"])
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    @patch.object(show_macsec.MacsecContext, "collect_mka", autospec=True)
    def test_show_mka_compact_valid_snapshot_has_no_health_columns(
            self, collect_mka):
        collect_mka.side_effect = populate_mka_records([mka_record()])
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
            "prim./fall.",
            "live",
            "peers",
            "Local-KS",
            "Keys",
            "distributed",
        ]
        assert "Key-server" not in header
        assert "SCI" not in header
        assert "Status" not in result.output
        assert "Age" not in result.output
        assert "query" not in result.output.lower()
        assert "last updated" not in result.output.lower()

    def test_mka_safe_formatters_reject_malformed_values(self):
        assert show_macsec._safe_hex("+011223344550001", (16,)) == "-"
        assert show_macsec._safe_hex(" 011223344550001", (16,)) == "-"
        assert show_macsec._format_milliseconds(None) == "-"
        assert show_macsec._format_milliseconds("invalid") == "-"
        assert show_macsec._format_milliseconds("2000") == "2000 ms"

    @patch.object(show_macsec.MacsecContext, "collect_mka", autospec=True)
    def test_show_mka_compact_keys_distributed_is_strict_and_secret_safe(
            self, collect_mka):
        valid = mka_record()
        valid["interface"] = "Ethernet0"
        valid["session"]["keys_distributed"] = "12"

        missing = mka_record()
        missing["interface"] = "Ethernet4"
        del missing["session"]["keys_distributed"]

        malformed = mka_record()
        malformed["interface"] = "Ethernet8"
        malformed["session"]["keys_distributed"] = "not-a-count"

        numeric_secret = "12" * 33
        poisoned = mka_record()
        poisoned["interface"] = "Ethernet12"
        poisoned["session"]["keys_distributed"] = numeric_secret
        poisoned["secrets"] = [numeric_secret, numeric_secret[2:]]

        collect_mka.side_effect = populate_mka_records([
            poisoned,
            malformed,
            missing,
            valid,
        ])
        runner = CliRunner()
        result = runner.invoke(show_macsec.macsec, ["--mka"])

        assert result.exit_code == 0, result.output
        header = result.output.splitlines()[0].split()
        assert "Keys" in header
        assert "distributed" in header
        rows = {
            columns[0]: columns
            for columns in (
                line.split()
                for line in result.output.splitlines()
                if line.startswith("Ethernet")
            )
        }
        assert rows["Ethernet0"][-1] == "12"
        assert rows["Ethernet4"][-1] == "-"
        assert rows["Ethernet8"][-1] == "-"
        assert rows["Ethernet12"][-1] == "-"
        assert numeric_secret not in result.output
        assert numeric_secret[2:] not in result.output

    def test_config_error_redacts_cak_shapes_without_hiding_ckn(self):
        decoded_current_cak = secret_cak[2:]
        stale_decoded_cak = "ab" * 32
        stale_raw_256_cak = "cd" * 64
        contextual_caks = ("ba" * 32, "dc" * 32, "12" * 32)
        safe_ckn = "ef" * 32
        error = (
            "encoded={} decoded={} previous_cak={} snapshot={} "
            "current_cak={} replacement_cak={} configured_cak={} "
            "ckn={} interface=Ethernet0"
        ).format(
            secret_cak,
            decoded_current_cak,
            stale_decoded_cak,
            stale_raw_256_cak,
            *contextual_caks,
            safe_ckn,
        )

        redacted = show_macsec._redact_known_secrets(
            error, [secret_cak]
        )

        assert secret_cak not in redacted
        assert decoded_current_cak not in redacted
        assert stale_decoded_cak not in redacted
        assert stale_raw_256_cak not in redacted
        assert "previous_cak=[redacted]" in redacted
        assert "snapshot=[redacted]" in redacted
        for label, cak in zip(
                ("current_cak", "replacement_cak", "configured_cak"),
                contextual_caks):
            assert cak not in redacted
            assert "{}=[redacted]".format(label) in redacted
        assert "ckn={}".format(safe_ckn) in redacted
        assert "interface=Ethernet0" in redacted

    def test_controlled_port_mode_state_matrix(self):
        cases = (
            ("active", "false", "true", "false", "secured"),
            ("active", "true", "false", "false", "authenticated-only"),
            ("active", "false", "false", "true", "failed"),
            ("active", "true", "true", "true", "failed"),
            ("not-active", "false", "false", "false", "inactive"),
            ("active", "true", "true", "false", "inconsistent"),
            ("active", "false", "false", "false", "inconsistent"),
            ("not-active", "true", "false", "false", "inconsistent"),
            ("invalid", "false", "true", "false", "-"),
            ("active", "invalid", "true", "false", "-"),
        )
        for kay_status, authenticated, secured, failed, expected in cases:
            session = {
                "kay_status": kay_status,
                "authenticated": authenticated,
                "secured": secured,
                "failed": failed,
                "primary_cak": secret_cak,
            }
            assert show_macsec._controlled_port_mode(session) == expected

        assert show_macsec._controlled_port_mode({
            "kay_status": "active",
            "authenticated": "false",
            "secured": "true",
        }) == "-"

    @patch.object(show_macsec.MacsecContext, "collect_mka", autospec=True)
    def test_show_mka_compact_live_peers_by_configured_role(self, collect_mka):
        primary_principal = mka_record()
        primary_principal["interface"] = "Ethernet0"
        primary_principal["participants"][0]["live_peers"] = "1"
        primary_principal["participants"][1]["live_peers"] = "1"

        fallback_principal_primary_live = mka_record()
        fallback_principal_primary_live["interface"] = "Ethernet4"
        fallback_principal_primary_live["participants"][0].update({
            "is_principal": "false",
            "live_peers": "3",
        })
        fallback_principal_primary_live["participants"][1].update({
            "is_principal": "true",
            "live_peers": "4",
        })

        fallback_principal_primary_not_live = mka_record()
        fallback_principal_primary_not_live["interface"] = "Ethernet8"
        fallback_principal_primary_not_live["participants"][0].update({
            "is_principal": "false",
            "live_peers": "0",
        })
        fallback_principal_primary_not_live["participants"][1].update({
            "is_principal": "true",
            "live_peers": "5",
        })

        primary_only = mka_record()
        primary_only["interface"] = "Ethernet12"
        primary_only["participants"] = primary_only["participants"][:1]
        primary_only["participants"][0]["live_peers"] = "6"

        duplicate_primary = mka_record()
        duplicate_primary["interface"] = "Ethernet16"
        duplicate_primary["participants"].append({
            "ckn": "21234567890123456789012345678912",
            "is_principal": "false",
            "is_primary": "true",
            "live_peers": "7",
        })

        malformed_role = mka_record()
        malformed_role["interface"] = "Ethernet20"
        malformed_role["participants"].append({
            "ckn": "31234567890123456789012345678912",
            "is_principal": "false",
            "is_primary": "invalid",
            "live_peers": "8",
        })

        collect_mka.side_effect = populate_mka_records([
            malformed_role,
            primary_only,
            fallback_principal_primary_not_live,
            primary_principal,
            duplicate_primary,
            fallback_principal_primary_live,
        ])
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
            "prim./fall.",
            "live",
            "peers",
            "Local-KS",
            "Keys",
            "distributed",
        ]
        assert "Key-server" not in header
        assert "SCI" not in header

        rows = {
            columns[0]: columns
            for columns in (
                line.split()
                for line in result.output.splitlines()
                if line.startswith("Ethernet")
            )
        }
        assert rows["Ethernet0"][4:6] == ["primary", "1/1"]
        assert rows["Ethernet4"][4:6] == ["fallback", "3/4"]
        assert rows["Ethernet8"][4:6] == ["fallback", "0/5"]
        assert rows["Ethernet12"][4:6] == ["primary", "6/-"]
        assert rows["Ethernet16"][4:6] == ["primary", "-/1"]
        assert rows["Ethernet20"][4:6] == ["primary", "-/-"]

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
        record = mka_record()
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
        detail = {
            label.strip(): value.strip()
            for label, value in (
                line.split(":", 1)
                for line in result.output.splitlines()
                if ":" in line
            )
        }
        assert detail["Controlled port mode"] == "secured"
        assert "Authenticated-only CP:" not in result.output
        assert "\nSecured:" not in result.output
        assert primary_ckn in result.output
        assert fallback_ckn in result.output
        assert secret_cak not in result.output
        assert secret_cak[2:] not in result.output
        assert "primary_cak" not in result.output
        assert "sak" not in result.output.lower()
        assert "Query status:" not in result.output
        assert "Last updated:" not in result.output
        assert detail["Config status"] == "in-sync"

    @patch.object(show_macsec.MacsecContext, "collect_mka", autospec=True)
    def test_show_mka_numeric_fields_redact_decimal_keys(self, collect_mka):
        record = mka_record()
        numeric_cak = "12" * 33
        record["secrets"] = [numeric_cak, numeric_cak[2:]]
        for field in (
            "actor_priority", "key_server_priority", "keys_distributed",
            "keys_received", "mka_hello_time_ms",
        ):
            record["session"][field] = numeric_cak
        for field in ("live_peers", "potential_peers", "mn"):
            record["participants"][0][field] = numeric_cak[2:]
        collect_mka.side_effect = populate_mka_records([record])

        result = CliRunner().invoke(
            show_macsec.macsec, ["--mka", "Ethernet0"]
        )

        assert result.exit_code == 0, result.output
        assert numeric_cak not in result.output
        assert numeric_cak[2:] not in result.output
        for label in (
            "Actor priority", "Key server priority", "Keys distributed",
            "Keys received", "MKA hello time",
        ):
            assert next(
                line.split(":", 1)[1].strip()
                for line in result.output.splitlines()
                if line.startswith("{}:".format(label))
            ) == "-"
        assert show_macsec._safe_uint_not_secret("34" * 33, []) == "-"

    @patch.object(show_macsec.MacsecContext, "collect_mka", autospec=True)
    def test_show_mka_detail_preserves_zero_key_server_sci(self, collect_mka):
        record = mka_record()
        record["session"]["key_server_sci"] = "0000000000000000"
        collect_mka.side_effect = populate_mka_records([record])
        runner = CliRunner()
        result = runner.invoke(show_macsec.macsec, ["--mka", "Ethernet0"])

        assert result.exit_code == 0, result.output
        detail = {
            label.strip(): value.strip()
            for label, value in (
                line.split(":", 1)
                for line in result.output.splitlines()
                if ":" in line
            )
        }
        assert detail["Key server SCI"] == "0000000000000000"

    @patch.object(show_macsec.MacsecContext, "collect_mka", autospec=True)
    def test_show_mka_missing_session_is_silent(self, collect_mka):
        collect_mka.side_effect = populate_mka_records([])
        runner = CliRunner()
        result = runner.invoke(show_macsec.macsec, ["--mka", "Ethernet0"])

        assert result.exit_code == 0
        assert result.output == ""

    @patch.object(show_macsec.MacsecContext, "collect_mka", autospec=True)
    def test_show_mka_zero_participant_session_renders_session_only(
            self, collect_mka):
        record = mka_record()
        record["participants"] = []
        collect_mka.side_effect = populate_mka_records([record])
        runner = CliRunner()

        compact = runner.invoke(show_macsec.macsec, ["--mka"])
        assert compact.exit_code == 0, compact.output
        compact_row = next(
            line.split() for line in compact.output.splitlines()
            if line.startswith("Ethernet0")
        )
        assert compact_row[0:6] == [
            "Ethernet0", "active", "true", "-", "-", "-/-"
        ]
        assert compact_row[-2:] == ["true", "7"]

        detail = runner.invoke(
            show_macsec.macsec, ["--mka", "Ethernet0"]
        )
        assert detail.exit_code == 0, detail.output
        assert "Interface:             Ethernet0" in detail.output
        assert "Controlled port mode:  secured" in detail.output
        assert "Keys distributed:      7" in detail.output
        assert "CKN  Role" not in detail.output
        assert "No MACsec MKA participant state found" not in detail.output
        assert primary_ckn not in detail.output
        assert fallback_ckn not in detail.output

    @patch.object(show_macsec.MacsecContext, "collect_mka", autospec=True)
    def test_show_mka_zero_participant_degraded_session_keeps_redacted_error(
            self, collect_mka):
        record = mka_record()
        record["participants"] = []
        record["session"]["config_status"] = "degraded"
        record["session"]["config_error"] = (
            "WPA rejected stale_cak={}".format(secret_cak)
        )
        collect_mka.side_effect = populate_mka_records([record])
        runner = CliRunner()

        result = runner.invoke(
            show_macsec.macsec, ["--mka", "Ethernet0"]
        )

        assert result.exit_code == 0, result.output
        assert "Config status:         degraded" in result.output
        assert "CONFIG ERROR:          WPA rejected stale_cak=[redacted]" in result.output
        assert secret_cak not in result.output
        assert secret_cak[2:] not in result.output
        assert "Query status:" not in result.output
        assert "Last updated:" not in result.output
        assert "CKN  Role" not in result.output
        assert "No MACsec MKA participant state found" not in result.output

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

    def test_mka_collection_skips_incomplete_snapshot_and_keeps_valid_port(
            self, capsys):
        context = show_macsec.MacsecContext.__new__(show_macsec.MacsecContext)
        context.mka_records = []
        context.config_db = MagicMock()
        context.config_db.get_entry.return_value = {
            "primary_cak": secret_cak,
        }
        context.multi_asic = MagicMock()
        context.multi_asic.current_namespace = ""
        context.db = state_db_for_snapshots(
            ["Ethernet0", "Ethernet4", "Ethernet8"],
            ["Ethernet0", "Ethernet8"],
            empty_participant_interfaces=["Ethernet8"],
        )

        show_macsec.MacsecContext.collect_mka.__wrapped__(context, None)

        assert [
            record["interface"] for record in context.mka_records
        ] == ["Ethernet0", "Ethernet4"]

        context.show_mka(None)
        output = capsys.readouterr().out
        assert "Ethernet0" in output
        assert "Ethernet4" in output
        assert "Ethernet8" not in output

    def test_mka_collection_skips_partial_session_and_keeps_valid_port(
            self, capsys):
        context = show_macsec.MacsecContext.__new__(show_macsec.MacsecContext)
        context.mka_records = []
        context.config_db = MagicMock()
        context.config_db.get_entry.return_value = {}
        context.multi_asic = MagicMock()
        context.multi_asic.current_namespace = ""
        missing_field = mka_record()["session"]
        del missing_field["keys_received"]
        context.db = state_db_for_snapshots(
            ["Ethernet0", "Ethernet4", "Ethernet8"],
            ["Ethernet0", "Ethernet4", "Ethernet8"],
            session_overrides={
                "MACSEC_MKA_SESSION_TABLE|Ethernet0": {
                    "profile": "rotation", "query_status": "error",
                },
                "MACSEC_MKA_SESSION_TABLE|Ethernet8": missing_field,
            },
        )

        show_macsec.MacsecContext.collect_mka.__wrapped__(context, None)

        assert [record["interface"] for record in context.mka_records] == [
            "Ethernet4"
        ]
        context.show_mka(None)
        output = capsys.readouterr().out
        assert "Ethernet4" in output
        assert "Ethernet0" not in output
        assert "Ethernet8" not in output

    def test_mka_collection_skips_partial_participant_and_keeps_valid_port(
            self, capsys):
        context = show_macsec.MacsecContext.__new__(show_macsec.MacsecContext)
        context.config_db = MagicMock()
        context.config_db.get_entry.return_value = {}
        context.multi_asic = MagicMock()
        context.multi_asic.current_namespace = ""
        participant_key = "MACSEC_MKA_PARTICIPANT_TABLE|Ethernet8|{}".format(
            primary_ckn
        )

        for missing_field in show_macsec.MKA_PARTICIPANT_FIELDS:
            partial = dict(mka_record()["participants"][0])
            del partial[missing_field]
            context.mka_records = []
            context.db = state_db_for_snapshots(
                ["Ethernet0", "Ethernet4", "Ethernet8"],
                ["Ethernet0", "Ethernet8"],
                participant_overrides={participant_key: partial},
            )

            show_macsec.MacsecContext.collect_mka.__wrapped__(context, None)

            assert [record["interface"] for record in context.mka_records] == [
                "Ethernet0", "Ethernet4"
            ], missing_field
            context.show_mka(None)
            output = capsys.readouterr().out
            assert "Ethernet0" in output
            assert "Ethernet4" in output
            assert "Ethernet8" not in output

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
