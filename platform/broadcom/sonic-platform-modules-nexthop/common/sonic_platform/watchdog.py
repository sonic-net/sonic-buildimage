#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import datetime
import time
from pathlib import Path
from sonic_platform_base.watchdog_base import WatchdogBase
from nexthop import fpga_lib
from sonic_py_common import syslogger

_SYSLOG_IDENTIFIER = "sonic_platform.watchdog"
_logger = syslogger.SysLogger(_SYSLOG_IDENTIFIER)
# Watchdog punching is paused if file is present
_WATCHDOG_PAUSE_FILE_PATH = Path("/var/lock/pddf-locks/watchdog.pause")
# How long the watchdog is armed for by the watchdog.timer
_WATCHDOG_PUNCH_DAEMON_ARM_SECONDS = 360


def _pause_watchdog_punching(duration: datetime.timedelta) -> None:
    """Creates the pause file."""
    try:
        pause_until_ts: int = int(time.time() + duration.total_seconds())
        with open(_WATCHDOG_PAUSE_FILE_PATH, "w") as f:
            f.write(str(pause_until_ts))
    except OSError as e:
        _logger.log_error(
            "Failed to write watchdog pause file. Continue without pausing "
            f"watchdog punching: {e}"
        )


def _unpause_watchdog_punching() -> None:
    # Remove the watchdog pause file to unpause
    _WATCHDOG_PAUSE_FILE_PATH.unlink(missing_ok=True)


class Watchdog(WatchdogBase):
    """
    Nexthop platform-specific placeholder Watchdog class
    """

    # Counter is 24 bits and should be interpreted as milliseconds
    _MAX_WATCHDOG_COUNTER_MILLISECONDS = 0xFFFFFF

    def __init__(
        self,
        fpga_pci_addr: str,
        event_driven_power_cycle_control_reg_offset: int,
        watchdog_counter_reg_offset: int,
    ):
        """
        Initialize the Watchdog class
        """
        super().__init__()
        self.fpga_pci_addr: str = fpga_pci_addr
        self.event_driven_power_cycle_control_reg_offset: int = (
            event_driven_power_cycle_control_reg_offset
        )
        self.watchdog_counter_reg_offset: int = watchdog_counter_reg_offset

    def _read_watchdog_counter_register(self) -> int:
        """Returns the value of the watchdog counter register."""
        return fpga_lib.read_32(
            pci_address=self.fpga_pci_addr, offset=self.watchdog_counter_reg_offset
        )

    def _read_watchdog_countdown_value_milliseconds(self) -> int:
        """Returns the value in the watchdog countdown, in milliseconds."""
        reg_val = self._read_watchdog_counter_register()
        return fpga_lib.get_field(reg_val=reg_val, bit_range=(0, 23))

    def _update_watchdog_countdown_value(self, milliseconds: int) -> None:
        """Updates the watchdog counter value."""
        reg_val = self._read_watchdog_counter_register()
        new_reg_val = fpga_lib.overwrite_field(
            reg_val=reg_val, bit_range=(0, 23), field_val=milliseconds
        )
        fpga_lib.write_32(
            pci_address=self.fpga_pci_addr,
            offset=self.watchdog_counter_reg_offset,
            val=new_reg_val,
        )

    def _read_watchdog_counter_enable(self) -> int:
        """Reads the bit of whether the counter is enabled."""
        reg_val = self._read_watchdog_counter_register()
        return fpga_lib.get_field(reg_val=reg_val, bit_range=(31, 31))

    def _toggle_watchdog_counter_enable(self, enable: bool) -> None:
        """Enables or disables the watchdog counter."""
        reg_val = self._read_watchdog_counter_register()
        new_reg_val = fpga_lib.overwrite_field(
            reg_val=reg_val, bit_range=(31, 31), field_val=int(enable)
        )
        fpga_lib.write_32(
            pci_address=self.fpga_pci_addr,
            offset=self.watchdog_counter_reg_offset,
            val=new_reg_val,
        )

    def _toggle_watchdog_reboot(self, enable: bool) -> None:
        """Enables or disables the capability of reboot induced by watchdog."""
        reg_val = fpga_lib.read_32(
            pci_address=self.fpga_pci_addr,
            offset=self.event_driven_power_cycle_control_reg_offset,
        )
        new_reg_val = fpga_lib.overwrite_field(
            reg_val=reg_val, bit_range=(4, 4), field_val=int(enable)
        )
        fpga_lib.write_32(
            pci_address=self.fpga_pci_addr,
            offset=self.event_driven_power_cycle_control_reg_offset,
            val=new_reg_val,
        )

    def _do_real_arm(self, seconds: int) -> int:
        """Arm the hardware watchdog.

        Returns:
            An integer specifying the *actual* number of seconds the watchdog
            was armed with. On failure returns -1.
        """
        try:
            self._toggle_watchdog_counter_enable(True)
            self._toggle_watchdog_reboot(True)
            self._update_watchdog_countdown_value(milliseconds=seconds*1_000)
        except Exception as e:
            _logger.log_error(f"cannot arm watchdog: {e}")
            return -1
        else:
            return seconds

    def arm_from_daemon(self) -> int:
        """Arm the watchdog with a predefined timeout.
        Meant to be called by watchdog punching.
        """
        return self._do_real_arm(_WATCHDOG_PUNCH_DAEMON_ARM_SECONDS)

    def arm(self, seconds: int) -> int:
        """
        Arm the hardware watchdog with a timeout of <seconds> seconds.
        If the watchdog is currently armed, calling this function will
        simply reset the timer to the provided value. If the underlying
        hardware does not support the value provided in <seconds>, this
        method should arm the watchdog with the *next greater* available
        value.

        Assumes an active punching timer that arms the watchdog for 6
        minutes (360 seconds), which is paused when `arm` is called and
        successfully arms the watchdog. The punching is paused until
        `disarm` is called.

        Returns:
            An integer specifying the *actual* number of seconds the watchdog
            was armed with. On failure returns -1.
        """
        milliseconds = seconds * 1_000

        if milliseconds <= 0 or milliseconds > self._MAX_WATCHDOG_COUNTER_MILLISECONDS:
            _logger.log_error(
                f"cannot arm watchdog with {milliseconds} ms. should be within "
                f"0 and {self._MAX_WATCHDOG_COUNTER_MILLISECONDS} ms"
            )
            return -1

        _pause_watchdog_punching(datetime.timedelta(seconds=seconds))
        ret = self._do_real_arm(seconds=seconds)
        if ret == -1:
            _unpause_watchdog_punching()
        return ret

    def disarm(self) -> bool:
        """
        Disarm the hardware watchdog

        Returns:
            A boolean, True if watchdog is disarmed successfully, False if not
        """
        try:
            self._toggle_watchdog_counter_enable(False)
            self._toggle_watchdog_reboot(False)
            # If any step above fails, do not attempt to resume watchdog punching
            _unpause_watchdog_punching()
        except Exception as e:
            _logger.log_error(f"cannot disarm watchdog: {e}")
            return False
        else:
            return True

    def is_armed(self) -> bool:
        """
        Retrieves the armed state of the hardware watchdog.

        Returns:
            A boolean, True if watchdog is armed, False if not
        """
        return bool(self._read_watchdog_counter_enable())

    def get_remaining_time(self) -> int:
        """
        If the watchdog is armed, retrieve the number of seconds remaining on
        the watchdog timer

        Returns:
            An integer specifying the number of seconds remaining on the
            watchdog timer. If the watchdog is not armed, returns -1.
        """
        if not self.is_armed():
            return -1

        countdown_milliseconds = self._read_watchdog_countdown_value_milliseconds()
        return int(countdown_milliseconds / 1_000)
