# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Digital Power Manager (DPM) base classes.

This module provides device-independent classes for
representing/retrieving/clearing records from a DPM device.
"""

import datetime

from abc import ABC, abstractmethod
from dataclasses import dataclass
from enum import Enum
from typing import Any, Type, TypeVar


@dataclass
class RebootCause:
    """Encapsulates the reboot cause information.

    This can be a HW power fault cause or a SW-triggered cause.
    """

    class Type(Enum):
        HARDWARE = "HW"
        SOFTWARE = "SW"

    type: Type
    # Name of the source that detected the reboot cause. For HW reboot causes,
    # this is generally the DPM name.
    source: str
    # Timestamp of the reboot cause. This could be the time elapsed since power-on,
    # or a UTC timestamp recorded by the `source`.
    timestamp: datetime.timedelta | datetime.datetime
    # Concise string representing the cause.
    cause: str
    # Longer string describing the cause.
    description: str
    # Should match one of the ChassisBase.REBOOT_CAUSE_* constants (e.g., "REBOOT_CAUSE_POWER_LOSS").
    chassis_reboot_cause_category: str


UNKNOWN_TIMESTAMP = datetime.datetime.min.replace(tzinfo=datetime.timezone.utc)


def timestamp_as_string(timestamp: datetime.timedelta | datetime.datetime) -> str:
    if timestamp == UNKNOWN_TIMESTAMP:
        return "unknown"

    if isinstance(timestamp, datetime.datetime):
        if timestamp.tzinfo is None:
            return timestamp.strftime("%Y-%m-%d %H:%M:%S")
        else:
            return timestamp.strftime("%Y-%m-%d %H:%M:%S %Z")
    return f"{timestamp.total_seconds():.6f}s after power-on"


class DpmRecord(ABC):
    """Base class for storing and processing a single record from a DPM.

    Subclasses must implement the abstract methods.
    """

    def __init__(self):
        self._power_fault_cause: RebootCause | None = None

    # Force subclass to always trigger DpmRecord.__init__().
    #
    # Subclasses can be a @dataclass, which generates its own __init__()
    # and does NOT call the superclass __init__().
    # So, we inject __post_init__() that always calls DpmRecord.__init__().
    def __init_subclass__(cls, **kawargs):
        super().__init_subclass__(**kawargs)

        original_post_init = getattr(cls, "__post_init__", None)

        def injected_post_init(self, *a, **k):
            # Call child's post-init first if it exists.
            if original_post_init:
                original_post_init(self, *a, **k)
            DpmRecord.__init__(self)

        cls.__post_init__ = injected_post_init

    def get_power_fault_cause(self) -> RebootCause | None:
        """Returns the power fault cause associated with this record if detected.

        Should be called only after set_metadata_and_process_power_fault_cause() is called.

        Returns:
            RebootCause object if a power fault is detected, None otherwise
        """
        return self._power_fault_cause

    @classmethod
    @abstractmethod
    def from_bytes(cls, data: bytes, dpm_name: str) -> "DpmRecord":
        """Creates a DpmRecord from raw bytes.

        Args:
            data: raw bytes representing a record from the DPM
            dpm_name: name of the DPM device that generated this record

        Returns:
            Parsed DpmRecord
        """
        pass

    @abstractmethod
    def is_valid(self) -> bool:
        """Returns True if the record is valid to be used, False otherwise."""
        pass

    @abstractmethod
    def set_metadata_and_process_power_fault_cause(self, platform_spec: dict[str, Any]) -> None:
        """Attaches metadata of the DPM provided in pd-plugin.json to help analyze this record.

        If this record indicates a power fault, the fault can be retrieved via get_power_fault_cause().
        """
        pass

    @abstractmethod
    def as_dict(self) -> dict[str, str]:
        """Returns a dictionary representation of this record.

        The returned dictionary is helpful for rendering the record in nh_reboot_cause.
        Derived classes can decide how to display each field.
        """
        pass


@dataclass
class DpmPowerUpEntry:
    """Encapsulates all information related to a single powerup.

    This specific powerup may or may not have experienced a reboot.
    """

    # Number of times the device has power cycled.
    # Note that this can wrap around, depending on the DPM's counter width.
    powerup_counter: int
    # If a power fault is None (i.e., not detected), it could be that
    # 1. power cycle has not happened on this powerup; or
    # 2. power cycle was caused by an unknown hardware event
    #    or by another hardware component that this DPM is not aware of.
    #    If it is the latter, other DPM(s) may have information about the fault.
    power_fault_cause: RebootCause | None
    # List of raw records from the DPM that relate to this powerup.
    dpm_records: list[DpmRecord]


_RecordT = TypeVar("_RecordT", bound="DpmRecord")


class DpmType(Enum):
    ADM1266 = "adm1266"


class DpmBase(ABC):
    """Base class for DPM device implementations.

    Provides common functionality for retrieving DPM's records and
    summary list of the previous powerups that this DPM is aware of.

    Subclasses must implement the abstract methods.
    """

    _type: DpmType
    _max_powerup_counter: int

    def __init__(self, name: str, platform_spec: dict[str, Any]):
        self._name = name
        self._platform_spec = platform_spec

    def __init_subclass__(cls, type: DpmType, max_powerup_counter: int, **kwargs):
        super().__init_subclass__(**kwargs)
        cls._type = type
        cls._max_powerup_counter = max_powerup_counter

    def get_name(self) -> str:
        """Returns the name of the DPM."""
        return self._name

    def get_type(self) -> DpmType:
        """Returns the type of the DPM."""
        return self._type

    @abstractmethod
    def read_raw_records(self) -> list[bytes]:
        """Reads the raw records from the DPM.

        Returns:
            List of raw bytes, each representing a single record
        """
        pass

    def read_records(self, real_record_type: Type[_RecordT]) -> list[_RecordT]:
        """Parses the raw records from the DPM into structured DpmRecord objects.

        Skips records that are not valid.

        Returns:
            List of DpmRecord objects
        """
        raw_records = self.read_raw_records()
        records = []
        for raw_record in raw_records:
            parsed_record = real_record_type.from_bytes(raw_record, self.get_name())
            if parsed_record.is_valid():
                parsed_record.set_metadata_and_process_power_fault_cause(self._platform_spec)
                records.append(parsed_record)
        return records

    @abstractmethod
    def get_powerup_counter(self) -> int:
        """Returns the number of times the DPM has experienced a power cycle

        (i.e., the counter of this current powerup)
        """
        pass

    def max_powerup_counter(self) -> int:
        """Returns the max number of powerup counter before it wraps around.
        
        For example, ADM1266 uses a 16-bit counter, so it returns 65535.
        """
        return self._max_powerup_counter

    @abstractmethod
    def get_powerup_entries(self) -> list[DpmPowerUpEntry]:
        """Returns a list of DpmPowerUpEntry, processed from raw records stored in this DPM.

        Each entry represents one powerup. The last powerup may or may not have experienced a reboot.

        Returns:
            List of DpmPowerUpEntry objects
        """
        pass

    @abstractmethod
    def clear_records(self) -> None:
        """Clears the DPM records that are stored in the DPM."""
        pass
