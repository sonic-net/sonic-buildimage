# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import os
import syslog
import time
from typing import NoReturn

from nexthop.pddf_config_parser import (
    extract_fpga_attrs,
    FpgaDeviceName,
)
from nexthop.fpga_lib import write_32
from nexthop.pcie_lib import get_cpu_card_fpga_bdf, get_switchcard_fpga_bdf, get_switchcard_fpga_0_bdf

REBOOT_DELAY_MS = 1000

# Countdown written to the watchdog powercycle counter to trigger it.
WATCHDOG_TRIGGER_MS = 100

VMCORE_FILE = "/proc/vmcore"


def in_capture_kernel() -> bool:
    """The kdump capture kernel exposes the crashed kernel's memory at /proc/vmcore."""
    try:
        return os.path.getsize(VMCORE_FILE) > 0
    except OSError:
        return False


def log_to_kernel(message):
    try:
        with open("/dev/kmsg", "w") as k:
            # Use ERR level so it's visible in dmesg and console.
            print(f"<{syslog.LOG_ERR}>nh_powercycle: {message}\n", file=k, end="")
    except Exception:
        pass


def watchdog_powercycle(config) -> NoReturn:
    """Power cycle by shortening the FPGA watchdog powercycle counter, so the
    DPM records the FPGA watchdog signature as the reboot cause."""
    if not in_capture_kernel():
        # The counter only counts down after the MSI counter has expired.
        # Elsewhere the write would just corrupt the armed countdown,
        # shortening the next kdump window.
        raise Exception("Not in the kdump capture kernel")
    watchdog_config = config.get("WATCHDOG")
    if not watchdog_config:
        raise Exception("No WATCHDOG device found in PDDF configuration")
    reg_offset = int(watchdog_config["dev_attr"]["watchdog_counter_reg_offset"], 16)
    parent = watchdog_config["dev_info"]["device_parent"]
    parent_fpga_name = config[parent]["dev_info"]["device_name"]

    bdf_getters = {
        FpgaDeviceName.CPU_CARD.value: get_cpu_card_fpga_bdf,
        FpgaDeviceName.SWITCHCARD.value: get_switchcard_fpga_bdf,
        FpgaDeviceName.SWITCHCARD_0.value: get_switchcard_fpga_0_bdf,
    }
    if parent_fpga_name not in bdf_getters:
        raise Exception(f"Unknown watchdog parent FPGA '{parent_fpga_name}'")
    bdf = bdf_getters[parent_fpga_name]()
    if not bdf:
        raise Exception(f"BDF not found for watchdog parent FPGA '{parent_fpga_name}'")

    log_to_kernel(
        f"Arming watchdog powercycle counter ({parent_fpga_name} reg={hex(reg_offset)}) to initiate reboot"
    )
    # Bits 0-23: countdown in milliseconds; bit 31: counter enable.
    write_32(bdf, reg_offset, (1 << 31) | WATCHDOG_TRIGGER_MS)

    # Wait for the countdown plus the usual reboot delay before concluding
    # the watchdog did not cut power.
    time.sleep(REBOOT_DELAY_MS / 1000)

    # If we reach here, the watchdog did not cut power.
    raise Exception("Watchdog powercycle did not trigger")


def powercycle(config) -> NoReturn:
    fpga_types = (
        FpgaDeviceName.CPU_CARD.value,
        FpgaDeviceName.SWITCHCARD.value,
        FpgaDeviceName.SWITCHCARD_0.value,
    )
    try:
        fpga_attrs = extract_fpga_attrs(config, fpga_types)
    except Exception as e:
        raise Exception("Failed to extract FPGA attributes from PDDF configuration") from e

    if not fpga_attrs:
        raise Exception("No FPGA attributes found in PDDF configuration")

    log_to_kernel("Writing to CPU card FPGA power cycle control register to initiate reboot")
    bdf = None
    try:
        attrs = fpga_attrs[FpgaDeviceName.CPU_CARD.value]
        bdf = get_cpu_card_fpga_bdf()
        write_32(bdf, attrs.pwr_cycle_reg_offset, attrs.pwr_cycle_enable_word)
    except Exception as e:
        log_to_kernel(
            "Error attempting power cycle via control register on CPU FPGA"
            f" {bdf if bdf else ''}: {e}, trying switchcard FPGA"
        )
    time.sleep(REBOOT_DELAY_MS / 1000)

    sc_fpga_name = None
    sc_bdf = None
    if FpgaDeviceName.SWITCHCARD.value in fpga_attrs:
        sc_fpga_name = FpgaDeviceName.SWITCHCARD.value
        sc_bdf = get_switchcard_fpga_bdf()
    elif FpgaDeviceName.SWITCHCARD_0.value in fpga_attrs:
        sc_fpga_name = FpgaDeviceName.SWITCHCARD_0.value
        sc_bdf = get_switchcard_fpga_0_bdf()
    else:
        raise Exception(
            "Error attempting power cycle via control register on switchcard FPGA"
            ": Switchcard FPGA bdf not found."
        )

    if sc_fpga_name is not None:
        try:
            log_to_kernel(f"Writing to '{sc_fpga_name}' power cycle control register to initiate reboot")
            attrs = fpga_attrs[sc_fpga_name]
            write_32(sc_bdf, attrs.pwr_cycle_reg_offset, attrs.pwr_cycle_enable_word)
        except Exception as e:
            raise Exception(
                "Error attempting power cycle via control register on switchcard FPGA"
                f" {sc_bdf if sc_bdf else ''}: {e}"
            ) from e

    time.sleep(REBOOT_DELAY_MS / 1000)

    # If we reach here, we silently failed to reboot the dataplane!
    raise Exception("Failed to initiate reboot")
