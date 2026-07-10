#!/usr/bin/env python

#############################################################################
# DELLEMC
#
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#############################################################################

try:
    import fcntl
    import os
    import time
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

QSFP_INFO_OFFSET = 128
SFP_LOCK_FILE="/etc/sonic/sfp_lock"


class Sfp(SfpOptoeBase):
    """
    DELLEMC Platform-specific Sfp class
    """

    def __init__(self, index, sfp_type, eeprom_path,
            sfp_control, sfp_ctrl_idx):
        SfpOptoeBase.__init__(self)
        self.sfp_type = sfp_type
        self.index = index + 1
        self.eeprom_path = eeprom_path
        self.sfp_control = sfp_control
        self.sfp_ctrl_idx = sfp_ctrl_idx

    def get_eeprom_path(self):
        return self.eeprom_path

    def get_name(self):
        return "QSFP+ or later"

    def read_eeprom(self, offset, num_bytes):
        try:
            fd = open(SFP_LOCK_FILE, "r")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return None
        try:
            with open(self.get_eeprom_path(), mode='rb', buffering=0) as f:
                fcntl.flock(fd, fcntl.LOCK_EX)
                self.set_modsel()
                f.seek(offset)
                read_bytes=f.read(num_bytes)
                fcntl.flock(fd, fcntl.LOCK_UN)
                return bytearray(read_bytes)
        except (OSError, IOError):
            return None

    def write_eeprom(self, offset, num_bytes, write_buffer):
        try:
            fd = open(SFP_LOCK_FILE, "r")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return None

        try:
            with open(self.get_eeprom_path(), mode='r+b', buffering=0) as f:
                fcntl.flock(fd, fcntl.LOCK_EX)
                self.set_modsel()
                f.seek(offset)
                f.write(write_buffer[0:num_bytes])
                fcntl.flock(fd, fcntl.LOCK_UN)
        except (OSError, IOError):
            return False
        return True

    def get_presence(self):
        """
        Retrieves the presence of the sfp
        """
        presence_ctrl = self.sfp_control + 'qsfp_modprs'

        try:
            fd = open(SFP_LOCK_FILE, "r")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False
        fcntl.flock(fd, fcntl.LOCK_EX)
        self.set_modsel()

        try:
            reg_file = open(presence_ctrl)
        except IOError as e:
            return False

        reg_hex = reg_file.readline().rstrip()

        # content is a string containing the hex
        # representation of the register
        reg_value = int(reg_hex, 16)

        # Mask off the bit corresponding to our port
        mask = (1 << self.sfp_ctrl_idx)

        fcntl.flock(fd, fcntl.LOCK_UN)

        # ModPrsL is active low
        if ((reg_value & mask) == 0):
            return True

        return False

    def get_modsel(self):
        modsel_ctrl = self.sfp_control + 'qsfp_modsel'
        try:
            reg_file = open(modsel_ctrl, "r+")
        except IOError as e:
            return False

        reg_hex = reg_file.readline().rstrip()

        # content is a string containing the hex
        # representation of the register
        reg_value = int(reg_hex, 16)

        # Mask off the bit corresponding to our port
        index = self.sfp_ctrl_idx

        mask = (1 << index)

        if ((reg_value & mask) == 1):
            modsel_state = False
        else:
            modsel_state = True

        return modsel_state

    def set_modsel(self):
        modsel_ctrl = self.sfp_control + 'qsfp_modsel'
        try:
            reg_file = open(modsel_ctrl, "r+")
        except IOError as e:
            return False

        reg_hex = reg_file.readline().rstrip()

        # content is a string containing the hex
        # representation of the register
        reg_value = int(reg_hex, 16)

        # Mask off the bit corresponding to our port
        index = self.sfp_ctrl_idx

        reg_value = reg_value | int("0xffffffff", 16)
        mask = (1 << index)

        reg_value = (reg_value & ~mask)

        # Convert our register value back to a hex string and write back
        content = hex(reg_value)

        reg_file.seek(0)
        reg_file.write(content)
        reg_file.close()

        return True

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        """
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

    def get_status(self):
        """
        Retrieves the operational status of the device
        """
        reset = self.get_reset_status()

        if (reset == True):
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
        Indicate whether this device  is replaceable.
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
        else:
            if not os.path.isfile(self.eeprom_path):
                return "EEPROM driver is not attached"

            try:
                with open(self.eeprom_path, mode="rb", buffering=0) as eeprom:
                    eeprom.seek(QSFP_INFO_OFFSET)
                    eeprom.read(1)
            except OSError as e:
                return "EEPROM read failed ({})".format(e.strerror)

        return self.SFP_STATUS_OK
