#!/usr/bin/env python

"""Unit tests for the sonic_platform.dcdc module."""

import os
import tempfile

import pytest

from unittest.mock import mock_open, patch

from fixtures.test_helpers_common import mock_pddf_data


@pytest.fixture
def dcdc_module():
    """Loads the module before each test. This is to let conftest.py inject deps first."""
    from sonic_platform import dcdc

    yield dcdc


def _make_dcdc(
    dcdc_module,
    cls_name="Dcdc",
    *,
    bus=1,
    addr=0x60,
    dev_type="nh_raa228234",
    index=0,
):
    """Builds a DCDC via the PDDF-based constructor."""
    pddf_data = mock_pddf_data(
        {
            "DCDC{0}".format(index): {
                "i2c": {
                    "topo_info": {
                        "parent_bus": hex(bus),
                        "dev_addr": hex(addr),
                        "dev_type": dev_type,
                    }
                }
            }
        }
    )
    cls = getattr(dcdc_module, cls_name)
    return cls(index, pddf_data, {"DCDC": {}})


def _write_debugfs_file(dir_path: str, name: str, content: str) -> None:
    with open(os.path.join(dir_path, name), "w", encoding="utf-8") as f:
        f.write(content)


# raa_dmpvr3 blackbox record sizes, mirrored from RaaDmpvr3BlackBoxRecord.
_RAA_RAM_RECORD_SIZE = 176
_RAA_NVM_RECORD_SIZE = 184


def _build_raa_dmpvr3_raw(
    *,
    nvm: bool = False,
    header: int = 0,
    rail_uptime_counter=(0, 0),
    controller_first_fault_detect: int = 0,
    rail_first_fault_detect=(0, 0),
    phase_uc_fault_detect: int = 0,
    phase_oc_fault_detect: int = 0,
    adc_uc_fault_detect: int = 0,
    adc_oc_fault_detect: int = 0,
    status_word=(0, 0),
    status_mfr_specific: int = 0,
    status_cml: int = 0,
    status_iout=(0, 0),
    status_vout=(0, 0),
    status_input=(0, 0),
    status_temperature=(0, 0),
    read_vin=(0, 0),
    read_vout=(0, 0),
    read_iin=(0, 0),
    read_iout=(0, 0),
    rail_read_temperature=(0, 0),
    controller_read_temperature: int = 0,
    vmon_iinsen=(0, 0),
    phase_temperature=(0,) * 20,
    phase_current=(0,) * 20,
) -> bytes:
    """Builds a raw raa_dmpvr3 blackbox record from field values."""
    size = _RAA_NVM_RECORD_SIZE if nvm else _RAA_RAM_RECORD_SIZE
    raw = bytearray(size)
    base = 0
    if nvm:
        raw[0:4] = (header & 0xFFFFFFFF).to_bytes(4, "little")
        base = 4

    def u32(off, val):
        raw[base + off : base + off + 4] = (val & 0xFFFFFFFF).to_bytes(4, "little")

    def u16(off, val):
        raw[base + off : base + off + 2] = (val & 0xFFFF).to_bytes(2, "little")

    def u8(off, val):
        raw[base + off] = val & 0xFF

    u32(0, rail_uptime_counter[0])
    u32(4, rail_uptime_counter[1])
    u32(8, controller_first_fault_detect)
    u32(12, rail_first_fault_detect[0])
    u32(16, rail_first_fault_detect[1])
    u32(20, phase_uc_fault_detect)
    u32(24, phase_oc_fault_detect)
    u32(28, adc_uc_fault_detect)
    u32(32, adc_oc_fault_detect)
    u16(48, status_word[0])
    u16(50, status_word[1])
    u8(52, status_mfr_specific)
    u8(53, status_cml)
    u8(56, status_iout[0])
    u8(57, status_iout[1])
    u8(58, status_vout[0])
    u8(59, status_vout[1])
    u8(60, status_input[0])
    u8(61, status_input[1])
    u8(62, status_temperature[0])
    u8(63, status_temperature[1])
    u16(64, read_vin[0])
    u16(66, read_vin[1])
    u16(68, read_vout[0])
    u16(70, read_vout[1])
    u16(72, read_iin[0])
    u16(74, read_iin[1])
    u16(76, read_iout[0])
    u16(78, read_iout[1])
    u8(80, rail_read_temperature[0])
    u8(81, rail_read_temperature[1])
    u8(82, controller_read_temperature)
    u16(88, vmon_iinsen[0])
    u16(90, vmon_iinsen[1])
    # phase_temperature: bytes within each 4-byte group are stored in reverse.
    for p, val in enumerate(phase_temperature):
        chunk = 92 + (p // 4) * 4
        u8(chunk + 3 - (p % 4), val)
    # phase_current: the two u16s within each 4-byte group are stored swapped.
    for p, val in enumerate(phase_current):
        chunk = 112 + (p // 2) * 4
        u16(chunk + 2 - (p % 2) * 2, val)
    return bytes(raw)


class TestDcdcDecoding:
    """Tests for PMBus status decoding (bit masking and bitfield expansion)."""

    def test_status_register_post_init_filters_bitfields(self, dcdc_module):
        """Only bits enabled in `mask` should survive in `bitfields`."""
        # Given - mask allows only bits 5 and 7
        reg = dcdc_module.StatusRegister(
            name="STATUS_TEST",
            bit_width=8,
            mask=0b1010_0000,
            bitfields={
                7: "BIT7",
                6: "BIT6",
                5: "BIT5",
                4: "BIT4",
                3: "BIT3",
                2: "BIT2",
                1: "BIT1",
                0: "BIT0",
            },
            debugfs_attribute="status_test",
            is_global=True,
        )

        # Then
        assert reg.bitfields == {7: "BIT7", 5: "BIT5"}

    def test_decode_statuses_respects_mask(self, dcdc_module):
        """Bits masked out by a chip's register entry must not appear in decoded output."""
        # When - nh_raa228234 hides FAN_FAULT (STATUS_WORD bit 10)
        nh_raa = _make_dcdc(dcdc_module, "Raa228234Dcdc", bus=1, addr=0x60, dev_type="nh_raa228234")
        with tempfile.TemporaryDirectory() as tmp_dir:
            _write_debugfs_file(tmp_dir, "status0", "0x0400")
            nh_raa._debugfs_dir = tmp_dir
            decoded_nh = nh_raa.read_statuses_via_debugfs()

        # Then - nh_raa228234's mask does not include bit 10 (FAN_FAULT)
        assert decoded_nh.rail_statuses_decoded[0] == {"STATUS_WORD": []}

        # When - xdpe1a2g5b exposes FAN_FAULT
        xdpe = _make_dcdc(dcdc_module, "Xdpe1a2g5bDcdc", bus=1, addr=0x70, dev_type="xdpe1a2g5b")
        with tempfile.TemporaryDirectory() as tmp_dir:
            _write_debugfs_file(tmp_dir, "status0", "0x0400")
            xdpe._debugfs_dir = tmp_dir
            decoded_xdpe = xdpe.read_statuses_via_debugfs()

        # Then - xdpe1a2g5b's mask includes bit 10 (FAN_FAULT)
        assert decoded_xdpe.rail_statuses_decoded[0] == {"STATUS_WORD": ["FAN_FAULT"]}

    def test_decode_statuses_unknown_register_skipped(self, dcdc_module):
        """debugfs files that don't map to a known register are silently skipped."""
        # Given - a recognized STATUS_WORD file alongside an unrecognized one.
        dcdc = _make_dcdc(dcdc_module, "Raa228234Dcdc", bus=1, addr=0x60, dev_type="nh_raa228234")
        with tempfile.TemporaryDirectory() as tmp_dir:
            _write_debugfs_file(tmp_dir, "status0", "0x0001")
            _write_debugfs_file(tmp_dir, "status0_bogus", "0xFF")  # not a known register
            dcdc._debugfs_dir = tmp_dir

            # When
            record = dcdc.read_statuses_via_debugfs()

        # Then - only the recognized register is captured.
        assert record.rail_statuses[0] == {"STATUS_WORD": 0x0001}
        assert record.rail_statuses_decoded[0] == {"STATUS_WORD": ["OTHER_STATUS_CHANGE"]}

    def test_decode_statuses_empty_record(self, dcdc_module):
        """A resolved debugfs dir with no readable files yields empty status dicts."""
        # Given - a debugfs dir with no register files present.
        dcdc = _make_dcdc(dcdc_module, "Raa228234Dcdc", bus=1, addr=0x60, dev_type="nh_raa228234")
        with tempfile.TemporaryDirectory() as tmp_dir:
            dcdc._debugfs_dir = tmp_dir

            # When
            record = dcdc.read_statuses_via_debugfs()

        # Then - per-rail dicts are still present (one per rail) but empty.
        assert record.global_statuses == {}
        assert record.global_statuses_decoded == {}
        assert record.rail_statuses == [{}, {}]
        assert record.rail_statuses_decoded == [{}, {}]


class TestDcdc:
    """Tests for `Dcdc` construction and debugfs I/O."""

    def test_init_known_dev_type(self, dcdc_module):
        """A chip-specific subclass exposes that chip's register table."""
        # When
        dcdc = _make_dcdc(dcdc_module, "Raa228234Dcdc", bus=1, addr=0x60, dev_type="nh_raa228234")

        # Then
        assert dcdc.name == "DCDC0"
        assert dcdc.dev_type == "nh_raa228234"  # dev_type keeps the driver prefix
        assert dcdc.get_model() == "raa228234"  # get_model strips the nh_ prefix
        assert dcdc.NUM_RAILS == 2
        assert "STATUS_MFR_SPECIFIC" in dcdc.registers  # chip-specific register

    def test_init_unknown_dev_type_uses_default(self, dcdc_module):
        """The base Dcdc falls back to the standard PMBus register model."""
        # When
        dcdc = _make_dcdc(dcdc_module, "Dcdc", bus=1, addr=0x60, dev_type="bogus_chip")

        # Then
        assert dcdc.NUM_RAILS == 2
        assert set(dcdc.registers) == {
            "STATUS_WORD",
            "STATUS_VOUT",
            "STATUS_IOUT",
            "STATUS_INPUT",
            "STATUS_TEMPERATURE",
            "STATUS_CML",
        }

    def test_read_statuses_via_debugfs_happy_path(self, dcdc_module):
        """Real debugfs reads populate both raw values and decoded bitfields."""
        # Given
        with tempfile.TemporaryDirectory() as tmp_dir:
            # Write canned values for a subset of the nh_raa228234 register set.
            # Globals: STATUS_CML, STATUS_MFR_SPECIFIC.
            _write_debugfs_file(tmp_dir, "status0_cml", "0x02")  # OTHER_COMMUNICATION_FAULT
            _write_debugfs_file(tmp_dir, "status0_mfr", "0x08")  # BLACKBOX_EVENT
            # Per-rail (rails 0 and 1): STATUS_WORD, STATUS_VOUT.
            _write_debugfs_file(tmp_dir, "status0", "0x0801")  # POWER_BAD, OTHER_STATUS_CHANGE
            _write_debugfs_file(tmp_dir, "status0_vout", "0x80")  # VOUT_OV_FAULT
            _write_debugfs_file(tmp_dir, "status1", "0x0001")  # OTHER_STATUS_CHANGE
            _write_debugfs_file(tmp_dir, "status1_vout", "0x00")

            dcdc = _make_dcdc(dcdc_module, "Raa228234Dcdc", bus=1, addr=0x60, dev_type="nh_raa228234")
            dcdc._debugfs_dir = tmp_dir

            # When
            record = dcdc.read_statuses_via_debugfs()

        # Then - raw values
        assert record.name == "DCDC0"
        assert record.dev_type == "nh_raa228234"
        assert record.global_statuses == {
            "STATUS_CML": 0x02,
            "STATUS_MFR_SPECIFIC": 0x08,
        }
        assert record.rail_statuses == [
            {"STATUS_WORD": 0x0801, "STATUS_VOUT": 0x80},
            {"STATUS_WORD": 0x0001, "STATUS_VOUT": 0x00},
        ]

        # Then - decoded bitfields populated alongside the raw values
        assert record.global_statuses_decoded == {
            "STATUS_CML": ["OTHER_COMMUNICATION_FAULT"],
            "STATUS_MFR_SPECIFIC": ["BLACKBOX_EVENT"],
        }
        assert record.rail_statuses_decoded == [
            {
                "STATUS_WORD": ["POWER_BAD", "OTHER_STATUS_CHANGE"],
                "STATUS_VOUT": ["VOUT_OV_FAULT"],
            },
            {
                "STATUS_WORD": ["OTHER_STATUS_CHANGE"],
                "STATUS_VOUT": [],
            },
        ]

    def test_read_statuses_via_debugfs_partial_files(self, dcdc_module):
        """Missing files are skipped; an empty trailing rail is still appended as {}."""
        # Given - only rail 0's STATUS_WORD is present.
        with tempfile.TemporaryDirectory() as tmp_dir:
            _write_debugfs_file(tmp_dir, "status0", "0x0001")

            dcdc = _make_dcdc(dcdc_module, "Raa228234Dcdc", bus=1, addr=0x60, dev_type="nh_raa228234")
            dcdc._debugfs_dir = tmp_dir

            # When
            record = dcdc.read_statuses_via_debugfs()

        # Then - rail 1 read nothing but is still present so its index stays 1.
        assert record.global_statuses == {}
        assert record.rail_statuses == [{"STATUS_WORD": 0x0001}, {}]

    def test_read_statuses_via_debugfs_empty_leading_rail(self, dcdc_module):
        """An empty rail 0 is kept as {} so a populated rail 1 retains its index."""
        # Given - only rail 1's STATUS_WORD is present.
        with tempfile.TemporaryDirectory() as tmp_dir:
            _write_debugfs_file(tmp_dir, "status1", "0x0001")

            dcdc = _make_dcdc(dcdc_module, "Raa228234Dcdc", bus=1, addr=0x60, dev_type="nh_raa228234")
            dcdc._debugfs_dir = tmp_dir

            # When
            record = dcdc.read_statuses_via_debugfs()

        # Then
        assert record.global_statuses == {}
        assert record.rail_statuses == [{}, {"STATUS_WORD": 0x0001}]

    def test_read_statuses_via_debugfs_no_hwmon_match(self, dcdc_module):
        """When the debugfs dir was not resolved, returns an empty record."""
        # Given - force `_resolve_debugfs_dir` to return None during construction.
        with patch("sonic_platform.dcdc.resolve_i2c_hwmon_paths", return_value=[]):
            dcdc = _make_dcdc(dcdc_module, "Raa228234Dcdc", bus=1, addr=0x60, dev_type="nh_raa228234")
        assert dcdc._debugfs_dir is None

        # When
        record = dcdc.read_statuses_via_debugfs()

        # Then
        assert record.global_statuses == {}
        assert record.rail_statuses == []

    def test_resolve_debugfs_dir_single_hwmon(self, dcdc_module):
        """A single hwmon match resolves to /sys/kernel/debug/pmbus/<hwmon>."""
        with patch(
            "sonic_platform.dcdc.resolve_i2c_hwmon_paths",
            return_value=["/sys/bus/i2c/devices/1-0060/hwmon/hwmon7"],
        ), patch("os.path.isdir", return_value=True):
            dcdc = _make_dcdc(dcdc_module, "Dcdc", bus=1, addr=0x60, dev_type="nh_raa228234")

        assert dcdc._debugfs_dir == "/sys/kernel/debug/pmbus/hwmon7"

    def test_resolve_debugfs_dir_multiple_matches(self, dcdc_module):
        """Multiple hwmon matches are treated as ambiguous and yield None."""
        with patch(
            "sonic_platform.dcdc.resolve_i2c_hwmon_paths",
            return_value=[
                "/sys/bus/i2c/devices/1-0060/hwmon/hwmon7",
                "/sys/bus/i2c/devices/1-0060/hwmon/hwmon8",
            ],
        ):
            dcdc = _make_dcdc(dcdc_module, "Dcdc", bus=1, addr=0x60, dev_type="nh_raa228234")

        assert dcdc._debugfs_dir is None


class TestRaaDmpvr3BlackBoxRecord:
    """Tests for RaaDmpvr3BlackBoxRecord parsing, validity, and rendering."""

    def test_from_bytes_ram_parses_fields(self, dcdc_module):
        """A RAM record (no header) parses its scalar and per-rail fields."""
        # Given
        raw = _build_raa_dmpvr3_raw(
            rail_uptime_counter=(100, 200),
            controller_first_fault_detect=0x12345678,
            rail_first_fault_detect=(0x0000_0001, 0x8000_0000),
            status_word=(0x0801, 0x0001),
            status_mfr_specific=0x08,
            status_cml=0x02,
            read_vin=(1234, 5678),
            read_iout=(125, 0),
            rail_read_temperature=(25, 26),
            controller_read_temperature=30,
            vmon_iinsen=(100, 200),
        )

        # When
        record = dcdc_module.RaaDmpvr3BlackBoxRecord.from_bytes(raw, "DCDC0:nh_raa228234")

        # Then
        assert record.name == "DCDC0:nh_raa228234"
        assert record.raw == raw
        assert record.header == 0  # RAM records have no header
        assert record.rail_uptime_counter == [100, 200]
        assert record.controller_first_fault_detect == 0x12345678
        assert record.rail_first_fault_detect == [0x0000_0001, 0x8000_0000]
        assert record.status_word == [0x0801, 0x0001]
        assert record.status_mfr_specific == 0x08
        assert record.status_cml == 0x02
        assert record.read_vin == [1234, 5678]
        assert record.read_iout == [125, 0]
        assert record.rail_read_temperature == [25, 26]
        assert record.controller_read_temperature == 30
        assert record.vmon_iinsen == [100, 200]

    def test_from_bytes_inverts_phase_byte_ordering(self, dcdc_module):
        """Phase statuses are byte-swapped within each blackbox word during parsing."""
        # Given - a record with one 4-byte group set for phase_temperature and
        # phase_current at their respective base offsets.
        raw = bytearray(_RAA_RAM_RECORD_SIZE)
        # phase_temperature group
        raw[92], raw[93], raw[94], raw[95] = 0xA0, 0xA1, 0xA2, 0xA3
        # phase_current group
        raw[112:114] = (0x1111).to_bytes(2, "little")
        raw[114:116] = (0x2222).to_bytes(2, "little")

        # When
        record = dcdc_module.RaaDmpvr3BlackBoxRecord.from_bytes(bytes(raw), "d")

        # Then - the first 4 phase temperatures come from the group in reverse,
        # decoded as two's complement.
        assert record.phase_temperature[0:4] == [-93, -94, -95, -96]
        # And the two phase currents in the group are swapped (high u16 first).
        assert record.phase_current[0:2] == [0x2222, 0x1111]

    def test_from_bytes_nvm_parses_header(self, dcdc_module):
        """An NVM record (184 bytes) parses the leading 4-byte header."""
        # Given
        raw = _build_raa_dmpvr3_raw(nvm=True, header=0xDEADBEEF, rail_uptime_counter=(10, 20))

        # When
        record = dcdc_module.RaaDmpvr3BlackBoxRecord.from_bytes(raw, "d")

        # Then - the header is consumed, and the fields after it still align.
        assert len(record.raw) == _RAA_NVM_RECORD_SIZE
        assert record.header == 0xDEADBEEF
        assert record.rail_uptime_counter == [10, 20]

    def test_is_valid_ram(self, dcdc_module):
        """A RAM record is valid if any byte is non-zero."""
        empty = dcdc_module.RaaDmpvr3BlackBoxRecord.from_bytes(
            bytes(_RAA_RAM_RECORD_SIZE), "d"
        )
        assert not empty.is_valid()

        populated = dcdc_module.RaaDmpvr3BlackBoxRecord.from_bytes(
            _build_raa_dmpvr3_raw(status_word=(0x0001, 0)), "d"
        )
        assert populated.is_valid()

    def test_is_valid_nvm(self, dcdc_module):
        """An NVM record is valid if its header is non-zero."""
        # header == 0 is invalid even when the rest of the body is populated.
        no_header = dcdc_module.RaaDmpvr3BlackBoxRecord.from_bytes(
            _build_raa_dmpvr3_raw(nvm=True, header=0, status_word=(0x0001, 0)), "d"
        )
        assert not no_header.is_valid()

        with_header = dcdc_module.RaaDmpvr3BlackBoxRecord.from_bytes(
            _build_raa_dmpvr3_raw(nvm=True, header=0x1), "d"
        )
        assert with_header.is_valid()

    def test_as_dict_formats_and_scales_values(self, dcdc_module):
        """as_dict applies the per-field unit scaling and includes the device name."""
        # Given
        raw = _build_raa_dmpvr3_raw(
            rail_uptime_counter=(100, 200),  # /10 -> seconds
            read_vin=(1234, 5678),  # /100 -> volts
            read_vout=(3300, 1800),  # /1000 -> volts
            read_iout=(125, 0),  # /10 -> amps
            controller_read_temperature=30,  # *2 -> degrees C
            status_word=(0x0801, 0x0001),
            status_cml=0x02,
        )
        record = dcdc_module.RaaDmpvr3BlackBoxRecord.from_bytes(raw, "DCDC0:nh_raa228234")

        # When
        d = record.as_dict()

        # Then
        assert d["name"] == "DCDC0:nh_raa228234"
        assert d["rail_uptime[2]"] == "10.00s, 20.00s"
        assert d["read_vin[2]"] == "12.34V, 56.78V"
        assert d["read_vout[2]"] == "3.30V, 1.80V"
        assert d["read_iout[2]"] == "12.50A, 0.00A"
        assert d["controller_read_temperature"] == "60.00°C"
        assert d["status_word[2]"] == "0b0000100000000001, 0b0000000000000001"
        assert d["status_cml"] == "0b00000010"


class TestRaa228234DcdcBlackbox:
    """Tests for Raa228234Dcdc blackbox reads (RAM and NVM sources)."""

    def test_read_blackbox_record_ram_happy_path(self, dcdc_module):
        """A valid `blackbox_ram` file yields one parsed record."""
        # Given - a populated RAM record written to the chip's blackbox_ram attribute.
        raw = _build_raa_dmpvr3_raw(
            rail_uptime_counter=(100, 200),
            status_word=(0x0801, 0x0001),
        )
        with tempfile.TemporaryDirectory() as hwmon_dir:
            _write_debugfs_file(hwmon_dir, "status0_mfr", "0x08")  # BLACKBOX_EVENT set
            dev_dir = os.path.join(hwmon_dir, "nh_raa228234")
            os.makedirs(dev_dir)
            with open(os.path.join(dev_dir, "blackbox_ram"), "wb") as f:
                f.write(raw)

            dcdc = _make_dcdc(
                dcdc_module, "Raa228234Dcdc", bus=1, addr=0x60, dev_type="nh_raa228234"
            )
            dcdc._debugfs_dir = hwmon_dir

            # When
            records = dcdc.decode_blackbox_records(dcdc.get_blackbox_raw())

        # Then
        assert len(records) == 1
        assert records[0].name == "DCDC0:nh_raa228234"
        assert records[0].is_valid()
        assert records[0].rail_uptime_counter == [100, 200]
        assert records[0].status_word == [0x0801, 0x0001]

    def test_read_blackbox_record_ram_empty_record_skipped(self, dcdc_module):
        """An all-zero (invalid) RAM record is dropped, yielding no records."""
        # Given
        with tempfile.TemporaryDirectory() as hwmon_dir:
            _write_debugfs_file(hwmon_dir, "status0_mfr", "0x08")  # BLACKBOX_EVENT set
            dev_dir = os.path.join(hwmon_dir, "nh_raa228234")
            os.makedirs(dev_dir)
            with open(os.path.join(dev_dir, "blackbox_ram"), "wb") as f:
                f.write(bytes(_RAA_RAM_RECORD_SIZE))

            dcdc = _make_dcdc(
                dcdc_module, "Raa228234Dcdc", bus=1, addr=0x60, dev_type="nh_raa228234"
            )
            dcdc._debugfs_dir = hwmon_dir

            # When
            records = dcdc.decode_blackbox_records(dcdc.get_blackbox_raw())

        # Then
        assert records == []

    def test_read_blackbox_record_nvm(self, dcdc_module):
        """In NVM mode, only the valid (non-zero header) records are returned."""
        # Given - 10 NVM records back to back; only the first has a header set.
        valid = _build_raa_dmpvr3_raw(nvm=True, header=0x1234, rail_uptime_counter=(5, 6))
        invalid = _build_raa_dmpvr3_raw(nvm=True, header=0)
        blob = valid + invalid * 9
        assert len(blob) == _RAA_NVM_RECORD_SIZE * 10

        with patch("builtins.open", mock_open(read_data=blob)):
            dcdc = _make_dcdc(
                dcdc_module, "Raa228234Dcdc", bus=1, addr=0x60, dev_type="nh_raa228234"
            )

            # When
            records = dcdc.decode_blackbox_records(dcdc.get_blackbox_raw(from_ram=False))

        # Then
        assert len(records) == 1
        assert records[0].header == 0x1234
        assert records[0].rail_uptime_counter == [5, 6]


class TestRaaDmpvr3Split:
    """The RAA DMPVR3 family shares one blackbox base with per-chip register tables."""

    @pytest.mark.parametrize(
        "dev_type, expected_cls",
        [
            ("nh_raa228234", "Raa228234Dcdc"),
            ("nh_raa228236", "Raa228236Dcdc"),
            ("nh_raa228244", "Raa228244Dcdc"),
        ],
    )
    def test_dev_type_dispatches_to_chip_subclass(self, dcdc_module, dev_type, expected_cls):
        # When - the base Dcdc is constructed for each RAA dev_type.
        dcdc = _make_dcdc(dcdc_module, "Dcdc", bus=1, addr=0x60, dev_type=dev_type)

        # Then - it dispatches to the chip subclass over the shared DMPVR3 base,
        # and the chip's register table (incl. its MFR-specific bits) is built.
        assert type(dcdc).__name__ == expected_cls
        assert isinstance(dcdc, dcdc_module.RaaDmpvr3Dcdc)
        assert "STATUS_MFR_SPECIFIC" in dcdc.registers

    @pytest.mark.parametrize(
        "dev_type", ["nh_raa228234", "nh_raa228236", "nh_raa228244"]
    )
    def test_all_raa_chips_share_inherited_blackbox(self, dcdc_module, dev_type):
        # Given - a populated RAM record for a chip reached via dev_type dispatch.
        raw = _build_raa_dmpvr3_raw(rail_uptime_counter=(1, 2), status_word=(0x0801, 0x0001))
        with tempfile.TemporaryDirectory() as hwmon_dir:
            _write_debugfs_file(hwmon_dir, "status0_mfr", "0x08")  # BLACKBOX_EVENT set
            dev_dir = os.path.join(hwmon_dir, dev_type)
            os.makedirs(dev_dir)
            with open(os.path.join(dev_dir, "blackbox_ram"), "wb") as f:
                f.write(raw)

            dcdc = _make_dcdc(dcdc_module, "Dcdc", bus=1, addr=0x60, dev_type=dev_type)
            dcdc._debugfs_dir = hwmon_dir

            # When - the blackbox is read/decoded via the inherited DMPVR3 methods.
            records = dcdc.decode_blackbox_records(dcdc.get_blackbox_raw())

        # Then
        assert len(records) == 1
        assert records[0].name == f"DCDC0:{dev_type}"
        assert records[0].rail_uptime_counter == [1, 2]
        assert records[0].status_word == [0x0801, 0x0001]
