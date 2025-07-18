#!/usr/bin/env python
#
# Clounix
# Name: chassis.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs
#

try:
    import os
    import sys
    import time
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.fan import Fan
    from sonic_platform.fan_drawer import FanDrawer
    from sonic_platform.psu import Psu
    from sonic_platform.qsfp import QSfp
    from sonic_platform.thermal import Thermal
    from sonic_platform.component import Component
    from sonic_platform.watchdog import Watchdog
    from .helper import APIHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SFP_STATUS_INSERTED = '1'
SFP_STATUS_REMOVED = '0'

# FAN status definition, shall be aligned with the definition in get_fan_change_event() of ChassisBase
FAN_STATUS_REMOVED = '0'
FAN_STATUS_INSERTED = '1'

# PSU status definition, shall be aligned with the definition in get_change_event() of ChassisBase
PSU_STATUS_REMOVED = '0'
PSU_STATUS_INSERTED = '1'

REBOOT_CAUSE_FILE = "/host/reboot-cause/reboot-cause.txt"
THERMAL_OVERLOAD_POSITION_FILE = "/host/reboot-cause/platform/thermal_overload_position"
ADDITIONAL_FAULT_CAUSE_FILE = "/host/reboot-cause/platform/additional_fault_cause"

class Chassis(ChassisBase):
    _voltage_list = []

    def __init__(self):
        self.__api_helper = APIHelper()
        chassis_conf = self.__api_helper.get_attr_conf("chassis")
        self.__conf = chassis_conf
        
        self.__num_of_psus     = len(chassis_conf['psus'])
        self.__num_of_sfps     = len(chassis_conf['sfps'])
        self.__num_of_thermals = len(chassis_conf['thermals'])
        self.__num_of_components = len(chassis_conf['components'])
        self.__num_of_fans = len(chassis_conf['fans'])
        self.__num_of_fan_drawers = len(chassis_conf['fan_drawers'])

        self.__start_of_qsfp   = 0
        self.__attr_led_path_prefix = '/sys/switch/sysled/'
        ChassisBase.__init__(self)
        self.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU = "Thermal Overload:CPU"
        self.REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC = "Thermal Overload:ASIC"
        self.REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER = "Thermal Overload:Other"
        self.REBOOT_CAUSE_WATCHDOG = "Watchdog"
        self.REBOOT_CAUSE_COLD_RESET = "CPU Cold Reset"
        self.REBOOT_CAUSE_WARM_RESET = "CPU Warm Reset"
        self.REBOOT_CAUSE_PSU_SHUTDOWN = "PSU Shutdown"
 
        # Initialize EEPROM
        self._eeprom = Eeprom()
        
        # Initialize CHASSIS FAN
        fan_conf=self.__conf['fans']
        for x in range(0, self.__num_of_fans):
            fan_conf[x].update({'container':'chassis'})
            fan_conf[x].update({'container_index': self.__index})
            fan = Fan(x, fan_conf)
            self._fan_list.append(fan)

        # Initialize CHASSIS FAN DRWAER
        for drawer_index in range(0, self.__num_of_fan_drawers):
            fandrawer_conf=chassis_conf['fan_drawers']
            fan_drawer = FanDrawer(drawer_index, fandrawer_conf)
            self._fan_drawer_list.append(fan_drawer)


        # Initialize PSU
        for index in range(0, self.__num_of_psus):
            psu_conf=chassis_conf['psus']
            psu = Psu(index, psu_conf)
            self._psu_list.append(psu)

        # Initialize SFP
        for index in range(0, self.__num_of_sfps):
            qsfp_conf=chassis_conf['sfps']
            sfp = QSfp(index, qsfp_conf)
            self._sfp_list.append(sfp)

        # Initialize CHASSIS THERMAL
        thermal_conf=self.__conf['thermals']
        for x in range(0, self.__num_of_thermals):
            thermal_conf[x].update({'container':'chassis'})
            thermal_conf[x].update({'container_index':0})
            thermal = Thermal(x, thermal_conf)
            self._thermal_list.append(thermal)

        # Initialize COMPONENT
        for index in range(0, self.__num_of_components):
            component_conf=chassis_conf['components']
            component = Component(index, component_conf)
            self._component_list.append(component)

        # Initialize WATCHDOG
        watchdog_conf=chassis_conf['watchdog']
        self._watchdog = Watchdog(watchdog_conf)

##############################################
# Device methods
##############################################
    def get_thermal_manager(self):
        from .thermal_manager import ThermalManager
        return ThermalManager

    def get_name(self):
        """
        Retrieves the name of the chassis
        Returns:
            string: The name of the chassis
        """
        return self.__conf['name']

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

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether Chassis is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False   

    def set_status_led(self, color):
        """
        Sets the state of the fan module status LED

        Args:
            color: A string representing the color with which to set the
                   fan module status LED

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        ret_val = False
        if color == "green":
            led_value = 1
        elif color == "red":
            led_value = 3
        elif color == "blink_green":
            led_value = 4
        else:
            return False
        ret_val = self.__api_helper.write_txt_file(self.__attr_led_path_prefix + 'sys_led_status',led_value)
        
        return ret_val

    def get_status_led(self):
        """
        Gets the state of the fan status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings above
        """
        color = "off"

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_led_path_prefix + 'sys_led_status')
        if (attr_rv != None):
            if (int(attr_rv, 16) == 0x1) :
                color = "green"
            elif (int(attr_rv, 16) == 0x4):
                color = "blink_green"
            elif (int(attr_rv, 16) == 0x0):
                color = "off"
            else:
                color = "red"

        return color

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

        try:
            # The index will start from 0
            sfp = self._sfp_list[index]
        except IndexError:
            sys.stderr.write("SFP index {} out of range (0-{})\n".format(
                             index, len(self._sfp_list)))
        return sfp

    def get_eeprom(self):
        """
        Retreives eeprom device on this chassis

        Returns:
            An object derived from WatchdogBase representing the hardware
            eeprom device
        """
        return self._eeprom

    def get_all_voltages(self):
        """
        Retrieves all voltages available on this chassis

        Returns:
            A list of objects derived from VoltageBase representing all voltages
            available on this chassis
        """
        return self._voltage_list

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis

        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._eeprom.base_mac_address()

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
        reboot_cause = self.REBOOT_CAUSE_NON_HARDWARE
        thermal_overload_pos = 'None'
        addational_fault_cause = 'None'
        hw_reboot_cause = 'Unknown'

        #reservered for hardware reboot cause
        if hw_reboot_cause != 'Unknown':
            reboot_cause = self.REBOOT_CAUSE_HARDWARE_OTHER

        #software reboot cause
        sw_reboot_cause = self.__api_helper.read_one_line_file(REBOOT_CAUSE_FILE) or "Unknown"
        description = sw_reboot_cause

        #check power down reboot-cause
        REBOOT_CAUSE_PWR_DOWN_PATH='/sys/devices/pci0000:00/0000:00:1f.3/i2c-0/i2c-1/i2c-4/4-0070/hwmon/hwmon4/pwr_down'
        REBOOT_CAUSE_CPLD_PATH='/sys/devices/pci0000:00/0000:00:1f.3/i2c-0/i2c-1/1-0018/reboot_cause'
        reboot_cause_pwr_down = 0
        reboot_cause_cpld = 0
        if os.path.isfile(REBOOT_CAUSE_PWR_DOWN_PATH):
            attr_rv = self.__api_helper.read_one_line_file(REBOOT_CAUSE_PWR_DOWN_PATH)
            reboot_cause_pwr_down = int(attr_rv, 16)

        #check watchdog/cold reboot
        if os.path.isfile(REBOOT_CAUSE_CPLD_PATH):
            attr_rv = self.__api_helper.read_one_line_file(REBOOT_CAUSE_CPLD_PATH)
            reboot_cause_cpld = int(attr_rv, 16)

        #thermal policy reboot cause
        if os.path.isfile(THERMAL_OVERLOAD_POSITION_FILE):
            thermal_overload_pos = self.__api_helper.read_one_line_file(THERMAL_OVERLOAD_POSITION_FILE)

        if os.path.isfile(ADDITIONAL_FAULT_CAUSE_FILE):
            addational_fault_cause = self.__api_helper.read_one_line_file(ADDITIONAL_FAULT_CAUSE_FILE)

        if (reboot_cause_pwr_down & 0x1):
            reboot_cause = self.REBOOT_CAUSE_PSU_SHUTDOWN
        elif (reboot_cause_cpld & 0x2):
            reboot_cause = self.REBOOT_CAUSE_WATCHDOG
        elif (reboot_cause_cpld & 0x4):
            reboot_cause = self.REBOOT_CAUSE_COLD_RESET
        elif (reboot_cause_cpld & 0x8):
            reboot_cause = self.REBOOT_CAUSE_WARM_RESET
        elif "CPU" in thermal_overload_pos:
            reboot_cause = self.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU
            description = thermal_overload_pos
            os.remove(THERMAL_OVERLOAD_POSITION_FILE)
        elif "NPU" in thermal_overload_pos:
            reboot_cause = self.REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC
            description = thermal_overload_pos
            os.remove(THERMAL_OVERLOAD_POSITION_FILE)
        elif thermal_overload_pos is not 'None':
            reboot_cause = self.REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER
            description = thermal_overload_pos
            os.remove(THERMAL_OVERLOAD_POSITION_FILE)

        if addational_fault_cause is not 'None':
            description = ' '.join([description, addational_fault_cause])
            os.remove(ADDITIONAL_FAULT_CAUSE_FILE)

        prev_reboot_cause = (reboot_cause, description)
        return prev_reboot_cause



    @property
    def _get_presence_bitmap(self):
        return self._sfp_list[1].get_all_presence()

    @property
    def _get_fan_status_bit_map(self):
        bit_map = 0
        for index in range(0,self.get_num_fan_drawers()):
            bit_map |= int(self.get_fan_drawer(index).get_fan(0).get_presence()) << index 
        return bit_map
    @property
    def _get_psu_status_bit_map(self):
        bit_map = 0
        for index in range(0,self.get_num_psus()):
            bit_map |= int(self.get_psu(index).get_presence()) << index
        return bit_map


    data = {'sfp_present':0,'fan_status':0,'psu_status':0}
    def get_transceiver_change_event(self, timeout=1000):
        port_dict = {}

        if timeout == 0:
            cd_ms = sys.maxsize
        else:
            cd_ms = timeout

        #poll per second
        while cd_ms > 0:
            reg_value = self._get_presence_bitmap
            changed_ports = self.data['present'] ^ reg_value
            if changed_ports != 0:
                break
            time.sleep(1)
            cd_ms = cd_ms - 1000

        if changed_ports != 0:
            for port in range(0, self.__num_of_sfps):
                # Mask off the bit corresponding to our port
                mask = (1 << (port - 0))
                if changed_ports & mask:
                    if (reg_value & mask) == 0:
                        port_dict[port] = SFP_STATUS_REMOVED
                    else:
                        port_dict[port] = SFP_STATUS_INSERTED

            # Update cache
            self.data['present'] = reg_value
            return True, port_dict
        else:
            return True, {}

    def get_fan_change_event(self, timeout=1000):
        fan_dict = {}

        if timeout == 0:
            cd_ms = sys.maxsize
        else:
            cd_ms = timeout

        #poll per second
        while cd_ms > 0:
            reg_value = self._get_fan_status_bit_map
            changed_fans = self.data['fan_status'] ^ reg_value
            if changed_fans != 0:
                break
            time.sleep(1)
            cd_ms = cd_ms - 1000

        if changed_fans != 0:
            for fan in range(0,self.get_num_fan_drawers()):
                # Mask off the bit corresponding to our fan
                mask = (1 << (fan - 0))
                if changed_fans & mask:
                    if (reg_value & mask) == 0:
                        fan_dict[fan] = PSU_STATUS_REMOVED
                    else:
                        fan_dict[fan] = PSU_STATUS_INSERTED
            # Update cache
            self.data['fan_status'] = reg_value
            return {True: fan_dict}
        else:
            return {False: {}}

    def get_psu_change_event(self, timeout=1000):
        psu_dict = {}

        if timeout == 0:
            cd_ms = sys.maxsize
        else:
            cd_ms = timeout

        #poll per second
        while cd_ms > 0:
            reg_value = self._get_psu_status_bit_map
            changed_psus = self.data['psu_status'] ^ reg_value
            if changed_psus != 0:
                break
            time.sleep(1)
            cd_ms = cd_ms - 1000

        if changed_psus != 0:
            for psu in range(0, self.get_num_psus()):
                # Mask off the bit corresponding to our psu
                mask = (1 << (psu - 0))
                if changed_psus & mask:
                    if (reg_value & mask) == 0:
                        psu_dict[psu] = FAN_STATUS_REMOVED
                    else:
                        psu_dict[psu] = FAN_STATUS_INSERTED
            # Update cache
            self.data['psu_status'] = reg_value
            return {True: psu_dict}
        else:
            return {False:{}}

    def get_change_event(self, timeout=0):
        res_dict = {
            'component': {},
            'fan': {},
            'module': {},
            'psu': {},
            'sfp': {},
            'thermal': {},
        }
        ''' get transceiver change event '''
        res_dict['sfp'].clear()
        status, res_dict['sfp'] = self.get_transceiver_change_event(timeout)
        return status, res_dict
