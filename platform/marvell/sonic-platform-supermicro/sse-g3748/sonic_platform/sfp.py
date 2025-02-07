#############################################################################
# Supermicro SSE-G3748
#
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information by using sfp refactor spec
#
#############################################################################
#
# There are 2 IO Expander connect to SFP transceiver status/control
#---------------------------------------------------------------------------
#
# SFP transceiver status:
#
# PCAL6524 -- [I2C 0x22]    24 bits I/O for SFP status and TX_DIS
#   Port 0 --    7      6      5      4        3      2      1      0
#            TX_FAULT TX_DIS  LOS  PRESENT TX_FAULT TX_DIS  LOS  PRESENT
#              SFP1    SFP1   SFP1  SFP1     SFP0    SFP0   SFP0  SFP0
#
#   Port 1 --    7      6      5      4        3      2      1      0
#            TX_FAULT TX_DIS  LOS  PRESENT TX_FAULT TX_DIS  LOS  PRESENT
#              SFP3    SFP3   SFP3  SFP3     SFP2    SFP2   SFP2  SFP2
#
#   Port 2 --    7      6      5      4        3      2      1      0
#            TX_FAULT TX_DIS  LOS  PRESENT TX_FAULT TX_DIS  LOS  PRESENT
#              SFP5    SFP5   SFP5  SFP5     SFP4    SFP4   SFP4  SFP4
# [TX_DIS bit is output, all other bits are input]
#---------------------------------------------------------------------------
#
# SFP transceiver control:
#
# PCAL6416A -- [I2C 0x20]  16 bits I/) Expander  SFP RS0 and RS1
#    Port 0 --   7    6    5    4        3    2    1    0
#               RS0  RS1  RS0  RS1      RS0  RS1  RS0  RS1
#                  SF2       SF3          SF0       SF1
#
#    Port 0 --   7    6    5    4        3    2    1    0
#               N/A  N/A  N/A  N/A      RS0  RS1  RS0  RS1
#                                         SF4       SF5
# [ All bits are output]
#
#############################################################################

try:
    import os
    import sys
    import time
    import subprocess
    import shlex
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_platform_base.sfp_base import SfpBase

    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

if sys.version_info[0] < 3:
    import commands as cmd
else:
    import subprocess as cmd

smbus_present = 1

try:
    import smbus
except ImportError as e:
    smbus_present = 0

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

COPPER_TYPE = "COPPER"
SFP_TYPE = "SFP"
RJ45_TYPE = "RJ45"

# SFP PORT numbers
SFP_PORT_START = 49
SFP_PORT_END = 54

SYSLOG_IDENTIFIER = "xcvrd"
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)

class Sfp(SfpOptoeBase):
    """
    Supermicro SSE-G3748 refactor Platform-specific Sfp class
    """

    def __init__(self, index, sfp_type, eeprom_path, port_i2c_map):
        SfpOptoeBase.__init__(self)
        self.index = index
        self.port_num = index
        self.eeprom_path = eeprom_path
        self.sfp_eeprom_path = eeprom_path
        #port_type is the native port type and sfp_type is the transceiver type
        #sfp_type will be detected in get_transceiver_info
        self.port_type = sfp_type
        if (self.index < SFP_PORT_START):
            self.port_type = RJ45_TYPE
        self.sfp_type = self.port_type

        self._port_to_i2c_mapping = port_i2c_map
        self.port_name = sfp_type + str(index)

        self._initialize_media(delay=False)

    def sfp_status_get_value(self, port_off):
        reg_in  = [0x00, 0x00, 0x01, 0x01, 0x02, 0x02]

        if smbus_present == 0:  # if called from sfputil outside of pmon
            regin = reg_in[port_off]
            cmdstatus, sfpstatus = cmd.getstatusoutput('sudo i2cget -y 1 0x22 %d' % regin)
            if cmdstatus:
                sonic_logger.log_warning("sfp cmdstatus i2c get failed %s" % status)
                return False
            sfpStatus = int(sfpstatus, 16)
        else:
            bus = smbus.SMBus(1)
            DEVICE_ADDRESS = 0x22
            DEVICE_REG = reg_in[port_off]
            sfpStatus = bus.read_byte_data(DEVICE_ADDRESS, DEVICE_REG)

        return sfpStatus

    def sfp_status_set_value(self, port_off, val):
        reg_out = [0x04, 0x04, 0x05, 0x05, 0x06, 0x06]

        if smbus_present == 0:  # if called from sfputil outside of pmon
            regout = reg_out[port_off]
            i2csetcmd = "i2cset -y 1 0x22 " + hex(regout) + hex(val)
            cmdstatus, output = cmd.getstatusoutput(i2csetcmd)
            if cmdstatus:
                sonic_logger.log_warning("sfp cmdstatus i2c write failed %s" % output )
                return False
        else:
            bus = smbus.SMBus(1)
            DEVICE_ADDRESS = 0x22
            DEVICE_REG = reg_out[port_off]
            bus.write_byte_data(DEVICE_ADDRESS, DEVICE_REG, val)

        return True

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

    def set_media_type(self):
        """
        Reads optic eeprom byte to determine media type inserted
        """
        eeprom_raw = []
        eeprom_raw = self._xcvr_api_factory._get_id()
        if eeprom_raw is not None:
            eeprom_raw = hex(eeprom_raw)
            if eeprom_raw in SFP_TYPE_LIST:
                self.sfp_type = 'SFP'
            elif eeprom_raw in QSFP_TYPE_LIST:
                self.sfp_type = 'QSFP'
            elif eeprom_raw in QSFP_DD_TYPE_LIST:
                self.sfp_type = 'QSFP_DD'
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
        """
        del_sfp_path = "/sys/class/i2c-adapter/i2c-{0}/delete_device".format(
            self._port_to_i2c_mapping[self.index])
        new_sfp_path = "/sys/class/i2c-adapter/i2c-{0}/new_device".format(
            self._port_to_i2c_mapping[self.index])
        driver_path = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/name".format(
            self._port_to_i2c_mapping[self.index])
        """
        i2c_bus = self.sfp_eeprom_path[27:].split('/')[0]
        del_sfp_path = "/sys/class/i2c-adapter/i2c-{0}/delete_device".format(i2c_bus)
        new_sfp_path = "/sys/class/i2c-adapter/i2c-{0}/new_device".format(i2c_bus)
        driver_path = "/sys/class/i2c-adapter/i2c-{0}/{0}-0050/name".format(i2c_bus)
        delete_device = "echo 0x50 >" + del_sfp_path

        if not os.path.isfile(driver_path):
            #print(driver_path, "does not exist")
            return False

        try:
            with os.fdopen(os.open(driver_path, os.O_RDONLY)) as filed:
                driver_name = filed.read()
                driver_name = driver_name.rstrip('\r\n')
                driver_name = driver_name.lstrip(" ")

            #Avoid re-initialization of the QSFP/SFP optic on QSFP/SFP port.
            if self.sfp_type == 'SFP' and driver_name in ['optoe1', 'optoe3']:
                subprocess.Popen(shlex.split(delete_device), stdout=subprocess.PIPE)
                time.sleep(0.2)
                new_device = "echo optoe2 0x50 >" + new_sfp_path
                subprocess.Popen(shlex.split(new_device), stdout=subprocess.PIPE)
                time.sleep(2)
            elif self.sfp_type == 'QSFP' and driver_name in ['optoe2', 'optoe3']:
                subprocess.Popen(shlex.split(delete_device), stdout=subprocess.PIPE)
                time.sleep(0.2)
                new_device = "echo optoe1 0x50 >" + new_sfp_path
                subprocess.Popen(shlex.split(new_device), stdout=subprocess.PIPE)
                time.sleep(2)
            elif self.sfp_type == 'QSFP_DD' and driver_name in ['optoe1', 'optoe2']:
                subprocess.Popen(shlex.split(delete_device), stdout=subprocess.PIPE)
                time.sleep(0.2)
                new_device = "echo optoe3 0x50 >" + new_sfp_path
                subprocess.Popen(shlex.split(new_device), stdout=subprocess.PIPE)
                time.sleep(2)

        except IOError as err:
            print("Error: Unable to open file: %s" %str(err))
            return False

        return True

    def get_transceiver_status(self):
        return {}

    def get_transceiver_info(self):
        if self.sfp_type == COPPER_TYPE or self.sfp_type == RJ45_TYPE:
            transceiver_info_keys = ['manufacturer',
                                     'model',
                                     'vendor_rev',
                                     'serial',
                                     'vendor_oui',
                                     'vendor_date',
                                     'connector',
                                     'encoding',
                                     'ext_identifier',
                                     'ext_rateselect_compliance',
                                     'cable_type',
                                     'cable_length',
                                     'specification_compliance',
                                     'nominal_bit_rate',
                                     'application_advertisement']
            transceiver_info_dict = dict.fromkeys(transceiver_info_keys, 'N/A')
            transceiver_info_dict['type'] = self.sfp_type
            return transceiver_info_dict
        else:
            return super(Sfp, self).get_transceiver_info()

    def get_transceiver_bulk_status(self):
        if self.sfp_type == COPPER_TYPE or self.sfp_type == RJ45_TYPE:
            transceiver_dom_info_dict = {}
            dom_info_dict_keys = ['temperature',    'voltage',
                                  'rx1power',       'rx2power',
                                  'rx3power',       'rx4power',
                                  'rx5power',       'rx6power',
                                  'rx7power',       'rx8power',
                                  'tx1bias',        'tx2bias',
                                  'tx3bias',        'tx4bias',
                                  'tx5bias',        'tx6bias',
                                  'tx7bias',        'tx8bias',
                                  'tx1power',       'tx2power',
                                  'tx3power',       'tx4power',
                                  'tx5power',       'tx6power',
                                  'tx7power',       'tx8power'
                                 ]
            transceiver_dom_info_dict = dict.fromkeys(dom_info_dict_keys, 'N/A')
            return transceiver_dom_info_dict
        else:
            return super(Sfp, self).get_transceiver_bulk_status()

    def get_transceiver_threshold_info(self):
        if self.sfp_type == COPPER_TYPE or self.sfp_type == RJ45_TYPE:
            transceiver_dom_threshold_info_dict = {}
            dom_info_dict_keys = ['temphighalarm',    'temphighwarning',
                                  'templowalarm',     'templowwarning',
                                  'vcchighalarm',     'vcchighwarning',
                                  'vcclowalarm',      'vcclowwarning',
                                  'rxpowerhighalarm', 'rxpowerhighwarning',
                                  'rxpowerlowalarm',  'rxpowerlowwarning',
                                  'txpowerhighalarm', 'txpowerhighwarning',
                                  'txpowerlowalarm',  'txpowerlowwarning',
                                  'txbiashighalarm',  'txbiashighwarning',
                                  'txbiaslowalarm',   'txbiaslowwarning'
                                 ]
            transceiver_dom_threshold_info_dict = dict.fromkeys(dom_info_dict_keys, 'N/A')
            return transceiver_dom_threshold_info_dict
        else:
            return super(Sfp, self).get_transceiver_threshold_info()

    # Below functions use base implementation in 
    # src/sonic_platform_base/sonic_xcvr/sfp_optoe_base.py
    #
    #   def get_model(self):
    #   def get_serial(self):
    #   def get_transceiver_loopback(self):
    #   def is_coherent_module(self):
    #   def get_transceiver_pm(self):
    #   def get_rx_los(self):
    #   def get_tx_fault(self):
    #   def get_tx_disable(self):
    #   def get_tx_disable_channel(self):
    #   def get_temperature(self):
    #   def get_voltage(self):
    #   def get_tx_bias(self):
    #   def get_rx_power(self):
    #   def get_tx_power(self):
    #   def tx_disable_channel(self, channel, disable):
    #   def set_power_override(self, power_override, power_set): 

    def tx_disable(self, tx_disable):
        """
        Disable SFP TX
        Args:
            tx_disable : A Boolean, True to enable tx_disable mode, False to disable
                         tx_disable mode.
        Returns:
            A boolean, True if tx_disable is set successfully, False if not
        """

        if self.sfp_type == COPPER_TYPE or self.sfp_type == RJ45_TYPE:
            return False

        port_offset = self.index-SFP_PORT_START
        sfpstatus=self.sfp_status_get_value(port_offset)

        tx_dis_bit = [0x04, 0x40, 0x04, 0x40, 0x04, 0x40]
        tx_en_bit  = [0xFB, 0xBF, 0xFB, 0xBF, 0xFB, 0xBF]

        if tx_disable == True:
            setbits = sfpstatus | tx_dis_bit[port_offset]
        else:
            setbits = sfpstatus & tx_en_bit[port_offset]

        self.sfp_status_set_value(port_offset, setbits)

        return True
        
    def get_power_override(self):
        """
        Retrieves the power-override status of this SFP
        Returns:
            A Boolean, True if power-override is enabled, False if disabled
        """
        if self.sfp_type == COPPER_TYPE or self.sfp_type == RJ45_TYPE:
            return False
        if self.sfp_type == SFP_TYPE:
            return False

    def get_eeprom_path(self):
        """
        Returns SFP eeprom path
        """
        return self.eeprom_path

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP
        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        if self.sfp_type == COPPER_TYPE or self.sfp_type == RJ45_TYPE:
            return False
        if self.sfp_type == SFP_TYPE:
            return False

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP
        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note  : lpmode can be overridden by set_power_override
        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """
        if self.sfp_type == COPPER_TYPE or self.sfp_type == RJ45_TYPE:
            return False
        if self.sfp_type == SFP_TYPE:
            return False

    def reset(self):
        """
        Reset SFP.
        Returns:
            A boolean, True if successful, False if not
        """
        # RJ45 and SFP ports not resettable
        return False

    def get_error_description(self):
        """
        Retrives the error descriptions of the SFP module

        Returns:
            String that represents the current error descriptions of vendor specific errors
            In case there are multiple errors, they should be joined by '|',
            like: "Bad EEPROM|Unsupported cable"
        """
        if self.sfp_type == COPPER_TYPE or self.sfp_type == RJ45_TYPE:
            return super(Sfp, self).get_error_description()

        if not self.get_presence():
            return self.SFP_STATUS_UNPLUGGED
        else:
            if not os.path.isfile(self.eeprom_path):
                return "EEPROM driver is not attached"

            if self.sfp_type == 'SFP':
                offset = SFP_INFO_OFFSET
            elif self.sfp_type == 'QSFP':
                offset = QSFP_INFO_OFFSET
            elif self.sfp_type == 'QSFP_DD':
                offset = QSFP_DD_PAGE0

            try:
                with open(self.eeprom_path, mode="rb", buffering=0) as eeprom:
                    eeprom.seek(offset)
                    eeprom.read(1)
            except OSError as e:
                return "EEPROM read failed ({})".format(e.strerror)

        return self.SFP_STATUS_OK

    def get_name(self):
        """
        Returns native transceiver type
            Returns:  string - The name of the device
        """
        return RJ45_TYPE if self.index < SFP_PORT_START else "SFP/SFP+/SFP28"

    def get_presence(self):
        """
        Retrieves the presence
        Returns:
            bool: True if is present, False if not
        """
        if self.sfp_type == COPPER_TYPE or self.sfp_type == RJ45_TYPE:
            return False
    
        port_offset = self.index-SFP_PORT_START
        sfpstatus = self.sfp_status_get_value(port_offset)

        if ((port_offset%2) == 1):
            sfpstatus = (sfpstatus >> 4) & 0X01
        else:
            sfpstatus = sfpstatus & 0x01

        if sfpstatus == 0:
            # Present. Turn off tx_disable
            self.tx_disable(False)
            return True

        # Not present. Turn on tx_disable
        self.tx_disable(True)
        return False

    def get_reset_status(self):
        """
        Retrives the reset status of SFP
        """
        reset_status = False
        return reset_status


    def get_status(self):
        """
        Retrieves the operational status of the device
        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence()

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

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device
        Returns:
            integer: The 1-based relative physical position in parent device
        """
        return self.index

