#!/usr/bin/env python
"""
#############################################################################
# SuperMicro SSE-T7132S
#
# Sfp contains an implementation of SONiC Platform Base API and
# provides the sfp status which are available in the platform
#
#############################################################################
"""

try:
    import os
    import time
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase

except ImportError as err:
    raise ImportError(str(err) + "- required module not found")

QSFP_INFO_OFFSET = 128
SFP_INFO_OFFSET = 0
QSFP_DD_PAGE0 = 0

SFP_TYPE_LIST = [
    '0x3' # SFP/SFP+/SFP28 and later
]
QSFP_TYPE_LIST = [
    '0x0c', # QSFP
    '0x0d', # QSFP+ or later
    '0x11'  # QSFP28 or later
]
QSFP_DD_TYPE_LIST = [
    '0x18' #QSFP_DD Type
]

OSFP_TYPE_LIST = [
    '0x19' # OSFP 8X Type
]

SFP_TYPE = "SFP"
QSFP_TYPE = "QSFP"
OSFP_TYPE = "OSFP"
QSFP_DD_TYPE = "QSFP_DD"
SFP_NAME = "Ethernet{}"

PORT_START = 0
PORT_END = 34
QSFP_PORT_START = 0
QSFP_PORT_END = 32

I2C_EEPROM_PATH = '/sys/bus/i2c/devices/i2c-{0}/{0}-0050/eeprom'
PORT_INFO_PATH= '/sys/class/t7132s_cpld'

class Sfp(SfpOptoeBase):
    """Platform-specific Sfp class"""
    PLATFORM = "x86_64-supermicro_sse_t7132s-r0"
    HWSKU = "Supermicro_sse_t7132s"

    _port_to_offset = [11, 30, 12, 29, 13, 28, 14, 27, 15, 34,
                       16, 33, 17, 32, 18, 31, 19, 38, 20, 37,
                       21, 36, 22, 35, 23, 42, 24, 41, 25, 40,
                       26, 39,
                       43, 44]

    def __init__(self, sfp_index=0):
        SfpOptoeBase.__init__(self)

        self.index = sfp_index      # for sfputil show error-status --fetch-from-hardware
        self._master_port = self.index - 1
        self._port_num = self.index
        self.sfp_type = QSFP_DD_TYPE
        #port_type is the native port type and sfp_type is the transceiver type
        #sfp_type will be detected in get_transceiver_info
        if self._master_port < QSFP_PORT_END:
            self.port_type = QSFP_DD_TYPE
            self.NUM_CHANNELS = 8
            self.port_name = "QSFP" + str(self._port_num)
            self._name = [SFP_NAME.format(str(self._master_port*8))]
        else:
            self.port_type = SFP_TYPE
            self.NUM_CHANNELS = 1
            self.port_name = "SFP" + str(self._port_num - QSFP_PORT_END)
            self._name = [SFP_NAME.format(str((QSFP_PORT_END*8)+(self._master_port - QSFP_PORT_END)))]
        self.sfp_type = self.port_type
        self.sfp_eeprom_path = self.get_eeprom_path()
        self._initialize_media(delay=False)

    def _detect_sfp_type(self):
        eeprom_raw = []
        eeprom_ready = True

        time_begin = time.time()
        eeprom_ready = False
        while (time.time() - time_begin) < 4:
            # read 2 more bytes to check eeprom ready
            eeprom_raw = self.read_eeprom(XCVR_TYPE_OFFSET, XCVR_TYPE_WIDTH + 2)
 
            if eeprom_raw:
                if eeprom_raw[0] in SFP_TYPE_CODE_LIST:
                    self.sfp_type = SFP_TYPE
                    eeprom_ready = True
                elif eeprom_raw[0] in QSFP_TYPE_CODE_LIST:
                    self.sfp_type = QSFP_TYPE
                    eeprom_ready = True
                elif eeprom_raw[0] in QSFP_DD_TYPE_CODE_LIST:
                    self.sfp_type = QSFP_DD_TYPE
                    eeprom_ready = True
                else:
                    self.sfp_type = self.port_type
                    if all([b == '00' for b in eeprom_raw]):
                        logger.Logger('SFP').log_warning(
                            "_detect_sfp_type: {} index {} by {} eeprom all 0".
                            format(self._name, self.index,
                                   inspect.currentframe().f_back.f_code.co_name))
                    else:
                        eeprom_ready = True
            else:
                logger.Logger('SFP').log_warning(
                    "_detect_sfp_type: {} index {} by {} eeprom none".
                    format(self._name, self.index,
                           inspect.currentframe().f_back.f_code.co_name))
                self.sfp_type = self.port_type

            if not eeprom_ready:
                # retry after sleep
                time.sleep(1)
            else:
                break;

        if self.sfp_type == QSFP_DD_TYPE:
            self.NUM_CHANNELS = 8
        elif self.sfp_type == QSFP_TYPE:
            self.NUM_CHANNELS = 4
        elif self.sfp_type == SFP_TYPE:
            self.NUM_CHANNELS = 1

        return eeprom_ready

    def get_eeprom_path(self):
        """
        Returns SFP eeprom path
        """
        port_eeprom_path = I2C_EEPROM_PATH.format(self._port_to_offset[self._master_port])
        return port_eeprom_path

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return self._name[0]

    def _initialize_media(self, delay=False):
        """
        Initialize the media type and eeprom driver for SFP
        """
        if delay:
            time.sleep(1)
            self._xcvr_api = None
            self.get_xcvr_api()

        self.set_media_type()
        self.reinit_sfp_driver()

    def get_presence(self):
        """
        Retrieves the presence of the SFP
        Returns:
            bool: True if SFP is present, False if not
        """
        sysfs_filename = "sfp_modabs" if self.port_type == SFP_TYPE else "qsfp_modprs"
        reg_path = "/".join([PORT_INFO_PATH, self.port_name, sysfs_filename])

        # Read status
        try:
            with open(reg_path) as reg_file:
                content = reg_file.readline().rstrip()
                reg_value = int(content)
                # Module present is active low
                if reg_value == 0:
                    return True
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        # not present
        return False

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        if self.port_type != QSFP_DD_TYPE:
            return False

        try:
            with open(
                "/".join([PORT_INFO_PATH, self.port_name, "qsfp_reset"])) as reg_file:
                # Read status
                content = reg_file.readline().rstrip()
                reg_value = int(content)
                # reset is active low
                if reg_value == 0:
                    return True
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        return False

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP
        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        if self.port_type != QSFP_DD_TYPE:
            return False

        try:
            with open(
                "/".join([PORT_INFO_PATH, self.port_name, "qsfp_lpmode"])) as reg_file:
                # Read status
                content = reg_file.readline().rstrip()
                reg_value = int(content)
                # low power mode is active high
                if reg_value == 0:
                    return False
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        return True

    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.
        Returns:
            A boolean, True if successful, False if not
        """
        if self.port_type != QSFP_DD_TYPE:
            return False

        try:
            with open(
                "/".join([PORT_INFO_PATH, self.port_name, "qsfp_reset"]), "w") as reg_file:
                # Convert our register value back to a hex string and write back
                reg_file.seek(0)
                reg_file.write(hex(0))
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        # Sleep 1 second to allow it to settle
        time.sleep(1)

        # Flip the bit back high and write back to the register to take port out of reset
        try:
            with open(
                "/".join([PORT_INFO_PATH, self.port_name, "qsfp_reset"]), "w") as reg_file:
                reg_file.seek(0)
                reg_file.write(hex(1))
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        return True

    def no_reset(self):
        """
        Set CPLD qsfp_reset to 1 for non-reset status.
        Returns:
            A boolean, True if successful, False if not
        """
        if self.port_type != QSFP_DD_TYPE:
            return False

        try:
            with open(
                "/".join([PORT_INFO_PATH, self.port_name, "qsfp_reset"]), "w") as reg_file:
                reg_file.seek(0)
                reg_file.write(hex(1))
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        return True

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP
        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note  : lpmode can be overridden by set_power_override
        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """
        if not self._detect_sfp_type():
            return False

        if self.port_type != QSFP_DD_TYPE:
            return False

        try:
            reg_file = open(
                "/".join([PORT_INFO_PATH, self.port_name, "qsfp_lpmode"]), "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        content = hex(lpmode)

        reg_file.seek(0)
        reg_file.write(content)
        reg_file.close()

        return True

    def set_media_type(self):
        """
        Reads optic eeprom byte to determine media type inserted
        """
        eeprom_raw = []
        eeprom_raw = self._xcvr_api_factory._get_id()
        if eeprom_raw is not None:
            eeprom_raw = hex(eeprom_raw)
            if eeprom_raw in SFP_TYPE_LIST:
                self.sfp_type = SFP_TYPE
            elif eeprom_raw in QSFP_TYPE_LIST:
                self.sfp_type = QSFP_TYPE
            elif eeprom_raw in QSFP_DD_TYPE_LIST:
                self.sfp_type = QSFP_DD_TYPE
            else:
                #Set native port type if EEPROM type is not recognized/readable
                self.sfp_type = self.port_type
        else:
            self.sfp_type = self.port_type

        return self.sfp_type

    def reinit_sfp_driver(self):
        """
        Changes the driver based on media type detected
        """

        i2c_bus = self.sfp_eeprom_path[25:].split('/')[0]
        del_sfp_path = "/sys/bus/i2c/devices/i2c-{0}/delete_device".format(i2c_bus)
        new_sfp_path = "/sys/bus/i2c/devices/i2c-{0}/new_device".format(i2c_bus)
        driver_path = "/sys/bus/i2c/devices/i2c-{0}/{0}-0050/name".format(i2c_bus)

        if not os.path.isfile(driver_path):
            print(driver_path, "does not exist")
            return False

        try:
            with os.fdopen(os.open(driver_path, os.O_RDONLY)) as filed:
                driver_name = filed.read()
                driver_name = driver_name.rstrip('\r\n')
                driver_name = driver_name.lstrip(" ")

            #Avoid re-initialization of the QSFP/SFP optic on QSFP/SFP port.
            if self.sfp_type == SFP_TYPE and driver_name in ['optoe1', 'optoe3']:
                with open(del_sfp_path, 'w') as f:
                    f.write('0x50\n')
                time.sleep(0.2)
                with open(new_sfp_path, 'w') as f:
                    f.write('optoe2 0x50\n')
                time.sleep(2)
            elif self.sfp_type == OSFP_TYPE and driver_name in ['optoe2', 'optoe3']:
                with open(del_sfp_path, 'w') as f:
                    f.write('0x50\n')
                time.sleep(0.2)
                with open(new_sfp_path, 'w') as f:
                    f.write('optoe1 0x50\n')
                time.sleep(2)
            elif self.sfp_type == QSFP_DD_TYPE and driver_name in ['optoe1', 'optoe2']:
                with open(del_sfp_path, 'w') as f:
                    f.write('0x50\n')
                time.sleep(0.2)
                with open(new_sfp_path, 'w') as f:
                    f.write('optoe3 0x50\n')
                time.sleep(2)

        except IOError as err:
            print("Error: Unable to open file: %s" %str(err))
            return False

        return True

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return 0

    @staticmethod
    def is_replaceable():
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
        else:
            if not os.path.isfile(self.sfp_eeprom_path):
                return "EEPROM driver is not attached"

            if self.sfp_type == SFP_TYPE:
                offset = SFP_INFO_OFFSET
            elif self.sfp_type == OSFP_TYPE:
                offset = QSFP_INFO_OFFSET
            elif self.sfp_type == QSFP_TYPE:
                offset = QSFP_INFO_OFFSET
            elif self.sfp_type == QSFP_DD_TYPE:
                offset = QSFP_DD_PAGE0
            else:
                return "Invalid SFP type {}".format(self.sfp_type)

            try:
                with open(self.sfp_eeprom_path, mode="rb", buffering=0) as eeprom:
                    eeprom.seek(offset)
                    eeprom.read(1)
            except OSError as e:
                return "EEPROM read failed ({})".format(e.strerror)

        return self.SFP_STATUS_OK