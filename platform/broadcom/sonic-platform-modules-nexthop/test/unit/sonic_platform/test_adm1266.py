#!/usr/bin/env python

import datetime
import pytest
import textwrap

from fixtures import test_helpers_adm1266
from fixtures.test_helpers_adm1266 import create_raw_adm1266_blackbox_record
from fixtures.test_helpers_common import temp_file


UNIX_EPOCH = datetime.datetime.fromtimestamp(0, tz=datetime.timezone.utc)


@pytest.fixture
def adm1266_module():
    """Loads the module before each test. This is to let conftest.py run first."""
    from sonic_platform import adm1266

    yield adm1266


class TestAdm1266BlackBoxRecord:
    """Test class for Adm1266BlackBoxRecord functionality."""

    def test_custom_epoch_in_adm1266_matches_expected(self, adm1266_module):
        # To make sure the test is not broken by a change in the custom epoch.
        assert adm1266_module.CUSTOM_EPOCH == test_helpers_adm1266.EXPECTED_CUSTOM_EPOCH

    def test_from_bytes(self, adm1266_module):
        # Given
        raw_bytes = create_raw_adm1266_blackbox_record(
            uid=1234,
            empty=False,
            vh_ov=0b0101,  # VH3, VH1
            vh_uv=0b1010,  # VH4, VH2
            current_state=9,
            last_state=8,
            vp_ov=0b1_0011_0011_0011,  # VP13, VP10, VP9, VP6, VP5, VP2, VP1
            vp_uv=0b0_1100_1100_1100,  # VP12, VP11, VP8, VP7, VP4, VP3
            gpio_in=0b1100_1100_0001,  # GPI7, GPI6, GPI9, GPI8, GPI1
            gpio_out=0b0011_0000_0110,  # GPO5, GPO4, GPO3, GPO2
            pdio_in=0b0000_0000_0000_0000,
            pdio_out=0b1111_1111_1111_1111,  # PDO16-1
            powerup_counter=456,
            timestamp=datetime.timedelta(seconds=1234),
            crc=0xAB,
        )

        # When
        record = adm1266_module.Adm1266BlackBoxRecord.from_bytes(
            raw_bytes,
            "test-dpm",
        )

        # Then
        assert record.uid == 1234
        assert record.vh_over_voltage.pins == [
            adm1266_module.Pin(adm1266_module.PinName.VH3),
            adm1266_module.Pin(adm1266_module.PinName.VH1),
        ]
        assert record.vh_under_voltage.pins == [
            adm1266_module.Pin(adm1266_module.PinName.VH4),
            adm1266_module.Pin(adm1266_module.PinName.VH2),
        ]
        assert record.current_state == 9
        assert record.last_state == 8
        assert record.vp_over_voltage.pins == [
            adm1266_module.Pin(adm1266_module.PinName.VP13),
            adm1266_module.Pin(adm1266_module.PinName.VP10),
            adm1266_module.Pin(adm1266_module.PinName.VP9),
            adm1266_module.Pin(adm1266_module.PinName.VP6),
            adm1266_module.Pin(adm1266_module.PinName.VP5),
            adm1266_module.Pin(adm1266_module.PinName.VP2),
            adm1266_module.Pin(adm1266_module.PinName.VP1),
        ]
        assert record.vp_under_voltage.pins == [
            adm1266_module.Pin(adm1266_module.PinName.VP12),
            adm1266_module.Pin(adm1266_module.PinName.VP11),
            adm1266_module.Pin(adm1266_module.PinName.VP8),
            adm1266_module.Pin(adm1266_module.PinName.VP7),
            adm1266_module.Pin(adm1266_module.PinName.VP4),
            adm1266_module.Pin(adm1266_module.PinName.VP3),
        ]
        assert record.gpio_in.pins == [
            adm1266_module.Pin(adm1266_module.PinName.GPI7),
            adm1266_module.Pin(adm1266_module.PinName.GPI6),
            adm1266_module.Pin(adm1266_module.PinName.GPI9),
            adm1266_module.Pin(adm1266_module.PinName.GPI8),
            adm1266_module.Pin(adm1266_module.PinName.GPI1),
        ]
        assert record.gpio_out.pins == [
            adm1266_module.Pin(adm1266_module.PinName.GPO5),
            adm1266_module.Pin(adm1266_module.PinName.GPO4),
            adm1266_module.Pin(adm1266_module.PinName.GPO3),
            adm1266_module.Pin(adm1266_module.PinName.GPO2),
        ]
        assert record.pdio_in.pins == []
        assert record.pdio_out.pins == [
            adm1266_module.Pin(adm1266_module.PinName.PDO16),
            adm1266_module.Pin(adm1266_module.PinName.PDO15),
            adm1266_module.Pin(adm1266_module.PinName.PDO14),
            adm1266_module.Pin(adm1266_module.PinName.PDO13),
            adm1266_module.Pin(adm1266_module.PinName.PDO12),
            adm1266_module.Pin(adm1266_module.PinName.PDO11),
            adm1266_module.Pin(adm1266_module.PinName.PDO10),
            adm1266_module.Pin(adm1266_module.PinName.PDO9),
            adm1266_module.Pin(adm1266_module.PinName.PDO8),
            adm1266_module.Pin(adm1266_module.PinName.PDO7),
            adm1266_module.Pin(adm1266_module.PinName.PDO6),
            adm1266_module.Pin(adm1266_module.PinName.PDO5),
            adm1266_module.Pin(adm1266_module.PinName.PDO4),
            adm1266_module.Pin(adm1266_module.PinName.PDO3),
            adm1266_module.Pin(adm1266_module.PinName.PDO2),
            adm1266_module.Pin(adm1266_module.PinName.PDO1),
        ]
        assert record.powerup_counter == 456
        assert record.timestamp == datetime.timedelta(seconds=1234)
        assert record.crc == 0xAB
        assert record.raw == raw_bytes
        assert record.dpm_name == "test-dpm"

    def test_from_bytes_with_custom_epoch_timestamp(self, adm1266_module):
        # Given
        TIMESTAMP = datetime.datetime(2025, 10, 20, 11, 25, 30, tzinfo=datetime.timezone.utc)
        raw_bytes = create_raw_adm1266_blackbox_record(
            uid=1234,
            timestamp=TIMESTAMP,
        )
        # When
        record = adm1266_module.Adm1266BlackBoxRecord.from_bytes(raw_bytes, "test-dpm")
        # Then
        assert record.timestamp == TIMESTAMP

    def test_non_empty_record_is_valid(self, adm1266_module):
        # Given
        raw_bytes = create_raw_adm1266_blackbox_record(uid=1234, empty=False)
        # When
        record = adm1266_module.Adm1266BlackBoxRecord.from_bytes(raw_bytes, "test-dpm")
        # Then
        assert record.is_valid()

    def test_empty_record_is_not_valid(self, adm1266_module):
        # Given
        raw_bytes = create_raw_adm1266_blackbox_record(uid=1234, empty=True)
        # When
        record = adm1266_module.Adm1266BlackBoxRecord.from_bytes(raw_bytes, "test-dpm")
        # Then
        assert not record.is_valid()

    def test_set_metadata_and_process_power_fault_cause(self, adm1266_module):
        # Given
        raw_bytes = create_raw_adm1266_blackbox_record(
            uid=1234,
            empty=False,
            pdio_in=0b0111_1010_1000_1000,  # PDI15, PDI14, PDI13, PDI12, PDI10, PDI8, PDI4
            gpio_in=0b0000_0000_0000_0000,
            vh_ov=0b0001,  # VH1
            vp_uv=0b0_0000_0000_0110,  # VP3, VP2
            timestamp=datetime.timedelta(seconds=30),
        )
        record = adm1266_module.Adm1266BlackBoxRecord.from_bytes(raw_bytes, "test-dpm")

        # When
        PLATFORM_SPEC_1 = {
            "dpm_signal_to_fault_cause": [
                {
                    "pdio_mask": "0b0000_0000_1000_1000",  # PDI8, PDI4
                    "gpio_mask": "0b0000_0000_0000_0000",
                    "pdio_value": "0b0000_0000_1000_1000",  # PDI8, PDI4
                    "gpio_value": "0b0000_0000_0000_0000",
                    "hw_cause": "HW_CAUSE_OF_PDI8_AND_PDI4",
                    "hw_desc": "HW_DESC_OF_PDI8_AND_PDI4",
                    "reboot_cause": "REBOOT_CAUSE_OF_PDI8_AND_PDI4",
                },
            ]
        }
        record.set_metadata_and_process_power_fault_cause(PLATFORM_SPEC_1)
        # Then
        assert record.get_power_fault_cause() == adm1266_module.RebootCause(
            type=adm1266_module.RebootCause.Type.HARDWARE,
            source="test-dpm",
            timestamp=datetime.timedelta(seconds=30),
            cause="HW_CAUSE_OF_PDI8_AND_PDI4",
            description="HW_DESC_OF_PDI8_AND_PDI4",
            chassis_reboot_cause_category="REBOOT_CAUSE_OF_PDI8_AND_PDI4",
        )

        # But When
        PLATFORM_SPEC_2 = {
            "dpm_signal_to_fault_cause": [
                {
                    "pdio_mask": "0b0000_0000_0000_0001",  # PDI1
                    "gpio_mask": "0b0000_0000_0000_0000",
                    "pdio_value": "0b0000_0000_0000_0001",  # PDI1
                    "gpio_value": "0b0000_0000_0000_0000",
                    "hw_cause": "HW_CAUSE_OF_PDI1",
                    "hw_desc": "HW_DESC_OF_PDI1",
                    "reboot_cause": "REBOOT_CAUSE_OF_PDI1",
                },
            ]
        }
        record.set_metadata_and_process_power_fault_cause(PLATFORM_SPEC_2)
        # Then
        assert record.get_power_fault_cause() is None

    def test_as_dict(self, adm1266_module):
        # Given
        raw_bytes = create_raw_adm1266_blackbox_record(
            uid=1234,
            empty=False,
            action_index=1,
            rule_index=3,
            vh_ov=0b0101,  # VH3, VH1
            vh_uv=0b1010,  # VH4, VH2
            current_state=9,
            last_state=8,
            vp_ov=0b1_0011_0011_0011,  # VP13, VP10, VP9, VP6, VP5, VP2, VP1
            vp_uv=0b0_1100_1100_1100,  # VP12, VP11, VP8, VP7, VP4, VP3
            gpio_in=0b1100_1100_0001,  # GPI7, GPI6, GPI9, GPI8, GPI1
            gpio_out=0b0011_0000_0110,  # GPO5, GPO4, GPO3, GPO2
            pdio_in=0b0000_0000_0001_0000,  # PDI5
            pdio_out=0b0000_0000_0000_0000,
            powerup_counter=456,
            timestamp=datetime.timedelta(seconds=1234),
            crc=0xAB,
        )
        record = adm1266_module.Adm1266BlackBoxRecord.from_bytes(raw_bytes, "test-dpm")
        PLATFORM_SPEC = {
            "dpm_signal_to_fault_cause": [
                {
                    "pdio_mask": "0b0000_0000_0001_0000",  # PDI5
                    "gpio_mask": "0b0000_0000_0000_0000",
                    "pdio_value": "0b0000_0000_0001_0000",  # PDI5
                    "gpio_value": "0b0000_0000_0000_0000",
                    "hw_cause": "HW_CAUSE_OF_PDI5",
                    "hw_desc": "some human-friendly message",
                    "reboot_cause": "REBOOT_CAUSE_OF_PDI5",
                },
            ],
            "pin_to_name": {
                "VH1": "POS12V",
                "VH2": "POS5V0",
                "VH4": "POS5V0_S0",
                "VP1": "POS1V0_A7",
                "VP2": "POS1V8_V7",
                "VP3": "POS3V3",
                "VP4": "POS1V0",
                "VP5": "POS1V2_A7",
                "VP6": "POS0V75_S5",
                "VP7": "POS1V8_S5",
                "VP8": "POS3V3_S5",
                "VP9": "POS1V1_S0",
                "VP10": "POS0V78_S0",
                "VP11": "POS0V75_S0",
                "VP12": "POS1V8_S0",
                "VP13": "POS3V3_S0",
                "PDI5": "FPGA_CMD_PCYC",
            },
        }
        record.set_metadata_and_process_power_fault_cause(PLATFORM_SPEC)
        # When
        record_dict = record.as_dict()
        # Then
        assert record_dict == {
            "dpm_name": "test-dpm",
            "timestamp": "1234.000000s after power-on",
            "power_fault_cause": "HW_CAUSE_OF_PDI5 (some human-friendly message); under_voltage: VH4(POS5V0_S0),VH2(POS5V0),VP12(POS1V8_S0),VP11(POS0V75_S0),VP8(POS3V3_S5),VP7(POS1V8_S5),VP4(POS1V0),VP3(POS3V3); over_voltage: VH3,VH1(POS12V),VP13(POS3V3_S0),VP10(POS0V78_S0),VP9(POS1V1_S0),VP6(POS0V75_S5),VP5(POS1V2_A7),VP2(POS1V8_V7),VP1(POS1V0_A7)",
            "uid": "1234",
            "byte_2": "0x00",
            "action_index": "1",
            "rule_index": "3",
            "vh_over_voltage_[4:1]": "0b0101 [VH3, VH1(POS12V)]",
            "vh_under_voltage_[4:1]": "0b1010 [VH4(POS5V0_S0), VH2(POS5V0)]",
            "current_state": "9",
            "last_state": "8",
            "vp_over_voltage_[13:1]": "0b1001100110011 [VP13(POS3V3_S0), VP10(POS0V78_S0), VP9(POS1V1_S0), VP6(POS0V75_S5), VP5(POS1V2_A7), VP2(POS1V8_V7), VP1(POS1V0_A7)]",
            "vp_under_voltage_[13:1]": "0b0110011001100 [VP12(POS1V8_S0), VP11(POS0V75_S0), VP8(POS3V3_S5), VP7(POS1V8_S5), VP4(POS1V0), VP3(POS3V3)]",
            "gpio_in_[7:4,9:8,R,R,R,3:1]": "0b110011000001 [GPI7, GPI6, GPI9, GPI8, GPI1]",
            "gpio_out_[7:4,9:8,R,R,R,3:1]": "0b001100000110 [GPO5, GPO4, GPO3, GPO2]",
            "pdio_in_[16:1]": "0b0000000000010000 [PDI5(FPGA_CMD_PCYC)]",
            "pdio_out_[16:1]": "0b0000000000000000",
            "powerup_counter": "456",
            "crc": "0xab",
            "raw": textwrap.dedent(
                """\
                d2 04 00 01 03 a5 09 00
                08 00 33 13 cc 0c c1 0c
                06 03 10 00 00 00 c8 01
                00 00 d2 04 00 00 00 00
                ff ff ff ff ff ff ff ff
                ff ff ff ff ff ff ff ff
                ff ff ff ff ff ff ff ff
                ff ff ff ff ff ff ff ab"""
            ),
        }

    def test_as_dict_with_custom_epoch_timestamp(self, adm1266_module):
        # Given
        TIMESTAMP = datetime.datetime(2025, 10, 20, 11, 25, 30, tzinfo=datetime.timezone.utc)
        raw_bytes = create_raw_adm1266_blackbox_record(
            uid=1234,
            timestamp=TIMESTAMP,
        )
        record = adm1266_module.Adm1266BlackBoxRecord.from_bytes(raw_bytes, "test-dpm")
        # When
        record_dict = record.as_dict()
        # Then
        assert record_dict["timestamp"] == "2025-10-20 11:25:30 UTC"

    def test_record_unique_name(self, adm1266_module):
        # Given
        raw_bytes = create_raw_adm1266_blackbox_record(uid=1234, powerup_counter=456)
        record = adm1266_module.Adm1266BlackBoxRecord.from_bytes(raw_bytes, "test-dpm")
        record_dict = record.as_dict()
        # When
        unique_name = adm1266_module.record_unique_name(record_dict)
        # Then
        assert unique_name == "test-dpm:powerup_456:uid_1234"

    def test_trim_record_dict(self, adm1266_module):
        # Given
        raw_bytes = create_raw_adm1266_blackbox_record(
            uid=1234,
            vh_uv=0b0001,  # VH1
            pdio_in=0b0000_0000_0001_0000,  # PDI5
            powerup_counter=456,
            timestamp=datetime.datetime(2025, 10, 20, 11, 25, 30, tzinfo=datetime.timezone.utc),
        )
        record = adm1266_module.Adm1266BlackBoxRecord.from_bytes(raw_bytes, "test-dpm")
        record_dict = record.as_dict()
        # When
        trimmed_record_dict = adm1266_module.trim_record_dict(record_dict)
        # Then
        assert trimmed_record_dict == {
            "timestamp": "2025-10-20 11:25:30 UTC",
            "power_fault_cause": "n/a; under_voltage: VH1; over_voltage: n/a",
            "gpio_in_[7:4,9:8,R,R,R,3:1]": "0b000000000000",
            "pdio_in_[16:1]": "0b0000000000010000 [PDI5]",
        }


class TestAdm1266:
    """Test class for Adm1266 functionality."""

    def test_get_name(self, adm1266_module):
        # Given
        adm1266 = adm1266_module.Adm1266(
            "test-dpm",
            platform_spec={"nvmem_path": "/dummy/path", "powerup_counter_path": "/dummy/path"},
        )
        # When
        name = adm1266.get_name()
        # Then
        assert name == "test-dpm"

    def test_get_type(self, adm1266_module):
        # Given
        adm1266 = adm1266_module.Adm1266(
            "test-dpm",
            platform_spec={"nvmem_path": "/dummy/path", "powerup_counter_path": "/dummy/path"},
        )
        # When
        dpm_type = adm1266.get_type()
        # Then
        assert dpm_type == adm1266_module.DpmType.ADM1266

    def test_set_rtc_epoch_offset_with_default_value(self, adm1266_module):
        # Given
        with temp_file(
            content="",
            file_prefix="rtc_epoch_offset",
            dir_prefix="test_adm1266",
        ) as rtc_epoch_offset_path:
            platform_spec = {
                "nvmem_path": "/dummy/path",
                "powerup_counter_path": "/dummy/path",
                "rtc_epoch_offset_path": rtc_epoch_offset_path,
            }
            # When
            adm1266 = adm1266_module.Adm1266("test-dpm", platform_spec)
            adm1266.set_rtc_epoch_offset()
            # Then
            with open(rtc_epoch_offset_path, "r") as file:
                assert file.read() == "1704067200"  # 1704067200s past 1970-01-01 is 2024-01-01.

    def test_set_rtc_epoch_offset_with_custom_value(self, adm1266_module):
        # Given
        with temp_file(
            content="",
            file_prefix="rtc_epoch_offset",
            dir_prefix="test_adm1266",
        ) as rtc_epoch_offset_path:
            platform_spec = {
                "nvmem_path": "/dummy/path",
                "powerup_counter_path": "/dummy/path",
                "rtc_epoch_offset_path": rtc_epoch_offset_path,
            }
            # When
            adm1266 = adm1266_module.Adm1266("test-dpm", platform_spec)
            adm1266.set_rtc_epoch_offset(1234567)
            # Then
            with open(rtc_epoch_offset_path, "r") as file:
                assert file.read() == "1234567"

    def test_read_raw_records(self, adm1266_module):
        # Given
        RECORDS = [
            create_raw_adm1266_blackbox_record(uid=1234, empty=False),
            create_raw_adm1266_blackbox_record(uid=9999, empty=True),
            create_raw_adm1266_blackbox_record(uid=1235, empty=False),
        ]
        INCOMPLETE_RECORD_BYTES = b"1" * (
            adm1266_module.Adm1266BlackBoxRecord.RECORD_SIZE_IN_BYTES - 1
        )
        with temp_file(
            content=b"".join(RECORDS) + INCOMPLETE_RECORD_BYTES,
            file_prefix="nvmem",
            dir_prefix="test_adm1266",
        ) as nvmem_path:
            adm1266 = adm1266_module.Adm1266(
                "test-dpm",
                platform_spec={"nvmem_path": nvmem_path, "powerup_counter_path": "/dummy/path"},
            )
            # When
            read_records = adm1266.read_raw_records()
            # Then
            assert read_records == RECORDS

    def test_read_raw_records_skip_empty_or_cleared_records(self, adm1266_module):
        # Given
        RECORDS = [
            b"\x00" * adm1266_module.Adm1266BlackBoxRecord.RECORD_SIZE_IN_BYTES,
            create_raw_adm1266_blackbox_record(uid=1234, empty=False),
            b"\xff" * adm1266_module.Adm1266BlackBoxRecord.RECORD_SIZE_IN_BYTES,
        ]
        with temp_file(
            content=b"".join(RECORDS),
            file_prefix="nvmem",
            dir_prefix="test_adm1266",
        ) as nvmem_path:
            adm1266 = adm1266_module.Adm1266(
                "test-dpm",
                platform_spec={"nvmem_path": nvmem_path, "powerup_counter_path": "/dummy/path"},
            )
            # When
            read_records = adm1266.read_raw_records()
            # Then
            assert read_records == [RECORDS[1]]

    def test_read_records(self, adm1266_module):
        # Given
        RECORDS = [
            create_raw_adm1266_blackbox_record(uid=1234, empty=False, powerup_counter=1),
            create_raw_adm1266_blackbox_record(uid=9999, empty=True),
            create_raw_adm1266_blackbox_record(uid=1235, empty=False),
        ]
        with temp_file(
            content=b"".join(RECORDS), file_prefix="nvmem", dir_prefix="test_adm1266"
        ) as nvmem_path:
            adm1266 = adm1266_module.Adm1266(
                "test-dpm",
                platform_spec={"nvmem_path": nvmem_path, "powerup_counter_path": "/dummy/path"},
            )
            # When
            records = adm1266.read_records(adm1266_module.Adm1266BlackBoxRecord)
            # Then
            assert all(isinstance(r, adm1266_module.Adm1266BlackBoxRecord) for r in records)
            assert len(records) == 2
            assert records[0].uid == 1234
            assert records[0].powerup_counter == 1
            assert records[1].uid == 1235

    def test_clear_records(self, adm1266_module):
        # Given
        RECORD = create_raw_adm1266_blackbox_record(uid=1234, empty=False)
        with temp_file(
            content=RECORD,
            file_prefix="nvmem",
            dir_prefix="test_adm1266",
        ) as nvmem_path:
            adm1266 = adm1266_module.Adm1266(
                "test-dpm",
                platform_spec={"nvmem_path": nvmem_path, "powerup_counter_path": "/dummy/path"},
            )
            # When
            assert adm1266.read_raw_records() != []
            adm1266.clear_records()
            # Then
            assert adm1266.read_raw_records() == []
            with open(nvmem_path, "rb") as file:
                assert file.read() == b"1"

    def test_get_powerup_entries(self, adm1266_module):
        # Given
        RECORDS = [
            # 1st powerup
            create_raw_adm1266_blackbox_record(uid=1234, powerup_counter=1),  # No PDI
            create_raw_adm1266_blackbox_record(
                uid=1235,
                powerup_counter=1,
                pdio_in=0b0000_0000_0000_0001,  # PDI1
                timestamp=datetime.timedelta(seconds=70),
            ),
            # 2nd powerup
            create_raw_adm1266_blackbox_record(
                uid=1236,
                powerup_counter=2,
                pdio_in=0b0000_0000_0000_0010,  # PDI2
                timestamp=datetime.datetime(2025, 10, 2, 23, 22, 56, tzinfo=datetime.timezone.utc),
            ),
            # 3rd powerup
            create_raw_adm1266_blackbox_record(uid=1237, powerup_counter=3),  # No PDI
            create_raw_adm1266_blackbox_record(uid=1238, powerup_counter=3),  # No PDI
            create_raw_adm1266_blackbox_record(
                uid=1239,
                powerup_counter=3,
                pdio_in=0b0000_0000_0000_0100,  # PDI3 - not mapped to a cause
            ),
        ]
        with temp_file(
            content=b"".join(RECORDS), file_prefix="nvmem", dir_prefix="test_adm1266"
        ) as nvmem_path:
            platform_spec = {
                "nvmem_path": nvmem_path,
                "powerup_counter_path": "/dummy/path",
                "dpm_signal_to_fault_cause": [
                    {
                        "pdio_mask": "0b0000_0000_0000_0001",  # PDI1
                        "gpio_mask": "0b0000_0000_0000_0000",
                        "pdio_value": "0b0000_0000_0000_0001",  # PDI1
                        "gpio_value": "0b0000_0000_0000_0000",
                        "hw_cause": "HW_CAUSE_OF_PDI1",
                        "hw_desc": "HW_DESC_OF_PDI1",
                        "reboot_cause": "REBOOT_CAUSE_OF_PDI1",
                    },
                    {
                        "pdio_mask": "0b0000_0000_0000_0010",  # PDI2
                        "gpio_mask": "0b0000_0000_0000_0000",
                        "pdio_value": "0b0000_0000_0000_0010",  # PDI2
                        "gpio_value": "0b0000_0000_0000_0000",
                        "hw_cause": "HW_CAUSE_OF_PDI2",
                        "hw_desc": "HW_DESC_OF_PDI2",
                        "reboot_cause": "REBOOT_CAUSE_OF_PDI2",
                    },
                ],
            }
            adm1266 = adm1266_module.Adm1266("test-dpm", platform_spec)

            # When
            reboot_causes = adm1266.get_powerup_entries()

            # Then
            assert len(reboot_causes) == 3

            assert reboot_causes[0].power_fault_cause == adm1266_module.RebootCause(
                type=adm1266_module.RebootCause.Type.HARDWARE,
                source="test-dpm",
                timestamp=datetime.timedelta(seconds=70),
                cause="HW_CAUSE_OF_PDI1",
                description="HW_DESC_OF_PDI1",
                chassis_reboot_cause_category="REBOOT_CAUSE_OF_PDI1",
            )
            assert len(reboot_causes[0].dpm_records) == 2
            assert reboot_causes[0].dpm_records[0].uid == 1234
            assert reboot_causes[0].dpm_records[1].uid == 1235

            assert reboot_causes[1].power_fault_cause == adm1266_module.RebootCause(
                type=adm1266_module.RebootCause.Type.HARDWARE,
                source="test-dpm",
                timestamp=datetime.datetime(2025, 10, 2, 23, 22, 56, tzinfo=datetime.timezone.utc),
                cause="HW_CAUSE_OF_PDI2",
                description="HW_DESC_OF_PDI2",
                chassis_reboot_cause_category="REBOOT_CAUSE_OF_PDI2",
            )
            assert len(reboot_causes[1].dpm_records) == 1
            assert reboot_causes[1].dpm_records[0].uid == 1236

            assert reboot_causes[2].power_fault_cause is None
            assert len(reboot_causes[2].dpm_records) == 3
            assert reboot_causes[2].dpm_records[0].uid == 1237
            assert reboot_causes[2].dpm_records[1].uid == 1238
            assert reboot_causes[2].dpm_records[2].uid == 1239
