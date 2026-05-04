# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""
DPU/rshim mapping and PCI detection.
"""

import logging
import os
import subprocess
import sys
from typing import Dict, List, Optional, Tuple

logger = logging.getLogger(__name__)

from sonic_py_common import device_info
from sonic_platform.device_data import DeviceDataManager, DpuInterfaceEnum

# PCI Device IDs
BFSOC_DEV_ID = "15b3:c2d5"  # rshim device
CX7_DEV_ID = "15b3:a2dc"  # dpu (ethernet) device


def validate_platform() -> None:
    """Validate platform.json is available; exit with code 1 if not."""
    if not device_info.get_platform():
        logger.error("Could not determine PLATFORM from device_info")
        sys.exit(1)

    if not device_info.get_path_to_platform_dir():
        logger.error("Could not determine PLATFORM directory from device_info")
        sys.exit(1)

    path = os.path.join(
        device_info.get_path_to_platform_dir(),
        device_info.PLATFORM_JSON_FILE,
    )
    if not os.path.isfile(path):
        logger.error("platform.json file not found at %s", path)
        sys.exit(1)


def _get_dpus_data():
    """Return DPUS dict from platform.json via DeviceDataManager, or None."""
    return DeviceDataManager.get_platform_dpus_data()


def list_dpus() -> List[str]:
    """Return list of DPU names from platform.json (keys of .DPUS)."""
    dpus_data = _get_dpus_data()
    if not dpus_data:
        return []
    return list(dpus_data.keys())


def dpu2rshim(dpu: str) -> Optional[str]:
    """Return rshim name for the given DPU from platform.json (.DPUS[dpu].rshim_info)."""
    rshim = DeviceDataManager.get_dpu_interface(dpu, DpuInterfaceEnum.RSHIM_INT.value)
    return rshim if rshim else None


def rshim2dpu(rshim: str) -> Optional[str]:
    """Return DPU name for the given rshim from platform.json (inverse of dpu2rshim)."""
    dpus_data = _get_dpus_data()
    if not dpus_data:
        return None
    for dpu, info in dpus_data.items():
        if isinstance(info, dict) and info.get(DpuInterfaceEnum.RSHIM_INT.value) == rshim:
            return dpu
    return None


def _run_lspci_d_n() -> str:
    """Run `lspci -D -n` and return stdout."""
    try:
        result = subprocess.run(["lspci", "-D", "-n"], capture_output=True, text=True, timeout=10)
        return result.stdout if result.returncode == 0 else ""
    except Exception:
        return ""


def get_dpus_detected_pci_bus_ids() -> Dict[str, Dict[str, str]]:
    """Return a dict of DPU names to (interface type -> bus id) of detected PCI devices.

    Detects the two PCI devices for each DPU: the CX7 (ethernet NIC) dpu device and the BFSOC
    (rshim) device. Returns only the ones that are detected. Is aware of ISOLATED MODE dpus.

    The return value is a dict mapping from dpu name (e.g. "dpu0") to a dict of interface type
    strings as defined by DpuInterfaceEnum values, to bus id (e.g. "0000:01:00.0").
    """
    # Note: this function detects the two PCI devices for each DPU: the CX7 (ethernet) device and
    # the BFSOC (rshim) device. Normally these will reside at the PCI bus IDs designated in the
    # platform.json file for each DPU. However, there is another case: on some legacy systems, the
    # DPUs are configured to run in ISOLATED MODE, in which case the BFSOC (rshim) device is present
    # and the CX7 (ethernet) device is not. Also, the BFSOC (rshim) device is installed at the PCI
    # bus ID that the CX7 (ethernet) device is usually assigned to. During an upgrade of all of the
    # devices in a SmartSwitch, if this version of the script is running on the switch, the switch
    # will have the newer platform.json file for non-ISOLATED-MODE, so the BFSOC (rshim) device
    # will be on the PCI bus ID that the CX7 (ethernet) device is assigned to in the platform.json
    # file. So the implementation below detects either device type at either bus ID associated with
    # the dpu.

    dpus = list_dpus()

    # Identify which bus IDs the devices are expected to be at according to platform.json.
    # Store in `needed_bus_ids`, a dict from bus ID to (dpu, interface type). The interface type
    # string is a DpuInterfaceEnum value.
    needed_bus_ids: Dict[str, Tuple[str, str]] = {}
    for dpu in dpus:
        # "bus_info"/"rshim_bus_info" in platform.json are the PCI bus IDs.
        bus_info = DeviceDataManager.get_dpu_interface(dpu, DpuInterfaceEnum.PCIE_INT.value)
        if bus_info:
            needed_bus_ids[bus_info] = (dpu, DpuInterfaceEnum.PCIE_INT.value)
        rshim_bus_info = DeviceDataManager.get_dpu_interface(
            dpu, DpuInterfaceEnum.RSHIM_PCIE_INT.value
        )
        if rshim_bus_info:
            needed_bus_ids[rshim_bus_info] = (dpu, DpuInterfaceEnum.RSHIM_PCIE_INT.value)

    # Scan the pci system to find the devices present on the needed bus IDs.
    # Build the return value `detected_bus_ids` which maps from dpu to (interface type -> bus ID)
    # for each detected device. The interface type string is a DpuInterfaceEnum value.
    lspci_out = _run_lspci_d_n()
    detected_bus_ids: Dict[str, Dict[str, str]] = {}
    for line in lspci_out.splitlines():
        line_parts = line.split()
        if len(line_parts) < 3:
            logger.warning(
                'Invalid `lspci -D -n` output line. Expected at least 3 columns. Skipping line: "%s"',
                line,
            )
            continue
        # lspci -D -n output line example: "0000:08:00.0 0200: 15b3:a2d5 (rev 01)"
        bus_id = line_parts[0]
        device_id = line_parts[2]
        if device_id == CX7_DEV_ID:
            actual_iface_type = DpuInterfaceEnum.PCIE_INT.value
        elif device_id == BFSOC_DEV_ID:
            actual_iface_type = DpuInterfaceEnum.RSHIM_PCIE_INT.value
        else:
            if bus_id in needed_bus_ids:
                logger.warning(
                    "Ignoring unexpected PCI device id %s at bus id %s while detecting DPU interfaces",
                    device_id,
                    bus_id,
                )
            continue
        if bus_id in needed_bus_ids:
            dpu, expected_iface_type = needed_bus_ids[bus_id]
            if dpu not in detected_bus_ids:
                detected_bus_ids[dpu] = {}
            if actual_iface_type == DpuInterfaceEnum.RSHIM_PCIE_INT.value:
                if expected_iface_type == DpuInterfaceEnum.PCIE_INT.value:
                    logger.warning(
                        "DPU %s: is running in ISOLATED MODE. "
                        "PCI device detected at bus id %s is device id %s, not a CX7 %s",
                        *(dpu, bus_id, device_id, CX7_DEV_ID),  # *(...) is autoformatter hack
                    )
                detected_bus_ids[dpu][actual_iface_type] = bus_id
            elif actual_iface_type == DpuInterfaceEnum.PCIE_INT.value:
                if expected_iface_type == DpuInterfaceEnum.RSHIM_PCIE_INT.value:
                    logger.error(
                        "PCI device detected for DPU %s at bus id %s is the CX7 device id %s, not "
                        "the expected BFSOC / rshim device id %s",
                        *(dpu, bus_id, device_id, BFSOC_DEV_ID),  # *(...) is autoformatter hack
                    )
                detected_bus_ids[dpu][actual_iface_type] = bus_id
            else:
                raise ValueError(f"Unknown interface type: {actual_iface_type}")  # A script bug.
    return detected_bus_ids


def remove_cx7_pci_device(pci_bus_id: str, log_prefix: str) -> None:
    """If the CX PCI device for the DPU is present, remove it.

    Logging messages will be prefixed with the log_prefix value (e.g. "rshim0: ").
    """
    logger.info("%sRemoving CX PCI device %s", log_prefix, pci_bus_id)
    remove_path = f"/sys/bus/pci/devices/{pci_bus_id}/remove"
    try:
        with open(remove_path, "w") as f:
            f.write("1")
    except OSError as e:
        logger.error("Failed to remove PCI device %s: %s", pci_bus_id, e)
