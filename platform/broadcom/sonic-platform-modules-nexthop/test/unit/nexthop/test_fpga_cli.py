import pytest
from click import BadParameter
from click.testing import CliRunner

from nexthop.fpga_cli import (
    read32,
    write32,
)


@pytest.mark.parametrize("pci_address", ["0000:e3:00.0", "0000:00:02.0"])
@pytest.mark.parametrize("offset", ["0x0", "0x4", "0xab0bac08", "0xac", "0xa0"])
@pytest.mark.parametrize("bits", ["0:0", "3:7", "15:21", "0:31"])
def test_read32_valid_args(pci_address, offset, bits):
    runner = CliRunner()
    result = runner.invoke(read32, [pci_address, offset, f"--bits={bits}"])
    assert result.exit_code != BadParameter.exit_code


@pytest.mark.parametrize("offset", ["0x1", "0x2", "0x3", "0xbeef"])
def test_read32_unaligned_offset(offset):
    runner = CliRunner()
    result = runner.invoke(read32, ["0000:00:02.0", offset])
    assert f"Offset ({offset}) must be 4 byte aligned" in result.output
    assert result.exit_code == BadParameter.exit_code


@pytest.mark.parametrize("offset", ["0xg", "0x", "0xcoffee"])
def test_read32_bad_hex_offset(offset):
    runner = CliRunner()
    result = runner.invoke(read32, ["0000:00:02.0", offset])
    assert f"Offset ({offset}) must be a valid hex number" in result.output
    assert result.exit_code == BadParameter.exit_code


@pytest.mark.parametrize("bits", ["-1:31", "0:", ":0", "0:15:31", "a:b"])
def test_read32_malformed_bits(bits):
    runner = CliRunner()
    result = runner.invoke(read32, ["0000:00:02.0", "0x0", f"--bits={bits}"])
    assert (
        f"'{bits}'. Expected format: '<start>:<end>' (e.g., '16:31')." in result.output
    )
    assert result.exit_code == BadParameter.exit_code


def test_read32_out_of_range_bits():
    runner = CliRunner()
    result = runner.invoke(read32, ["0000:00:02.0", "0x0", "--bits=32:127"])
    assert f"Start bit (32) and end bit (127) must be within [0, 31]" in result.output
    assert result.exit_code == BadParameter.exit_code


def test_read32_invalid_start_bits():
    runner = CliRunner()
    result = runner.invoke(read32, ["0000:00:02.0", "0x0", "--bits=5:4"])
    assert f"Start bit (5) can't be greater than end bit (4)" in result.output
    assert result.exit_code == BadParameter.exit_code


@pytest.mark.parametrize("pci_address", ["0000:e3:00.0", "0000:00:02.0"])
@pytest.mark.parametrize("offset", ["0x0", "0x4", "0xab0bac08", "0xac", "0xa0"])
@pytest.mark.parametrize("bits", ["0:0", "3:7", "15:21", "0:31"])
def test_write32_valid_args(pci_address, offset, bits):
    runner = CliRunner()
    result = runner.invoke(write32, [pci_address, offset, "0x1", f"--bits={bits}"])
    assert result.exit_code != BadParameter.exit_code


@pytest.mark.parametrize("offset", ["0x1", "0x2", "0x3", "0xbeef"])
def test_write32_unaligned_offset(offset):
    runner = CliRunner()
    result = runner.invoke(write32, ["0000:00:02.0", offset, "0xdeadbeef"])
    assert f"Offset ({offset}) must be 4 byte aligned" in result.output
    assert result.exit_code == BadParameter.exit_code


@pytest.mark.parametrize("offset", ["0xg", "0x", "0xcoffee"])
def test_write32_bad_hex_offset(offset):
    runner = CliRunner()
    result = runner.invoke(write32, ["0000:00:02.0", offset, "0xdeadbeef"])
    assert f"Offset ({offset}) must be a valid hex number" in result.output
    assert result.exit_code == BadParameter.exit_code


@pytest.mark.parametrize("bits", ["-1:31", "0:", ":0", "0:15:31", "a:b"])
def test_write32_malformed_bits(bits):
    runner = CliRunner()
    result = runner.invoke(
        write32, ["0000:00:02.0", "0x0", "0xdeadbeef", f"--bits={bits}"]
    )
    assert (
        f"'{bits}'. Expected format: '<start>:<end>' (e.g., '16:31')." in result.output
    )
    assert result.exit_code == BadParameter.exit_code


def test_write32_out_of_range_bits():
    runner = CliRunner()
    result = runner.invoke(
        write32, ["0000:00:02.0", "0x0", "0xdeadbeef", "--bits=32:127"]
    )
    assert f"Start bit (32) and end bit (127) must be within [0, 31]" in result.output
    assert result.exit_code == BadParameter.exit_code


def test_write32_invalid_start_bits():
    runner = CliRunner()
    result = runner.invoke(write32, ["0000:00:02.0", "0x0", "0xdeadbeef", "--bits=5:4"])
    assert f"Start bit (5) can't be greater than end bit (4)" in result.output
    assert result.exit_code == BadParameter.exit_code


def test_write32_value_exceeds_bits():
    runner = CliRunner()
    result = runner.invoke(write32, ["0000:00:02.0", "0x0", "0x10", "--bits=1:4"])
    assert f"value (0x10) must be smaller than or equal to 0xf" in result.output
    assert result.exit_code == BadParameter.exit_code
