#!/usr/bin/env python

#############################################################################
# Celestica
#
# Component contains an implementation of SONiC Platform Base API and
# provides the chassis management function
#
#############################################################################

import os
import time

try:
    from sonic_platform_pddf_base.pddf_chassis import PddfChassis
    from sonic_platform_pddf_base.pddf_eeprom import PddfEeprom
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.fan_drawer import FanDrawer
    from sonic_platform.fan import Fan
    from sonic_platform.sfp import Sfp  
    from sonic_platform.watchdog import Watchdog
    from sonic_platform.component import Component
    from .helper import APIHelper
    from .thermal import NonPddfThermal, NONPDDF_THERMAL_SENSORS
    import sys
    import subprocess
    import shutil
    import re
    from sonic_py_common import device_info
    from sonic_platform_base.sfp_base import SfpBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

RESET_SOURCE_BIOS_REG = 0xA107


ORG_HW_REBOOT_CAUSE_FILE="/host/reboot-cause/hw-reboot-cause.txt"
TMP_HW_REBOOT_CAUSE_FILE="/tmp/hw-reboot-cause.txt"
SYS_LED_SYSFS_PATH = "/sys/devices/platform/sys_cpld/sys_led"

class Chassis(PddfChassis):
    """
    PDDF Platform-specific Chassis class
    """

    # Provide the functions/variables below for which implementation is to be overwritten
    
    sfp_status_dict = {}
    
    def __init__(self, pddf_data=None, pddf_plugin_data=None):

        PddfChassis.__init__(self, pddf_data, pddf_plugin_data)
        self._airflow_direction = None
        self._watchdog = None
        self._api_helper = APIHelper()          
        self.__initialize_components()

        for port_idx in range(1, self.platform_inventory['num_ports'] + 1):
            self.sfp_status_dict[port_idx] = self.get_sfp(port_idx).get_presence()

        if not self._api_helper.with_bmc():
            thermal_count = len(self._thermal_list)
            for idx, name in enumerate(NONPDDF_THERMAL_SENSORS):
                thermal = NonPddfThermal(thermal_count + idx, name)
                self._thermal_list.append(thermal)

    def __initialize_components(self):

        self.NUM_COMPONENT = 9
    
        if self._api_helper.with_bmc(): 
            self.NUM_COMPONENT = self.NUM_COMPONENT + 1

        for index in range(0, self.NUM_COMPONENT):
            component = Component(index)
            self._component_list.append(component)

    def get_name(self):
        return "DS5000"

    def initizalize_system_led(self):
        return True

    def get_sfp(self, index):
        sfp = None

        try:
            if index == 0:
                raise IndexError
            sfp = self._sfp_list[index - 1]
        except IndexError:
            sys.stderr.write("SFP index {} out of range (1-{})\n".format(
                index, len(self._sfp_list)))

        return sfp

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
        status, hw_reboot_cause = self._api_helper.cpld_lpc_read(RESET_SOURCE_BIOS_REG)
        if status == False: 
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, 'Hardware reason')

         # This tmp copy is to retain the reboot-cause only for the current boot
        if os.path.isfile(ORG_HW_REBOOT_CAUSE_FILE):
            shutil.move(ORG_HW_REBOOT_CAUSE_FILE, TMP_HW_REBOOT_CAUSE_FILE)

        if hw_reboot_cause == "0x33" and os.path.isfile(TMP_HW_REBOOT_CAUSE_FILE):
            with open(TMP_HW_REBOOT_CAUSE_FILE) as hw_cause_file:
                reboot_info = hw_cause_file.readline().rstrip('\n')
                match = re.search(r'Reason:(.*),Time:(.*)', reboot_info)
                if match is not None:
                    if match.group(1) == 'system':
                        return (self.REBOOT_CAUSE_NON_HARDWARE, 'System cold reboot')

        if hw_reboot_cause == "0x77":
            reboot_cause = self.REBOOT_CAUSE_POWER_LOSS
            description = 'Power Cycle Reset'
        elif hw_reboot_cause == "0x66":
            reboot_cause = self.REBOOT_CAUSE_WATCHDOG
            description = 'Hardware Watchdog Reset'
        elif hw_reboot_cause == "0x44":
            reboot_cause = self.REBOOT_CAUSE_NON_HARDWARE
            description = 'CPU Warm Reset'
        elif hw_reboot_cause == "0x33":
            reboot_cause = self.REBOOT_CAUSE_NON_HARDWARE
            description = 'Soft-Set Cold Reset'
        elif hw_reboot_cause == "0x22":
            reboot_cause = self.REBOOT_CAUSE_NON_HARDWARE
            description = 'Soft-Set Warm Reset'
        elif hw_reboot_cause == "0x11":
            reboot_cause = self.REBOOT_CAUSE_POWER_LOSS
            description = 'Power On Reset'
        else:
            reboot_cause = self.REBOOT_CAUSE_HARDWARE_OTHER
            description = 'Hardware reason'

        return (reboot_cause, description)

    def get_watchdog(self):
        """
        Retreives hardware watchdog device on this chassis
        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device
        """
        if self._watchdog is None:
            self._watchdog = Watchdog()

        return self._watchdog
        
    def get_revision(self):
        return self._eeprom.revision_str()
        
    @staticmethod
    def get_position_in_parent():
        return -1
        
    @staticmethod
    def is_replaceable():
        return False

    def get_status_led(self):
        led_color = "unknown"
        try:
            with open(SYS_LED_SYSFS_PATH, "r") as fd:
                led_color = fd.read().strip()
        except (FileNotFoundError, IOError):
            pass

        return led_color

    def set_status_led(self, color):     
        try:
            with open(SYS_LED_SYSFS_PATH, "w") as fd:
                fd.write(color)
        except (FileNotFoundError, IOError):
            return False

        return True

    def get_port_or_cage_type(self, index):
        if index in range(1, 64+1):
            return SfpBase.SFP_PORT_TYPE_BIT_OSFP
        elif index in range(65, 66+1):
            return SfpBase.SFP_PORT_TYPE_BIT_SFP28
        else:
            raise NotImplementedError

    ##############################################################
    ###################### Event methods #########################
    ##############################################################
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
        # SFP event
        sfp_dict = {}

        start_time = time.time()
        time_period = timeout / float(1000)  # Convert msecs to secss

        while time.time() < (start_time + time_period) or timeout == 0:
            for port_idx in range(1, self.platform_inventory['num_ports'] + 1):
                presence = self.get_sfp(port_idx).get_presence()
                if self.sfp_status_dict[port_idx] != presence:
                   self.sfp_status_dict[port_idx] = presence
                   sfp_dict[port_idx] = '1' if presence else '0'

            if sfp_dict:
                return True, {'sfp': sfp_dict}

            time.sleep(0.5)

        return True, {'sfp': {}}  # Timeout

    def get_thermal_manager(self):
        """
        Retrieves thermal manager class on this chasssis
        Returns:
            A class derived from ThermalManagerBase representing the
            specified thermal manager
        """
        if not self._api_helper.with_bmc():
            from .thermal_manager import ThermalManager
            return ThermalManager
        return None

    def get_airflow_direction(self):
        if self._airflow_direction == None:
            try:
                vendor_extn = self._eeprom.get_vendor_extn()
                airflow_type = vendor_extn.split()[2][2:4] # either 0xfb or 0xbf
                if airflow_type == 'FB':
                    direction = 'exhaust'
                elif airflow_type == 'BF':
                    direction = 'intake'
                else:
                    direction = 'N/A'
            except (AttributeError, IndexError):
                direction = 'N/A'

            self._airflow_direction = direction

        return self._airflow_direction
