# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""
Library to perform common SPI device operation
"""

import os
import subprocess
import time

from dataclasses import dataclass
from nexthop.pddf_loader import load_pddf_device_config

SPI_DEV_DIR = "/sys/bus/spi/devices"


@dataclass
class SpiDeviceInfo:
    controller_name: str | None
    controller_idx: int | None
    component_name: str | None
    device_cs: int | None
    mtd_partition_label: str | None = None


def spi_device_to_component(device_name) -> str | None:
    """Find the component name associated with a spi device

    :param device_name: the spi device name
    :return: component name if found, otherwise None
    """
    if "ASIC_BOOT_FLASH" in device_name:
        component_name = device_name.replace("ASIC_BOOT_FLASH", "ASIC_PCIE")
    elif "CPUCARD" in device_name or "SWITCHCARD" in device_name or "MEZZCARD" in device_name:
        component_name = device_name.replace("CONFIG_FLASH", "FPGA")
    elif "MGMT_SWITCH" in device_name:
        component_name = "MGMT_SWITCH"
    # TODO: Add MGMT PHY support
    else:
        component_name = None

    # TODO: Add validation against platform_components.json OR platform.json
    return component_name


def component_to_spi_device(component_name) -> str | None:
    """Find the spi device name associated with a component

    :param device_name: the component name
    :return: component name if found, otherwise None
    """
    if "FPGA" in component_name:
        device_name = component_name.replace("FPGA", "CONFIG_FLASH")
    elif "ASIC_PCIE" in component_name:
        device_name = component_name.replace("PCIE", "BOOT_FLASH")
    elif "MGMT_SWITCH" in component_name:
        device_name = "MGMT_SWITCH_EEPROM"
    # TODO: Add MGMT PHY support
    else:
        device_name = None

    # TODO: Add validation against platform_component.json OR platform.json
    return device_name


def get_spi_device_info(device_name, pddf_config=None) -> SpiDeviceInfo | None:
    """Get SPI device info for a given device name"""
    if pddf_config is None:
        pddf_config = load_pddf_device_config()

    spi_device_obj = pddf_config.get(device_name)
    if not spi_device_obj:
        return None

    component_name = spi_device_to_component(device_name)

    dev_attr = spi_device_obj["dev_attr"]
    device_cs = dev_attr["chip_select"]
    controller_name = spi_device_obj["dev_info"]["device_parent"]
    controller_obj = pddf_config.get(controller_name)
    if not controller_obj:
        controller_idx = None
        controller_name = None
    else:
        controller_idx = controller_obj["dev_attr"]["spi_controller_idx"]
    mtd_label = dev_attr.get("mtd_partition_label")
    return SpiDeviceInfo(
        controller_name=controller_name,
        controller_idx=controller_idx,
        component_name=component_name,
        device_cs=device_cs,
        mtd_partition_label=mtd_label,
    )


def get_spidev_path(device_name: str, pddf_config=None) -> str:
    """Return the /dev/spidevN.M path for a PDDF SPI device.

    Reads controller_idx and chip_select from pddf-device.json without requiring
    the device to be present in the filesystem.

    Raises RuntimeError if the device or its controller is not found in the config.
    """
    info = get_spi_device_info(device_name, pddf_config)
    if info is None or info.controller_idx is None or info.device_cs is None:
        raise RuntimeError(f"{device_name}: not found in PDDF config or missing controller_idx/chip_select")
    return f"/dev/spidev{info.controller_idx}.{info.device_cs}"


def _apply_pddf_spi_enable_commands(spi_device_name: str, pddf_config=None) -> None:
    """Run PDDF spi_mode enable hooks on parent controller and device before create.

    FPGA virtual SPI controllers are gated behind register writes in spi_mode_commands
    (e.g. enabling the controller block in CPUCARD_FPGA). On NH-4210, switchcard config
    flashes share spi1.0 behind an FPGA mux; FLASH0 vs FLASH1 are selected via mux bits
    before spi-nor probes. cmdlinepart partitions at MTD registration time, so mux and
    controller enables must be applied before pddfparse registers spi-nor.
    """
    if pddf_config is None:
        pddf_config = load_pddf_device_config()
    device = pddf_config.get(spi_device_name)
    if not device:
        return

    def run_enables(obj: dict) -> None:
        for command in obj.get("spi_mode_commands", []):
            enable = command.get("enable")
            if enable:
                subprocess.run(enable, shell=True, check=False)

    parent_name = device.get("dev_info", {}).get("device_parent")
    if parent_name:
        parent = pddf_config.get(parent_name)
        if parent:
            run_enables(parent)
    run_enables(device)


def _run_pddfparse_subtree(op: str, spi_device_name: str) -> None:
    flag = f"--{op}-subtree"
    result = subprocess.run(
        ["pddfparse.py", flag, spi_device_name],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode:
        msg = (result.stderr or result.stdout or "").strip()
        raise RuntimeError(f"pddfparse.py {flag} {spi_device_name} failed: {msg}")


def _assert_update_partition_only(spi_device_name: str, pddf_config=None) -> None:
    """Fail closed if cmdlinepart did not partition the config-flash master.

    With cmdlinepart active only the named update partition gets an mtd node. If the
    parser never ran (module not loaded, or mtd->name did not match the cmdline mtd-id),
    spi-nor registers the whole chip as a single master named like the raw spi device
    (e.g. spi1.0), silently exposing the golden region. Detect both cases and refuse.
    """
    info = get_spi_device_info(spi_device_name, pddf_config)
    want_label = info.mtd_partition_label if info else None
    if not want_label:
        return
    expected_spi_name = f"spi{info.controller_idx}.{info.device_cs}"
    mtd_dev_dir = os.path.join(SPI_DEV_DIR, expected_spi_name, "mtd")
    if not os.path.isdir(mtd_dev_dir):
        raise RuntimeError(f"{spi_device_name}: no mtd dir {mtd_dev_dir} after create")
    names = {
        e: _read_mtd_name(mtd_dev_dir, e)
        for e in os.listdir(mtd_dev_dir)
        if not e.endswith("ro")
    }
    if expected_spi_name in names.values():
        raise RuntimeError(
            f"{spi_device_name}: unpartitioned master {expected_spi_name} exposed "
            f"(cmdlinepart did not run); refusing to expose golden region. "
            f"mtd entries={names}"
        )
    if want_label not in names.values():
        raise RuntimeError(
            f"{spi_device_name}: expected MTD partition {want_label!r} not present "
            f"after create; mtd entries={names}"
        )


def _wait_for_path(path: str, timeout_seconds: float) -> None:
    """Poll until path exists; raise RuntimeError if it does not appear within timeout_seconds."""
    deadline = time.monotonic() + timeout_seconds
    while not os.path.exists(path):
        if time.monotonic() >= deadline:
            raise RuntimeError(f"timed out after {timeout_seconds}s waiting for {path} to appear")
        time.sleep(0.05)


def create_spi_subtree(
    spi_device_name: str,
    pddf_config=None,
    wait_for_path: str | None = None,
    timeout_seconds: float = 5.0,
) -> None:
    """Create a PDDF SPI device subtree for firmware update.

    FPGA config flashes on multiboot platforms are partitioned at MTD registration
    time via kernel cmdline cmdlinepart.mtdparts= (see platform installer.conf).
    Run PDDF enable hooks (FPGA mux) before creating the subtree so spi-nor
    registers the expected master.

    If wait_for_path is given, poll until that filesystem path exists (e.g.
    /dev/spidevN.M or /dev/mtdX) before returning, raising RuntimeError on timeout_seconds.
    The subtree is deleted on any failure to keep state consistent.
    """
    _apply_pddf_spi_enable_commands(spi_device_name, pddf_config)
    _run_pddfparse_subtree("create", spi_device_name)
    if wait_for_path is not None:
        try:
            _wait_for_path(wait_for_path, timeout_seconds)
        except RuntimeError:
            try:
                _run_pddfparse_subtree("delete", spi_device_name)
            except RuntimeError:
                pass
            raise
    try:
        _assert_update_partition_only(spi_device_name, pddf_config)
    except RuntimeError:
        try:
            _run_pddfparse_subtree("delete", spi_device_name)
        except RuntimeError:
            pass
        raise


def delete_spi_subtree(spi_device_name: str, pddf_config=None) -> None:
    """Delete the PDDF SPI device subtree."""
    _run_pddfparse_subtree("delete", spi_device_name)


def _read_mtd_name(mtd_dev_dir: str, mtd_entry: str) -> str | None:
    """Return MTD label from sysfs for a given mtdN under the SPI device's mtd directory."""
    for base in (os.path.join(mtd_dev_dir, mtd_entry, "name"), f"/sys/class/mtd/{mtd_entry}/name"):
        if os.path.isfile(base):
            with open(base, encoding="utf-8") as f:
                return f.read().strip()
    return None


def get_mtd_device_path(spi_device_name: str) -> str:
    """
    Get the mtd device /dev/mtdX name for a given SPI device name

    Args:
        spi_device_name: SPI device name (e.g. ASIC_BOOT_FLASH)

    Returns:
        mtd device name (e.g. /dev/mtd0) if found

    Raises:
        Exception if not able to find the mtd device path
    """
    spi_device_info = get_spi_device_info(spi_device_name)
    if spi_device_info is None or spi_device_info.controller_idx is None or spi_device_info.device_cs is None:
        raise Exception(f"Failed to get SPI device info for {spi_device_name}")
    expected_spi_name = f"spi{spi_device_info.controller_idx}.{spi_device_info.device_cs}"
    spi_dev_path = os.path.join(SPI_DEV_DIR, expected_spi_name)
    if not os.path.isdir(spi_dev_path):
        raise Exception(f"SPI device {expected_spi_name} not found in {SPI_DEV_DIR}")
    mtd_dev_dir = os.path.join(spi_dev_path, "mtd")
    if not os.path.isdir(mtd_dev_dir):
        raise Exception(f"mtd directory {mtd_dev_dir} not created for {expected_spi_name}")
    entries = sorted(e for e in os.listdir(mtd_dev_dir) if not e.endswith("ro"))
    want_label = spi_device_info.mtd_partition_label
    if want_label:
        for e in entries:
            if _read_mtd_name(mtd_dev_dir, e) == want_label:
                mtd_dev = os.path.join("/dev", e)
                if not os.path.exists(mtd_dev):
                    raise Exception(f"mtd device {mtd_dev} not found in /dev")
                return mtd_dev
        raise Exception(
            f"No MTD partition named {want_label!r} under {mtd_dev_dir}; entries={entries}"
        )

    if len(entries) != 1:
        raise Exception(f"Expected exactly one mtd device under {mtd_dev_dir}, found: {entries}")
    mtd_dev = os.path.join("/dev", entries[0])
    if not os.path.exists(mtd_dev):
        raise Exception(f"mtd device {mtd_dev} not found in /dev")
    return mtd_dev
