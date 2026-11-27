#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2021-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
from sonic_platform import utils

try:
    from sonic_platform_base.sonic_pcie.pcie_common import PcieUtil
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SYSFS_PCI_DEVICE_PATH = '/sys/bus/pci/devices/'

# PCI class 0x0108 is a Non-Volatile memory controller, and lspci starts the device name in pcie.yaml with it
NVME_CLASS_PREFIX = '0x0108'
NVME_NAME_PREFIX = 'Non-Volatile memory controller'

# Constants from module_base.py
PCIE_DETACH_INFO_TABLE = "PCIE_DETACH_INFO"
PCIE_OPERATION_DETACHING = "detaching"


class Pcie(PcieUtil):

    def pcie_check_dpu(self, bus, dev, fn):
        # Special handling for Bluefield Devices
        # Ideally even with BIOS updates, the PCI ID for bluefield devices should not change.
        try:
            # Connect to STATE_DB to check for detached devices
            if not self.state_db:
                import swsscommon
                self.state_db = swsscommon.swsscommon.DBConnector("STATE_DB", 0)
            key_dict = f"{PCIE_DETACH_INFO_TABLE}|0000:{bus}:{dev}.{fn}"
            detach_info_dict = dict(self.state_db.hgetall(key_dict))
            if detach_info_dict and detach_info_dict.get("dpu_state") == PCIE_OPERATION_DETACHING:
                # Do not add this device to confInfo list
                return None
            elif self.check_pcie_sysfs(bus=int(bus, base=16), device=int(dev, base=16), func=int(fn, base=16)):
                return "Passed"
            return "Failed"
        except Exception as e:
            self.logger.log_error(f"Error: {e}")
            return None

    # check the current PCIe device with config file and return the result
    # use bus from _device_id_to_bus_map instead of from yaml file
    def get_pcie_check(self):
        self.load_config_file()
        return_confInfo = []
        nvme_devices = list(self._nvme_devices)
        for item_conf in self.confInfo:
            id_conf = item_conf["id"]
            dev_conf = item_conf["dev"]
            fn_conf = item_conf["fn"]
            bus_conf = item_conf["bus"]
            pcie_device_id = f"0000:{bus_conf}:{dev_conf}.{fn_conf}"
            if pcie_device_id in self.dpu_pcie_devices:
                status = self.pcie_check_dpu(bus_conf, dev_conf, fn_conf)
                if status:
                    item_conf["result"] = status
                    return_confInfo.append(item_conf)
                continue

            if item_conf.get("name", "").startswith(NVME_NAME_PREFIX):
                # The NVMe SSD has second sources with other device ids, so match it by its PCI class,
                # and show the name lspci gives the SSD that is fitted instead of the one in the yaml file
                nvme_device = self._take_nvme_device(nvme_devices, dev_conf, fn_conf)
                bus_conf = nvme_device[0] if nvme_device else None
                if self._nvme_names.get(nvme_device):
                    item_conf["name"] = self._nvme_names[nvme_device]
            else:
                bus_conf = self._device_id_to_bus_map.get(f"{id_conf}:{dev_conf}")
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
            pattern_for_device_folder = re.search(r'....:(..):(..)\..', folder)
            if pattern_for_device_folder:
                bus = pattern_for_device_folder.group(1)
                dev = pattern_for_device_folder.group(2)
                with open(os.path.join('/sys/bus/pci/devices', folder, 'device'), 'r') as device_file:
                    # The 'device' file contain an hex repesantaion of the id key in the yaml file.
                    # Example of the file contact:
                    # 0x6fe2
                    # We will strip the new line character, and remove the 0x prefix that is not needed.
                    device_id = device_file.read().strip().replace('0x', '')
                    self._device_id_to_bus_map[f"{device_id}:{dev}"] = bus

    # Create the list of (bus, dev, fn) of the NVMe devices, found by the PCI class instead of the device id,
    # and the name lspci gives each of them
    def _create_nvme_device_list(self):
        nvme_folders = []
        self._nvme_names = {}
        for folder in os.listdir(SYSFS_PCI_DEVICE_PATH):
            pattern_for_device_folder = re.search(r'....:(..):(..)\.(.)', folder)
            if pattern_for_device_folder:
                with open(os.path.join(SYSFS_PCI_DEVICE_PATH, folder, 'class'), 'r') as class_file:
                    # Example of the file content: 0x010802
                    if class_file.read().strip().startswith(NVME_CLASS_PREFIX):
                        nvme_folders.append((pattern_for_device_folder.groups(), folder))
        nvme_folders.sort()
        self._nvme_devices = [nvme_device for nvme_device, _ in nvme_folders]
        for nvme_device, folder in nvme_folders:
            # The folder name holds the PCI domain too; without it lspci shows this bus:dev.fn in every domain.
            # Example of the output, where the name follows the bus:dev.fn as in the yaml file:
            # 01:00.0 Non-Volatile memory controller: Silicon Motion, Inc. SM2268XT (DRAM-less) NVMe SSD ...
            # Only the first line names this device. If lspci gives no name, the name in the yaml file is shown
            output = utils.run_command(['lspci', '-s', folder])
            if output and ' ' in output:
                self._nvme_names[nvme_device] = output.splitlines()[0].split(' ', 1)[1]

    # Take an NVMe device with the given dev and fn out of nvme_devices,
    # so that each NVMe entry in the yaml file needs its own NVMe device to pass
    @staticmethod
    def _take_nvme_device(nvme_devices, dev, fn):
        for nvme_device in nvme_devices:
            _, nvme_dev, nvme_fn = nvme_device
            if int(nvme_dev, base=16) == int(dev, base=16) and int(nvme_fn, base=16) == int(fn, base=16):
                nvme_devices.remove(nvme_device)
                return nvme_device
        return None

    def __init__(self, platform_path):
        PcieUtil.__init__(self, platform_path)
        self._create_device_id_to_bus_map()
        self._create_nvme_device_list()
        self.dpu_pcie_devices = self.get_dpu_pcie_devices()
        self.state_db = None
        self.logger = logger.Logger()