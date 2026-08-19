import sys

from unittest import mock
from click.testing import CliRunner

sys.path.append('../cli/config/plugins/')
import macsec


profile_name = "test"
primary_cak = "2363647040534355560e000802065d574d400e000e030307075f0e5050000e5541"
primary_ckn = "01234567890123456789012345678912"
fallback_cak = "1159485744465e5a537272050a1011073557475152020c0e040c57223a357d7d71"
fallback_ckn = "6162636465666768696A6B6C6D6E6F70"
rotated_cak = "3946080a0407070303530256560a0450465053035256040c57223a357d7d715541"
rotated_ckn = "98765432109876543210987654321098"
rotated_fallback_cak = "0c0e040c57223a357d7d711159485744465e5a537272050a10110735574751520c"
rotated_fallback_ckn = "7172737475767778797A7B7C7D7E7F80"


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


    def test_macsec_fallback_profile(self, mock_cfgdb):
        cfgdb = mock_cfgdb
        runner = CliRunner()

        result = runner.invoke(macsec.macsec, ["profile", "add", profile_name,
                "--primary_cak=" + primary_cak, "--primary_ckn=" + primary_ckn,
                "--fallback_cak=" + fallback_cak, "--fallback_ckn=" + fallback_ckn],
                obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        profile_table = cfgdb.get_entry("MACSEC_PROFILE", profile_name)
        assert profile_table["fallback_cak"] == fallback_cak
        assert profile_table["fallback_ckn"] == fallback_ckn

    def test_macsec_invalid_fallback_profile(self, mock_cfgdb):
        cfgdb = mock_cfgdb
        runner = CliRunner()

        # Fallback CAK without a fallback CKN
        result = runner.invoke(macsec.macsec, ["profile", "add", profile_name,
                "--primary_cak=" + primary_cak, "--primary_ckn=" + primary_ckn,
                "--fallback_cak=" + fallback_cak], obj=cfgdb)
        assert result.exit_code != 0

        # Fallback CKN without a fallback CAK
        result = runner.invoke(macsec.macsec, ["profile", "add", profile_name,
                "--primary_cak=" + primary_cak, "--primary_ckn=" + primary_ckn,
                "--fallback_ckn=" + fallback_ckn], obj=cfgdb)
        assert result.exit_code != 0

        # Fallback CKN colliding with the primary CKN
        result = runner.invoke(macsec.macsec, ["profile", "add", profile_name,
                "--primary_cak=" + primary_cak, "--primary_ckn=" + primary_ckn,
                "--fallback_cak=" + fallback_cak, "--fallback_ckn=" + primary_ckn],
                obj=cfgdb)
        assert result.exit_code != 0

        # Fallback CAK of the wrong length for the cipher suite
        result = runner.invoke(macsec.macsec, ["profile", "add", profile_name,
                "--primary_cak=" + primary_cak, "--primary_ckn=" + primary_ckn,
                "--fallback_cak=" + primary_ckn, "--fallback_ckn=" + fallback_ckn],
                obj=cfgdb)
        assert result.exit_code != 0

        # Fallback CAK that is not a hex string
        result = runner.invoke(macsec.macsec, ["profile", "add", profile_name,
                "--primary_cak=" + primary_cak, "--primary_ckn=" + primary_ckn,
                "--fallback_cak=" + "z" * 66, "--fallback_ckn=" + fallback_ckn],
                obj=cfgdb)
        assert result.exit_code != 0

    def test_macsec_profile_update(self, mock_cfgdb):
        cfgdb = mock_cfgdb
        runner = CliRunner()

        result = runner.invoke(macsec.macsec, ["profile", "add", profile_name,
                "--primary_cak=" + primary_cak, "--primary_ckn=" + primary_ckn,
                "--fallback_cak=" + fallback_cak, "--fallback_ckn=" + fallback_ckn],
                obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

        # Attach the profile so the update takes the hot-update path
        result = runner.invoke(macsec.macsec, ["port", "add", "Ethernet0", profile_name], obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

        # The old CKN of the primary selects the primary key
        result = runner.invoke(macsec.macsec, ["profile", "update", profile_name,
                "--old_ckn=" + primary_ckn, "--new_ckn=" + rotated_ckn,
                "--new_cak=" + rotated_cak], obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        profile_table = cfgdb.get_entry("MACSEC_PROFILE", profile_name)
        assert profile_table["primary_cak"] == rotated_cak
        assert profile_table["primary_ckn"] == rotated_ckn
        # The fallback and every other field are left alone
        assert profile_table["fallback_cak"] == fallback_cak
        assert profile_table["fallback_ckn"] == fallback_ckn
        assert profile_table["priority"] == "255"
        assert profile_table["cipher_suite"] == "GCM-AES-128"
        assert profile_table["policy"] == "security"
        assert profile_table["send_sci"] == "true"

        # The old CKN of the fallback selects the fallback key
        result = runner.invoke(macsec.macsec, ["profile", "update", profile_name,
                "--old_ckn=" + fallback_ckn, "--new_ckn=" + rotated_fallback_ckn,
                "--new_cak=" + rotated_fallback_cak], obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        profile_table = cfgdb.get_entry("MACSEC_PROFILE", profile_name)
        assert profile_table["fallback_cak"] == rotated_fallback_cak
        assert profile_table["fallback_ckn"] == rotated_fallback_ckn
        assert profile_table["primary_cak"] == rotated_cak
        assert profile_table["primary_ckn"] == rotated_ckn

        # The old CKN is matched case insensitively, as CKNs are hex strings
        result = runner.invoke(macsec.macsec, ["profile", "update", profile_name,
                "--old_ckn=" + rotated_fallback_ckn.swapcase(),
                "--new_ckn=" + fallback_ckn, "--new_cak=" + fallback_cak],
                obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        profile_table = cfgdb.get_entry("MACSEC_PROFILE", profile_name)
        assert profile_table["fallback_ckn"] == fallback_ckn

        result = runner.invoke(macsec.macsec, ["port", "del", "Ethernet0"], obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

        result = runner.invoke(macsec.macsec, ["profile", "del", profile_name], obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_macsec_profile_update_requires_new_ckn(self, mock_cfgdb):
        cfgdb = mock_cfgdb
        runner = CliRunner()

        result = runner.invoke(macsec.macsec, ["profile", "add", profile_name,
                "--primary_cak=" + primary_cak, "--primary_ckn=" + primary_ckn,
                "--fallback_cak=" + fallback_cak, "--fallback_ckn=" + fallback_ckn],
                obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

        # A CA is keyed by its CKN, so re-keying the primary under the same CKN
        # is rejected rather than silently deferred to the next restart
        result = runner.invoke(macsec.macsec, ["profile", "update", profile_name,
                "--old_ckn=" + primary_ckn, "--new_ckn=" + primary_ckn,
                "--new_cak=" + rotated_cak], obj=cfgdb)
        assert result.exit_code != 0

        # Including when the CKN only differs in case
        result = runner.invoke(macsec.macsec, ["profile", "update", profile_name,
                "--old_ckn=" + fallback_ckn, "--new_ckn=" + fallback_ckn.swapcase(),
                "--new_cak=" + rotated_fallback_cak], obj=cfgdb)
        assert result.exit_code != 0

        profile_table = cfgdb.get_entry("MACSEC_PROFILE", profile_name)
        assert profile_table["primary_cak"] == primary_cak
        assert profile_table["primary_ckn"] == primary_ckn
        assert profile_table["fallback_cak"] == fallback_cak
        assert profile_table["fallback_ckn"] == fallback_ckn

    def test_macsec_profile_update_rejects_other_fields(self, mock_cfgdb):
        cfgdb = mock_cfgdb
        runner = CliRunner()

        result = runner.invoke(macsec.macsec, ["profile", "add", profile_name,
                "--primary_cak=" + primary_cak, "--primary_ckn=" + primary_ckn],
                obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

        # 'update' only rotates a key, every other field is fixed at 'add'
        for option in ("--priority=64", "--cipher_suite=GCM-AES-256",
                       "--policy=integrity_only", "--no_send_sci",
                       "--send_sci", "--rekey_period=1800",
                       "--enable_replay_protect", "--disable_replay_protect",
                       "--replay_window=100", "--remove_fallback",
                       "--primary_cak=" + rotated_cak,
                       "--primary_ckn=" + rotated_ckn,
                       "--fallback_cak=" + fallback_cak,
                       "--fallback_ckn=" + fallback_ckn):
            result = runner.invoke(macsec.macsec, ["profile", "update", profile_name,
                    "--old_ckn=" + primary_ckn, "--new_ckn=" + rotated_ckn,
                    "--new_cak=" + rotated_cak, option], obj=cfgdb)
            assert result.exit_code != 0, "{} was accepted".format(option)

        # The stored profile is untouched by the rejected updates
        profile_table = cfgdb.get_entry("MACSEC_PROFILE", profile_name)
        assert profile_table["primary_cak"] == primary_cak
        assert profile_table["primary_ckn"] == primary_ckn
        assert profile_table["priority"] == "255"
        assert profile_table["cipher_suite"] == "GCM-AES-128"
        assert profile_table["policy"] == "security"
        assert profile_table["send_sci"] == "true"

    def test_macsec_invalid_profile_update(self, mock_cfgdb):
        cfgdb = mock_cfgdb
        runner = CliRunner()

        # Update a profile that doesn't exist
        result = runner.invoke(macsec.macsec, ["profile", "update", profile_name,
                "--old_ckn=" + primary_ckn, "--new_ckn=" + rotated_ckn,
                "--new_cak=" + rotated_cak], obj=cfgdb)
        assert result.exit_code != 0

        result = runner.invoke(macsec.macsec, ["profile", "add", profile_name,
                "--primary_cak=" + primary_cak, "--primary_ckn=" + primary_ckn],
                obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

        # Each of the three key fields is mandatory
        result = runner.invoke(macsec.macsec, ["profile", "update", profile_name], obj=cfgdb)
        assert result.exit_code != 0
        result = runner.invoke(macsec.macsec, ["profile", "update", profile_name,
                "--new_ckn=" + rotated_ckn, "--new_cak=" + rotated_cak], obj=cfgdb)
        assert result.exit_code != 0
        result = runner.invoke(macsec.macsec, ["profile", "update", profile_name,
                "--old_ckn=" + primary_ckn, "--new_cak=" + rotated_cak], obj=cfgdb)
        assert result.exit_code != 0
        result = runner.invoke(macsec.macsec, ["profile", "update", profile_name,
                "--old_ckn=" + primary_ckn, "--new_ckn=" + rotated_ckn], obj=cfgdb)
        assert result.exit_code != 0

        # An old CKN that names no configured key
        result = runner.invoke(macsec.macsec, ["profile", "update", profile_name,
                "--old_ckn=" + fallback_ckn, "--new_ckn=" + rotated_ckn,
                "--new_cak=" + rotated_cak], obj=cfgdb)
        assert result.exit_code != 0

        # A new CAK of the wrong length for the cipher suite of the profile
        result = runner.invoke(macsec.macsec, ["profile", "update", profile_name,
                "--old_ckn=" + primary_ckn, "--new_ckn=" + rotated_ckn,
                "--new_cak=" + primary_ckn], obj=cfgdb)
        assert result.exit_code != 0

        # A new CAK that is not a hex string
        result = runner.invoke(macsec.macsec, ["profile", "update", profile_name,
                "--old_ckn=" + primary_ckn, "--new_ckn=" + rotated_ckn,
                "--new_cak=" + "z" * 66], obj=cfgdb)
        assert result.exit_code != 0

        # A new CKN that is not a hex string
        result = runner.invoke(macsec.macsec, ["profile", "update", profile_name,
                "--old_ckn=" + primary_ckn, "--new_ckn=zzzz",
                "--new_cak=" + rotated_cak], obj=cfgdb)
        assert result.exit_code != 0

        profile_table = cfgdb.get_entry("MACSEC_PROFILE", profile_name)
        assert profile_table["primary_cak"] == primary_cak
        assert profile_table["primary_ckn"] == primary_ckn

        result = runner.invoke(macsec.macsec, ["profile", "del", profile_name], obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_macsec_profile_update_without_fallback(self, mock_cfgdb):
        cfgdb = mock_cfgdb
        runner = CliRunner()

        result = runner.invoke(macsec.macsec, ["profile", "add", profile_name,
                "--primary_cak=" + primary_cak, "--primary_ckn=" + primary_ckn],
                obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

        # An unattached profile can be rotated without a fallback covering it
        result = runner.invoke(macsec.macsec, ["profile", "update", profile_name,
                "--old_ckn=" + primary_ckn, "--new_ckn=" + rotated_ckn,
                "--new_cak=" + rotated_cak], obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        profile_table = cfgdb.get_entry("MACSEC_PROFILE", profile_name)
        assert profile_table["primary_ckn"] == rotated_ckn

        # Once it is attached the rotation would strand the port
        result = runner.invoke(macsec.macsec, ["port", "add", "Ethernet0", profile_name], obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        result = runner.invoke(macsec.macsec, ["profile", "update", profile_name,
                "--old_ckn=" + rotated_ckn, "--new_ckn=" + primary_ckn,
                "--new_cak=" + primary_cak], obj=cfgdb)
        assert result.exit_code != 0
        profile_table = cfgdb.get_entry("MACSEC_PROFILE", profile_name)
        assert profile_table["primary_ckn"] == rotated_ckn

        result = runner.invoke(macsec.macsec, ["port", "del", "Ethernet0"], obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

    def test_macsec_profile_reject_promote_fallback(self, mock_cfgdb):
        cfgdb = mock_cfgdb
        runner = CliRunner()

        result = runner.invoke(macsec.macsec, ["profile", "add", profile_name,
                "--primary_cak=" + primary_cak, "--primary_ckn=" + primary_ckn,
                "--fallback_cak=" + fallback_cak, "--fallback_ckn=" + fallback_ckn],
                obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        result = runner.invoke(macsec.macsec, ["port", "add", "Ethernet0", profile_name], obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)

        # Promoting the standing fallback to primary is not a rotation
        result = runner.invoke(macsec.macsec, ["profile", "update", profile_name,
                "--old_ckn=" + primary_ckn, "--new_ckn=" + fallback_ckn,
                "--new_cak=" + fallback_cak], obj=cfgdb)
        assert result.exit_code != 0

        # Nor is retiring the primary by rotating the fallback onto its CKN
        result = runner.invoke(macsec.macsec, ["profile", "update", profile_name,
                "--old_ckn=" + fallback_ckn, "--new_ckn=" + primary_ckn,
                "--new_cak=" + primary_cak], obj=cfgdb)
        assert result.exit_code != 0

        profile_table = cfgdb.get_entry("MACSEC_PROFILE", profile_name)
        assert profile_table["primary_cak"] == primary_cak
        assert profile_table["primary_ckn"] == primary_ckn
        assert profile_table["fallback_cak"] == fallback_cak
        assert profile_table["fallback_ckn"] == fallback_ckn

        # Rotating the primary onto a fresh CKN is accepted, the fallback covers it
        result = runner.invoke(macsec.macsec, ["profile", "update", profile_name,
                "--old_ckn=" + primary_ckn, "--new_ckn=" + rotated_ckn,
                "--new_cak=" + rotated_cak], obj=cfgdb)
        assert result.exit_code == 0, "exit code: {}, Exception: {}, Traceback: {}".format(result.exit_code, result.exception, result.exc_info)
        profile_table = cfgdb.get_entry("MACSEC_PROFILE", profile_name)
        assert profile_table["primary_cak"] == rotated_cak
        assert profile_table["primary_ckn"] == rotated_ckn
        assert profile_table["fallback_cak"] == fallback_cak
        assert profile_table["fallback_ckn"] == fallback_ckn

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
