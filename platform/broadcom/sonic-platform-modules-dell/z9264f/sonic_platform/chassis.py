#!/usr/bin/env python

#############################################################################
# DELLEMC Z9264F
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
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.component import Component
    from sonic_platform.psu import Psu
    from sonic_platform.watchdog import Watchdog
    from sonic_platform.fan_drawer import FanDrawer
    from sonic_platform.thermal import Thermal
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

MAX_Z9264F_FANTRAY =4
MAX_Z9264F_COMPONENT = 8 # BIOS,BMC,FPGA,SYSTEM CPLD,4 SLAVE CPLDs
MAX_Z9264F_PSU = 2
MAX_Z9264F_THERMAL = 8


class Chassis(ChassisBase):
    """
    DELLEMC Platform-specific Chassis class
    """

    REBOOT_CAUSE_PATH = "/host/reboot-cause/platform/reboot_reason"
    OIR_FD_PATH = "/sys/bus/pci/devices/0000:04:00.0/port_msi"

    oir_fd = -1
    epoll = -1

    _global_port_pres_dict = {}

    def __init__(self):
        ChassisBase.__init__(self)
        # sfp.py will read eeprom contents and retrive the eeprom data.
        # We pass the eeprom path from chassis.py
        self.PORT_START = 1
        self.PORT_END = 66
        PORTS_IN_BLOCK = (self.PORT_END + 1)
        _sfp_port = list(range(65, self.PORT_END + 1))
        eeprom_base = "/sys/bus/i2c/devices/i2c-{0}/{0}-0050/eeprom"

        for index in range(self.PORT_START, PORTS_IN_BLOCK):
            port_num = index + 1
            eeprom_path = eeprom_base.format(port_num)
            if index not in _sfp_port:
                sfp_node = Sfp(index, 'QSFP', eeprom_path)
            else:
                sfp_node = Sfp(index, 'SFP', eeprom_path)
            self._sfp_list.append(sfp_node)

        self._eeprom = Eeprom()

        self._watchdog = Watchdog()

        for i in range(MAX_Z9264F_COMPONENT):
            component = Component(i)
            self._component_list.append(component)

        for i in range(MAX_Z9264F_PSU):
            psu = Psu(i)
            self._psu_list.append(psu)

        for i in range(MAX_Z9264F_FANTRAY):
            fandrawer = FanDrawer(i)
            self._fan_drawer_list.append(fandrawer)
            self._fan_list.extend(fandrawer._fan_list)

        for i in range(MAX_Z9264F_THERMAL):
            thermal = Thermal(i)
            self._thermal_list.append(thermal)

        for port_num in range(self.PORT_START, (self.PORT_END + 1)):
            #presence = self.get_sfp(port_num).get_presence()
            self._global_port_pres_dict[port_num] = '0'

    def __del__(self):
        if self.oir_fd != -1:
            self.epoll.unregister(self.oir_fd.fileno())
            self.epoll.close()
            self.oir_fd.close()

    def _get_register(self, reg_file):
        retval = 'ERR'
        if (not os.path.isfile(reg_file)):
            print(reg_file,  'not found !')
            return retval

        try:
            with os.fdopen(os.open(reg_file, os.O_RDONLY)) as fd:
                retval = fd.read()
        except Exception:
            pass
        retval = retval.rstrip('\r\n')
        retval = retval.lstrip(" ")
        return retval

    def _check_interrupts(self, port_dict):
        retval = 0
        is_port_dict_updated = False
        for port_num in range(self.PORT_START, (self.PORT_END + 1)):
            sfp = self.get_sfp(port_num)
            presence = sfp.get_presence()
            if(presence and (self._global_port_pres_dict[port_num] == '0')):
                is_port_dict_updated = True
                self._global_port_pres_dict[port_num] = '1'
                port_dict[port_num] = '1'
                self.get_sfp(port_num)._initialize_media(delay=True)
            elif(not presence and (self._global_port_pres_dict[port_num] == '1')):
                is_port_dict_updated = True
                self._global_port_pres_dict[port_num] = '0'
                port_dict[port_num] = '0'
        return retval, is_port_dict_updated

    def get_change_event(self, timeout=0):
        """
        Returns a nested dictionary containing all devices which have
        experienced a change at chassis level
        """
        port_dict = {}
        change_dict = {}
        change_dict['sfp'] = port_dict
        if timeout != 0:
            timeout = timeout / 1000
        try:
            # We get notified when there is a MSI interrupt (vector 4/5)CVR
            # Open the sysfs file and register the epoll object
            self.oir_fd = os.fdopen(os.open(self.OIR_FD_PATH, os.O_RDONLY))
            if self.oir_fd != -1:
                # Do a dummy read before epoll register
                self.oir_fd.read()
                self.epoll = select.epoll()
                self.epoll.register(
                    self.oir_fd.fileno(), select.EPOLLIN & select.EPOLLET)
            else:
                print("get_transceiver_change_event : unable to create fd")
                return False, change_dict
            # Check for missed interrupts by invoking self._check_interrupts
            # which will update the port_dict.
            while True:
                interrupt_count_start = self._get_register(self.OIR_FD_PATH)
                retval, is_port_dict_updated = self._check_interrupts(port_dict)
                if ((retval == 0) and (is_port_dict_updated is True)):
                    return True, change_dict
                interrupt_count_end = self._get_register(self.OIR_FD_PATH)
                if (interrupt_count_start == 'ERR' or
                        interrupt_count_end == 'ERR'):
                    print("get_transceiver_change_event : \
                        unable to retrive interrupt count")
                    break
                # check_interrupts() itself may take upto 100s of msecs.
                # We detect a missed interrupt based on the count
                if interrupt_count_start == interrupt_count_end:
                    break
            # Block until an xcvr is inserted or removed with timeout = -1
            events = self.epoll.poll(
                timeout=timeout if timeout != 0 else -1)
            if events:
                # check interrupts and return the change_dict
                retval, is_port_dict_updated = \
                                          self._check_interrupts(port_dict)
                if (retval != 0):
                    return False, change_dict
            return True, change_dict
        except Exception:
            return False, change_dict
        finally:
            if self.oir_fd != -1:
                self.epoll.unregister(self.oir_fd.fileno())
                self.epoll.close()
                self.oir_fd.close()
                self.oir_fd = -1
                self.epoll = -1

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

        try:
            # The index will start from 1
            sfp = self._sfp_list[index-1]
        except IndexError:
            sys.stderr.write("SFP index {} out of range (1-{})\n".format(
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
        Retrieves the serial number of the chassis (Service tag)
        Returns:
            string: Serial number of chassis
        """
        return self._eeprom.serial_str()

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
        try:
            with open(self.REBOOT_CAUSE_PATH) as fd:
                reboot_cause = int(fd.read(), 16)
        except Exception:
            return (self.REBOOT_CAUSE_NON_HARDWARE, None)

        if reboot_cause & 0x1:
            return (self.REBOOT_CAUSE_POWER_LOSS, None)
        elif reboot_cause & 0x2:
            return (self.REBOOT_CAUSE_NON_HARDWARE, None)
        elif reboot_cause & 0x4:
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, "PSU Shutdown")
        elif reboot_cause & 0x8:
            return (self.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU, None)
        elif reboot_cause & 0x10:
            return (self.REBOOT_CAUSE_WATCHDOG, None)
        elif reboot_cause & 0x20:
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, "BMC Shutdown")
        elif reboot_cause & 0x40:
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, "Hot-Swap Shutdown")
        elif reboot_cause & 0x80:
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, "Reset Button Shutdown")
        elif reboot_cause & 0x100:
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, "Reset Button Cold Reboot")
        else:
            return (self.REBOOT_CAUSE_NON_HARDWARE, None)
