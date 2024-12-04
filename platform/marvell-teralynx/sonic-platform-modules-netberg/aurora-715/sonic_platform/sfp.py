#!/usr/bin/env python

#############################################################################
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSUs status which are available in the platform
#
#############################################################################

try:
    from sonic_platform_base.sfp_base import SfpBase
    from sonic_py_common.logger import Logger
    from sonic_platform_base.sonic_sfp.sff8472 import sff8472InterfaceId
    from sonic_platform_base.sonic_sfp.sff8472 import sff8472Dom
    from sonic_platform_base.sonic_sfp.sff8436 import sff8436InterfaceId
    from sonic_platform_base.sonic_sfp.sff8436 import sff8436Dom
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

logger = Logger("sfp")

#############################################################################
#
#  SFP :
#     2 wire address 1010000x (A0h)  (i2c 0x50)
#     ------------------------------------------------ 0
#     |    Serial ID Defined by SFP MSA (96 bytes)   |      <-- get_transceiver_info
#     |----------------------------------------------| 95
#     |    Vendor Specific              (32 bytes)   |
#     |----------------------------------------------| 127
#     |    Reserved, SFF-8079           (128 byte)   |
#     ------------------------------------------------ 255
#
#     2 wire address 1010001x (A2h)  (i2c 0x51)
#     ------------------------------------------------ 0
#     |    Alarm and Warning Threshold  (56 bytes)   |      <-- get_transceiver_threshold_info
#     |----------------------------------------------| 55
#     |    Cal Constants                (40 bytes)   |
#     |----------------------------------------------| 95
#     |    Real Time Diagnostic Interface(24 bytes)  |      <-- get_transceiver_bulk_status
#     |----------------------------------------------| 119
#     |    Vendor Specific              (7  bytes)   |
#     |----------------------------------------------| 126
#     |    Page Select Byte             (Optional)   |      -> select one of the below pages as
#     ------------------------------------------------ 127     address 128-255
#
#     Page 00h/01h
#     |----------------------------------------------| 128
#     |    User Writeable EEPROM         (120 bytes) |
#     |----------------------------------------------| 247
#     |    Vendor Specific                (8  bytes) |
#     ------------------------------------------------ 255
#     Page 02h
#     |----------------------------------------------| 128
#     |    Control Function              (128 bytes) |
#     ------------------------------------------------ 255
#     Page 03h-7Fh
#     |----------------------------------------------| 128
#     |    Reserved                      (128 bytes) |
#     ------------------------------------------------ 255
#     Page 80h-FFh
#     |----------------------------------------------| 128
#     |    Vendor Specific               (128 bytes) |
#     ------------------------------------------------ 255
#
# ==========================================================
#  QSFP :
#     2 wire address 1010000x (A0h)  (i2c 0x50)
#
#     ------------------------------------------------ 0
#     |    ID and status                 (3 bytes)   |
#     |----------------------------------------------| 2
#     |    Interrupt Flags              (19 bytes)   |
#     |----------------------------------------------| 21
#     |    Module Monitors              (12 bytes)   |      <-- get_transceiver_bulk_status
#     |----------------------------------------------| 33
#     |    Channel Monitors             (12 bytes)   |
#     |----------------------------------------------| 81
#     |    Reserved                      (4 bytes)   |
#     |----------------------------------------------| 85
#     |    Control                       (12 bytes)  |      <-  get_power_override
#     |----------------------------------------------| 97
#     |    ~~~                                       |
#     |----------------------------------------------| 126
#     |    Page Select Byte                          |      -> select one of the below pages as
#     ------------------------------------------------ 127     address 128-255
#
#
#     Page 00h
#     |----------------------------------------------| 128
#     |    Base ID Filed                 (64 bytes)  |      <-- get_transceiver_info
#     |----------------------------------------------| 191
#     |    Extended ID                   (32 bytes)  |
#     |----------------------------------------------| 223
#     |    Vendor Specific ID            (8  bytes)  |
#     ------------------------------------------------ 255
#     Page 01h (optional)
#     |----------------------------------------------| 128
#     |    ~~~~                          (128 bytes) |
#     ------------------------------------------------ 255
#     Page 02h (optional)
#     |----------------------------------------------| 128
#     |    User EEPROM Data              (128 bytes) |
#     ------------------------------------------------ 255

#     Page 03h (optional on QSFP28)
#     |----------------------------------------------| 128
#     |    Module Threshold              (48 bytes)  |      <-- get_transceiver_threshold_info
#     |----------------------------------------------| 175
#     |    Channel Threshold             (48 bytes)  |
#     |----------------------------------------------| 223
#     |    Reserved                       (2 bytes)  |
#     |----------------------------------------------| 225
#     | Vendor Specific Channel Controls (16 bytes)  |
#     |----------------------------------------------| 241
#     |    Channel Monitor Masks         (12 bytes)  |
#     |----------------------------------------------| 253
#     |    Reserved                       (2 bytes)  |
#     ------------------------------------------------ 255
#
#############################################################################

# function eunm
SFP_GET_PRESENCE = 0
SFP_RESET = 1
SFP_GET_LOW_POWER_MODE = 2
SFP_SET_LOW_POWER_MODE = 3

# XCVR type definition
SFP_TYPE = 0
QSFP_TYPE = 1

SFP_TYPE_CODE_LIST = [
    '03'  # SFP/SFP+/SFP28
]
QSFP_TYPE_CODE_LIST = [
    '0d',  # QSFP+ or later
    '11'  # QSFP28 or later
]

# index for A0H, A2H sysfs file path
INDEX_A0H = 0
INDEX_A2H = 1

# offset/width definition
OFFSET = 0
WIDTH = 1

QSFP_UPPER_MEMORY_PAGE00_OFFSET = 128
QSFP_UPPER_MEMORY_PAGE03_OFFSET = 512

SFP_A0H_OFFSET = 0
SFP_A2H_OFFSET = 0

XCVR_DOM_CAPABILITY_OFFSET = 92
XCVR_DOM_CAPABILITY_WIDTH = 2
ID_FIELD_WIDTH = 92

QSFP_VERSION_COMPLIANCE_OFFSET = 1
QSFP_VERSION_COMPLIANCE_WIDTH = 1
QSFP_INTERFACE_BULK_WIDTH = 20
QSFP_OPTION_VALUE_OFFSET = 192
QSFP_OPTION_VALUE_WIDTH = 4
QSFP_ID_FIELDS = {
    # NAME : [OFFSET:WIDTH]
    'TYPE':  [0, 1],
    'EXTID': [1, 1],
    'CONNECTOR': [2, 1],
    'XCVR_COMPLIANCE': [3, 8],
    'ENCODING': [11, 1],
    'NORMAL_BITRATE': [12, 1],
    'EXT_RATE_SEL_COMPLIANCE': [13, 1],
    'CABLE_LEN': [14, 5],
    'DEVICE_TECH': [19, 1],
    'VENDOR_NAME': [20, 16],
    'EXT_XCVR_CODE': [36, 1],
    'VENDOR_OUI': [37, 3],
    'VENDOR_PN': [40, 16],
    'VENDOR_REV': [56, 2],
    'WAVELENGTH': [58, 2],
    'WAVELENGTH_TOLERANCE': [60, 2],
    'MAX_CASE_TEMP': [62, 1],
    'CC_BASE': [63, 1],
    'OPTIONS': [64, 4],
    'VENDOR_SN': [68, 16],
    'VENDOR_DATE': [84, 8],
    # DOM
    'TEMPERATURE': [22, 2],
    'VOLTAGE': [26, 2],
    'CHANNEL_MON': [34, 24],
    'CONTROL': [86, 12],
    # PAGE03
    'MODULE_THRESHOLD': [0, 24],
    'CHANNEL_THRESHOLD': [50, 24]
}
SFP_INTERFACE_BULK_WIDTH = 21
SFP_ID_FIELDS = {
    # NAME : [OFFSET:WIDTH]
    'TYPE':  [0, 1],
    'EXTID': [1, 1],
    'CONNECTOR': [2, 1],
    'XCVR_COMPLIANCE': [3, 8],
    'ENCODING': [11, 1],
    'NORMAL_BITRATE': [12, 1],
    'EXT_RATE_SEL_COMPLIANCE': [13, 1],
    'CABLE_LEN': [14, 6],
    'VENDOR_NAME': [20, 16],
    'EXT_XCVR_CODE': [36, 1],
    'VENDOR_OUI': [37, 3],
    'VENDOR_PN': [40, 16],
    'VENDOR_REV': [56, 4],
    'WAVELENGTH': [60, 2],
    # UNALLOCATED WIDDTH: 1
    'CC_BASE': [63, 1],
    'OPTIONS': [64, 2],
    'BITRATE_MAX': [66, 1],
    # BITRATE_MAX ??
    'BITRATE_MAX_1': [67, 1],
    'VENDOR_SN': [68, 16],
    'VENDOR_DATE': [84, 8],
    # DOM
    'TEMPERATURE': [96, 2],
    'VOLTAGE': [98, 2],
    'CHANNEL_MON': [100, 6],
    'MODULE_THRESHOLD': [0, 40]
}


qsfp_cable_length_tup = ('Length(km)', 'Length OM3(2m)',
                         'Length OM2(m)', 'Length OM1(m)',
                         'Length Cable Assembly(m)')

sfp_cable_length_tup = ('LengthSMFkm-UnitsOfKm', 'LengthSMF(UnitsOf100m)',
                        'Length50um(UnitsOf10m)', 'Length62.5um(UnitsOfm)',
                        'LengthCable(UnitsOfm)', 'LengthOM3(UnitsOf10m)')

sfp_compliance_code_tup = ('10GEthernetComplianceCode', 'InfinibandComplianceCode',
                           'ESCONComplianceCodes', 'SONETComplianceCodes',
                           'EthernetComplianceCodes', 'FibreChannelLinkLength',
                           'FibreChannelTechnology', 'SFP+CableTechnology',
                           'FibreChannelTransmissionMedia', 'FibreChannelSpeed')

qsfp_compliance_code_tup = ('10/40G Ethernet Compliance Code', 'SONET Compliance codes',
                            'SAS/SATA compliance codes', 'Gigabit Ethernet Compliant codes',
                            'Fibre Channel link length/Transmitter Technology',
                            'Fibre Channel transmission media', 'Fibre Channel Speed')


class Sfp(SfpBase):
    """Platform-specific Sfp class"""

    def __init__(self, index, eeprom_path_list, sfp_type, ext_sysfile_list=None):
        # index: port index, start from 0
        # eeprom_path_list : a list of path to eeprom sysfile
        #   [0]: for 0x50
        #   [1]: for 0x51
        # ext_sysfile_list: used to get other function of sfp
        #   [0]: present
        #   [1]: reset
        #   [2]: get lowpower mode
        #   [3]: set lowpower mode
        # ext_sysfile_list[0]: QSFP path
        # ext_sysfile_list[1]: SFP  path
        self.index = index
        self.eeprom_path_list = eeprom_path_list
        # self._get_sfp_type(sfp_type)
        self.sfp_type = sfp_type
        if self.sfp_type == QSFP_TYPE:
            self.present_file = ext_sysfile_list[0] + \
                'qsfp{}_present'.format(self.index)
            self.reset_file = ext_sysfile_list[0] + \
                'qsfp{}_reset'.format(self.index)
            self.lp_file = ext_sysfile_list[0] + \
                'qsfp{}_low_power'.format(self.index)
        else:
            self.present_file = ext_sysfile_list[1] + \
                'sfp{}_present'.format(self.index)
            self.reset_file = None
            self.lp_file = None

    def _get_sfp_type(self, sfp_type):
        ty = self._read_eeprom_bytes(
            SFP_ID_FIELDS['TYPE'][OFFSET], SFP_ID_FIELDS['TYPE'][WIDTH], INDEX_A0H)
        if ty is not None:
            if ty[0] in SFP_TYPE_CODE_LIST:
                self.sfp_type = SFP_TYPE
            if ty[0] in QSFP_TYPE_CODE_LIST:
                self.sfp_type = QSFP_TYPE
            else:
                self.sfp_type = sfp_type
                logger.log_warning("Unreganized sfp type of module {} . unsupported, treated as specified type {}".format(
                    self.index, sfp_type))
        else:
            self.sfp_type = sfp_type

    def _read_eeprom_bytes(self, offset, num_bytes, path_idx):
        eeprom_raw = []

        eeprom = None

        for i in range(0, num_bytes):
            eeprom_raw.append("0x00")

        try:
            eeprom = open(
                self.eeprom_path_list[path_idx], mode="rb", buffering=0)
            eeprom.seek(offset)
            raw_data = eeprom.read(num_bytes)
            for nb in range(0, len(raw_data)):
                eeprom_raw[nb] = hex(ord(raw_data[nb]))[2:].zfill(2)
        except Exception as ex:
            logger.log_error("Fail to read eeprom {}".format(
                self.eeprom_path_list[path_idx]))
            logger.log_error("  {}".format(ex))
            if eeprom is not None:
                eeprom.close()
            return None

        return eeprom_raw

    def __read_attr_file(self, filepath, line=0xFF):
        try:
            with open(filepath, 'r') as fd:
                if line == 0xFF:
                    data = fd.read()
                    return data.rstrip('\r\n')
                else:
                    data = fd.readlines()
                    return data[line].rstrip('\r\n')
        except Exception as ex:
            logger.log_error(
                "Unable to open {} due to {}".format(filepath, repr(ex)))

        return None

    def __write_attr_file(self, filepath, data):
        try:
            with open(filepath, 'w') as fd:
                return fd.write(data)
        except Exception as ex:
            logger.log_error(
                "Unable to open {} due to {}".format(filepath, repr(ex)))
        return 0

    def get_presence(self):
        if self.present_file is not None:
            data = self.__read_attr_file(self.present_file)
            if data is not None:
                if int(data) == 1:
                    return True
        return False

    def _convert_string_to_num(self, value_str):
        if "-inf" in value_str:
            return '-inf'
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
            self.dom_tx_power_supported = False
            self.calibration = 0
            return

        if self.sfp_type == QSFP_TYPE:
            self.calibration = 1
            sfpi_obj = sff8436InterfaceId()
            if sfpi_obj is None:
                self.dom_supported = False
            offset = 128

            # QSFP capability byte parse, through this byte can know whether it support tx_power or not.
            # TODO: in the future when decided to migrate to support SFF-8636 instead of SFF-8436,
            # need to add more code for determining the capability and version compliance
            # in SFF-8636 dom capability definitions evolving with the versions.
            qsfp_dom_capability_raw = self._read_eeprom_bytes(
                (offset + XCVR_DOM_CAPABILITY_OFFSET), XCVR_DOM_CAPABILITY_WIDTH, INDEX_A0H)
            if qsfp_dom_capability_raw is not None:
                qsfp_version_compliance_raw = self._read_eeprom_bytes(
                    QSFP_VERSION_COMPLIANCE_OFFSET, QSFP_VERSION_COMPLIANCE_WIDTH, INDEX_A0H)
                qsfp_version_compliance = int(
                    qsfp_version_compliance_raw[0], 16)
                qspf_dom_capability = int(qsfp_dom_capability_raw[0], 16)
                if qsfp_version_compliance >= 0x08:
                    self.dom_temp_supported = (qspf_dom_capability & 0x20 != 0)
                    self.dom_volt_supported = (qspf_dom_capability & 0x10 != 0)
                    self.dom_rx_power_supported = (
                        qspf_dom_capability & 0x08 != 0)
                    self.dom_tx_power_supported = (
                        qspf_dom_capability & 0x04 != 0)
                else:
                    self.dom_temp_supported = True
                    self.dom_volt_supported = True
                    self.dom_rx_power_supported = (
                        qspf_dom_capability & 0x08 != 0)
                    self.dom_tx_power_supported = True
                self.dom_supported = True
                self.calibration = 1
                qsfp_option_value_raw = self._read_eeprom_bytes(
                    QSFP_OPTION_VALUE_OFFSET, QSFP_OPTION_VALUE_WIDTH, INDEX_A0H)
                if qsfp_option_value_raw is not None:
                    sfpd_obj = sff8436Dom()
                    if sfpd_obj is None:
                        return None
                    self.optional_capability = sfpd_obj.parse_option_params(
                        qsfp_option_value_raw, 0)
                    self.dom_tx_disable_supported = self.optional_capability[
                        'data']['TxDisable']['value'] == 'On'
            else:
                self.dom_supported = False
                self.dom_temp_supported = False
                self.dom_volt_supported = False
                self.dom_rx_power_supported = False
                self.dom_tx_power_supported = False
                self.calibration = 0
        elif self.sfp_type == SFP_TYPE:
            sfpi_obj = sff8472InterfaceId()
            if sfpi_obj is None:
                return None
            sfp_dom_capability_raw = self._read_eeprom_bytes(
                XCVR_DOM_CAPABILITY_OFFSET, XCVR_DOM_CAPABILITY_WIDTH, INDEX_A0H)
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
                self.dom_tx_disable_supported = (
                    int(sfp_dom_capability_raw[1], 16) & 0x40 != 0)
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
        serial                     |1*255VCHAR     |serial number of the SFP
        manufacturer               |1*255VCHAR     |SFP vendor name
        model                      |1*255VCHAR     |SFP model name
        connector                  |1*255VCHAR     |connector information
        encoding                   |1*255VCHAR     |encoding information
        ext_identifier             |1*255VCHAR     |extend identifier
        ext_rateselect_compliance  |1*255VCHAR     |extended rateSelect compliance
        cable_length               |INT            |cable length in m
        nominal_bit_rate           |INT            |nominal bit rate by 100Mbs
        specification_compliance   |1*255VCHAR     |specification compliance
        vendor_date                |1*255VCHAR     |vendor date
        vendor_oui                 |1*255VCHAR     |vendor OUI
        ========================================================================
         """
        transceiver_info_dict_keys = [
            'type',                      'vendor_rev',
            'serial',                 'manufacturer',
            'model',                 'connector',
            'encoding',                  'ext_identifier',
            'ext_rateselect_compliance', 'cable_type',
            'cable_length',              'nominal_bit_rate',
            'specification_compliance',  'vendor_date',
            'vendor_oui', 'application_advertisement']

        transceiver_info_dict = dict.fromkeys(
            transceiver_info_dict_keys, 'N/A')

        if self.sfp_type == QSFP_TYPE:
            field_offset = QSFP_UPPER_MEMORY_PAGE00_OFFSET  # upper memory map: Page 00h
            Id_field = QSFP_ID_FIELDS
            info_bulk_width = QSFP_INTERFACE_BULK_WIDTH
            sfpi_obj = sff8436InterfaceId()

        elif self.sfp_type == SFP_TYPE:
            # lower memory map: A0h (SFP i2c 0x50)
            field_offset = SFP_A0H_OFFSET
            Id_field = SFP_ID_FIELDS
            info_bulk_width = SFP_INTERFACE_BULK_WIDTH
            sfpi_obj = sff8472InterfaceId()
        else:
            logger.log_error("Unsupported sfp type")
            return None

        if sfpi_obj is None:
            logger.log_error("sfpi_obj create fail")
            return None

        # read Base ID field
        sfp_interface_bulk_raw = self._read_eeprom_bytes(
            field_offset, ID_FIELD_WIDTH, INDEX_A0H)
        if sfp_interface_bulk_raw is None:
            logger.log_error(
                " Fail to read BaseID field of module {}".format(self.index+1))
            return None

        start = 0
        end = start + info_bulk_width
        sfp_interface_bulk_data = sfpi_obj.parse_sfp_info_bulk(
            sfp_interface_bulk_raw[start: end], 0)

        start = Id_field['VENDOR_NAME'][OFFSET]
        end = start + Id_field['VENDOR_NAME'][WIDTH]
        sfp_vendor_name_data = sfpi_obj.parse_vendor_name(
            sfp_interface_bulk_raw[start: end], 0)

        start = Id_field['VENDOR_PN'][OFFSET]
        end = start + Id_field['VENDOR_PN'][WIDTH]
        sfp_vendor_pn_data = sfpi_obj.parse_vendor_pn(
            sfp_interface_bulk_raw[start: end], 0)

        start = Id_field['VENDOR_REV'][OFFSET]
        end = start + Id_field['VENDOR_REV'][WIDTH]
        sfp_vendor_rev_data = sfpi_obj.parse_vendor_rev(
            sfp_interface_bulk_raw[start: end], 0)

        start = Id_field['VENDOR_SN'][OFFSET]
        end = start + Id_field['VENDOR_SN'][WIDTH]
        sfp_vendor_sn_data = sfpi_obj.parse_vendor_sn(
            sfp_interface_bulk_raw[start: end], 0)

        start = Id_field['VENDOR_OUI'][OFFSET]
        end = start + Id_field['VENDOR_OUI'][WIDTH]
        sfp_vendor_oui_data = sfpi_obj.parse_vendor_oui(
            sfp_interface_bulk_raw[start: end], 0)

        start = Id_field['VENDOR_DATE'][OFFSET]
        end = start + Id_field['VENDOR_DATE'][WIDTH]
        sfp_vendor_date_data = sfpi_obj.parse_vendor_date(
            sfp_interface_bulk_raw[start: end], 0)

        compliance_code_dict = {}

        transceiver_info_dict['type'] = sfp_interface_bulk_data['data']['type']['value']
        transceiver_info_dict['manufacturer'] = sfp_vendor_name_data['data']['Vendor Name']['value']
        transceiver_info_dict['model'] = sfp_vendor_pn_data['data']['Vendor PN']['value']
        transceiver_info_dict['vendor_rev'] = sfp_vendor_rev_data['data']['Vendor Rev']['value']
        transceiver_info_dict['serial'] = sfp_vendor_sn_data['data']['Vendor SN']['value']
        transceiver_info_dict['vendor_oui'] = sfp_vendor_oui_data['data']['Vendor OUI']['value']
        transceiver_info_dict['vendor_date'] = sfp_vendor_date_data[
            'data']['VendorDataCode(YYYY-MM-DD Lot)']['value']
        transceiver_info_dict['connector'] = sfp_interface_bulk_data['data']['Connector']['value']
        transceiver_info_dict['encoding'] = sfp_interface_bulk_data['data']['EncodingCodes']['value']
        transceiver_info_dict['ext_identifier'] = sfp_interface_bulk_data['data']['Extended Identifier']['value']
        transceiver_info_dict['ext_rateselect_compliance'] = sfp_interface_bulk_data['data']['RateIdentifier']['value']

        if self.sfp_type == QSFP_TYPE:
            for key in qsfp_cable_length_tup:
                if key in sfp_interface_bulk_data['data']:
                    transceiver_info_dict['cable_type'] = key
                    transceiver_info_dict['cable_length'] = str(
                        sfp_interface_bulk_data['data'][key]['value'])

            for key in qsfp_compliance_code_tup:
                if key in sfp_interface_bulk_data['data']['Specification compliance']['value']:
                    compliance_code_dict[key] = sfp_interface_bulk_data['data']['Specification compliance']['value'][key]['value']
            transceiver_info_dict['specification_compliance'] = str(
                compliance_code_dict)
            transceiver_info_dict['nominal_bit_rate'] = str(
                sfp_interface_bulk_data['data']['Nominal Bit Rate(100Mbs)']['value'])
        else:
            for key in sfp_cable_length_tup:
                if key in sfp_interface_bulk_data['data']:
                    transceiver_info_dict['cable_type'] = key
                    transceiver_info_dict['cable_length'] = str(
                        sfp_interface_bulk_data['data'][key]['value'])

            for key in sfp_compliance_code_tup:
                if key in sfp_interface_bulk_data['data']['Specification compliance']['value']:
                    compliance_code_dict[key] = sfp_interface_bulk_data['data']['Specification compliance']['value'][key]['value']
            transceiver_info_dict['specification_compliance'] = str(
                compliance_code_dict)
            transceiver_info_dict['nominal_bit_rate'] = str(
                sfp_interface_bulk_data['data']['NominalSignallingRate(UnitsOf100Mbd)']['value'])

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
        transceiver_dom_info_dict_keys = [
            'temperature',  'voltage',
            'rx1power',     'rx2power',
            'rx3power',     'rx4power',
            'tx1bias',      'tx2bias',
            'tx3bias',      'tx4bias',
            'tx1power',     'tx2power',
            'tx3power',     'tx4power']

        transceiver_dom_info_dict = dict.fromkeys(
            transceiver_dom_info_dict_keys, 'N/A')

        path_idx = INDEX_A0H

        self._dom_capability_detect()
        if not self.dom_supported:
            return transceiver_dom_info_dict

        if self.sfp_type == QSFP_TYPE:
            field_offset = 0  # lower memory map: A0h
            Id_field = QSFP_ID_FIELDS
            sfpd_obj = sff8436Dom()
        elif self.sfp_type == SFP_TYPE:
            if self.eeprom_path_list[INDEX_A2H] is not 'n/a':
                # lower memory map: A2h (SFP i2c 0x51)
                field_offset = SFP_A2H_OFFSET
                path_idx = INDEX_A2H
            else:
                field_offset = 256
            Id_field = SFP_ID_FIELDS
            sfpd_obj = sff8472Dom()
        else:
            return None

        if sfpd_obj is None:
            return transceiver_dom_info_dict
        sfpd_obj._calibration_type = self.calibration

        dom_temperature_raw = self._read_eeprom_bytes(
            field_offset + Id_field['TEMPERATURE'][OFFSET], Id_field['TEMPERATURE'][WIDTH], path_idx)
        if dom_temperature_raw is not None:
            dom_temperature_data = sfpd_obj.parse_temperature(
                dom_temperature_raw, 0)
            transceiver_dom_info_dict['temperature'] = dom_temperature_data['data']['Temperature']['value']

        dom_voltage_raw = self._read_eeprom_bytes(
            field_offset + Id_field['VOLTAGE'][OFFSET], Id_field['VOLTAGE'][WIDTH], path_idx)
        if dom_voltage_raw is not None:
            dom_voltage_data = sfpd_obj.parse_voltage(dom_voltage_raw, 0)
            transceiver_dom_info_dict['voltage'] = dom_voltage_data['data']['Vcc']['value']

        dom_channel_monitor_raw = self._read_eeprom_bytes(
            field_offset + Id_field['CHANNEL_MON'][OFFSET], Id_field['CHANNEL_MON'][WIDTH], path_idx)
        if dom_channel_monitor_raw is not None:
            if self.sfp_type == QSFP_TYPE:
                dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params_with_tx_power(
                    dom_channel_monitor_raw, 0)
                transceiver_dom_info_dict['tx1power'] = dom_channel_monitor_data['data']['TX1Power']['value']
                transceiver_dom_info_dict['tx2power'] = dom_channel_monitor_data['data']['TX2Power']['value']
                transceiver_dom_info_dict['tx3power'] = dom_channel_monitor_data['data']['TX3Power']['value']
                transceiver_dom_info_dict['tx4power'] = dom_channel_monitor_data['data']['TX4Power']['value']
                transceiver_dom_info_dict['rx1power'] = dom_channel_monitor_data['data']['RX1Power']['value']
                transceiver_dom_info_dict['rx2power'] = dom_channel_monitor_data['data']['RX2Power']['value']
                transceiver_dom_info_dict['rx3power'] = dom_channel_monitor_data['data']['RX3Power']['value']
                transceiver_dom_info_dict['rx4power'] = dom_channel_monitor_data['data']['RX4Power']['value']
                transceiver_dom_info_dict['tx1bias'] = dom_channel_monitor_data['data']['TX1Bias']['value']
                transceiver_dom_info_dict['tx2bias'] = dom_channel_monitor_data['data']['TX2Bias']['value']
                transceiver_dom_info_dict['tx3bias'] = dom_channel_monitor_data['data']['TX3Bias']['value']
                transceiver_dom_info_dict['tx4bias'] = dom_channel_monitor_data['data']['TX4Bias']['value']

            else:
                dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params(
                    dom_channel_monitor_raw, 0)
                transceiver_dom_info_dict['rx1power'] = dom_channel_monitor_data['data']['RXPower']['value']
                transceiver_dom_info_dict['tx1bias'] = dom_channel_monitor_data['data']['TXBias']['value']
                transceiver_dom_info_dict['tx1power'] = dom_channel_monitor_data['data']['TXPower']['value']

        for key in transceiver_dom_info_dict:
            transceiver_dom_info_dict[key] = self._convert_string_to_num(
                transceiver_dom_info_dict[key])

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

        transceiver_dom_threshold_info_dict = dict.fromkeys(
            dom_info_dict_keys, 'N/A')

        path_idx = INDEX_A0H

        self._dom_capability_detect()
        if not self.dom_supported:
            return transceiver_dom_threshold_info_dict

        if self.sfp_type == QSFP_TYPE:
            field_offset = QSFP_UPPER_MEMORY_PAGE03_OFFSET  # uppper memory map: Page 03h
            Id_field = QSFP_ID_FIELDS
            sfpd_obj = sff8436Dom()
        elif self.sfp_type == SFP_TYPE:
            if self.eeprom_path_list[INDEX_A2H] is not 'n/a':
                # lower memory map: A2h (SFP i2c 0x51)
                field_offset = SFP_A2H_OFFSET
                path_idx = INDEX_A2H
            else:
                field_offset = 256
            Id_field = SFP_ID_FIELDS
            sfpd_obj = sff8472Dom()
        else:
            return transceiver_dom_threshold_info_dict

        if sfpd_obj is None:
            return transceiver_dom_threshold_info_dict

        dom_module_threshold_raw = self._read_eeprom_bytes(
            field_offset + Id_field['MODULE_THRESHOLD'][OFFSET], Id_field['MODULE_THRESHOLD'][WIDTH], path_idx)

        if dom_module_threshold_raw is None:
            return transceiver_dom_threshold_info_dict

        if self.sfp_type == QSFP_TYPE:
            dom_module_threshold_data = sfpd_obj.parse_module_threshold_values(
                dom_module_threshold_raw, 0)

            dom_channel_threshold_raw = self._read_eeprom_bytes(
                field_offset + Id_field['CHANNEL_THRESHOLD'][OFFSET], Id_field['CHANNEL_THRESHOLD'][WIDTH], path_idx)

            if dom_channel_threshold_raw is None:
                return transceiver_dom_threshold_info_dict

            dom_channel_threshold_data = sfpd_obj.parse_channel_threshold_values(
                dom_channel_threshold_raw, 0)

            transceiver_dom_threshold_info_dict['rxpowerhighalarm'] = dom_channel_threshold_data['data']['RxPowerHighAlarm']['value']
            transceiver_dom_threshold_info_dict['rxpowerhighwarning'] = dom_channel_threshold_data['data']['RxPowerHighWarning']['value']
            transceiver_dom_threshold_info_dict['rxpowerlowalarm'] = dom_channel_threshold_data['data']['RxPowerLowAlarm']['value']
            transceiver_dom_threshold_info_dict['rxpowerlowwarning'] = dom_channel_threshold_data['data']['RxPowerLowWarning']['value']
            transceiver_dom_threshold_info_dict['txpowerhighalarm'] = dom_channel_threshold_data['data']['TxPowerHighAlarm']['value']
            transceiver_dom_threshold_info_dict['txpowerhighwarning'] = dom_channel_threshold_data['data']['TxPowerHighWarning']['value']
            transceiver_dom_threshold_info_dict['txpowerlowalarm'] = dom_channel_threshold_data['data']['TxPowerLowAlarm']['value']
            transceiver_dom_threshold_info_dict['txpowerlowwarning'] = dom_channel_threshold_data['data']['TxPowerLowWarning']['value']
            transceiver_dom_threshold_info_dict['txbiashighalarm'] = dom_channel_threshold_data['data']['TxBiasHighAlarm']['value']
            transceiver_dom_threshold_info_dict['txbiashighwarning'] = dom_channel_threshold_data['data']['TxBiasHighWarning']['value']
            transceiver_dom_threshold_info_dict['txbiaslowalarm'] = dom_channel_threshold_data['data']['TxBiasLowAlarm']['value']
            transceiver_dom_threshold_info_dict['txbiaslowwarning'] = dom_channel_threshold_data['data']['TxBiasLowWarning']['value']
        else:  # SFP_TYPE
            dom_module_threshold_data = sfpd_obj.parse_alarm_warning_threshold(
                dom_module_threshold_raw, 0)
            transceiver_dom_threshold_info_dict['rxpowerhighalarm'] = dom_module_threshold_data['data']['RXPowerHighAlarm']['value']
            transceiver_dom_threshold_info_dict['rxpowerhighwarning'] = dom_module_threshold_data['data']['RXPowerHighWarning']['value']
            transceiver_dom_threshold_info_dict['rxpowerlowalarm'] = dom_module_threshold_data['data']['RXPowerLowAlarm']['value']
            transceiver_dom_threshold_info_dict['rxpowerlowwarning'] = dom_module_threshold_data['data']['RXPowerLowWarning']['value']
            transceiver_dom_threshold_info_dict['txpowerhighalarm'] = dom_module_threshold_data['data']['TXPowerHighAlarm']['value']
            transceiver_dom_threshold_info_dict['txpowerhighwarning'] = dom_module_threshold_data['data']['TXPowerHighWarning']['value']
            transceiver_dom_threshold_info_dict['txpowerlowalarm'] = dom_module_threshold_data['data']['TXPowerLowAlarm']['value']
            transceiver_dom_threshold_info_dict['txpowerlowwarning'] = dom_module_threshold_data['data']['TXPowerLowWarning']['value']
            transceiver_dom_threshold_info_dict['txbiashighalarm'] = dom_module_threshold_data['data']['BiasHighAlarm']['value']
            transceiver_dom_threshold_info_dict['txbiashighwarning'] = dom_module_threshold_data['data']['BiasHighWarning']['value']
            transceiver_dom_threshold_info_dict['txbiaslowalarm'] = dom_module_threshold_data['data']['BiasLowAlarm']['value']
            transceiver_dom_threshold_info_dict['txbiaslowwarning'] = dom_module_threshold_data['data']['BiasLowWarning']['value']

        transceiver_dom_threshold_info_dict['temphighalarm'] = dom_module_threshold_data['data']['TempHighAlarm']['value']
        transceiver_dom_threshold_info_dict['temphighwarning'] = dom_module_threshold_data['data']['TempHighWarning']['value']
        transceiver_dom_threshold_info_dict['templowalarm'] = dom_module_threshold_data['data']['TempLowAlarm']['value']
        transceiver_dom_threshold_info_dict['templowwarning'] = dom_module_threshold_data['data']['TempLowWarning']['value']
        transceiver_dom_threshold_info_dict['vcchighalarm'] = dom_module_threshold_data['data']['VccHighAlarm']['value']
        transceiver_dom_threshold_info_dict['vcchighwarning'] = dom_module_threshold_data['data']['VccHighWarning']['value']
        transceiver_dom_threshold_info_dict['vcclowalarm'] = dom_module_threshold_data['data']['VccLowAlarm']['value']
        transceiver_dom_threshold_info_dict['vcclowwarning'] = dom_module_threshold_data['data']['VccLowWarning']['value']

        return transceiver_dom_threshold_info_dict

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP

        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        raise NotImplementedError

    def get_rx_los(self):
        """
        Retrieves the RX LOS (lost-of-signal) status of SFP

        Returns:
            A Boolean, True if SFP has RX LOS, False if not.
            Note : RX LOS status is latched until a call to get_rx_los or a reset.
        """
        raise NotImplementedError

    def get_tx_fault(self):
        """
        Retrieves the TX fault status of SFP

        Returns:
            A Boolean, True if SFP has TX fault, False if not
            Note : TX fault status is lached until a call to get_tx_fault or a reset.
        """
        raise NotImplementedError

    def get_tx_disable(self):
        """
        Retrieves the tx_disable status of this SFP

        Returns:
            A Boolean, True if tx_disable is enabled, False if disabled
        """
        raise NotImplementedError

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
        if self.lp_file is not None:
            data = self.__read_attr_file(self.lp_file, 0)
            if data is not None:
                if int(data) == 1:
                    return True
        return False

    def get_power_override(self):
        """
        Retrieves the power-override status of this SFP

        Returns:
            A Boolean, True if power-override is enabled, False if disabled
        """

        if self.sfp_type == QSFP_TYPE:
            Id_field = QSFP_ID_FIELDS
            sfpd_obj = sff8436Dom()
            if sfpd_obj is None:
                return False

            dom_control_raw = self._read_eeprom_bytes(
                Id_field['CONTROL'][OFFSET], Id_field['CONTROL'][WIDTH], INDEX_A0H)
            if dom_control_raw is not None:
                dom_control_data = sfpd_obj.parse_control_bytes(
                    dom_control_raw, 0)
                return ('On' == dom_control_data['data']['PowerOverride'])

            return False
        else:
            return NotImplementedError

    def get_temperature(self):
        """
        Retrieves the temperature of this SFP

        Returns:
            An integer number of current temperature in Celsius
        """
        path_idx = INDEX_A0H

        if self.sfp_type == QSFP_TYPE:
            field_offset = 0  # lower memory map: A0h
            Id_field = QSFP_ID_FIELDS
            sfpd_obj = sff8436Dom()
        elif self.sfp_type == SFP_TYPE:
            # lower memory map: A2h (SFP i2c 0x51)
            field_offset = SFP_A2H_OFFSET
            Id_field = SFP_ID_FIELDS
            path_idx = INDEX_A2H
            sfpd_obj = sff8472Dom()
        else:
            return None

        if sfpd_obj is None:
            return None

        dom_temperature_raw = self._read_eeprom_bytes(
            field_offset + Id_field['TEMPERATURE'][OFFSET], Id_field['TEMPERATURE'][WIDTH], path_idx)
        if dom_temperature_raw is not None:
            dom_temperature_data = sfpd_obj.parse_temperature(
                dom_temperature_raw, 0)
            temp = self._convert_string_to_num(
                dom_temperature_data['data']['Temperature']['value'])
            return temp

        return None

    def get_voltage(self):
        """
        Retrieves the supply voltage of this SFP

        Returns:
            An integer number of supply voltage in mV
        """
        path_idx = INDEX_A0H
        if self.sfp_type == QSFP_TYPE:
            field_offset = 0  # lower memory map: A0h
            Id_field = QSFP_ID_FIELDS
            sfpd_obj = sff8436Dom()
        elif self.sfp_type == SFP_TYPE:
            # lower memory map: A2h (SFP i2c 0x51)
            field_offset = SFP_A2H_OFFSET
            Id_field = SFP_ID_FIELDS
            path_idx = INDEX_A2H
            sfpd_obj = sff8472Dom()
        else:
            return None

        if sfpd_obj is None:
            return None

        dom_voltage_raw = self._read_eeprom_bytes(
            field_offset + Id_field['VOLTAGE'][OFFSET], Id_field['VOLTAGE'][WIDTH], path_idx)
        if dom_voltage_raw is not None:
            dom_voltage_data = sfpd_obj.parse_voltage(dom_voltage_raw, 0)
            volt = self._convert_string_to_num(
                dom_voltage_data['data']['Vcc']['value'])
            return volt

        return None

    def get_tx_bias(self):
        """
        Retrieves the TX bias current of this SFP

        Returns:
            A list of four integer numbers, representing TX bias in mA
            for channel 0 to channel 4.
            Ex. ['110.09', '111.12', '108.21', '112.09']
        """
        path_idx = INDEX_A0H
        tx_bias_list = []

        if self.sfp_type == QSFP_TYPE:
            field_offset = 0  # lower memory map: A0h
            Id_field = QSFP_ID_FIELDS
            sfpd_obj = sff8436Dom()
        elif self.sfp_type == SFP_TYPE:
            # lower memory map: A2h (SFP i2c 0x51)
            field_offset = SFP_A2H_OFFSET
            Id_field = SFP_ID_FIELDS
            path_idx = INDEX_A2H
            sfpd_obj = sff8472Dom()
        else:
            return None

        if sfpd_obj is None:
            return None

        dom_channel_monitor_raw = self._read_eeprom_bytes(
            field_offset + Id_field['CHANNEL_MON'][OFFSET], Id_field['CHANNEL_MON'][WIDTH], path_idx)
        if dom_channel_monitor_raw is not None:
            if self.sfp_type == QSFP_TYPE:
                dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params_with_tx_power(
                    dom_channel_monitor_raw, 0)
                tx_bias_list.append(self._convert_string_to_num(
                    dom_channel_monitor_data['data']['TX1Bias']['value']))
                tx_bias_list.append(self._convert_string_to_num(
                    dom_channel_monitor_data['data']['TX2Bias']['value']))
                tx_bias_list.append(self._convert_string_to_num(
                    dom_channel_monitor_data['data']['TX3Bias']['value']))
                tx_bias_list.append(self._convert_string_to_num(
                    dom_channel_monitor_data['data']['TX4Bias']['value']))

            else:
                dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params(
                    dom_channel_monitor_raw, 0)
                tx_bias_list.append(self._convert_string_to_num(
                    dom_channel_monitor_data['data']['TXBias']['value']))

            return tx_bias_list

        return None

    def get_rx_power(self):
        """
        Retrieves the received optical power for this SFP

        Returns:
            A list of four integer numbers, representing received optical
            power in mW for channel 0 to channel 4.
            Ex. ['1.77', '1.71', '1.68', '1.70']
        """
        path_idx = INDEX_A0H
        rx_power_list = []

        if self.sfp_type == QSFP_TYPE:
            field_offset = 0  # lower memory map: A0h
            Id_field = QSFP_ID_FIELDS
            sfpd_obj = sff8436Dom()
        elif self.sfp_type == SFP_TYPE:
            # lower memory map: A2h (SFP i2c 0x51)
            field_offset = SFP_A2H_OFFSET
            Id_field = SFP_ID_FIELDS
            path_idx = INDEX_A2H
            sfpd_obj = sff8472Dom()
        else:
            return None

        if sfpd_obj is None:
            return None

        dom_channel_monitor_raw = self._read_eeprom_bytes(
            field_offset + Id_field['CHANNEL_MON'][OFFSET], Id_field['CHANNEL_MON'][WIDTH], path_idx)
        if dom_channel_monitor_raw is not None:
            if self.sfp_type == QSFP_TYPE:
                dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params_with_tx_power(
                    dom_channel_monitor_raw, 0)
                rx_power_list.append(self._convert_string_to_num(
                    dom_channel_monitor_data['data']['RX1Power']['value']))
                rx_power_list.append(self._convert_string_to_num(
                    dom_channel_monitor_data['data']['RX2Power']['value']))
                rx_power_list.append(self._convert_string_to_num(
                    dom_channel_monitor_data['data']['RX3Power']['value']))
                rx_power_list.append(self._convert_string_to_num(
                    dom_channel_monitor_data['data']['RX4Power']['value']))

            else:
                dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params(
                    dom_channel_monitor_raw, 0)
                rx_power_list.append(self._convert_string_to_num(
                    dom_channel_monitor_data['data']['RXPower']['value']))

            return rx_power_list

        return None

    def get_tx_power(self):
        """
        Retrieves the TX power of this SFP

        Returns:
            A list of four integer numbers, representing TX power in mW
            for channel 0 to channel 4.
            Ex. ['1.86', '1.86', '1.86', '1.86']
        """
        path_idx = INDEX_A0H
        tx_power_list = []

        if self.sfp_type == QSFP_TYPE:
            field_offset = 0  # lower memory map: A0h
            Id_field = QSFP_ID_FIELDS
            sfpd_obj = sff8436Dom()
        elif self.sfp_type == SFP_TYPE:
            # lower memory map: A2h (SFP i2c 0x51)
            field_offset = SFP_A2H_OFFSET
            Id_field = SFP_ID_FIELDS
            path_idx = INDEX_A2H
            sfpd_obj = sff8472Dom()
        else:
            return None

        if sfpd_obj is None:
            return None

        dom_channel_monitor_raw = self._read_eeprom_bytes(
            field_offset + Id_field['CHANNEL_MON'][OFFSET], Id_field['CHANNEL_MON'][WIDTH], path_idx)
        if dom_channel_monitor_raw is not None:
            if self.sfp_type == QSFP_TYPE:
                dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params_with_tx_power(
                    dom_channel_monitor_raw, 0)
                tx_power_list.append(self._convert_string_to_num(
                    dom_channel_monitor_data['data']['TX1Power']['value']))
                tx_power_list.append(self._convert_string_to_num(
                    dom_channel_monitor_data['data']['TX2Power']['value']))
                tx_power_list.append(self._convert_string_to_num(
                    dom_channel_monitor_data['data']['TX3Power']['value']))
                tx_power_list.append(self._convert_string_to_num(
                    dom_channel_monitor_data['data']['TX4Power']['value']))

            else:
                dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params(
                    dom_channel_monitor_raw, 0)
                tx_power_list.append(self._convert_string_to_num(
                    dom_channel_monitor_data['data']['TXPower']['value']))

            return tx_power_list

        return None

    def reset(self):
        """
        Reset SFP and return all user module settings to their default srate.

        Returns:
            A boolean, True if successful, False if not
        """
        if self.reset_file is not None:
            ret = self.__write_attr_file(self.reset_file, '1')
            if ret != 0:
                return True
        return False

    def tx_disable(self, tx_disable):
        """
        Disable SFP TX for all channels

        Args:
            tx_disable : A Boolean, True to enable tx_disable mode, False to disable
                         tx_disable mode.

        Returns:
            A boolean, True if tx_disable is set successfully, False if not
        """
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
        if self.lp_file is not None:
            if lpmode is True:
                ret = self.__write_attr_file(self.lp_file, "1")
            else:
                ret = self.__write_attr_file(self.lp_file, "0")
            if ret != 0:
                return True
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
        return False
