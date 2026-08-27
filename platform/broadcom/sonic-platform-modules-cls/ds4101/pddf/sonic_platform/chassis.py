#!/usr/bin/env python
# @Company ：Celestica
# @Time    : 2023/3/2 10:19
# @Mail    : yajiang@celestica.com
# @Author  : jiang tao

try:
    import sys
    import time
    import syslog
    import subprocess
    import shutil
    import os
    import re
    from . import helper
    from . import component
    from .watchdog import Watchdog
    from .thermal import ThermalMon, THERMAL_THRESHOLDS
    from sonic_platform_pddf_base.pddf_chassis import PddfChassis
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

bcm_exist = helper.APIHelper().get_bmc_status()
SET_SYS_STATUS_LED = ["0x3a", "0x39", "0x02", "0x00"]
SET_LED_MODE_Manual = ["0x3a", "0x42", "0x02", "0x00"]
SET_LED_MODE_Auto = ["0x3a", "0x42", "0x02", "0x01"]
REBOOT_CAUSE_PATH = "/sys/devices/platform/cpld_wdt/reason"
HW_REBOOT_CAUSE_FILE="/host/reboot-cause/hw-reboot-cause.txt"
SYS_LED_SYSFS_PATH = "/sys/bus/platform/devices/sys_cpld/sys_led"
TMP_HW_REBOOT_CAUSE_FILE="/tmp/hw-reboot-cause.txt"

class Chassis(PddfChassis):
    """
    PDDF Platform-specific Chassis class
    """
    sfp_status_dict = {}

    def __init__(self, pddf_data=None, pddf_plugin_data=None):
        self.helper = helper.APIHelper()
        PddfChassis.__init__(self, pddf_data, pddf_plugin_data)

        for port_idx in range(1, self.platform_inventory['num_ports'] + 1):
            self.sfp_status_dict[port_idx] = self.get_sfp(port_idx).get_presence()

        if not bcm_exist:
            thermal_count = len(self._thermal_list)
            thermal_monitor_sensors = sorted(THERMAL_THRESHOLDS.keys())
            for idx, name in enumerate(thermal_monitor_sensors):
                thermal = ThermalMon(thermal_count + idx, name)
                self._thermal_list.append(thermal)

        for index in range(self.platform_inventory['num_component']):
            component_obj = component.Component(index)
            self._component_list.append(component_obj)

    @staticmethod
    def _getstatusoutput(cmd):
        try:
            data = subprocess.check_output(cmd, shell=False,
                                           universal_newlines=True, stderr=subprocess.STDOUT)
            status = 0
        except subprocess.CalledProcessError as ex:
            data = ex.output
            status = ex.returncode
        if data[-1:] == '\n':
            data = data[:-1]
        return status, data

    @staticmethod
    def initizalize_system_led():
        return True

    def get_status_led(self):
        sys_led_color = "unknown"
        try:
            with open(SYS_LED_SYSFS_PATH, "r") as fd:
                sys_led_color = fd.read().rstrip('\n')
        except Exception as err:
            print(f"Failed to get status LED due to: {err}")
        return sys_led_color

    def set_status_led(self, color):     
        try:
            with open(SYS_LED_SYSFS_PATH, "w") as fd:
                fd.write(color)
        except Exception as err:
            print(f"Failed to set status LED due to: {err}")
            return False
        return True

    def get_sfp(self, index):
        """
        Retrieves sfp represented by (1-based) index <index>
        For Quanta the index in sfputil.py starts from 1, so override
        Args:
            index: An integer, the index (1-based) of the sfp to retrieve.
            The index should be the sequence of a physical port in a chassis,
            starting from 1.
        Returns:
            An object dervied from SfpBase representing the specified sfp
        """
        sfp = None

        try:
            if index == 0:
                raise IndexError
            sfp = self._sfp_list[index - 1]
        except IndexError:
            sys.stderr.write("override: SFP index {} out of range (1-{})\n".format(
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
            REBOOT_CAUSE_POWER_LOSS = "Power Loss"
            REBOOT_CAUSE_THERMAL_OVERLOAD_CPU = "Thermal Overload: CPU"
            REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC = "Thermal Overload: ASIC"
            REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER = "Thermal Overload: Other"
            REBOOT_CAUSE_INSUFFICIENT_FAN_SPEED = "Insufficient Fan Speed"
            REBOOT_CAUSE_WATCHDOG = "Watchdog"
            REBOOT_CAUSE_HARDWARE_OTHER = "Hardware - Other"
            REBOOT_CAUSE_NON_HARDWARE = "Non-Hardware"
        """

        # This tmp copy is to retain the reboot-cause only for the current boot
        if os.path.isfile(HW_REBOOT_CAUSE_FILE):
            shutil.move(HW_REBOOT_CAUSE_FILE, TMP_HW_REBOOT_CAUSE_FILE)        
        reboot_cause = self.helper.read_txt_file(REBOOT_CAUSE_PATH) or "Unknown"

        reboot_cause_description = {
            '0x11': (self.REBOOT_CAUSE_POWER_LOSS, "Power on reset"),
            '0x22': (self.REBOOT_CAUSE_HARDWARE_CPU, "Soft-set CPU warm reset"),
            '0x44': (self.REBOOT_CAUSE_NON_HARDWARE, "CPU warm reset"),
            '0x66': (self.REBOOT_CAUSE_WATCHDOG, "Hardware Watchdog Reset"),
            '0x77': (self.REBOOT_CAUSE_HARDWARE_CPU, "Power cycle reset"),

        }
        if reboot_cause == '0x33':
            if os.path.isfile(TMP_HW_REBOOT_CAUSE_FILE):
                with open(TMP_HW_REBOOT_CAUSE_FILE) as hw_cause_file:
                    reboot_info = hw_cause_file.readline().rstrip('\n')
                    match = re.search(r'Reason:(.*),Time:(.*)', reboot_info)
                    if match is not None and match.group(1) == 'system':
                        return (self.REBOOT_CAUSE_NON_HARDWARE, 'System cold reboot')
            return (self.REBOOT_CAUSE_HARDWARE_CPU, "Soft-set CPU cold reset")

        return reboot_cause_description.get(reboot_cause,
                                    (self.REBOOT_CAUSE_NON_HARDWARE, "Unknown reason"))

    def get_revision(self):
        """
        Retrieves the hardware revision for the chassis
        Returns:
            A string containing the hardware revision for this chassis.
        """
        return self._eeprom.get_revision()

    def get_watchdog(self):
        """
        Retreives hardware watchdog device on this chassis

        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device
        """

        try:

            if self._watchdog is None:
                # Create the watchdog Instance
                self._watchdog = Watchdog()
        except Exception as E:
            syslog.syslog(syslog.LOG_ERR, "Fail to load watchdog due to {}".format(E))
        return self._watchdog

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
