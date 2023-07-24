#
# Copyright (c) 2019-2021 NVIDIA CORPORATION & AFFILIATES.
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
#############################################################################
# Mellanox
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Chassis information which are available in the platform
#
#############################################################################

try:
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_py_common.logger import Logger
    import os
    from functools import reduce
    from .utils import extract_RJ45_ports_index
    from . import utils
    from .device_data import DeviceDataManager
    import re
    import time
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")


RJ45_TYPE = "RJ45"

DMI_FILE = '/sys/firmware/dmi/entries/2-0/raw'
DMI_HEADER_LEN = 15
DMI_PRODUCT_NAME = "Product Name"
DMI_MANUFACTURER = "Manufacturer"
DMI_VERSION = "Version"
DMI_SERIAL = "Serial Number"
DMI_ASSET_TAG = "Asset Tag"
DMI_LOC = "Location In Chassis"
DMI_TABLE_MAP = {
                    DMI_PRODUCT_NAME: 0,
                    DMI_MANUFACTURER: 1,
                    DMI_VERSION: 2,
                    DMI_SERIAL: 3,
                    DMI_ASSET_TAG: 4,
                    DMI_LOC: 5
                }

HWMGMT_SYSTEM_ROOT = '/var/run/hw-management/system/'

#reboot cause related definitions
REBOOT_CAUSE_ROOT = HWMGMT_SYSTEM_ROOT

REBOOT_CAUSE_FILE_LENGTH = 1

REBOOT_TYPE_KEXEC_FILE = "/proc/cmdline"
REBOOT_TYPE_KEXEC_PATTERN_WARM = ".*SONIC_BOOT_TYPE=(warm|fastfast).*"
REBOOT_TYPE_KEXEC_PATTERN_FAST = ".*SONIC_BOOT_TYPE=(fast|fast-reboot).*"

# Global logger class instance
logger = Logger()

class Chassis(ChassisBase):
    """Platform-specific Chassis class"""

    # System status LED
    _led = None

    # System UID LED
    _led_uid = None

    def __init__(self):
        super(Chassis, self).__init__()

        # Initialize DMI data
        self.dmi_data = None

        # move the initialization of each components to their dedicated initializer
        # which will be called from platform
        #
        # Multiple scenarios need to be taken into consideration regarding the SFP modules initialization.
        # - Platform daemons
        #   - Can access multiple or all SFP modules
        # - sfputil
        #   - Sometimes can access only one SFP module
        #   - Call get_sfp to get one SFP module object
        #
        # We should initialize all SFP modules only if it is necessary because initializing SFP module is time-consuming.
        # This means,
        # - If get_sfp is called,
        #    - If the _sfp_list isn't initialized, initialize it first.
        #    - Only the SFP module being required should be initialized.
        # - If get_all_sfps is called,
        #    - If the _sfp_list isn't initialized, initialize it first.
        #    - All SFP modules need to be initialized.
        #      But the SFP modules that have already been initialized should not be initialized for the second time.
        #      This can caused by get_sfp being called before.
        #
        # Due to the complexity of SFP modules initialization, we have to introduce two initialized flags for SFP modules
        # - sfp_module_partial_initialized:
        #    - False: The _sfp_list is [] (SFP stuff has never been accessed)
        #    - True: The _sfp_list is a list whose length is number of SFP modules supported by the platform
        # - sfp_module_full_initialized:
        #    - False: All SFP modules have not been created
        #    - True: All SFP modules have been created
        #
        self.sfp_initialized_count = 0
        self.sfp_event = None
        self.reboot_cause_initialized = False

        self.sfp_module = None

        # Build the RJ45 port list from platform.json and hwsku.json
        self._RJ45_port_inited = False
        self._RJ45_port_list = None

        logger.log_info("Chassis loaded successfully")

    def __del__(self):
        if self.sfp_event:
            self.sfp_event.deinitialize()

        if self._sfp_list:
            if self.sfp_module.SFP.shared_sdk_handle:
                self.sfp_module.deinitialize_sdk_handle(self.sfp_module.SFP.shared_sdk_handle)

    @property
    def RJ45_port_list(self):
        if not self._RJ45_port_inited:
            self._RJ45_port_list = extract_RJ45_ports_index()
            self._RJ45_port_inited = True
        return self._RJ45_port_list

    ##############################################
    # PSU methods
    ##############################################

    def initialize_psu(self):
        if not self._psu_list:
            from .psu import Psu, FixedPsu
            psu_count = DeviceDataManager.get_psu_count()
            hot_swapable = DeviceDataManager.is_psu_hotswapable()

            # Initialize PSU list
            for index in range(psu_count):
                if hot_swapable:
                    psu = Psu(index)
                else:
                    psu = FixedPsu(index)
                self._psu_list.append(psu)

    def get_num_psus(self):
        """
        Retrieves the number of power supply units available on this chassis

        Returns:
            An integer, the number of power supply units available on this
            chassis
        """
        self.initialize_psu()
        return len(self._psu_list)

    def get_all_psus(self):
        """
        Retrieves all power supply units available on this chassis

        Returns:
            A list of objects derived from PsuBase representing all power
            supply units available on this chassis
        """
        self.initialize_psu()
        return self._psu_list

    def get_psu(self, index):
        """
        Retrieves power supply unit represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the power supply unit to
            retrieve

        Returns:
            An object dervied from PsuBase representing the specified power
            supply unit
        """
        self.initialize_psu()
        return super(Chassis, self).get_psu(index)

    ##############################################
    # Fan methods
    ##############################################

    def initialize_fan(self):
        if not self._fan_drawer_list:
            from .fan import Fan
            from .fan_drawer import RealDrawer, VirtualDrawer

            hot_swapable = DeviceDataManager.is_fan_hotswapable()
            drawer_num = DeviceDataManager.get_fan_drawer_count()
            fan_num = DeviceDataManager.get_fan_count()
            fan_num_per_drawer = fan_num // drawer_num
            drawer_ctor = RealDrawer if hot_swapable else VirtualDrawer
            fan_index = 0
            for drawer_index in range(drawer_num):
                drawer = drawer_ctor(drawer_index)
                self._fan_drawer_list.append(drawer)
                for index in range(fan_num_per_drawer):
                    fan = Fan(fan_index, drawer, index + 1)
                    fan_index += 1
                    drawer._fan_list.append(fan)

    def get_num_fan_drawers(self):
        """
        Retrieves the number of fan drawers available on this chassis

        Returns:
            An integer, the number of fan drawers available on this chassis
        """
        return DeviceDataManager.get_fan_drawer_count()

    def get_all_fan_drawers(self):
        """
        Retrieves all fan drawers available on this chassis

        Returns:
            A list of objects derived from FanDrawerBase representing all fan
            drawers available on this chassis
        """
        self.initialize_fan()
        return self._fan_drawer_list

    def get_fan_drawer(self, index):
        """
        Retrieves fan drawers represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the fan drawer to
            retrieve

        Returns:
            An object dervied from FanDrawerBase representing the specified fan
            drawer
        """
        self.initialize_fan()
        return super(Chassis, self).get_fan_drawer(index)

    ##############################################
    # SFP methods
    ##############################################

    def _import_sfp_module(self):
        if not self.sfp_module:
            from . import sfp as sfp_module
            self.sfp_module = sfp_module
        return self.sfp_module

    def initialize_single_sfp(self, index):
        sfp_count = self.get_num_sfps()
        if index < sfp_count:
            if not self._sfp_list:
                self._sfp_list = [None] * sfp_count

            if not self._sfp_list[index]:
                sfp_module = self._import_sfp_module()
                if self.RJ45_port_list and index in self.RJ45_port_list:
                    self._sfp_list[index] = sfp_module.RJ45Port(index)
                else:
                    self._sfp_list[index] = sfp_module.SFP(index)
                self.sfp_initialized_count += 1

    def initialize_sfp(self):
        if not self._sfp_list:
            sfp_module = self._import_sfp_module()
            sfp_count = self.get_num_sfps()
            for index in range(sfp_count):
                if self.RJ45_port_list and index in self.RJ45_port_list:
                    sfp_object = sfp_module.RJ45Port(index)
                else:
                    sfp_object = sfp_module.SFP(index)
                self._sfp_list.append(sfp_object)
            self.sfp_initialized_count = sfp_count
        elif self.sfp_initialized_count != len(self._sfp_list):
            sfp_module = self._import_sfp_module()
            for index in range(len(self._sfp_list)):
                if self._sfp_list[index] is None:
                    if self.RJ45_port_list and index in self.RJ45_port_list:
                        self._sfp_list[index] = sfp_module.RJ45Port(index)
                    else:
                        self._sfp_list[index] = sfp_module.SFP(index)
            self.sfp_initialized_count = len(self._sfp_list)

    def get_num_sfps(self):
        """
        Retrieves the number of sfps available on this chassis

        Returns:
            An integer, the number of sfps available on this chassis
        """
        return DeviceDataManager.get_sfp_count()

    def get_all_sfps(self):
        """
        Retrieves all sfps available on this chassis

        Returns:
            A list of objects derived from SfpBase representing all sfps
            available on this chassis
        """
        self.initialize_sfp()
        return self._sfp_list

    def get_sfp(self, index):
        """
        Retrieves sfp represented by (1-based) index <index>

        Args:
            index: An integer, the index (1-based) of the sfp to retrieve.
                   The index should be the sequence of a physical port in a chassis,
                   starting from 1.
                   For example, 1 for Ethernet0, 2 for Ethernet4 and so on.

        Returns:
            An object dervied from SfpBase representing the specified sfp
        """
        index = index - 1
        self.initialize_single_sfp(index)
        return super(Chassis, self).get_sfp(index)

    def get_port_or_cage_type(self, index):
        """
        Retrieves sfp port or cage type corresponding to physical port <index>

        Args:
            index: An integer (>=0), the index of the sfp to retrieve.
                   The index should correspond to the physical port in a chassis.
                   For example:-
                   1 for Ethernet0, 2 for Ethernet4 and so on for one platform.
                   0 for Ethernet0, 1 for Ethernet4 and so on for another platform.

        Returns:
            The masks of all types of port or cage that can be supported on the port
            Types are defined in sfp_base.py
            Eg.
                Both SFP and SFP+ are supported on the port, the return value should be 0x0a
                which is 0x02 | 0x08
        """
        index = index - 1
        if self.RJ45_port_list and index in self.RJ45_port_list:
            from sonic_platform_base.sfp_base import SfpBase
            return SfpBase.SFP_PORT_TYPE_BIT_RJ45
        raise NotImplementedError

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
        self.initialize_sfp()
        # Initialize SFP event first
        if not self.sfp_event:
            from .sfp_event import sfp_event
            self.sfp_event = sfp_event(self.RJ45_port_list)
            self.sfp_event.initialize()

        wait_for_ever = (timeout == 0)
        # select timeout should be no more than 1000ms to ensure fast shutdown flow
        select_timeout = 1000.0 if timeout >= 1000 else float(timeout)
        port_dict = {}
        error_dict = {}
        begin = time.time()
        while True:
            status = self.sfp_event.check_sfp_status(port_dict, error_dict, select_timeout)
            if bool(port_dict):
                break

            if not wait_for_ever:
                elapse = time.time() - begin
                if elapse * 1000 > timeout:
                    break

        if status:
            if port_dict:
                self.reinit_sfps(port_dict)
            result_dict = {'sfp': port_dict}
            if error_dict:
                result_dict['sfp_error'] = error_dict
            return True, result_dict
        else:
            return True, {'sfp': {}}

    def reinit_sfps(self, port_dict):
        """
        Re-initialize SFP if there is any newly inserted SFPs
        :param port_dict: SFP event data
        :return:
        """
        from . import sfp
        for index, status in port_dict.items():
            if status == sfp.SFP_STATUS_INSERTED:
                try:
                    self._sfp_list[index - 1].reinit()
                except Exception as e:
                    logger.log_error("Fail to re-initialize SFP {} - {}".format(index, repr(e)))

    def _show_capabilities(self):
        """
        This function is for debug purpose
        Some features require a xSFP module to support some capabilities but it's unrealistic to
        check those modules one by one.
        So this function is introduce to show some capabilities of all xSFP modules mounted on the device.
        """
        self.initialize_sfp()
        for s in self._sfp_list:
            try:
                print("index {} tx disable {} dom {} calibration {} temp {} volt {} power (tx {} rx {})".format(s.index,
                    s.dom_tx_disable_supported,
                    s.dom_supported,
                    s.calibration,
                    s.dom_temp_supported,
                    s.dom_volt_supported,
                    s.dom_rx_power_supported,
                    s.dom_tx_power_supported
                    ))
            except:
                print("fail to retrieve capabilities for module index {}".format(s.index))

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
            An object dervied from ThermalBase representing the specified thermal
        """
        self.initialize_thermals()
        return super(Chassis, self).get_thermal(index)

    ##############################################
    # EEPROM methods
    ##############################################

    def initialize_eeprom(self):
        if not self._eeprom:
            from .eeprom import Eeprom
            # Initialize EEPROM
            self._eeprom = Eeprom()

    def get_eeprom(self):
        """
        Retreives eeprom device on this chassis

        Returns:
            An object derived from WatchdogBase representing the hardware
            eeprom device
        """
        self.initialize_eeprom()
        return self._eeprom

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        self.initialize_eeprom()
        return self._eeprom.get_product_name()

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        self.initialize_eeprom()
        return self._eeprom.get_part_number()

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis

        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        self.initialize_eeprom()
        return self._eeprom.get_base_mac()

    def get_serial(self):
        """
        Retrieves the hardware serial number for the chassis

        Returns:
            A string containing the hardware serial number for this chassis.
        """
        self.initialize_eeprom()
        return self._eeprom.get_serial_number()

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the chassis

        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
        """
        self.initialize_eeprom()
        return self._eeprom.get_system_eeprom_info()

    ##############################################
    # Component methods
    ##############################################

    def initialize_components(self):
        if not utils.is_host():
            return
        if not self._component_list:
            # Initialize component list
            from .component import ComponentONIE, ComponentSSD, ComponentBIOS, ComponentCPLD
            self._component_list.append(ComponentONIE())
            self._component_list.append(ComponentSSD())
            self._component_list.append(DeviceDataManager.get_bios_component())
            self._component_list.extend(DeviceDataManager.get_cpld_component_list())

    def get_num_components(self):
        """
        Retrieves the number of components available on this chassis

        Returns:
            An integer, the number of components available on this chassis
        """
        self.initialize_components()
        return len(self._component_list)

    def get_all_components(self):
        """
        Retrieves all components available on this chassis

        Returns:
            A list of objects derived from ComponentBase representing all components
            available on this chassis
        """
        self.initialize_components()
        return self._component_list

    def get_component(self, index):
        """
        Retrieves component represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the component to retrieve

        Returns:
            An object dervied from ComponentBase representing the specified component
        """
        self.initialize_components()
        return super(Chassis, self).get_component(index)

    ##############################################
    # System LED methods
    ##############################################

    def initizalize_system_led(self):
        if not Chassis._led:
            from .led import SystemLed, \
                SystemUidLed
            Chassis._led = SystemLed()
            Chassis._led_uid = SystemUidLed()

    def set_status_led(self, color):
        """
        Sets the state of the system LED

        Args:
            color: A string representing the color with which to set the
                   system LED

        Returns:
            bool: True if system LED state is set successfully, False if not
        """
        self.initizalize_system_led()
        return False if not Chassis._led else Chassis._led.set_status(color)

    def get_status_led(self):
        """
        Gets the state of the system LED

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        self.initizalize_system_led()
        return None if not Chassis._led else Chassis._led.get_status()

    def set_uid_led(self, color):
        """
        Sets the state of the system UID LED

        Args:
            color: A string representing the color with which to set the
                   system UID LED

        Returns:
            bool: True if system LED state is set successfully, False if not
        """
        self.initizalize_system_led()
        return False if not Chassis._led_uid else Chassis._led_uid.set_status(color)

    def get_uid_led(self):
        """
        Gets the state of the system UID LED

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        self.initizalize_system_led()
        return None if not Chassis._led_uid else Chassis._led_uid.get_status()

    def get_watchdog(self):
        """
        Retrieves hardware watchdog device on this chassis

        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device

        Note:
            We overload this method to ensure that watchdog is only initialized
            when it is referenced. Currently, only one daemon can open the watchdog.
            To initialize watchdog in the constructor causes multiple daemon
            try opening watchdog when loading and constructing a chassis object
            and fail. By doing so we can eliminate that risk.
        """
        try:
            if self._watchdog is None:
                from .watchdog import get_watchdog
                self._watchdog = get_watchdog()
        except Exception as e:
            logger.log_info("Fail to load watchdog due to {}".format(repr(e)))

        return self._watchdog


    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        if self.dmi_data is None:
            self.dmi_data = self._parse_dmi(DMI_FILE)

        return self.dmi_data.get(DMI_VERSION, "N/A")

    def _parse_dmi(self, filename):
        """
        Read DMI data chassis data and returns a dictionary of values

        Returns:
            A dictionary containing the dmi table of the switch chassis info
        """
        result = {}
        try:
            if not os.access(filename, os.R_OK):
                return result

            with open(filename, "rb") as fileobj:
                data = fileobj.read()

            body = data[DMI_HEADER_LEN:]
            records = body.split(b'\x00')

            for k, v in DMI_TABLE_MAP.items():
                result[k] = records[v].decode("utf-8")

        except Exception as e:
            logger.log_error("Fail to decode DMI {} due to {}".format(filename, repr(e)))

        return result

    def _verify_reboot_cause(self, filename):
        '''
        Open and read the reboot cause file in
        /var/run/hwmanagement/system (which is defined as REBOOT_CAUSE_ROOT)
        If a reboot cause file doesn't exists, returns '0'.
        '''
        return bool(utils.read_int_from_file(os.path.join(REBOOT_CAUSE_ROOT, filename), log_func=None))

    def initialize_reboot_cause(self):
        self.reboot_major_cause_dict = {
            'reset_main_pwr_fail'       :   self.REBOOT_CAUSE_POWER_LOSS,
            'reset_aux_pwr_or_ref'      :   self.REBOOT_CAUSE_POWER_LOSS,
            'reset_comex_pwr_fail'      :   self.REBOOT_CAUSE_POWER_LOSS,
            'reset_asic_thermal'        :   self.REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC,
            'reset_comex_thermal'       :   self.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU,
            'reset_hotswap_or_wd'       :   self.REBOOT_CAUSE_WATCHDOG,
            'reset_comex_wd'            :   self.REBOOT_CAUSE_WATCHDOG,
            'reset_swb_wd'              :   self.REBOOT_CAUSE_WATCHDOG,
            'reset_sff_wd'              :   self.REBOOT_CAUSE_WATCHDOG,
            'reset_hotswap_or_halt'     :   self.REBOOT_CAUSE_HARDWARE_OTHER,
            'reset_voltmon_upgrade_fail':   self.REBOOT_CAUSE_HARDWARE_OTHER,
            'reset_reload_bios'         :   self.REBOOT_CAUSE_HARDWARE_BIOS,
            'reset_fw_reset'            :   self.REBOOT_CAUSE_HARDWARE_RESET_FROM_ASIC,
            'reset_from_asic'           :   self.REBOOT_CAUSE_HARDWARE_RESET_FROM_ASIC,
            'reset_long_pb'             :   self.REBOOT_CAUSE_HARDWARE_BUTTON,
            'reset_short_pb'            :   self.REBOOT_CAUSE_HARDWARE_BUTTON
        }
        self.reboot_minor_cause_dict = {}
        self.reboot_by_software = 'reset_sw_reset'
        self.reboot_cause_initialized = True

    def _parse_warmfast_reboot_from_proc_cmdline(self):
        if os.path.isfile(REBOOT_TYPE_KEXEC_FILE):
            with open(REBOOT_TYPE_KEXEC_FILE) as cause_file:
                cause_file_kexec = cause_file.readline()
            m = re.search(REBOOT_TYPE_KEXEC_PATTERN_WARM, cause_file_kexec)
            if m and m.group(1):
                return 'warm-reboot'
            m = re.search(REBOOT_TYPE_KEXEC_PATTERN_FAST, cause_file_kexec)
            if m and m.group(1):
                return 'fast-reboot'
        return None

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
        #read reboot causes files in the following order

        # To avoid the leftover hardware reboot cause confusing the reboot cause determine service
        # Skip the hardware reboot cause check if warm/fast reboot cause found from cmdline
        if utils.is_host():
            reboot_cause = self._parse_warmfast_reboot_from_proc_cmdline()
            if reboot_cause:
                return self.REBOOT_CAUSE_NON_HARDWARE, ''

        if not self.reboot_cause_initialized:
            self.initialize_reboot_cause()

        for reset_file, reset_cause in self.reboot_major_cause_dict.items():
            if self._verify_reboot_cause(reset_file):
                return reset_cause, ''

        for reset_file, reset_cause in self.reboot_minor_cause_dict.items():
            if self._verify_reboot_cause(reset_file):
                return self.REBOOT_CAUSE_HARDWARE_OTHER, reset_cause

        if self._verify_reboot_cause(self.reboot_by_software):
            logger.log_info("Hardware reboot cause: the system was rebooted due to software requesting")
        else:
            logger.log_info("Hardware reboot cause: no hardware reboot cause found")

        return self.REBOOT_CAUSE_NON_HARDWARE, ''

    def get_thermal_manager(self):
        from .thermal_manager import ThermalManager
        return ThermalManager

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


class ModularChassis(Chassis):
    def __init__(self):
        super(ModularChassis, self).__init__()
        self.module_initialized_count = 0

    def is_modular_chassis(self):
        """
        Retrieves whether the sonic instance is part of modular chassis

        Returns:
            A bool value, should return False by default or for fixed-platforms.
            Should return True for supervisor-cards, line-cards etc running as part
            of modular-chassis.
        """
        return True

    ##############################################
    # Module methods
    ##############################################
    def initialize_single_module(self, index):
        count = self.get_num_modules()
        if index < count:
            if not self._module_list:
                self._module_list = [None] * count

            if not self._module_list[index]:
                from .module import Module
                self._module_list[index] = Module(index + 1)
                self.module_initialized_count += 1

    def initialize_modules(self):
        if not self._module_list:
            from .module import Module
            count = self.get_num_modules()
            for index in range(1, count + 1):
                self._module_list.append(Module(index))
            self.module_initialized_count = count
        elif self.module_initialized_count != len(self._module_list):
            from .module import Module
            for index in range(len(self._module_list)):
                if self._module_list[index] is None:
                    self._module_list[index] = Module(index + 1)
            self.module_initialized_count = len(self._module_list)

    def get_num_modules(self):
        """
        Retrieves the number of modules available on this chassis

        Returns:
            An integer, the number of modules available on this chassis
        """
        return DeviceDataManager.get_linecard_count()

    def get_all_modules(self):
        """
        Retrieves all modules available on this chassis

        Returns:
            A list of objects derived from ModuleBase representing all
            modules available on this chassis
        """
        self.initialize_modules()
        return self._module_list

    def get_module(self, index):
        """
        Retrieves module represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the module to
            retrieve

        Returns:
            An object dervied from ModuleBase representing the specified
            module
        """
        self.initialize_single_module(index)
        return super(ModularChassis, self).get_module(index)

    @utils.default_return(-1)
    def get_module_index(self, module_name):
        """
        Retrieves module index from the module name

        Args:
            module_name: A string, prefixed by SUPERVISOR, LINE-CARD or FABRIC-CARD
            Ex. SUPERVISOR0, LINE-CARD1, FABRIC-CARD5

        Returns:
            An integer, the index of the ModuleBase object in the module_list
        """
        return int(module_name[len('LINE-CARD')-1:])

    ##############################################
    # SFP methods
    ##############################################

    def get_num_sfps(self):
        """
        Retrieves the number of sfps available on this chassis

        Returns:
            An integer, the number of sfps available on this chassis
        """
        return reduce(lambda x, y: x + y, (x.get_num_sfps() for x in self.get_all_modules()))

    def get_all_sfps(self):
        """
        Retrieves all sfps available on this chassis

        Returns:
            A list of objects derived from SfpBase representing all sfps
            available on this chassis
        """
        return reduce(lambda x, y: x + y, (x.get_all_sfps() for x in self.get_all_modules()))

    def get_sfp(self, index):
        """
        Retrieves sfp represented by (1-based) index <index>

        Args:
            index: An integer, the index (1-based) of the sfp to retrieve.
                   The index should be the sequence of a physical port in a chassis,
                   starting from 1.
                   For example, 1 for Ethernet0, 2 for Ethernet4 and so on.

        Returns:
            An object dervied from SfpBase representing the specified sfp
        """
        sfp_index = index % DeviceDataManager.get_linecard_max_port_count() - 1
        slot_id = int((index - sfp_index - 1) / 16) + 1
        module = self.get_module(slot_id - 1)
        if not module:
            return None

        return module.get_sfp(sfp_index - 1)
