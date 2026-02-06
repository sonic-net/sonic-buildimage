#!/usr/bin/env python

#############################################################################
# DELLEMC
#
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#############################################################################

try:
    import os
    import time
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Sfp(SfpOptoeBase):
    """
    DELLEMC Platform-specific Sfp class
    """

    def __init__(self, index, sfp_type, eeprom_path,
            sfp_control, sfp_ctrl_idx):
        SfpOptoeBase.__init__(self)
        self.sfp_type = sfp_type
        self.index = index
        self.eeprom_path = eeprom_path
        self.sfp_control = sfp_control
        self.sfp_ctrl_idx = sfp_ctrl_idx

    def get_eeprom_path(self):
        return self.eeprom_path

    def get_name(self):
        if self.index > 31:
            return "SFP+"
        return "QSFP+ or later"
    def get_presence(self):
        """
        Retrieves the presence of the sfp
        """
        if self.index > 31:
            return False
        presence_ctrl = self.sfp_control + 'qsfp_modprs'
        try:
            reg_file = open(presence_ctrl)
        except IOError as e:
            return False

        reg_hex = reg_file.readline().rstrip()

        # content is a string containing the hex
        # representation of the register
        reg_value = int(reg_hex, 16)

        # Mask off the bit corresponding to our port
        index = self.sfp_ctrl_idx

        # Mask off the bit corresponding to our port
        mask = (1 << index)

        # ModPrsL is active low
        if ((reg_value & mask) == 0):
            return True

        return False

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        """
        if self.index > 31:
            return False
        reset_status = None
        reset_ctrl = self.sfp_control + 'qsfp_reset'
        try:
            reg_file = open(reset_ctrl, "r+")
        except IOError as e:
            return False

        reg_hex = reg_file.readline().rstrip()

        # content is a string containing the hex
        # representation of the register
        reg_value = int(reg_hex, 16)

        # Mask off the bit corresponding to our port
        index = self.sfp_ctrl_idx

        mask = (1 << index)

        if ((reg_value & mask) == 0):
            reset_status = True
        else:
            reset_status = False

        return reset_status

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP
        """
        if self.index > 31:
            return False
        lpmode_ctrl = self.sfp_control + 'qsfp_lpmode'
        try:
            reg_file = open(lpmode_ctrl, "r+")
        except IOError as e:
            return False

        reg_hex = reg_file.readline().rstrip()

        # content is a string containing the hex
        # representation of the register
        reg_value = int(reg_hex, 16)

        # Mask off the bit corresponding to our port
        index = self.sfp_ctrl_idx

        mask = (1 << index)

        if ((reg_value & mask) == 0):
            lpmode_state = False
        else:
            lpmode_state = True

        return lpmode_state


    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.
        """
        if self.index > 31:
            return False
        reset_ctrl = self.sfp_control + 'qsfp_reset'
        try:
            # Open reset_ctrl in both read & write mode
            reg_file = open(reset_ctrl, "r+")
        except IOError as e:
            return False

        reg_hex = reg_file.readline().rstrip()
        reg_value = int(reg_hex, 16)

        # Mask off the bit corresponding to our port
        index = self.sfp_ctrl_idx

        # Mask off the bit corresponding to our port
        mask = (1 << index)

        # ResetL is active low
        reg_value = (reg_value & ~mask)

        # Convert our register value back to a
        # hex string and write back
        reg_file.seek(0)
        reg_file.write(hex(reg_value))
        reg_file.close()

        # Sleep 1 second to allow it to settle
        time.sleep(1)

        # Flip the bit back high and write back to the
        # register to take port out of reset
        try:
            reg_file = open(reset_ctrl, "w")
        except IOError as e:
            return False

        reg_value = reg_value | mask
        reg_file.seek(0)
        reg_file.write(hex(reg_value))
        reg_file.close()

        return True

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP
        """
        if self.index > 31:
            return False
        lpmode_ctrl = self.sfp_control + 'qsfp_lpmode'
        try:
            reg_file = open(lpmode_ctrl, "r+")
        except IOError as e:
            return False

        reg_hex = reg_file.readline().rstrip()

        # content is a string containing the hex
        # representation of the register
        reg_value = int(reg_hex, 16)

        # Mask off the bit corresponding to our port
        index = self.sfp_ctrl_idx

        mask = (1 << index)

        # LPMode is active high; set or clear the bit accordingly
        if lpmode is True:
            reg_value = (reg_value | mask)
        else:
            reg_value = (reg_value & ~mask)

        # Convert our register value back to a hex string and write back
        content = hex(reg_value)

        reg_file.seek(0)
        reg_file.write(content)
        reg_file.close()

        return True
