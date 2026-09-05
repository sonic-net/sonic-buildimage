#!/usr/bin/env python
#
# Name: chassis.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs 
#

try:
    import os
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.fan import Fan
    from sonic_platform.psu import Psu
    from sonic_platform.sfp import Sfp
    from sonic_platform.qsfp import QSfp
    from sonic_platform.thermal import Thermal
    from sonic_platform.transceiver_event import TransceiverEvent
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Chassis(ChassisBase):

    def __init__(self):
        ChassisBase.__init__(self)
        self.__num_of_fans     = 8
        self.__num_of_psus     = 2
        self.__num_of_sfps     = 56
        self.__start_of_qsfp   = 48
        self.__num_of_thermals = 5

        # Initialize EEPROM
        self._eeprom = Eeprom()

        # Initialize FAN
        for index in range(1, self.__num_of_fans + 1):
            fan = Fan(index, False, 0)
            self._fan_list.append(fan)

        # Initialize PSU
        for index in range(1, self.__num_of_psus + 1):
            psu = Psu(index)
            self._psu_list.append(psu)

        # Initialize SFP
        for index in range(0, self.__num_of_sfps):
            if index < self.__start_of_qsfp:
                sfp = Sfp(index)
            else:
                sfp = QSfp(index)
            self._sfp_list.append(sfp)

        # Initialize THERMAL
        for index in range(0, self.__num_of_thermals):
            thermal = Thermal(index)
            self._thermal_list.append(thermal)

        # Initialize TRANSCEIVER EVENT MONITOR
        self.__xcvr_event = TransceiverEvent()

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
        return self._eeprom.base_mac_address()

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
        raise NotImplementedError

##############################################
# Other methods
##############################################

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

        rc, xcvr_event = self.__xcvr_event.get_transceiver_change_event(timeout) 

        return rc, {'sfp': xcvr_event}

