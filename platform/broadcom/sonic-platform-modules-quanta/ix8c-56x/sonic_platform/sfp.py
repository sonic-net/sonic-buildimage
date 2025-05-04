#!/usr/bin/env python

#############################################################################
# Quanta
#
# Sfp contains an implementation of SONiC Platform Base API and
# provides the sfp device status which are available in the platform
#
#############################################################################

import os
import time
from ctypes import create_string_buffer

try:
     from sonic_platform_base.sfp_base import SfpBase
     from sonic_platform_base.sonic_sfp.sff8472 import sff8472InterfaceId
     from sonic_platform_base.sonic_sfp.sff8472 import sff8472Dom
     from sonic_platform_base.sonic_sfp.sff8436 import sff8436InterfaceId
     from sonic_platform_base.sonic_sfp.sff8436 import sff8436Dom
     from sonic_platform_base.sonic_sfp.sfputilhelper import SfpUtilHelper
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

QSFP_INFO_OFFSET = 128
QSFP_DOM_OFFSET = 0
SFP_INFO_OFFSET = 0
SFP_DOM_OFFSET = 256

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
XCVR_HW_REV_WIDTH_QSFP = 2
XCVR_HW_REV_WIDTH_SFP = 4
XCVR_VENDOR_SN_OFFSET = 68
XCVR_VENDOR_SN_WIDTH = 16
XCVR_VENDOR_DATE_OFFSET = 84
XCVR_VENDOR_DATE_WIDTH = 8
XCVR_DOM_CAPABILITY_OFFSET = 92
XCVR_DOM_CAPABILITY_WIDTH = 2

XCVR_INTERFACE_DATA_START = 0
XCVR_INTERFACE_DATA_SIZE = 92

QSFP_DOM_BULK_DATA_START = 22
QSFP_DOM_BULK_DATA_SIZE = 36
SFP_DOM_BULK_DATA_START = 96
SFP_DOM_BULK_DATA_SIZE = 10

# Offset for values in QSFP eeprom
QSFP_DOM_REV_OFFSET = 1
QSFP_DOM_REV_WIDTH = 1
QSFP_TEMPE_OFFSET = 22
QSFP_TEMPE_WIDTH = 2
QSFP_VOLT_OFFSET = 26
QSFP_VOLT_WIDTH = 2
QSFP_VERSION_COMPLIANCE_OFFSET = 1
QSFP_VERSION_COMPLIANCE_WIDTH = 1
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
QSFP_MODULE_THRESHOLD_OFFSET = 512
QSFP_MODULE_THRESHOLD_WIDTH = 24
QSFP_CHANNEL_THRESHOLD_OFFSET = 560
QSFP_CHANNEL_THRESHOLD_WIDTH = 16
QSFP_POWEROVERRIDE_OFFSET = 93
QSFP_POWEROVERRIDE_WIDTH = 1
QSFP_POWEROVERRIDE_BIT = 0
QSFP_POWERSET_BIT = 1
QSFP_OPTION_VALUE_OFFSET = 192
QSFP_OPTION_VALUE_WIDTH = 4

SFP_TEMPE_OFFSET = 96
SFP_TEMPE_WIDTH = 2
SFP_VOLT_OFFSET = 98
SFP_VOLT_WIDTH = 2
SFP_CHANNL_MON_OFFSET = 100
SFP_CHANNL_MON_WIDTH = 6
SFP_MODULE_THRESHOLD_OFFSET = 0
SFP_MODULE_THRESHOLD_WIDTH = 40
SFP_CHANNL_THRESHOLD_OFFSET = 112
SFP_CHANNL_THRESHOLD_WIDTH = 2
SFP_STATUS_CONTROL_OFFSET = 110
SFP_STATUS_CONTROL_WIDTH = 1
SFP_TX_DISABLE_HARD_BIT = 7
SFP_TX_DISABLE_SOFT_BIT = 6

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


class Sfp(SfpBase):
    """Platform-specific Sfp class"""

    # Port number
    PORT_START = 1
    PORT_END = 56
    QSFP_START = 49
    QSFP_END = 56

    dom_supported = True
    dom_temp_supported = True
    dom_volt_supported = True
    dom_rx_power_supported = True
    dom_tx_power_supported = True
    dom_tx_disable_supported = True
    calibration = 1

    # Path to QSFP sysfs
    PLATFORM_ROOT_PATH = "/usr/share/sonic/device"
    PMON_HWSKU_PATH = "/usr/share/sonic/hwsku"
    HOST_CHK_CMD = "docker > /dev/null 2>&1"

    PLATFORM = "x86_64-quanta_ix8c_bwde-r0"
    HWSKU  = "Quanta-IX8C-56X"

    def __init__(self, sfp_index, sfp_type):
        # Init index
        self.index = sfp_index
        self.port_num = self.index
        self.sfp_type = sfp_type
        # Init eeprom path
        eeprom_path = '/sys/bus/i2c/devices/i2c-{0}/{0}-0050/eeprom'
        self.port_to_eeprom_mapping = {}
        self.port_to_i2c_mapping = {
             1 : 13,
             2 : 14,
             3 : 15,
             4 : 16,
             5 : 17,
             6 : 18,
             7 : 19,
             8 : 20,
             9 : 21,
            10 : 22,
            11 : 23,
            12 : 24,
            13 : 25,
            14 : 26,
            15 : 27,
            16 : 28,
            17 : 29,
            18 : 30,
            19 : 31,
            20 : 32,
            21 : 33,
            22 : 34,
            23 : 35,
            24 : 36,
            25 : 37,
            26 : 38,
            27 : 39,
            28 : 40,
            29 : 41,
            30 : 42,
            31 : 43,
            32 : 44,
            33 : 45,
            34 : 46,
            35 : 47,
            36 : 48,
            37 : 49,
            38 : 50,
            39 : 51,
            40 : 52,
            41 : 53,
            42 : 54,
            43 : 55,
            44 : 56,
            45 : 57,
            46 : 58,
            47 : 59,
            48 : 60,
            49 : 61,#QSFP49
            50 : 62,#QSFP50
            51 : 63,#QSFP51
            52 : 64,#QSFP52
            53 : 65,#QSFP53
            54 : 66,#QSFP54
            55 : 67,#QSFP55
            56 : 68,#QSFP56
        }

        for x in range(self.PORT_START, self.PORT_END + 1):
            port_eeprom_path = eeprom_path.format(self.port_to_i2c_mapping[x])
            self.port_to_eeprom_mapping[x] = port_eeprom_path

        self.info_dict_keys = ['type', 'vendor_rev', 'serialnum', 'manufacturer',
            'model', 'connector', 'encoding', 'ext_identifier',
            'ext_rateselect_compliance', 'cable_type', 'cable_length',
            'nominal_bit_rate', 'specification_compliance', 'vendor_date',
            'vendor_oui', 'application_advertisement']

        self.dom_dict_keys = ['rx_los', 'tx_fault', 'reset_status', 'power_lpmode',
            'tx_disable', 'tx_disable_channel', 'temperature', 'voltage',
            'rx1power', 'rx2power', 'rx3power', 'rx4power', 'tx1bias', 'tx2bias',
            'tx3bias', 'tx4bias', 'tx1power', 'tx2power', 'tx3power', 'tx4power']

        self.threshold_dict_keys = ['temphighalarm', 'temphighwarning',
            'templowalarm', 'templowwarning', 'vcchighalarm', 'vcchighwarning',
            'vcclowalarm', 'vcclowwarning', 'rxpowerhighalarm', 'rxpowerhighwarning',
            'rxpowerlowalarm', 'rxpowerlowwarning', 'txpowerhighalarm',
            'txpowerhighwarning', 'txpowerlowalarm', 'txpowerlowwarning',
            'txbiashighalarm', 'txbiashighwarning', 'txbiaslowalarm', 'txbiaslowwarning']

        SfpBase.__init__(self)


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

    def __is_host(self):
        return os.system(self.HOST_CHK_CMD) == 0

    def __get_path_to_port_config_file(self):
        platform_path = "/".join([self.PLATFORM_ROOT_PATH, self.PLATFORM])
        hwsku_path = "/".join([platform_path, self.HWSKU]
                              ) if self.__is_host() else self.PMON_HWSKU_PATH
        return "/".join([hwsku_path, "port_config.ini"])

    def get_presence(self):
        """
        Retrieves the presence of the SFP module
        Returns:
            bool: True if SFP module is present, False if not
        """
        # Check for invalid port_num
        if self.port_num < self.PORT_START or self.port_num > self.PORT_END:
            return False

        try:
            if self.sfp_type == SFP_TYPE:
                reg_file = open("/sys/class/cpld-sfp28/port-"+str(self.port_num)+"/pre_n")
            else:
                reg_file = open("/sys/class/gpio/gpio"
                    +str((self.port_num-self.QSFP_START)*4+34)+"/value")
        except IOError as e:
            print ("Error: unable to open file: %s" % str(e))
            return False

        reg_value = reg_file.readline().rstrip()
        reg_file.close()
        if self.sfp_type == SFP_TYPE:
            if reg_value == '1':
                return True
        else:
            if reg_value == '0':
                return True

        return False

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
                eeprom_raw[n] = hex(raw[n])[2:].zfill(2)
        except Exception:
            eeprom_raw = None
        finally:
            if sysfsfile_eeprom:
                sysfsfile_eeprom.close()

        return eeprom_raw

    def _dom_capability_detect(self):
        if not self.get_presence():
            self.dom_supported = False
            self.dom_temp_supported = False
            self.dom_volt_supported = False
            self.dom_rx_power_supported = False
            self.dom_tx_power_supported = False
            self.calibration = 0
            return

        if self.sfp_type == "QSFP":
            self.calibration = 1
            sfpi_obj = sff8436InterfaceId()
            if sfpi_obj is None:
                self.dom_supported = False
            offset = 128

            # QSFP capability byte parse, through this byte can know whether it support tx_power or not.
            # TODO: in the future when decided to migrate to support SFF-8636 instead of SFF-8436,
            # need to add more code for determining the capability and version compliance
            # in SFF-8636 dom capability definitions evolving with the versions.
            qsfp_dom_capability_raw = self.__read_eeprom_specific_bytes(
                (offset + XCVR_DOM_CAPABILITY_OFFSET), XCVR_DOM_CAPABILITY_WIDTH)
            if qsfp_dom_capability_raw is not None:
                qsfp_dom_capability = int(qsfp_dom_capability_raw[0], 16)

                qsfp_version_compliance_raw = self.__read_eeprom_specific_bytes(
                    QSFP_VERSION_COMPLIANCE_OFFSET, QSFP_VERSION_COMPLIANCE_OFFSET)
                if qsfp_version_compliance_raw is not None:
                    qsfp_version_compliance = int(qsfp_version_compliance_raw[0], 16)
                else:
                    self.dom_supported = False
                    self.dom_temp_supported = False
                    self.dom_volt_supported = False
                    self.dom_rx_power_supported = False
                    self.dom_tx_power_supported = False
                    self.calibration = 0
                    return

                if qsfp_version_compliance >= 0x08:
                    self.dom_temp_supported = (qsfp_dom_capability & 0x20 != 0)
                    self.dom_volt_supported = (qsfp_dom_capability & 0x10 != 0)
                    self.dom_rx_power_supported = (qsfp_dom_capability & 0x08 != 0)
                    self.dom_tx_power_supported = (qsfp_dom_capability & 0x04 != 0)
                else:
                    self.dom_temp_supported = True
                    self.dom_volt_supported = True
                    self.dom_rx_power_supported = (qsfp_dom_capability & 0x08 != 0)
                    self.dom_tx_power_supported = True
                self.dom_supported = True
                self.calibration = 1
                self.dom_tx_disable_supported = True
            else:
                self.dom_supported = False
                self.dom_temp_supported = False
                self.dom_volt_supported = False
                self.dom_rx_power_supported = False
                self.dom_tx_power_supported = False
                self.calibration = 0
        elif self.sfp_type == "SFP":
            sfpi_obj = sff8472InterfaceId()
            if sfpi_obj is None:
                return None
            sfp_dom_capability_raw = self.__read_eeprom_specific_bytes(
                XCVR_DOM_CAPABILITY_OFFSET, XCVR_DOM_CAPABILITY_WIDTH)
            if sfp_dom_capability_raw is not None:
                sfp_dom_capability = int(sfp_dom_capability_raw[0], 16)
                self.dom_supported = (sfp_dom_capability & 0x40 != 0)
                if self.dom_supported:
                    self.dom_temp_supported = True
                    self.dom_volt_supported = True
                    self.dom_rx_power_supported = True
                    self.dom_tx_power_supported = True
                    if sfp_dom_capability & 0x20 != 0:
                        self.calibration = 1
                    elif sfp_dom_capability & 0x10 != 0:
                        self.calibration = 2
                    else:
                        self.calibration = 0
                else:
                    self.dom_temp_supported = False
                    self.dom_volt_supported = False
                    self.dom_rx_power_supported = False
                    self.dom_tx_power_supported = False
                    self.calibration = 0
                self.dom_tx_disable_supported = (int(sfp_dom_capability_raw[1], 16) & 0x40 != 0)
        else:
            self.dom_supported = False
            self.dom_temp_supported = False
            self.dom_volt_supported = False
            self.dom_rx_power_supported = False
            self.dom_tx_power_supported = False

    def get_transceiver_info(self):
        """
        Retrieves transceiver info of this SFP
        Returns:
            A dict which contains following keys/values :
        ========================================================================
        keys                       |Value Format   |Information
        ---------------------------|---------------|----------------------------
        type                       |1*255VCHAR     |type of SFP
        vendor_rev                 |1*255VCHAR     |vendor revision of SFP
        serialnum                  |1*255VCHAR     |serial number of the SFP
        manufacturer            |1*255VCHAR     |SFP vendor name
        model                  |1*255VCHAR     |SFP model name
        connector                  |1*255VCHAR     |connector information
        encoding                   |1*255VCHAR     |encoding information
        ext_identifier             |1*255VCHAR     |extend identifier
        ext_rateselect_compliance  |1*255VCHAR     |extended rateSelect compliance
        cable_length               |INT            |cable length in m
        nominal_bit_rate           |INT            |nominal bit rate by 100Mbs
        specification_compliance   |1*255VCHAR     |specification compliance
        vendor_date                |1*255VCHAR     |vendor date
        vendor_oui                 |1*255VCHAR     |vendor OUI
        application_advertisement  |1*255VCHAR     |supported applications advertisement
        ========================================================================
         """

        transceiver_info_dict = {}
        compliance_code_dict = {}
        transceiver_info_dict = dict.fromkeys(self.info_dict_keys, 'N/A')
        transceiver_info_dict['specification_compliance'] = '{}'
        if not self.get_presence():
            return transceiver_info_dict

        if self.sfp_type == QSFP_TYPE:
            offset = QSFP_INFO_OFFSET
            vendor_rev_width = XCVR_HW_REV_WIDTH_QSFP
            interface_info_bulk_width = XCVR_INTFACE_BULK_WIDTH_QSFP

            sfpi_obj = sff8436InterfaceId()
            if sfpi_obj is None:
                print("Error: sfp_object open failed")
                return transceiver_info_dict

        elif self.sfp_type == SFP_TYPE:
            offset = SFP_INFO_OFFSET
            vendor_rev_width = XCVR_HW_REV_WIDTH_SFP
            interface_info_bulk_width = XCVR_INTFACE_BULK_WIDTH_SFP

            sfpi_obj = sff8472InterfaceId()
            if sfpi_obj is None:
                print("Error: sfp_object open failed")
                return transceiver_info_dict
        else:
            return transceiver_info_dict

        # Add retry for xcvr eeprom to get ready
        max_retry = 10
        for i in range(0,max_retry):
            sfp_interface_bulk_raw = self.__read_eeprom_specific_bytes(
                offset + XCVR_INTERFACE_DATA_START, XCVR_INTERFACE_DATA_SIZE)
            if sfp_interface_bulk_raw is not None:
                break
            else:
                if not self.get_presence():
                    return transceiver_info_dict
                elif i < max_retry-1:
                    time.sleep(0.5)

        if sfp_interface_bulk_raw is None:
            return transceiver_info_dict

        start = XCVR_INTFACE_BULK_OFFSET - XCVR_INTERFACE_DATA_START
        end = start + interface_info_bulk_width
        sfp_interface_bulk_data = sfpi_obj.parse_sfp_info_bulk(sfp_interface_bulk_raw[start : end], 0)

        start = XCVR_VENDOR_NAME_OFFSET - XCVR_INTERFACE_DATA_START
        end = start + XCVR_VENDOR_NAME_WIDTH
        sfp_vendor_name_data = sfpi_obj.parse_vendor_name(sfp_interface_bulk_raw[start : end], 0)

        start = XCVR_VENDOR_PN_OFFSET - XCVR_INTERFACE_DATA_START
        end = start + XCVR_VENDOR_PN_WIDTH
        sfp_vendor_pn_data = sfpi_obj.parse_vendor_pn(sfp_interface_bulk_raw[start : end], 0)

        start = XCVR_HW_REV_OFFSET - XCVR_INTERFACE_DATA_START
        end = start + vendor_rev_width
        sfp_vendor_rev_data = sfpi_obj.parse_vendor_rev(sfp_interface_bulk_raw[start : end], 0)

        start = XCVR_VENDOR_SN_OFFSET - XCVR_INTERFACE_DATA_START
        end = start + XCVR_VENDOR_SN_WIDTH
        sfp_vendor_sn_data = sfpi_obj.parse_vendor_sn(sfp_interface_bulk_raw[start : end], 0)

        start = XCVR_VENDOR_OUI_OFFSET - XCVR_INTERFACE_DATA_START
        end = start + XCVR_VENDOR_OUI_WIDTH
        sfp_vendor_oui_data = sfpi_obj.parse_vendor_oui(sfp_interface_bulk_raw[start : end], 0)

        start = XCVR_VENDOR_DATE_OFFSET - XCVR_INTERFACE_DATA_START
        end = start + XCVR_VENDOR_DATE_WIDTH
        sfp_vendor_date_data = sfpi_obj.parse_vendor_date(sfp_interface_bulk_raw[start : end], 0)
        transceiver_info_dict['type'] = sfp_interface_bulk_data \
            ['data']['type']['value']
        transceiver_info_dict['manufacturer'] = sfp_vendor_name_data \
            ['data']['Vendor Name']['value']
        transceiver_info_dict['model'] = sfp_vendor_pn_data \
            ['data']['Vendor PN']['value']
        transceiver_info_dict['vendor_rev'] = sfp_vendor_rev_data \
            ['data']['Vendor Rev']['value']
        transceiver_info_dict['serial'] = sfp_vendor_sn_data \
            ['data']['Vendor SN']['value']
        transceiver_info_dict['vendor_oui'] = sfp_vendor_oui_data \
            ['data']['Vendor OUI']['value']
        transceiver_info_dict['vendor_date'] = sfp_vendor_date_data \
            ['data']['VendorDataCode(YYYY-MM-DD Lot)']['value']
        transceiver_info_dict['connector'] = sfp_interface_bulk_data \
            ['data']['Connector']['value']
        transceiver_info_dict['encoding'] = sfp_interface_bulk_data \
            ['data']['EncodingCodes']['value']
        transceiver_info_dict['ext_identifier'] = sfp_interface_bulk_data \
            ['data']['Extended Identifier']['value']
        transceiver_info_dict['ext_rateselect_compliance'] = sfp_interface_bulk_data \
            ['data']['RateIdentifier']['value']
        transceiver_info_dict['type_abbrv_name'] = sfp_interface_bulk_data \
            ['data']['type_abbrv_name']['value']
        if self.sfp_type == QSFP_TYPE:
            for key in qsfp_cable_length_tup:
                if key in sfp_interface_bulk_data['data']:
                    transceiver_info_dict['cable_type'] = key
                    transceiver_info_dict['cable_length'] = str(
                        sfp_interface_bulk_data['data'][key]['value'])

            for key in qsfp_compliance_code_tup:
                if key in sfp_interface_bulk_data['data']['Specification compliance']['value']:
                    compliance_code_dict[key] = sfp_interface_bulk_data \
                        ['data']['Specification compliance']['value'][key]['value']
            transceiver_info_dict['specification_compliance'] = str(compliance_code_dict)

            transceiver_info_dict['nominal_bit_rate'] = str(sfp_interface_bulk_data \
                ['data']['Nominal Bit Rate(100Mbs)']['value'])
        else:
            for key in sfp_cable_length_tup:
                if key in sfp_interface_bulk_data['data']:
                    transceiver_info_dict['cable_type'] = key
                    transceiver_info_dict['cable_length'] = str(
                        sfp_interface_bulk_data['data'][key]['value'])

            for key in sfp_compliance_code_tup:
                if key in sfp_interface_bulk_data['data']['Specification compliance']['value']:
                    compliance_code_dict[key] = sfp_interface_bulk_data \
                        ['data']['Specification compliance']['value'][key]['value']
            transceiver_info_dict['specification_compliance'] = str(compliance_code_dict)

            transceiver_info_dict['nominal_bit_rate'] = str(sfp_interface_bulk_data \
                ['data']['NominalSignallingRate(UnitsOf100Mbd)']['value'])

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

        if not self.get_presence():
            return {}

        self._dom_capability_detect()

        if self.sfp_type == QSFP_TYPE:
            sfpd_obj = sff8436Dom()
            sfpi_obj = sff8436InterfaceId()

            if not sfpi_obj or not sfpd_obj:
                return {}

            transceiver_dom_info_dict = dict.fromkeys(self.dom_dict_keys, 'N/A')
            offset = QSFP_DOM_OFFSET
            offset_xcvr = QSFP_INFO_OFFSET

            # QSFP capability byte parse, through this byte can know whether it support tx_power or not.
            # TODO: in the future when decided to migrate to support SFF-8636 instead of SFF-8436,
            # need to add more code for determining the capability and version compliance
            # in SFF-8636 dom capability definitions evolving with the versions.
            qsfp_dom_capability_raw = self.__read_eeprom_specific_bytes(
                (offset_xcvr + XCVR_DOM_CAPABILITY_OFFSET), XCVR_DOM_CAPABILITY_WIDTH)
            if qsfp_dom_capability_raw is not None:
                qsfp_dom_capability_data = sfpi_obj.parse_dom_capability(
                    qsfp_dom_capability_raw, 0)
            else:
                return None

            dom_temperature_raw = self.__read_eeprom_specific_bytes(
                (offset + QSFP_TEMPE_OFFSET), QSFP_TEMPE_WIDTH)
            if dom_temperature_raw is not None:
                dom_temperature_data = sfpd_obj.parse_temperature(
                    dom_temperature_raw, 0)
                transceiver_dom_info_dict['temperature'] = dom_temperature_data['data']['Temperature']['value']

            dom_voltage_raw = self.__read_eeprom_specific_bytes(
                (offset + QSFP_VOLT_OFFSET), QSFP_VOLT_WIDTH)
            if dom_voltage_raw is not None:
                dom_voltage_data = sfpd_obj.parse_voltage(dom_voltage_raw, 0)
                transceiver_dom_info_dict['voltage'] = dom_voltage_data['data']['Vcc']['value']

            qsfp_dom_rev_raw = self.__read_eeprom_specific_bytes(
                (offset + QSFP_DOM_REV_OFFSET), QSFP_DOM_REV_WIDTH)
            if qsfp_dom_rev_raw is not None:
                qsfp_dom_rev_data = sfpd_obj.parse_sfp_dom_rev(qsfp_dom_rev_raw, 0)
                qsfp_dom_rev = qsfp_dom_rev_data['data']['dom_rev']['value']

            # The tx_power monitoring is only available on QSFP which compliant with SFF-8636
            # and claimed that it support tx_power with one indicator bit.
            dom_channel_monitor_data = {}
            dom_channel_monitor_raw = None
            qsfp_tx_power_support = qsfp_dom_capability_data['data']['Tx_power_support']['value']
            if (qsfp_dom_rev[0:8] != 'SFF-8636' or (qsfp_dom_rev[0:8] == 'SFF-8636' and qsfp_tx_power_support != 'on')):
                dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                    (offset + QSFP_CHANNL_MON_OFFSET), QSFP_CHANNL_MON_WIDTH)
                if dom_channel_monitor_raw is not None:
                    dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params(
                        dom_channel_monitor_raw, 0)

            else:
                dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                    (offset + QSFP_CHANNL_MON_OFFSET), QSFP_CHANNL_MON_WITH_TX_POWER_WIDTH)
                if dom_channel_monitor_raw is not None:
                    dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params_with_tx_power(
                        dom_channel_monitor_raw, 0)
                    transceiver_dom_info_dict['tx1power'] = dom_channel_monitor_data['data']['TX1Power']['value']
                    transceiver_dom_info_dict['tx2power'] = dom_channel_monitor_data['data']['TX2Power']['value']
                    transceiver_dom_info_dict['tx3power'] = dom_channel_monitor_data['data']['TX3Power']['value']
                    transceiver_dom_info_dict['tx4power'] = dom_channel_monitor_data['data']['TX4Power']['value']

            if dom_channel_monitor_raw:
                transceiver_dom_info_dict['rx1power'] = dom_channel_monitor_data['data']['RX1Power']['value']
                transceiver_dom_info_dict['rx2power'] = dom_channel_monitor_data['data']['RX2Power']['value']
                transceiver_dom_info_dict['rx3power'] = dom_channel_monitor_data['data']['RX3Power']['value']
                transceiver_dom_info_dict['rx4power'] = dom_channel_monitor_data['data']['RX4Power']['value']
                transceiver_dom_info_dict['tx1bias'] = dom_channel_monitor_data['data']['TX1Bias']['value']
                transceiver_dom_info_dict['tx2bias'] = dom_channel_monitor_data['data']['TX2Bias']['value']
                transceiver_dom_info_dict['tx3bias'] = dom_channel_monitor_data['data']['TX3Bias']['value']
                transceiver_dom_info_dict['tx4bias'] = dom_channel_monitor_data['data']['TX4Bias']['value']
        elif self.sfp_type == SFP_TYPE:
            sfpd_obj = sff8472Dom()
            if not sfpd_obj:
                return {}

            eeprom_ifraw = self.__read_eeprom_specific_bytes(0, SFP_DOM_OFFSET)
            if eeprom_ifraw is not None:
                sfpi_obj = sff8472InterfaceId(eeprom_ifraw)
                cal_type = sfpi_obj.get_calibration_type()
                sfpd_obj._calibration_type = cal_type

            offset = SFP_DOM_OFFSET
            transceiver_dom_info_dict = dict.fromkeys(self.dom_dict_keys, 'N/A')
            dom_temperature_raw = self.__read_eeprom_specific_bytes(
                (offset + SFP_TEMPE_OFFSET), SFP_TEMPE_WIDTH)

            if dom_temperature_raw is not None:
                dom_temperature_data = sfpd_obj.parse_temperature(
                    dom_temperature_raw, 0)
                transceiver_dom_info_dict['temperature'] = dom_temperature_data['data']['Temperature']['value']

            dom_voltage_raw = self.__read_eeprom_specific_bytes(
                (offset + SFP_VOLT_OFFSET), SFP_VOLT_WIDTH)
            if dom_voltage_raw is not None:
                dom_voltage_data = sfpd_obj.parse_voltage(dom_voltage_raw, 0)
                transceiver_dom_info_dict['voltage'] = dom_voltage_data['data']['Vcc']['value']

            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                (offset + SFP_CHANNL_MON_OFFSET), SFP_CHANNL_MON_WIDTH)
            if dom_channel_monitor_raw is not None:
                dom_voltage_data = sfpd_obj.parse_channel_monitor_params(
                    dom_channel_monitor_raw, 0)
                transceiver_dom_info_dict['tx1power'] = dom_voltage_data['data']['TXPower']['value']
                transceiver_dom_info_dict['rx1power'] = dom_voltage_data['data']['RXPower']['value']
                transceiver_dom_info_dict['tx1bias'] = dom_voltage_data['data']['TXBias']['value']
        else:
            return None

        for key in transceiver_dom_info_dict:
            transceiver_dom_info_dict[key] = self._convert_string_to_num(
                transceiver_dom_info_dict[key])

        transceiver_dom_info_dict['rx_los'] = self.get_rx_los()
        transceiver_dom_info_dict['tx_fault'] = self.get_tx_fault()
        transceiver_dom_info_dict['reset_status'] = self.get_reset_status()
        transceiver_dom_info_dict['lp_mode'] = self.get_lpmode()
        transceiver_dom_info_dict['tx_disable'] = self.get_tx_disable()
        transceiver_dom_info_dict['tx_disable_channel'] = self.get_tx_disable_channel()

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
        transceiver_dom_threshold_info_dict_keys = ['temphighalarm',    'temphighwarning',    'templowalarm',    'templowwarning',
                                                        'vcchighalarm',     'vcchighwarning',     'vcclowalarm',     'vcclowwarning',
                                                        'rxpowerhighalarm', 'rxpowerhighwarning', 'rxpowerlowalarm', 'rxpowerlowwarning',
                                                        'txpowerhighalarm', 'txpowerhighwarning', 'txpowerlowalarm', 'txpowerlowwarning',
                                                        'txbiashighalarm',  'txbiashighwarning',  'txbiaslowalarm',  'txbiaslowwarning']

        if self.sfp_type == QSFP_TYPE:
            sfpd_obj = sff8436Dom()
            if not self.get_presence() or not sfpd_obj:
                return {}
            DOM_OFFSET  = 0
            transceiver_dom_threshold_dict = dict.fromkeys(transceiver_dom_threshold_info_dict_keys, 'N/A')
            offset = DOM_OFFSET

            dom_module_threshold_raw = self.__read_eeprom_specific_bytes((offset + QSFP_MODULE_THRESHOLD_OFFSET), QSFP_MODULE_THRESHOLD_WIDTH)
            if dom_module_threshold_raw is not None:
                module_threshold_values = sfpd_obj.parse_module_threshold_values(dom_module_threshold_raw, 0)
                module_threshold_data = module_threshold_values.get('data')
                if module_threshold_data:
                    transceiver_dom_threshold_dict['temphighalarm']   = module_threshold_data['TempHighAlarm']['value']
                    transceiver_dom_threshold_dict['templowalarm']    = module_threshold_data['TempLowAlarm']['value']
                    transceiver_dom_threshold_dict['temphighwarning'] = module_threshold_data['TempHighWarning']['value']
                    transceiver_dom_threshold_dict['templowwarning']  = module_threshold_data['TempLowWarning']['value']
                    transceiver_dom_threshold_dict['vcchighalarm']    = module_threshold_data['VccHighAlarm']['value']
                    transceiver_dom_threshold_dict['vcclowalarm']     = module_threshold_data['VccLowAlarm']['value']
                    transceiver_dom_threshold_dict['vcchighwarning']  = module_threshold_data['VccHighWarning']['value']
                    transceiver_dom_threshold_dict['vcclowwarning']   = module_threshold_data['VccLowWarning']['value']

            dom_channel_thres_raw = self.__read_eeprom_specific_bytes((offset + QSFP_CHANNEL_THRESHOLD_OFFSET), QSFP_CHANNEL_THRESHOLD_WIDTH)
            if dom_channel_thres_raw is not None:
                channel_threshold_values = sfpd_obj.parse_channel_threshold_values(dom_channel_thres_raw, 0)
                channel_threshold_data = channel_threshold_values.get('data')
            else:
                channel_threshold_data = None
            if channel_threshold_data:
                transceiver_dom_threshold_dict['rxpowerhighalarm']   = channel_threshold_data['RxPowerHighAlarm']['value']
                transceiver_dom_threshold_dict['rxpowerlowalarm']    = channel_threshold_data['RxPowerLowAlarm']['value']
                transceiver_dom_threshold_dict['rxpowerhighwarning'] = channel_threshold_data['RxPowerHighWarning']['value']
                transceiver_dom_threshold_dict['rxpowerlowwarning']  = channel_threshold_data['RxPowerLowWarning']['value']
                transceiver_dom_threshold_dict['txpowerhighalarm']   = "0.0dBm"
                transceiver_dom_threshold_dict['txpowerlowalarm']    = "0.0dBm"
                transceiver_dom_threshold_dict['txpowerhighwarning'] = "0.0dBm"
                transceiver_dom_threshold_dict['txpowerlowwarning']  = "0.0dBm"
                transceiver_dom_threshold_dict['txbiashighalarm']    = channel_threshold_data['TxBiasHighAlarm']['value']
                transceiver_dom_threshold_dict['txbiaslowalarm']     = channel_threshold_data['TxBiasLowAlarm']['value']
                transceiver_dom_threshold_dict['txbiashighwarning']  = channel_threshold_data['TxBiasHighWarning']['value']
                transceiver_dom_threshold_dict['txbiaslowwarning']   = channel_threshold_data['TxBiasLowWarning']['value']

            for key in transceiver_dom_threshold_dict:
                transceiver_dom_threshold_dict[key] = self._convert_string_to_num(transceiver_dom_threshold_dict[key])

            return transceiver_dom_threshold_dict

        elif self.sfp_type == SFP_TYPE:
            sfpd_obj = sff8472Dom()

            if not self.get_presence() and not sfpd_obj:
                return {}
            DOM_OFFSET  = 256
            eeprom_ifraw = self.__read_eeprom_specific_bytes(0, DOM_OFFSET)
            if eeprom_ifraw is not None:
                sfpi_obj = sff8472InterfaceId(eeprom_ifraw)
                cal_type = sfpi_obj.get_calibration_type()
                sfpd_obj._calibration_type = cal_type

            offset = DOM_OFFSET
            transceiver_dom_threshold_info_dict = dict.fromkeys(transceiver_dom_threshold_info_dict_keys, 'N/A')
            dom_module_threshold_raw = self.__read_eeprom_specific_bytes((offset + SFP_MODULE_THRESHOLD_OFFSET), SFP_MODULE_THRESHOLD_WIDTH)
            if dom_module_threshold_raw is not None:
                dom_module_threshold_data = sfpd_obj.parse_alarm_warning_threshold(dom_module_threshold_raw, 0)

                transceiver_dom_threshold_info_dict['temphighalarm']      = dom_module_threshold_data['data']['TempHighAlarm']['value']
                transceiver_dom_threshold_info_dict['templowalarm']       = dom_module_threshold_data['data']['TempLowAlarm']['value']
                transceiver_dom_threshold_info_dict['temphighwarning']    = dom_module_threshold_data['data']['TempHighWarning']['value']
                transceiver_dom_threshold_info_dict['templowwarning']     = dom_module_threshold_data['data']['TempLowWarning']['value']

                transceiver_dom_threshold_info_dict['vcchighalarm']       = dom_module_threshold_data['data']['VoltageHighAlarm']['value']
                transceiver_dom_threshold_info_dict['vcclowalarm']        = dom_module_threshold_data['data']['VoltageLowAlarm']['value']
                transceiver_dom_threshold_info_dict['vcchighwarning']     = dom_module_threshold_data['data']['VoltageHighWarning']['value']
                transceiver_dom_threshold_info_dict['vcclowwarning']      = dom_module_threshold_data['data']['VoltageLowWarning']['value']

                transceiver_dom_threshold_info_dict['txbiashighalarm']    = dom_module_threshold_data['data']['BiasHighAlarm']['value']
                transceiver_dom_threshold_info_dict['txbiaslowalarm']     = dom_module_threshold_data['data']['BiasLowAlarm']['value']
                transceiver_dom_threshold_info_dict['txbiashighwarning']  = dom_module_threshold_data['data']['BiasHighWarning']['value']
                transceiver_dom_threshold_info_dict['txbiaslowwarning']   = dom_module_threshold_data['data']['BiasLowWarning']['value']

                transceiver_dom_threshold_info_dict['txpowerhighalarm']   = dom_module_threshold_data['data']['TXPowerHighAlarm']['value']
                transceiver_dom_threshold_info_dict['txpowerlowalarm']    = dom_module_threshold_data['data']['TXPowerLowAlarm']['value']
                transceiver_dom_threshold_info_dict['txpowerhighwarning'] = dom_module_threshold_data['data']['TXPowerHighWarning']['value']
                transceiver_dom_threshold_info_dict['txpowerlowwarning']  = dom_module_threshold_data['data']['TXPowerLowWarning']['value']

                transceiver_dom_threshold_info_dict['rxpowerhighalarm']   = dom_module_threshold_data['data']['RXPowerHighAlarm']['value']
                transceiver_dom_threshold_info_dict['rxpowerlowalarm']    = dom_module_threshold_data['data']['RXPowerLowAlarm']['value']
                transceiver_dom_threshold_info_dict['rxpowerhighwarning'] = dom_module_threshold_data['data']['RXPowerHighWarning']['value']
                transceiver_dom_threshold_info_dict['rxpowerlowwarning']  = dom_module_threshold_data['data']['RXPowerLowWarning']['value']

            for key in transceiver_dom_threshold_info_dict:
                transceiver_dom_threshold_info_dict[key] = self._convert_string_to_num(transceiver_dom_threshold_info_dict[key])

            return transceiver_dom_threshold_info_dict

        else:
            return None

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        # SFP doesn't support this feature
        if self.sfp_type == SFP_TYPE:
            return False
        elif self.sfp_type == QSFP_TYPE:
            try:
                reg_file = open("/sys/class/gpio/gpio"
                    +str((self.port_num-self.QSFP_START)*4+32)+"/value")
            except IOError as e:
                print ("Error: unable to open file: %s" % str(e))
                return None
            reg_value = int(reg_file.readline().rstrip())
            reg_file.close()
            if reg_value == 0:
                return True
            else:
                return False
        else:
            return None

    def get_rx_los(self):
        """
        Retrieves the RX LOS (lost-of-signal) status of SFP
        Returns:
            A Boolean, True if SFP has RX LOS, False if not.
            Note : RX LOS status is latched until a call to get_rx_los or a reset.
        """
        if not self.dom_supported:
            return None

        rx_los_list = []

        if self.sfp_type == QSFP_TYPE:
            offset = 0
            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                (offset + QSFP_CHANNL_RX_LOS_STATUS_OFFSET), QSFP_CHANNL_RX_LOS_STATUS_WIDTH)
            if dom_channel_monitor_raw is not None:
                rx_los_data = int(dom_channel_monitor_raw[0], 16)
                rx_los_list.append(rx_los_data & 0x01 != 0)
                rx_los_list.append(rx_los_data & 0x02 != 0)
                rx_los_list.append(rx_los_data & 0x04 != 0)
                rx_los_list.append(rx_los_data & 0x08 != 0)
        elif self.sfp_type == SFP_TYPE:
            offset = 256
            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                (offset + SFP_STATUS_CONTROL_OFFSET), SFP_STATUS_CONTROL_WIDTH)
            if dom_channel_monitor_raw is not None:
                rx_los_data = int(dom_channel_monitor_raw[0], 16)
                rx_los_list.append(rx_los_data & 0x02 != 0)
            else:
                return None
        else:
            return None

        return rx_los_list

    def get_tx_fault(self):
        """
        Retrieves the TX fault status of SFP
        Returns:
            A Boolean, True if SFP has TX fault, False if not
            Note : TX fault status is lached until a call to get_tx_fault or a reset.
        """
        if not self.dom_supported:
            return None

        tx_fault_list = []

        if self.sfp_type == QSFP_TYPE:
            offset = 0
            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                (offset + QSFP_CHANNL_TX_FAULT_STATUS_OFFSET), QSFP_CHANNL_TX_FAULT_STATUS_WIDTH)
            if dom_channel_monitor_raw is not None:
                tx_fault_data = int(dom_channel_monitor_raw[0], 16)
                tx_fault_list.append(tx_fault_data & 0x01 != 0)
                tx_fault_list.append(tx_fault_data & 0x02 != 0)
                tx_fault_list.append(tx_fault_data & 0x04 != 0)
                tx_fault_list.append(tx_fault_data & 0x08 != 0)
        elif self.sfp_type == SFP_TYPE:
            offset = 256
            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                (offset + SFP_STATUS_CONTROL_OFFSET), SFP_STATUS_CONTROL_WIDTH)
            if dom_channel_monitor_raw is not None:
                tx_fault_data = int(dom_channel_monitor_raw[0], 16)
                tx_fault_list.append(tx_fault_data & 0x04 != 0)
            else:
                return None
        else:
            return None

        return tx_fault_list

    def get_tx_disable(self):
        """
        Retrieves the tx_disable status of this SFP
        Returns:
            A Boolean, True if tx_disable is enabled, False if disabled
        """
        if not self.dom_supported:
            return None

        tx_disable_list = []

        if self.sfp_type == QSFP_TYPE:
            offset = 0
            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                (offset + QSFP_CHANNL_DISABLE_STATUS_OFFSET), QSFP_CHANNL_DISABLE_STATUS_WIDTH)
            if dom_channel_monitor_raw is not None:
                tx_disable_data = int(dom_channel_monitor_raw[0], 16)
                tx_disable_list.append(tx_disable_data & 0x01 != 0)
                tx_disable_list.append(tx_disable_data & 0x02 != 0)
                tx_disable_list.append(tx_disable_data & 0x04 != 0)
                tx_disable_list.append(tx_disable_data & 0x08 != 0)
        elif self.sfp_type == SFP_TYPE:
            offset = 256
            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                (offset + SFP_STATUS_CONTROL_OFFSET), SFP_STATUS_CONTROL_WIDTH)
            if dom_channel_monitor_raw is not None:
                tx_disable_data = int(dom_channel_monitor_raw[0], 16)
                tx_disable_list.append(tx_disable_data & 0xC0 != 0)
            else:
                return None
        else:
            return None

        return tx_disable_list

    def get_tx_disable_channel(self):
        """
        Retrieves the TX disabled channels in this SFP
        Returns:
            A hex of 4 bits (bit 0 to bit 3 as channel 0 to channel 3) to represent
            TX channels which have been disabled in this SFP.
            As an example, a returned value of 0x5 indicates that channel 0
            and channel 2 have been disabled.
        """
        # SFP doesn't support this feature
        if self.sfp_type == SFP_TYPE:
            return 0
        elif self.sfp_type == QSFP_TYPE:
            tx_disable_list = self.get_tx_disable()
            if tx_disable_list is None:
                return 0
            tx_disabled = 0
            for i in range(len(tx_disable_list)):
                if tx_disable_list[i]:
                    tx_disabled |= 1 << i
        else:
            return None

        return tx_disabled

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this QSFP module
        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        # SFP doesn't support this feature
        if self.sfp_type == SFP_TYPE:
            return False
        elif self.sfp_type == QSFP_TYPE:
            try:
                reg_file = open("/sys/class/gpio/gpio"
                    +str((self.port_num-self.QSFP_START)*4+35)+"/value")
            except IOError as e:
                print ("Error: unable to open file: %s" % str(e))
                return False
            reg_value = int(reg_file.readline().rstrip())
            reg_file.close()
            if reg_value == 0:
                return False
            else:
                return True
        else:
            return None

    def get_power_override(self):
        """
        Retrieves the power-override status of this SFP
        Returns:
            A Boolean, True if power-override is enabled, False if disabled
        """
		# SFP doesn't support this feature
        if self.sfp_type == SFP_TYPE:
            return False
        elif self.sfp_type == QSFP_TYPE:
            offset = 0
            sfpd_obj = sff8436Dom()
            if sfpd_obj is None:
                return False

            dom_control_raw = self.__read_eeprom_specific_bytes(
                (offset + QSFP_POWEROVERRIDE_OFFSET), QSFP_POWEROVERRIDE_WIDTH)
            if dom_control_raw is not None:
                if int(dom_control_raw[0],16) & (0x01 << QSFP_POWEROVERRIDE_BIT):
                    return True
                else:
                    return False
        else:
            return None

    def get_temperature(self):
        """
        Retrieves the temperature of this SFP
        Returns:
            An integer number of current temperature in Celsius
        """
        if not self.dom_supported:
            return None
        if self.sfp_type == QSFP_TYPE:
            offset = 0

            sfpd_obj = sff8436Dom()
            if sfpd_obj is None:
                return None

            if self.dom_temp_supported:
                dom_temperature_raw = self.__read_eeprom_specific_bytes(
                    (offset + QSFP_TEMPE_OFFSET), QSFP_TEMPE_WIDTH)
                if dom_temperature_raw is not None:
                    dom_temperature_data = sfpd_obj.parse_temperature(dom_temperature_raw, 0)
                    temp = self._convert_string_to_num(
                        dom_temperature_data['data']['Temperature']['value'])
                    return temp
                else:
                    return None
        elif self.sfp_type == SFP_TYPE:
            offset = 256
            sfpd_obj = sff8472Dom()
            if sfpd_obj is None:
                return None
            sfpd_obj._calibration_type = 1

            dom_temperature_raw = self.__read_eeprom_specific_bytes(
                (offset + SFP_TEMPE_OFFSET), SFP_TEMPE_WIDTH)
            if dom_temperature_raw is not None:
                dom_temperature_data = sfpd_obj.parse_temperature(dom_temperature_raw, 0)
                temp = self._convert_string_to_num(
                    dom_temperature_data['data']['Temperature']['value'])
                return temp
            else:
                return None
        else:
            return None

    def get_voltage(self):
        """
        Retrieves the supply voltage of this SFP
        Returns:
            An integer number of supply voltage in mV
        """
        if not self.dom_supported:
            return None
        if self.sfp_type == QSFP_TYPE:
            offset = 0

            sfpd_obj = sff8436Dom()
            if sfpd_obj is None:
                return None

            if self.dom_volt_supported:
                dom_voltage_raw = self.__read_eeprom_specific_bytes(
                    (offset + QSFP_VOLT_OFFSET), QSFP_VOLT_WIDTH)
                if dom_voltage_raw is not None:
                    dom_voltage_data = sfpd_obj.parse_voltage(dom_voltage_raw, 0)
                    voltage = self._convert_string_to_num(
                        dom_voltage_data['data']['Vcc']['value'])
                    return voltage
                else:
                    return None
        elif self.sfp_type == SFP_TYPE:
            offset = 256

            sfpd_obj = sff8472Dom()
            if sfpd_obj is None:
                return None

            sfpd_obj._calibration_type = self.calibration

            dom_voltage_raw = self.__read_eeprom_specific_bytes(
                (offset + SFP_VOLT_OFFSET), SFP_VOLT_WIDTH)
            if dom_voltage_raw is not None:
                dom_voltage_data = sfpd_obj.parse_voltage(dom_voltage_raw, 0)
                voltage = self._convert_string_to_num(
                    dom_voltage_data['data']['Vcc']['value'])
                return voltage
            else:
                return None
        else:
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
        if self.sfp_type == QSFP_TYPE:
            offset = 0

            sfpd_obj = sff8436Dom()
            if sfpd_obj is None:
                return None

            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                (offset + QSFP_CHANNL_MON_OFFSET), QSFP_CHANNL_MON_WITH_TX_POWER_WIDTH)
            if dom_channel_monitor_raw is not None:
                dom_channel_monitor_data = \
                    sfpd_obj.parse_channel_monitor_params_with_tx_power(dom_channel_monitor_raw, 0)
                tx_bias_list.append(self._convert_string_to_num(
                    dom_channel_monitor_data['data']['TX1Bias']['value']))
                tx_bias_list.append(self._convert_string_to_num(
                    dom_channel_monitor_data['data']['TX2Bias']['value']))
                tx_bias_list.append(self._convert_string_to_num(
                    dom_channel_monitor_data['data']['TX3Bias']['value']))
                tx_bias_list.append(self._convert_string_to_num(
                    dom_channel_monitor_data['data']['TX4Bias']['value']))
        elif self.sfp_type == SFP_TYPE:
            offset = 256

            sfpd_obj = sff8472Dom()
            if sfpd_obj is None:
                return None
            sfpd_obj._calibration_type = self.calibration

            if self.dom_supported:
                dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                    (offset + SFP_CHANNL_MON_OFFSET), SFP_CHANNL_MON_WIDTH)
                if dom_channel_monitor_raw is not None:
                    dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params( \
                        dom_channel_monitor_raw, 0)
                    tx_bias_list.append(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['TXBias']['value']))
                else:
                    return None
            else:
                return None
        else:
            return None

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

        if self.sfp_type == QSFP_TYPE:
            offset = 0

            sfpd_obj = sff8436Dom()
            if sfpd_obj is None:
                return None

            if self.dom_rx_power_supported:
                dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                    (offset + QSFP_CHANNL_MON_OFFSET), QSFP_CHANNL_MON_WITH_TX_POWER_WIDTH)
                if dom_channel_monitor_raw is not None:
                    dom_channel_monitor_data = \
                        sfpd_obj.parse_channel_monitor_params_with_tx_power(dom_channel_monitor_raw, 0)
                    rx_power_list.append(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['RX1Power']['value']))
                    rx_power_list.append(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['RX2Power']['value']))
                    rx_power_list.append(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['RX3Power']['value']))
                    rx_power_list.append(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['RX4Power']['value']))
                else:
                    return None
            else:
                return None
        elif self.sfp_type == SFP_TYPE:
            offset = 256

            sfpd_obj = sff8472Dom()
            if sfpd_obj is None:
                return None

            if self.dom_supported:
                sfpd_obj._calibration_type = self.calibration

                dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                    (offset + SFP_CHANNL_MON_OFFSET), SFP_CHANNL_MON_WIDTH)
                if dom_channel_monitor_raw is not None:
                    dom_channel_monitor_data = \
                        sfpd_obj.parse_channel_monitor_params(dom_channel_monitor_raw, 0)
                    rx_power_list.append(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['RXPower']['value']))
                else:
                    return None
            else:
                return None
        else:
            return None

        return rx_power_list

    def get_tx_power(self):
        """
        Retrieves the TX power of this SFP
        Returns:
            A list of four integer numbers, representing TX power in mW
            for channel 0 to channel 4.
            Ex. ['1.86', '1.86', '1.86', '1.86']
        """
        tx_power_list = []

        if self.sfp_type == QSFP_TYPE:
            offset = 0

            sfpd_obj = sff8436Dom()
            if sfpd_obj is None:
                return None

            if self.dom_tx_power_supported:
                dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                    (offset + QSFP_CHANNL_MON_OFFSET), QSFP_CHANNL_MON_WITH_TX_POWER_WIDTH)
                if dom_channel_monitor_raw is not None:
                    dom_channel_monitor_data = \
                        sfpd_obj.parse_channel_monitor_params_with_tx_power(dom_channel_monitor_raw, 0)
                    tx_power_list.append(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['TX1Power']['value']))
                    tx_power_list.append(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['TX2Power']['value']))
                    tx_power_list.append(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['TX3Power']['value']))
                    tx_power_list.append(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['TX4Power']['value']))
                else:
                    return None
            else:
                return None
        elif self.sfp_type == SFP_TYPE:
            offset = 256
            sfpd_obj = sff8472Dom()
            if sfpd_obj is None:
                return None

            if self.dom_supported:
                sfpd_obj._calibration_type = self.calibration

                dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                    (offset + SFP_CHANNL_MON_OFFSET), SFP_CHANNL_MON_WIDTH)
                if dom_channel_monitor_raw is not None:
                    dom_channel_monitor_data = \
                        sfpd_obj.parse_channel_monitor_params(dom_channel_monitor_raw, 0)
                    tx_power_list.append(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['TXPower']['value']))
                else:
                    return None
            else:
                return None
        else:
            return None

        return tx_power_list

    def reset(self):
        """
        Reset SFP and return all user module settings to their default state.
        Returns:
            A boolean, True if successful, False if not
        """
        if not self.get_presence():
            return False

        # SFP doesn't support this feature
        if self.sfp_type == SFP_TYPE:
            return False
        elif self.sfp_type == QSFP_TYPE:
            try:
                with open("/sys/class/gpio/gpio"+str((self.port_num-self.QSFP_START)*4+32)+"/direction", 'r+') as f:
                    f.write('out')
                with open("/sys/class/gpio/gpio"+str((self.port_num-self.QSFP_START)*4+32)+"/value", "r+") as f:
                    f.write('0')
            except IOError as e:
                print ("Error: unable to open file: %s" % str(e))
                return False

            # Sleep 2 second to allow it to settle
            time.sleep(2)

            # Flip the value back write back to the register to take port out of reset
            try:
                with open("/sys/class/gpio/gpio"+str((self.port_num-self.QSFP_START)*4+32)+"/value", "r+") as f:
                    f.write('1')
            except IOError as e:
                print ("Error: unable to open file: %s" % str(e))
                return False
        else:
            return None

        return True

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

        if self.sfp_type == SFP_TYPE:
            if self.dom_tx_disable_supported:
                offset = 256
                sysfs_sfp_i2c_client_eeprom_path = self.port_to_eeprom_mapping[self.port_num]
                status_control_raw = self.__read_eeprom_specific_bytes(
                    (offset + SFP_STATUS_CONTROL_OFFSET), SFP_STATUS_CONTROL_WIDTH)
                if status_control_raw is not None:
                    # Set bit 6 for Soft TX Disable Select
                    # 01000000 = 64 and 10111111 = 191
                    tx_disable_bit = 64 if tx_disable else 191
                    status_control = int(status_control_raw[0], 16)
                    tx_disable_ctl = (status_control | tx_disable_bit) if tx_disable else (
                        status_control & tx_disable_bit)
                    try:
                        sysfsfile_eeprom = open(
                            sysfs_sfp_i2c_client_eeprom_path, mode="r+b", buffering=0)
                        buffer = create_string_buffer(1)
                        buffer[0] = tx_disable_ctl
                        # Write to eeprom
                        sysfsfile_eeprom.seek(offset + SFP_STATUS_CONTROL_OFFSET)
                        sysfsfile_eeprom.write(buffer[0])
                    except IOError as e:
                        print("Error: unable to open file: %s" % str(e))
                        return False
                    finally:
                        if sysfsfile_eeprom:
                            sysfsfile_eeprom.close()
                            time.sleep(0.01)
                    return True
                return False
            else:
                return False
        elif self.sfp_type == QSFP_TYPE:
            if self.dom_tx_disable_supported:
                channel_mask = 0x0f
                if tx_disable:
                    return self.tx_disable_channel(channel_mask, True)
                else:
                    return self.tx_disable_channel(channel_mask, False)
            else:
                return False
        else:
            return None

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
        if not self.get_presence():
            return False
        # SFP doesn't support this feature
        if self.sfp_type == SFP_TYPE:
            return False
        elif self.sfp_type == QSFP_TYPE:
            if self.dom_tx_disable_supported:
                sysfsfile_eeprom = None
                try:
                    channel_state = self.get_tx_disable_channel()
                    if disable:
                        tx_disable_ctl = channel_state | channel
                    else:
                        tx_disable_ctl = channel_state & (~channel)
                    buffer = create_string_buffer(1)
                    buffer[0] = tx_disable_ctl
                    # Write to eeprom
                    sysfsfile_eeprom = open(
                        self.port_to_eeprom_mapping[self.port_num], "r+b")
                    sysfsfile_eeprom.seek(QSFP_CONTROL_OFFSET)
                    sysfsfile_eeprom.write(buffer[0])
                except IOError as e:
                    print ("Error: unable to open file: %s" % str(e))
                    return False
                finally:
                    if sysfsfile_eeprom is not None:
                        sysfsfile_eeprom.close()
                        time.sleep(0.01)
                return True
            else:
                return False
        else:
            return None

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
        # SFP doesn't support this feature
        if self.sfp_type == SFP_TYPE:
            return False
        elif self.sfp_type == QSFP_TYPE:
            try:
                reg_file = open("/sys/class/gpio/gpio"
                    +str((self.port_num-self.QSFP_START)*4+35)+"/value", "r+")
            except IOError as e:
                print ("Error: unable to open file: %s" % str(e))
                return False

            # LPMode is active high; set or clear the bit accordingly
            if lpmode:
                reg_value = 1
            else:
                reg_value = 0

            reg_file.write(hex(reg_value))
            reg_file.close()
        else:
            return None

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
        # SFP doesn't support this feature
        if not self.get_presence():
            return False

        if self.sfp_type == SFP_TYPE:
            return False
        elif self.sfp_type == QSFP_TYPE:
            try:
                power_override_bit = 0
                if power_override:
                    power_override_bit |= 1 << 0

                power_set_bit = 0
                if power_set:
                    power_set_bit |= 1 << 1

                buffer = create_string_buffer(1)
                buffer[0] = power_override_bit | power_set_bit
                # Write to eeprom
                sysfsfile_eeprom = open(self.port_to_eeprom_mapping[self.port_num], "r+b")
                sysfsfile_eeprom.seek(QSFP_POWEROVERRIDE_OFFSET)
                sysfsfile_eeprom.write(buffer[0])
            except IOError as e:
                print ("Error: unable to open file: %s" % str(e))
                return False
            finally:
                if sysfsfile_eeprom is not None:
                    sysfsfile_eeprom.close()
                    time.sleep(0.01)
        else:
            return None

        return True

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        sfputil_helper = SfpUtilHelper()
        sfputil_helper.read_porttab_mappings(self.__get_path_to_port_config_file())
        print ("self.index{}".format(self.index))
        name = sfputil_helper.logical[self.index-1] or "Unknown"
        return name

