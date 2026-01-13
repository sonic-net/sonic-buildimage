#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2021-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

########################################################################
#
# Module contains a platform specific implementation of SONiC Platform
# Base PCIe class
#
########################################################################
import os
import re
from sonic_py_common import logger
from sonic_platform.device_data import DeviceDataManager, DpuInterfaceEnum

try:
    from sonic_platform_base.sonic_pcie.pcie_common import PcieUtil
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SYSFS_PCI_DEVICE_PATH = '/sys/bus/pci/devices/'

# Constants from module_base.py
PCIE_DETACH_INFO_TABLE = "PCIE_DETACH_INFO"
PCIE_OPERATION_DETACHING = "detaching"


class Pcie(PcieUtil):
    # check the current PCIe device with config file and return the result
    # use bus from _device_id_to_bus_map instead of from yaml file
    def get_pcie_check(self):
        self.load_config_file()
        return_confInfo = []
        for item_conf in self.confInfo:
            id_conf = item_conf["id"]
            dev_conf = item_conf["dev"]
            fn_conf = item_conf["fn"]
            bus_conf = item_conf["bus"]
            pcie_device_id = f"0000:{bus_conf}:{dev_conf}.{fn_conf}"
            if pcie_device_id in self.dpu_pcie_devices:
                # Special handling for Bluefield Devices
                # Ideally even with BIOS updates, the PCI ID for bluefield devices should not change.
                try:
                    # Connect to STATE_DB to check for detached devices
                    if not os.environ.get('UNITTEST'):
                        import swsscommon
                        self.state_db = swsscommon.swsscommon.DBConnector("STATE_DB", 0)
                    key_dict = f"{PCIE_DETACH_INFO_TABLE}|0000:{bus_conf}:{dev_conf}.{fn_conf}"
                    detach_info_dict = dict(self.state_db.hgetall(key_dict))
                    if detach_info_dict and detach_info_dict.get("dpu_state") == PCIE_OPERATION_DETACHING:
                        # Do not add this device to confInfo list
                        continue
                    elif self.check_pcie_sysfs(bus=int(bus_conf, base=16), device=int(dev_conf, base=16), func=int(fn_conf, base=16)):
                        # Add device to confInfo list if not present in state_db
                        item_conf["result"] = "Passed"
                    else:
                        item_conf["result"] = "Failed"
                    return_confInfo.append(item_conf)
                    continue
                except Exception as e:
                    self.logger.log_error(f"Error: {e}")
                    pass
            bus_conf = self._device_id_to_bus_map.get(str(id_conf))
            if bus_conf and self.check_pcie_sysfs(bus=int(bus_conf, base=16), device=int(dev_conf, base=16),
                                                  func=int(fn_conf, base=16)):
                item_conf["result"] = "Passed"
            else:
                item_conf["result"] = "Failed"
            return_confInfo.append(item_conf)
        return return_confInfo

    def get_dpu_pcie_devices(self):
        dpu_count = DeviceDataManager.get_dpu_count()
        if dpu_count == 0:
            return []
        dpu_pcie_devices = []
        for dpu_id in range(dpu_count):
            dpu_name = f"dpu{dpu_id}"
            pci_dev_id = DeviceDataManager.get_dpu_interface(dpu_name, DpuInterfaceEnum.PCIE_INT.value)
            rshim_pci_dev_id = DeviceDataManager.get_dpu_interface(dpu_name, DpuInterfaceEnum.RSHIM_PCIE_INT.value)
            if not pci_dev_id or not rshim_pci_dev_id:
                continue
            dpu_pcie_devices.append(pci_dev_id)
            dpu_pcie_devices.append(rshim_pci_dev_id)
        return dpu_pcie_devices


    # Create
    def _create_device_id_to_bus_map(self):
        self._device_id_to_bus_map = {}
        self.load_config_file()
        device_folders = os.listdir(SYSFS_PCI_DEVICE_PATH)
        for folder in device_folders:
            # For each folder in the sysfs tree we check if it matches the normal PCIe device folder pattern,
            # If match we add the device id from the device file and the bus from the folder name to the map
            #
            # Example for device folder name: 0000:ff:0b.1
            #
            # The folder name is built from:
            #   4 hex digit of domain
            #   colon ':'
            #   2 hex digit of bus - this is what we are looking for
            #   colon ':'
            #   2 hex digit of id
            #   dot '.'
            #   1 digit of fn
            pattern_for_device_folder = re.search(r'....:(..):..\..', folder)
            if pattern_for_device_folder:
                bus = pattern_for_device_folder.group(1)
                with open(os.path.join('/sys/bus/pci/devices', folder, 'device'), 'r') as device_file:
                    # The 'device' file contain an hex repesantaion of the id key in the yaml file.
                    # Example of the file contact:
                    # 0x6fe2
                    # We will strip the new line character, and remove the 0x prefix that is not needed.
                    device_id = device_file.read().strip().replace('0x', '')
                    self._device_id_to_bus_map[device_id] = bus

    def __init__(self, platform_path):
        PcieUtil.__init__(self, platform_path)
        self._create_device_id_to_bus_map()
        self.dpu_pcie_devices = self.get_dpu_pcie_devices()
        self.state_db = None
        self.logger = logger.Logger()