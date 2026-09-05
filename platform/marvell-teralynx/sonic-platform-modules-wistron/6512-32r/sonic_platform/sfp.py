#!/usr/bin/env python

#############################################################################
# Sfp contains an implementation of SONiC Platform Base API and
# provides the sfp device status which are available in the platform
#############################################################################
try:
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_py_common.logger import Logger
    import sys
    import time
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")


QSFP_TYPE_CODE_LIST = [
    '0x0d', # QSFP+ or later
    '0x11'  # QSFP28 or later
]

QSFP_DD_TYPE_CODE_LIST = [
    '0x18' # QSFP-DD Double Density 8X Pluggable Transceiver
]


QSFP_TYPE = "QSFP"
QSFP_DD_TYPE = "QSFP_DD"


# Global logger class instance
logger = Logger()


class Sfp(SfpOptoeBase):
    """Platform-specific Sfp class"""

    # Port number
    PORT_START = 0
    PORT_END = 31

    port_to_i2c_mapping = {
        0: '10',
        1: '11',
        2: '12',
        3: '13',
        4: '14',
        5: '15',
        6: '16',
        7: '17',
        8: '18',
        9: '19',
        10: '1a',
        11: '1b',
        12: '1c',
        13: '1d',
        14: '1e',
        15: '1f',
        16: '20',
        17: '21',
        18: '22',
        19: '23',
        20: '24',
        21: '25',
        22: '26',
        23: '27',
        24: '28',
        25: '29',
        26: '2a',
        27: '2b',
        28: '2c',
        29: '2d',
        30: '2e',
        31: '2f'
    }

    RESET_1_16_PATH = "/sys/bus/i2c/devices/0-0006/port{}_reset"
    RESET_17_32_PATH = "/sys/bus/i2c/devices/0-0007/port{}_reset"
    PRS_1_16_PATH = "/sys/bus/i2c/devices/0-0006/port{}_present"
    PRS_17_32_PATH = "/sys/bus/i2c/devices/0-0007/port{}_present"
    LPMODE_1_16_PATH = "/sys/bus/i2c/devices/0-0006/port{}_lpmode"
    LPMODE_17_32_PATH = "/sys/bus/i2c/devices/0-0007/port{}_lpmode"

    def __init__(self, sfp_index, sfp_type):
        # Init index
        SfpOptoeBase.__init__(self)
        self.index = sfp_index
        self.sfp_type = sfp_type

        self.port_num = self.index + 1
        self.port_type = self.sfp_type

        # Init eeprom path
        eeprom_path_prefix = '/sys/bus/i2c/devices/0-00'
        self.port_to_eeprom1_mapping = {}
        self.port_to_eeprom2_mapping = {}
        self.port_to_eeprom3_mapping = {}
        self.port_to_eeprom4_mapping = {}
        self.port_to_eeprom12_mapping = {}
        self.port_to_power_mode_mapping = {}
        self.port_to_grid_mapping = {}
        self.port_to_freq_mapping = {}
        self.port_to_outp_mapping = {}
        for x in range(self.PORT_START, self.PORT_END + 1):
            self.port_to_eeprom1_mapping[x] = eeprom_path_prefix + self.port_to_i2c_mapping[x] + '/eeprom1'
            self.port_to_eeprom2_mapping[x] = eeprom_path_prefix + self.port_to_i2c_mapping[x] + '/eeprom2'
            self.port_to_eeprom3_mapping[x] = eeprom_path_prefix + self.port_to_i2c_mapping[x] + '/eeprom3'
            self.port_to_eeprom4_mapping[x] = eeprom_path_prefix + self.port_to_i2c_mapping[x] + '/eeprom_pg4'
            self.port_to_eeprom12_mapping[x] = eeprom_path_prefix + self.port_to_i2c_mapping[x] + '/eeprom_pg12'
            self.port_to_power_mode_mapping[x] = eeprom_path_prefix + self.port_to_i2c_mapping[x] + '/power_mode'
            self.port_to_grid_mapping[x] = eeprom_path_prefix + self.port_to_i2c_mapping[x] + '/grid'
            self.port_to_freq_mapping[x] = eeprom_path_prefix + self.port_to_i2c_mapping[x] + '/freq'
            self.port_to_outp_mapping[x] = eeprom_path_prefix + self.port_to_i2c_mapping[x] + '/output_power'

        self.reinit()

    def reinit(self):
        self._detect_sfp_type(self.sfp_type)

    def _detect_sfp_type(self, sfp_type):
        eeprom_raw = []
        eeprom_raw = self._xcvr_api_factory._get_id()
        if eeprom_raw is not None:
            eeprom_raw = hex(eeprom_raw)
            if eeprom_raw in QSFP_TYPE_CODE_LIST:
                self.sfp_type = QSFP_TYPE
            elif eeprom_raw in QSFP_DD_TYPE_CODE_LIST:
                self.sfp_type = QSFP_DD_TYPE
            else:
                # Set native port type if EEPROM type is not recognized/readable
                self.sfp_type = self.port_type
        else:
            self.sfp_type = self.port_type

    def get_presence(self):
        """
        Retrieves the presence of the SFP
        Returns:
            bool: True if SFP is present, False if not
        """
        presence = False
        try:
            if self.index < 16:
                pres_path = self.PRS_1_16_PATH.format(self.port_num)
            else:
                pres_path = self.PRS_17_32_PATH.format(self.port_num)
            with open(pres_path, 'r') as sfp_presence:
                presence = int(sfp_presence.read(), 16)
        except IOError:
            return False
        logger.log_info("debug:port_ %s sfp presence is %s" % (str(self.index), str(presence)))
        return presence == True

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        # SFP doesn't support this feature
        return False

    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.
        Returns:
            A boolean, True if successful, False if not
        """
        try:
            if self.index < 16:
                file_path = self.RESET_1_16_PATH.format(self.port_num)
            else:
                file_path = self.RESET_17_32_PATH.format(self.port_num)

            with open(file_path, 'w') as fd:
                fd.write(str(1))
                time.sleep(1)
                fd.write(str(0))
                time.sleep(1)

        except IOError:
            return False

        return True

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP
        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        lpmode = False
        try:
            if self.index < 16:
                lpmode_path = self.LPMODE_1_16_PATH.format(self.port_num)
            else:
                lpmode_path = self.LPMODE_17_32_PATH.format(self.port_num)
            with open(lpmode_path, 'r') as sfp_lpmode:
                lpmode = int(sfp_lpmode.read(), 16)
        except IOError:
            return False
        return lpmode == 1

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP
        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note: lpmode can be overridden by set_power_override
        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """
        try:
            if self.index < 16:
                lpmode_path = self.LPMODE_1_16_PATH.format(self.port_num)
            else:
                lpmode_path = self.LPMODE_17_32_PATH.format(self.port_num)
            val_file = open(lpmode_path, 'w')
            val_file.write('1' if lpmode else '0')
            val_file.close()
            self.set_power_override(True, True if lpmode is True else False)
            return True
        except IOError:
            val_file.close()
            return False

    def get_power_override(self):
        """
        Retrieves the power-override status of this SFP
        Returns:
            A Boolean, True if power-override is enabled, False if disabled
        """
        # SFP doesn't support this feature
        return False

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
        if not self.get_presence():
            return False
        if self.sfp_type == QSFP_TYPE:
            try:
                power_override_bit = (1 << 0) if power_override else 0
                power_set_bit = (1 << 1) if power_set else (1 << 3)

                # Write to eeprom
                with open(self.port_to_power_mode_mapping[self.index], "w") as fd:
                    fd.write(str((power_override_bit | power_set_bit)))
                    time.sleep(0.01)
                    fd.close()
            except Exception as e:
                print('Error: unable to open file: ', str(e))
                fd.close()
                return False
            return True
        elif self.sfp_type == QSFP_DD_TYPE:
            try:
                power_override_bit = (1 << 6)
                power_set_bit = (1 << 4) if power_set else (0 << 4)

                # Write to eeprom
                with open(self.port_to_power_mode_mapping[self.index], "w") as fd:
                    fd.write(str((power_override_bit | power_set_bit)))
                    time.sleep(0.01)
                    fd.close()
            except Exception as e:
                print('Error: unable to open file: ', str(e))
                fd.close()
                return False
            return True

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """

        name = 'port'+str(self.index + 1)
        return name

    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence() and not self.get_reset_status()

    def get_position_in_parent(self):
        """
        Returns:
            Temp return 0
        """
        return 0

    def is_replaceable(self):
        """
        Retrieves if replaceable
        Returns:
            A boolean value, True if replaceable
        """
        return True

    def get_error_description(self):
        """
        Get error description

        Args:
            error_code: The error code returned by _get_error_code

        Returns:
            The error description
        """
        if self.get_presence():
            return self.SFP_STATUS_OK
        else:
            return self.SFP_STATUS_UNPLUGGED

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
        if not self.get_presence():
            return None
        if self.sfp_type == None:
            return None

        return bytearray([int(x, 16) for x in self._read_eeprom_specific_bytes(offset, num_bytes)])

    def write_eeprom(self, offset, num_bytes, write_buffer):
        """
        write eeprom specfic bytes beginning from a random offset with size as num_bytes
        and write_buffer as the required bytes

        Args:
             offset :
                     Integer, the offset from which the read transaction will start
             num_bytes:
                     Integer, the number of bytes to be written
             write_buffer:
                     bytearray, raw bytes buffer which is to be written beginning at the offset

        Returns:
            a Boolean, true if the write succeeded and false if it did not succeed.
        """
        if not self.get_presence():
            return False
        if self.sfp_type == QSFP_DD_TYPE:
            # offset check
            if offset == (0x12 * 128 + 128):
                # Write to eeprom
                with open(self.port_to_grid_mapping[self.index], "w") as fd:
                    fd.write(str((int(str(write_buffer.hex()), 16))))
                    time.sleep(0.01)
                    fd.close()
            elif offset == (0x12 * 128 + 136):
                # Write to eeprom
                with open(self.port_to_freq_mapping[self.index], "w") as fd:
                    fd.write(str((int(str(write_buffer.hex()), 16))))
                    time.sleep(0.01)
                    fd.close()
            elif offset == (0x12 * 128 + 200):
                # Write to eeprom
                with open(self.port_to_outp_mapping[self.index], "w") as fd:
                    fd.write(str((int(str(write_buffer.hex()), 16))))
                    time.sleep(0.01)
                    fd.close()
            else:
                return False
        else:
            return False

    def _read_eeprom_specific_bytes(self, offset, num_bytes, page = 0):
        sysfsfile_eeprom = None
        eeprom_raw = []
        for i in range(0, num_bytes):
            eeprom_raw.append("0x00")

        if offset < 256:
            pg = 0
        else:
            pg = (offset // 128) - 1
            offset = (offset % 128)

        if pg == 0x0:
            sysfs_sfp_i2c_client_eeprom_path = self.port_to_eeprom1_mapping[self.index]
        elif pg == 0x1:
            sysfs_sfp_i2c_client_eeprom_path = self.port_to_eeprom2_mapping[self.index]
        elif pg == 0x2:
            sysfs_sfp_i2c_client_eeprom_path = self.port_to_eeprom2_mapping[self.index]
            offset = offset + 128
        elif pg == 0x3:
            sysfs_sfp_i2c_client_eeprom_path = self.port_to_eeprom3_mapping[self.index]
        elif pg == 0x10:
            sysfs_sfp_i2c_client_eeprom_path = self.port_to_eeprom3_mapping[self.index]
            offset = offset + 128
        elif pg == 0x11:
            sysfs_sfp_i2c_client_eeprom_path = self.port_to_eeprom3_mapping[self.index]
            offset = offset + 256
        elif pg == 0x4:
            sysfs_sfp_i2c_client_eeprom_path = self.port_to_eeprom4_mapping[self.index]
        elif pg == 0x12:
            sysfs_sfp_i2c_client_eeprom_path = self.port_to_eeprom12_mapping[self.index]
        else:
            return eeprom_raw

        try:
            sysfsfile_eeprom = open(sysfs_sfp_i2c_client_eeprom_path, mode="rb", buffering=0)
            sysfsfile_eeprom.seek(offset)
            raw = sysfsfile_eeprom.read(num_bytes)
            if sys.version_info[0] >= 3:
                for n in range(0, num_bytes):
                    eeprom_raw[n] = hex(raw[n])[2:].zfill(2)
            else:
                for n in range(0, num_bytes):
                    eeprom_raw[n] = hex(ord(raw[n]))[2:].zfill(2)

        except BaseException:
            pass
        finally:
            if sysfsfile_eeprom:
                sysfsfile_eeprom.close()

        return eeprom_raw

