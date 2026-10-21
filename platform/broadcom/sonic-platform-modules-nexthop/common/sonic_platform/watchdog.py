#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import logging
from collections.abc import Callable
from sonic_platform_base.watchdog_base import WatchdogBase
from nexthop import fpga_lib
from sonic_py_common import syslogger

_SYSLOG_IDENTIFIER = "sonic_platform.watchdog"
_logger = syslogger.SysLogger(_SYSLOG_IDENTIFIER, log_level=logging.INFO)
# Counter is 24 bits and should be interpreted as milliseconds
_MAX_WATCHDOG_COUNTER_MILLISECONDS = 0xFFFFFF
# Power cycle counter timeout in 2-counter mode; counts down after the MSI
# counter expires.
_WATCHDOG_POWER_CYCLE_TIMEOUT_SECONDS = 60


def _read_watchdog_counter_register(fpga_pci_addr: str, reg_offset: int) -> int:
    """Returns the value of the watchdog counter register."""
    return fpga_lib.read_32(pci_address=fpga_pci_addr, offset=reg_offset)


def _read_watchdog_countdown_value_milliseconds(
    fpga_pci_addr: str, reg_offset: int
) -> int:
    """Returns the value in the watchdog countdown, in milliseconds."""
    reg_val = _read_watchdog_counter_register(fpga_pci_addr, reg_offset)
    return fpga_lib.get_field(reg_val=reg_val, bit_range=(0, 23))


def _update_watchdog_countdown_value(
    fpga_pci_addr: str, milliseconds: int, reg_offset: int
) -> None:
    """Updates the watchdog counter value."""
    reg_val = _read_watchdog_counter_register(fpga_pci_addr, reg_offset)
    new_reg_val = fpga_lib.overwrite_field(
        reg_val=reg_val, bit_range=(0, 23), field_val=milliseconds
    )
    fpga_lib.write_32(
        pci_address=fpga_pci_addr,
        offset=reg_offset,
        val=new_reg_val,
    )


def _read_watchdog_counter_enable(fpga_pci_addr: str, reg_offset: int) -> int:
    """Reads the bit of whether the counter is enabled."""
    reg_val = _read_watchdog_counter_register(fpga_pci_addr, reg_offset)
    return fpga_lib.get_field(reg_val=reg_val, bit_range=(31, 31))


def _toggle_watchdog_counter_enable(
    fpga_pci_addr: str, enable: bool, reg_offset: int
) -> None:
    """Enables or disables the watchdog counter."""
    reg_val = _read_watchdog_counter_register(fpga_pci_addr, reg_offset)
    new_reg_val = fpga_lib.overwrite_field(
        reg_val=reg_val, bit_range=(31, 31), field_val=int(enable)
    )
    fpga_lib.write_32(
        pci_address=fpga_pci_addr,
        offset=reg_offset,
        val=new_reg_val,
    )


def _arm_counter(fpga_pci_addr: str, reg_offset: int, seconds: int) -> None:
    """Sets the counter countdown value and enables it."""
    _update_watchdog_countdown_value(
        fpga_pci_addr,
        milliseconds=seconds * 1_000,
        reg_offset=reg_offset,
    )
    _toggle_watchdog_counter_enable(fpga_pci_addr, True, reg_offset)


def _toggle_watchdog_reboot(
    fpga_pci_addr: str,
    enable: bool,
    control_reg_offset: int,
    control_reg_bit: int,
) -> None:
    """Enables or disables the capability of reboot induced by watchdog."""
    reg_val = fpga_lib.read_32(
        pci_address=fpga_pci_addr,
        offset=control_reg_offset,
    )
    new_reg_val = fpga_lib.overwrite_field(
        reg_val=reg_val,
        bit_range=(control_reg_bit, control_reg_bit),
        field_val=int(enable),
    )
    fpga_lib.write_32(
        pci_address=fpga_pci_addr,
        offset=control_reg_offset,
        val=new_reg_val,
    )


def _validated_arm(seconds: int, do_real_arm: Callable[[int], int]) -> int:
    """Validate the requested timeout, then arm the watchdog via do_real_arm.

    Returns:
        An integer specifying the *actual* number of seconds the watchdog
        was armed with. On failure returns -1.
    """
    milliseconds = seconds * 1_000

    if milliseconds <= 0 or milliseconds > _MAX_WATCHDOG_COUNTER_MILLISECONDS:
        _logger.log_error(
            f"cannot arm watchdog with {milliseconds} ms. should be within "
            f"0 and {_MAX_WATCHDOG_COUNTER_MILLISECONDS} ms"
        )
        return -1

    return do_real_arm(seconds)


def _disarm_watchdog(
    fpga_pci_addr: str,
    control_reg_offset: int,
    control_reg_bit: int,
    counter_regs: list[int],
) -> bool:
    """Disables the given counters and the watchdog-induced reboot."""
    try:
        for counter_reg in counter_regs:
            _toggle_watchdog_counter_enable(fpga_pci_addr, False, counter_reg)
        _toggle_watchdog_reboot(
            fpga_pci_addr, False, control_reg_offset, control_reg_bit
        )
    except Exception as e:
        _logger.log_error(f"cannot disarm watchdog: {e}")
        return False
    else:
        return True


def _get_remaining_seconds(fpga_pci_addr: str, counter_reg: int) -> int:
    """Returns the number of seconds remaining on the given counter, or -1
    if it is not enabled."""
    if not _read_watchdog_counter_enable(fpga_pci_addr, counter_reg):
        return -1

    countdown_milliseconds = _read_watchdog_countdown_value_milliseconds(
        fpga_pci_addr, counter_reg
    )
    return countdown_milliseconds // 1_000


class WatchdogSimple(WatchdogBase):
    """
    Nexthop platform-specific Watchdog using a single hardware counter.

    The single counter (watchdog_counter_powercycle_reg) is armed with the
    requested timeout; expiry triggers a power cycle.
    """

    def __init__(
        self,
        fpga_pci_addr: str,
        event_driven_power_cycle_control_reg_offset: int,
        event_driven_power_cycle_control_bit: int,
        watchdog_counter_powercycle_reg: int,
    ):
        super().__init__()
        self.fpga_pci_addr: str = fpga_pci_addr
        self.event_driven_power_cycle_control_reg_offset: int = (
            event_driven_power_cycle_control_reg_offset
        )
        self.event_driven_power_cycle_control_bit: int = (
            event_driven_power_cycle_control_bit
        )
        self.watchdog_counter_powercycle_reg: int = watchdog_counter_powercycle_reg

    def _do_real_arm(self, seconds: int) -> int:
        """Arm the hardware watchdog with the requested timeout (power cycle).

        Returns:
            An integer specifying the *actual* number of seconds the watchdog
            was armed with. On failure returns -1.
        """
        _logger.log_info(
            f"Arming powercycle counter (reg={hex(self.watchdog_counter_powercycle_reg)}) "
            f"with {seconds}s"
        )
        try:
            _arm_counter(
                self.fpga_pci_addr, self.watchdog_counter_powercycle_reg, seconds
            )
            _toggle_watchdog_reboot(
                self.fpga_pci_addr,
                True,
                self.event_driven_power_cycle_control_reg_offset,
                self.event_driven_power_cycle_control_bit,
            )
        except Exception as e:
            _logger.log_error(f"cannot arm watchdog: {e}")
            return -1
        else:
            return seconds

    def arm(self, seconds: int) -> int:
        """
        Arm the hardware watchdog with a timeout of <seconds> seconds.
        If the watchdog is currently armed, calling this function will
        simply reset the timer to the provided value. If the underlying
        hardware does not support the value provided in <seconds>, this
        method should arm the watchdog with the *next greater* available
        value.

        Returns:
            An integer specifying the *actual* number of seconds the watchdog
            was armed with. On failure returns -1.
        """
        return _validated_arm(seconds, self._do_real_arm)

    def disarm(self) -> bool:
        """Disarm the hardware watchdog."""
        _logger.log_info(
            f"Disarming powercycle counter (reg={hex(self.watchdog_counter_powercycle_reg)})"
        )
        return _disarm_watchdog(
            self.fpga_pci_addr,
            self.event_driven_power_cycle_control_reg_offset,
            self.event_driven_power_cycle_control_bit,
            [self.watchdog_counter_powercycle_reg],
        )

    def is_armed(self) -> bool:
        """Retrieves the armed state of the hardware watchdog."""
        return bool(
            _read_watchdog_counter_enable(
                self.fpga_pci_addr, self.watchdog_counter_powercycle_reg
            )
        )

    def get_remaining_time(self) -> int:
        """
        Returns the number of seconds remaining on the watchdog timer, or -1
        if the watchdog is not armed.
        """
        return _get_remaining_seconds(
            self.fpga_pci_addr, self.watchdog_counter_powercycle_reg
        )


class Watchdog(WatchdogBase):
    """
    Nexthop platform-specific Watchdog using two hardware counters.

    The MSI counter (watchdog_counter_msi_reg) is armed with the requested
    timeout and fires an MSI interrupt on expiry. The power cycle counter
    (watchdog_counter_powercycle_reg) is armed with a fixed timeout and
    starts counting down once the MSI counter expires; its expiry triggers
    a power cycle.
    """

    def __init__(
        self,
        fpga_pci_addr: str,
        event_driven_power_cycle_control_reg_offset: int,
        event_driven_power_cycle_control_bit: int,
        watchdog_counter_powercycle_reg: int,
        watchdog_counter_msi_reg: int,
    ):
        super().__init__()
        self.fpga_pci_addr: str = fpga_pci_addr
        self.event_driven_power_cycle_control_reg_offset: int = (
            event_driven_power_cycle_control_reg_offset
        )
        self.event_driven_power_cycle_control_bit: int = (
            event_driven_power_cycle_control_bit
        )
        self.watchdog_counter_powercycle_reg: int = watchdog_counter_powercycle_reg
        self.watchdog_counter_msi_reg: int = watchdog_counter_msi_reg

    def _do_real_arm(self, seconds: int) -> int:
        """Arm both hardware watchdogs.

        - The power cycle counter is armed with a fixed timeout (counts down
          after the MSI counter expires, then triggers power cycle).
        - The MSI counter is armed with the requested timeout (fires MSI
          interrupt on expiry).

        Returns:
            An integer specifying the *actual* number of seconds the watchdog
            was armed with. On failure returns -1.
        """
        _logger.log_info(
            f"Arming MSI counter (reg={hex(self.watchdog_counter_msi_reg)}) with "
            f"{seconds}s and powercycle counter "
            f"(reg={hex(self.watchdog_counter_powercycle_reg)}) with "
            f"{_WATCHDOG_POWER_CYCLE_TIMEOUT_SECONDS}s"
        )
        try:
            _arm_counter(
                self.fpga_pci_addr,
                self.watchdog_counter_powercycle_reg,
                _WATCHDOG_POWER_CYCLE_TIMEOUT_SECONDS,
            )
            _arm_counter(self.fpga_pci_addr, self.watchdog_counter_msi_reg, seconds)
            _toggle_watchdog_reboot(
                self.fpga_pci_addr,
                True,
                self.event_driven_power_cycle_control_reg_offset,
                self.event_driven_power_cycle_control_bit,
            )
        except Exception as e:
            _logger.log_error(f"cannot arm watchdog: {e}")
            return -1
        else:
            return seconds

    def arm(self, seconds: int) -> int:
        """
        Arm the hardware watchdog with a timeout of <seconds> seconds.
        If the watchdog is currently armed, calling this function will
        simply reset the timer to the provided value. If the underlying
        hardware does not support the value provided in <seconds>, this
        method should arm the watchdog with the *next greater* available
        value.

        Returns:
            An integer specifying the *actual* number of seconds the watchdog
            was armed with. On failure returns -1.
        """
        return _validated_arm(seconds, self._do_real_arm)

    def disarm(self) -> bool:
        """Disarm both hardware watchdog counters."""
        _logger.log_info(
            f"Disarming MSI counter (reg={hex(self.watchdog_counter_msi_reg)}) and "
            f"powercycle counter (reg={hex(self.watchdog_counter_powercycle_reg)})"
        )
        return _disarm_watchdog(
            self.fpga_pci_addr,
            self.event_driven_power_cycle_control_reg_offset,
            self.event_driven_power_cycle_control_bit,
            [self.watchdog_counter_msi_reg, self.watchdog_counter_powercycle_reg],
        )

    def is_armed(self) -> bool:
        """Retrieves the armed state of the watchdog (checks the MSI counter)."""
        return bool(
            _read_watchdog_counter_enable(
                self.fpga_pci_addr, self.watchdog_counter_msi_reg
            )
        )

    def get_remaining_time(self) -> int:
        """
        Returns the number of seconds remaining on the MSI counter, or -1 if
        the watchdog is not armed.
        """
        return _get_remaining_seconds(
            self.fpga_pci_addr, self.watchdog_counter_msi_reg
        )
