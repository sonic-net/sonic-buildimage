#!/usr/bin/env python
"""
Version: 1.0

Module contains an implementation of SONiC Platform Base API and
provides the Sfp information
"""
try:
    import os
    import fcntl
    import time
    from monotonic import monotonic as _time
    from vendor_sonic_platform.devcfg import Devcfg
    from vendor_sonic_platform.utils import UtilsHelper
    from sonic_platform_base.sfp_byted import SfpByted
except ImportError as error:
    raise ImportError(str(error) + "- required module not found")

START_ADAP = UtilsHelper.get_i2c_adapt_start()
POWER_ON_SET_FILE = '/tmp/xcvr_power_on/set'


class Sfp(SfpByted):
    """
    Platform-specific Sfp class
    """

    def __init__(self, index, parent=None, cage_type=None):
        self._index = index + 1

        self.sfp_base_path = Devcfg.SFP_BASH_PATH.format(self._index)
        self.i2c_optoe_path = Devcfg.I2C_DIR.format(index + START_ADAP, index + START_ADAP)
        super(Sfp, self).__init__(self._index, parent, cage_type, self.i2c_optoe_path)
        self._old_presence = self.get_presence()
        if self._old_presence:
            self.presence_timestamp = _time()
        else:
            self.presence_timestamp = 0

    def get_optic_temp(self):
        """
        Retrieves the temperature(str) of the sfp
        """
        if not self.get_presence():
            self.presence_timestamp = 0
            return 0
        else:
            if not self.presence_timestamp:
                self.presence_timestamp = _time()

        # get temp when presence > 2s
        if _time() - self.presence_timestamp < 2:
            return 0

        if not self.is_power_on():
            return 0

        self.xcvr_type_refresh()
        sfpd_obj = self.xcvr.dom(calibration_type=self.get_calibration())
        dom_temp_raw = self.read_eeprom(self.xcvr.dom_offset + self.xcvr.temp_offset_in_dom,
                                        self.xcvr.temp_width)

        if not dom_temp_raw:
            return 0

        temperature_data = sfpd_obj.parse_temperature(dom_temp_raw, 0).get('data', {})
        temperature = temperature_data.get('Temperature', {}).get('value', 'N/A')

        return float(temperature.strip('C'))

    def get_presence(self):
        """
        Retrieves the presence of the sfp
        """

        presence_ctrl = self.sfp_base_path + 'present'

        try:
            with open(presence_ctrl, 'r') as reg_file:
                content = reg_file.readline().rstrip()
        except Exception as err:
            self.log_error(str(err))
            return False

        if content == "1":
            return True

        return False

    def get_rx_los(self):
        """
        Retrieves the RX LOS (loss-of-signal) status of SFP
        Implemnted by ODM if the sfp rx_los pin is link to a cpld.
        Otherwise call super(Sfp, self).xxx()
        Returns:
            A list of boolean values, representing the RX LOS status
            of each available channel, value is True if SFP channel
            has RX LOS, False if not.
            E.g., for a tranceiver with four channels: [False, False, True, False]
            Note : RX LOS status is latched until a call to get_rx_los or a reset.
        """

        return super(Sfp, self).get_rx_los()

    def get_tx_fault(self):
        """
        Retrieves the TX fault status of SFP
        Implemnted by ODM if the sfp tx_fault pin is link to a cpld.
        Otherwise call super(Sfp, self).xxx()
        Returns:
            A list of boolean values, representing the TX fault status
            of each available channel, value is True if SFP channel
            has TX fault, False if not.
            E.g., for a tranceiver with four channels: [False, False, True, False]
            Note : TX fault status is lached until a call to get_tx_fault or a reset.
        """
        return super(Sfp, self).get_tx_fault()

    def get_tx_disable(self):
        """
        Retrieves the tx_disable status of this SFP
        Implemnted by ODM if the sfp tx_disable pin is link to a cpld.
        Otherwise call super(Sfp, self).xxx()
        Returns:
            A list of boolean values, representing the TX disable status
            of each available channel, value is True if SFP channel
            is TX disabled, False if not.
            E.g., for a tranceiver with four channels: [False, False, True, False]
        """
        if not self.get_presence():
            return []
        if self._cage_type == "SFP":
            try:
                cpld_dir = '/sys/switch/debug/cpld/board_cpld'
                byte_index, bit_index = (self.index - 1) // 8, 1 << (self.index - 1) % 8
                with open(cpld_dir, 'rb') as read_fd:
                    for line in read_fd.readlines():
                        if line.startswith('0x0090:'):
                            # tx_disable is located in board_cpld 0x95-0x9a
                            r_tx = int(line.split()[6 + byte_index], 16)
                            break
                ret = False if r_tx & bit_index != 0 else True
                return [ret]
            except Exception as err:
                self.log_error("get SFP tx_disable failed .{}".format(err))
                return []
        else:
            return super(Sfp, self).get_tx_disable()

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        if self.get_presence():
            if self._cage_type == "SFP":
                return False

        reg_hex = self.read_attr(self.sfp_base_path + 'reset')
        return False if reg_hex == '1' else True

    def set_reset(self, reset):
        """
        Reset SFP and return all user module settings to their default state.
        Args:
            reset: True  ---- set reset
                   False ---- set unreset
        Returns:
            A boolean, True if successful, False if not
        """
        if self._cage_type == "SFP":
            return False

        reset_file = self.sfp_base_path + "reset"
        value = "0" if reset is True else "1"
        return self.write_attr(reset_file, value)

    def tx_disable(self, tx_disable):
        """
        Disable SFP TX for all channels
        Implemnted by ODM if the sfp tx_disable pin is link to a cpld.
        Otherwise call super(Sfp, self).xxx()
        Args:
            tx_disable : A Boolean, True to enable tx_disable mode, False to disable
                         tx_disable mode.

        Returns:
            A boolean, True if tx_disable is set successfully, False if not
        """
        if not self.get_presence():
            return False

        self.log_info('{} set tx_disable {}...'.format(self.get_name(), tx_disable))
        if self._cage_type == "SFP":
            try:
                cpld_dir = '/sys/switch/debug/cpld/board_cpld'
                byte_index, bit_index = (self.index - 1) // 8, 1 << (self.index - 1) % 8
                with open(cpld_dir, 'rb') as read_fd:
                    for line in read_fd.readlines():
                        if line.startswith('0x0090:'):
                            # tx_disable is located in board_cpld 0x95-0x9a
                            r_tx = int(line.split()[5 + byte_index], 16)
                            break
                set_byte = r_tx | bit_index if tx_disable else r_tx & ~(bit_index)
                os.system(
                    "echo {index}:{byte} > {path}".format(
                        index=hex(0x95 + byte_index),
                        byte=hex(set_byte),
                        path=cpld_dir))
            except Exception as err:
                self.log_error("set SFP tx_disable failed .{}".format(err))
                return False
            return True
        else:
            return super(Sfp, self).tx_disable(tx_disable)

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP

        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        if not self.get_presence():
            return False

        if self._cage_type == "SFP":
            return False
        else:
            lpmode_path = self.sfp_base_path + "lpmode"
            return True if self.read_attr(lpmode_path) == '1' else False

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP

        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note  : lpmode can be overridden by set_power_override

        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """
        if not self.get_presence():
            return False

        self.log_info('{} set low power mode {}...'.format(self.get_name(), lpmode))
        if self._cage_type == "SFP":
            return False
        else:
            lpmode_path = self.sfp_base_path + "lpmode"
            set = "1" if lpmode is True else "0"
            return self.write_attr(lpmode_path, set)

    def is_power_on(self):
        """
        Retrieves the sfp power_on status

        Returns:
            True  -- power on
            False -- power off
        """
        if not os.path.exists(POWER_ON_SET_FILE):
            return False

        value = self.read_attr(Devcfg.SFP_DIR + 'power_on')

        return True if '1' in value else False
