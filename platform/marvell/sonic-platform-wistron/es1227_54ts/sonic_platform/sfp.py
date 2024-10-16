import os
import time
import sys
import subprocess
from ctypes import create_string_buffer

try:
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_platform_base.sonic_sfp.sfputilhelper import SfpUtilHelper
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

if sys.version_info[0] < 3:
    import commands as cmd
else:
    import subprocess as cmd

COPPER_TYPE = "COPPER"
SFP_TYPE = "SFP"

SYSLOG_IDENTIFIER = "xcvrd"
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)

class Sfp(SfpOptoeBase):
    """Platform-specific Sfp class"""

    # Paths
    EEPROM_PATH = "/sys/bus/i2c/devices/{0}-0050/eeprom"
    GPIO_PATH = "/sys/class/gpio/gpio{0}/value"

    PORT_EEPROM_I2C_MAPPING = {
        49: 5,
        50: 4,
        51: 7,
        52: 6,
        53: 9,
        54: 8,
    }

    PORT_PRESENT_GPIO_MAPPING = {
        50: 472,
        49: 476,
        52: 480,
        51: 484,
        54: 488,
        53: 492,
    }

    PORT_TX_DISABLE_GPIO_MAPPING = {
        50: 473,
        49: 477,
        52: 481,
        51: 485,
        54: 489,
        53: 493,
    }

    PORT_RX_LOS_GPIO_MAPPING = {
        50: 474,
        49: 478,
        52: 482,
        51: 486,
        54: 490,
        53: 494,
    }

    PORT_TX_FAULT_GPIO_MAPPING = {
        50: 475,
        49: 479,
        52: 483,
        51: 487,
        54: 491,
        53: 495,
    }

    PORT_SFP_EA_VALUE = {
        'fiber': {
            '1000': {
                49: {
                    'pre': 0,
                    'attn': 10,
                    'post': 0,
                    },
                50: {
                    'pre': 0,
                    'attn': 10,
                    'post': 0,
                    },
                51: {
                    'pre': 0,
                    'attn': 10,
                    'post': 0,
                    },
                52: {
                    'pre': 0,
                    'attn': 10,
                    'post': 0,
                    },
                53: {
                    'pre': 0,
                    'attn': 10,
                    'post': 0,
                    },
                54: {
                    'pre': 0,
                    'attn': 10,
                    'post': 0,
                    },
            },
            '10000': {
                49: {
                    'pre': 1,
                    'attn': 8,
                    'post': 4,
                    },
                50: {
                    'pre': 1,
                    'attn': 8,
                    'post': 4,
                    },
                51: {
                    'pre': 1,
                    'attn': 8,
                    'post': 4,
                    },
                52: {
                    'pre': 1,
                    'attn': 8,
                    'post': 4,
                    },
                53: {
                    'pre': 1,
                    'attn': 8,
                    'post': 4,
                    },
                54: {
                    'pre': 1,
                    'attn': 8,
                    'post': 4,
                    },
            },
            '25000': {
                49: {
                    'pre': 2,
                    'attn': 0,
                    'post': 8,
                    },
                50: {
                    'pre': 2,
                    'attn': 0,
                    'post': 8,
                    },
                51: {
                    'pre': 2,
                    'attn': 0,
                    'post': 8,
                    },
                52: {
                    'pre': 2,
                    'attn': 0,
                    'post': 8,
                    },
                53: {
                    'pre': 2,
                    'attn': 0,
                    'post': 8,
                    },
                54: {
                    'pre': 2,
                    'attn': 0,
                    'post': 8,
                    },
            },
        },

        'copper': {
            '1000': {
                49: {
                    'pre': 0,
                    'attn': 0,
                    'post': 0,
                    },
                50: {
                    'pre': 0,
                    'attn': 0,
                    'post': 0,
                    },
                51: {
                    'pre': 0,
                    'attn': 0,
                    'post': 0,
                    },
                52: {
                    'pre': 0,
                    'attn': 0,
                    'post': 0,
                    },
                53: {
                    'pre': 0,
                    'attn': 0,
                    'post': 0,
                    },
                54: {
                    'pre': 0,
                    'attn': 0,
                    'post': 0,
                    },
            },
            '10000': {
                49: {
                    'pre': 1,
                    'attn': 8,
                    'post': 4,
                    },
                50: {
                    'pre': 1,
                    'attn': 8,
                    'post': 4,
                    },
                51: {
                    'pre': 1,
                    'attn': 8,
                    'post': 4,
                    },
                52: {
                    'pre': 1,
                    'attn': 8,
                    'post': 4,
                    },
                53: {
                    'pre': 1,
                    'attn': 8,
                    'post': 4,
                    },
                54: {
                    'pre': 1,
                    'attn': 8,
                    'post': 4,
                    },
            },
            '25000': {
                49: {
                    'pre': 8,
                    'attn': 8,
                    'post': 2,
                    },
                50: {
                    'pre': 8,
                    'attn': 8,
                    'post': 2,
                    },
                51: {
                    'pre': 8,
                    'attn': 8,
                    'post': 2,
                    },
                52: {
                    'pre': 8,
                    'attn': 8,
                    'post': 2,
                    },
                53: {
                    'pre': 8,
                    'attn': 8,
                    'post': 2,
                    },
                54: {
                    'pre': 8,
                    'attn': 8,
                    'post': 2,
                    },
                },
        },
    }
    port_to_i2c_mapping = 0

    def __init__(self, index, sfp_type):
        SfpOptoeBase.__init__(self)
        # Init index
        self.index = index
        self.port_num = index
        self.sfp_type = sfp_type
        #self.eeprom_path = eeprom_path
        #self.port_to_i2c_mapping = port_i2c_map
        self.port_to_eeprom_mapping = {}

        if self.sfp_type == COPPER_TYPE:
            self.port_to_eeprom_mapping[index] = 'N/A'
        else:
            self.port_to_eeprom_mapping[index] = self.EEPROM_PATH.format(self.PORT_EEPROM_I2C_MAPPING[self.index])

    def get_eeprom_path(self):
        return self.port_to_eeprom_mapping[self.port_num]

    def __read_eeprom_specific_bytes(self, offset, num_bytes):
        sysfsfile_eeprom = None
        eeprom_raw = []
        for i in range(0, num_bytes):
            eeprom_raw.append("0x00")

        sysfs_sfp_i2c_client_eeprom_path = self.port_to_eeprom_mapping[self.port_num]
        try:
            sysfsfile_eeprom = open(
                sysfs_sfp_i2c_client_eeprom_path, mode="rb", buffering=0)
            sysfsfile_eeprom.seek(offset)
            raw = sysfsfile_eeprom.read(num_bytes)
            for n in range(0, num_bytes):
                if sys.version_info[0] >= 3:
                    eeprom_raw[n] = hex(raw[n])[2:].zfill(2)
                else:
                    eeprom_raw[n] = hex(ord(raw[n]))[2:].zfill(2)
        except Exception:
            pass
        finally:
            if sysfsfile_eeprom:
                sysfsfile_eeprom.close()

        return eeprom_raw

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        return False  # SFP port doesn't support this feature


    def get_rx_los(self):
        """
        Retrieves the RX LOS (lost-of-signal) status of SFP
        Returns:
            A Boolean, True if SFP has RX LOS, False if not.
            Note : RX LOS status is latched until a call to get_rx_los or a reset.
        """
        if self.sfp_type == COPPER_TYPE:
            return False
        if self.sfp_type == SFP_TYPE:
            cmdstatus, value = cmd.getstatusoutput('cat {}'.format(self.GPIO_PATH.format(self.PORT_RX_LOS_GPIO_MAPPING[self.index])))
            if cmdstatus:
                sonic_logger.log_warning("sfp rx los cmdstatus get failed")
                return False
            if int(value) == 1:
                return True
            else:
                return False

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

        if self.sfp_type == COPPER_TYPE:
            return False
        if self.sfp_type == SFP_TYPE:
            cmdstatus, value = cmd.getstatusoutput('cat {}'.format(self.GPIO_PATH.format(self.PORT_TX_FAULT_GPIO_MAPPING[self.index])))
            if cmdstatus:
                sonic_logger.log_warning("sfp tx fault cmdstatus get failed")
                return False
            if int(value) == 1:
                return True
            else:
                return False


    def get_tx_disable(self):
        """
        Retrieves the tx_disable status of this SFP
        Returns:
            A list of boolean values, representing the TX disable status
            of each available channel, value is True if SFP channel
            is TX disabled, False if not.
            E.g., for a tranceiver with four channels: [False, False, True, False]
        """
        if self.sfp_type == COPPER_TYPE:
            return None
        else:
            cmdstatus, value = cmd.getstatusoutput('cat {}'.format(self.GPIO_PATH.format(self.PORT_TX_DISABLE_GPIO_MAPPING[self.index])))
            if cmdstatus:
                sonic_logger.log_warning("sfp present cmdstatus get failed")
                return False
            if int(value) == 1:
                return [True]
            else:
                return [False]


    def get_tx_disable_channel(self):
        """
        Retrieves the TX disabled channels in this SFP
        Returns:
            A hex of 4 bits (bit 0 to bit 3 as channel 0 to channel 3) to represent
            TX channels which have been disabled in this SFP.
            As an example, a returned value of 0x5 indicates that channel 0
            and channel 2 have been disabled.
        """
        tx_disable_list = self.get_tx_disable()
        if tx_disable_list is None:
            return 0
        tx_disabled = 0
        for i in range(len(tx_disable_list)):
            if tx_disable_list[i]:
                tx_disabled |= 1 << i
        return tx_disabled

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP
        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        # SFP doesn't support this feature
        return False

    def get_power_set(self):

        # SFP doesn't support this feature
        return False

    def get_power_override(self):
        """
        Retrieves the power-override status of this SFP
        Returns:
            A Boolean, True if power-override is enabled, False if disabled
        """
        return False  # SFP doesn't support this feature

    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.
        Returns:
            A boolean, True if successful, False if not
        """

        return False  # SFP doesn't support this feature

    def tx_disable(self, tx_disable):
        """
        Disable SFP TX for all channels
        Args:
            tx_disable : A Boolean, True to enable tx_disable mode, False to disable
                         tx_disable mode.
        Returns:
            A boolean, True if tx_disable is set successfully, False if not
        """
        if self.sfp_type == COPPER_TYPE:
            return False
        if self.sfp_type == SFP_TYPE:
            gpiopin = self.GPIO_PATH.format(self.PORT_TX_DISABLE_GPIO_MAPPING[self.index])
            cmdstatus, value = cmd.getstatusoutput('echo {} > {}'.format(tx_disable, gpiopin))
            if cmdstatus:
                sonic_logger.log_warning("sfp tx_disable cmdstatus get failed")
                return False
        return True

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

        return False  # SFP doesn't support this feature

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP
        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note  : lpmode can be overridden by set_power_override
        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """

        return False  # SFP doesn't support this feature

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

        return False  # SFP doesn't support this feature

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return "Ethernet" + str(self.index - 1)

    def get_presence(self):
        """
        Retrieves the presence of the device
        Returns:
            bool: True if device is present, False if not
        """
        if self.sfp_type == COPPER_TYPE:
            return False
        if self.sfp_type == SFP_TYPE:
            cmdstatus, value = cmd.getstatusoutput('cat {}'.format(self.GPIO_PATH.format(self.PORT_PRESENT_GPIO_MAPPING[self.index])))
            if cmdstatus:
                sonic_logger.log_warning("sfp present cmdstatus get failed")
                return False
            if int(value) == 0:
                return True
            else:
                return False


    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence()

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
        if self.sfp_type == "SFP":
            return True
        else:
            return False

    def is_copper_cable(self):
        import ast
        cable_info = self.get_transceiver_info()
        if cable_info is not None:
            compliance = ast.literal_eval(cable_info['specification_compliance'])
            if compliance['SFP+CableTechnology'] == 'Passive Cable':
                return True

        return False

    def apply_ea_value(self):
        import telnetlib
        tn = telnetlib.Telnet('localhost', '12345', 5)
        is_copper = self.is_copper_cable()
        port_name = self.get_name()
        speed_cmd = 'redis-cli -n 4 hget "PORT|{}"  "speed"'.format(port_name)
        fec_cmd = 'redis-cli -n 4 hget "PORT|{}"  "fec"'.format(port_name)
        sta_cmd = 'redis-cli -n 4 hget "PORT|{}"  "admin_status"'.format(port_name)

        cmdstatus, admin_sta = cmd.getstatusoutput(sta_cmd)
        if admin_sta == 'down':
            return

        cmdstatus, speed = cmd.getstatusoutput(speed_cmd)
        cmdstatus, fec_r = cmd.getstatusoutput(fec_cmd)

        if fec_r == 'none':
            fec = 'disabled'
        elif fec_r == 'fc':
            fec = 'fc_fec'
        else:
            fec = 'rs_fec'

        ea = self.PORT_SFP_EA_VALUE['copper' if is_copper else 'fiber'][speed][self.index]
        port_cmd = "interface range ethernet 0/{}".format(self.index - 1)
        intf_cmd = "speed {} mode {}".format(speed, '1000Base_X' if speed == '1000' else 'KR' if is_copper else 'SR_LR')
        fec_cmd = "fec {}".format(fec)
        ea_cmd = "serdes tx amplitude {} emph {} {} 0 0".format(ea['attn'], ea['post'], ea['pre'])

        tn.write("config".encode('ascii') + b"\r\n")
        tn.write(port_cmd.encode('ascii') + b"\r\n")
        tn.write("no speed".encode('ascii') + b"\r\n")
        tn.write(intf_cmd.encode('ascii') + b"\r\n")
        tn.write(fec_cmd.encode('ascii') + b"\r\n")
        tn.write(ea_cmd.encode('ascii') + b"\r\n")
        tn.write("no shutdown".encode('ascii') + b"\r\n")
        tn.write("CLIexit".encode('ascii') + b"\r\n")

    def get_error_description(self):
        """
        Retrives the error descriptions of the SFP module
        Returns:
            String that represents the current error descriptions of vendor specific errors
            In case there are multiple errors, they should be joined by '|',
            like: "Bad EEPROM|Unsupported cable"
        """
        if not self.get_presence():
            err_descr = self.SFP_STATUS_UNPLUGGED
        else:
            err_descr = self.SFP_STATUS_OK

        return err_descr
