#!/usr/bin/env python3
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

import functools
import os
import re
import shlex
import subprocess
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from typing import Callable

from sonic_platform.device_data import DeviceDataManager
from tabulate import tabulate

COMPONENT_VERSIONS_FILE = "/etc/mlnx/component-versions"
HEADERS = ["COMPONENT", "COMPILATION", "ACTUAL"]


def parse_fw_version(output: str, platform: str) -> list[str]:
    try:
        versions = []
        root = ET.fromstring(output)
        devices = filter(lambda x: platform in x.get('type'), root.findall('.//Device'))
        devices = sorted(devices, key=lambda x: x.get("pciName"))

        for device in devices:
            fw_elem = device.find('.//Versions/FW_Running')
            if fw_elem is None:
                fw_elem = device.find('.//Versions/FW')
            versions.append(fw_elem.get('current'))
        return versions
    except ET.ParseError:
        return ["N/A"]


def parse_fw_version_sw(output: str) -> list[str]:
    return parse_fw_version(output, "Spectrum")


def parse_fw_version_dpu(output: str) -> list[str]:
    return parse_fw_version(output, "BlueField")


@dataclass
class ComponentRule:
    deb_package: str | None = None
    cmd: str = ""
    regex: str | None = None
    is_from_syncd: bool = False
    fn: Callable[[str], list[str]] | None = None


COMPONENT_RULES_SW = {
    "MFT": ComponentRule(deb_package="mft"),
    "HW_MANAGEMENT": ComponentRule(deb_package="hw-management", regex=r".*1\.mlnx\.([0-9.]*)"),
    "SDK": ComponentRule(deb_package="sys-sdk-main", regex=r".*1\.mlnx\.([0-9.]*)", is_from_syncd=True),
    "SAI": ComponentRule(deb_package="mlnx-sai", regex=r".*1\.mlnx\.([A-Za-z0-9.]*)", is_from_syncd=True),
    "SAI_API_HEADERS": ComponentRule(cmd="dpkg -s mlnx-sai", regex="X-Sai-Headers-Version: ([0-9.]+)", is_from_syncd=True),
    "FW": ComponentRule(cmd="mlxfwmanager --query --query-format xml", regex=r"[0-9]{2}[\._]([0-9._-]*)", fn=parse_fw_version_sw),
    "KERNEL": ComponentRule(cmd="uname -r", regex=r"(.*)-[a-z0-9]+$"),
    "RSHIM": ComponentRule(deb_package="rshim"),
}


COMPONENT_RULES_DPU = {
    "MFT": ComponentRule(deb_package="mft"),
    "SDK": ComponentRule(deb_package="sdn-appliance", is_from_syncd=True),
    "SAI": ComponentRule(deb_package="mlnx-sai", regex=r".*1\.mlnx\.([A-Za-z0-9.]*)", is_from_syncd=True),
    "SAI_API_HEADERS": ComponentRule(cmd="dpkg -s mlnx-sai", regex="X-Sai-Headers-Version: ([0-9.]+)", is_from_syncd=True),
    "FW": ComponentRule(cmd="mlxfwmanager --query --query-format xml", regex=r"[0-9]{2}[\._]([0-9._-]*)", fn=parse_fw_version_dpu),
    "KERNEL": ComponentRule(cmd="uname -r", regex=r"(.*)-[a-z0-9]+$"),
    "BFSOC": ComponentRule(deb_package="mlxbf-bootimages"),
}


UNAVAILABLE_PLATFORM_VERSIONS = {
    "ONIE": "N/A",
    "SSD": "N/A",
    "BIOS": "N/A",
    "CPLD": "N/A"
}


@functools.cache
def get_asic_type():
    from sonic_py_common import device_info
    return device_info.get_sonic_version_info().get('asic_type')


@functools.cache
def get_component_rules():
    return COMPONENT_RULES_SW if get_asic_type() == "mellanox" else COMPONENT_RULES_DPU


def process_rule(rule: ComponentRule) -> tuple[bool, str | list[str]]:
    asic_count = DeviceDataManager.get_asic_count() if get_asic_type() == "mellanox" else 1

    cmd = rule.cmd
    if rule.deb_package:
        cmd = "dpkg-query -W -f='${Version}' " + rule.deb_package

    versions = []
    cmd_args = shlex.split(cmd)
    if asic_count > 1 and rule.is_from_syncd:
        for asic in range(asic_count):
            asic_cmd = ["docker", "exec", "-it", f"syncd{asic}"] + cmd_args
            version = subprocess.run(asic_cmd, shell=False, stdout=subprocess.PIPE, text=True).stdout
            versions.append(version)
    else:
        if rule.is_from_syncd:
            run_cmd = ["docker", "exec", "-it", "syncd"] + cmd_args
        else:
            run_cmd = cmd_args
        version = subprocess.run(run_cmd, shell=False, stdout=subprocess.PIPE, text=True).stdout
        versions = [version]

    if rule.fn:
        if len(versions) != 1:
            print(f"Error: expecting a single input for {rule.deb_package} parsing, got {len(versions)} outputs")
            return True, ["N/A"]
        versions = rule.fn(versions[0])

    if rule.regex:
        versions[:] = [re.search(rule.regex, version).group(1) for version in versions]

    if len(set(versions)) == 1:
        return True, versions[0]
    return False, versions


def parse_compiled_components_file():
    rules = get_component_rules()
    compiled_versions = {comp: "N/A" for comp in list(rules.keys()) + ["SIMX"]}

    if not os.path.exists(COMPONENT_VERSIONS_FILE):
        return compiled_versions

    with open(COMPONENT_VERSIONS_FILE, 'r') as component_versions:
        for line in component_versions.readlines():
            try:
                comp, version = line.split()
                compiled_versions[comp] = version
            except ValueError:
                continue

    return compiled_versions


def get_pdp():
    from fwutil.lib import PlatformDataProvider
    return PlatformDataProvider()


def get_platform_component_versions():
    pdp = get_pdp()
    chassis_name = pdp.chassis.get_name()
    versions_map = pdp.chassis_component_map.get(chassis_name, {})

    if not versions_map or len(versions_map) == 0:
        return UNAVAILABLE_PLATFORM_VERSIONS

    platform_versions = {}
    for component_name, component in versions_map.items():
        platform_versions[component_name] = component.get_firmware_version()

    return platform_versions


def get_current_version(comp: str) -> tuple[bool, str | list[str]]:
    try:
        return process_rule(get_component_rules()[comp])
    except Exception:
        return True, "N/A"


def format_output_table(table):
    return tabulate(table, HEADERS)


def main():

    if os.getuid() != 0:
        print("Error: Root privileges are required")
        return

    compiled_versions = parse_compiled_components_file()
    simx_compiled_ver = compiled_versions.pop("SIMX")

    # Add compiled versions to table
    output_table = []
    for comp in compiled_versions.keys():
        unique, versions = get_current_version(comp)
        if unique:
            output_table.append([comp, compiled_versions[comp], versions])
        else:
            for idx, v in enumerate(versions):
                output_table.append([f"{comp} ASIC{idx}", compiled_versions[comp], v])

    # Handle if SIMX
    if hasattr(DeviceDataManager, "is_simx_platform") and DeviceDataManager.is_simx_platform():
        simx_actual_ver = DeviceDataManager.get_simx_version()
        output_table.append(["SIMX", simx_compiled_ver, simx_actual_ver])
        platform_versions = UNAVAILABLE_PLATFORM_VERSIONS
    else:
        platform_versions = get_platform_component_versions()

    # Add actual versions to table
    for comp in platform_versions.keys():
        output_table.append([comp, "-", platform_versions[comp]])

    print(format_output_table(output_table))


if __name__ == "__main__":
    main()
