import os
import sys

try:
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

# SFP PORT numbers
SFP_PORT_START = 1
SFP_PORT_END = 12

QSFP_PORT_START = 13
QSFP_PORT_END = 14

SYSLOG_IDENTIFIER = "xcvrd"
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)

class Sfp(SfpOptoeBase):
    """Platform-specific Sfp class"""

    # The Virgo MB CPLD (bus 18, addr 0x75) is managed by the virgo2_cpld
    # kernel driver. It exports per-port status/control as flat sysfs
    # attributes and owns a 14-channel I2C mux whose child buses are numbered
    # 31..44 (port N -> bus 30+N). Each transceiver EEPROM (0x50) is
    # enumerated on its own child bus by the platform init script, and the
    # kernel mux selects the channel transparently on access.
    CPLD_SYSFS_DIR = "/sys/bus/i2c/devices/18-0075"
    SFP_MUX_BASE_BUS = 30

    def __init__(self, index, sfp_type):
        SfpOptoeBase.__init__(self)

        self.port_num = index  # 1-based physical port number
        self.sfp_type = sfp_type
        self.port_type = sfp_type
        # sysfs attribute prefix used by the CPLD driver for this port type.
        self._cpld_prefix = "qsfp" if sfp_type == "QSFP" else "sfp"
        # Each cage's EEPROM lives on its own CPLD mux child bus.
        self.eeprom_path = "/sys/bus/i2c/devices/{}-0050/eeprom".format(
            self.SFP_MUX_BASE_BUS + self.port_num)
        self.PORT_EEPROM_PATH = self.eeprom_path


    def __read_eeprom_specific_bytes_raw(self, offset, num_bytes):
        if not os.path.exists(self.PORT_EEPROM_PATH):
            sonic_logger.log_error("path %s is not exist" % self.PORT_EEPROM_PATH)
            return None

        sysfsfile_eeprom = None
        raw = None

        try:
            sysfsfile_eeprom = open(self.PORT_EEPROM_PATH, mode="rb", buffering=0)
            sysfsfile_eeprom.seek(offset)
            raw = sysfsfile_eeprom.read(num_bytes)
        except Exception as e:
            pass
        finally:
            if sysfsfile_eeprom:
                sysfsfile_eeprom.close()
        return raw

    def __read_eeprom_specific_bytes(self, offset, num_bytes):

        raw = self.__read_eeprom_specific_bytes_raw(offset, num_bytes)
        if raw is None:
            return None

        eeprom_raw = []
        for i in range(0, num_bytes):
            eeprom_raw.append("0x00")

        for n in range(0, num_bytes):
            eeprom_raw[n] = hex(raw[n])[2:].zfill(2)

        return eeprom_raw


    def __cpld_attr_path(self, field):
        return "{}/{}{}_{}".format(self.CPLD_SYSFS_DIR, self._cpld_prefix,
                                   self.port_num, field)

    def __read_cpld_attr(self, field):
        """Read an integer CPLD port attribute; return None on failure."""
        try:
            with open(self.__cpld_attr_path(field), "r") as f:
                return int(f.read().strip())
        except (OSError, ValueError):
            sonic_logger.log_warning("Failed to read CPLD attribute %s" % field)
            return None

    def __write_cpld_attr(self, field, value):
        """Write an integer CPLD port attribute; return True on success."""
        try:
            with open(self.__cpld_attr_path(field), "w") as f:
                f.write(str(int(value)))
        except (OSError, ValueError):
            sonic_logger.log_warning("Failed to write CPLD attribute %s" % field)
            return False
        return True

########  DEVICE BASE API  #########

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return "Ethernet{}".format(self.port_num - 1)

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        # Driver normalizes the active-low CPLD bit: 1 = present, 0 = absent.
        value = self.__read_cpld_attr("present")
        if value is None:
            return False

        return value == 1

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return not self.get_reset_status()

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return self.port_num

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

########  SFP BASE API  #########

    def get_transceiver_info(self):
        port_info = None
        try:
            port_info = SfpOptoeBase.get_transceiver_info(self)
        except NotImplementedError:
            pass
        if port_info is not None:
            port_info['hardware_rev'] = 'N/A'
        # Temporarily use eeprom stub info
        if port_info is None and self.get_presence():
            port_info = {
                "type": "N/A",
                "type_abbrv_name": 'N/A',
                "vendor_rev": 'N/A',
                "serial": 'N/A',
                "manufacturer": 'N/A',
                "model": 'N/A',
                "connector": 'N/A',
                "encoding": 'N/A',
                "ext_identifier": 'N/A',
                "ext_rateselect_compliance": 'N/A',
                "cable_type": 'N/A',
                "cable_length": 0,
                "nominal_bit_rate": 'N/A',
                "specification_compliance": 'N/A',
                "vendor_date": 'N/A',
                "vendor_oui": 'N/A',
                "application_advertisement": "N/A",
                "hardware_rev": 'N/A'
            }

        return port_info

    def get_transceiver_threshold_info(self):
        return None

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP

        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        # Only QSFP28 cages have a CPLD reset line (ZSQCR1).
        if self.sfp_type != "QSFP":
            return False

        # Driver reports 1 when the module is held in reset.
        value = self.__read_cpld_attr("reset")
        if value is None:
            return False

        return value == 1

    def get_rx_los(self):
        """
        Retrieves the RX LOS (loss-of-signal) status of SFP

        Returns:
            A list of boolean values, representing the RX LOS status
            of each available channel, value is True if SFP channel
            has RX LOS, False if not.
            E.g., for a tranceiver with four channels: [False, False, True, False]
            Note : RX LOS status is latched until a call to get_rx_los or a reset.
        """
        # Only SFP28 cages expose an RX-LOS status bit.
        if self.sfp_type != "SFP":
            return [False]

        value = self.__read_cpld_attr("rx_los")
        if value is None:
            return [False]

        return [value == 1]

    def get_tx_fault(self):
        """
        Retrieves the TX fault status of SFP

        Returns:
            A list of boolean values, representing the TX fault status
            of each available channel, value is True if SFP channel
            has TX fault, False if not.
            E.g., for a tranceiver with four channels: [False, False, True, False]
            Note : TX fault status is lached until a call to get_tx_fault or a reset.
        """
        # Only SFP28 cages expose a TX-fault status bit.
        if self.sfp_type != "SFP":
            return [False]

        value = self.__read_cpld_attr("tx_fault")
        if value is None:
            return [False]

        return [value == 1]

    def get_tx_disable(self):
        """
        Retrieves the tx_disable status of this SFP

        Returns:
            A list of boolean values, representing the TX disable status
            of each available channel, value is True if SFP channel
            is TX disabled, False if not.
            E.g., for a tranceiver with four channels: [False, False, True, False]
        """
        if self.sfp_type != "SFP":
            return [False]

        value = self.__read_cpld_attr("tx_disable")
        if value is None:
            return [False]

        return [value == 1]

    def get_tx_disable_channel(self):
        """
        Retrieves the TX disabled channels in this SFP

        Returns:
            A hex of 4 bits (bit 0 to bit 3 as channel 0 to channel 3) to represent
            TX channels which have been disabled in this SFP.
            As an example, a returned value of 0x5 indicates that channel 0
            and channel 2 have been disabled.
        """
        raise NotImplementedError

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP

        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        # Only QSFP28 cages expose an LPMode control line (ZSQCR3, 0x10).
        if self.sfp_type != "QSFP":
            return False

        value = self.__read_cpld_attr("lpmode")
        if value is None:
            return False

        return value == 1

    def get_power_override(self):
        """
        Retrieves the power-override status of this SFP

        Returns:
            A Boolean, True if power-override is enabled, False if disabled
        """
        return False

    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.

        Returns:
            A boolean, True if successful, False if not
        """
        # RJ45 and SFP ports are not resettable; QSFP28 uses ZSQCR1.
        if self.sfp_type != "QSFP":
            return False

        # Writing 1 tells the CPLD to pulse RESETn; the hardware auto-releases
        # back to normal operation, so no manual assert/deassert timing is
        # needed here.
        return self.__write_cpld_attr("reset", 1)

    def tx_disable(self, tx_disable):
        """
        Disable SFP TX for all channels

        Args:
            tx_disable : A Boolean, True to enable tx_disable mode, False to disable
                         tx_disable mode.

        Returns:
            A boolean, True if tx_disable is set successfully, False if not
        """
        if self.sfp_type != "SFP":
            return False

        # Driver handles the read-modify-write of the CPLD TX-disable register.
        return self.__write_cpld_attr("tx_disable", 1 if tx_disable else 0)

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
        # if (channel != 0):
        #     return False

        # return self.tx_disable(disable)
        raise NotImplementedError

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP

        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note  : lpmode can be overridden by set_power_override

        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """
        # Only QSFP28 cages expose an LPMode control line (ZSQCR3).
        if self.sfp_type != "QSFP":
            return False

        return self.__write_cpld_attr("lpmode", 1 if lpmode else 0)

    def set_power_override(self, power_override, power_set):
        """
        Sets SFP power level using power_override and power_set

        Args:
            power_override :
                    A Boolean, True to override set_lpmode and use power_set
                    to control SFP power, False to disable SFP power control
                    through power_override/power_set and use set_lpmode
                    to control SFP power.
            power_set :
                    Only valid when power_override is True.
                    A Boolean, True to set SFP to low power mode, False to set
                    SFP to high power mode.

        Returns:
            A boolean, True if power-override and power_set are set successfully,
            False if not
        """
        raise NotImplementedError

    def read_eeprom(self, offset, num_bytes):
        """
        read eeprom specfic bytes beginning from a random offset with size as num_bytes

        Args:
             offset :
                     Integer, the offset from which the read transaction will start
             num_bytes:
                     Integer, the number of bytes to be read

        Returns:
            bytearray, if raw sequence of bytes are read correctly from the offset of size num_bytes
            None, if the read_eeprom fails
        """
        raw = self.__read_eeprom_specific_bytes_raw(offset, num_bytes)
        if raw is None:
            return None
        return bytearray(raw)

    def write_eeprom(self, offset, num_bytes, write_buffer):
        """
        write eeprom specific bytes beginning from a random offset with size as num_bytes
        The kernel CPLD mux selects this port's channel transparently on access.
        """
        try:
            with open(self.eeprom_path, mode="r+b", buffering=0) as f:
                f.seek(offset)
                f.write(bytes(write_buffer[0:num_bytes]))
        except (OSError, IOError):
            return False
        return True

    def get_eeprom_path(self):
        return self.eeprom_path

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

########  CUSTOM SFP API  #########

    def get_transceiver_pm(self):
        '''
        xcvr api is not implemented
        '''
        return None

    def get_transceiver_status(self):
        """
        Below part is added to avoid failing xcvrd
        """
        raise NotImplementedError
