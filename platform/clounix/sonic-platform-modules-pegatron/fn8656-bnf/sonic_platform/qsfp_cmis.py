#!/usr/bin/env python
#
# Name: qsfp.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs
#

import os
import time
import logging
import struct
import datetime
from ctypes import create_string_buffer

try:
    from sonic_platform_base.sfp_base import SfpBase
    from sonic_platform_base.sonic_sfp.qsfp_dd import qsfp_dd_InterfaceId
    from sonic_platform_base.sonic_sfp.qsfp_dd import qsfp_dd_Dom
    from .helper import APIHelper
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

# definitions of the offset and width for values in XCVR info eeprom
XCVR_INTFACE_BULK_OFFSET = 0
XCVR_INTFACE_BULK_WIDTH_QSFP = 20
XCVR_INTFACE_BULK_WIDTH_SFP = 21
XCVR_TYPE_OFFSET = 0
XCVR_TYPE_WIDTH = 1
XCVR_EXT_TYPE_OFFSET = 1
XCVR_EXT_TYPE_WIDTH = 1
XCVR_CONNECTOR_OFFSET = 2
XCVR_CONNECTOR_WIDTH = 1
XCVR_COMPLIANCE_CODE_OFFSET = 3
XCVR_COMPLIANCE_CODE_WIDTH = 8
XCVR_ENCODING_OFFSET = 11
XCVR_ENCODING_WIDTH = 1
XCVR_NBR_OFFSET = 12
XCVR_NBR_WIDTH = 1
XCVR_EXT_RATE_SEL_OFFSET = 13
XCVR_EXT_RATE_SEL_WIDTH = 1
XCVR_CABLE_LENGTH_OFFSET = 14
XCVR_CABLE_LENGTH_WIDTH_QSFP = 5
XCVR_CABLE_LENGTH_WIDTH_SFP = 6
XCVR_VENDOR_NAME_OFFSET = 20
XCVR_VENDOR_NAME_WIDTH = 16
XCVR_VENDOR_OUI_OFFSET = 37
XCVR_VENDOR_OUI_WIDTH = 3
XCVR_VENDOR_PN_OFFSET = 40
XCVR_VENDOR_PN_WIDTH = 16
XCVR_HW_REV_OFFSET = 56
XCVR_HW_REV_WIDTH_OSFP = 2
XCVR_HW_REV_WIDTH_QSFP = 2
XCVR_HW_REV_WIDTH_SFP = 4
XCVR_EXT_SPECIFICATION_COMPLIANCE_OFFSET = 64
XCVR_EXT_SPECIFICATION_COMPLIANCE_WIDTH = 1
XCVR_VENDOR_SN_OFFSET = 68
XCVR_VENDOR_SN_WIDTH = 16
XCVR_VENDOR_DATE_OFFSET = 84
XCVR_VENDOR_DATE_WIDTH = 8
XCVR_DOM_CAPABILITY_OFFSET = 92
XCVR_DOM_CAPABILITY_WIDTH = 2

# definitions of the offset and width for values in XCVR_QSFP_DD info eeprom
XCVR_EXT_TYPE_OFFSET_QSFP_DD = 72
XCVR_EXT_TYPE_WIDTH_QSFP_DD = 2
XCVR_CONNECTOR_OFFSET_QSFP_DD = 75
XCVR_CONNECTOR_WIDTH_QSFP_DD = 1
XCVR_CABLE_LENGTH_OFFSET_QSFP_DD = 74
XCVR_CABLE_LENGTH_WIDTH_QSFP_DD = 1
XCVR_HW_REV_OFFSET_QSFP_DD = 36
XCVR_HW_REV_WIDTH_QSFP_DD = 2
XCVR_VENDOR_DATE_OFFSET_QSFP_DD = 54
XCVR_VENDOR_DATE_WIDTH_QSFP_DD = 8
XCVR_DOM_CAPABILITY_OFFSET_QSFP_DD = 2
XCVR_DOM_CAPABILITY_WIDTH_QSFP_DD = 1
XCVR_MEDIA_TYPE_OFFSET_QSFP_DD = 85
XCVR_MEDIA_TYPE_WIDTH_QSFP_DD = 1
XCVR_FIRST_APPLICATION_LIST_OFFSET_QSFP_DD = 86
XCVR_FIRST_APPLICATION_LIST_WIDTH_QSFP_DD = 32
XCVR_SECOND_APPLICATION_LIST_OFFSET_QSFP_DD = 351
XCVR_SECOND_APPLICATION_LIST_WIDTH_QSFP_DD = 28

# to improve performance we retrieve all eeprom data via a single ethtool command
# in function get_transceiver_info and get_transceiver_bulk_status
# XCVR_INTERFACE_DATA_SIZE stands for the max size to be read
# this variable is only used by get_transceiver_info.
# please be noted that each time some new value added to the function
# we should make sure that it falls into the area
# [XCVR_INTERFACE_DATA_START, XCVR_INTERFACE_DATA_SIZE] or
# adjust XCVR_INTERFACE_MAX_SIZE to contain the new data
# It's same for [QSFP_DOM_BULK_DATA_START, QSFP_DOM_BULK_DATA_SIZE] and
# [SFP_DOM_BULK_DATA_START, SFP_DOM_BULK_DATA_SIZE] which are used by
# get_transceiver_bulk_status
XCVR_INTERFACE_DATA_START = 0
XCVR_INTERFACE_DATA_SIZE = 92
SFP_MODULE_ADDRA2_OFFSET = 256
SFP_MODULE_THRESHOLD_OFFSET = 0
SFP_MODULE_THRESHOLD_WIDTH = 56

QSFP_DOM_BULK_DATA_START = 22
QSFP_DOM_BULK_DATA_SIZE = 36
SFP_DOM_BULK_DATA_START = 96
SFP_DOM_BULK_DATA_SIZE = 10

QSFP_DD_DOM_BULK_DATA_START = 14
QSFP_DD_DOM_BULK_DATA_SIZE = 4

# definitions of the offset for values in OSFP info eeprom
OSFP_TYPE_OFFSET = 0
OSFP_VENDOR_NAME_OFFSET = 129
OSFP_VENDOR_PN_OFFSET = 148
OSFP_HW_REV_OFFSET = 164
OSFP_VENDOR_SN_OFFSET = 166

# definitions of the offset for values in QSFP_DD info eeprom
QSFP_DD_TYPE_OFFSET = 0
QSFP_DD_VENDOR_NAME_OFFSET = 1
QSFP_DD_VENDOR_PN_OFFSET = 20
QSFP_DD_VENDOR_SN_OFFSET = 38
QSFP_DD_VENDOR_OUI_OFFSET = 17

#definitions of the offset and width for values in DOM info eeprom
QSFP_DOM_REV_OFFSET = 1
QSFP_DOM_REV_WIDTH = 1
QSFP_TEMPE_OFFSET = 22
QSFP_TEMPE_WIDTH = 2
QSFP_VOLT_OFFSET = 26
QSFP_VOLT_WIDTH = 2
QSFP_VERSION_COMPLIANCE_OFFSET = 1
QSFP_VERSION_COMPLIANCE_WIDTH = 2
QSFP_CHANNL_MON_OFFSET = 34
QSFP_CHANNL_MON_WIDTH = 16
QSFP_CHANNL_MON_WITH_TX_POWER_WIDTH = 24
QSFP_CHANNL_DISABLE_STATUS_OFFSET = 86
QSFP_CHANNL_DISABLE_STATUS_WIDTH = 1
QSFP_CHANNL_RX_LOS_STATUS_OFFSET = 3
QSFP_CHANNL_RX_LOS_STATUS_WIDTH = 1
QSFP_CHANNL_TX_FAULT_STATUS_OFFSET = 4
QSFP_CHANNL_TX_FAULT_STATUS_WIDTH = 1
QSFP_CONTROL_OFFSET = 86
QSFP_CONTROL_WIDTH = 8
QSFP_MODULE_MONITOR_OFFSET = 0
QSFP_MODULE_MONITOR_WIDTH = 9
QSFP_POWEROVERRIDE_OFFSET = 93
QSFP_POWEROVERRIDE_WIDTH = 1
QSFP_POWEROVERRIDE_BIT = 0
QSFP_POWERSET_BIT = 1
QSFP_OPTION_VALUE_OFFSET = 192
QSFP_OPTION_VALUE_WIDTH = 4

QSFP_MODULE_THRESHOLD_OFFSET = 128
QSFP_MODULE_THRESHOLD_WIDTH = 24
QSFP_CHANNL_THRESHOLD_OFFSET = 176
QSFP_CHANNL_THRESHOLD_WIDTH = 24

CMIS_LOWER_PAGE_START   = 0
CMIS_LOWER_PAGE_REVISION_COMPLIANCE_OFFSET = 1
CMIS_LOWER_PAGE_FLAT_MEM = 2
CMIS_LOWER_PAGE_MODULE_STATE_OFFSET = 3

CMIS_UPPER_PAGE01_START = 128
CMIS_UPPER_PAGE01_TX_DISABLE_IMPLEMENTED = 155


CMIS_UPPER_PAGE02_START = 384
CMIS_UPPER_PAGE10_START = 2048
CMIS_UPPER_PAGE10_DATA_PATH_DEINIT_OFFSET = 128
CMIS_UPPER_PAGE10_TX_DISABLE_OFFSET = 130
CMIS_UPPER_PAGE10_STAGED0_APPLY_DATA_PATH_INIT_OFFSET = 143
CMIS_UPPER_PAGE10_STAGED0_APSEL_CONTROL_OFFSET = 145

CMIS_UPPER_PAGE11_START = 2176
CMIS_UPPER_PAGE11_DATA_PATH_STATE_OFFSET = 128
CMIS_UPPER_PAGE11_CONFIG_ERROR_CODE_OFFSET = 202

SFP_TEMPE_OFFSET = 96
SFP_TEMPE_WIDTH = 2
SFP_VOLT_OFFSET = 98
SFP_VOLT_WIDTH = 2
SFP_CHANNL_MON_OFFSET = 100
SFP_CHANNL_MON_WIDTH = 6


SFP_CHANNL_STATUS_OFFSET = 110
SFP_CHANNL_STATUS_WIDTH = 1

SFP_CHANNL_THRESHOLD_OFFSET = 112
SFP_CHANNL_THRESHOLD_WIDTH = 2
SFP_STATUS_CONTROL_OFFSET = 110
SFP_STATUS_CONTROL_WIDTH = 1
SFP_TX_DISABLE_HARD_BIT = 7
SFP_TX_DISABLE_SOFT_BIT = 6

QSFP_DD_TEMPE_OFFSET = 14
QSFP_DD_TEMPE_WIDTH = 2
QSFP_DD_VOLT_OFFSET = 16
QSFP_DD_VOLT_WIDTH = 2
QSFP_DD_TX_BIAS_OFFSET = 170
QSFP_DD_TX_BIAS_WIDTH = 16
QSFP_DD_RX_POWER_OFFSET = 186
QSFP_DD_RX_POWER_WIDTH = 16
QSFP_DD_TX_POWER_OFFSET = 26
QSFP_DD_TX_POWER_WIDTH = 16
QSFP_DD_CHANNL_MON_OFFSET = 154
QSFP_DD_CHANNL_MON_WIDTH = 48
QSFP_DD_CHANNL_DISABLE_STATUS_OFFSET = 86
QSFP_DD_CHANNL_DISABLE_STATUS_WIDTH = 1
QSFP_DD_CHANNL_RX_LOS_STATUS_OFFSET = 19
QSFP_DD_CHANNL_RX_LOS_STATUS_WIDTH = 1
QSFP_DD_CHANNL_TX_FAULT_STATUS_OFFSET = 7
QSFP_DD_CHANNL_TX_FAULT_STATUS_WIDTH = 1
QSFP_DD_MODULE_THRESHOLD_OFFSET = 128
QSFP_DD_MODULE_THRESHOLD_WIDTH = 72
QSFP_DD_CHANNL_STATUS_OFFSET = 26
QSFP_DD_CHANNL_STATUS_WIDTH = 1
DOM_OFFSET = 0
# identifier value of xSFP module which is in the first byte of the EEPROM
# if the identifier value falls into SFP_TYPE_CODE_LIST the module is treated as a SFP module and parsed according to 8472
# for QSFP_TYPE_CODE_LIST the module is treated as a QSFP module and parsed according to 8436/8636
# Originally the type (SFP/QSFP) of each module is determined according to the SKU dictionary
# where the type of each FP port is defined. The content of EEPROM is parsed according to its type.
# However, sometimes the SFP module can be fit in an adapter and then pluged into a QSFP port.
# In this case the EEPROM content is in format of SFP but parsed as QSFP, causing failure.
# To resolve that issue the type field of the xSFP module is also fetched so that we can know exectly what type the
# module is. Currently only the following types are recognized as SFP/QSFP module.
# Meanwhile, if the a module's identifier value can't be recognized, it will be parsed according to the SKU dictionary.
# This is because in the future it's possible that some new identifier value which is not regonized but backward compatible
# with the current format and by doing so it can be parsed as much as possible.
SFP_TYPE_CODE_LIST = [
    '03' # SFP/SFP+/SFP28
]
QSFP_TYPE_CODE_LIST = [
    '0d', # QSFP+ or later
    '11' # QSFP28 or later
]
QSFP_DD_TYPE_CODE_LIST = [
    '18' # QSFP-DD Double Density 8X Pluggable Transceiver
    '1b' # QSFP-DD Double Density 8X Pluggable Transceiver
]

qsfp_cable_length_tup = ('Length(km)', 'Length OM3(2m)',
                         'Length OM2(m)', 'Length OM1(m)',
                         'Length Cable Assembly(m)')

sfp_cable_length_tup = ('LengthSMFkm-UnitsOfKm', 'LengthSMF(UnitsOf100m)',
                        'Length50um(UnitsOf10m)', 'Length62.5um(UnitsOfm)',
                        'LengthCable(UnitsOfm)', 'LengthOM3(UnitsOf10m)')

sfp_compliance_code_tup = ('10GEthernetComplianceCode', 'InfinibandComplianceCode',
                            'ESCONComplianceCodes', 'SONETComplianceCodes',
                            'EthernetComplianceCodes','FibreChannelLinkLength',
                            'FibreChannelTechnology', 'SFP+CableTechnology',
                            'FibreChannelTransmissionMedia','FibreChannelSpeed')

qsfp_compliance_code_tup = ('10/40G Ethernet Compliance Code', 'SONET Compliance codes',
                            'SAS/SATA compliance codes', 'Gigabit Ethernet Compliant codes',
                            'Fibre Channel link length/Transmitter Technology',
                            'Fibre Channel transmission media', 'Fibre Channel Speed')

SFP_TYPE = "SFP"
QSFP_TYPE = "QSFP"
OSFP_TYPE = "OSFP"
QSFP_DD_TYPE = "QSFP_DD"

TRANSCEIVER_PATH = '/sys/switch/transceiver/'

MODULE_STATE = {
    1: 'ModuleLowPwr',
    2: 'ModulePwrUp',
    3: 'ModuleReady',
    4: 'ModulePwrDn',
    5: 'ModuleFault'
}
DATAPATH_STATE = {
    1: 'DataPathDeactivated',
    2: 'DataPathInit',
    3: 'DataPathDeinit',
    4: 'DataPathActivated',
    5: 'DataPathTxTurnOn',
    6: 'DataPathTxTurnOff',
    7: 'DataPathInitialized',
}
CONFIG_STATUS = {
    0: 'ConfigUndefined',
    1: 'ConfigSuccess',
    2: 'ConfigRejected',
    3: 'ConfigRejectedInvalidAppSel',
    4: 'ConfigRejectedInvalidDataPath',
    5: 'ConfigRejectedInvalidSI',
    6: 'ConfigRejectedLaneInUse',
    7: 'ConfigRejectedPartialDataPath',
    12: 'ConfigInProgress',
}

CMIS_STATE_UNKNOWN   = 'UNKNOWN'
CMIS_STATE_INSERTED  = 'INSERTED'
CMIS_STATE_DP_DEINIT = 'DP_DEINIT'
CMIS_STATE_AP_CONF   = 'AP_CONFIGURED'
CMIS_STATE_DP_INIT   = 'DP_INIT'
CMIS_STATE_DP_TXON   = 'DP_TXON'
CMIS_STATE_READY     = 'READY'
CMIS_STATE_REMOVED   = 'REMOVED'
CMIS_STATE_FAILED    = 'FAILED'

CMIS_MAX_RETRIES     = 3
CMIS_DEF_EXPIRED     = 1 # seconds, default expiration time
class QSfp_CMIS(SfpBase):

    port_start = 0
    port_end = 55
    NUM_CHANNELS = 8

    dom_supported = True
    dom_temp_supported = True
    dom_volt_supported = True
    dom_rx_power_supported = True
    dom_tx_power_supported = True
    dom_tx_disable_supported = True
    calibration = 1

    def __init__(self, index,qsfp_conf):
        self.__index = index
        self.__conf = qsfp_conf
        self.__api_helper = APIHelper()

        self.__attr_path_prefix = '/sys/switch/transceiver/eth{}/'.format(self.__index+1)

        self.__platform = "x86_64-pegatron_fn8656-bnf-r0"
        self.__hwsku    = "fn8656-bnf"

        self.__presence_attr = None
        self.__eeprom_path = None

        if self.__index in range(0, self.port_end + 1):
            self.__eeprom_path = TRANSCEIVER_PATH+'eth{}/'.format(self.__index+1)+'eeprom'
            self.__presence_attr = TRANSCEIVER_PATH+'eth{}/'.format(self.__index+1)+'present'
            self.__reset_attr = TRANSCEIVER_PATH+'eth{}/'.format(self.__index+1)+'reset'
            self.__lpmode_attr = TRANSCEIVER_PATH+'eth{}/'.format(self.__index+1)+'lpmode'
        SfpBase.__init__(self)

        self.cmis_state = CMIS_STATE_UNKNOWN 
        self.cmis_retries = 0
        self.cmis_expired = None

        self._detect_sfp_type(QSFP_DD_TYPE)
        self.info_dict_keys = ['type', 'hardware_rev', 'serial', 'manufacturer',
            'model', 'connector', 'encoding', 'ext_identifier',
            'ext_rateselect_compliance', 'cable_type', 'cable_length',
            'nominal_bit_rate', 'specification_compliance', 'vendor_date', 'vendor_oui', 'application_advertisement']

    def _detect_sfp_type(self, sfp_type):
        sfp_type = QSFP_DD_TYPE

        if not self.get_presence():
            self.sfp_type = sfp_type
            return

        eeprom_raw = []

        eeprom_raw = self._read_eeprom_specific_bytes(
            XCVR_TYPE_OFFSET, XCVR_TYPE_WIDTH)
        if eeprom_raw:
            if eeprom_raw[0] in SFP_TYPE_CODE_LIST:
                self.sfp_type = SFP_TYPE
            elif eeprom_raw[0] in QSFP_TYPE_CODE_LIST:
                self.sfp_type = QSFP_TYPE
            elif eeprom_raw[0] in QSFP_DD_TYPE_CODE_LIST:
                self.sfp_type = QSFP_DD_TYPE
            else:
                self.sfp_type = sfp_type
        else:
            self.sfp_type = sfp_type

    def __get_attr_value(self, attr_path):

        retval = 'ERR'
        if (not os.path.isfile(attr_path)):
            return retval

        try:
            with open(attr_path, 'r') as fd:
                retval = fd.read()
        except Exception as error:
            logging.error("Unable to open ", attr_path, " file !")

        retval = retval.rstrip(' \t\n\r')
        return retval

    def __is_host(self):
        return os.system("docker > /dev/null 2>&1") == 0

    def __get_path_to_port_config_file(self):
        host_platform_root_path = '/usr/share/sonic/device'
        docker_hwsku_path = '/usr/share/sonic/hwsku'

        host_platform_path = "/".join([host_platform_root_path, self.__platform])
        hwsku_path = "/".join([host_platform_path, self.__hwsku]) if self.__is_host() else docker_hwsku_path

        return "/".join([hwsku_path, "port_config.ini"])

    def _read_eeprom_specific_bytes(self, offset, num_bytes):
        """
        read register from qsfp eeprom

        Args:
            offset:
                Integer, lower page : 0 - 127,upper page: page*128 + upper offset
            num_bytes:
                Interger
        Returns:
            a character list 
        """
        sysfsfile_eeprom = None
        eeprom_raw = []

        for i in range(0, num_bytes):
            eeprom_raw.append("0x00")

        sysfs_eeprom_path = self.__eeprom_path
        try:
            sysfsfile_eeprom = open(sysfs_eeprom_path, mode="rb", buffering=0)
            sysfsfile_eeprom.seek(offset)
            raw = sysfsfile_eeprom.read(num_bytes)
            raw_len = len(raw)
            for n in range(0, raw_len):
                eeprom_raw[n] = hex(raw[n])[2:].zfill(2)
        #except BaseException:
        #    pass
        finally:
            if sysfsfile_eeprom:
                sysfsfile_eeprom.close()

        return eeprom_raw
        
    def _write_eeprom_specific_bytes(self, offset, data):
        """
        write one byte to qsfp eeprom

        Args:
            offset:
                Integer, lower page : 0 - 127,upper page: page*128 + upper offset
            data:
                Interger

        Returns:
            Boolean, true if success otherwise false
        """
        sysfs_eeprom_path = self.__eeprom_path

        buffer = create_string_buffer(1)
        buffer[0] = struct.pack('B',data)
        try:
            sysfsfile_eeprom = open(sysfs_eeprom_path, mode='rb+', buffering=0)
            sysfsfile_eeprom.seek(offset)
            sysfsfile_eeprom.write(buffer[0])
            sysfsfile_eeprom.close()
        except OSError as e:
            print(" %s" % str(e), True)
            return False
        return True

    def read_eeprom(self, offset, num_bytes):
        sysfsfile_eeprom = None
        eeprom_read_bytes = []
        eeprom_raw = []
        raw = []

        for i in range(0, num_bytes):
            eeprom_raw.append("0x00")

        sysfs_eeprom_path = self.__eeprom_path
        try:
            sysfsfile_eeprom = open(sysfs_eeprom_path, mode="rb", buffering=0)
            sysfsfile_eeprom.seek(offset)
            raw = sysfsfile_eeprom.read(num_bytes)
            raw_len = len(raw)
            eeprom_read_bytes = bytearray(raw)
            for n in range(0, raw_len):
                eeprom_raw[n] = hex(raw[n])[2:].zfill(2)
        except Exception:
            pass
        finally:
            if sysfsfile_eeprom:
                sysfsfile_eeprom.close()

        return eeprom_read_bytes

    def write_eeprom(self, offset, num_bytes, write_buffer):
        try:
            with open(self.__eeprom_path, mode='r+b', buffering=0) as f:
                f.seek(offset)
                f.write(write_buffer[0:num_bytes])
                f.close()
        except (OSError, IOError):
            return False
        return True

    def _convert_string_to_num(self, value_str):
        if "-inf" in value_str:
            return 'N/A'
        elif "Unknown" in value_str:
            return 'N/A'
        elif 'dBm' in value_str:
            t_str = value_str.rstrip('dBm')
            return float(t_str)
        elif 'mA' in value_str:
            t_str = value_str.rstrip('mA')
            return float(t_str)
        elif 'C' in value_str:
            t_str = value_str.rstrip('C')
            return float(t_str)
        elif 'Volts' in value_str:
            t_str = value_str.rstrip('Volts')
            return float(t_str)
        else:
            return 'N/A'

    def _dom_capability_detect(self):
        if not self.get_presence():
            self.dom_supported = False
            self.dom_temp_supported = False
            self.dom_volt_supported = False
            self.dom_rx_power_supported = False
            self.dom_tx_bias_power_supported = False
            self.dom_tx_power_supported = False
            self.calibration = 0
            return

        sfpi_obj = qsfp_dd_InterfaceId()
        if sfpi_obj is None:
            self.dom_supported = False

        offset = 0
        # two types of QSFP-DD cable types supported: Copper and Optical.
        qsfp_dom_capability_raw = self._read_eeprom_specific_bytes((offset + XCVR_DOM_CAPABILITY_OFFSET_QSFP_DD), XCVR_DOM_CAPABILITY_WIDTH_QSFP_DD)
        if qsfp_dom_capability_raw is not None:
            self.dom_temp_supported = True
            self.dom_volt_supported = True
            dom_capability = sfpi_obj.parse_dom_capability(qsfp_dom_capability_raw, 0)
            if dom_capability['data']['Flat_MEM']['value'] == 'Off':
                self.dom_supported = True
                self.second_application_list = True
                self.dom_rx_power_supported = True
                self.dom_tx_power_supported = True
                self.dom_tx_bias_power_supported = True
                self.dom_thresholds_supported = True
                self.dom_rx_tx_power_bias_supported = True
            else:
                self.dom_supported = False
                self.second_application_list = False
                self.dom_rx_power_supported = False
                self.dom_tx_power_supported = False
                self.dom_tx_bias_power_supported = False
                self.dom_thresholds_supported = False
                self.dom_rx_tx_power_bias_supported = False
        else:
            self.dom_supported = False
            self.dom_temp_supported = False
            self.dom_volt_supported = False
            self.dom_rx_power_supported = False
            self.dom_tx_power_supported = False
            self.dom_tx_bias_power_supported = False
            self.dom_thresholds_supported = False
            self.dom_rx_tx_power_bias_supported = False

##############################################
# Device methods
##############################################

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return self.__conf[self.__index]['name']

    def get_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        presence = False

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'present')
        if (attr_rv != None):
            attr_rv = int(attr_rv, 16)
            if ((attr_rv & 0x1)  == 1):
                presence = True
        return presence

    def get_all_presence(self):
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """

        presence_bit = 0
        attr_rv = self.__api_helper.read_one_line_file(TRANSCEIVER_PATH + 'present')
        if (attr_rv != None):
            presence_bit = int(attr_rv, 16)
        return presence_bit

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        transceiver_info_dict = self.get_transceiver_info()
        return transceiver_info_dict.get("model", "N/A")

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        transceiver_info_dict = self.get_transceiver_info()
        return transceiver_info_dict.get("serial", "N/A")

    def get_status(self):
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self.get_presence() and not self.get_reset_status()

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return self.__index

    def is_replaceable(self):
        """
        Indicate whether Chassis is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True
##############################################
# SFP methods
##############################################

    def get_transceiver_type(self,origin_transceiver_type):
        transceiver_type = origin_transceiver_type
        if transceiver_type != 'Unknown':
            return transceiver_type

        type_of_transceiver = {
            '00': 'Unknown or unspecified',
            '01': 'GBIC',
            '02': 'Module/connector soldered to motherboard',
            '03': 'SFP/SFP+/SFP28',
            '04': '300 pin XBI',
            '05': 'XENPAK',
            '06': 'XFP',
            '07': 'XFF',
            '08': 'XFP-E',
            '09': 'XPAK',
            '0a': 'X2',
            '0b': 'DWDM-SFP/SFP+',
            '0c': 'QSFP',
            '0d': 'QSFP+ or later',
            '0e': 'CXP or later',
            '0f': 'Shielded Mini Multilane HD 4X',
            '10': 'Shielded Mini Multilane HD 8X',
            '11': 'QSFP28 or later',
            '12': 'CXP2 (aka CXP28) or later',
            '13': 'CDFP (Style 1/Style2)',
            '14': 'Shielded Mini Multilane HD 4X Fanout Cable',
            '15': 'Shielded Mini Multilane HD 8X Fanout Cable',
            '16': 'CDFP (Style 3)',
            '17': 'microQSFP',
            '18': 'QSFP-DD Double Density 8X Pluggable Transceiver',
            '19': 'OSFP 8X Pluggable Transceiver',
            '1a': 'SFP-DD Double Density 2X Pluggable Transceiver',
            '1b': 'DSFP Dual Small Form Factor Pluggable Transceiver',
            '1c': 'x4 MiniLink/OcuLink',
            '1d': 'x8 MiniLink',
            '1e': 'QSFP+ or later with CMIS'
        }

        offset = 128
        sfp_type_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_TYPE_OFFSET), XCVR_TYPE_WIDTH)
        transceiver_type = type_of_transceiver.get(sfp_type_raw[0])
        return transceiver_type

    def get_transceiver_type_abbr_name(self,origin_transceiver_type):
        transceiver_type_abbr_name = origin_transceiver_type
        if transceiver_type_abbr_name != 'Unknown':
            return transceiver_type_abbr_name

        type_abbrv_name = {
            '00': 'Unknown',
            '01': 'GBIC',
            '02': 'Soldered',
            '03': 'SFP',
            '04': 'XBI300',
            '05': 'XENPAK',
            '06': 'XFP',
            '07': 'XFF',
            '08': 'XFP-E',
            '09': 'XPAK',
            '0a': 'X2',
            '0b': 'DWDM-SFP',
            '0c': 'QSFP',
            '0d': 'QSFP+',
            '0e': 'CXP',
            '0f': 'HD4X',
            '10': 'HD8X',
            '11': 'QSFP28',
            '12': 'CXP2',
            '13': 'CDFP-1/2',
            '14': 'HD4X-Fanout',
            '15': 'HD8X-Fanout',
            '16': 'CDFP-3',
            '17': 'MicroQSFP',
            '18': 'QSFP-DD',
            '19': 'OSFP-8X',
            '1a': 'SFP-DD',
            '1b': 'DSFP',
            '1c': 'Link-x4',
            '1d': 'Link-x8',
            '1e': 'QSFP+C'
        }
        offset = 128
        sfp_type_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_TYPE_OFFSET), XCVR_TYPE_WIDTH)
        transceiver_type_abbr_name = type_abbrv_name.get(sfp_type_raw[0])
        return transceiver_type_abbr_name



    def get_transceiver_info(self):
        """
        Retrieves transceiver info of this SFP

        Returns:
            A dict which contains following keys/values :
        ========================================================================
        keys                       |Value Format   |Information
        ---------------------------|---------------|----------------------------
        type                       |1*255VCHAR     |type of SFP
        hardware_rev                |1*255VCHAR     |hardware version of SFP
        serial                  |1*255VCHAR     |serial number of the SFP
        manufacturer            |1*255VCHAR     |SFP vendor name
        model                  |1*255VCHAR     |SFP model name
        connector                  |1*255VCHAR     |connector information
        encoding                   |1*255VCHAR     |encoding information
        ext_identifier             |1*255VCHAR     |extend identifier
        ext_rateselect_compliance  |1*255VCHAR     |extended rateSelect compliance
        cable_length               |INT            |cable length in m
        mominal_bit_rate           |INT            |nominal bit rate by 100Mbs
        specification_compliance   |1*255VCHAR     |specification compliance
        vendor_date                |1*255VCHAR     |vendor date
        vendor_oui                 |1*255VCHAR     |vendor OUI
        ========================================================================
        """
        transceiver_info_dict = {}
        transceiver_info_dict = dict.fromkeys(self.info_dict_keys, 'N/A')
        transceiver_info_dict['specification_compliance'] = '{}'

        if not self.get_presence():
            return transceiver_info_dict

        self._detect_sfp_type(self.sfp_type)
        self._dom_capability_detect()

        sfpi_obj = qsfp_dd_InterfaceId()
        if not sfpi_obj:
            return None

        offset = 128
        sfp_type_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_TYPE_OFFSET), XCVR_TYPE_WIDTH)
        sfp_type_data = sfpi_obj.parse_sfp_type(sfp_type_raw, 0)
        sfp_type_abbrv_name = sfpi_obj.parse_sfp_type_abbrv_name(sfp_type_raw, 0)

        sfp_vendor_name_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_VENDOR_NAME_OFFSET), XCVR_VENDOR_NAME_WIDTH)
        sfp_vendor_name_data = sfpi_obj.parse_vendor_name(sfp_vendor_name_raw, 0)

        sfp_vendor_pn_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_VENDOR_PN_OFFSET), XCVR_VENDOR_PN_WIDTH)
        sfp_vendor_pn_data = sfpi_obj.parse_vendor_pn(sfp_vendor_pn_raw, 0)

        sfp_vendor_rev_raw = self._read_eeprom_specific_bytes((offset + XCVR_HW_REV_OFFSET_QSFP_DD), XCVR_HW_REV_WIDTH_QSFP_DD)
        sfp_vendor_rev_data = sfpi_obj.parse_vendor_rev(sfp_vendor_rev_raw, 0)

        sfp_vendor_sn_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_VENDOR_SN_OFFSET), XCVR_VENDOR_SN_WIDTH)
        sfp_vendor_sn_data = sfpi_obj.parse_vendor_sn(sfp_vendor_sn_raw, 0)

        sfp_vendor_oui_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_VENDOR_OUI_OFFSET), XCVR_VENDOR_OUI_WIDTH)
        sfp_vendor_oui_data = sfpi_obj.parse_vendor_oui(sfp_vendor_oui_raw, 0)

        sfp_vendor_date_raw = self._read_eeprom_specific_bytes((offset + XCVR_VENDOR_DATE_OFFSET_QSFP_DD), XCVR_VENDOR_DATE_WIDTH_QSFP_DD)
        sfp_vendor_date_data = sfpi_obj.parse_vendor_date(sfp_vendor_date_raw, 0)

        sfp_connector_raw = self._read_eeprom_specific_bytes((offset + XCVR_CONNECTOR_OFFSET_QSFP_DD), XCVR_CONNECTOR_WIDTH_QSFP_DD)
        sfp_connector_data = sfpi_obj.parse_connector(sfp_connector_raw, 0)

        sfp_ext_identifier_raw = self._read_eeprom_specific_bytes((offset + XCVR_EXT_TYPE_OFFSET_QSFP_DD), XCVR_EXT_TYPE_WIDTH_QSFP_DD)
        sfp_ext_identifier_data = sfpi_obj.parse_ext_iden(sfp_ext_identifier_raw, 0)

        sfp_cable_len_raw = self._read_eeprom_specific_bytes((offset + XCVR_CABLE_LENGTH_OFFSET_QSFP_DD), XCVR_CABLE_LENGTH_WIDTH_QSFP_DD)
        sfp_cable_len_data = sfpi_obj.parse_cable_len(sfp_cable_len_raw, 0)

        sfp_media_type_raw = self._read_eeprom_specific_bytes(XCVR_MEDIA_TYPE_OFFSET_QSFP_DD, XCVR_MEDIA_TYPE_WIDTH_QSFP_DD)
        if sfp_media_type_raw is not None:
            sfp_media_type_dict = sfpi_obj.parse_media_type(sfp_media_type_raw, 0)
            if sfp_media_type_dict is None:
                return None

            host_media_list = ""
            sfp_application_type_first_list = self._read_eeprom_specific_bytes((XCVR_FIRST_APPLICATION_LIST_OFFSET_QSFP_DD), XCVR_FIRST_APPLICATION_LIST_WIDTH_QSFP_DD)
            if self.second_application_list:
                possible_application_count = 15
                sfp_application_type_second_list = self._read_eeprom_specific_bytes((XCVR_SECOND_APPLICATION_LIST_OFFSET_QSFP_DD), XCVR_SECOND_APPLICATION_LIST_WIDTH_QSFP_DD)
                if sfp_application_type_first_list is not None and sfp_application_type_second_list is not None:
                    sfp_application_type_list = sfp_application_type_first_list + sfp_application_type_second_list
                else:
                    return None
            else:
                possible_application_count = 8
                if sfp_application_type_first_list is not None:
                    sfp_application_type_list = sfp_application_type_first_list
                else:
                    return None

            for i in range(0, possible_application_count):
                if sfp_application_type_list[i * 4] == 'ff':
                    break
                host_electrical, media_interface = sfpi_obj.parse_application(sfp_media_type_dict, sfp_application_type_list[i * 4], sfp_application_type_list[i * 4 + 1])
                host_media_list = host_media_list + host_electrical + ' - ' + media_interface + '\n\t\t\t\t   '
        else:
            return None

        transceiver_info_dict['manufacturer'] = str(sfp_vendor_name_data['data']['Vendor Name']['value'])
        transceiver_info_dict['model'] = str(sfp_vendor_pn_data['data']['Vendor PN']['value'])
        transceiver_info_dict['hardware_rev'] = str(sfp_vendor_rev_data['data']['Vendor Rev']['value'])
        transceiver_info_dict['serial'] = sfp_vendor_sn_data['data']['Vendor SN']['value']
        transceiver_info_dict['vendor_oui'] = str(sfp_vendor_oui_data['data']['Vendor OUI']['value'])
        transceiver_info_dict['vendor_date'] = str(sfp_vendor_date_data['data']['VendorDataCode(YYYY-MM-DD Lot)']['value'])
        transceiver_info_dict['connector'] = str(sfp_connector_data['data']['Connector']['value'])
        transceiver_info_dict['encoding'] = "Not supported for CMIS cables"
        transceiver_info_dict['ext_identifier'] = str(sfp_ext_identifier_data['data']['Extended Identifier']['value'])
        transceiver_info_dict['ext_rateselect_compliance'] = "Not supported for CMIS cables"
        transceiver_info_dict['specification_compliance'] = "Not supported for CMIS cables"
        transceiver_info_dict['cable_type'] = "Length Cable Assembly(m)"
        transceiver_info_dict['cable_length'] = str(sfp_cable_len_data['data']['Length Cable Assembly(m)']['value'])
        transceiver_info_dict['nominal_bit_rate'] = "Not supported for CMIS cables"
        transceiver_info_dict['application_advertisement'] = host_media_list
        transceiver_info_dict['type'] = self.get_transceiver_type(str(sfp_type_data['data']['type']['value']))
        transceiver_info_dict['type_abbrv_name'] = self.get_transceiver_type_abbr_name(str(sfp_type_abbrv_name['data']['type_abbrv_name']['value']))

        #media_settings.json only match media_type
        if sfp_media_type_raw[0] == '03':
            transceiver_info_dict['specification_compliance'] = "{'media_interface':'DAC'}"
            if transceiver_info_dict['type'] == "QSFP-DD Double Density 8X Pluggable Transceiver":
                transceiver_info_dict['specification_compliance'] = "media_interface: DAC"
        elif sfp_media_type_raw[0] == '04' or sfp_media_type_raw[0] == '01' or sfp_media_type_raw[0] == '02':
            transceiver_info_dict['specification_compliance'] = "{'media_interface':'AOC'}"
            if transceiver_info_dict['type'] == "QSFP-DD Double Density 8X Pluggable Transceiver":
                transceiver_info_dict['specification_compliance'] = "media_interface: AOC"

        return transceiver_info_dict

    def get_transceiver_bulk_status(self):
        """
        Retrieves transceiver bulk status of this SFP

        Returns:
            A dict which contains following keys/values :
        ========================================================================
        keys                       |Value Format   |Information
        ---------------------------|---------------|----------------------------
        rx_los                     |BOOLEAN        |RX loss-of-signal status, True if has RX los, False if not.
        tx_fault                   |BOOLEAN        |TX fault status, True if has TX fault, False if not.
        reset_status               |BOOLEAN        |reset status, True if SFP in reset, False if not.
        lp_mode                    |BOOLEAN        |low power mode status, True in lp mode, False if not.
        tx_disable                 |BOOLEAN        |TX disable status, True TX disabled, False if not.
        tx_disabled_channel        |HEX            |disabled TX channels in hex, bits 0 to 3 represent channel 0
                                   |               |to channel 3.
        temperature                |INT            |module temperature in Celsius
        voltage                    |INT            |supply voltage in mV
        tx<n>bias                  |INT            |TX Bias Current in mA, n is the channel number,
                                   |               |for example, tx2bias stands for tx bias of channel 2.
        rx<n>power                 |INT            |received optical power in mW, n is the channel number,
                                   |               |for example, rx2power stands for rx power of channel 2.
        tx<n>power                 |INT            |TX output power in mW, n is the channel number,
                                   |               |for example, tx2power stands for tx power of channel 2.
        ========================================================================
        """
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

        if not self.get_presence():
            return {}

        self._dom_capability_detect()

        offset = 0
        sfpd_obj = qsfp_dd_Dom()
        if sfpd_obj is None:
            return transceiver_dom_info_dict

        dom_data_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_DOM_BULK_DATA_START), QSFP_DD_DOM_BULK_DATA_SIZE)
        if dom_data_raw is None:
            return transceiver_dom_info_dict

        if self.dom_temp_supported:
            start = QSFP_DD_TEMPE_OFFSET - QSFP_DD_DOM_BULK_DATA_START
            end = start + QSFP_DD_TEMPE_WIDTH
            dom_temperature_data = sfpd_obj.parse_temperature(dom_data_raw[start : end], 0)
            temp = self._convert_string_to_num(dom_temperature_data['data']['Temperature']['value'])
            if temp is not None:
                transceiver_dom_info_dict['temperature'] = temp

        if self.dom_volt_supported:
            start = QSFP_DD_VOLT_OFFSET - QSFP_DD_DOM_BULK_DATA_START
            end = start + QSFP_DD_VOLT_WIDTH
            dom_voltage_data = sfpd_obj.parse_voltage(dom_data_raw[start : end], 0)
            volt = self._convert_string_to_num(dom_voltage_data['data']['Vcc']['value'])
            if volt is not None:
                transceiver_dom_info_dict['voltage'] = volt

        if self.dom_rx_tx_power_bias_supported:
            # page 11h
            offset = CMIS_UPPER_PAGE11_START
            dom_data_raw = self._read_eeprom_specific_bytes(offset + QSFP_DD_CHANNL_MON_OFFSET, QSFP_DD_CHANNL_MON_WIDTH)
            if dom_data_raw is None:
                return transceiver_dom_info_dict
            dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params(dom_data_raw, 0)

            if self.dom_tx_power_supported:
                transceiver_dom_info_dict['tx1power'] = str(self._convert_string_to_num(dom_channel_monitor_data['data']['TX1Power']['value']))
                transceiver_dom_info_dict['tx2power'] = str(self._convert_string_to_num(dom_channel_monitor_data['data']['TX2Power']['value']))
                transceiver_dom_info_dict['tx3power'] = str(self._convert_string_to_num(dom_channel_monitor_data['data']['TX3Power']['value']))
                transceiver_dom_info_dict['tx4power'] = str(self._convert_string_to_num(dom_channel_monitor_data['data']['TX4Power']['value']))
                transceiver_dom_info_dict['tx5power'] = str(self._convert_string_to_num(dom_channel_monitor_data['data']['TX5Power']['value']))
                transceiver_dom_info_dict['tx6power'] = str(self._convert_string_to_num(dom_channel_monitor_data['data']['TX6Power']['value']))
                transceiver_dom_info_dict['tx7power'] = str(self._convert_string_to_num(dom_channel_monitor_data['data']['TX7Power']['value']))
                transceiver_dom_info_dict['tx8power'] = str(self._convert_string_to_num(dom_channel_monitor_data['data']['TX8Power']['value']))

            if self.dom_rx_power_supported:
                transceiver_dom_info_dict['rx1power'] = str(self._convert_string_to_num(dom_channel_monitor_data['data']['RX1Power']['value']))
                transceiver_dom_info_dict['rx2power'] = str(self._convert_string_to_num(dom_channel_monitor_data['data']['RX2Power']['value']))
                transceiver_dom_info_dict['rx3power'] = str(self._convert_string_to_num(dom_channel_monitor_data['data']['RX3Power']['value']))
                transceiver_dom_info_dict['rx4power'] = str(self._convert_string_to_num(dom_channel_monitor_data['data']['RX4Power']['value']))
                transceiver_dom_info_dict['rx5power'] = str(self._convert_string_to_num(dom_channel_monitor_data['data']['RX5Power']['value']))
                transceiver_dom_info_dict['rx6power'] = str(self._convert_string_to_num(dom_channel_monitor_data['data']['RX6Power']['value']))
                transceiver_dom_info_dict['rx7power'] = str(self._convert_string_to_num(dom_channel_monitor_data['data']['RX7Power']['value']))
                transceiver_dom_info_dict['rx8power'] = str(self._convert_string_to_num(dom_channel_monitor_data['data']['RX8Power']['value']))

            if self.dom_tx_bias_power_supported:
                transceiver_dom_info_dict['tx1bias'] = str(dom_channel_monitor_data['data']['TX1Bias']['value'])
                transceiver_dom_info_dict['tx2bias'] = str(dom_channel_monitor_data['data']['TX2Bias']['value'])
                transceiver_dom_info_dict['tx3bias'] = str(dom_channel_monitor_data['data']['TX3Bias']['value'])
                transceiver_dom_info_dict['tx4bias'] = str(dom_channel_monitor_data['data']['TX4Bias']['value'])
                transceiver_dom_info_dict['tx5bias'] = str(dom_channel_monitor_data['data']['TX5Bias']['value'])
                transceiver_dom_info_dict['tx6bias'] = str(dom_channel_monitor_data['data']['TX6Bias']['value'])
                transceiver_dom_info_dict['tx7bias'] = str(dom_channel_monitor_data['data']['TX7Bias']['value'])
                transceiver_dom_info_dict['tx8bias'] = str(dom_channel_monitor_data['data']['TX8Bias']['value'])

        return transceiver_dom_info_dict

    def get_transceiver_threshold_info(self):
        """
        Retrieves transceiver threshold info of this SFP

        Returns:
            A dict which contains following keys/values :
        ========================================================================
        keys                       |Value Format   |Information
        ---------------------------|---------------|----------------------------
        temphighalarm              |FLOAT          |High Alarm Threshold value of temperature in Celsius.
        templowalarm               |FLOAT          |Low Alarm Threshold value of temperature in Celsius.
        temphighwarning            |FLOAT          |High Warning Threshold value of temperature in Celsius.
        templowwarning             |FLOAT          |Low Warning Threshold value of temperature in Celsius.
        vcchighalarm               |FLOAT          |High Alarm Threshold value of supply voltage in mV.
        vcclowalarm                |FLOAT          |Low Alarm Threshold value of supply voltage in mV.
        vcchighwarning             |FLOAT          |High Warning Threshold value of supply voltage in mV.
        vcclowwarning              |FLOAT          |Low Warning Threshold value of supply voltage in mV.
        rxpowerhighalarm           |FLOAT          |High Alarm Threshold value of received power in dBm.
        rxpowerlowalarm            |FLOAT          |Low Alarm Threshold value of received power in dBm.
        rxpowerhighwarning         |FLOAT          |High Warning Threshold value of received power in dBm.
        rxpowerlowwarning          |FLOAT          |Low Warning Threshold value of received power in dBm.
        txpowerhighalarm           |FLOAT          |High Alarm Threshold value of transmit power in dBm.
        txpowerlowalarm            |FLOAT          |Low Alarm Threshold value of transmit power in dBm.
        txpowerhighwarning         |FLOAT          |High Warning Threshold value of transmit power in dBm.
        txpowerlowwarning          |FLOAT          |Low Warning Threshold value of transmit power in dBm.
        txbiashighalarm            |FLOAT          |High Alarm Threshold value of tx Bias Current in mA.
        txbiaslowalarm             |FLOAT          |Low Alarm Threshold value of tx Bias Current in mA.
        txbiashighwarning          |FLOAT          |High Warning Threshold value of tx Bias Current in mA.
        txbiaslowwarning           |FLOAT          |Low Warning Threshold value of tx Bias Current in mA.
        ========================================================================
        """
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
        if not self.get_presence():
            return {}

        if not self.dom_supported:
            return transceiver_dom_threshold_info_dict

        if not self.dom_thresholds_supported:
            return transceiver_dom_threshold_info_dict

        sfpd_obj = qsfp_dd_Dom()
        if sfpd_obj is None:
            return transceiver_dom_threshold_info_dict

        # page 02
        offset = CMIS_UPPER_PAGE02_START
        dom_module_threshold_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_MODULE_THRESHOLD_OFFSET), QSFP_DD_MODULE_THRESHOLD_WIDTH)
        if dom_module_threshold_raw is None:
            return transceiver_dom_threshold_info_dict

        dom_module_threshold_data = sfpd_obj.parse_module_threshold_values(dom_module_threshold_raw, 0)

        # Threshold Data
        transceiver_dom_threshold_info_dict['temphighalarm'] = dom_module_threshold_data['data']['TempHighAlarm']['value']
        transceiver_dom_threshold_info_dict['temphighwarning'] = dom_module_threshold_data['data']['TempHighWarning']['value']
        transceiver_dom_threshold_info_dict['templowalarm'] = dom_module_threshold_data['data']['TempLowAlarm']['value']
        transceiver_dom_threshold_info_dict['templowwarning'] = dom_module_threshold_data['data']['TempLowWarning']['value']
        transceiver_dom_threshold_info_dict['vcchighalarm'] = dom_module_threshold_data['data']['VccHighAlarm']['value']
        transceiver_dom_threshold_info_dict['vcchighwarning'] = dom_module_threshold_data['data']['VccHighWarning']['value']
        transceiver_dom_threshold_info_dict['vcclowalarm'] = dom_module_threshold_data['data']['VccLowAlarm']['value']
        transceiver_dom_threshold_info_dict['vcclowwarning'] = dom_module_threshold_data['data']['VccLowWarning']['value']
        transceiver_dom_threshold_info_dict['rxpowerhighalarm'] = dom_module_threshold_data['data']['RxPowerHighAlarm']['value']
        transceiver_dom_threshold_info_dict['rxpowerhighwarning'] = dom_module_threshold_data['data']['RxPowerHighWarning']['value']
        transceiver_dom_threshold_info_dict['rxpowerlowalarm'] = dom_module_threshold_data['data']['RxPowerLowAlarm']['value']
        transceiver_dom_threshold_info_dict['rxpowerlowwarning'] = dom_module_threshold_data['data']['RxPowerLowWarning']['value']
        transceiver_dom_threshold_info_dict['txbiashighalarm'] = dom_module_threshold_data['data']['TxBiasHighAlarm']['value']
        transceiver_dom_threshold_info_dict['txbiashighwarning'] = dom_module_threshold_data['data']['TxBiasHighWarning']['value']
        transceiver_dom_threshold_info_dict['txbiaslowalarm'] = dom_module_threshold_data['data']['TxBiasLowAlarm']['value']
        transceiver_dom_threshold_info_dict['txbiaslowwarning'] = dom_module_threshold_data['data']['TxBiasLowWarning']['value']
        transceiver_dom_threshold_info_dict['txpowerhighalarm'] = dom_module_threshold_data['data']['TxPowerHighAlarm']['value']
        transceiver_dom_threshold_info_dict['txpowerhighwarning'] = dom_module_threshold_data['data']['TxPowerHighWarning']['value']
        transceiver_dom_threshold_info_dict['txpowerlowalarm'] = dom_module_threshold_data['data']['TxPowerLowAlarm']['value']
        transceiver_dom_threshold_info_dict['txpowerlowwarning'] = dom_module_threshold_data['data']['TxPowerLowWarning']['value']


        return transceiver_dom_threshold_info_dict

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP

        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        reset_status = False

        attr_rv = self.__api_helper.read_one_line_file(self.__attr_path_prefix + 'reset')
        if (attr_rv != None):
            attr_rv = int(attr_rv, 16)
            if ((attr_rv & 0x1)  == 1):
                reset_status = True
        return reset_status

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
        if not self.dom_supported:
            return None

        rx_los_list = []

        if self.dom_rx_tx_power_bias_supported:
            offset = 512
            dom_channel_monitor_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_CHANNL_RX_LOS_STATUS_OFFSET), QSFP_DD_CHANNL_RX_LOS_STATUS_WIDTH)
            if dom_channel_monitor_raw is not None:
                rx_los_data = int(dom_channel_monitor_raw[0], 8)
                rx_los_list.append(rx_los_data & 0x01 != 0)
                rx_los_list.append(rx_los_data & 0x02 != 0)
                rx_los_list.append(rx_los_data & 0x04 != 0)
                rx_los_list.append(rx_los_data & 0x08 != 0)
                rx_los_list.append(rx_los_data & 0x10 != 0)
                rx_los_list.append(rx_los_data & 0x20 != 0)
                rx_los_list.append(rx_los_data & 0x40 != 0)
                rx_los_list.append(rx_los_data & 0x80 != 0)

        return rx_los_list

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
        if not self.dom_supported:
            return None

        return None

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP

        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        lpmode = False
        attr_path = self.__lpmode_attr

        attr_rv = self.__get_attr_value(attr_path)
        if (attr_rv != 'ERR'):
            if (int(attr_rv,16) == 1):
                lpmode = True

        return lpmode

    def get_power_override(self):
        """
        Retrieves the power-override status of this SFP

        Returns:
            A Boolean, True if power-override is enabled, False if disabled
        """
        return None

    def get_temperature(self):
        """
        Retrieves the temperature of this SFP

        Returns:
            An integer number of current temperature in Celsius
        """
        offset = 0

        sfpd_obj = qsfp_dd_Dom()
        if sfpd_obj is None:
            return None

        if self.dom_temp_supported:
            dom_temperature_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_TEMPE_OFFSET), QSFP_DD_TEMPE_WIDTH)
            if dom_temperature_raw is not None:
                dom_temperature_data = sfpd_obj.parse_temperature(dom_temperature_raw, 0)
                temp = self._convert_string_to_num(dom_temperature_data['data']['Temperature']['value'])
                return temp
        return None

    def get_voltage(self):
        """
        Retrieves the supply voltage of this SFP

        Returns:
            An integer number of supply voltage in mV
        """
        offset = 128

        sfpd_obj = qsfp_dd_Dom()
        if sfpd_obj is None:
            return None

        if self.dom_volt_supported:
            dom_voltage_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_VOLT_OFFSET), QSFP_DD_VOLT_WIDTH)
            if dom_voltage_raw is not None:
                dom_voltage_data = sfpd_obj.parse_voltage(dom_voltage_raw, 0)
                voltage = self._convert_string_to_num(dom_voltage_data['data']['Vcc']['value'])
                return voltage
        return None


    def get_tx_bias(self):
        """
        Retrieves the TX bias current of this SFP

        Returns:
            A list of four integer numbers, representing TX bias in mA
            for channel 0 to channel 4.
            Ex. ['110.09', '111.12', '108.21', '112.09']
        """
        tx_bias_list = []
        # page 11h
        if self.dom_rx_tx_power_bias_supported:
            offset = CMIS_UPPER_PAGE11_START
            sfpd_obj = qsfp_dd_Dom()
            if sfpd_obj is None:
                return None

            if self.dom_tx_bias_power_supported:
                dom_tx_bias_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_TX_BIAS_OFFSET), QSFP_DD_TX_BIAS_WIDTH)
                if dom_tx_bias_raw is not None:
                    dom_tx_bias_data = sfpd_obj.parse_dom_tx_bias(dom_tx_bias_raw, 0)
                    tx_bias_list.append(self._convert_string_to_num(dom_tx_bias_data['data']['TX1Bias']['value']))
                    tx_bias_list.append(self._convert_string_to_num(dom_tx_bias_data['data']['TX2Bias']['value']))
                    tx_bias_list.append(self._convert_string_to_num(dom_tx_bias_data['data']['TX3Bias']['value']))
                    tx_bias_list.append(self._convert_string_to_num(dom_tx_bias_data['data']['TX4Bias']['value']))
                    tx_bias_list.append(self._convert_string_to_num(dom_tx_bias_data['data']['TX5Bias']['value']))
                    tx_bias_list.append(self._convert_string_to_num(dom_tx_bias_data['data']['TX6Bias']['value']))
                    tx_bias_list.append(self._convert_string_to_num(dom_tx_bias_data['data']['TX7Bias']['value']))
                    tx_bias_list.append(self._convert_string_to_num(dom_tx_bias_data['data']['TX8Bias']['value']))
        return tx_bias_list

    def get_rx_power(self):
        """
        Retrieves the received optical power for this SFP

        Returns:
            A list of four integer numbers, representing received optical
            power in mW for channel 0 to channel 4.
            Ex. ['1.77', '1.71', '1.68', '1.70']
        """
        rx_power_list = []
        # page 11
        if self.dom_rx_tx_power_bias_supported:
            offset = CMIS_UPPER_PAGE11_START
            sfpd_obj = qsfp_dd_Dom()
            if sfpd_obj is None:
                return None

            if self.dom_rx_power_supported:
                dom_rx_power_raw = self._read_eeprom_specific_bytes((offset + QSFP_DD_RX_POWER_OFFSET), QSFP_DD_RX_POWER_WIDTH)
                if dom_rx_power_raw is not None:
                    dom_rx_power_data = sfpd_obj.parse_dom_rx_power(dom_rx_power_raw, 0)
                    rx_power_list.append(self._convert_string_to_num(dom_rx_power_data['data']['RX1Power']['value']))
                    rx_power_list.append(self._convert_string_to_num(dom_rx_power_data['data']['RX2Power']['value']))
                    rx_power_list.append(self._convert_string_to_num(dom_rx_power_data['data']['RX3Power']['value']))
                    rx_power_list.append(self._convert_string_to_num(dom_rx_power_data['data']['RX4Power']['value']))
                    rx_power_list.append(self._convert_string_to_num(dom_rx_power_data['data']['RX5Power']['value']))
                    rx_power_list.append(self._convert_string_to_num(dom_rx_power_data['data']['RX6Power']['value']))
                    rx_power_list.append(self._convert_string_to_num(dom_rx_power_data['data']['RX7Power']['value']))
                    rx_power_list.append(self._convert_string_to_num(dom_rx_power_data['data']['RX8Power']['value']))
        return rx_power_list

    def get_tx_power(self):
        """
        Retrieves the TX power of this SFP

        Returns:
            A list of four integer numbers, representing TX power in mW
            for channel 0 to channel 4.
            Ex. ['1.86', '1.86', '1.86', '1.86']
        """
        return None

    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.

        Returns:
            A boolean, True if successful, False if not
        """

        try:
            reg_file = open(self.__reset_attr, "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_value = 1
        reg_file.write(hex(reg_value))
        reg_file.close()

        # Sleep 2 second to allow it to settle
        time.sleep(2)

        try:
            reg_file = open(self.__reset_attr, "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        reg_value = 0
        reg_file.write(hex(reg_value))
        reg_file.close()

        return True

    def is_mem_flat(self):
        offset = CMIS_LOWER_PAGE_START + CMIS_LOWER_PAGE_FLAT_MEM
        data = self._read_eeprom_specific_bytes(offset, 1)
        is_flat_memory = (int(data[0],16) & 0xff) >> 7
        return is_flat_memory

    def get_tx_disable_support(self):
        offset = CMIS_UPPER_PAGE01_START + CMIS_UPPER_PAGE01_TX_DISABLE_IMPLEMENTED
        data = self._read_eeprom_specific_bytes(offset, 1)
        tx_disable_implemented = ((int(data[0],16) & 0xff) & (0x1 << 1)) >> 1
        return not self.is_mem_flat() and tx_disable_implemented
    
    def get_tx_disable(self):
        """
        Retrieves the tx_disable status of this SFP

        Returns:
            A Boolean List
        """
        tx_disable_support = self.get_tx_disable_support()
        if tx_disable_support is None:
            return None
        if not tx_disable_support:
            return ["N/A" for _ in range(self.NUM_CHANNELS)]
        offset = CMIS_UPPER_PAGE10_START + CMIS_UPPER_PAGE10_TX_DISABLE_OFFSET
        tx_disable = self._read_eeprom_specific_bytes(offset, 1)
        
        return [bool(int(tx_disable[0],16) & (1 << i)) for i in range(self.NUM_CHANNELS)]

    def get_tx_disable_channel(self):
        """
        Retrieves the TX disabled channels in this SFP

        Returns:
            A hex of 8 bits (bit 0 to bit 7 as channel 0 to channel 7) to represent
            TX channels which have been disabled in this SFP.
            As an example, a returned value of 0x5 indicates that channel 0
            and channel 2 have been disabled.
        """
        tx_disable_support = self.get_tx_disable_support()
        if tx_disable_support is None:
            return None
        if not tx_disable_support:
            return 'N/A'
        
        offset = CMIS_UPPER_PAGE10_START + CMIS_UPPER_PAGE10_TX_DISABLE_OFFSET
        tx_disable = self._read_eeprom_specific_bytes(offset, 1)
        return int(tx_disable[0],16)

    def tx_disable(self, tx_disable):
        """
        Disable SFP TX for all channels

        Args:
            tx_disable : A Boolean, True to enable tx_disable mode, False to disable
                         tx_disable mode.

        Returns:
            A boolean, True if tx_disable is set successfully, False if not
        """
        if not self.get_presence():
            return False

        if self.dom_tx_disable_supported:
            channel_mask = 0xff
            if tx_disable:
                return self.tx_disable_channel(channel_mask, True)
            else:
                return self.tx_disable_channel(channel_mask, False)
        else:
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
        channel_state = self.get_tx_disable_channel()
        if channel_state is None or channel_state == 'N/A':
            return False

        for i in range(self.NUM_CHANNELS):
            mask = (1 << i)
            if not (channel & mask):
                continue
            if disable:
                channel_state |= mask
            else:
                channel_state &= ~mask
        offset = CMIS_UPPER_PAGE10_START + CMIS_UPPER_PAGE10_TX_DISABLE_OFFSET

        return self._write_eeprom_specific_bytes(offset, channel_state)
    
    def set_datapath_deinit(self, channel):
        """
        Put the CMIS datapath into the de-initialized state

        Args:
            channel:
                Integer, a bitmask of the lanes on the host side
                e.g. 0x5 for lane 0 and lane 2.

        Returns:
            Boolean, true if success otherwise false
        """
        offset = CMIS_LOWER_PAGE_START + CMIS_LOWER_PAGE_REVISION_COMPLIANCE_OFFSET
        cmis_major = self._read_eeprom_specific_bytes(offset, 1)
    
        offset = CMIS_UPPER_PAGE10_START + CMIS_UPPER_PAGE10_DATA_PATH_DEINIT_OFFSET
        data = self._read_eeprom_specific_bytes(offset, 1)
        deinit_value = int(data[0],16)
        for lane in range(self.NUM_CHANNELS):
            if ((1 << lane) & channel) == 0:
                continue
            if int(cmis_major[0],16) >= 4: # CMIS v4 onwards
                deinit_value |= (1 << lane)
            else:               # CMIS v3
                deinit_value &= ~(1 << lane)

        return self._write_eeprom_specific_bytes(offset,deinit_value)

    def set_datapath_init(self, channel):
        """
        Put the CMIS datapath into the de-initialized state

        Args:
            channel:
                Integer, a bitmask of the lanes on the host side
                e.g. 0x5 for lane 0 and lane 2.

        Returns:
            Boolean, true if success otherwise false
        """
        offset = CMIS_LOWER_PAGE_START + CMIS_LOWER_PAGE_REVISION_COMPLIANCE_OFFSET
        cmis_major = self._read_eeprom_specific_bytes(offset, 1)
    
        offset = CMIS_UPPER_PAGE10_START + CMIS_UPPER_PAGE10_DATA_PATH_DEINIT_OFFSET
        data = self._read_eeprom_specific_bytes(offset, 1)
        deinit_value = int(data[0],16)
        for lane in range(self.NUM_CHANNELS):
            if ((1 << lane) & channel) == 0:
                continue
            if int(cmis_major[0],16) >= 4: # CMIS v4 onwards
                deinit_value &= ~(1 << lane)
            else:               # CMIS v3
                deinit_value |= (1 << lane)

        return self._write_eeprom_specific_bytes(offset,deinit_value)

    def get_module_state(self):
        """
        get the module state

        Args:
        Returns:
            string in dict MODULE_STATE 
        """
        offset = CMIS_LOWER_PAGE_START + CMIS_LOWER_PAGE_MODULE_STATE_OFFSET
        data = self._read_eeprom_specific_bytes(offset, 1)
        state = (int(data[0],16) & 0xe) >> 1
        return MODULE_STATE[state]

    def test_module_state(self, states):
        """
        Check if the CMIS module is in the specified state

        Args:
            states:
                List, a string list of states

        Returns:
            Boolean, true if it's in the specified state, otherwise false
        """
        return self.get_module_state() in states

    def get_datapath_state(self,channel):
        """
        get the channel datapath state

        Args:
            channel:
                Integer, a bitmask of the lanes on the host side
                e.g. 0x5 for lane 0 and lane 2.
        Returns:
            string in dict DATAPATH_STATE 
        """
        state = ''
        for lane in range(self.NUM_CHANNELS):
            if ((1 << lane) & channel) == 0:
                continue
            offset = CMIS_UPPER_PAGE11_START + CMIS_UPPER_PAGE11_DATA_PATH_STATE_OFFSET + (lane//2)
            data = self._read_eeprom_specific_bytes(offset, 1)
            tmp_data = int(data[0],16)
            if tmp_data % 2:
                state = tmp_data >> 4
            else:
                state = tmp_data & 0xf
        return DATAPATH_STATE[state]

    def test_datapath_state(self, channel, states):
        """
        Check if the CMIS datapath states are in the specified state

        Args:
            channel:
                Integer, a bitmask of the lanes on the host side
                e.g. 0x5 for lane 0 and lane 2.
            states:
                List, a string list of states

        Returns:
            Boolean, true if all lanes are in the specified state, otherwise false
        """
        return self.get_datapath_state(channel) in states

    def set_application(self, channel, appl_code):
        """
        Update the selected application code to the specified lanes on the host side

        Args:
            channel:
                Integer, a bitmask of the lanes on the host side
                e.g. 0x5 for lane 0 and lane 2.
            appl_code:
                Integer, the desired application code

        Returns:
            Boolean, true if success otherwise false
        """
        # Update the application selection
        lane_first = -1
        for lane in range(self.NUM_CHANNELS):
            if ((1 << lane) & channel) == 0:
                continue
            if lane_first < 0:
                lane_first = lane
            offset = CMIS_UPPER_PAGE10_START + CMIS_UPPER_PAGE10_STAGED0_APSEL_CONTROL_OFFSET + lane
            data = (appl_code << 4) | (lane_first << 1)
            self._write_eeprom_specific_bytes(offset, data)

        offset = CMIS_UPPER_PAGE10_START + CMIS_UPPER_PAGE10_STAGED0_APPLY_DATA_PATH_INIT_OFFSET
        #data = (appl_code << 4) | (lane_first << 1)
        # Apply DataPathInit
        return self._write_eeprom_specific_bytes(offset, channel)

    def get_config_datapath_hostlane_status(self,channel):
        """
        return configuration command execution result status for 
        the datapath of each host lane

        Args:
            channel:
                Integer, a bitmask of the lanes on the host side
                e.g. 0x5 for lane 0 and lane 2.
        Returns:
            string in dict CONFIG_STATUS 
        """
        state = ''
        for lane in range(self.NUM_CHANNELS):
            if ((1 << lane) & channel) == 0:
                continue
            offset = CMIS_UPPER_PAGE11_START + CMIS_UPPER_PAGE11_CONFIG_ERROR_CODE_OFFSET + (lane//2)
            data = self._read_eeprom_specific_bytes(offset, 1)
            tmp_data = int(data[0],16)
            if tmp_data % 2:
                state = tmp_data >> 4
            else:
                state = tmp_data & 0xf
        return CONFIG_STATUS[state]

    def test_config_error(self, channel, states):
        """
        Check if the CMIS configuration states are in the specified state

        Args:
            channel:
                Integer, a bitmask of the lanes on the host side
                e.g. 0x5 for lane 0 and lane 2.
            states:
                List, a string list of states

        Returns:
            Boolean, true if all lanes are in the specified state, otherwise false
        """
        return self.get_config_datapath_hostlane_status(channel) in states

    def reset_cmis_init(self,retries):
        self.cmis_state = CMIS_STATE_INSERTED
        self.cmis_retries = retries
        self.cmis_expired = None # No expiration

    def init_sequence(self):
        host_lanes = 0xff
        retries = self.cmis_retries
        expired = self.cmis_expired
        now = datetime.datetime.now()
        self.cmis_state = CMIS_STATE_INSERTED
        
        while self.cmis_state is not CMIS_STATE_READY:
          # CMIS state transitions
            if self.cmis_state == CMIS_STATE_INSERTED:
                """
                appl = self.get_cmis_application_desired( host_lanes, host_speed)
                if appl < 1:
                    logging.errorr("port {}: no suitable app for the port".format(self.__index))
                    self.cmis_state = CMIS_STATE_FAILED
                    continue

                has_update = self.is_cmis_application_update_required(host_lanes, host_speed)
                if not has_update:
                    # No application updates
                    logging.error("port {}: READY".format(self.__index))
                    self.cmis_state = CMIS_STATE_READY
                    continue
                """
                # D.2.2 Software Deinitialization
                self.set_datapath_deinit(host_lanes)
                self.set_lpmode(True)
                if not self.test_module_state(['ModuleReady', 'ModuleLowPwr']):
                    logging.error("port {}: unable to enter low-power mode".format(self.__index))
                    self.cmis_retries = retries + 1
                    continue

                # D.1.3 Software Configuration and Initialization
                if not self.tx_disable_channel(host_lanes, True):
                    logging.error("port {}: unable to turn off tx power".format(self.__index))
                    self.cmis_retries = retries + 1
                    continue
                self.set_lpmode(False)

                # TODO: Use fine grained time when the CMIS memory map is available
                self.cmis_expired = now + datetime.timedelta(seconds=CMIS_DEF_EXPIRED)
                self.cmis_state = CMIS_STATE_DP_DEINIT
            elif self.cmis_state == CMIS_STATE_DP_DEINIT:
                if not self.test_module_state(['ModuleReady']):
                    if (expired is not None) and (expired <= now):
                        logging.error("port {}: timeout for 'ModuleReady'".format(self.__index))
                        self.reset_cmis_init(retries + 1)
                    continue
                if not self.test_datapath_state(host_lanes, ['DataPathDeinit', 'DataPathDeactivated']):
                    if (expired is not None) and (expired <= now):
                        logging.error("port {}: timeout for 'DataPathDeinit'".format(self.__index))
                        self.reset_cmis_init(retries + 1)
                    continue

                """
                # D.1.3 Software Configuration and Initialization
                appl = self.get_cmis_application_desired(host_lanes, host_speed)
                if appl < 1:
                    logging.error("port {}: no suitable app for the port".format(self.__index))
                    self.cmis_state = CMIS_STATE_FAILED
                    continue
                if not self.set_application(host_lanes, appl):
                """
                if not self.set_application(host_lanes, 1):
                    logging.error("{}: unable to set application".format(self.__index))
                    self.reset_cmis_init(retries + 1)
                    continue

                # TODO: Use fine grained time when the CMIS memory map is available
                self.cmis_expired = now + datetime.timedelta(seconds=CMIS_DEF_EXPIRED)
                self.cmis_state = CMIS_STATE_AP_CONF
            elif self.cmis_state == CMIS_STATE_AP_CONF:
                if not self.test_config_error(host_lanes, ['ConfigSuccess']):
                    if (expired is not None) and (expired <= now):
                        logging.error("port {}: timeout for 'ConfigSuccess'".format(self.__index))
                        self.reset_cmis_init(retries + 1)
                    continue

                # D.1.3 Software Configuration and Initialization
                self.set_datapath_init(host_lanes)

                # TODO: Use fine grained time when the CMIS memory map is available
                self.cmis_expired = now + datetime.timedelta(seconds=CMIS_DEF_EXPIRED)
                self.cmis_state = CMIS_STATE_DP_INIT
            elif self.cmis_state == CMIS_STATE_DP_INIT:
                if not self.test_datapath_state(host_lanes, ['DataPathInitialized']):
                    if (expired is not None) and (expired <= now):
                        logging.error("port {}: timeout for 'DataPathInitialized'".format(self.__index))
                        self.reset_cmis_init(retries + 1)
                    continue

                # D.1.3 Software Configuration and Initialization
                if not self.tx_disable_channel(host_lanes, False):
                    logging.error("port {}: unable to turn on tx power".format(self.__index))
                    self.reset_cmis_init(retries + 1)
                    continue

                # TODO: Use fine grained timeout when the CMIS memory map is available
                self.cmis_expired = now + datetime.timedelta(seconds=CMIS_DEF_EXPIRED)
                self.cmis_state = CMIS_STATE_DP_TXON
            elif self.cmis_state == CMIS_STATE_DP_TXON:
                if not self.test_datapath_state(host_lanes, ['DataPathActivated']):
                    if (expired is not None) and (expired <= now):
                        logging.error("port {}: timeout for 'DataPathActivated'".format(self.__index))
                        self.reset_cmis_init(retries + 1)
                    continue
                logging.error("port {}: READY".format(self.__index))
                self.cmis_state = CMIS_STATE_READY
        return self.cmis_state


    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP

        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note  : lpmode can be overridden by set_power_override

        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """
        try:
            reg_file = open(self.__lpmode_attr, "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        #reg_value = int(reg_file.readline().rstrip(),16)

        if lpmode is True:
            reg_value = 1
        else:
            reg_value = 0

        reg_file.write(hex(reg_value))
        reg_file.close()

        return True

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
        return True

