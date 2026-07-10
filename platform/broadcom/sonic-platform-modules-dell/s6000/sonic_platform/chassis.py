#!/usr/bin/env python

#############################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#############################################################################

try:
    import os
    import time
    import subprocess
    import struct
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.sfp import Sfp
    from sonic_platform.eeprom import Eeprom, EepromS6000
    from sonic_platform.fan_drawer import FanDrawer
    from sonic_platform.psu import Psu
    from sonic_platform.thermal import Thermal
    from sonic_platform.component import Component
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


MAX_S6000_FANTRAY = 3
MAX_S6000_PSU = 2
MAX_S6000_THERMAL = 6
MAX_S6000_COMPONENT = 4

HYST_RANGE = 5
LEVEL0_THRESHOLD = 25
LEVEL1_THRESHOLD = 30
LEVEL2_THRESHOLD = 45
LEVEL3_THRESHOLD = 60
LEVEL4_THRESHOLD = 80
LEVEL5_THRESHOLD = 85


class Chassis(ChassisBase):
    """
    DELLEMC Platform-specific Chassis class
    """
    CPLD_DIR = "/sys/devices/platform/dell-s6000-cpld.0"

    sfp_control = ""
    PORT_START = 0
    PORT_END = 0
    reset_reason_dict = {}
    reset_reason_dict[0xe] = ChassisBase.REBOOT_CAUSE_NON_HARDWARE
    reset_reason_dict[0x6] = ChassisBase.REBOOT_CAUSE_NON_HARDWARE
    reset_reason_dict[0x7] = ChassisBase.REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER

    _num_monitor_thermals = 3
    _monitor_thermal_list = []
    _is_fan_control_enabled = False
    _fan_control_initialised = False

    _global_port_pres_dict = {}

    def __init__(self):
        ChassisBase.__init__(self)
        self.status_led_reg = "system_led"
        self.supported_led_color = ['green', 'blinking green', 'amber', 'blinking amber']
        # Initialize SFP list
        self.PORT_START = 0
        self.PORT_END = 31
        EEPROM_OFFSET = 20
        PORTS_IN_BLOCK = (self.PORT_END + 1)

        # sfp.py will read eeprom contents and retrive the eeprom data.
        # It will also provide support sfp controls like reset and setting
        # low power mode.
        # We pass the eeprom path and sfp control path from chassis.py
        # So that sfp.py implementation can be generic to all platforms
        eeprom_base = "/sys/bus/i2c/devices/i2c-{0}/{0}-0050/eeprom"
        self.sfp_control = "/sys/devices/platform/dell-s6000-cpld.0/"

        for index in range(0, PORTS_IN_BLOCK):
            eeprom_path = eeprom_base.format(index + EEPROM_OFFSET)
            sfp_node = Sfp(index, 'QSFP', eeprom_path, self.sfp_control, index)
            self._sfp_list.append(sfp_node)

        # Get Transceiver status
        self.modprs_register = self._get_transceiver_status()

        with open("/sys/class/dmi/id/product_name", "r") as fd:
            board_type = fd.read()

        if 'S6000-ON' in board_type:
            self._eeprom = Eeprom()
        else:
            self._eeprom = EepromS6000()

        for i in range(MAX_S6000_FANTRAY):
            fandrawer = FanDrawer(i)
            self._fan_drawer_list.append(fandrawer)
            self._fan_list.extend(fandrawer._fan_list)

        for i in range(MAX_S6000_PSU):
            psu = Psu(i)
            self._psu_list.append(psu)

        for i in range(MAX_S6000_THERMAL):
            thermal = Thermal(i)
            self._thermal_list.append(thermal)

        for i in range(MAX_S6000_COMPONENT):
            component = Component(i)
            self._component_list.append(component)

        for port_num in range(self.PORT_START, (self.PORT_END + 1)):
            self._global_port_pres_dict[port_num] = '0'

    def _get_cpld_register(self, reg_name):
        rv = 'ERR'
        mb_reg_file = self.CPLD_DIR+'/'+reg_name

        if (not os.path.isfile(mb_reg_file)):
            return rv

        try:
            with open(mb_reg_file, 'r') as fd:
                rv = fd.read()
        except IOError:
            rv = 'ERR'

        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv

    def _set_cpld_register(self, reg_name, value):
        # On successful write, returns the value will be written on
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'
        cpld_reg_file = self.CPLD_DIR+'/'+reg_name

        if (not os.path.isfile(cpld_reg_file)):
            return rv

        try:
            with open(cpld_reg_file, 'w') as fd:
                rv = fd.write(str(value))
        except IOError:
            rv = 'ERR'

        return rv

    def _nvram_write(self, offset, val):
        resource = "/dev/nvram"
        fd = os.open(resource, os.O_RDWR)
        if (fd < 0):
            print('File open failed ',resource)
            return
        if (os.lseek(fd, offset, os.SEEK_SET) != offset):
            print('lseek failed on ',resource)
            return
        ret = os.write(fd, struct.pack('B', val))
        if ret != 1:
            print('Write failed ',str(ret))
            return
        os.close(fd)

    def _init_fan_control(self):

        if not self._fan_control_initialised:
            for i in range(self._num_monitor_thermals):
                self._monitor_thermal_list.append(Thermal(i))
            self._fan_control_initialised = True

    def get_name(self):
        """
        Retrieves the name of the chassis
        Returns:
            string: The name of the chassis
        """
        return self._eeprom.get_model()

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
        return self._eeprom.get_serial()

    def get_status(self):
        """
        Retrieves the operational status of the chassis
        Returns:
            bool: A boolean value, True if chassis is operating properly
            False if not
        """
        return True

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

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis

        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._eeprom.get_base_mac()

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return self._eeprom.get_revision()

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

    def get_reboot_cause(self):
        """
        Retrieves the cause of the previous reboot
        """
        # In S6000, We track the reboot reason by writing the reason in
        # NVRAM. Only Warmboot and Coldboot reason are supported here.
        # Since it does not support any hardware reason, we return
        # non_hardware as default
        lrr = self._get_cpld_register('last_reboot_reason')
        if (lrr != 'ERR'):
            reset_reason = int(lrr, base=16)
            if (reset_reason in self.reset_reason_dict):
                return (self.reset_reason_dict[reset_reason], None)

        return (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, None)

    def _get_transceiver_status(self):
        presence_ctrl = self.sfp_control + 'qsfp_modprs'
        try:
            reg_file = open(presence_ctrl)

        except IOError as e:
            return False

        content = reg_file.readline().rstrip()
        reg_file.close()

        return int(content, 16)

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
                  value is a dictionary with key:value pairs in the
                  format of {'device_id':'device_event'},
                  where device_id is the device ID for this device and
                        device_event,
                             status='1' represents device inserted,
                             status='0' represents device removed.
                  Ex. {'fan':{'0':'0', '2':'1'}, 'sfp':{'11':'0'}}
                      indicates that fan 0 has been removed, fan 2
                      has been inserted and sfp 11 has been removed.
        """
        start_time = time.time()
        port_dict = {}
        ret_dict = {"sfp": port_dict}
        port = self.PORT_START
        forever = False

        if timeout == 0:
            forever = True
        elif timeout > 0:
            timeout = timeout / float(1000) # Convert to secs
        else:
            return False, {}
        end_time = start_time + timeout

        if (start_time > end_time):
            return False, ret_dict # Time wrap or possibly incorrect timeout

        while (timeout >= 0):
            # Check for OIR events and return updated port_dict
            reg_value = self._get_transceiver_status()
            if (reg_value != self.modprs_register):
                changed_ports = (self.modprs_register ^ reg_value)
                while (port >= self.PORT_START and port <= self.PORT_END):
                    # Mask off the bit corresponding to our port
                    mask = (1 << port)
                    if (changed_ports & mask):
                        # ModPrsL is active low
                        if reg_value & mask == 0:
                            port_dict[port] = '1'
                        else:
                            port_dict[port] = '0'
                    port += 1

                # Update reg value
                self.modprs_register = reg_value
                return True, ret_dict

            if forever:
                time.sleep(1)
            else:
                timeout = end_time - time.time()
                if timeout >= 1:
                    time.sleep(1) # We poll at 1 second granularity
                else:
                    if timeout > 0:
                        time.sleep(timeout)
                    return True, ret_dict
        return False, ret_dict

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
        if color not in self.supported_led_color:
            return False

        # Change color string format to the one used by driver
        color = color.replace('amber', 'yellow')
        color = color.replace('blinking ', 'blink_')
        rv = self._set_cpld_register(self.status_led_reg, color)
        if (rv != 'ERR'):
            return True
        else:
            return False

    def get_status_led(self):
        """
        Gets the state of the system LED

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        status_led = self._get_cpld_register(self.status_led_reg)
        if (status_led != 'ERR'):
            status_led = status_led.replace('yellow', 'amber')
            status_led = status_led.replace('blink_', 'blinking ')
            return status_led
        else:
            return None

    def get_thermal_manager(self):
        """
        Retrieves thermal manager class on this chassis

        Returns:
            A class derived from ThermalManagerBase representing the
            specified thermal manager
        """
        from .thermal_manager import ThermalManager
        return ThermalManager

    def set_fan_control_status(self, enable):

        if enable and not self._is_fan_control_enabled:
            self._init_fan_control()
            for thermal in self._monitor_thermal_list:
                thermal.set_high_threshold(LEVEL5_THRESHOLD, force=True)
            self._is_fan_control_enabled = True
        elif not enable and self._is_fan_control_enabled:
            for thermal in self._monitor_thermal_list:
                thermal.set_high_threshold(LEVEL4_THRESHOLD, force=True)
            self._is_fan_control_enabled = False

    def get_monitor_thermals(self):
        return self._monitor_thermal_list

    def thermal_shutdown(self):
        # Update reboot cause
        self._nvram_write(0x49, 0x7)

        subprocess.call('sync')
        time.sleep(1)
        for thermal in self._monitor_thermal_list:
            thermal.set_high_threshold(LEVEL4_THRESHOLD, force=True)

    @staticmethod
    def get_system_thermal_level(curr_thermal_level, system_temperature):

        def get_level_in_hystersis(curr_level, level1, level2):
            if curr_level != level1 and curr_level != level2:
                return level1 if abs(curr_level - level1) < abs(curr_level - level2) else level2
            else:
                return curr_level

        if system_temperature < LEVEL0_THRESHOLD:
            curr_thermal_level = 0
        elif LEVEL0_THRESHOLD <= system_temperature < LEVEL1_THRESHOLD:
            curr_thermal_level = get_level_in_hystersis(curr_thermal_level, 0, 1)
        elif LEVEL1_THRESHOLD <= system_temperature <= (LEVEL2_THRESHOLD - HYST_RANGE):
            curr_thermal_level = 1
        elif (LEVEL2_THRESHOLD - HYST_RANGE) < system_temperature < LEVEL2_THRESHOLD:
            curr_thermal_level = get_level_in_hystersis(curr_thermal_level, 1, 2)
        elif LEVEL2_THRESHOLD <= system_temperature <= (LEVEL3_THRESHOLD - HYST_RANGE):
            curr_thermal_level = 2
        elif (LEVEL3_THRESHOLD - HYST_RANGE) < system_temperature < LEVEL3_THRESHOLD:
            curr_thermal_level = get_level_in_hystersis(curr_thermal_level, 2, 3)
        elif LEVEL3_THRESHOLD <= system_temperature < LEVEL4_THRESHOLD:
            curr_thermal_level = 3
        else:
            curr_thermal_level = 4

        return curr_thermal_level

    @staticmethod
    def is_over_temperature(temperature_list):

        over_temperature = False
        for temperature in temperature_list:
            if temperature > LEVEL4_THRESHOLD:
                over_temperature = True
                break

        return over_temperature
