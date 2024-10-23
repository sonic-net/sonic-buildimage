# {C} Copyright 2023 AMD Systems Inc. All rights reserved
#############################################################################
# Pensando
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Chassis information which are available in the platform
#
#############################################################################

try:
    import os
    import threading
    import time
    import sys
    import subprocess
    from sonic_platform_base.chassis_base import ChassisBase
    from .helper import APIHelper
    import syslog
    import inspect
    from sonic_py_common import syslogger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

PORT_START = 1
PORT_END = 2
NUM_THERMAL = 2
NUM_VOLTAGE_SENSORS = 3
NUM_CURRENT_SENSORS = 3
HOST_REBOOT_CAUSE_PATH = "/host/reboot-cause/"
REBOOT_CAUSE_FILE = "reboot-cause.txt"
HOST_CHK_CMD = "docker > /dev/null 2>&1"
REBOOT_CAUSE_SOFTWARE = "Software causes"
REBOOT_CAUSE_EXTERNAL = "External causes"
RESET_CAUSE_PATH = "/sys/firmware/pensando/rstcause/this_cause"

#cpld masks for system led
SYSTEM_LED_GREEN   = 0x7
SYSTEM_LED_YELLOW  = 0x38
SYSTEM_LED_REG     = 0x15

SYSLOG_IDENTIFIER = "sonic_platform.chassis"
logger_instance = syslogger.SysLogger(SYSLOG_IDENTIFIER)

def log_info(msg, also_print_to_console=False):
    logger_instance.log_info(msg, also_print_to_console)

REBOOT_CAUSE_MAP = {
    25 : "ASIC warm reset",
    24 : "eSecure enabled",
    23 : "AC power cycle/CPLD reload",
    20 : "Host power cycle",
    16 : "Software power cycle",
    19 : "Therm trip",
    18 : "VRD fault",
    12 : "Runtime watchdog reset",
    8 : "Pcie reset",
    2 : "Pcie reset",
    3 : "NCSID power cycle",
    0 : "Reboot",
    1 : "Kernel Panic",
    15 : "Hardware watchdog reset",
}


class Chassis(ChassisBase):
    """Platform-specific Chassis class"""
    __shared_state = dict()
    _sfp_module_initialized = False
    _thermals_initialized = False
    _voltage_sensor_initialized = False
    _current_sensor_initialized = False

    def __init__(self):
        self.__dict__ = self.__shared_state
        self._api_helper = APIHelper()
        ChassisBase.__init__(self)

        if not self._api_helper.is_host():
            self._api_helper.setup_cpldapp()

        self.interrupt_handler = None
        self.event_handler = None

        log_info("System chassis is ready")

    ##############################################
    # System LED methods
    ##############################################

    def initizalize_system_led(self):
        '''Handled by sysmond in dpu container'''
        return True

    def get_status_led(self):
        cmd = "cpldapp -r {}".format(str(SYSTEM_LED_REG))
        try:
            output = self._api_helper.runCMD(cmd)
            reg_val = int(output, 16)
            if (reg_val & SYSTEM_LED_GREEN):
                return self.STATUS_LED_COLOR_GREEN
            elif (reg_val & SYSTEM_LED_YELLOW):
                return self.STATUS_LED_COLOR_AMBER
            else:
                return self.STATUS_LED_COLOR_OFF
        except:
            return self.STATUS_LED_COLOR_OFF

    def set_status_led(self, color):
        '''handled by sysmond in dpu container'''
        return False


    ##############################################
    # SFP methods
    ##############################################

    def get_num_sfps(self):
        if not self._sfp_module_initialized:
            self.__initialize_sfp()
        return len(self._sfp_list)

    def get_all_sfps(self):
        if not self._sfp_module_initialized:
            self.__initialize_sfp()
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
        sfp = None
        if not self._sfp_module_initialized:
            self.__initialize_sfp()

        try:
            # The index will start from 1
            if index <= PORT_END:
                sfp = self._sfp_list[index-1]
            else:
                from sonic_platform.sfp import NullSfp
                sfp = NullSfp()
        except IndexError:
            sys.stderr.write("SFP index {} out of range (1-{})\n".format(
                             index, len(self._sfp_list)))
        return sfp

    def __initialize_sfp(self):
        from sonic_platform.sfp import Sfp
        if self._api_helper.check_xcvrs_present():
            _sfp_name_map = self._api_helper.read_port_config()
            for index in range(PORT_START, PORT_END+1):
                sfp = Sfp(index-1, _sfp_name_map[index-1])
                self._sfp_list.append(sfp)
            self._sfp_module_initialized = True
            log_info("System sfps are initialized")


    ##############################################
    # THERMAL methods
    ##############################################

    def get_num_thermals(self):
        if not self._thermals_initialized:
            self.__initialize_thermals()
        return len(self._thermal_list)

    def get_all_thermals(self):
        if not self._thermals_initialized:
            self.__initialize_thermals()
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
        thermal = None

        try:
            if not self._thermals_initialized:
                self.__initialize_thermals()
            thermal = self._thermal_list[index]
        except IndexError:
            sys.stderr.write("THERMAL index {} out of range (0-{})\n".format(
                             index, len(self._thermal_list)-1))
        return thermal

    def __initialize_thermals(self):
        from sonic_platform.thermal import Thermal
        global NUM_THERMAL
        board_id = self._api_helper.get_board_id()
        if board_id == 130:
            NUM_THERMAL = 3
        if Thermal._thermals_available():
            for index in range(0, NUM_THERMAL):
                thermal = Thermal(index)
                self._thermal_list.append(thermal)
            self._thermals_initialized = True
            log_info("System thermal sensors are initialized")

    ##############################################
    # Voltage Sensor Methods
    ##############################################

    def get_num_voltage_sensors(self):
        if not self._voltage_sensor_initialized:
            self.__initialize_voltage_sensors()
        return len(self._voltage_sensor_list)

    def get_all_voltage_sensors(self):
        if not self._voltage_sensor_initialized:
            self.__initialize_voltage_sensors()
        return self._voltage_sensor_list

    def get_voltage_sensor(self, index):
        if not self._voltage_sensor_initialized:
            self.__initialize_voltage_sensors()
        try:
            voltage_sensor = self._voltage_sensor_list[index]
        except IndexError:
            sys.stderr.write("Voltage sensor index {} out of range (0-{})\n".format(
                             index, len(self._voltage_sensor_list)-1))
        return voltage_sensor

    def __initialize_voltage_sensors(self):
        from sonic_platform.sensor import VoltageSensor
        if VoltageSensor._validate_voltage_sensors():
            for index in range(0, NUM_VOLTAGE_SENSORS):
                voltage = VoltageSensor(index)
                self._voltage_sensor_list.append(voltage)
            self._voltage_sensor_initialized = True
            log_info("system voltage sensors are initialised")

    ##############################################
    # Current Sensor Methods
    ##############################################

    def get_num_current_sensors(self):
        if not self._current_sensor_initialized:
            self.__initialize_current_sensors()
        return len(self._current_sensor_list)

    def get_all_current_sensors(self):
        if not self._current_sensor_initialized:
            self.__initialize_current_sensors()
        return self._current_sensor_list

    def get_current_sensor(self, index):
        if not self._current_sensor_initialized:
            self.__initialize_current_sensors()
        try:
            current_sensor = self._current_sensor_list[index]
        except IndexError:
            sys.stderr.write("Current sensor index {} out of range (0-{})\n".format(
                             index, len(self._current_sensor_list)-1))
        return current_sensor

    def __initialize_current_sensors(self):
        from sonic_platform.sensor import CurrentSensor
        if CurrentSensor._validate_current_sensors():
            for index in range(0, NUM_CURRENT_SENSORS):
                current = CurrentSensor(index)
                self._current_sensor_list.append(current)
            self._current_sensor_initialized = True
            log_info("system current sensors are initialised")

    ##############################################
    # Component methods
    ##############################################

    def get_all_components(self):
        try:
            if self._api_helper.is_host():
                if len(self._component_list) == 0:
                    self.__initialize_components()
                return self._component_list
        except IndexError:
            sys.stderr.write("Failed to initialize components\n")
        return []

    def get_component(self, index):
        """
        Retrieves component represented by (0-based) index <index>
        Args:
            index: An integer, the index (0-based) of the component to retrieve
        Returns:
            An object dervied from ComponentBase representing the specified component
        """
        component = None

        try:
            if self._api_helper.is_host():
                if len(self._component_list) == 0:
                    self.__initialize_components()
                component = self._component_list[index]
        except IndexError:
            sys.stderr.write("Component index {} out of range (0-{})\n".format(
                             index, len(self._component_list)-1))

        return

    def __initialize_components(self):
        from sonic_platform.component import Component
        component_data = Component.populate_component_data()
        num_component = len(component_data.keys())
        for index in range(0, num_component):
            component = Component(component_index = index)
            self._component_list.append(component)
        log_info("System components are initialized")

    ##############################################
    # Other methods
    ##############################################

    def get_eeprom(self):
        if not hasattr(self, "_eeprom") or self._eeprom is None:
                self.__initialize_eeprom()
        return self._eeprom

    def __initialize_eeprom(self):
        from sonic_platform.eeprom import Eeprom
        self._eeprom = Eeprom()
        log_info("system eeprom is ready")

    def get_watchdog(self):
        from sonic_platform.watchdog import Watchdog
        watchdog = Watchdog()
        log_info("Watchdog initialized")
        return watchdog

    ##############################################
    # device or chassis methods
    ##############################################

    def get_name(self):
        """
        Retrieves the name of the chassis
        Returns:
            string: The name of the chassis
        """
        model = ""
        hwsku = self._api_helper.get_hwsku()
        try:
            model = self.get_model()
        except:
            return hwsku
        return hwsku + " " + model

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
        return self._eeprom.get_part_number()

    def get_serial(self):
        """
        Retrieves the serial number of the chassis (Service tag)
        Returns:
            string: Serial number of chassis
        """
        if not hasattr(self, "_eeprom") or self._eeprom is None:
            self.__initialize_eeprom()
        return self._eeprom.get_serial_number()

    def get_status(self):
        """
        Retrieves the operational status of the chassis
        Returns:
            bool: A boolean value, True if chassis is operating properly
            False if not
        """
        return True

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis
        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        if not hasattr(self, "_eeprom") or self._eeprom is None:
            self.__initialize_eeprom()
        return self._eeprom.get_base_mac()

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the chassis
        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
        """
        if not hasattr(self, "_eeprom") or self._eeprom is None:
            self.__initialize_eeprom()
        return self._eeprom.system_eeprom_info()

    def get_my_slot(self):
        cmd = "cpldapp -r 0xA"
        try:
            slot_id = self._api_helper.runCMD(cmd)
            return int(slot_id,16)
        except:
            return -1

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
        bits = REBOOT_CAUSE_MAP.keys()
        try:
            reset_cause = int(self._api_helper.readline_txt_file(RESET_CAUSE_PATH),16)
            for bit in bits:
                if((reset_cause >> bit) & 1):
                    if bit <= 15:
                        return (REBOOT_CAUSE_SOFTWARE, REBOOT_CAUSE_MAP[bit])
                    else:
                        return (REBOOT_CAUSE_EXTERNAL, REBOOT_CAUSE_MAP[bit])

            reboot_cause_path = (HOST_REBOOT_CAUSE_PATH + REBOOT_CAUSE_FILE)
            sw_reboot_cause = self._api_helper.readline_txt_file(reboot_cause_path) or "Unknown"
            if "Unknown" in sw_reboot_cause:
                return (self.REBOOT_CAUSE_HARDWARE_OTHER, "NPU side powercycle")
            return (self.REBOOT_CAUSE_NON_HARDWARE, sw_reboot_cause)
        except:
            return (self.REBOOT_CAUSE_NON_HARDWARE, "Unknown")

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

    def get_interrupt_handler(self):
        from sonic_platform.interrupt import InterruptHandler
        self.interrupt_handler = InterruptHandler()
        return self.interrupt_handler

    def get_event_handler(self):
        from sonic_platform.interrupt import EventHandler
        self.event_handler = EventHandler()
        return self.event_handler
