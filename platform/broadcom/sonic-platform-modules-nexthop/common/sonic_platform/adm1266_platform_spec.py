# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""ADM1266 platform-specific configuration and mappings.

Provides platform-specific data for ADM1266 devices including power rail
mappings, fault tables, signal definitions, and device paths.
"""

from typing import Any, Dict, List


class Adm1266PlatformSpec:
    """Platform-specific configuration for an ADM1266 device.

    Encapsulates platform-specific data including:
        - Power rail to PDIO mappings for voltage monitoring
        - DPM fault signals
        - Power fault cause descriptions and reboot cause mappings
        - NVMEM device path for blackbox data access

    Attributes:
        name: Device name identifier
        nvmem_path: Path to NVMEM device for blackbox data
        vpx_to_rail_desc: VP (voltage positive) to rail mappings
        vhx_to_rail_desc: VH (voltage high) to rail mappings
        dpm_signal_to_fault_cause: DPM signal pattern to fault cause mappings
    """

    def __init__(self, name: str, pddf_plugin_data: Dict[str, Any]):
        """Initialize platform specification from PDDF plugin data.

        Args:
            name: Device name identifier
            pddf_plugin_data: PDDF plugin data dictionary containing DPM configuration
        """
        self.name = name
        dpm_info = pddf_plugin_data["DPM"][name]
        self.nvmem_path = dpm_info["nvmem_path"]

        self.vpx_to_rail_desc: Dict[int, str] = {
            int(k): v for k, v in dpm_info["vpx_to_rail_desc"].items()
        }
        self.vhx_to_rail_desc: Dict[int, str] = {
            int(k): v for k, v in dpm_info["vhx_to_rail_desc"].items()
        }
        self.dpm_signal_to_fault_cause: List[Dict[str, Any]] = []
        for entry in dpm_info.get("dpm_signal_to_fault_cause", []):
            converted = {
                "pdio_mask": int(entry["pdio_mask"], 16),
                "gpio_mask": int(entry["gpio_mask"], 16),
                "pdio_value": int(entry["pdio_value"], 16),
                "gpio_value": int(entry["gpio_value"], 16),
                "hw_cause": entry["hw_cause"],
                "hw_desc": entry["hw_desc"],
                "summary": entry["summary"],
                "reboot_cause": entry["reboot_cause"]
            }
            self.dpm_signal_to_fault_cause.append(converted)

    def get_vpx_to_rail_desc(self) -> Dict[int, str]:
        """Get VP (voltage positive) to rail descriptions.

        Returns:
            Dictionary mapping VP indices to rail descriptions
        """
        return self.vpx_to_rail_desc

    def get_vhx_to_rail_desc(self) -> Dict[int, str]:
        """Get VH (voltage high) to rail descriptions.

        Returns:
            Dictionary mapping VH indices to rail descriptions
        """
        return self.vhx_to_rail_desc

    def get_dpm_signal_to_fault_cause(self) -> List[Dict[str, Any]]:
        """Get DPM signal pattern to fault cause mappings.

        Returns:
            List of signal pattern entries with integer mask/value fields and string fault cause info
        """
        return self.dpm_signal_to_fault_cause

    def get_nvmem_path(self) -> str:
        """Get NVMEM device path for blackbox data access.

        Returns:
            Path to NVMEM device file
        """
        return self.nvmem_path

    def get_name(self) -> str:
        """Get device name identifier.

        Returns:
            Device name string
        """
        return self.name
