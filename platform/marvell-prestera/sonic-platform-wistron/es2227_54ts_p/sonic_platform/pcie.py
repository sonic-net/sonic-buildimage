########################################################################
# Copyright (C) 2021, mrvl CORPORATION & AFFILIATES. All rights reserved.
# Module contains a platform specific implementation of SONiC Platform
# Base PCIe class
########################################################################
import os
import re

try:
    from sonic_platform_base.sonic_pcie.pcie_common import PcieUtil
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SYSFS_PCI_DEVICE_PATH = '/sys/bus/pci/devices/'

class Pcie(PcieUtil):
    def get_pcie_check(self):
        self.load_config_file()
        for item_conf in self.confInfo:
            id_conf = item_conf["id"]
            dev_conf = item_conf["dev"]
            fn_conf = item_conf["fn"]
            bus_conf = self._device_id_to_bus_map.get(str(id_conf))
            if bus_conf and self.check_pcie_sysfs(bus=int(bus_conf, base=16), device=int(dev_conf, base=16),
                                                  func=int(fn_conf, base=16)):
                item_conf["result"] = "Passed"
            else:
                item_conf["result"] = "Failed"
        return self.confInfo

    def _create_device_id_to_bus_map(self):
        self._device_id_to_bus_map = {}
        self.load_config_file()
        device_folders = os.listdir(SYSFS_PCI_DEVICE_PATH)
        for folder in device_folders:
            pattern_for_device_folder = re.search('....:(..):..\..', folder)
            if pattern_for_device_folder:
                bus = pattern_for_device_folder.group(1)
                with open(os.path.join(SYSFS_PCI_DEVICE_PATH, folder, 'device'), 'r') as device_file:
                    # The 'device' file contain an hex repesantaion of the id key in the yaml file.
                    # We will strip the new line character, and remove the 0x prefix that is not needed.
                    device_id = device_file.read().strip().replace('0x', '')
                    self._device_id_to_bus_map[device_id] = bus

    def __init__(self, platform_path):
        PcieUtil.__init__(self, platform_path)
        self._create_device_id_to_bus_map()
