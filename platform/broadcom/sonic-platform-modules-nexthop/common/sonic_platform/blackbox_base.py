# SPDX-License-Identifier: Apache-2.0

"""Blackbox capability mixin.

Provides:
- BlackBoxBase: abstract mixin for devices whose hardware exposes a blackbox
  of fault records. Only blackbox-capable device types derive from it; it is
  not part of the generic DeviceBase contract.
"""

from sonic_platform.blackbox_record_base import BlackBoxRecordBase


class BlackBoxBase:
    """
    Abstract base class for interfacing with a device's blackbox
    """

    def get_blackbox_raw(self) -> bytes:
        """
        Reads the raw fault records stored on the device

        Returns:
            Raw bytes from blackbox
        """
        raise NotImplementedError

    def clear_blackbox(self):
        """
        Clears the blackbox fault records that are stored on the device
        """
        raise NotImplementedError

    def decode_blackbox_records(self, raw) -> list[BlackBoxRecordBase]:
        """
        Decodes the raw blackbox into a structured record containing the status
        and fault fields

        Args:
            raw: list of raw blackbox bytes

        Returns:
            a list of BlackBoxRecordBase objects
        """
        raise NotImplementedError
