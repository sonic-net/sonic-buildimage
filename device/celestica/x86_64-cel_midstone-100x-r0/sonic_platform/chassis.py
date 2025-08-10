#!/usr/bin/env python

#############################################################################
# Celestica
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Chassis information which are available in the platform
#
#############################################################################

try:
    import sys
    import time
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.helper import APIHelper
    from sonic_platform.fan_drawer import FanDrawer
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

NUM_FAN_TRAY = 5
NUM_FAN = 2
NUM_PSU = 2
NUM_THERMAL = 18
NUM_SFP = 64
NUM_COMPONENT = 11
RESET_REGISTER = "0xA106"
HOST_REBOOT_CAUSE_PATH = "/host/reboot-cause/"
REBOOT_CAUSE_FILE = "reboot-cause.txt"
PREV_REBOOT_CAUSE_FILE = "previous-reboot-cause.txt"
GETREG_PATH = "/sys/devices/platform/sys_cpld/getreg"
HOST_CHK_CMD = "docker > /dev/null 2>&1"


class Chassis(ChassisBase):
    """Platform-specific Chassis class"""

    def __init__(self):
        #self.config_data = {}
        ChassisBase.__init__(self)
        self._api_helper = APIHelper()

        self.port_start = 0
        self.port_end = NUM_SFP - 1
        self.sfp_module_initialized = False
        self.fan_module_initialized = False
        self.__initialize_eeprom()
        self.is_host = self._api_helper.is_host()

        
        self.__initialize_fan()
        self.__initialize_psu()
        self.__initialize_thermals()
        self.__initialize_sfp()
        self.__initialize_components()

        self.modprs_register = self.get_transceiver_status()


    def __initialize_sfp(self):
        from sonic_platform.sfp import Sfp
        for index in range(0, NUM_SFP):
            sfp = Sfp(index)
            self._sfp_list.append(sfp)
        self.sfp_module_initialized = True

    def __initialize_psu(self):
        from sonic_platform.psu import Psu
        for index in range(0, NUM_PSU):
            psu = Psu(index)
            self._psu_list.append(psu)

    def __initialize_fan(self):
        from sonic_platform.fan import Fan
        for i in range(NUM_FAN_TRAY):
            fandrawer = FanDrawer(i)
            self._fan_drawer_list.append(fandrawer)
            self._fan_list.extend(fandrawer._fan_list)
        self.fan_module_initialized = True
    
    def __initialize_thermals(self):
        from sonic_platform.thermal import Thermal
        airflow = self.__get_air_flow()
        for index in range(0, NUM_THERMAL):
            thermal = Thermal(index, airflow)
            self._thermal_list.append(thermal)

    def __initialize_eeprom(self):
        from sonic_platform.eeprom import Tlv
        self._eeprom = Tlv()

    def __initialize_components(self):
        from sonic_platform.component import Component
        for index in range(0, NUM_COMPONENT):
            component = Component(index)
            self._component_list.append(component)

    def __get_air_flow(self):
        air_flow_path = '/usr/share/sonic/device/{}/fan_airflow'.format(self._api_helper.platform) if self.is_host else '/usr/share/sonic/platform/fan_airflow'
        air_flow = self._api_helper.read_one_line_file(air_flow_path)
        return air_flow or 'B2F'

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis
        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._eeprom.get_mac()

    def get_serial_number(self):
        """
        Retrieves the hardware serial number for the chassis
        Returns:
            A string containing the hardware serial number for this chassis.
        """
        return self._eeprom.get_serial()

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the chassis
        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
        """
        return self._eeprom.get_eeprom()

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
        reboot_cause_path = (HOST_REBOOT_CAUSE_PATH + REBOOT_CAUSE_FILE)
        sw_reboot_cause = self._api_helper.read_txt_file(
            reboot_cause_path) or "Unknown"
        hw_reboot_cause = self._api_helper.get_cpld_reg_value(
            GETREG_PATH, RESET_REGISTER)

        prev_reboot_cause = {    
            '0x11': (self.REBOOT_CAUSE_POWER_LOSS, "The last reset is Power on reset"),
            '0x22': (self.REBOOT_CAUSE_HARDWARE_OTHER, "The last reset is soft-set CPU warm reset"),
            '0x33': (self.REBOOT_CAUSE_HARDWARE_OTHER, "The last reset is soft-set CPU cold reset"),
            '0x44': (self.REBOOT_CAUSE_NON_HARDWARE, "The last reset is CPU warm reset"),
            '0x55': (self.REBOOT_CAUSE_NON_HARDWARE, "The last reset is CPU cold reset"),
            '0x66': (self.REBOOT_CAUSE_WATCHDOG, "The last reset is watchdog reset"),
            '0x77': (self.REBOOT_CAUSE_HARDWARE_OTHER, "The last reset is power cycle reset")
            
        }.get(hw_reboot_cause, (self.REBOOT_CAUSE_HARDWARE_OTHER, 'Unknown reason'))

        if sw_reboot_cause != 'Unknown':
            prev_reboot_cause = (
                self.REBOOT_CAUSE_NON_HARDWARE, sw_reboot_cause)

        return prev_reboot_cause

    ##############################################################
    ####################### SFP methods ##########################
    ##############################################################
    def get_num_sfps(self):
        """
        Retrieves the number of sfps available on this chassis
        Returns:
            An integer, the number of sfps available on this chassis
        """
        if not self.sfp_module_initialized:
            self.__initialize_sfp()

        return len(self._sfp_list)

    def get_all_sfps(self):
        """
        Retrieves all sfps available on this chassis
        Returns:
            A list of objects derived from SfpBase representing all sfps
            available on this chassis
        """
        if not self.sfp_module_initialized:
            self.__initialize_sfp()

        return self._sfp_list

    def get_sfp(self, index):
        """
        Retrieves sfp represented by (0-based) index <index>
        Args:
            index: An integer, the index (0-based) of the sfp to retrieve.
            The index should be the sequence of a physical port in a chassis,
            starting from 0.
            For example, 0 for Ethernet0, 1 for Ethernet4 and so on.
        Returns:
            An object dervied from SfpBase representing the specified sfp
        """
        sfp = None
        if not self.sfp_module_initialized:
            self.__initialize_sfp()

        try:
            sfp = self._sfp_list[index]
        except IndexError:
            sys.stderr.write("SFP index {} out of range (0-{})\n".format(
                             index, len(self._sfp_list)))
        return sfp
    ##############################################################
    ####################### Other methods ########################
    ##############################################################

    def get_watchdog(self):
        """
        Retreives hardware watchdog device on this chassis
        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device
        """
        if self._watchdog is None:
            from sonic_platform.watchdog import Watchdog
            self._watchdog = Watchdog()

        return self._watchdog

    ##############################################################
    ###################### Device methods ########################
    ##############################################################

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return self._api_helper.hwsku

    def get_presence(self):
        """
        Retrieves the presence of the Chassis
        Returns:
            bool: True if Chassis is present, False if not
        """

        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device
        Returns:
            string: Model/part number of device
        """
        return self._eeprom.get_pn()

    def get_serial(self):
        """
        Retrieves the serial number of the device
        Returns:
            string: Serial number of device
        """
        return self.get_serial_number()

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return True

    def get_thermal_manager(self):
        from .thermal_manager import ThermalManager
        return ThermalManager
        
    def get_fan_status(self):
        if not self.fan_module_initialized:
            self.__initialize_fan()

        content = 0
        for fan in self._fan_list:
            if fan.get_presence():
                content = content << 1 | 1
            else:
                content = content << 1
        
        return content 
        
    def get_transceiver_status(self):
        if not self.sfp_module_initialized:
            self.__initialize_sfp()

        content = 0
        index = 0
        for sfp in self.get_all_sfps():
            if sfp.get_presence():
                content = content | (1 << index)
            index = index + 1
        return content 
       
    def get_num_components(self):
         
        return len(self._component_list)                

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
            
        start_time = time.time()
        port_dict = {}
        change_dict = {'fan':{}, 'sfp':{}}
        port = self.port_start
        forever = False
        change_event = False

        if timeout == 0:
            forever = True
        elif timeout > 0:
            timeout = timeout / float(1000) # Convert to secs
        else:
            print("get_transceiver_change_event:Invalid timeout value", timeout)
            return False, {}

        end_time = start_time + timeout
        if start_time > end_time:
            print('get_transceiver_change_event:' \
                       'time wrap / invalid timeout value', timeout)

            return False, {} # Time wrap or possibly incorrect timeout

        while timeout >= 0:
            # Check for OIR events and return updated port_dict
            reg_value = self.get_transceiver_status()
            if reg_value != self.modprs_register:
                changed_ports = self.modprs_register ^ reg_value
                while port >= self.port_start and port <= self.port_end:

                    # Mask off the bit corresponding to our port
                    mask = (1 << port)

                    if changed_ports & mask:
                        # ModPrsL is active low
                        if reg_value & mask == 0:
                            port_dict[port] = '0'
                        else:
                            port_dict[port] = '1'

                    port += 1

                # Update reg value
                self.modprs_register = reg_value
                change_dict['sfp'] = port_dict
                change_event = True
            if change_event:
                return True, change_dict

            if forever:
                time.sleep(1)
            else:
                timeout = end_time - time.time()
                if timeout >= 1:
                    time.sleep(1) # We poll at 1 second granularity
                else:
                    if timeout > 0:
                        time.sleep(timeout)
                    return True, change_dict
        print("get_transceiver_change_event: Should not reach here.")
        return False, change_dict


