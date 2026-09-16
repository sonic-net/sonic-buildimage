import sys
import datetime

from unittest import mock
from click.testing import CliRunner

sys.path.append('../cli/config/plugins/')
import macsec


profile_name = "test"
primary_cak = "2363647040534355560e000802065d574d400e000e030307075f0e5050000e5541"
primary_ckn = "01234567890123456789012345678912"
fallback_cak = "3363647040534355560e000802065d574d400e000e030307075f0e5050000e5541"
fallback_ckn = "11234567890123456789012345678912"
replacement_cak = "4363647040534355560e000802065d574d400e000e030307075f0e5050000e5541"
replacement_ckn = "21234567890123456789012345678912"


def create_state_db(profile, ports, unsafe_port=None):
    state_db = mock.MagicMock()
    state_db.STATE_DB = "STATE_DB"
    state_db.get_db_separator.return_value = "|"

    session_rows = {}
    participant_rows = {}
    for port in ports:
        session_rows["MACSEC_MKA_SESSION_TABLE|{}".format(port)] = {
            "profile": profile,
            "kay_status": "active",
            "authenticated": "true",
            "secured": "true",
            "failed": "false",
            "query_status": "error" if port == unsafe_port else "ok",
            "config_status": "in-sync",
            "last_updated": datetime.datetime.now(
                datetime.timezone.utc
            ).isoformat().replace("+00:00", "Z"),
        }
        participant_rows[
            "MACSEC_MKA_PARTICIPANT_TABLE|{}|{}".format(port, primary_ckn)
        ] = {
            "is_primary": "true",
            "active": "true",
            "live_peers": "1",
        }
        participant_rows[
            "MACSEC_MKA_PARTICIPANT_TABLE|{}|{}".format(port, fallback_ckn)
        ] = {
            "is_primary": "false",
            "active": "true",
            "live_peers": "1",
        }

    def get_all(db_name, key):
        return session_rows.get(key, participant_rows.get(key, {}))

    def keys(db_name, pattern):
        prefix = pattern[:-1]
        return [key for key in participant_rows if key.startswith(prefix)]

    state_db.get_all.side_effect = get_all
    state_db.keys.side_effect = keys
    return state_db


class TestConfigMACsec(object):
    def test_plugin_registration(self):
        cli = mock.MagicMock()
        macsec.register(cli)
        cli.add_command.assert_called_once_with(macsec.macsec)

    def test_default_profile(self, mock_cfgdb):
        cfgdb = mock_cfgdb
        runner = CliRunner()
        result = runner.invoke(macsec.macsec,
                ["profile", "add", profile_name, "--primary_cak=" + primary_cak,"--primary_ckn=" + primary_ckn],
                obj=cfgdb)
        assert result.exit_code == 0
        profile_table = cfgdb.get_entry("MACSEC_PROFILE", profile_name)
        assert profile_table
        assert profile_table["priority"] == "255"
        assert profile_table["cipher_suite"] == "GCM-AES-128"
        assert profile_table["primary_cak"] == primary_cak
        assert profile_table["primary_ckn"] == primary_ckn
        assert profile_table["policy"] == "security"
        assert "enable_replay_protect" not in profile_table
        assert "replay_window" not in profile_table
        assert profile_table["send_sci"] == "true"
        assert "rekey_period" not in profile_table

        result = runner.invoke(macsec.macsec, ["profile", "del", profile_name], obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        profile_table = cfgdb.get_entry("MACSEC_PROFILE", profile_name)
        assert not profile_table

    def test_macsec_valid_profile(self, mock_cfgdb):
        cfgdb = mock_cfgdb
        runner = CliRunner()

        profile_name = "test"
        profile_map = {
            "primary_cak": "3946080a0407070303530256560a04504650530352565e731f1a5c4f524f4b5a5e547b79777c6663754b5e465253050d0d0503565a48470b0b030604020c520a54",
            "primary_ckn": "01234567890123456789012345678912",
            "priority": 64,
            "cipher_suite": "GCM-AES-XPN-256",
            "policy": "integrity_only",
            "enable_replay_protect": None,
            "replay_window": 100,
            "no_send_sci": None,
            "rekey_period": 30 * 60,
        }
        options = [profile_name]
        for k, v in profile_map.items():
            options.append("--" + k)
            if v is not None:
                options[-1] += "=" + str(v)

        result = runner.invoke(macsec.macsec, ["profile", "add"] + options, obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        profile_table = cfgdb.get_entry("MACSEC_PROFILE", profile_name)
        assert profile_table
        assert profile_table["priority"] == str(profile_map["priority"])
        assert profile_table["cipher_suite"] == profile_map["cipher_suite"]
        assert profile_table["primary_cak"] == profile_map["primary_cak"]
        assert profile_table["primary_ckn"] == profile_map["primary_ckn"]
        assert profile_table["policy"] == profile_map["policy"]
        if "enable_replay_protect" in profile_map:
            assert "enable_replay_protect" in profile_table and profile_table["enable_replay_protect"] == "true"
            assert profile_table["replay_window"] == str(profile_map["replay_window"])
        if "send_sci" in profile_map:
            assert profile_table["send_sci"] == "true"
        if "no_send_sci" in profile_map:
            assert profile_table["send_sci"] == "false"
        if "rekey_period" in profile_map:
            assert profile_table["rekey_period"] == str(profile_map["rekey_period"])

    def test_fallback_profile_validation(self, mock_cfgdb):
        cfgdb = mock_cfgdb
        runner = CliRunner()

        result = runner.invoke(
            macsec.macsec,
            [
                "profile", "add", profile_name,
                "--primary_cak=" + primary_cak,
                "--primary_ckn=" + primary_ckn,
                "--fallback_cak=" + fallback_cak,
            ],
            obj=cfgdb,
        )
        assert result.exit_code != 0
        assert "must be supplied together" in result.output

        result = runner.invoke(
            macsec.macsec,
            [
                "profile", "add", profile_name,
                "--primary_cak=" + primary_cak,
                "--primary_ckn=" + primary_ckn,
                "--fallback_cak=" + fallback_cak,
                "--fallback_ckn=" + primary_ckn.upper(),
            ],
            obj=cfgdb,
        )
        assert result.exit_code != 0
        assert "must differ" in result.output

        result = runner.invoke(
            macsec.macsec,
            [
                "profile", "add", profile_name,
                "--primary_cak=" + primary_cak,
                "--primary_ckn=" + primary_ckn,
                "--fallback_cak=" + fallback_cak,
                "--fallback_ckn=" + fallback_ckn,
            ],
            obj=cfgdb,
        )
        assert result.exit_code == 0
        profile_table = cfgdb.get_entry("MACSEC_PROFILE", profile_name)
        assert profile_table["fallback_cak"] == fallback_cak
        assert profile_table["fallback_ckn"] == fallback_ckn

    def test_profile_rejects_noncanonical_hex_and_encoded_cak_salt(self, mock_cfgdb):
        cfgdb = mock_cfgdb
        runner = CliRunner()

        invalid_ckn = "0x" + primary_ckn[2:]
        result = runner.invoke(
            macsec.macsec,
            [
                "profile", "add", profile_name,
                "--primary_cak=" + primary_cak,
                "--primary_ckn=" + invalid_ckn,
            ],
            obj=cfgdb,
        )
        assert result.exit_code != 0
        assert "primary_ckn" in result.output

        for invalid_cak in (
            "ff" + primary_cak[2:],
            "53" + primary_cak[2:],
        ):
            result = runner.invoke(
                macsec.macsec,
                [
                    "profile", "add", profile_name,
                    "--primary_cak=" + invalid_cak,
                    "--primary_ckn=" + primary_ckn,
                ],
                obj=cfgdb,
            )
            assert result.exit_code != 0
            assert "primary_cak" in result.output
            assert "salt index between 00 and 52" in result.output

    def test_update_unattached_profile_by_old_ckn(self, mock_cfgdb):
        cfgdb = mock_cfgdb
        runner = CliRunner()
        result = runner.invoke(
            macsec.macsec,
            [
                "profile", "add", profile_name,
                "--primary_cak=" + primary_cak,
                "--primary_ckn=" + primary_ckn,
                "--fallback_cak=" + fallback_cak,
                "--fallback_ckn=" + fallback_ckn,
                "--priority=7",
            ],
            obj=cfgdb,
        )
        assert result.exit_code == 0

        result = runner.invoke(
            macsec.macsec,
            [
                "profile", "update", profile_name,
                "--old_ckn=" + fallback_ckn,
                "--new_ckn=" + replacement_ckn,
                "--new_cak=" + replacement_cak,
            ],
            obj=cfgdb,
        )
        assert result.exit_code == 0
        profile_table = cfgdb.get_entry("MACSEC_PROFILE", profile_name)
        assert profile_table["primary_cak"] == primary_cak
        assert profile_table["primary_ckn"] == primary_ckn
        assert profile_table["fallback_cak"] == replacement_cak
        assert profile_table["fallback_ckn"] == replacement_ckn
        assert profile_table["priority"] == "7"

    def test_update_rejects_ambiguous_ckn_replacements(self, mock_cfgdb):
        cfgdb = mock_cfgdb
        runner = CliRunner()
        result = runner.invoke(
            macsec.macsec,
            [
                "profile", "add", profile_name,
                "--primary_cak=" + primary_cak,
                "--primary_ckn=" + primary_ckn,
                "--fallback_cak=" + fallback_cak,
                "--fallback_ckn=" + fallback_ckn,
            ],
            obj=cfgdb,
        )
        assert result.exit_code == 0

        result = runner.invoke(
            macsec.macsec,
            [
                "profile", "update", profile_name,
                "--old_ckn=" + primary_ckn,
                "--new_ckn=" + primary_ckn.upper(),
                "--new_cak=" + replacement_cak,
            ],
            obj=cfgdb,
        )
        assert result.exit_code != 0
        assert "must differ from old_ckn" in result.output

        result = runner.invoke(
            macsec.macsec,
            [
                "profile", "update", profile_name,
                "--old_ckn=" + primary_ckn,
                "--new_ckn=" + fallback_ckn.upper(),
                "--new_cak=" + replacement_cak,
            ],
            obj=cfgdb,
        )
        assert result.exit_code != 0
        assert "must differ from the other configured CKN" in result.output

        result = runner.invoke(
            macsec.macsec,
            [
                "profile", "update", profile_name,
                "--old_ckn=" + replacement_ckn,
                "--new_ckn=" + primary_ckn,
                "--new_cak=" + replacement_cak,
            ],
            obj=cfgdb,
        )
        assert result.exit_code != 0
        assert "does not match" in result.output

    @mock.patch("macsec.SonicV2Connector")
    def test_update_attached_profile_with_safe_alternate(self, connector, mock_cfgdb):
        cfgdb = mock_cfgdb
        runner = CliRunner()
        result = runner.invoke(
            macsec.macsec,
            [
                "profile", "add", profile_name,
                "--primary_cak=" + primary_cak,
                "--primary_ckn=" + primary_ckn,
                "--fallback_cak=" + fallback_cak,
                "--fallback_ckn=" + fallback_ckn,
            ],
            obj=cfgdb,
        )
        assert result.exit_code == 0
        result = runner.invoke(
            macsec.macsec,
            ["port", "add", "Ethernet0", profile_name],
            obj=cfgdb,
        )
        assert result.exit_code == 0

        connector.return_value = create_state_db(profile_name, ["Ethernet0"])
        result = runner.invoke(
            macsec.macsec,
            [
                "profile", "update", profile_name,
                "--old_ckn=" + primary_ckn,
                "--new_ckn=" + replacement_ckn,
                "--new_cak=" + replacement_cak,
            ],
            obj=cfgdb,
        )
        assert result.exit_code == 0, result.output
        profile_table = cfgdb.get_entry("MACSEC_PROFILE", profile_name)
        assert profile_table["primary_cak"] == replacement_cak
        assert profile_table["primary_ckn"] == replacement_ckn
        assert profile_table["fallback_cak"] == fallback_cak
        assert profile_table["fallback_ckn"] == fallback_ckn
        connector.assert_called_once_with(
            use_unix_socket_path=True, namespace=""
        )

    @mock.patch("macsec.SonicV2Connector")
    def test_update_is_all_or_nothing_across_attached_ports(self, connector, mock_cfgdb):
        cfgdb = mock_cfgdb
        runner = CliRunner()
        result = runner.invoke(
            macsec.macsec,
            [
                "profile", "add", profile_name,
                "--primary_cak=" + primary_cak,
                "--primary_ckn=" + primary_ckn,
                "--fallback_cak=" + fallback_cak,
                "--fallback_ckn=" + fallback_ckn,
            ],
            obj=cfgdb,
        )
        assert result.exit_code == 0
        cfgdb.set_entry(
            "PORT", "Ethernet0",
            {"admin_status": "up", "macsec": profile_name},
        )
        cfgdb.set_entry(
            "PORT", "Ethernet4",
            {"admin_status": "up", "macsec": profile_name},
        )

        connector.return_value = create_state_db(
            profile_name, ["Ethernet0", "Ethernet4"], unsafe_port="Ethernet4"
        )
        result = runner.invoke(
            macsec.macsec,
            [
                "profile", "update", profile_name,
                "--old_ckn=" + primary_ckn,
                "--new_ckn=" + replacement_ckn,
                "--new_cak=" + replacement_cak,
            ],
            obj=cfgdb,
        )
        assert result.exit_code != 0
        assert "Ethernet4" in result.output
        assert "query_status is error" in result.output
        profile_table = cfgdb.get_entry("MACSEC_PROFILE", profile_name)
        assert profile_table["primary_cak"] == primary_cak
        assert profile_table["primary_ckn"] == primary_ckn
        assert replacement_cak not in result.output


    def test_macsec_invalid_profile(self, mock_cfgdb):
        cfgdb = mock_cfgdb
        runner = CliRunner()

        # Loss primary cak and primary ckn
        result = runner.invoke(macsec.macsec, ["profile", "add", "test"], obj=cfgdb)
        assert result.exit_code != 0

        # Invalid primary cak
        result = runner.invoke(macsec.macsec, ["profile", "add", "test",
                "--primary_cak=abcdfghjk90123456789012345678912","--primary_ckn=01234567890123456789012345678912",
                "--cipher_suite=GCM-AES-128"], obj=cfgdb)
        assert result.exit_code != 0

        # Invalid primary cak length
        result = runner.invoke(macsec.macsec, ["profile", "add", "test",
                "--primary_cak=01234567890123456789012345678912","--primary_ckn=01234567890123456789012345678912",
                "--cipher_suite=GCM-AES-256"], obj=cfgdb)
        assert result.exit_code != 0


    def test_macsec_port(self, mock_cfgdb):
        cfgdb = mock_cfgdb
        runner = CliRunner()

        result = runner.invoke(macsec.macsec, ["profile", "add", "test",
                "--primary_cak=2363647040534355560e000802065d574d400e000e030307075f0e5050000e5541","--primary_ckn=01234567890123456789012345678912"],
                obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        result = runner.invoke(macsec.macsec, ["port", "add", "Ethernet0", "test"], obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        port_table = cfgdb.get_entry("PORT", "Ethernet0")
        assert port_table 
        assert port_table["macsec"] == "test"
        assert port_table["admin_status"] == "up"

        result = runner.invoke(macsec.macsec, ["profile", "del", "test"], obj=cfgdb)
        assert result.exit_code != 0

        result = runner.invoke(macsec.macsec, ["port", "del", "Ethernet0"], obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        port_table = cfgdb.get_entry("PORT", "Ethernet0")
        assert "macsec" not in port_table or not port_table["macsec"]
        assert port_table["admin_status"] == "up"

        # Test deleting on port without it enabled
        result = runner.invoke(macsec.macsec, ["port", "del", "Ethernet0"], obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)


    def test_macsec_invalid_operation(self, mock_cfgdb):
        cfgdb = mock_cfgdb
        runner = CliRunner()

        # Enable nonexisted profile 
        result = runner.invoke(macsec.macsec, ["port", "add", "Ethernet0", "test"], obj=cfgdb)
        assert result.exit_code != 0

        # Delete nonexisted profile
        result = runner.invoke(macsec.macsec, ["profile", "del", "test"], obj=cfgdb)
        assert result.exit_code != 0

        result = runner.invoke(macsec.macsec, ["profile", "add", "test", "--primary_cak=2363647040534355560e000802065d574d400e000e030307075f0e5050000e5541","--primary_ckn=01234567890123456789012345678912"], obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        # Repeat add profile
        result = runner.invoke(macsec.macsec, ["profile", "add", "test", "--primary_cak=2363647040534355560e000802065d574d400e000e030307075f0e5050000e5541","--primary_ckn=01234567890123456789012345678912"], obj=cfgdb)
        assert result.exit_code != 0
