#!/usr/bin/env python
#
# Name: chassis.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs
#

try:
    import re
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.fan_drawer import FanDrawer
    from sonic_platform.psu import Psu
    from sonic_platform.sfp import Sfp
    from sonic_platform.qsfp import QSfp
    from sonic_platform.thermal import Thermal
    from sonic_platform.component import Component
    from sonic_platform.watchdog import Watchdog
    from sonic_platform.event_monitor import EventMonitor
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

PMON_REBOOT_CAUSE_PATH = "/usr/share/sonic/platform/reboot-cause/"
REBOOT_CAUSE_FILE = "reboot-cause.txt"
PREV_REBOOT_CAUSE_FILE = "previous-reboot-cause.txt"
monitor = None


class Chassis(ChassisBase):

    __num_of_fans = 4
    __num_of_psus = 2
    __num_of_sfps = 56
    __start_of_qsfp = 48
    __num_of_thermals = 15
    __num_of_components = 4

    def __init__(self):
        ChassisBase.__init__(self)

        # Initialize EEPROM
        self._eeprom = Eeprom()
        self._eeprom_data = self._eeprom.get_eeprom_data()

        # Initialize FAN
        for index in range(self.__num_of_fans):
            fandrawer = FanDrawer(index)
            self._fan_drawer_list.append(fandrawer)
            self._fan_list.extend(fandrawer._fan_list)

        # Initialize PSU
        for index in range(self.__num_of_psus):
            psu = Psu(index)
            self._psu_list.append(psu)

        # Initialize SFP
        for index in range(self.__num_of_sfps):
            if index < self.__start_of_qsfp:
                sfp = Sfp(index)
            else:
                sfp = QSfp(index)
            self._sfp_list.append(sfp)

        # Initialize THERMAL
        for index in range(self.__num_of_thermals):
            thermal = Thermal(index)
            self._thermal_list.append(thermal)

        # Initialize COMPONENT
        for index in range(self.__num_of_components):
            component = Component(index)
            self._component_list.append(component)

        # Initialize WATCHDOG
        self._watchdog = Watchdog()

    def __read_txt_file(self, file_path):
        try:
            with open(file_path, 'r') as fd:
                data = fd.read()
                return data.strip()
        except IOError:
            pass
        return None


##############################################
# Device methods
##############################################

    def get_name(self):
        """
        Retrieves the name of the chassis
        Returns:
            string: The name of the chassis
        """
        return self._eeprom.modelstr()

    def get_presence(self):
        """
        Retrieves the presence of the chassis
        Returns:
            bool: True if chassis is present, False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the chassis
        Returns:
            string: Model/part number of chassis
        """
        return self._eeprom.part_number_str()

    def get_serial(self):
        """
        Retrieves the serial number of the chassis
        Returns:
            string: Serial number of chassis
        """
        return self._eeprom.serial_number_str()

    def get_status(self):
        """
        Retrieves the operational status of the chassis
        Returns:
            bool: A boolean value, True if chassis is operating properly
            False if not
        """
        return True

##############################################
# Chassis methods
##############################################

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis

        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._eeprom.base_mac_addr(self._eeprom_data)

    def get_serial_number(self):
        """
        Retrieves the hardware serial number for the chassis

        Returns:
            A string containing the hardware serial number for this chassis.
        """
        return self._eeprom.serial_number_str(self._eeprom_data)

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the chassis

        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
            Ex. { '0x21':'AG9064', '0x22':'V1.0', '0x23':'AG9064-0109867821',
                  '0x24':'001c0f000fcd0a', '0x25':'02/03/2018 16:22:00',
                  '0x26':'01', '0x27':'REV01', '0x28':'AG9064-C2358-16G'}
        """
        return self._eeprom.system_eeprom_info()

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
        description = 'None'
        reboot_cause = self.REBOOT_CAUSE_HARDWARE_OTHER

        reboot_cause_path = PMON_REBOOT_CAUSE_PATH + REBOOT_CAUSE_FILE
        prev_reboot_cause_path = PMON_REBOOT_CAUSE_PATH + PREV_REBOOT_CAUSE_FILE

        sw_reboot_cause = self.__read_txt_file(reboot_cause_path) or "Unknown"
        prev_sw_reboot_cause = self.__read_txt_file(
            prev_reboot_cause_path) or "Unknown"

        if sw_reboot_cause == "Unknown" and prev_sw_reboot_cause in ("Unknown", self.REBOOT_CAUSE_POWER_LOSS):
            reboot_cause = self.REBOOT_CAUSE_POWER_LOSS
            description = prev_sw_reboot_cause
        elif sw_reboot_cause != "Unknown":
            reboot_cause = self.REBOOT_CAUSE_NON_HARDWARE
            description = sw_reboot_cause
        elif prev_reboot_cause_path != "Unknown":
            reboot_cause = self.REBOOT_CAUSE_NON_HARDWARE
            description = prev_sw_reboot_cause

        return (reboot_cause, description)

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
        global monitor
        port_dict = {}
        while True:
            with EventMonitor(timeout) as monitor:
                while True:
                    event = monitor.get_events()

                    if not bool(event):
                        return True, {'sfp': port_dict}
                    else:
                        if event['SUBSYSTEM'] == 'swps':
                            portname = event['DEVPATH'].split("/")[-1]
                            rc = re.match(r"port(?P<num>\d+)", portname)
                            if rc is not None:
                                if event['ACTION'] == "remove":
                                    remove_num = int(rc.group("num"))
                                    port_dict[remove_num] = "0"
                                elif event['ACTION'] == "add":
                                    add_num = int(rc.group("num"))
                                    port_dict[add_num] = "1"
                                return True, {'sfp': port_dict}
                            return False, {'sfp': port_dict}
                        else:
                            pass
