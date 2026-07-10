# -*- coding: utf-8 -*

#############################################################################
# Ruijie B6510-48VS8CQ
#
# Module contains an implementation of SONiC Platform Base API and
# provides the platform information
#
#############################################################################

try:
    import os
    import time
    from ctypes import create_string_buffer
    from sonic_platform_base.sfp_base import SfpBase
    from sonic_platform_base.sonic_sfp.sff8472 import sff8472InterfaceId
    from sonic_platform_base.sonic_sfp.sff8472 import sff8472Dom
    from sonic_platform_base.sonic_sfp.sff8436 import sff8436InterfaceId
    from sonic_platform_base.sonic_sfp.sff8436 import sff8436Dom

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

#############################################################################
# Ruijie B6510-48VS8CQ
PORT_START = 0
PORT_END = 55
PORTS_IN_BLOCK = 56

PORT_QSFP_START = 48
PORT_QSFP_END = 56

EEPROM_OFFSET = 11
#############################################################################

IDENTITY_EEPROM_ADDR = 0x50

INFO_OFFSET = 128
DOM_OFFSET = 0
DOM_OFFSET1 = 384

SFP_TYPE = "SFP"
QSFP_TYPE = "QSFP"
OSFP_TYPE = "OSFP"

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

# definitions of the offset for values in OSFP info eeprom
OSFP_TYPE_OFFSET = 0
OSFP_VENDOR_NAME_OFFSET = 129
OSFP_VENDOR_PN_OFFSET = 148
OSFP_HW_REV_OFFSET = 164
OSFP_VENDOR_SN_OFFSET = 166

# Offset for values in QSFP eeprom
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
QSFP_POWERMODE_OFFSET = 93
QSFP_POWEROVERRIDE_WIDTH = 1
QSFP_POWERSET_BIT = 1
QSFP_OPTION_VALUE_OFFSET = 192
QSFP_OPTION_VALUE_WIDTH = 4
QSFP_MODULE_UPPER_PAGE3_START = 384
QSFP_MODULE_THRESHOLD_WIDTH = 24
QSFP_MODULE_THRESHOLD_OFFSET = 128
QSFP_CHANNL_THRESHOLD_OFFSET = 176
QSFP_CHANNL_THRESHOLD_WIDTH = 24

SFP_MODULE_ADDRA2_OFFSET = 256
SFP_MODULE_THRESHOLD_OFFSET = 0
SFP_MODULE_THRESHOLD_WIDTH = 56
SFP_CHANNL_THRESHOLD_OFFSET = 112
SFP_CHANNL_THRESHOLD_WIDTH = 2

SFP_TEMPE_OFFSET = 96
SFP_TEMPE_WIDTH = 2
SFP_VOLT_OFFSET = 98
SFP_VOLT_WIDTH = 2
SFP_CHANNL_MON_OFFSET = 100
SFP_CHANNL_MON_WIDTH = 6
SFP_CHANNL_STATUS_OFFSET = 110
SFP_CHANNL_STATUS_WIDTH = 1

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
    """
    DELLEMC Platform-specific Sfp class
    """

    SFP_DEVICE_TYPE = "optoe2"
    QSFP_DEVICE_TYPE = "optoe1"
    port_to_i2cbus_mapping = {}
    def __init__(self, index):
        self.index = index
        self.port_num = self.index + PORT_START
        self.sfp_type = self.__get_sfp_info()
        self.dom_supported = False
        for x in range(PORT_START, PORTS_IN_BLOCK):
            self.port_to_i2cbus_mapping[x] = (x + EEPROM_OFFSET)

        self.info_dict_keys = ['type', 'vendor_rev', 'serial', 'manufacturer', 'model', 'connector', 'encoding', 'ext_identifier',
                               'ext_rateselect_compliance', 'cable_type', 'cable_length', 'nominal_bit_rate', 'specification_compliance', 'vendor_date', 'vendor_oui']

        self.dom_dict_keys = ['rx_los', 'tx_fault', 'reset_status', 'lp_mode', 'tx_disable', 'tx_disabled_channel', 'temperature', 'voltage',
                              'rx1power', 'rx2power', 'rx3power', 'rx4power', 'tx1bias', 'tx2bias', 'tx3bias', 'tx4bias', 'tx1power', 'tx2power', 'tx3power', 'tx4power']

        self.threshold_dict_keys = ['temphighalarm', 'temphighwarning', 'templowalarm', 'templowwarning', 'vcchighalarm', 'vcchighwarning', 'vcclowalarm', 'vcclowwarning', 'rxpowerhighalarm', 'rxpowerhighwarning',
                                    'rxpowerlowalarm', 'rxpowerlowwarning', 'txpowerhighalarm', 'txpowerhighwarning', 'txpowerlowalarm', 'txpowerlowwarning', 'txbiashighalarm', 'txbiashighwarning', 'txbiaslowalarm', 'txbiaslowwarning']

        self.__dom_capability_detect()

    @property
    def qsfp_ports(self):
        return range(PORT_QSFP_START, PORT_QSFP_END)

    def get_presence(self):
        # Check for invalid self.port_num
        if self.port_num < PORT_START or self.port_num > PORT_END:
            return False

        if self.port_num <= 7:
            presence_path = "/sys/bus/i2c/devices/1-0034/sfp_presence1"
        elif self.port_num >= 8 and self.port_num <= 15:
            presence_path = "/sys/bus/i2c/devices/1-0034/sfp_presence2"
        elif self.port_num >= 16 and self.port_num <= 23:
            presence_path = "/sys/bus/i2c/devices/1-0034/sfp_presence3"
        elif self.port_num >= 24 and self.port_num <= 31:
            presence_path = "/sys/bus/i2c/devices/1-0036/sfp_presence4"
        elif self.port_num >= 32 and self.port_num <= 39:
            presence_path = "/sys/bus/i2c/devices/1-0036/sfp_presence5"
        elif self.port_num >= 40 and self.port_num <= 47:
            presence_path = "/sys/bus/i2c/devices/1-0036/sfp_presence6"
        elif self.port_num >= 48 and self.port_num <= 55:
            presence_path = "/sys/bus/i2c/devices/1-0036/sfp_presence7"
        else:
            return False

        try:
            data = open(presence_path, "rb")
        except IOError:
            return False

        presence_data = data.read(2)
        if presence_data == "":
            return False
        result = int(presence_data, 16)
        data.close()

        # ModPrsL is active low
        if result & (1 << (self.port_num % 8)) == 0:
            return True

        return False

    def __get_sfp_info(self):
        if PORT_START <= self.port_num <= PORT_END:
            if self.port_num in self.qsfp_ports:
                sfp_type = QSFP_TYPE
            else:
                sfp_type = SFP_TYPE
        else:
            return "port range error"
        return sfp_type

    def __dom_capability_detect(self):
        if not self.get_presence():
            self.dom_supported = False
            self.dom_temp_supported = False
            self.dom_volt_supported = False
            self.dom_rx_power_supported = False
            self.dom_tx_power_supported = False
            self.qsfp_page3_available = False
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
                qsfp_version_compliance_raw = self.__read_eeprom_specific_bytes(
                    QSFP_VERSION_COMPLIANCE_OFFSET, QSFP_VERSION_COMPLIANCE_WIDTH)
                qsfp_version_compliance = int(
                    qsfp_version_compliance_raw[0], 16)
                dom_capability = sfpi_obj.parse_qsfp_dom_capability(
                    qsfp_dom_capability_raw, 0)
                if qsfp_version_compliance >= 0x08:
                    self.dom_temp_supported = dom_capability['data']['Temp_support']['value'] == 'On'
                    self.dom_volt_supported = dom_capability['data']['Voltage_support']['value'] == 'On'
                    self.dom_rx_power_supported = dom_capability['data']['Rx_power_support']['value'] == 'On'
                    self.dom_tx_power_supported = dom_capability['data']['Tx_power_support']['value'] == 'On'
                else:
                    self.dom_temp_supported = True
                    self.dom_volt_supported = True
                    self.dom_rx_power_supported = dom_capability['data']['Rx_power_support']['value'] == 'On'
                    self.dom_tx_power_supported = True

                self.dom_supported = True
                self.calibration = 1
                sfpd_obj = sff8436Dom()
                if sfpd_obj is None:
                    return None
                qsfp_option_value_raw = self.__read_eeprom_specific_bytes(
                    QSFP_OPTION_VALUE_OFFSET, QSFP_OPTION_VALUE_WIDTH)
                if qsfp_option_value_raw is not None:
                    optional_capability = sfpd_obj.parse_option_params(
                        qsfp_option_value_raw, 0)
                    self.dom_tx_disable_supported = optional_capability[
                        'data']['TxDisable']['value'] == 'On'
                dom_status_indicator = sfpd_obj.parse_dom_status_indicator(
                    qsfp_version_compliance_raw, 1)
                self.qsfp_page3_available = dom_status_indicator['data']['FlatMem']['value'] == 'Off'
            else:
                self.dom_supported = False
                self.dom_temp_supported = False
                self.dom_volt_supported = False
                self.dom_rx_power_supported = False
                self.dom_tx_power_supported = False
                self.calibration = 0
                self.qsfp_page3_available = False

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
                self.dom_tx_disable_supported = (
                    int(sfp_dom_capability_raw[1], 16) & 0x40 != 0)
        else:
            self.dom_supported = False
            self.dom_temp_supported = False
            self.dom_volt_supported = False
            self.dom_rx_power_supported = False
            self.dom_tx_power_supported = False
    
    def __add_new_sfp_device(self, sysfs_sfp_i2c_adapter_path, devaddr, devtype):
        try:
            sysfs_nd_path = "%s/new_device" % sysfs_sfp_i2c_adapter_path

            # Write device address to new_device file
            nd_file = open(sysfs_nd_path, "w")
            nd_str = "%s %s" % (devtype, hex(devaddr))
            nd_file.write(nd_str)
            nd_file.close()

        except Exception as err:
            print("Error writing to new device file: %s" % str(err))
            return 1
        else:
            return 0
    
    def __get_port_eeprom_path(self, port_num, devid):
        if self.get_presence() is False:
            print("port %d Not present" % port_num)
            return None

        eeprom_path = '/sys/bus/i2c/devices/i2c-{0}/{0}-0050'
        i2c_path = '/sys/bus/i2c/devices/i2c-{0}'
        sysfs_sfp_i2c_client_path = eeprom_path.format(self.port_to_i2cbus_mapping[port_num])
        sysfs_sfp_i2c_adapter_path= i2c_path.format(self.port_to_i2cbus_mapping[port_num])
        # If sfp device is not present on bus, Add it
        if not os.path.exists(sysfs_sfp_i2c_client_path):
            if port_num in self.qsfp_ports:
                ret = self.__add_new_sfp_device(
                        sysfs_sfp_i2c_adapter_path, devid, self.QSFP_DEVICE_TYPE)
            else:
                ret = self.__add_new_sfp_device(
                        sysfs_sfp_i2c_adapter_path, devid, self.SFP_DEVICE_TYPE)
            if ret != 0:
                print("Error adding sfp device")
                return None

        sysfs_sfp_i2c_client_eeprom_path = "%s/eeprom" % sysfs_sfp_i2c_client_path

        return sysfs_sfp_i2c_client_eeprom_path

    def __read_eeprom_specific_bytes(self, offset, num_bytes):
        sysfsfile_eeprom = None
        eeprom_raw = []
        for i in range(0, num_bytes):
            eeprom_raw.append("0x00")

        sysfs_sfp_i2c_client_eeprom_path = self.__get_port_eeprom_path(self.port_num, IDENTITY_EEPROM_ADDR)
        if sysfs_sfp_i2c_client_eeprom_path is None:
            return eeprom_raw
        try:
            sysfsfile_eeprom = open(sysfs_sfp_i2c_client_eeprom_path, mode="rb", buffering=0)
            sysfsfile_eeprom.seek(offset)
            raw = sysfsfile_eeprom.read(num_bytes)
            for n in range(0, num_bytes):
                eeprom_raw[n] = hex(raw[n])[2:].zfill(2)
        except IOError:
            pass
        finally:
            if sysfsfile_eeprom:
                sysfsfile_eeprom.close()

        return eeprom_raw

    def __convert_string_to_num(self, value_str):
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
        if self.port_num < PORT_START or self.port_num > PORT_END:
            return None
        compliance_code_dict = {}
        transceiver_info_dict = dict.fromkeys(self.info_dict_keys, 'N/A')
        '''
        if not self.get_presence():
            return transceiver_info_dict
        '''
        # ToDo: OSFP tranceiver info parsing not fully supported.
        # in inf8628.py lack of some memory map definition
        # will be implemented when the inf8628 memory map ready
        #port_eeprom_path = self.sfputilbase.__get_port_eeprom_path(self.port_num, IDENTITY_EEPROM_ADDR)
        if self.sfp_type == QSFP_TYPE:
            offset = 128
            vendor_rev_width = XCVR_HW_REV_WIDTH_QSFP
            interface_info_bulk_width = XCVR_INTFACE_BULK_WIDTH_QSFP

            sfpi_obj = sff8436InterfaceId()
            if sfpi_obj is None:
                print("Error: sfp_object open failed")
                return None

        else:
            offset = 0
            vendor_rev_width = XCVR_HW_REV_WIDTH_SFP
            interface_info_bulk_width = XCVR_INTFACE_BULK_WIDTH_SFP

            sfpi_obj = sff8472InterfaceId()
            if sfpi_obj is None:
                print("Error: sfp_object open failed")
                return None

        sfp_interface_bulk_raw = self.__read_eeprom_specific_bytes(
            offset + XCVR_INTERFACE_DATA_START, XCVR_INTERFACE_DATA_SIZE)
        if sfp_interface_bulk_raw is None:
            return None

        start = XCVR_INTFACE_BULK_OFFSET - XCVR_INTERFACE_DATA_START
        end = start + interface_info_bulk_width
        sfp_interface_bulk_data = sfpi_obj.parse_sfp_info_bulk(
            sfp_interface_bulk_raw[start: end], 0)

        start = XCVR_VENDOR_NAME_OFFSET - XCVR_INTERFACE_DATA_START
        end = start + XCVR_VENDOR_NAME_WIDTH
        sfp_vendor_name_data = sfpi_obj.parse_vendor_name(
            sfp_interface_bulk_raw[start: end], 0)

        start = XCVR_VENDOR_PN_OFFSET - XCVR_INTERFACE_DATA_START
        end = start + XCVR_VENDOR_PN_WIDTH
        sfp_vendor_pn_data = sfpi_obj.parse_vendor_pn(
            sfp_interface_bulk_raw[start: end], 0)

        start = XCVR_HW_REV_OFFSET - XCVR_INTERFACE_DATA_START
        end = start + vendor_rev_width
        sfp_vendor_rev_data = sfpi_obj.parse_vendor_rev(
            sfp_interface_bulk_raw[start: end], 0)

        start = XCVR_VENDOR_SN_OFFSET - XCVR_INTERFACE_DATA_START
        end = start + XCVR_VENDOR_SN_WIDTH
        sfp_vendor_sn_data = sfpi_obj.parse_vendor_sn(
            sfp_interface_bulk_raw[start: end], 0)

        start = XCVR_VENDOR_OUI_OFFSET - XCVR_INTERFACE_DATA_START
        end = start + XCVR_VENDOR_OUI_WIDTH
        sfp_vendor_oui_data = sfpi_obj.parse_vendor_oui(
            sfp_interface_bulk_raw[start: end], 0)

        start = XCVR_VENDOR_DATE_OFFSET - XCVR_INTERFACE_DATA_START
        end = start + XCVR_VENDOR_DATE_WIDTH
        sfp_vendor_date_data = sfpi_obj.parse_vendor_date(
            sfp_interface_bulk_raw[start: end], 0)
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
        self.__dom_capability_detect()
        if self.port_num < PORT_START or self.port_num > PORT_END:
            return None
        transceiver_dom_info_dict = dict.fromkeys(self.dom_dict_keys, 'N/A')

        if self.sfp_type == QSFP_TYPE:
            offset = 128
            qsfp_dom_capability_raw = self.__read_eeprom_specific_bytes(
            (offset + XCVR_DOM_CAPABILITY_OFFSET), XCVR_DOM_CAPABILITY_WIDTH)
            if qsfp_dom_capability_raw is not None:
                self.dom_supported = True
            if not self.dom_supported:
                return transceiver_dom_info_dict

            offset = 0
            sfpd_obj = sff8436Dom()
            if sfpd_obj is None:
                return transceiver_dom_info_dict

            dom_data_raw = self.__read_eeprom_specific_bytes(
                (offset + QSFP_DOM_BULK_DATA_START), QSFP_DOM_BULK_DATA_SIZE)
            if dom_data_raw is None:
                return transceiver_dom_info_dict

            if self.dom_temp_supported:
                start = QSFP_TEMPE_OFFSET - QSFP_DOM_BULK_DATA_START
                end = start + QSFP_TEMPE_WIDTH
                dom_temperature_data = sfpd_obj.parse_temperature(
                    dom_data_raw[start: end], 0)
                temp = self.__convert_string_to_num(
                    dom_temperature_data['data']['Temperature']['value'])
                if temp is not None:
                    transceiver_dom_info_dict['temperature'] = temp

            if self.dom_volt_supported:
                start = QSFP_VOLT_OFFSET - QSFP_DOM_BULK_DATA_START
                end = start + QSFP_VOLT_WIDTH
                dom_voltage_data = sfpd_obj.parse_voltage(
                    dom_data_raw[start: end], 0)
                volt = self.__convert_string_to_num(
                    dom_voltage_data['data']['Vcc']['value'])
                if volt is not None:
                    transceiver_dom_info_dict['voltage'] = volt

            start = QSFP_CHANNL_MON_OFFSET - QSFP_DOM_BULK_DATA_START
            end = start + QSFP_CHANNL_MON_WITH_TX_POWER_WIDTH
            dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params_with_tx_power(
                dom_data_raw[start: end], 0)

            if self.dom_tx_power_supported:
                transceiver_dom_info_dict['tx1power'] = self.__convert_string_to_num(
                    dom_channel_monitor_data['data']['TX1Power']['value'])
                transceiver_dom_info_dict['tx2power'] = self.__convert_string_to_num(
                    dom_channel_monitor_data['data']['TX2Power']['value'])
                transceiver_dom_info_dict['tx3power'] = self.__convert_string_to_num(
                    dom_channel_monitor_data['data']['TX3Power']['value'])
                transceiver_dom_info_dict['tx4power'] = self.__convert_string_to_num(
                    dom_channel_monitor_data['data']['TX4Power']['value'])

            if self.dom_rx_power_supported:
                transceiver_dom_info_dict['rx1power'] = self.__convert_string_to_num(
                    dom_channel_monitor_data['data']['RX1Power']['value'])
                transceiver_dom_info_dict['rx2power'] = self.__convert_string_to_num(
                    dom_channel_monitor_data['data']['RX2Power']['value'])
                transceiver_dom_info_dict['rx3power'] = self.__convert_string_to_num(
                    dom_channel_monitor_data['data']['RX3Power']['value'])
                transceiver_dom_info_dict['rx4power'] = self.__convert_string_to_num(
                    dom_channel_monitor_data['data']['RX4Power']['value'])

            transceiver_dom_info_dict['tx1bias'] = dom_channel_monitor_data['data']['TX1Bias']['value']
            transceiver_dom_info_dict['tx2bias'] = dom_channel_monitor_data['data']['TX2Bias']['value']
            transceiver_dom_info_dict['tx3bias'] = dom_channel_monitor_data['data']['TX3Bias']['value']
            transceiver_dom_info_dict['tx4bias'] = dom_channel_monitor_data['data']['TX4Bias']['value']

        else:
            if not self.dom_supported:
                return transceiver_dom_info_dict

            offset = 256
            sfpd_obj = sff8472Dom()
            if sfpd_obj is None:
                return transceiver_dom_info_dict
            sfpd_obj._calibration_type = self.calibration

            if sfpd_obj._calibration_type == 1:
                dom_data_raw = self.__read_eeprom_specific_bytes(
                    (offset + SFP_DOM_BULK_DATA_START), SFP_DOM_BULK_DATA_SIZE)
                start = SFP_TEMPE_OFFSET - SFP_DOM_BULK_DATA_START
                end = start + SFP_TEMPE_WIDTH
                dom_temperature_data = sfpd_obj.parse_temperature(
                    dom_data_raw[start: end], 0)

                start = SFP_VOLT_OFFSET - SFP_DOM_BULK_DATA_START
                end = start + SFP_VOLT_WIDTH
                dom_voltage_data = sfpd_obj.parse_voltage(
                    dom_data_raw[start: end], 0)

                start = SFP_CHANNL_MON_OFFSET - SFP_DOM_BULK_DATA_START
                end = start + SFP_CHANNL_MON_WIDTH
                dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params(
                    dom_data_raw[start: end], 0)
            else:
                dom_data_raw = self.__read_eeprom_specific_bytes(
                    (offset), offset + 256)
                dom_temperature_data = sfpd_obj.parse_temperature(dom_data_raw, 0)

                dom_voltage_data = sfpd_obj.parse_voltage(dom_data_raw, 0)

                dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params(dom_data_raw, 0)

            transceiver_dom_info_dict['temperature'] = self.__convert_string_to_num(
                dom_temperature_data['data']['Temperature']['value'])
            transceiver_dom_info_dict['voltage'] = self.__convert_string_to_num(
                dom_voltage_data['data']['Vcc']['value'])
            transceiver_dom_info_dict['rx1power'] = self.__convert_string_to_num(
                dom_channel_monitor_data['data']['RXPower']['value'])
            transceiver_dom_info_dict['tx1bias'] = self.__convert_string_to_num(
                dom_channel_monitor_data['data']['TXBias']['value'])
            transceiver_dom_info_dict['tx1power'] = self.__convert_string_to_num(
                dom_channel_monitor_data['data']['TXPower']['value'])

        transceiver_dom_info_dict['rx_los'] = self.get_rx_los()
        transceiver_dom_info_dict['tx_fault'] = self.get_tx_fault()
        transceiver_dom_info_dict['reset_status'] = self.get_reset_status()
        transceiver_dom_info_dict['tx_disable'] = self.get_tx_disable()
        transceiver_dom_info_dict['tx_disabled_channel'] = self.get_tx_disable_channel()
        transceiver_dom_info_dict['lp_mode'] = self.get_lpmode()

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
        self.__dom_capability_detect()
        if self.port_num < PORT_START or self.port_num > PORT_END:
            return None
        transceiver_dom_threshold_info_dict = dict.fromkeys(
            self.threshold_dict_keys, 'N/A')

        if self.sfp_type == OSFP_TYPE:
            pass

        elif self.sfp_type == QSFP_TYPE:
            if not self.dom_supported or not self.qsfp_page3_available:
                return transceiver_dom_threshold_info_dict

            # Dom Threshold data starts from offset 384
            # Revert offset back to 0 once data is retrieved
            offset = QSFP_MODULE_UPPER_PAGE3_START
            sfpd_obj = sff8436Dom()
            if sfpd_obj is None:
                return transceiver_dom_threshold_info_dict

            dom_module_threshold_raw = self.__read_eeprom_specific_bytes(
                (offset + QSFP_MODULE_THRESHOLD_OFFSET), QSFP_MODULE_THRESHOLD_WIDTH)
            if dom_module_threshold_raw is None:
                return transceiver_dom_threshold_info_dict

            dom_module_threshold_data = sfpd_obj.parse_module_threshold_values(
                dom_module_threshold_raw, 0)

            dom_channel_threshold_raw = self.__read_eeprom_specific_bytes((offset + QSFP_CHANNL_THRESHOLD_OFFSET),
                                                                          QSFP_CHANNL_THRESHOLD_WIDTH)
            if dom_channel_threshold_raw is None:
                return transceiver_dom_threshold_info_dict
            dom_channel_threshold_data = sfpd_obj.parse_channel_threshold_values(
                dom_channel_threshold_raw, 0)

            # Threshold Data
            transceiver_dom_threshold_info_dict['temphighalarm'] = dom_module_threshold_data['data']['TempHighAlarm']['value']
            transceiver_dom_threshold_info_dict['temphighwarning'] = dom_module_threshold_data['data']['TempHighWarning']['value']
            transceiver_dom_threshold_info_dict['templowalarm'] = dom_module_threshold_data['data']['TempLowAlarm']['value']
            transceiver_dom_threshold_info_dict['templowwarning'] = dom_module_threshold_data['data']['TempLowWarning']['value']
            transceiver_dom_threshold_info_dict['vcchighalarm'] = dom_module_threshold_data['data']['VccHighAlarm']['value']
            transceiver_dom_threshold_info_dict['vcchighwarning'] = dom_module_threshold_data['data']['VccHighWarning']['value']
            transceiver_dom_threshold_info_dict['vcclowalarm'] = dom_module_threshold_data['data']['VccLowAlarm']['value']
            transceiver_dom_threshold_info_dict['vcclowwarning'] = dom_module_threshold_data['data']['VccLowWarning']['value']
            transceiver_dom_threshold_info_dict['rxpowerhighalarm'] = dom_channel_threshold_data['data']['RxPowerHighAlarm']['value']
            transceiver_dom_threshold_info_dict['rxpowerhighwarning'] = dom_channel_threshold_data['data']['RxPowerHighWarning']['value']
            transceiver_dom_threshold_info_dict['rxpowerlowalarm'] = dom_channel_threshold_data['data']['RxPowerLowAlarm']['value']
            transceiver_dom_threshold_info_dict['rxpowerlowwarning'] = dom_channel_threshold_data['data']['RxPowerLowWarning']['value']
            transceiver_dom_threshold_info_dict['txbiashighalarm'] = dom_channel_threshold_data['data']['TxBiasHighAlarm']['value']
            transceiver_dom_threshold_info_dict['txbiashighwarning'] = dom_channel_threshold_data['data']['TxBiasHighWarning']['value']
            transceiver_dom_threshold_info_dict['txbiaslowalarm'] = dom_channel_threshold_data['data']['TxBiasLowAlarm']['value']
            transceiver_dom_threshold_info_dict['txbiaslowwarning'] = dom_channel_threshold_data['data']['TxBiasLowWarning']['value']
            transceiver_dom_threshold_info_dict['txpowerhighalarm'] = dom_channel_threshold_data['data']['TxPowerHighAlarm']['value']
            transceiver_dom_threshold_info_dict['txpowerhighwarning'] = dom_channel_threshold_data['data']['TxPowerHighWarning']['value']
            transceiver_dom_threshold_info_dict['txpowerlowalarm'] = dom_channel_threshold_data['data']['TxPowerLowAlarm']['value']
            transceiver_dom_threshold_info_dict['txpowerlowwarning'] = dom_channel_threshold_data['data']['TxPowerLowWarning']['value']

        else:
            offset = SFP_MODULE_ADDRA2_OFFSET

            if not self.dom_supported:
                return transceiver_dom_threshold_info_dict

            sfpd_obj = sff8472Dom(None, self.calibration)
            if sfpd_obj is None:
                return transceiver_dom_threshold_info_dict

            sfpd_obj._calibration_type = self.calibration

            if sfpd_obj._calibration_type == 1:
                dom_module_threshold_raw = self.__read_eeprom_specific_bytes((offset + SFP_MODULE_THRESHOLD_OFFSET),
                                                                            SFP_MODULE_THRESHOLD_WIDTH)
            else:
                dom_module_threshold_raw = self.__read_eeprom_specific_bytes(
                    (offset), offset + 256)
            if dom_module_threshold_raw is not None:
                dom_module_threshold_data = sfpd_obj.parse_alarm_warning_threshold(
                    dom_module_threshold_raw, 0)
            else:
                return transceiver_dom_threshold_info_dict

            # Threshold Data
            transceiver_dom_threshold_info_dict['temphighalarm'] = dom_module_threshold_data['data']['TempHighAlarm']['value']
            transceiver_dom_threshold_info_dict['templowalarm'] = dom_module_threshold_data['data']['TempLowAlarm']['value']
            transceiver_dom_threshold_info_dict['temphighwarning'] = dom_module_threshold_data['data']['TempHighWarning']['value']
            transceiver_dom_threshold_info_dict['templowwarning'] = dom_module_threshold_data['data']['TempLowWarning']['value']
            transceiver_dom_threshold_info_dict['vcchighalarm'] = dom_module_threshold_data['data']['VoltageHighAlarm']['value']
            transceiver_dom_threshold_info_dict['vcclowalarm'] = dom_module_threshold_data['data']['VoltageLowAlarm']['value']
            transceiver_dom_threshold_info_dict['vcchighwarning'] = dom_module_threshold_data[
                'data']['VoltageHighWarning']['value']
            transceiver_dom_threshold_info_dict['vcclowwarning'] = dom_module_threshold_data['data']['VoltageLowWarning']['value']
            transceiver_dom_threshold_info_dict['txbiashighalarm'] = dom_module_threshold_data['data']['BiasHighAlarm']['value']
            transceiver_dom_threshold_info_dict['txbiaslowalarm'] = dom_module_threshold_data['data']['BiasLowAlarm']['value']
            transceiver_dom_threshold_info_dict['txbiashighwarning'] = dom_module_threshold_data['data']['BiasHighWarning']['value']
            transceiver_dom_threshold_info_dict['txbiaslowwarning'] = dom_module_threshold_data['data']['BiasLowWarning']['value']
            transceiver_dom_threshold_info_dict['txpowerhighalarm'] = dom_module_threshold_data['data']['TXPowerHighAlarm']['value']
            transceiver_dom_threshold_info_dict['txpowerlowalarm'] = dom_module_threshold_data['data']['TXPowerLowAlarm']['value']
            transceiver_dom_threshold_info_dict['txpowerhighwarning'] = dom_module_threshold_data['data']['TXPowerHighWarning']['value']
            transceiver_dom_threshold_info_dict['txpowerlowwarning'] = dom_module_threshold_data['data']['TXPowerLowWarning']['value']
            transceiver_dom_threshold_info_dict['rxpowerhighalarm'] = dom_module_threshold_data['data']['RXPowerHighAlarm']['value']
            transceiver_dom_threshold_info_dict['rxpowerlowalarm'] = dom_module_threshold_data['data']['RXPowerLowAlarm']['value']
            transceiver_dom_threshold_info_dict['rxpowerhighwarning'] = dom_module_threshold_data['data']['RXPowerHighWarning']['value']
            transceiver_dom_threshold_info_dict['rxpowerlowwarning'] = dom_module_threshold_data['data']['RXPowerLowWarning']['value']

        return transceiver_dom_threshold_info_dict

    def get_temperature(self):
        """
        Retrieves the temperature of this SFP

        Returns:
            An integer number of current temperature in Celsius
        """
        transceiver_bulk_status = self.get_transceiver_bulk_status()
        if transceiver_bulk_status is not None:
            return transceiver_bulk_status.get("temperature", "N/A")
        else:
            return None

    def get_voltage(self):
        """
        Retrieves the supply voltage of this SFP

        Returns:
            An integer number of supply voltage in mV
        """
        transceiver_bulk_status = self.get_transceiver_bulk_status()
        if transceiver_bulk_status is not None:
            return transceiver_bulk_status.get("voltage", "N/A")
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

        if self.port_num < PORT_START or self.port_num > PORT_END:
            return None
        tx_bias_list = []
        if self.sfp_type == OSFP_TYPE:
            # OSFP not supported on our platform yet.
            return None

        elif self.sfp_type == QSFP_TYPE:
            offset = 0
            sfpd_obj = sff8436Dom()
            if sfpd_obj is None:
                return None

            if self.dom_rx_power_supported:
                dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                    (offset + QSFP_CHANNL_MON_OFFSET), QSFP_CHANNL_MON_WITH_TX_POWER_WIDTH)
                if dom_channel_monitor_raw is not None:
                    dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params_with_tx_power(
                        dom_channel_monitor_raw, 0)
                    tx_bias_list.append(self.__convert_string_to_num(
                        dom_channel_monitor_data['data']['TX1Bias']['value']))
                    tx_bias_list.append(self.__convert_string_to_num(
                        dom_channel_monitor_data['data']['TX2Bias']['value']))
                    tx_bias_list.append(self.__convert_string_to_num(
                        dom_channel_monitor_data['data']['TX3Bias']['value']))
                    tx_bias_list.append(self.__convert_string_to_num(
                        dom_channel_monitor_data['data']['TX4Bias']['value']))
                else:
                    return None
            else:
                return None
        else:
            offset = 256

            sfpd_obj = sff8472Dom()
            if sfpd_obj is None:
                return None

            if self.dom_supported:
                sfpd_obj._calibration_type = self.calibration

                dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                    (offset + SFP_CHANNL_MON_OFFSET), SFP_CHANNL_MON_WIDTH)
                if dom_channel_monitor_raw is not None:
                    dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params(
                        dom_channel_monitor_raw, 0)
                    tx_bias_list.append(self.__convert_string_to_num(
                        dom_channel_monitor_data['data']['TXBias']['value']))
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
        if self.port_num < PORT_START or self.port_num > PORT_END:
            return None
        rx_power_list = []
        if self.sfp_type == OSFP_TYPE:
            # OSFP not supported on our platform yet.
            return None

        elif self.sfp_type == QSFP_TYPE:
            offset = 0
            sfpd_obj = sff8436Dom()
            if sfpd_obj is None:
                return None

            if self.dom_rx_power_supported:
                dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                    (offset + QSFP_CHANNL_MON_OFFSET), QSFP_CHANNL_MON_WITH_TX_POWER_WIDTH)
                if dom_channel_monitor_raw is not None:
                    dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params_with_tx_power(
                        dom_channel_monitor_raw, 0)
                    rx_power_list.append(self.__convert_string_to_num(
                        dom_channel_monitor_data['data']['RX1Power']['value']))
                    rx_power_list.append(self.__convert_string_to_num(
                        dom_channel_monitor_data['data']['RX2Power']['value']))
                    rx_power_list.append(self.__convert_string_to_num(
                        dom_channel_monitor_data['data']['RX3Power']['value']))
                    rx_power_list.append(self.__convert_string_to_num(
                        dom_channel_monitor_data['data']['RX4Power']['value']))
                else:
                    return None
            else:
                return None
        else:
            offset = 256

            sfpd_obj = sff8472Dom()
            if sfpd_obj is None:
                return None

            if self.dom_supported:
                sfpd_obj._calibration_type = self.calibration

                dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                    (offset + SFP_CHANNL_MON_OFFSET), SFP_CHANNL_MON_WIDTH)
                if dom_channel_monitor_raw is not None:
                    dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params(
                        dom_channel_monitor_raw, 0)
                    rx_power_list.append(self.__convert_string_to_num(
                        dom_channel_monitor_data['data']['RXPower']['value']))
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
        if self.port_num < PORT_START or self.port_num > PORT_END:
            return None
        tx_power_list = []
        if self.sfp_type == OSFP_TYPE:
            # OSFP not supported on our platform yet.
            return None

        elif self.sfp_type == QSFP_TYPE:
            offset = 0
            sfpd_obj = sff8436Dom()
            if sfpd_obj is None:
                return None

            if self.dom_tx_power_supported:
                dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                    (offset + QSFP_CHANNL_MON_OFFSET), QSFP_CHANNL_MON_WITH_TX_POWER_WIDTH)
                if dom_channel_monitor_raw is not None:
                    dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params_with_tx_power(
                        dom_channel_monitor_raw, 0)
                    tx_power_list.append(self.__convert_string_to_num(
                        dom_channel_monitor_data['data']['TX1Power']['value']))
                    tx_power_list.append(self.__convert_string_to_num(
                        dom_channel_monitor_data['data']['TX2Power']['value']))
                    tx_power_list.append(self.__convert_string_to_num(
                        dom_channel_monitor_data['data']['TX3Power']['value']))
                    tx_power_list.append(self.__convert_string_to_num(
                        dom_channel_monitor_data['data']['TX4Power']['value']))
                else:
                    return None
            else:
                return None
        else:
            offset = 256
            sfpd_obj = sff8472Dom()
            if sfpd_obj is None:
                return None

            if self.dom_supported:
                sfpd_obj._calibration_type = self.calibration

                dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                    (offset + SFP_CHANNL_MON_OFFSET), SFP_CHANNL_MON_WIDTH)
                if dom_channel_monitor_raw is not None:
                    dom_channel_monitor_data = sfpd_obj.parse_channel_monitor_params(
                        dom_channel_monitor_raw, 0)
                    tx_power_list.append(self.__convert_string_to_num(
                        dom_channel_monitor_data['data']['TXPower']['value']))
                else:
                    return None
            else:
                return None

        return tx_power_list

    def get_rx_los(self):
        """
        Retrieves the RX LOS (lost-of-signal) status of SFP
        Returns:
            A Boolean, True if SFP has RX LOS, False if not.
            Note : RX LOS status is latched until a call to get_rx_los or a reset.
        """
        if self.port_num < PORT_START or self.port_num > PORT_END:
            return None
        if not self.dom_supported:
            return None

        rx_los_list = []
        if self.sfp_type == OSFP_TYPE:
            return None
        elif self.sfp_type == QSFP_TYPE:
            offset = 0
            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                (offset + QSFP_CHANNL_RX_LOS_STATUS_OFFSET), QSFP_CHANNL_RX_LOS_STATUS_WIDTH)
            if dom_channel_monitor_raw is not None:
                rx_los_data = int(dom_channel_monitor_raw[0], 16)
                rx_los_list.append(rx_los_data & 0x01 != 0)
                rx_los_list.append(rx_los_data & 0x02 != 0)
                rx_los_list.append(rx_los_data & 0x04 != 0)
                rx_los_list.append(rx_los_data & 0x08 != 0)
        else:
            offset = 256
            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                (offset + SFP_CHANNL_STATUS_OFFSET), SFP_CHANNL_STATUS_WIDTH)
            if dom_channel_monitor_raw is not None:
                rx_los_data = int(dom_channel_monitor_raw[0], 16)
                rx_los_list.append(rx_los_data & 0x02 != 0)
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
        if self.port_num < PORT_START or self.port_num > PORT_END:
            return None
        if not self.dom_supported:
            return None
        tx_fault_list = []
        if self.sfp_type == OSFP_TYPE:
            return None
        elif self.sfp_type == QSFP_TYPE:
            offset = 0
            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                (offset + QSFP_CHANNL_TX_FAULT_STATUS_OFFSET), QSFP_CHANNL_TX_FAULT_STATUS_WIDTH)
            if dom_channel_monitor_raw is not None:
                tx_fault_data = int(dom_channel_monitor_raw[0], 16)
                tx_fault_list.append(tx_fault_data & 0x01 != 0)
                tx_fault_list.append(tx_fault_data & 0x02 != 0)
                tx_fault_list.append(tx_fault_data & 0x04 != 0)
                tx_fault_list.append(tx_fault_data & 0x08 != 0)
        else:
            offset = 256
            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                (offset + SFP_CHANNL_STATUS_OFFSET), SFP_CHANNL_STATUS_WIDTH)
            if dom_channel_monitor_raw is not None:
                tx_fault_data = int(dom_channel_monitor_raw[0], 16)
                tx_fault_list.append(tx_fault_data & 0x04 != 0)
            else:
                return None

        return tx_fault_list

    def get_tx_disable(self):
        """
        Retrieves the tx_disable status of this SFP

        Returns:
            A Boolean, True if tx_disable is enabled, False if disabled
        """
        # Check for invalid self.port_num
        if self.port_num < PORT_START or self.port_num > PORT_END:
            return None
        if not self.dom_supported:
            return None

        tx_disable_list = []

        if self.sfp_type == OSFP_TYPE:
            return None
        elif self.sfp_type == QSFP_TYPE:
            offset = 0
            dom_channel_monitor_raw = self.__read_eeprom_specific_bytes(
                (offset + QSFP_CHANNL_DISABLE_STATUS_OFFSET), QSFP_CHANNL_DISABLE_STATUS_WIDTH)
            if dom_channel_monitor_raw is not None:
                tx_disable_data = int(dom_channel_monitor_raw[0], 16)
                tx_disable_list.append(tx_disable_data & 0x01 != 0)
                tx_disable_list.append(tx_disable_data & 0x02 != 0)
                tx_disable_list.append(tx_disable_data & 0x04 != 0)
                tx_disable_list.append(tx_disable_data & 0x08 != 0)
        else:
            if self.port_num <= 7:
                txdis_path = "/sys/bus/i2c/devices/1-0034/sfp_txdis1"
            elif self.port_num >= 8 and self.port_num <= 15:
                txdis_path = "/sys/bus/i2c/devices/1-0034/sfp_txdis2"
            elif self.port_num >= 16 and self.port_num <= 23:
                txdis_path = "/sys/bus/i2c/devices/1-0034/sfp_txdis3"
            elif self.port_num >= 24 and self.port_num <= 31:
                txdis_path = "/sys/bus/i2c/devices/1-0036/sfp_txdis4"
            elif self.port_num >= 32 and self.port_num <= 39:
                txdis_path = "/sys/bus/i2c/devices/1-0036/sfp_txdis5"
            elif self.port_num >= 40 and self.port_num <= 47:
                txdis_path = "/sys/bus/i2c/devices/1-0036/sfp_txdis6"
            else:
                return None
            try:
                data = open(txdis_path, "rb")
            except IOError:
                return None

            txdis_data = data.read(2)
            if txdis_data == "":
                return None
            result = int(txdis_data, 16)
            data.close()

            # ModPrsL is active low
            tx_disable_list.append(result & (1 << (self.port_num % 8)) != 0)

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
        '''
        Not support LPMode pin to control lpmde.
        This function is affected by the  Power_over-ride and Power_set software control bits (byte 93 bits 0,1)
        '''
        if self.port_num < PORT_START or self.port_num > PORT_END:
            return False
        if self.sfp_type == QSFP_TYPE:
            offset = 0
            sfpd_obj = sff8436Dom()
            if sfpd_obj is None:
                return False

            dom_control_raw = self.__read_eeprom_specific_bytes(
                offset + QSFP_CONTROL_OFFSET, QSFP_CONTROL_WIDTH) if self.get_presence() else None
            if dom_control_raw is not None:
                dom_control_data = sfpd_obj.parse_control_bytes(dom_control_raw, 0)
                lpmode = ('On' == dom_control_data['data']['PowerSet']['value'])
                power_override = ('On' == dom_control_data['data']['PowerOverride']['value'])
                if lpmode == power_override == True:
                    return True
        elif self.sfp_type == SFP_TYPE:
            # SFP doesn't support this feature
            return False
        return False

    def get_power_override(self):
        """
        Retrieves the power-override status of this SFP

        Returns:
            A Boolean, True if power-override is enabled, False if disabled
        """
        if self.port_num < PORT_START or self.port_num > PORT_END:
            return False
        if self.sfp_type == QSFP_TYPE:
            offset = 0
            sfpd_obj = sff8436Dom()
            if sfpd_obj is None:
                return False

            dom_control_raw = self.__read_eeprom_specific_bytes(
                offset + QSFP_CONTROL_OFFSET, QSFP_CONTROL_WIDTH) if self.get_presence() else None
            if dom_control_raw is not None:
                dom_control_data = sfpd_obj.parse_control_bytes(dom_control_raw, 0)
                power_override = ('On' == dom_control_data['data']['PowerOverride']['value'])
                return power_override
        elif self.sfp_type == SFP_TYPE:
            # SFP doesn't support this feature
            return False
        return False

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP

        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        if self.port_num < PORT_START or self.port_num > PORT_END:
            return False
        if self.sfp_type == QSFP_TYPE:
            if self.port_num >= 48 and self.port_num <= 55:
                reset_path = "/sys/bus/i2c/devices/1-0036/qsfp_reset"
            else:
                return False
            try:
                data = open(reset_path, "rb")
            except IOError:
                return None

            reset_data = data.read(2)
            if reset_data == "":
                return None
            result = int(reset_data, 16)
            data.close()
            reset_status = result & (1 << (self.port_num % 8)) == 0
            return reset_status

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
        if self.port_num < PORT_START or self.port_num > PORT_END:
            return False
        if self.sfp_type == QSFP_TYPE:
            offset = 0
            sysfsfile_eeprom = None
            try:
                tx_disable_ctl = 0xf if tx_disable else 0x0
                buffer = create_string_buffer(1)
                buffer[0] = chr(tx_disable_ctl)
                # Write to eeprom
                sysfs_sfp_i2c_client_eeprom_path = self.__get_port_eeprom_path(self.port_num, IDENTITY_EEPROM_ADDR)
                sysfsfile_eeprom = open(sysfs_sfp_i2c_client_eeprom_path, "r+b")
                sysfsfile_eeprom.seek(offset + QSFP_CONTROL_OFFSET)
                sysfsfile_eeprom.write(buffer[0])
            except IOError as e:
                print("Error: unable to open file: %s" % str(e))
                return False
            finally:
                if sysfsfile_eeprom is not None:
                    sysfsfile_eeprom.close()
                    time.sleep(0.01)
            return True
        elif self.sfp_type == SFP_TYPE:
            if self.port_num <= 7:
                txdis_path = "/sys/bus/i2c/devices/1-0034/sfp_txdis1"
            elif self.port_num >= 8 and self.port_num <= 15:
                txdis_path = "/sys/bus/i2c/devices/1-0034/sfp_txdis2"
            elif self.port_num >= 16 and self.port_num <= 23:
                txdis_path = "/sys/bus/i2c/devices/1-0034/sfp_txdis3"
            elif self.port_num >= 24 and self.port_num <= 31:
                txdis_path = "/sys/bus/i2c/devices/1-0036/sfp_txdis4"
            elif self.port_num >= 32 and self.port_num <= 39:
                txdis_path = "/sys/bus/i2c/devices/1-0036/sfp_txdis5"
            elif self.port_num >= 40 and self.port_num <= 47:
                txdis_path = "/sys/bus/i2c/devices/1-0036/sfp_txdis6"
            else:
                return False
            try:
                data = open(txdis_path, "r+")
                txdis_data = data.read(2)
                if txdis_data == "":
                    return False
                result = int(txdis_data, 16)
                if tx_disable:
                    result = result | (1 << (self.port_num % 8))
                else:
                    result = result & (~(1 << (self.port_num % 8)))
                data.seek(0)
                sres = hex(result)[2:]
                data.write(sres)
                data.close()
            except IOError as e:
                print("Error: unable to open file: %s" % str(e))
                return False

            return True
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
        if self.port_num < PORT_START or self.port_num > PORT_END:
            return False
        if self.sfp_type == QSFP_TYPE:
            offset = 0
            if 0 <= channel <= 15:
                try:
                    channel_state = self.get_tx_disable_channel()
                    if disable:
                        tx_disable_ctl = channel_state | channel
                    else:
                        tx_disable_ctl = channel_state & (~channel)
                    buffer = create_string_buffer(1)
                    buffer[0] = chr(tx_disable_ctl)
                    # Write to eeprom
                    sysfs_sfp_i2c_client_eeprom_path = self.__get_port_eeprom_path(self.port_num, IDENTITY_EEPROM_ADDR)
                    sysfsfile_eeprom = open(sysfs_sfp_i2c_client_eeprom_path, "r+b")
                    sysfsfile_eeprom.seek(offset + QSFP_CONTROL_OFFSET)
                    sysfsfile_eeprom.write(buffer[0])
                finally:
                    if sysfsfile_eeprom is not None:
                        sysfsfile_eeprom.close()
                        time.sleep(0.01)
                return True
        elif self.sfp_type == SFP_TYPE:
            # SFP doesn't support this feature
            return False
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
        '''
        Not support LPMode pin to control lpmde.
        This function is affected by the  Power_over-ride and Power_set software control bits (byte 93 bits 0,1)
        '''
        if lpmode:
            return self.set_power_override(True, lpmode)
        else:
            return self.set_power_override(False, lpmode)

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
        if self.port_num < PORT_START or self.port_num > PORT_END:
            return False
        if self.sfp_type == QSFP_TYPE:
            offset = 0
            try:
                power_override_bit = 0
                if power_override:
                    power_override_bit |= 1 << 0

                power_set_bit = 0
                if power_set:
                    power_set_bit |= 1 << 1

                buffer = create_string_buffer(1)
                buffer[0] = chr(power_override_bit | power_set_bit)
                # Write to eeprom
                sysfs_sfp_i2c_client_eeprom_path = self.__get_port_eeprom_path(self.port_num, IDENTITY_EEPROM_ADDR)
                sysfsfile_eeprom = open(sysfs_sfp_i2c_client_eeprom_path, "r+b")
                sysfsfile_eeprom.seek(offset + QSFP_POWERMODE_OFFSET)
                sysfsfile_eeprom.write(buffer[0])
            except IOError as e:
                print("Error: unable to open file: %s" % str(e))
                return False
            finally:
                if sysfsfile_eeprom is not None:
                    sysfsfile_eeprom.close()
                    time.sleep(0.01)
            return True
        elif self.sfp_type == SFP_TYPE:
            # SFP doesn't support this feature
            return False
        return False

    def reset(self, reset):
        """
        Reset SFP and return all user module settings to their default srate.

        Returns:
            A boolean, True if successful, False if not
        """
        if self.port_num < PORT_START or self.port_num > PORT_END:
            return False
        if self.sfp_type == QSFP_TYPE:
            if self.port_num >= 48 and self.port_num <= 55:
                reset_path = "/sys/bus/i2c/devices/1-0036/qsfp_reset"
            else:
                return False
            try:
                data = open(reset_path, "r+")
                reset_data = data.read(2)
                if reset_data == "":
                    return False
                result = int(reset_data, 16)
                if reset:
                    result = result & (~(1 << (self.port_num % 8)))
                else:
                    result = result | (1 << (self.port_num % 8))
                data.seek(0)
                sres = hex(result)[2:]
                data.write(sres)
                data.close()
            except IOError as e:
                print("Error: unable to open file: %s" % str(e))
                return False

            return True
        elif self.sfp_type == SFP_TYPE:
            # SFP doesn't support this feature
            return False
        return False

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
        raise NotImplementedError

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
        raise NotImplementedError

