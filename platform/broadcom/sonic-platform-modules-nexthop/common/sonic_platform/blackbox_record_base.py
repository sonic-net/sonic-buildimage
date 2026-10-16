# SPDX-License-Identifier: Apache-2.0

"""Blackbox record contract.

Provides:
- BlackBoxRecordBase: abstract base for a single record produced by a device's
  blackbox. Each device type defines its own subclass that knows how to
  parse raw bytes from the device and how to serialize/deserialize itself
  as one JSONL line.
"""

from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import ClassVar


@dataclass
class BlackBoxRecordBase(ABC):
    """Base class for blackbox fault records.

    Each device type parses its blackbox into its own subclass. Subclasses
    register the dev_types they handle via the register_record() class
    decorator, so consumers can resolve a record class by dev_type without
    importing the concrete subclass.
    """

    # Device name
    name: str
    # Raw blackbox data
    raw: bytes

    _record_registry: ClassVar["dict[str, type[BlackBoxRecordBase]]"] = {}

    @classmethod
    def register_record(cls, *dev_types: str):
        """Class decorator registering a record subclass for one or more dev_types."""
        def _decorator(record_cls):
            for dev_type in dev_types:
                cls._record_registry[dev_type] = record_cls
            return record_cls
        return _decorator

    @classmethod
    def get_record_class(cls, dev_type: str) -> "type[BlackBoxRecordBase] | None":
        """Returns the record class registered for dev_type, or None."""
        return cls._record_registry.get(dev_type)

    @classmethod
    @abstractmethod
    def from_bytes(cls, data: bytes, dev_name: str) -> "BlackBoxRecordBase":
        """Creates a BlackBoxRecordBase from raw bytes.

        Args:
            data: raw bytes representing a record from a device
            dev_name: name of the device that generated this record

        Returns:
            Parsed BlackBoxRecordBase
        """
        pass

    def get_raw(self) -> bytes:
        """Returns the raw bytes of this record."""
        return self.raw

    @abstractmethod
    def is_valid(self) -> bool:
        """Returns True if the record is valid to be used, False otherwise."""
        pass

    @abstractmethod
    def as_dict(self) -> dict:
        """Returns a JSON-serializable dict representation of this record.

        Dataclass subclasses can simply: `return dataclasses.asdict(self)`.
        Used both for JSONL persistence and for CLI rendering.
        """
        pass
