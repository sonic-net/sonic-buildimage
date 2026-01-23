# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""ADM1266 structures for black box data processing.

Provides classes and helpers for representing an ADM1266 device
and parsing the ADM1266 blackbox data.
"""

import datetime

from dataclasses import dataclass
from enum import Enum
from itertools import groupby
from typing import Any, cast
from sonic_platform.dpm_base import (
    DpmRecord,
    DpmType,
    DpmBase,
    DpmPowerUpEntry,
    RebootCause,
    timestamp_as_string,
)
from sonic_py_common import syslogger


UNIX_EPOCH = datetime.datetime.fromtimestamp(0, tz=datetime.timezone.utc)

# This MUST match ADM1266_NH_CUSTOM_EPOCH_OFFSET in nh_adm1266.c kernel driver.
# ADM1266 only uses 4 bytes to store the seconds, which rollover every ~136 years.
# If using the normal UNIX epoch (1970), the timestamp will rollover in 2106.
# So, we use a custom epoch to maximize the usable time range.
CUSTOM_EPOCH = datetime.datetime(2024, 1, 1, tzinfo=datetime.timezone.utc)
EPOCH_OFFSET_SECONDS = int((CUSTOM_EPOCH - UNIX_EPOCH).total_seconds())  # 1704067200

SYSLOG_IDENTIFIER = "sonic_platform.adm1266"
logger = syslogger.SysLogger(SYSLOG_IDENTIFIER)


class PinName(Enum):
    """ADM1266 pin name."""

    # High-voltage Supply Fault Detection inputs.
    VH1 = "VH1"
    VH2 = "VH2"
    VH3 = "VH3"
    VH4 = "VH4"

    # Primary-voltage Supply Fault Detection inputs.
    VP1 = "VP1"
    VP2 = "VP2"
    VP3 = "VP3"
    VP4 = "VP4"
    VP5 = "VP5"
    VP6 = "VP6"
    VP7 = "VP7"
    VP8 = "VP8"
    VP9 = "VP9"
    VP10 = "VP10"
    VP11 = "VP11"
    VP12 = "VP12"
    VP13 = "VP13"

    # General Purpose Inputs.
    GPI1 = "GPI1"
    GPI2 = "GPI2"
    GPI3 = "GPI3"
    GPI4 = "GPI4"
    GPI5 = "GPI5"
    GPI6 = "GPI6"
    GPI7 = "GPI7"
    GPI8 = "GPI8"
    GPI9 = "GPI9"
    # General Purpose Outputs.
    GPO1 = "GPO1"
    GPO2 = "GPO2"
    GPO3 = "GPO3"
    GPO4 = "GPO4"
    GPO5 = "GPO5"
    GPO6 = "GPO6"
    GPO7 = "GPO7"
    GPO8 = "GPO8"
    GPO9 = "GPO9"

    # Programmable Driver Inputs.
    PDI1 = "PDI1"
    PDI2 = "PDI2"
    PDI3 = "PDI3"
    PDI4 = "PDI4"
    PDI5 = "PDI5"
    PDI6 = "PDI6"
    PDI7 = "PDI7"
    PDI8 = "PDI8"
    PDI9 = "PDI9"
    PDI10 = "PDI10"
    PDI11 = "PDI11"
    PDI12 = "PDI12"
    PDI13 = "PDI13"
    PDI14 = "PDI14"
    PDI15 = "PDI15"
    PDI16 = "PDI16"
    # Programmable Driver Outputs.
    PDO1 = "PDO1"
    PDO2 = "PDO2"
    PDO3 = "PDO3"
    PDO4 = "PDO4"
    PDO5 = "PDO5"
    PDO6 = "PDO6"
    PDO7 = "PDO7"
    PDO8 = "PDO8"
    PDO9 = "PDO9"
    PDO10 = "PDO10"
    PDO11 = "PDO11"
    PDO12 = "PDO12"
    PDO13 = "PDO13"
    PDO14 = "PDO14"
    PDO15 = "PDO15"
    PDO16 = "PDO16"


@dataclass
class Pin:
    """ADM1266 pin information."""

    name: PinName
    # Optional alias for the pin, e.g. VH3 as "3V3_WEST" which is the rail name.
    nickname: str | None = None

    def __str__(self) -> str:
        if self.nickname:
            # Example: "VH3(3V3_WEST)"
            return f"{self.name.value}({self.nickname})"
        else:
            return self.name.value


@dataclass
class PinStatuses:
    """Statuses of ADM1266 pins of the same type (e.g. all are VH pins, or VP pins, or etc.)."""

    raw_val: int
    pins: list[Pin]

    @classmethod
    def from_int(cls, raw_val: int, bitmask_to_pin: dict[int, PinName]):
        pins = []
        for mask, pin_type in bitmask_to_pin.items():
            if raw_val & mask:
                pins.append(Pin(pin_type))
        return cls(raw_val, pins)

    def set_pin_nickname(self, pin_to_nickname: dict[str, str]):
        for pin_info in self.pins:
            if pin_info.name.value in pin_to_nickname:
                pin_info.nickname = pin_to_nickname[pin_info.name.value]

    def to_str(self, bit_width) -> str:
        if self.pins:
            # Example: "0b0101 [VH3(3V3_WEST), VH1]"
            pins_str = ", ".join([str(pin_info) for pin_info in self.pins])
            return f"0b{self.raw_val:0{bit_width}b} [{pins_str}]"
        else:
            # Example: "0b0000"
            return f"0b{self.raw_val:0{bit_width}b}"


@dataclass
class Adm1266BlackBoxRecord(DpmRecord):
    """ADM1266 black box record. Refer to ADM1266 datasheet Table 79: Black Box Data Format.

    Represents a single fault record from the ADM1266 blackbox.
    """

    # byte [1:0]
    # Unique ID for this black box record.
    uid: int
    # byte 2
    # Empty boolean, Reserved, Page index that current record is saved in, JUMP_TYPE.
    byte_2: int
    # byte 3
    action_index: int
    # byte 4
    rule_index: int
    # byte 5
    # Over/under voltage status of the VHx pins.
    vh_over_voltage: PinStatuses
    vh_under_voltage: PinStatuses
    # byte [7:6]
    # The state in which the black box write was triggered.
    current_state: int
    # byte [9:8]
    # The state from which the black box write was entered.
    last_state: int
    # byte [11:10]
    # Overvoltage status of the VPx pins.
    vp_over_voltage: PinStatuses
    # byte [13:12]
    # Undervoltage status of the VPx pins.
    vp_under_voltage: PinStatuses
    # byte [15:14]
    # Input status of GPIOx pins.
    gpio_in: PinStatuses
    # byte [17:16]
    # Output status of GPIOx pins.
    gpio_out: PinStatuses
    # byte [19:18]
    # Input status of PDIOx pins.
    pdio_in: PinStatuses
    # byte [21:20]
    # Output status of PDIOx pins.
    pdio_out: PinStatuses
    # byte [23:22]
    # Number of times the device has power cycled.
    powerup_counter: int
    # byte [31:24]
    # The time when the black box record was triggered.
    # This could be the time elapsed since power-on, or a UTC timestamp sourced from the DPM clock.
    timestamp: datetime.timedelta | datetime.datetime
    # byte 63
    # CRC-8 of the record.
    crc: int

    # Raw data for this record (byte [63:0])
    raw: bytes

    # Name of the DPM device that this record belongs to.
    dpm_name: str

    RECORD_SIZE_IN_BYTES = 64

    # Bitmask -> PinName mappings. Used for decoding the byte(s) for pin statuses.
    BITMASK_TO_VH = {
        (1 << 3): PinName.VH4,  # bit 3
        (1 << 2): PinName.VH3,  # bit 2
        (1 << 1): PinName.VH2,  # bit 1
        (1 << 0): PinName.VH1,  # bit 0
    }
    BITMASK_TO_VP = {
        (1 << 12): PinName.VP13,  # bit 12
        (1 << 11): PinName.VP12,  # bit 11
        (1 << 10): PinName.VP11,  # bit 10
        (1 << 9): PinName.VP10,  # bit 9
        (1 << 8): PinName.VP9,  # bit 8
        (1 << 7): PinName.VP8,  # bit 7
        (1 << 6): PinName.VP7,  # bit 6
        (1 << 5): PinName.VP6,  # bit 5
        (1 << 4): PinName.VP5,  # bit 4
        (1 << 3): PinName.VP4,  # bit 3
        (1 << 2): PinName.VP3,  # bit 2
        (1 << 1): PinName.VP2,  # bit 1
        (1 << 0): PinName.VP1,  # bit 0
    }
    BITMASK_TO_GPI = {
        (1 << 11): PinName.GPI7,  # bit 11
        (1 << 10): PinName.GPI6,  # bit 10
        (1 << 9): PinName.GPI5,  # bit 9
        (1 << 8): PinName.GPI4,  # bit 8
        (1 << 7): PinName.GPI9,  # bit 7
        (1 << 6): PinName.GPI8,  # bit 6
        (1 << 2): PinName.GPI3,  # bit 2
        (1 << 1): PinName.GPI2,  # bit 1
        (1 << 0): PinName.GPI1,  # bit 0
    }
    BITMASK_TO_GPO = {
        (1 << 11): PinName.GPO7,  # bit 11
        (1 << 10): PinName.GPO6,  # bit 10
        (1 << 9): PinName.GPO5,  # bit 9
        (1 << 8): PinName.GPO4,  # bit 8
        (1 << 7): PinName.GPO9,  # bit 7
        (1 << 6): PinName.GPO8,  # bit 6
        (1 << 2): PinName.GPO3,  # bit 2
        (1 << 1): PinName.GPO2,  # bit 1
        (1 << 0): PinName.GPO1,  # bit 0
    }
    BITMASK_TO_PDI = {
        (1 << 15): PinName.PDI16,  # bit 15
        (1 << 14): PinName.PDI15,  # bit 14
        (1 << 13): PinName.PDI14,  # bit 13
        (1 << 12): PinName.PDI13,  # bit 12
        (1 << 11): PinName.PDI12,  # bit 11
        (1 << 10): PinName.PDI11,  # bit 10
        (1 << 9): PinName.PDI10,  # bit 9
        (1 << 8): PinName.PDI9,  # bit 8
        (1 << 7): PinName.PDI8,  # bit 7
        (1 << 6): PinName.PDI7,  # bit 6
        (1 << 5): PinName.PDI6,  # bit 5
        (1 << 4): PinName.PDI5,  # bit 4
        (1 << 3): PinName.PDI4,  # bit 3
        (1 << 2): PinName.PDI3,  # bit 2
        (1 << 1): PinName.PDI2,  # bit 1
        (1 << 0): PinName.PDI1,  # bit 0
    }
    BITMASK_TO_PDO = {
        (1 << 15): PinName.PDO16,  # bit 15
        (1 << 14): PinName.PDO15,  # bit 14
        (1 << 13): PinName.PDO14,  # bit 13
        (1 << 12): PinName.PDO13,  # bit 12
        (1 << 11): PinName.PDO12,  # bit 11
        (1 << 10): PinName.PDO11,  # bit 10
        (1 << 9): PinName.PDO10,  # bit 9
        (1 << 8): PinName.PDO9,  # bit 8
        (1 << 7): PinName.PDO8,  # bit 7
        (1 << 6): PinName.PDO7,  # bit 6
        (1 << 5): PinName.PDO6,  # bit 5
        (1 << 4): PinName.PDO5,  # bit 4
        (1 << 3): PinName.PDO4,  # bit 3
        (1 << 2): PinName.PDO3,  # bit 2
        (1 << 1): PinName.PDO2,  # bit 1
        (1 << 0): PinName.PDO1,  # bit 0
    }

    @classmethod
    def from_bytes(cls, data: bytes, dpm_name: str):
        """Creates Adm1266BlackBoxRecord from raw bytes.

        Args:
            data: 64-byte data representing a black box record
            dpm_name: Name of the DPM device for this black box
                      (for display purposes)

        Returns:
            Parsed BlackBoxRecord
        """
        if len(data) != cls.RECORD_SIZE_IN_BYTES:
            raise ValueError(
                f"Adm1266BlackBoxRecord: expected {cls.RECORD_SIZE_IN_BYTES} bytes, got {len(data)} bytes"
            )

        def to_int(field_bytes: bytes) -> int:
            # ADM1266 uses little endian.
            return int.from_bytes(field_bytes, byteorder="little")

        def to_timedelta_or_datetime(field_bytes: bytes) -> datetime.timedelta | datetime.datetime:
            BIT_TO_SECONDS = float(1 / float(2**16))  # 1 bit = 1 / 2^16 seconds

            secs = int.from_bytes(field_bytes[2:6], "little")
            fraction = int.from_bytes(field_bytes[0:2], "little") * BIT_TO_SECONDS

            elapsed_seconds = secs + fraction

            # Relative to power-on.
            # We assume that the custom epoch (`rtc_epoch_offset`) hadn't been set
            # if the record shows that the elapsed time is <7 days.
            SEVEN_DAY_SECONDS = 7 * 24 * 60 * 60
            if elapsed_seconds < SEVEN_DAY_SECONDS:
                return datetime.timedelta(seconds=elapsed_seconds)

            # Relative to the custom epoch.
            return CUSTOM_EPOCH + datetime.timedelta(seconds=elapsed_seconds)

        data_5_bit_3_0 = data[5] & 0b1111
        data_5_bit_7_4 = (data[5] >> 4) & 0b1111

        return cls(
            uid=to_int(data[0:2]),
            byte_2=data[2],
            action_index=data[3],
            rule_index=data[4],
            vh_over_voltage=PinStatuses.from_int(data_5_bit_3_0, cls.BITMASK_TO_VH),
            vh_under_voltage=PinStatuses.from_int(data_5_bit_7_4, cls.BITMASK_TO_VH),
            current_state=to_int(data[6:8]),
            last_state=to_int(data[8:10]),
            vp_over_voltage=PinStatuses.from_int(to_int(data[10:12]), cls.BITMASK_TO_VP),
            vp_under_voltage=PinStatuses.from_int(to_int(data[12:14]), cls.BITMASK_TO_VP),
            gpio_in=PinStatuses.from_int(to_int(data[14:16]), cls.BITMASK_TO_GPI),
            gpio_out=PinStatuses.from_int(to_int(data[16:18]), cls.BITMASK_TO_GPO),
            pdio_in=PinStatuses.from_int(to_int(data[18:20]), cls.BITMASK_TO_PDI),
            pdio_out=PinStatuses.from_int(to_int(data[20:22]), cls.BITMASK_TO_PDO),
            powerup_counter=to_int(data[22:24]),
            timestamp=to_timedelta_or_datetime(data[24:32]),
            crc=data[63],
            raw=data,
            dpm_name=dpm_name,
        )

    def is_valid(self) -> bool:
        is_empty = self.byte_2 & 0b0000_0001 == 1
        return not is_empty

    def _get_power_fault_cause_from_signal(
        self, dpm_signal_to_fault_cause: list[dict[str, Any]]
    ) -> RebootCause | None:
        """Returns the power fault cause from the given signal pattern to fault cause mappings."""
        for entry in dpm_signal_to_fault_cause:
            pdio_mask = int(entry["pdio_mask"], 0)
            pdio_value = int(entry["pdio_value"], 0)
            gpio_mask = int(entry["gpio_mask"], 0)
            gpio_value = int(entry["gpio_value"], 0)
            if (self.pdio_in.raw_val & pdio_mask) == pdio_value and \
               (self.gpio_in.raw_val & gpio_mask) == gpio_value:
                return RebootCause(
                    type=RebootCause.Type.HARDWARE,
                    source=self.dpm_name,
                    timestamp=self.timestamp,
                    cause=entry["hw_cause"],
                    description=entry["hw_desc"],
                    chassis_reboot_cause_category=entry["reboot_cause"],
                )
        return None

    def set_metadata_and_process_power_fault_cause(self, platform_spec: dict[str, Any]):
        """Sets metadata from the given platform spec.

        Such as
        - Rail name of each VH pin
        - Rail name of each VP pin
        - Nickname of each GPI/GPO/PDI/PDO pin
        - Power fault cause (if PDI & GPI signal matches a known pattern)
        """
        # Set nicknames for each pin.
        if "pin_to_name" in platform_spec:
            pin_to_name = platform_spec["pin_to_name"]
            self.vh_over_voltage.set_pin_nickname(pin_to_name)
            self.vh_under_voltage.set_pin_nickname(pin_to_name)
            self.vp_over_voltage.set_pin_nickname(pin_to_name)
            self.vp_under_voltage.set_pin_nickname(pin_to_name)
            self.gpio_in.set_pin_nickname(pin_to_name)
            self.gpio_out.set_pin_nickname(pin_to_name)
            self.pdio_in.set_pin_nickname(pin_to_name)
            self.pdio_out.set_pin_nickname(pin_to_name)

        # Analyze the signal pattern to determine the power fault cause.
        self._power_fault_cause = self._get_power_fault_cause_from_signal(
            platform_spec.get("dpm_signal_to_fault_cause", {})
        )

    def as_dict(self) -> dict[str, str]:
        def raw_pretty(data: bytes) -> str:
            """Returns multi-line hex dump (8 rows of 8 bytes each)."""
            rows = []
            for i in range(0, 64, 8):
                chunk = data[i : i + 8]
                rows.append(" ".join(f"{b:02x}" for b in chunk))
            return "\n".join(rows)

        def pins_with_under_voltage() -> str:
            pins = self.vh_under_voltage.pins + self.vp_under_voltage.pins
            return ",".join(map(str, pins)) or "n/a"

        def pins_with_over_voltage() -> str:
            pins = self.vh_over_voltage.pins + self.vp_over_voltage.pins
            return ",".join(map(str, pins)) or "n/a"

        def power_fault_cause_summary() -> str:
            power_fault_cause = (
                f"{self._power_fault_cause.cause} ({self._power_fault_cause.description})"
                if self._power_fault_cause
                else "n/a"
            )
            return "; ".join(
                [
                    power_fault_cause,
                    f"under_voltage: {pins_with_under_voltage()}",
                    f"over_voltage: {pins_with_over_voltage()}",
                ]
            )

        return {
            "timestamp": timestamp_as_string(self.timestamp),
            "dpm_name": self.dpm_name,
            "power_fault_cause": power_fault_cause_summary(),
            "uid": str(self.uid),
            "byte_2": f"0x{self.byte_2:02x}",
            "action_index": str(self.action_index),
            "rule_index": str(self.rule_index),
            "vh_over_voltage_[4:1]": self.vh_over_voltage.to_str(bit_width=4),
            "vh_under_voltage_[4:1]": self.vh_under_voltage.to_str(bit_width=4),
            "current_state": str(self.current_state),
            "last_state": str(self.last_state),
            "vp_over_voltage_[13:1]": self.vp_over_voltage.to_str(bit_width=13),
            "vp_under_voltage_[13:1]": self.vp_under_voltage.to_str(bit_width=13),
            "gpio_in_[7:4,9:8,R,R,R,3:1]": self.gpio_in.to_str(bit_width=12),
            "gpio_out_[7:4,9:8,R,R,R,3:1]": self.gpio_out.to_str(bit_width=12),
            "pdio_in_[16:1]": self.pdio_in.to_str(bit_width=16),
            "pdio_out_[16:1]": self.pdio_out.to_str(bit_width=16),
            "powerup_counter": str(self.powerup_counter),
            "crc": f"0x{self.crc:02x}",
            "raw": raw_pretty(self.raw),
        }

def record_unique_name(dict_representation: dict[str, str]) -> str:
    """Returns a unique name for this adm1266 record, given its dict.

    The given dict_representation should be an output from Admn1266BlackBoxRecord.as_dict().
    """
    return f"{dict_representation['dpm_name']}:powerup_{dict_representation['powerup_counter']}:uid_{dict_representation['uid']}"

def trim_record_dict(dict_representation: dict[str, str]) -> dict[str, str]:
    """Returns the given adm1266 record with only keys that are helpful for reboot-cause debugging.

    The given dict_representation should be an output from Admn1266BlackBoxRecord.as_dict().
    """
    KEYS_TO_KEEP = (
        "timestamp",
        "power_fault_cause",
        "gpio_in_[7:4,9:8,R,R,R,3:1]",
        "pdio_in_[16:1]",
    )
    return {k: dict_representation[k] for k in KEYS_TO_KEEP if k in dict_representation}


class Adm1266(DpmBase, type=DpmType.ADM1266, max_powerup_counter=65535):
    """ADM1266 device."""

    def __init__(self, name: str, platform_spec: dict[str, Any]):
        super().__init__(name, platform_spec)

        self._nvmem_path = platform_spec["nvmem_path"]
        self._rtc_epoch_offset_path = platform_spec.get("rtc_epoch_offset_path", None)
        self._powerup_counter_path = platform_spec["powerup_counter_path"]

    def set_rtc_epoch_offset(self, epoch_offset_sec: int = EPOCH_OFFSET_SECONDS):
        """Writes to sysfs to set the custom epoch on the ADM1266's RTC.

        This method is intended to be called periodically with the same epoch_offset_sec
        for periodic synchronization of ADM1266's RTC and the system time.

        To reduce the complexity, it's advised to use `EPOCH_OFFSET_SECONDS` which
        matches the default value in kernel's driver.

        Args:
            epoch_offset_sec: Epoch offset in seconds since UNIX epoch (1970-01-01).
                              Default to EPOCH_OFFSET_SECONDS (2024-01-01).
        """
        if not self._rtc_epoch_offset_path:
            raise ValueError(f"rtc_epoch_offset_path not specified in pd-plugin.json")

        with open(self._rtc_epoch_offset_path, "w") as file:
            file.write(str(epoch_offset_sec))

    def read_raw_records(self) -> list[bytes]:
        """Reads the blackbox data from the nvmem sysfs file.

        Skips records that are all 0xFF (erased) or all 0x00 (empty area).

        Returns:
            List of raw records, each record is 64 bytes.
        """

        def is_erased_or_empty(raw_record: bytes) -> bool:
            return all(b == 0xFF for b in raw_record) or all(b == 0x00 for b in raw_record)

        with open(self._nvmem_path, "rb") as file:
            data = file.read()

            SIZE = Adm1266BlackBoxRecord.RECORD_SIZE_IN_BYTES
            records = []
            for start in range(0, len(data), SIZE):
                raw_record = data[start : start + SIZE]
                if len(raw_record) != SIZE or is_erased_or_empty(raw_record):
                    continue
                records.append(raw_record)
            return records

    def get_powerup_counter(self) -> int:
        with open(self._powerup_counter_path, "r") as file:
            return int(file.read())

    def get_powerup_entries(self) -> list[DpmPowerUpEntry]:
        records = self.read_records(Adm1266BlackBoxRecord)
        records.sort(key=lambda record: record.uid)
        grouped_by_powerup = groupby(records, lambda record: record.powerup_counter)

        powerups = []
        for _, group in grouped_by_powerup:
            group_list = list(group)
            last_record = group_list[-1]  # record with the highest UID determines the cause

            powerups.append(
                DpmPowerUpEntry(
                    powerup_counter=last_record.powerup_counter,
                    power_fault_cause=last_record.get_power_fault_cause(),
                    dpm_records=cast(list[DpmRecord], group_list),
                )
            )
        return powerups

    def clear_records(self) -> None:
        """Clears the blackbox data by writing to the nvmem path."""
        with open(self._nvmem_path, "wb") as file:
            file.write(b"1")
