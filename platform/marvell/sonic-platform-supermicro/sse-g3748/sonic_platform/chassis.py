#!/usr/bin/env python
##################################################################
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
##################################################################

try:
    import os
    import sys
    import glob
    import time
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.sfp import Sfp
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.fan import Fan
    from .fan_drawer import RealDrawer
    from sonic_platform.psu import Psu
    from sonic_platform.thermal import Thermal
    from sonic_py_common import logger
    from sonic_platform.component import Component
    from sonic_platform.watchdog import WatchdogImplBase

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

smbus_present = 1
try:
    import smbus
except ImportError as e:
    smbus_present = 0

if sys.version_info[0] < 3:
    import commands as cmd
else:
    import subprocess as cmd


MAX_SELECT_DELAY = 3600
COPPER_PORT_START = 1
COPPER_PORT_END = 48
SFP_PORT_START =49 
SFP_PORT_END = 54 
PORT_END = 54
MAX_COMPONENT=3

# Device counts
MAX_G3748_FAN_DRAWERS = 1
MAX_G3748_FANS_PER_DRAWER = 2
MAX_G3748_PSU = 2
MAX_G3748_THERMAL = 2

SYSLOG_IDENTIFIER = "chassis"
sonic_logger=logger.Logger(SYSLOG_IDENTIFIER)

class Chassis(ChassisBase):
    """
    Platform-specific Chassis class
    derived from Dell S6000 platform.
    customized for the platform.
    """
    reset_reason_dict = {}
    reset_reason_dict[0x02] = ChassisBase.REBOOT_CAUSE_POWER_LOSS
    reset_reason_dict[0x20] = ChassisBase.REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC

    reset_reason_dict[0x08] = ChassisBase.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU
    reset_reason_dict[0x10] = ChassisBase.REBOOT_CAUSE_WATCHDOG
    PLATFORM_ROOT_PATH = "/usr/share/sonic/device"
    PMON_HWSKU_PATH = "/usr/share/sonic/hwsku"
    PLATFORM = "arm64-supermicro_sse_g3748-r0"
    HWSKU = "supermicro_sse_g3748"

    I2C_CLASS_DIR = "/sys/class/i2c-adapter/"
    I2C_DEV_MAPPING = (['i2c-0/0-0066/hwmon/', 1])

    def __init__(self):
        ChassisBase.__init__(self)
        self.system_led_supported_color = ['off', 'green', 'amber', 'blue', 'blinking green', 'blinking amber', 'blinking blue']
        # Port numbers for Initialize SFP list
        self.COPPER_PORT_START = COPPER_PORT_START
        self.COPPER_PORT_END = COPPER_PORT_END
        self.SFP_PORT_START = SFP_PORT_START
        self.SFP_PORT_END = SFP_PORT_END
        self.PORT_END = PORT_END

        G3748CPLD_I2C_DIR = "/sys/bus/i2c/devices/0-0066/"

        # for non-sfp ports create dummy objects for copper / non-sfp ports
        for index in range(self.COPPER_PORT_START, self.COPPER_PORT_END+1):
            sfp_node = Sfp(index, 'RJ45', 'N/A', 'N/A')
            self._sfp_list.append(sfp_node)

        # Verify optoe2 driver SFP eeprom devices were enumerated and exist
        # then create the sfp nodes
        eeprom_path = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/eeprom"
        mux_dev = sorted(glob.glob("/sys/class/i2c-adapter/i2c-[2-7]"))
        y = 0
        for index in range(self.SFP_PORT_START, self.SFP_PORT_END+1):
            mux_dev_num = mux_dev[y]
            port_i2c_map = mux_dev_num[-1]
            y = y + 1
            port_eeprom_path = eeprom_path.format(port_i2c_map)
            if not os.path.exists(port_eeprom_path):
                sonic_logger.log_info("path %s didnt exist" % port_eeprom_path)
            sfp_node = Sfp(index, 'SFP', port_eeprom_path, port_i2c_map)
            self._sfp_list.append(sfp_node)
        self.sfp_event_initialized = False

        # Instantiate ONIE system eeprom object
        self._eeprom = Eeprom()

        # Construct lists fans, power supplies, thermals & components
        drawer_num = MAX_G3748_FAN_DRAWERS
        fan_num_per_drawer = MAX_G3748_FANS_PER_DRAWER
        drawer_ctor = RealDrawer
        fan_index = 0
        for drawer_index in range(drawer_num):
            drawer = drawer_ctor(drawer_index)
            self._fan_drawer_list.append(drawer)
            for index in range(fan_num_per_drawer):
                fan = Fan(fan_index, drawer)
                fan_index += 1
                drawer._fan_list.append(fan)
                self._fan_list.append(fan)

        for i in range(MAX_G3748_PSU):
            psu = Psu(i)
            self._psu_list.append(psu)

        for i in range(MAX_G3748_THERMAL):
            thermal = Thermal(i)
            self._thermal_list.append(thermal)

        for i in range(MAX_COMPONENT):
             component = Component(i)
             self._component_list.append(component)

        self.fan_sys_frontled_reg = G3748CPLD_I2C_DIR+"fan_sys_frontled".format()

    def _get_i2c_register(self, reg_file):
        # On successful read, returns the value read from given
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(reg_file)):
            return rv

        try:
            with open(reg_file, 'r') as fd:
                rv = fd.read()
        except Exception as e:
            rv = 'ERR'

        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv

    def _set_i2c_register(self, reg_file, value):
        # On successful write, the value read will be written on
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(reg_file)):
            return rv

        try:
            with open(reg_file, 'w') as fd:
                rv = fd.write(str(value))
        except Exception as e:
            rv = 'ERR'

        # Ensure that the write operation has succeeded
        if (int(self._get_i2c_register(reg_file)) != value ):
            time.sleep(3)
            if (int(self._get_i2c_register(reg_file)) != value ):
                rv = 'ERR'

        return rv

    def get_presence(self):
        """
        Retrieves the presence of the chassis
        Returns:
            bool: True if chassis is present, False if not
        """
        return True

    def get_status(self):
        """
        Retrieves the operational status of the chassis
        Returns:
            bool: A boolean value, True if chassis is operating properly
            False if not
        """
        return True

    ##############################################
    # EEPROM methods
    ##############################################

    def get_eeprom(self):
        """
        Retreives eeprom device on this chassis
        Returns:
            An object representing the hardware eeprom device
        """
        return self._eeprom

    def get_name(self):
        """
        Retrieves the name of the chassis
        Returns:
            string: The name of the chassis
        """
        return self._eeprom.modelstr()

    def get_model(self):
        """
        Retrieves the model number (or part number) of the chassis
        Returns:
            string: Model/part number of chassis
        """
        return self._eeprom.part_number_str()

    def get_serial(self):
        """
        Retrieves the serial number of the chassis (Service tag)
        Returns:
            string: Serial number of chassis
        """
        return self._eeprom.serial_str()

    def get_revision(self):
        """
        Retrieves the device version of the chassis
        Returns:
            string: device version of chassis
        """
        return self._eeprom.device_version()

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
            A string containing the hardware serial number for this
            chassis.
        """
        return self._eeprom.serial_number_str()

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the
        chassis

        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their
            corresponding values.
        """
        return self._eeprom.system_eeprom_info()

    ##############################################
    # SFP methods
    ##############################################

    def get_sfp(self, index):
        """
        Retrieves sfp represented by (1-based) index <index>
        Args:
            index: An integer, the index (1-based) of the sfp to retrieve.
            The index should be the sequence of physical SFP ports in a chassis,
            starting from 1.
            For example, 1 for first SFP port in the chassis and so on.
        Returns:
            An object dervied from SfpBase representing the specified sfp
        """
        sfp = None
        try:
            # The index will start from 1
            sfp = self._sfp_list[index-1]
        except IndexError:
            sys.stderr.write("SFP index {} out of range (1-{})\n".format(
                             index, len(self._sfp_list)))
        return sfp

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
        #if index in range(self.COPPER_PORT_START, self.COPPER_PORT_END):
        if (index >= self.COPPER_PORT_START and index <= self.COPPER_PORT_END):
            from sonic_platform_base.sfp_base import SfpBase
            return SfpBase.SFP_PORT_TYPE_BIT_RJ45
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
        """
        # Initialize SFP event first
        if not self.sfp_event_initialized:
            from sonic_platform.sfp_event import sfp_event
            self.sfp_event = sfp_event()
            self.sfp_event.initialize()
            self.MAX_SELECT_EVENT_RETURNED = self.PORT_END
            self.sfp_event_initialized = True

        wait_for_ever = (timeout == 0)
        port_dict = {}
        if wait_for_ever:
            # xrcvd will call this monitor loop in the "SYSTEM_READY" state
            # logger.log_info(" wait_for_ever get_change_event %d" % timeout)
            timeout = MAX_SELECT_DELAY
            while True:
                status = self.sfp_event.check_sfp_status( port_dict, timeout)
                if not port_dict == {}:
                    break
        else:
            # At boot up and in "INIT" state call from xrcvd will have a timeout value
            # return true without change after timeout and will transition to "SYSTEM_READY"
            # logger.log_info(" initial get_change_event %d" % timeout )
            status = self.sfp_event.check_sfp_status( port_dict, timeout)

        if status:
            return True, {'sfp':port_dict}
        else:
            return True, {'sfp':{}}

    ##############################################
    # THERMAL methods
    ##############################################

    def get_thermal_manager(self):
        from .thermal_manager import ThermalManager
        return ThermalManager

    ##############################################
    # System LED methods
    ##############################################

    def initizalize_system_led(self):
        return True

    def set_status_led(self, color):
        """
        Sets the state of the system LED

        Args:
            color: A string representing the color with which to set the
                   system LED

        Returns:
            bool: True if system LED state is set successfully, False if not
        """
        #sonic_logger.log_warning("  System LED set_status_led color:{}".format(color))
        if color not in self.system_led_supported_color:
            return False

        if (color == 'off'):
            setbits = 0x00
        elif (color == 'blue'):
            setbits = 0x03
        elif (color == 'amber'):
            setbits = 0x02
        elif (color == 'green'):
            setbits = 0x01
        elif (color == 'blinking blue'):
            setbits = 0x07
        elif (color == 'blinking amber'):
            setbits = 0x06
        elif (color == 'blinking green'):
            setbits = 0x05
        else:
            return False

        value = self._get_i2c_register(self.fan_sys_frontled_reg)
        value = int(value) & 0xF0 | setbits
        self._set_i2c_register(self.fan_sys_frontled_reg, value)
        sonic_logger.log_warning("  System LED set color:{} value:{}".format(color, value))

        return True

    def get_status_led(self):
        """
        Gets the state of the system LED

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        value = self._get_i2c_register(self.fan_sys_frontled_reg)
        value = int(value) & 0x07

        if value == 0x00:
            color = 'off'
        elif value == 0x01:
            color = 'green'
        elif value == 0x02:
            color = 'amber'
        elif value == 0x06:
            color = 'amber_blink'
        elif value == 0x05:
            color = 'green_blink'
        elif value == 0x04:
            color = 'off'
        else:
            return None

        sonic_logger.log_warning("  System LED get color %s" % color)
        return color

    def get_position_in_parent(self):
        """
	Prasanna : This method is added, since M0 test named below fails without this method. 
	platform_tests/api/test_chassis.py::TestChassisApi::test_get_position_in_parent[lab-inno-01]
	As done in other platforms such as nokia etc, return -1.

        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
	Prasanna : This method is added, since M0 test named below fails without this method. 
	platform_tests/api/test_chassis.py::TestChassisApi::test_is_replaceable[lab-inno-01]
	As done in other platforms such as nokia etc, return False.

        Indicate whether Chassis is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

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
        return (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, None)

    def get_watchdog(self):
        """
        Retrieves hardware watchdog device on this chassis

        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device
        """
        try:
            if self._watchdog is None:
                watchdog_device_path = "/dev/watchdog0"
                self._watchdog = WatchdogImplBase(watchdog_device_path)
        except Exception as e:
            sonic_logger.log_warning(" Fail to load watchdog {}".format(repr(e)))

        return self._watchdog
