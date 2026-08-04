#!/usr/bin/env python

try:
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_py_common.logger import Logger

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SFP_TYPE = "SFP"
QSFP_TYPE = "QSFP"
QSFP_DD_TYPE = "QSFP_DD"

GPIO_OFFSET = 9984
GPIO_VALUE = "/sys/class/gpio/gpio{}/value"
EEPROM_PATH = "/sys/bus/i2c/devices/{}-0050/eeprom"

logger = Logger('sonic-platform-sfp')

class Sfp(SfpOptoeBase):
    """
    BFN Platform-specific SFP class
    """

    def __init__(self, port_num):
        SfpOptoeBase.__init__(self)
        self.index = port_num
        self.port_num = port_num
        self.sfp_type = QSFP_TYPE

        self.eeprom_path = EEPROM_PATH.format(9 + (port_num ^ 1))
        # print(self.eeprom_path)

    def __get_attr_value(self, filepath):
        try:
            with open(filepath, 'r') as fd:
                # text
                data = fd.readlines()
                return data[0].rstrip('\r\n')
        except FileNotFoundError:
            logger.log_error(f"File {filepath} not found.  Aborting")
        except (OSError, IOError) as ex:
            logger.log_error("Cannot open - {}: {}".format(filepath, repr(ex)))

        return 'ERR'

    def get_presence(self):
        """
        Retrieves the presence of the sfp
        """
        presence = False

        gpio = GPIO_OFFSET + 224 + (16 if self.index < 16 else - 16)
        attr_rv = self.__get_attr_value(
            GPIO_VALUE.format(gpio + (self.index) ^ 1))
        if attr_rv != 'ERR':
            if attr_rv == "1":
                presence = True
        else:
            raise SyntaxError

        return presence

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP
        """
        lpmode = False
        gpio = GPIO_OFFSET + 160 + (16 if self.index < 16 else - 16)
        attr_rv = self.__get_attr_value(
            GPIO_VALUE.format(gpio + (self.index) ^ 1))
        if attr_rv != 'ERR':
            if attr_rv == "1":
                lpmode = True
        else:
            raise SyntaxError

        return lpmode

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP
        """
        return False

    def get_eeprom_path(self):
        return self.eeprom_path

    def write_eeprom(self, offset, num_bytes, write_buffer):
        # Not supported at the moment
        return False

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return "sfp{}".format(self.index)

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        """
        status = False
        gpio = GPIO_OFFSET + 128 + (16 if self.index < 16 else - 16)
        attr_rv = self.__get_attr_value(
            GPIO_VALUE.format(gpio + (self.index) ^ 1))
        if attr_rv != 'ERR':
            if attr_rv == "1":
                status = True
        else:
            raise SyntaxError

        return status

    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.
        """
        return False

    def get_status(self):
        """
        Retrieves the operational status of the device
        """
        reset = self.get_reset_status()

        if reset:
            status = False
        else:
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
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def get_error_description(self):
        """
        Retrives the error descriptions of the SFP module
        Returns:
            String that represents the current error descriptions of vendor specific errors
            In case there are multiple errors, they should be joined by '|',
            like: "Bad EEPROM|Unsupported cable"
        """
        if not self.get_presence():
            return self.SFP_STATUS_UNPLUGGED
        return self.SFP_STATUS_OK

    def tx_disable(self, tx_disable):
        """
        Disable SFP TX for all channels
        Args:
            tx_disable : A Boolean, True to enable tx_disable mode, False to disable
                         tx_disable mode.
        Returns:
            A boolean, True if tx_disable is set successfully, False if not
        """
        if self.sfp_type == QSFP_TYPE:
            return self.tx_disable_channel(0xF, tx_disable)
        return False

    def tx_disable_channel(self, channel, disable):
        """
        Sets the tx_disable for specified SFP channels

        Args:
            channel : A hex of 4 bits (bit 0 to bit 3) which represent channel 0 to 3,
                      e.g. 0x5 for channel 0 and channel 2.
            disable : A boolean, True to disable TX channels specified in channel,
                      False to enable

        Returns:
            A boolean, True if successful, False if not
        """
        return False

    def get_power_override(self):
        return False

    def set_power_override(self, power_override, power_set):
        return False
