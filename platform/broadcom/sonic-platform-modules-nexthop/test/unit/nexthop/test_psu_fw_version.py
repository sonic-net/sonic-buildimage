# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for nexthop.psu_fw_version."""

import subprocess
from unittest.mock import patch

import pytest


@pytest.fixture(scope="function")
def psu_fw_version():
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    from nexthop import psu_fw_version

    yield psu_fw_version


def reply(stdout="", returncode=0, stderr=""):
    return subprocess.CompletedProcess(args=[], returncode=returncode, stdout=stdout, stderr=stderr)


MURATA = b"D1U74T-W-2400-12-HB4C"


def model_block(model):
    return [len(model), *model]


# The model reads that precede every version read for a Murata PSU.
MODEL_REPLIES = [reply(f"0x{len(MURATA):02x}\n"), reply(" ".join(f"0x{b:02x}" for b in model_block(MURATA)) + "\n")]


def fake_psu(block, model=MURATA):
    """subprocess.run side effect for a PSU whose MFR_MODEL is `model` and whose block at
    any other register is `block` (length byte, then payload): each `rN` read gets the
    first N bytes, 0xff after the block's end."""

    def run(cmd, **kwargs):
        n = int(cmd[-1][1:])
        data = model_block(model) if cmd[-2] == "0x9a" else block
        raw = (list(data) + [0xFF] * n)[:n]
        return reply(" ".join(f"0x{b:02x}" for b in raw) + "\n")

    return run


def reads(i2ctransfer):
    """The rN length of each i2ctransfer call after the MFR_MODEL reads."""
    return [call.args[0][-1] for call in i2ctransfer.call_args_list if call.args[0][-2] != "0x9a"]


@pytest.fixture
def i2ctransfer(psu_fw_version):
    """Patches subprocess.run; set .return_value or .side_effect per test."""
    with patch.object(psu_fw_version.subprocess, "run") as run:
        yield run


def run(module, capsys, *extra):
    module.main(["--bus", "87", "--address", "0x58", *extra])
    return capsys.readouterr().out.strip()


@pytest.mark.parametrize(
    "data,expected",
    [
        # Murata PSU on NH-5010; 5-byte reply
        (bytes([0x02, 0x01, 0x00, 0x00, 0x00]), "0.1.2"),
        # Murata PSU on NH-4010
        (bytes([0x01, 0x01, 0x00]), "0.1.1"),
        # Bel PSU on NH-4220; leading zero byte is not lost
        (bytes([0x00, 0x01, 0x00]), "0.1.0"),
        # A zero byte in the middle is not lost
        (bytes([0x05, 0x00, 0x01]), "1.0.5"),
        # Bit 7 of the major byte is masked
        (bytes([0x03, 0x02, 0x81]), "1.2.3"),
    ],
)
def test_format_version(psu_fw_version, data, expected):
    assert psu_fw_version.format_version(data) == expected


def test_format_version_too_short(psu_fw_version):
    with pytest.raises(ValueError):
        psu_fw_version.format_version(bytes([0x02, 0x01]))


def test_reads_length_byte_then_block(psu_fw_version, i2ctransfer, capsys):
    i2ctransfer.side_effect = fake_psu([3, 0x02, 0x01, 0x00])
    assert run(psu_fw_version, capsys) == "0.1.2"
    assert [call.args[0] for call in i2ctransfer.call_args_list] == [
        ["i2ctransfer", "-f", "-y", "87", "w1@0x58", "0x9a", "r1"],
        ["i2ctransfer", "-f", "-y", "87", "w1@0x58", "0x9a", f"r{len(MURATA) + 1}"],
        ["i2ctransfer", "-f", "-y", "87", "w1@0x58", "0xd9", "r1"],
        ["i2ctransfer", "-f", "-y", "87", "w1@0x58", "0xd9", "r4"],
    ]


def test_reads_whole_longer_block(psu_fw_version, i2ctransfer, capsys):
    """A 7-byte block (Artesyn CSU2400AT-3-5M0) is read to its end, no further."""
    i2ctransfer.side_effect = fake_psu([7, 0xE6, 0xE6, 0x8A, 0x00, 0x00, 0x01, 0x00])
    assert run(psu_fw_version, capsys) == "10.230.230"
    assert reads(i2ctransfer) == ["r1", "r8"]


def test_length_byte_capped_at_smbus_block_max(psu_fw_version, i2ctransfer, capsys):
    i2ctransfer.side_effect = fake_psu([0x40, 0x02, 0x01, 0x00])
    assert run(psu_fw_version, capsys) == "0.1.2"
    assert reads(i2ctransfer) == ["r1", "r33"]


@pytest.mark.parametrize(
    "block,expected",
    [
        # Murata D1U74T-W-3200 (humm223): 5-byte block
        ([5, 0x02, 0x01, 0x00, 0x00, 0x00], "0.1.2"),
        # Artesyn CSU2400AT-3-5M0 (gold404): 7-byte block
        ([7, 0xE6, 0xE6, 0x8A, 0x00, 0x00, 0x01, 0x00], "10.230.230"),
        # Murata D1U74T-W-2400
        ([3, 0x01, 0x01, 0x00], "0.1.1"),
        # Bel TEC5200 (blkt202): the old driver showed 0.0.0
        ([3, 0x00, 0x01, 0x00], "0.1.0"),
        # Artesyn CSU2400AT (gold107): bit 7 of the major byte is masked
        ([3, 0x0F, 0x0F, 0x80], "0.15.15"),
        # AcBel NWR002000
        ([3, 0x04, 0x04, 0x01], "1.4.4"),
        # AcBel FSQ015, both PSU firmware images seen on seag163
        ([3, 0x04, 0x06, 0x01], "1.6.4"),
        ([3, 0x07, 0x06, 0x5B], "91.6.7"),
    ],
)
def test_real_psus(psu_fw_version, i2ctransfer, capsys, block, expected):
    i2ctransfer.side_effect = fake_psu(block)
    assert run(psu_fw_version, capsys) == expected
    assert reads(i2ctransfer) == ["r1", f"r{block[0] + 1}"]


@pytest.mark.parametrize(
    "model,block,expected",
    [
        # harp159 PSU2: 0x9B is ASCII "05 06"; its 0xD9 holds c5 ff ff ff ff ff
        (b"DPS1500AB", [5, *b"05 06"], "05 06"),
        # Trailing padding is dropped
        (b"DPS1500AB", [6, *b"05 06 "], "05 06"),
    ],
)
def test_delta_reads_mfr_revision(psu_fw_version, i2ctransfer, capsys, model, block, expected):
    i2ctransfer.side_effect = fake_psu(block, model=model)
    assert run(psu_fw_version, capsys) == expected
    registers = [call.args[0][-2] for call in i2ctransfer.call_args_list]
    assert registers == ["0x9a", "0x9a", "0x9b", "0x9b"]


@pytest.mark.parametrize(
    "block",
    [
        [0],
        [5, *b"     "],
        [5, 0xC5, 0xFF, 0xFF, 0xFF, 0xFF],
        [3, *b"0\x0016"],
    ],
)
def test_delta_na_bad_mfr_revision(psu_fw_version, i2ctransfer, capsys, block):
    i2ctransfer.side_effect = fake_psu(block, model=b"DPS1500AB")
    assert run(psu_fw_version, capsys) == "N/A"


@pytest.mark.parametrize(
    "model,block,expected",
    [
        # harp205 PSU2: 0xD9 holds 01 00 00; its 0x9B is the hardware revision "S0"
        (b"DPS-1800AB-14 A", [3, 0x01, 0x00, 0x00], "0.0.1"),
        (b"TDPS-5200CB A", [3, 0x02, 0x01, 0x00], "0.1.2"),
    ],
)
def test_other_delta_models_use_register(psu_fw_version, i2ctransfer, capsys, model, block, expected):
    """Only DPS1500AB uses MFR_REVISION; other Delta PSUs keep --register."""
    i2ctransfer.side_effect = fake_psu(block, model=model)
    assert run(psu_fw_version, capsys) == expected
    assert [call.args[0][-2] for call in i2ctransfer.call_args_list] == ["0x9a", "0x9a", "0xd9", "0xd9"]


def test_second_read_shorter_than_requested(psu_fw_version, i2ctransfer, capsys):
    """Only the bytes that came back are used."""
    i2ctransfer.side_effect = [*MODEL_REPLIES, reply("0x05\n"), reply("0x05 0x02 0x01 0x00\n")]
    assert run(psu_fw_version, capsys) == "0.1.2"


@pytest.mark.parametrize(
    "result",
    [
        # Empty slot: the address NACKs
        reply(returncode=1, stderr="Error: Sending messages failed: No such device or address"),
        # Exits non-zero without a message
        reply(returncode=2),
        # No output
        reply(""),
        # Not hex
        reply("Error: garbage\n"),
    ],
)
def test_na(psu_fw_version, i2ctransfer, capsys, result):
    i2ctransfer.return_value = result
    assert run(psu_fw_version, capsys) == "N/A"


def test_na_short_block(psu_fw_version, i2ctransfer, capsys):
    i2ctransfer.side_effect = fake_psu([2, 0x02, 0x01])
    assert run(psu_fw_version, capsys) == "N/A"


def test_na_empty_block_reads_once(psu_fw_version, i2ctransfer, capsys):
    i2ctransfer.side_effect = fake_psu([0])
    assert run(psu_fw_version, capsys) == "N/A"
    assert reads(i2ctransfer) == ["r1"]


def test_na_when_second_read_fails(psu_fw_version, i2ctransfer, capsys):
    i2ctransfer.side_effect = [*MODEL_REPLIES, reply("0x03\n"), reply(returncode=1, stderr="Error: Sending messages failed")]
    assert run(psu_fw_version, capsys) == "N/A"


@pytest.mark.parametrize(
    "exc",
    [
        subprocess.TimeoutExpired(cmd="i2ctransfer", timeout=3),
        # i2ctransfer is not installed
        FileNotFoundError(2, "No such file or directory", "i2ctransfer"),
    ],
)
def test_na_when_i2ctransfer_fails_to_run(psu_fw_version, i2ctransfer, capsys, exc):
    i2ctransfer.side_effect = exc
    assert run(psu_fw_version, capsys) == "N/A"


@pytest.mark.parametrize(
    "args",
    [
        # Reserved by the I2C spec, refused by i2ctransfer
        ["--bus", "87", "--address", "0x07", "--register", "0xD9"],
        ["--bus", "87", "--address", "0x78", "--register", "0xD9"],
        # Not a 7-bit address
        ["--bus", "87", "--address", "0xb0", "--register", "0xD9"],
        # Not a command code
        ["--bus", "87", "--address", "0x58", "--register", "0x1D9"],
    ],
)
def test_bad_args(psu_fw_version, i2ctransfer, args):
    with pytest.raises(SystemExit):
        psu_fw_version.main(args)
    i2ctransfer.assert_not_called()


def test_register_defaults_to_0xd9(psu_fw_version, i2ctransfer, capsys):
    i2ctransfer.side_effect = fake_psu([3, 0x02, 0x01, 0x00])
    assert run(psu_fw_version, capsys) == "0.1.2"
    assert [call.args[0][-2] for call in i2ctransfer.call_args_list] == ["0x9a", "0x9a", "0xd9", "0xd9"]


@pytest.mark.parametrize("model", [MURATA, b"DPS1500AB"])
def test_register_override_skips_model(psu_fw_version, i2ctransfer, capsys, model):
    """An explicit --register is read as-is, even on a DPS1500AB."""
    i2ctransfer.side_effect = fake_psu([3, 0x02, 0x01, 0x00], model=model)
    assert run(psu_fw_version, capsys, "--register", "0xE0") == "0.1.2"
    assert {call.args[0][-2] for call in i2ctransfer.call_args_list} == {"0xe0"}


@pytest.mark.parametrize("address", ["0x08", "0x77"])
def test_address_bounds_accepted(psu_fw_version, i2ctransfer, capsys, address):
    i2ctransfer.side_effect = fake_psu([3, 0x02, 0x01, 0x00])
    psu_fw_version.main(["--bus", "87", "--address", address, "--register", "0xD9"])
    assert capsys.readouterr().out.strip() == "0.1.2"
    assert i2ctransfer.call_args.args[0][4] == f"w1@{address}"


def test_logs_with_tag(psu_fw_version, i2ctransfer, capsys):
    """Errors are logged once under the psu_fw_version tag, not prefixed again."""
    i2ctransfer.return_value = reply(returncode=1, stderr="Error: Sending messages failed")
    with patch.object(psu_fw_version.syslog, "openlog") as openlog, \
         patch.object(psu_fw_version.syslog, "syslog") as log:
        assert run(psu_fw_version, capsys) == "N/A"
    openlog.assert_called_once_with("psu_fw_version")
    assert not log.call_args.args[1].startswith("psu_fw_version")
    assert "87-0058" in log.call_args.args[1]


@pytest.mark.parametrize(
    "result,level",
    [
        # Empty slot: the address NACKs, which is expected and not an error
        (reply(returncode=1, stderr="Error: Sending messages failed: No such device or address"), "LOG_DEBUG"),
        # Any other transfer failure is
        (reply(returncode=1, stderr="Error: Sending messages failed: Connection timed out"), "LOG_ERR"),
        (reply(returncode=2), "LOG_ERR"),
    ],
)
def test_log_level(psu_fw_version, i2ctransfer, capsys, result, level):
    i2ctransfer.return_value = result
    with patch.object(psu_fw_version.syslog, "syslog") as log:
        assert run(psu_fw_version, capsys) == "N/A"
    log.assert_called_once()
    assert log.call_args.args[0] == getattr(psu_fw_version.syslog, level)
