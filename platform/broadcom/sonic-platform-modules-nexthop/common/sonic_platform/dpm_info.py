# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

from typing import Dict, Any

class DpmInfo:
    def __init__(self, name, pddf_plugin_data):
        self.name = name
        dpm_info = pddf_plugin_data["DPM"][name]
        self.nvmem_path = dpm_info["nvmem_path"]

        self.vp_to_pdio_desc: Dict[int, Dict[str, Any]] = {
            int(k): v for k, v in dpm_info["vp_to_pdio_desc"].items()
        }
        self.vh_to_pdio_desc: Dict[int, Dict[int, Any]] = {
            int(k): v for k, v in dpm_info["vh_to_pdio_desc"].items()
        }
        self.dpm_signals: Dict[int, int] = {
            int(k): v for k, v in dpm_info["dpm_signals"].items()
        }
        self.dpm_table: Dict[int, str] = {
            int(k): v for k, v in dpm_info["dpm_table"].items()
        }
        self.power_fault_cause: Dict[int, Dict[str, str]] = {
            int(k): v for k, v in dpm_info["power_fault_cause"].items()
        }

    def get_vp_to_pdio_desc(self):
        return self.vp_to_pdio_desc

    def get_vh_to_pdio_desc(self):
        return self.vh_to_pdio_desc

    def get_dpm_signals(self):
        return self.dpm_signals

    def get_dpm_table(self):
        return self.dpm_table

    def get_power_fault_cause(self):
        return self.power_fault_cause

    def get_nvmem_path(self):
        return self.nvmem_path

    def get_name(self):
        return self.name
