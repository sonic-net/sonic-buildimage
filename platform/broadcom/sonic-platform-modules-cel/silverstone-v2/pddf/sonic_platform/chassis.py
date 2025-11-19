#!/usr/bin/env python
# @Company ：Celestica
# @Time    : 2023/7/22 15:37
# @Mail    : yajiang@celestica.com
# @Author  : jiang tao

try:
    import sys
    import time
    import syslog
    import subprocess
    import os
    import re
    import shutil
    from . import helper
    from . import component
    from .watchdog import Watchdog
    from sonic_platform_pddf_base.pddf_chassis import PddfChassis
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

BMC_EXIST = helper.APIHelper().get_bmc_status()

REBOOT_CAUSE_PATH = "/sys/devices/platform/cpld_wdt/reason"
SET_SYS_STATUS_LED = "0x3A 0x39 0x02 0x00 {}"

ORG_HW_REBOOT_CAUSE_FILE="/host/reboot-cause/hw-reboot-cause.txt"
TMP_HW_REBOOT_CAUSE_FILE="/tmp/hw-reboot-cause.txt"


class Chassis(PddfChassis):
    """
    PDDF Platform-specific Chassis class
    """
    sfp_status_dict = {}

    def __init__(self, pddf_data=None, pddf_plugin_data=None):
        self.helper = helper.APIHelper()
        PddfChassis.__init__(self, pddf_data, pddf_plugin_data)
        self._watchdog = None
        self._airflow_direction = None

        for port_idx in range(1, self.platform_inventory['num_ports'] + 1):
            present = self.get_sfp(port_idx).get_presence()
            self.sfp_status_dict[port_idx] = '1' if present else '0'
        for index in range(self.platform_inventory['num_components']):
            component_obj = component.Component(index)
            self._component_list.append(component_obj)

    @staticmethod
    def _getstatusoutput(cmd):
        status = 0
        ret, data = subprocess.getstatusoutput(cmd)
        if ret != 0:
            status = ret
        if data[-1:] == '\n':
            data = data[:-1]
        
        return status, data

    @staticmethod
    def initizalize_system_led():
        return True

    def get_status_led(self):
        return self.get_system_led("SYS_LED")

    def set_status_led(self, color):
        if color == self.get_status_led():
            return False

        if BMC_EXIST:
            sys_led_color_map = {
                'off': '00',
                'green': '01',
                'amber': '02',
                'amber_blink_1hz': '03',
                'amber_blink_4hz': '04',
                'green_blink_1hz': '05',
                'green_blink_4hz': '06',
                'alternate_blink_1hz': '07',
                'alternate_blink_4hz': '08'
            }
            color_val = sys_led_color_map.get(color.lower(), None)
            if color_val is None:
                print("SYS LED color %s not support!" % color)
                return False

            status, _ = self.helper.ipmi_raw(SET_SYS_STATUS_LED.format(color_val))

            return status
        else:
            result = self.set_system_led("SYS_LED", color)
            return result

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
        reboot_cause = self.helper.read_txt_file(REBOOT_CAUSE_PATH) or "Unknown"

        # This tmp copy is to retain the reboot-cause only for the current boot
        if os.path.isfile(ORG_HW_REBOOT_CAUSE_FILE):
            shutil.move(ORG_HW_REBOOT_CAUSE_FILE, TMP_HW_REBOOT_CAUSE_FILE)

        if reboot_cause == "0x33" and os.path.isfile(TMP_HW_REBOOT_CAUSE_FILE):
            with open(TMP_HW_REBOOT_CAUSE_FILE) as hw_cause_file:
                reboot_info = hw_cause_file.readline().rstrip('\n')
                match = re.search(r'Reason:(.*),Time:(.*)', reboot_info)
                if match is not None:
                    if match.group(1) == 'system':
                        return (self.REBOOT_CAUSE_NON_HARDWARE, 'System cold reboot')

        reboot_cause_description = {
            '0x11': (self.REBOOT_CAUSE_POWER_LOSS, "Power on Reset"),
            '0x22': (self.REBOOT_CAUSE_NON_HARDWARE, "Soft-set CPU warm reset"),
            '0x33': (self.REBOOT_CAUSE_HARDWARE_OTHER, "CPU cold reset"),
            '0x44': (self.REBOOT_CAUSE_NON_HARDWARE, "CPU warm reset"),
            '0x66': (self.REBOOT_CAUSE_WATCHDOG, "Hardware Watchdog Reset"),

        }
        prev_reboot_cause = reboot_cause_description.get(reboot_cause,
                                                         (self.REBOOT_CAUSE_NON_HARDWARE, "Unknown reason"))
        return prev_reboot_cause

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
        sfp_dict = {}

        sfp_removed = '0'
        sfp_inserted = '1'

        sfp_present = True
        sfp_absent = False

        start_time = time.time()
        time_period = timeout / float(1000)  # Convert msecs to secss

        while time.time() < (start_time + time_period) or timeout == 0:
            for port_idx in range(1, self.platform_inventory['num_ports'] + 1):
                if self.sfp_status_dict[port_idx] == sfp_removed and \
                        self.get_sfp(port_idx).get_presence() == sfp_present:
                    sfp_dict[port_idx] = sfp_inserted
                    self.sfp_status_dict[port_idx] = sfp_inserted
                elif self.sfp_status_dict[port_idx] == sfp_inserted and \
                        self.get_sfp(port_idx).get_presence() == sfp_absent:
                    sfp_dict[port_idx] = sfp_removed
                    self.sfp_status_dict[port_idx] = sfp_removed

            if sfp_dict:
                return True, {'sfp': sfp_dict}

            time.sleep(0.5)

        return True, {'sfp': {}}  # Timeout

    def get_revision(self):
        """
        Retrieves the hardware revision for the chassis
        Returns:
            A string containing the hardware revision for this chassis.
        """
        return self._eeprom.get_revision()

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
