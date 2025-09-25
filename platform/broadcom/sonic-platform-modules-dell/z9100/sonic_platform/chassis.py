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
    from sonic_platform.component import Component
    from sonic_platform.eeprom import Eeprom
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


MAX_Z9100_FANTRAY = 5
MAX_Z9100_PSU = 2
MAX_Z9100_THERMAL = 8
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

    def __init__(self):
        ChassisBase.__init__(self)
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

    def __del__(self):
        if self.oir_fd != -1:
            self.epoll.unregister(self.oir_fd.fileno())
            self.epoll.close()
            self.oir_fd.close()

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

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis

        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._eeprom.base_mac_addr()

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the chassis

        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
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
        reset_reason = int(self._get_pmc_register('smf_reset_reason'))
        power_reason = int(self._get_pmc_register('smf_poweron_reason'))

        # Reset_Reason = 11 ==> PowerLoss
        # So return the reboot reason from Last Power_Reason Dictionary
        # If Reset_Reason is not 11 return from Reset_Reason dictionary
        # Also check if power_reason, reset_reason are valid values by
        # checking key presence in dictionary else return
        # REBOOT_CAUSE_HARDWARE_OTHER as the Power_Reason and Reset_Reason
        # registers returned invalid data
        if (reset_reason == 11):
            if (power_reason in self.power_reason_dict):
                return (self.power_reason_dict[power_reason], None)
        else:
            if (reset_reason in self.reset_reason_dict):
                return (self.reset_reason_dict[reset_reason], None)

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
