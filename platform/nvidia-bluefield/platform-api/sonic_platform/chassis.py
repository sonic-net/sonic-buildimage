#
# Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
# Apache-2.0
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

try:
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform_base.watchdog_base import WatchdogBase
    from sonic_py_common.logger import Logger
    import os
    from functools import reduce
    from time import sleep
    import psutil
    import ipaddress
    import socket

    from . import utils
    from .device_data import DeviceDataManager
    from .sfp import Sfp
    from .sfp_event import SfpEvent
    from .eeprom import Eeprom
    from .watchdog import Watchdog
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

MAX_SELECT_DELAY = 3600


# Global logger class instance
logger = Logger()


class Chassis(ChassisBase):
    """Platform-specific Chassis class"""

    def __init__(self):
        super(Chassis, self).__init__()

        # Initialize DMI data
        self.dmi_data = None

        self.device_data = DeviceDataManager()
        self._initialize_sfp()
        self.sfp_event = None
        self._eeprom = Eeprom()
        self._watchdog = Watchdog()

        logger.log_info("Chassis loaded successfully")

    def _get_dpu_id(self):
        ip = None
        midplane_inft = psutil.net_if_addrs().get('eth0-midplane', [])

        for address in midplane_inft:
            if address.family == socket.AF_INET:
                ip = ipaddress.IPv4Address(address.address)
                break

        if not ip:
            raise RuntimeError("Midplane interface IP address is not available")

        last_byte = int(str(ip).split('.')[-1])

        return last_byte - 1

    def _initialize_sfp(self):
        self._sfp_list = []

        sfp_count = self.get_num_sfps()
        for index in range(sfp_count):
            sfp_module = Sfp(index, self.device_data.get_sfp_data(index))
            self._sfp_list.append(sfp_module)
        self._sfp_event = SfpEvent(self._sfp_list)

    def get_sfp(self, index):
        return super(Chassis, self).get_sfp(index - 1)

    def get_num_sfps(self):
        """
        Retrieves the number of sfps available on this chassis

        Returns:
            An integer, the number of sfps available on this chassis
        """
        return self.device_data.get_sfp_count()

    def get_change_event(self, timeout=0):
        """
        Returns a nested dictionary containing all devices which have
        experienced a change at chassis level

        Args:
            timeout: Timeout in milliseconds (optional). If timeout == 0,
                this method will block until a change is detected.

        Returns:
            (bool, dict):
                - True if call successful, False if not;
                - A nested dictionary where key is a device type,
                  value is a dictionary with key:value pairs in the format of
                  {'device_id':'device_event'},
                  where device_id is the device ID for this device and
                        device_event,
                             status='1' represents device inserted,
                             status='0' represents device removed.
                  Ex. {'fan':{'0':'0', '2':'1'}, 'sfp':{'11':'0'}}
                      indicates that fan 0 has been removed, fan 2
                      has been inserted and sfp 11 has been removed.
        """

        return self._sfp_event.get_sfp_event(timeout)

    def get_eeprom(self):
        """
        Retrieves eeprom device on this chassis

        Returns:
            An object derived from WatchdogBase representing the hardware
            eeprom device
        """
        return self._eeprom

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return self._eeprom.get_product_name()

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        return self._eeprom.get_part_number()

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis

        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._eeprom.get_base_mac()

    def get_serial(self):
        """
        Retrieves the hardware serial number for the chassis

        Returns:
            A string containing the hardware serial number for this chassis.
        """
        return self._eeprom.get_serial_number()

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the chassis

        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
        """
        return self._eeprom.get_system_eeprom_info()

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return self._eeprom.get_revision()

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

    ##############################################
    # THERMAL methods
    ##############################################

    def initialize_thermals(self):
        if not self._thermal_list:
            from .thermal import initialize_chassis_thermals
            # Initialize thermals
            self._thermal_list = initialize_chassis_thermals()

    def get_num_thermals(self):
        """
        Retrieves the number of thermals available on this chassis

        Returns:
            An integer, the number of thermals available on this chassis
        """
        self.initialize_thermals()
        return len(self._thermal_list)

    def get_all_thermals(self):
        """
        Retrieves all thermals available on this chassis

        Returns:
            A list of objects derived from ThermalBase representing all thermals
            available on this chassis
        """
        self.initialize_thermals()
        return self._thermal_list

    def get_thermal(self, index):
        """
        Retrieves thermal unit represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the thermal to
            retrieve

        Returns:
            An object derived from ThermalBase representing the specified thermal
        """
        self.initialize_thermals()
        return super(Chassis, self).get_thermal(index)

    ##############################################
    # System LED methods
    ##############################################

    def initizalize_system_led(self):
        pass

    def set_status_led(self, color):
        """
        Sets the state of the system LED

        Args:
            color: A string representing the color with which to set the
                   system LED

        Returns:
            bool: True if system LED state is set successfully, False if not
        """
        return False

    def get_status_led(self):
        """
        Gets the state of the system LED

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        return 'N/A'

    def get_reboot_cause(self):
        """
        Retrieves the cause of the previous reboot
        Returns:
            A tuple (string, string) where the first element is a string
            containing the cause of the previous reboot. This string must be
            one of the predefined strings in this class. If the first string
            is "REBOOT_CAUSE_HARDWARE_OTHER", the second string can be used
            to pass a description of the reboot cause.
        """
        return "N/A", "N/A"

    ##############################################
    # SmartSwitch methods
    ##############################################

    def get_dpu_id(self, **kwargs):
        """
        For the smart switch DPU retrieves the ID of the DPU.
        Returns None for non-smartswitch chassis.

        Returns:
            An integer, indicating the DPU ID Ex: name:DPU0 return value 0,
            name:DPU1 return value 1, name:DPUX return value X
        """
        return self._get_dpu_id()

    def is_smartswitch(self):
        """
        Retrieves whether the sonic instance is part of smartswitch

        Returns:
            Returns:True for SmartSwitch and False for other platforms
        """
        return True

    def is_dpu(self):
        """
        Retrieves whether the SONiC instance runs on the DPU

        Returns:
            True if the SONiC instance runs on the DPU else False
        """
        return True
