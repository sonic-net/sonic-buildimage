#!/usr/bin/env python

########################################################################
# DellEMC S6100
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Modules' information which are available in the platform
#
########################################################################


try:
    import os
    from sonic_platform_base.module_base import ModuleBase
    from sonic_platform.sfp import Sfp
    from sonic_platform.component import Component
    from sonic_platform.eeprom import Eeprom
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Module(ModuleBase):
    """DellEMC Platform-specific Module class"""

    HWMON_DIR = "/sys/devices/platform/SMF.512/hwmon/"
    HWMON_NODE = os.listdir(HWMON_DIR)[0]
    MAILBOX_DIR = HWMON_DIR + HWMON_NODE

    IOM_I2C_MAPPING = { 1: 14, 2: 16, 3: 15, 4: 17 }
    EEPROM_I2C_MAPPING = {
          # IOM 1
           0: [6, 66],  1: [6, 67],  2: [6, 68],  3: [6, 69],
           4: [6, 70],  5: [6, 71],  6: [6, 72],  7: [6, 73],
           8: [6, 74],  9: [6, 75], 10: [6, 76], 11: [6, 77],
          12: [6, 78], 13: [6, 79], 14: [6, 80], 15: [6, 81],
          # IOM 2
          16: [8, 34], 17: [8, 35], 18: [8, 36], 19: [8, 37],
          20: [8, 38], 21: [8, 39], 22: [8, 40], 23: [8, 41],
          24: [8, 42], 25: [8, 43], 26: [8, 44], 27: [8, 45],
          28: [8, 46], 29: [8, 47], 30: [8, 48], 31: [8, 49],
          # IOM 3
          32: [7, 50], 33: [7, 51], 34: [7, 52], 35: [7, 53],
          36: [7, 54], 37: [7, 55], 38: [7, 56], 39: [7, 57],
          40: [7, 58], 41: [7, 59], 42: [7, 60], 43: [7, 61],
          44: [7, 62], 45: [7, 63], 46: [7, 64], 47: [7, 65],
          # IOM 4
          48: [9, 18], 49: [9, 19], 50: [9, 20], 51: [9, 21],
          52: [9, 22], 53: [9, 23], 54: [9, 24], 55: [9, 25],
          56: [9, 26], 57: [9, 27], 58: [9, 28], 59: [9, 29],
          60: [9, 30], 61: [9, 31], 62: [9, 32], 63: [9, 33]
      }

    def __init__(self, module_index):
        ModuleBase.__init__(self)
        # Modules are 1-based in DellEMC platforms
        self.index = module_index + 1
        self.port_start = (self.index - 1) * 16
        self.port_end = (self.index * 16) - 1
        self.port_i2c_line = self.IOM_I2C_MAPPING[self.index]
        self._eeprom = Eeprom(iom_eeprom=True, i2c_line=self.port_i2c_line)

        self.iom_status_reg = "iom_status"
        self.iom_presence_reg = "iom_presence"

        component = Component(is_module=True, iom_index=self.index,
                              i2c_line=self.port_i2c_line, dependency=self)
        self._component_list.append(component)

        eeprom_base = "/sys/bus/i2c/devices/i2c-{0}/i2c-{1}/{1}-0050/eeprom"
        sfp_ctrl_base = "/sys/bus/i2c/devices/i2c-{0}/{0}-003e/"

        # sfp.py will read eeprom contents and retrive the eeprom data.
        # It will also provide support sfp controls like reset and setting
        # low power mode.
        for index in range(self.port_start, self.port_end + 1):
            eeprom_path = eeprom_base.format(self.EEPROM_I2C_MAPPING[index][0],
                                             self.EEPROM_I2C_MAPPING[index][1])
            sfp_control = sfp_ctrl_base.format(self.port_i2c_line)
            sfp_node = Sfp(index, 'QSFP', eeprom_path, sfp_control, index)
            self._sfp_list.append(sfp_node)

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

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return "IOM{}: {}".format(self.index, self._eeprom.modelstr())

    def get_presence(self):
        """
        Retrieves the presence of the Module

        Returns:
            bool: True if Module is present, False if not
        """
        status = False
        iom_presence = self._get_pmc_register(self.iom_presence_reg)
        if (iom_presence != 'ERR'):
            iom_presence = int(iom_presence,16)
            if (~iom_presence & (1 << (self.index - 1))):
                status = True

        return status

    def get_model(self):
        """
        Retrieves the part number of the module

        Returns:
            string: part number of module
        """
        return self._eeprom.part_number_str()

    def get_serial(self):
        """
        Retrieves the serial number of the module

        Returns:
            string: Serial number of module
        """
        return self._eeprom.serial_number_str()

    def get_status(self):
        """
        Retrieves the operational status of the Module

        Returns:
            bool: True if Module is operating properly, False if not
        """
        status = False
        iom_status = self._get_pmc_register(self.iom_status_reg)
        if (iom_status != 'ERR'):
            iom_status = int(iom_status,16)
            if (~iom_status & (1 << (self.index - 1))):
                status = True

        return status

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return self.index

    def is_replaceable(self):
        """
        Indicate whether Module is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the module

        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        # In S6100, individual modules doesn't have MAC address
        return '00:00:00:00:00:00'

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the module

        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
            Ex. { ‘0x21’:’AG9064’, ‘0x22’:’V1.0’, ‘0x23’:’AG9064-0109867821’,
                  ‘0x24’:’001c0f000fcd0a’, ‘0x25’:’02/03/2018 16:22:00’,
                  ‘0x26’:’01’, ‘0x27’:’REV01’, ‘0x28’:’AG9064-C2358-16G’}
        """
        return self._eeprom.system_eeprom_info()

    def get_description(self):
        """
        Retrieves the platform vendor's product description of the module

        Returns:
            A string, providing the vendor's product description of the module.
        """
        return self._eeprom.modelstr()

    def get_slot(self):
        """
        Retrieves the platform vendor's slot number of the module

        Returns:
            An integer, indicating the slot number in the chassis
        """
        return self.index

    def get_oper_status(self):
        """
        Retrieves the operational status of the module

        Returns:
            A string, the operational status of the module from one of the
            predefined status values: MODULE_STATUS_EMPTY, MODULE_STATUS_OFFLINE,
            MODULE_STATUS_FAULT, MODULE_STATUS_PRESENT or MODULE_STATUS_ONLINE
        """
        if self.get_presence():
            if self.get_status():
                return self.MODULE_STATUS_ONLINE
            else:
                return self.MODULE_STATUS_PRESENT
        else:
            return self.MODULE_STATUS_EMPTY

    def get_maximum_consumed_power(self):
        """
        Retrives the maximum power drawn by this module

        Returns:
            A float, with value of the maximum consumable power of the
            module.
        """
        return 97.23
