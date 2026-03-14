#!/usr/bin/env python
#
# Name: chassis.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs
#

try:
    import sys
    import time
    import syslog
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.psu import Psu
    from sonic_platform.sfp import Sfp
    from sonic_platform.fan import Fan
    from sonic_platform.fan_drawer import FanDrawer
    from sonic_platform.thermal import Thermal

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Chassis(ChassisBase):

    def __init__(self):
        ChassisBase.__init__(self)
        self.__num_of_psus     = 2
        self.__num_of_ports    = 56
        self.__num_of_sfps     = 48
        self.__num_of_fan_drawers = 6
        self.__fan_per_drawer = 2
        self.__num_of_thermals = 18
        self.__xcvr_presence = {}

        # Initialize EEPROM
        self._eeprom = Eeprom()

        # Initialize watchdog
        #self._watchdog = Watchdog()

        # Initialize FAN
        fan_index = 1
        for drawer_index in range(1, self.__num_of_fan_drawers + 1):
            drawer_fan_list = []
            for index in range(0, self.__fan_per_drawer):
                fan = Fan(fan_index, False)
                fan_index += 1
                self._fan_list.append(fan)
                drawer_fan_list.append(fan)
            fan_drawer = FanDrawer(drawer_index, drawer_fan_list)
            self._fan_drawer_list.append(fan_drawer)

        # Initialize thermal
        for index in range(1, self.__num_of_thermals + 1):
            thermal = Thermal(index)
            self._thermal_list.append(thermal)

        # Initialize PSU and PSU_FAN
        for index in range(1, self.__num_of_psus + 1):
            psu = Psu(index)
            self._psu_list.append(psu)

        # Initialize SFP
        for index in range(1, self.__num_of_ports + 1):
            if index in range(1, self.__num_of_sfps + 1):
                sfp = Sfp(index, 'SFP')
            else:
                sfp = Sfp(index, 'QSFP')

            self._sfp_list.append(sfp)

        for index in range(1, self.__num_of_ports + 1):
            self.__xcvr_presence[index] = self._sfp_list[index-1].get_presence()

        # Initialize components
        from sonic_platform.component import ComponentBIOS, ComponentBMC, ComponentCPLD, ComponentPCIE
        self._component_list.append(ComponentBIOS())
        self._component_list.append(ComponentBMC())
        self._component_list.extend(ComponentCPLD.get_component_list())
        self._component_list.append(ComponentPCIE())

##############################################
# Device methods
##############################################

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
            if (index == 0):
                raise IndexError
            sfp = self._sfp_list[index-1]
        except IndexError:
            sys.stderr.write("override: SFP index {} out of range (1-{})\n".format(
                             index, len(self._sfp_list)))

        return sfp

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
        return self._eeprom.base_mac_addr()

    def get_serial_number(self):
        """
        Retrieves the hardware serial number for the chassis

        Returns:
            A string containing the hardware serial number for this chassis.
        """
        return self._eeprom.serial_number_str()

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
        return (ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER, "Invalid Reason")

    ##############################################
    # Other methods
    ##############################################
    def get_watchdog(self):
        """
        Retreives hardware watchdog device on this chassis

        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device
        """
        try:
            if self._watchdog is None:
                from sonic_platform.watchdog import Watchdog
                # Create the watchdog Instance
                self._watchdog = Watchdog()

        except Exception as e:
            syslog.syslog(syslog.LOG_ERR, "Fail to load watchdog due to {}".format(e))
        return self._watchdog

    def get_change_event(self, timeout=0):
        """
        Currently only support transceiver change events
        """

        start_ms = time.time() * 1000
        xcvr_change_event_dict = {}
        event = False

        while True:
            time.sleep(0.5)
            for index in range(1, self.__num_of_ports + 1):
                cur_xcvr_presence = self._sfp_list[index-1].get_presence()
                if cur_xcvr_presence != self.__xcvr_presence[index]:
                    if cur_xcvr_presence is True:
                        xcvr_change_event_dict[index] = '1'
                        self.__xcvr_presence[index] = True
                    elif cur_xcvr_presence is False:
                        xcvr_change_event_dict[index] = '0'
                        self.__xcvr_presence[index] = False
                    event = True

            if event is True:
                return True, {'sfp':xcvr_change_event_dict}

            if timeout:
                now_ms = time.time() * 1000
                if (now_ms - start_ms >= timeout):
                    return True, {'sfp':xcvr_change_event_dict}

