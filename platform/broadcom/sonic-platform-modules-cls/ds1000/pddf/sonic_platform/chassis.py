#############################################################################
# PDDF
# Module contains an implementation of SONiC Chassis API
#
#############################################################################
import os
import time
import sys
import subprocess
import re

try:
    from sonic_platform_pddf_base.pddf_chassis import PddfChassis
    from sonic_platform_pddf_base.pddf_eeprom import PddfEeprom
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.thermal import Thermal
    from sonic_platform.watchdog import Watchdog
    from sonic_py_common import device_info
    from sonic_platform_base.sfp_base import SfpBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

NUM_COMPONENTS = 4
NUM_SENSORS = 4
HW_REBOOT_CAUSE_FILE="/host/reboot-cause/hw-reboot-cause.txt"

class Chassis(PddfChassis):
    """
    PDDF Platform-specific Chassis class
    """
    sfp_status_dict = {}

    def __init__(self, pddf_data=None, pddf_plugin_data=None):

        PddfChassis.__init__(self, pddf_data, pddf_plugin_data)
        (self.platform, self.hwsku) = device_info.get_platform_and_hwsku()
        self._watchdog = None
        self._airflow_direction = None

        self.__initialize_components()

        for sfp in self._sfp_list:
            present = sfp.get_presence()
            self.sfp_status_dict[sfp.index] = '1' if present else '0'

        # PDDF doesn't support CPU internal temperature sensor
        # Hence it is created from chassis init override and
        # handled appropriately in thermal APIs
        self._thermal_list.append(Thermal(NUM_SENSORS))

    def __initialize_components(self):
        from sonic_platform.component import Component
        for index in range(0, NUM_COMPONENTS):
            component = Component(index)
            self._component_list.append(component)

    # Provide the functions/variables below for which implementation is to be overwritten

    def initizalize_system_led(self):
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
        hw_reboot_cause = ""
        with open("/sys/devices/platform/cpld_wdt/reason", "r") as f:
            hw_reboot_cause = f.read().strip()

        if hw_reboot_cause == "0x99":
            reboot_cause = self.REBOOT_CAUSE_HARDWARE_OTHER
            description = 'NPU overload reset'
        elif hw_reboot_cause == "0x88":
            reboot_cause = self.REBOOT_CAUSE_HARDWARE_OTHER
            description = 'CPU overload reset'
        elif hw_reboot_cause == "0x77":
            reboot_cause = self.REBOOT_CAUSE_WATCHDOG
            description = 'Hardware Watchdog Reset'
        elif hw_reboot_cause == "0x66":
            reboot_cause = self.REBOOT_CAUSE_HARDWARE_OTHER
            description = 'GPIO Warm Reset'
        elif hw_reboot_cause == "0x55":
            reboot_cause = self.REBOOT_CAUSE_HARDWARE_OTHER
            description = 'CPU Cold Reset'
        elif hw_reboot_cause == "0x44":
            reboot_cause = self.REBOOT_CAUSE_NON_HARDWARE
            description = 'CPU Warm Reset'
        elif hw_reboot_cause == "0x33":
            reboot_cause = self.REBOOT_CAUSE_HARDWARE_OTHER
            description = 'Soft-Set Cold Reset'
        elif hw_reboot_cause == "0x22":
            reboot_cause = self.REBOOT_CAUSE_HARDWARE_OTHER
            description = 'Soft-Set Warm Reset'
        elif hw_reboot_cause == "0x11":
            reboot_cause = self.REBOOT_CAUSE_POWER_LOSS
            description = 'Power Loss'
        elif hw_reboot_cause == "0x00":
            reboot_cause = self.REBOOT_CAUSE_HARDWARE_OTHER
            description = 'Cold Powercycle'
            if os.path.isfile(HW_REBOOT_CAUSE_FILE):
                with open(HW_REBOOT_CAUSE_FILE) as hw_cause_file:
                    reboot_info = hw_cause_file.readline().rstrip('\n')
                    match = re.search(r'Reason:(.*),Time:(.*)', reboot_info)
                    if match is not None:
                        if  match.group(1) == 'temp_fatal':
                            description = 'Fatal temperature trip [Time:{}]'.format(match.group(2))
                        elif match.group(1) == 'temp_critical':
                            description = 'Critical temperature reboot [Time:{}]'.format(match.group(2))
                        elif match.group(1) == 'system':
                            reboot_cause = self.REBOOT_CAUSE_NON_HARDWARE
                            description = 'System cold reboot'
        else:
            reboot_cause = self.REBOOT_CAUSE_NON_HARDWARE
            description = 'Unkown Reason'

        return (reboot_cause, description)

    def get_revision(self):
        version_str = self._eeprom.revision_str()

        if version_str != "NA":
            return str(bytearray(version_str, 'ascii')[0])

        return version_str

    @staticmethod
    def get_position_in_parent():
        return -1

    @staticmethod
    def is_replaceable():
        return False

    def set_status_led(self, color):
        if color == self.get_system_led("SYS_LED"):
            return True

        return self.set_system_led("SYS_LED", color)

    def get_status_led(self):
        return self.get_system_led("SYS_LED")

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
        if index in range(1, 48+1):
            return SfpBase.SFP_PORT_TYPE_BIT_RJ45
        elif index in range(49, 56+1):
            return (SfpBase.SFP_PORT_TYPE_BIT_SFP | SfpBase.SFP_PORT_TYPE_BIT_SFP_PLUS)
        else:
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
                  Specifically for SFP event, besides SFP plug in and plug out,
                  there are some other error event could be raised from SFP, when
                  these error happened, SFP eeprom will not be avalaible, XCVRD shall
                  stop to read eeprom before SFP recovered from error status.
                      status='2' I2C bus stuck,
                      status='3' Bad eeprom,
                      status='4' Unsupported cable,
                      status='5' High Temperature,
                      status='6' Bad cable.
        """

        sfp_dict = {}

        SFP_REMOVED = '0'
        SFP_INSERTED = '1'

        SFP_PRESENT = True
        SFP_ABSENT = False

        start_time = time.time()
        time_period = timeout/float(1000) #Convert msecs to secs

        while time.time() < (start_time + time_period) or timeout == 0:
            for sfp in self._sfp_list:
                port_idx = sfp.index
                if self.sfp_status_dict[port_idx] == SFP_REMOVED and \
                    sfp.get_presence() == SFP_PRESENT:
                    sfp_dict[port_idx] = SFP_INSERTED
                    self.sfp_status_dict[port_idx] = SFP_INSERTED
                elif self.sfp_status_dict[port_idx] == SFP_INSERTED and \
                    sfp.get_presence() == SFP_ABSENT:
                    sfp_dict[port_idx] = SFP_REMOVED
                    self.sfp_status_dict[port_idx] = SFP_REMOVED

            if sfp_dict:
                return True, {'sfp':sfp_dict}

            time.sleep(0.5)

        return True, {'sfp':{}} # Timeout

    def get_airflow_direction(self):
        if self._airflow_direction == None:
            try:
                vendor_extn = self._eeprom.get_vendor_extn()
                airflow_type = vendor_extn.split()[2][2:4] # Either 0xFB or 0xBF
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
