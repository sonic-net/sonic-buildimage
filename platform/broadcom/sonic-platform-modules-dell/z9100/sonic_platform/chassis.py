#!/usr/bin/env python

#############################################################################
# DELLEMC Z9100
#
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#############################################################################

try:
    import os
    import select
    import sys
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.sfp import Sfp
    from sonic_platform.fan_drawer import FanDrawer
    from sonic_platform.psu import Psu
    from sonic_platform.thermal import Thermal
    from sonic_platform.watchdog import Watchdog, WatchdogTCO
    from sonic_platform.component import Component
    from sonic_platform.eeprom import Eeprom
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

MAX_Z9100_FANTRAY = 5
MAX_Z9100_PSU = 2
MAX_Z9100_THERMAL = 12
MAX_Z9100_COMPONENT = 6


class Chassis(ChassisBase):
    """
    DELLEMC Platform-specific Chassis class
    """

    HWMON_DIR = "/sys/devices/platform/SMF.512/hwmon/"
    HWMON_NODE = os.listdir(HWMON_DIR)[0]
    MAILBOX_DIR = HWMON_DIR + HWMON_NODE
    EEPROM_I2C_MAPPING = {
        0: [9, 18], 1: [9, 19], 2: [9, 20], 3: [9, 21],
        4: [9, 22], 5: [9, 23], 6: [9, 24], 7: [9, 25],
        8: [8, 26], 9: [8, 27], 10: [8, 28], 11: [8, 29],
        12: [8, 31], 13: [8, 30], 14: [8, 33], 15: [8, 32],  # Remapped 4 entries
        16: [7, 34], 17: [7, 35], 18: [7, 36], 19: [7, 37],
        20: [7, 38], 21: [7, 39], 22: [7, 40], 23: [7, 41],
        24: [6, 42], 25: [6, 43], 26: [6, 44], 27: [6, 45],
        28: [6, 46], 29: [6, 47], 30: [6, 48], 31: [6, 49]
    }
    PORT_I2C_MAPPING = {
        # 0th Index = i2cLine, 1st Index = portIdx in i2cLine
        0: [14, 0], 1: [14, 1], 2: [14, 2], 3: [14, 3],
        4: [14, 4], 5: [14, 5], 6: [14, 6], 7: [14, 7],
        8: [14, 8], 9: [14, 9], 10: [14, 10], 11: [14, 11],
        12: [15, 0], 13: [15, 1], 14: [15, 2], 15: [15, 3],
        16: [15, 4], 17: [15, 5], 18: [15, 6], 19: [15, 7],
        20: [15, 8], 21: [15, 9], 22: [16, 0], 23: [16, 1],
        24: [16, 2], 25: [16, 3], 26: [16, 4], 27: [16, 5],
        28: [16, 6], 29: [16, 7], 30: [16, 8], 31: [16, 9]
    }

    OIR_FD_PATH = "/sys/devices/platform/dell_ich.0/sci_int_gpio_sus6"

    reset_reason_dict = {}
    reset_reason_dict[11] = ChassisBase.REBOOT_CAUSE_POWER_LOSS
    reset_reason_dict[33] = ChassisBase.REBOOT_CAUSE_WATCHDOG
    reset_reason_dict[44] = ChassisBase.REBOOT_CAUSE_NON_HARDWARE
    reset_reason_dict[55] = ChassisBase.REBOOT_CAUSE_NON_HARDWARE

    power_reason_dict = {}
    power_reason_dict[11] = ChassisBase.REBOOT_CAUSE_POWER_LOSS
    power_reason_dict[22] = ChassisBase.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU
    power_reason_dict[33] = ChassisBase.REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC
    power_reason_dict[44] = ChassisBase.REBOOT_CAUSE_INSUFFICIENT_FAN_SPEED

    status_led_reg_to_color = {
        0x00: 'green', 0x01: 'blinking green', 0x02: 'amber',
        0x04: 'amber', 0x08: 'blinking amber', 0x10: 'blinking amber'
    }

    color_to_status_led_reg = {
        'green': 0x00, 'blinking green': 0x01,
        'amber': 0x02, 'blinking amber': 0x08
    }

    _global_port_pres_dict = {}

    def __init__(self):
        ChassisBase.__init__(self)
        self.status_led_reg = "sys_status_led"
        self.supported_led_color = ['green', 'blinking green', 'amber', 'blinking amber']
        
        self.oir_fd = -1
        self.epoll = -1
        PORT_START = 0
        PORT_END = 31
        PORTS_IN_BLOCK = (PORT_END + 1)


        # sfp.py will read eeprom contents and retrive the eeprom data.
        # It will also provide support sfp controls like reset and setting
        # low power mode.
        # We pass the eeprom path and sfp control path from chassis.py
        # So that sfp.py implementation can be generic to all platforms
        eeprom_base = "/sys/bus/i2c/devices/i2c-{0}/i2c-{1}/{1}-0050/eeprom"
        sfp_ctrl_base = "/sys/bus/i2c/devices/i2c-{0}/{0}-003e/"
        for index in range(0, PORTS_IN_BLOCK):
            eeprom_path = eeprom_base.format(self.EEPROM_I2C_MAPPING[index][0],
                                             self.EEPROM_I2C_MAPPING[index][1])
            sfp_control = sfp_ctrl_base.format(self.PORT_I2C_MAPPING[index][0])
            sfp_node = Sfp(index, 'QSFP', eeprom_path, sfp_control,
                           self.PORT_I2C_MAPPING[index][1])
            self._sfp_list.append(sfp_node)

        # Append 10 G SFP ports
        self._sfp_list.append(Sfp(33,'SFP',"/sys/bus/i2c/devices/i2c-11/11-0050/eeprom","",33))
        self._sfp_list.append(Sfp(34,'SFP',"/sys/bus/i2c/devices/i2c-12/12-0050/eeprom","",34))

        # Initialize EEPROM
        self._eeprom = Eeprom()
        for i in range(MAX_Z9100_FANTRAY):
            fandrawer = FanDrawer(i)
            self._fan_drawer_list.append(fandrawer)
            self._fan_list.extend(fandrawer._fan_list)

        for i in range(MAX_Z9100_PSU):
            psu = Psu(i)
            self._psu_list.append(psu)

        for i in range(MAX_Z9100_THERMAL):
            thermal = Thermal(i)
            self._thermal_list.append(thermal)

        for i in range(MAX_Z9100_COMPONENT):
            component = Component(i)
            self._component_list.append(component)

        for i in self._sfp_list:
            presence = i.get_presence()
            if presence:
                self._global_port_pres_dict[i.index] = '1'
            else:
                self._global_port_pres_dict[i.index] = '0'

        bios_ver = self.get_component(0).get_firmware_version()
        bios_minor_ver = bios_ver.split("-")[-1]
        if bios_minor_ver.isdigit() and (int(bios_minor_ver) >= 9):
            self._watchdog = WatchdogTCO()
        else:
            self._watchdog = Watchdog()

    def __del__(self):
        if self.oir_fd != -1:
            self.epoll.unregister(self.oir_fd.fileno())
            self.epoll.close()
            self.oir_fd.close()

    def _get_reboot_reason_smf_register(self):
        # In S6100, mb_poweron_reason register will
        # Returns 0xaa or 0xcc on software reload
        # Returns 0x88 on cold-reboot happened during software reload
        # Returns 0xff or 0xbb on power-cycle
        # Returns 0xdd on Watchdog
        # Returns 0xee on Thermal Shutdown
        # Returns 0x99 on Unknown reset
        smf_mb_reg_reason = self._get_pmc_register('mb_poweron_reason')
        return int(smf_mb_reg_reason, 16)
    
    def _get_pmc_register(self, reg_name):
        # On successful read, returns the value read from given
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'
        mb_reg_file = self.MAILBOX_DIR + '/' + reg_name

        if (not os.path.isfile(mb_reg_file)):
            return rv

        try:
            with open(mb_reg_file, 'r') as fd:
                rv = fd.read()
        except Exception as error:
            rv = 'ERR'

        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv
    
    def _set_pmc_register(self, reg_name, value):
        # On successful write, returns the length of value written on
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'
        mb_reg_file = self.MAILBOX_DIR + '/' + reg_name

        if (not os.path.isfile(mb_reg_file)):
            return rv

        try:
            with open(mb_reg_file, 'w') as fd:
                rv = fd.write(str(value))
        except IOError:
            rv = 'ERR'

        return rv
    
    def _get_register(self, reg_file):
        # On successful read, returns the value read from given
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(reg_file)):
            return rv

        try:
            with open(reg_file, 'r') as fd:
                rv = fd.read()
        except Exception as error:
            rv = 'ERR'

        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv

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
        Retrieves the serial number of the chassis (Service tag)
        Returns:
            string: Serial number of chassis
        """
        return self._eeprom.serial_str()

    def get_sfp(self, index):
        """
        Retrieves sfp represented by (1-based) index <index>

        Args:
            index: An integer, the index (1-based) of the sfp to retrieve.
                   The index should be the sequence of a physical port in a chassis,
                   starting from 1.
                   For example, 0 for Ethernet0, 1 for Ethernet4 and so on.

        Returns:
            An object dervied from SfpBase representing the specified sfp
        """
        sfp = None

        try:
            sfp = self._sfp_list[index-1]
        except IndexError:
            sys.stderr.write("SFP index {} out of range (1-{})\n".format(
                             index, len(self._sfp_list)-1))
        return sfp

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
        return self._eeprom.base_mac_addr()
    
    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return self._eeprom.revision_str()
    
    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the chassis

        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
        """
        return self._eeprom.system_eeprom_info()


    def get_module_index(self, module_name):
        """
        Retrieves module index from the module name

        Args:
            module_name: A string, prefixed by SUPERVISOR, LINE-CARD or FABRIC-CARD
            Ex. SUPERVISOR0, LINE-CARD1, FABRIC-CARD5
        Returns:
            An integer, the index of the ModuleBase object in the module_list
        """
        module_index = re.match(r'IOM([1-4])', module_name).group(1)
        return int(module_index) - 1

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
        reset_reason = int(self._get_pmc_register('smf_reset_reason'))
        power_reason = int(self._get_pmc_register('smf_poweron_reason'))
        smf_mb_reg_reason = self._get_reboot_reason_smf_register()

        if ((smf_mb_reg_reason == 0xbb) or (smf_mb_reg_reason == 0xff)):
            return (ChassisBase.REBOOT_CAUSE_POWER_LOSS, None)
        elif ((smf_mb_reg_reason == 0xaa) or (smf_mb_reg_reason == 0xcc)):
            return (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, None)
        elif (smf_mb_reg_reason == 0x88):
            return (ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER, "CPU Reset")
        elif (smf_mb_reg_reason == 0xdd):
            return (ChassisBase.REBOOT_CAUSE_WATCHDOG, None)
        elif (smf_mb_reg_reason == 0xee):
            return (self.power_reason_dict[power_reason], None)
        elif (reset_reason == 66):
            return (ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER,
                    "Emulated Cold Reset")
        elif (reset_reason == 77):
            return (ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER,
                    "Emulated Warm Reset")
        else:
            return (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, None)

        return (ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER, "Invalid Reason")

    def _check_interrupts(self, port_dict):
        is_port_dict_updated = False

        cpld2_abs_int = self._get_register(
                    "/sys/bus/i2c/devices/i2c-14/14-003e/qsfp_abs_int")
        cpld2_abs_sta = self._get_register(
                    "/sys/bus/i2c/devices/i2c-14/14-003e/qsfp_abs_sta")
        cpld3_abs_int = self._get_register(
                    "/sys/bus/i2c/devices/i2c-15/15-003e/qsfp_abs_int")
        cpld3_abs_sta = self._get_register(
                    "/sys/bus/i2c/devices/i2c-15/15-003e/qsfp_abs_sta")
        cpld4_abs_int = self._get_register(
                    "/sys/bus/i2c/devices/i2c-16/16-003e/qsfp_abs_int")
        cpld4_abs_sta = self._get_register(
                    "/sys/bus/i2c/devices/i2c-16/16-003e/qsfp_abs_sta")

        if (cpld2_abs_int == 'ERR' or cpld2_abs_sta == 'ERR' or
                cpld3_abs_int == 'ERR' or cpld3_abs_sta == 'ERR' or
                cpld4_abs_int == 'ERR' or cpld4_abs_sta == 'ERR'):
            return False, is_port_dict_updated

        cpld2_abs_int = int(cpld2_abs_int, 16)
        cpld2_abs_sta = int(cpld2_abs_sta, 16)
        cpld3_abs_int = int(cpld3_abs_int, 16)
        cpld3_abs_sta = int(cpld3_abs_sta, 16)
        cpld4_abs_int = int(cpld4_abs_int, 16)
        cpld4_abs_sta = int(cpld4_abs_sta, 16)

        # Make it contiguous (discard reserved bits)
        interrupt_reg = (cpld2_abs_int & 0xfff) |\
                        ((cpld3_abs_int & 0x3ff) << 12) |\
                        ((cpld4_abs_int & 0x3ff) << 22)
        status_reg = (cpld2_abs_sta & 0xfff) |\
                     ((cpld3_abs_sta & 0x3ff) << 12) |\
                     ((cpld4_abs_sta & 0x3ff) << 22)

        for port in range(self.get_num_sfps()):
            if interrupt_reg & (1 << port):
                # update only if atleast one port has generated
                # interrupt
                is_port_dict_updated = True
                if status_reg & (1 << port):
                    # status reg 1 => optics is removed
                    port_dict[port+1] = '0'
                else:
                    # status reg 0 => optics is inserted
                    port_dict[port+1] = '1'

        return True, is_port_dict_updated

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
        port_dict = {}
        ret_dict = {'sfp': port_dict}
        if timeout != 0:
            timeout = timeout / 1000
        try:
            # We get notified when there is an SCI interrupt from GPIO SUS6
            # Open the sysfs file and register the epoll object
            self.oir_fd = open(self.OIR_FD_PATH, "r")
            if self.oir_fd != -1:
                # Do a dummy read before epoll register
                self.oir_fd.read()
                self.epoll = select.epoll()
                self.epoll.register(self.oir_fd.fileno(),
                                    select.EPOLLIN & select.EPOLLET)
            else:
                return False, ret_dict

            # Check for missed interrupts by invoking self.check_interrupts
            # which will update the port_dict.
            while True:
                interrupt_count_start = self._get_register(self.OIR_FD_PATH)

                retval, is_port_dict_updated = \
                                          self._check_interrupts(port_dict)
                if (retval is True) and (is_port_dict_updated is True):
                    return True, ret_dict

                interrupt_count_end = self._get_register(self.OIR_FD_PATH)

                if (interrupt_count_start == 'ERR' or
                        interrupt_count_end == 'ERR'):
                    break

                # check_interrupts() itself may take upto 100s of msecs.
                # We detect a missed interrupt based on the count
                if interrupt_count_start == interrupt_count_end:
                    break

            # Block until an xcvr is inserted or removed with timeout = -1
            events = self.epoll.poll(timeout=timeout if timeout != 0 else -1)
            if events:
                # check interrupts and return the port_dict
                retval, is_port_dict_updated = \
                                          self._check_interrupts(port_dict)

            return retval, ret_dict
        except Exception:
            return False, ret_dict
        finally:
            if self.oir_fd != -1:
                self.epoll.unregister(self.oir_fd.fileno())
                self.epoll.close()
                self.oir_fd.close()
                self.oir_fd = -1
                self.epoll = -1

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

        value = self.color_to_status_led_reg[color]
        rv = self._set_pmc_register(self.status_led_reg, value)
        if (rv != 'ERR'):
            return True
        else:
            return False

    def get_status_led(self):
        """
        Gets the state of the system LED

        Returns:
            A string, one of the valid LED color strings which could be
            vendor specified.
        """
        reg_val = self._get_pmc_register(self.status_led_reg)
        if (reg_val != 'ERR'):
            return self.status_led_reg_to_color.get(int(reg_val, 16), None)
        else:
            return "N/A"