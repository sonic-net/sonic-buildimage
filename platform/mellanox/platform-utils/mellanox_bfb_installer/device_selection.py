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
Validate user selections and get corresponding device info, etc. for the system.
"""

from dataclasses import dataclass
import logging
import os
import sys
from typing import Callable, List, Optional, Tuple

from mellanox_bfb_installer import platform_dpu
from mellanox_bfb_installer.platform_dpu import dpu2rshim, rshim2dpu
from sonic_platform.device_data import DpuInterfaceEnum

logger = logging.getLogger(__name__)


def _user_dpu_selection_to_dpus_from_platform_json(
    dpus: Optional[str],
    rshims: Optional[str],
    script_name: str,
    print_usage_callback: Callable[[], None],
) -> Tuple[List[str], bool]:

    all_dpus_list: List[str] = sorted(platform_dpu.list_dpus())
    if not all_dpus_list:
        logger.error(
            "No DPUs found! Make sure to run the %s script from the Smart Switch host device/switch!",
            script_name,
        )
        print_usage_callback()
        sys.exit(1)

    target_dpu_list: List[str] = []
    user_selected_all = False

    if dpus is not None and dpus.strip() == "":
        logger.error("If dpu parameter is provided, it cannot be empty!")
        print_usage_callback()
        sys.exit(1)
    if rshims is not None and rshims.strip() == "":
        logger.error("If rshim parameter is provided, it cannot be empty!")
        print_usage_callback()
        sys.exit(1)

    if dpus and rshims:
        logger.error("Both dpu and rshim selection provided! Please provide only one of them.")
        print_usage_callback()
        sys.exit(1)
    if dpus == "all" or rshims == "all":
        target_dpu_list = all_dpus_list
        user_selected_all = True
    elif dpus:
        target_dpu_list = [s.strip() for s in dpus.split(",")]
        for dpu in target_dpu_list:
            if not dpu:
                logger.error(
                    "If providing a list of DPUs, it cannot contain empty strings! (Check for extra commas.)"
                )
                print_usage_callback()
                sys.exit(1)
            if dpu not in all_dpus_list:
                logger.error(
                    'DPU "%s" is not found in platform.json dpus list: [%s]!',
                    dpu,
                    ", ".join(all_dpus_list),
                )
                print_usage_callback()
                sys.exit(1)
    elif rshims:
        _rshim_list = [s.strip() for s in rshims.split(",")]
        for rshim in _rshim_list:
            if not rshim:
                logger.error(
                    "If providing a list of rshims, it cannot contain empty strings! (Check for extra commas.)"
                )
                print_usage_callback()
                sys.exit(1)
            dpu = rshim2dpu(rshim)
            if not dpu:
                logger.error('No DPU in platform.json exists with rshim "%s"', rshim)
                sys.exit(1)
            target_dpu_list.append(dpu)
    else:
        logger.error("Input Error: No dpus specified! Please specify dpus using the --dpu option.")
        print_usage_callback()
        sys.exit(1)

    return target_dpu_list, user_selected_all


def _validate_config_files(config_paths: List[str]) -> None:
    for config_file in config_paths:
        if not os.path.isfile(config_file):
            logger.error(
                "Config provided %s is not a file! Please check the config file path",
                config_file,
            )
            sys.exit(1)


def _parse_config_paths(
    configs: Optional[str], num_dpus: int, user_selected_all_dpus: bool
) -> List[Optional[str]]:
    if configs is None:
        return [None] * num_dpus

    config_list = [s.strip() for s in configs.split(",") if s.strip()]
    if len(config_list) == 1:
        _validate_config_files(config_list)
        return [config_list[0]] * num_dpus
    elif user_selected_all_dpus:
        logger.error('Cannot specify "all" for dpus and more than one config file!')
        sys.exit(1)
    elif len(config_list) == num_dpus:
        _validate_config_files(config_list)
        return config_list
    logger.error(
        "Number of config files does not match the number of DPUs selected: %s and %s",
        len(config_list),
        num_dpus,
    )
    sys.exit(1)


@dataclass(frozen=True)
class TargetInfo:
    dpu: str
    rshim: str
    # Only used for detaching the dpu CX7 pci device. If this is None, it's not detected, so no need to detach.
    dpu_pci_bus_id: Optional[str]
    rshim_pci_bus_id: str
    config_path: Optional[str]


def get_targets(
    dpus: Optional[str],
    rshims: Optional[str],
    configs: Optional[str],
    script_name: str,
    print_usage_callback: Callable[[], None],
) -> List[TargetInfo]:
    """Return a list of targets to install on based on the user's selections.

    The user's selections are parsed and converted to a list of DPUs which is validated against
    the platform JSON file, as well as the currently installed devices on the PCI bus.

    Returns a list of TargetInfo objects, which is all the target-specific information needed to
    install the BFB image on the target DPU.
    """
    target_dpus_list, user_selected_all_dpus = _user_dpu_selection_to_dpus_from_platform_json(
        dpus=dpus, rshims=rshims, script_name=script_name, print_usage_callback=print_usage_callback
    )

    dpus_detected_pci_bus_ids = platform_dpu.get_dpus_detected_pci_bus_ids()

    config_paths = _parse_config_paths(configs, len(target_dpus_list), user_selected_all_dpus)

    target_devices = []
    for dpu, config_path in zip(target_dpus_list, config_paths, strict=True):
        rshim = dpu2rshim(dpu)
        if not rshim:
            logger.error("DPU %s: No rshim mapping found!", dpu)
            sys.exit(1)
        bus_ids = dpus_detected_pci_bus_ids.get(dpu, None)
        if not bus_ids:
            logger.error("DPU %s: no devices detected on the PCI bus!", dpu)
            sys.exit(1)
        # dpu_pci_bus_id = None is OK. Might be running in ISOLATED MODE. Only used to disconnect.
        dpu_pci_bus_id = bus_ids.get(DpuInterfaceEnum.PCIE_INT.value, None)
        rshim_pci_bus_id = bus_ids.get(DpuInterfaceEnum.RSHIM_PCIE_INT.value, None)
        if not rshim_pci_bus_id:
            logger.error("DPU %s: rshim %s is not detected on the PCI bus!", dpu, rshim)
            sys.exit(1)
        target_devices.append(
            TargetInfo(
                dpu=dpu,
                rshim=rshim,
                dpu_pci_bus_id=dpu_pci_bus_id,
                rshim_pci_bus_id=rshim_pci_bus_id,
                config_path=config_path,
            )
        )

    return target_devices
